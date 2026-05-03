#define WIN32_LEAN_AND_MEAN
#include "httplib.h"
#include "Student.h"
#include "Course.h"
#include "DegreePlan.h"
#include "DegreeAudit.h"
#include "AcademicRecord.h"
#include "Alert.h"
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <set>

// ═══════════════════════════════════════════════════════════════════════
//  SERVER.CPP
//  Entry point of the application. Loads the course catalog from CSV,
//  initializes all core objects, starts the cpp-httplib server on
//  localhost:8080, and defines every HTTP route the frontend calls.
//  All data lives in memory — no database. Student data is persisted
//  to student_data.txt on save and reloaded automatically at startup.
// ═══════════════════════════════════════════════════════════════════════

// Global catalog pointer used by load helper to look up Course objects
// by course code when reconstructing AcademicRecords from disk.
static const std::vector<Course>* g_catalog = nullptr;

// ───────────────────────────────────────────────────────────────────────
//  HELPER — TRIM
//  Removes leading and trailing whitespace from a string in place.
//  Used by load helpers when parsing the pretty-text save file so a
//  value like "  Computer Science  " becomes "Computer Science".
// ───────────────────────────────────────────────────────────────────────
static std::string trim(const std::string& s) {
	size_t start = s.find_first_not_of(" \t\r\n");
	size_t end = s.find_last_not_of(" \t\r\n");
	if (start == std::string::npos) return "";
	return s.substr(start, end - start + 1);
}

// ───────────────────────────────────────────────────────────────────────
//  HELPER — TIME CONFLICT CHECK
//  Section::toJson() emits times as display strings like "1:00 PM" and
//  "2:50 PM". To compare them we parse back into total minutes since
//  midnight. timeToMinutes("1:00 PM") returns 13*60+0 = 780.
//
//  Returns -1 on parse failure so callers can skip the comparison
//  rather than treat 0 as a valid midnight time.
// ───────────────────────────────────────────────────────────────────────
static int timeToMinutes(const std::string& display) {
	// Expected format: "H:MM AM" or "HH:MM PM"
	if (display.size() < 6) return -1;

	size_t colon = display.find(':');
	if (colon == std::string::npos) return -1;

	int hour = 0, minute = 0;
	try {
		hour = std::stoi(display.substr(0, colon));
		minute = std::stoi(display.substr(colon + 1, 2));
	}
	catch (...) {
		return -1;
	}

	// Find AM/PM suffix — search from the end.
	bool isPM = (display.find("PM") != std::string::npos);
	bool isAM = (display.find("AM") != std::string::npos);
	if (!isPM && !isAM) return -1;

	// Convert to 24-hour clock.
	// 12 AM is midnight (0), 12 PM is noon (12), other PMs add 12.
	if (isPM && hour != 12) hour += 12;
	if (isAM && hour == 12) hour = 0;

	return hour * 60 + minute;
}

// Returns true if two sections meet on at least one shared day AND
// their meeting time windows overlap on that day.
//
// Two sections conflict when:
//   1. They share at least one day in their meetingDays lists, AND
//   2. Their start–end intervals on that day overlap
//
// Sections with no meeting days or no parseable times can never
// conflict — we conservatively return false rather than guess.
static bool sectionsConflict(const Section& a, const Section& b) {
	const auto& aDays = a.getMeetingDays();
	const auto& bDays = b.getMeetingDays();
	if (aDays.empty() || bDays.empty()) return false;

	// Find any shared day.
	bool sharedDay = false;
	for (const std::string& d : aDays) {
		for (const std::string& d2 : bDays) {
			if (d == d2) { sharedDay = true; break; }
		}
		if (sharedDay) break;
	}
	if (!sharedDay) return false;

	int aStart = timeToMinutes(a.getStartTime());
	int aEnd = timeToMinutes(a.getEndTime());
	int bStart = timeToMinutes(b.getStartTime());
	int bEnd = timeToMinutes(b.getEndTime());

	// If any time is unparseable we can't reliably detect overlap.
	if (aStart < 0 || aEnd < 0 || bStart < 0 || bEnd < 0) return false;

	// Two intervals [aStart,aEnd] and [bStart,bEnd] overlap iff
	// aStart < bEnd AND bStart < aEnd.
	return (aStart < bEnd) && (bStart < aEnd);
}

// ───────────────────────────────────────────────────────────────────────
//  HELPER — PARSE JSON FIELD
// ───────────────────────────────────────────────────────────────────────
std::string extractJson(const std::string& body, const std::string& key) {
	std::string search = "\"" + key + "\":\"";
	size_t start = body.find(search);
	if (start == std::string::npos) return "";
	start += search.length();
	size_t end = body.find("\"", start);
	if (end == std::string::npos) return "";
	return body.substr(start, end - start);
}

int extractJsonInt(const std::string& body, const std::string& key) {
	std::string search = "\"" + key + "\":";
	size_t start = body.find(search);
	if (start == std::string::npos) return 0;
	start += search.length();
	size_t end = body.find_first_of(",}", start);
	if (end == std::string::npos) return 0;
	try { return std::stoi(body.substr(start, end - start)); }
	catch (...) { return 0; }
}

// ───────────────────────────────────────────────────────────────────────
//  HELPER — SAVE STUDENT TO FILE
//  Writes the full student state to student_data.txt in a clean
//  human-readable format that loadStudentFromFile() parses back.
//
//  Format:
//    === Student Academic Record ===
//    Name: John Smith
//    Major: Computer Science
//    GPA: 3.45
//    Credit Hours: 16
//
//    --- Completed Courses ---
//    CMPS1044 | Computer Science I | A | 4 | Fall 2024 | Completed
//    CMPS1063 | Data Structures    | B+ | 3 | Spring 2025 | Completed
//
//  The pipe character '|' is used as a delimiter because course names
//  often contain commas, ampersands, and other punctuation that would
//  break a CSV-style parser. Pipes are very rare in course titles.
// ───────────────────────────────────────────────────────────────────────
void saveStudentToFile(const Student& student) {
	std::ofstream outfile("student_data.txt");
	if (!outfile.is_open()) return;

	outfile << "=== Student Academic Record ===" << std::endl;
	outfile << "Name: " << student.getName() << std::endl;
	outfile << "Major: " << student.getMajor() << std::endl;
	outfile << "GPA: " << student.getGpa() << std::endl;
	outfile << "Credit Hours: " << student.getTotalCreditHours() << std::endl;
	outfile << std::endl;
	outfile << "--- Completed Courses ---" << std::endl;

	// Each record on its own line, fields separated by " | ".
	// loadStudentFromFile() splits on " | " and rebuilds the record.
	for (const AcademicRecord& record : student.getRecords()) {
		outfile << record.getCourseCode() << " | "
			<< record.getCourse().getName() << " | "
			<< record.getGrade() << " | "
			<< record.getCreditHours() << " | "
			<< record.getSemester() << " | "
			<< (record.isCompleted() ? "Completed" : "In Progress")
			<< std::endl;
	}

	outfile.close();
}

// ───────────────────────────────────────────────────────────────────────
//  HELPER — LOAD STUDENT FROM FILE
//  Reads student_data.txt and restores the full student state.
//  Returns true if any data was loaded, false if the file doesn't exist.
//
//  Parser is line-based:
//    1. Lines starting with "Name: " or "Major: " set the student fields.
//    2. After the "--- Completed Courses ---" header, every non-empty
//       line is treated as a record: code | name | grade | credits |
//       semester | status.
//    3. Each course code is looked up in the global catalog so the
//       reconstructed AcademicRecord carries full Course details
//       (including the embedded Section).
//
//  Records that reference a course code not in the catalog are skipped.
//  This protects against stale save files where courses were renamed
//  or removed from the CSV.
// ───────────────────────────────────────────────────────────────────────
bool loadStudentFromFile(Student& student) {
	std::ifstream infile("student_data.txt");
	if (!infile.is_open()) {
		std::cout << "[load] student_data.txt not found" << std::endl;
		return false;
	}

	std::cout << "[load] reading student_data.txt..." << std::endl;

	std::string line;
	bool inRecordsSection = false;
	bool anythingLoaded = false;
	int  lineNum = 0;

	while (std::getline(infile, line)) {
		lineNum++;
		std::string trimmed = trim(line);
		if (trimmed.empty()) continue;

		// ── Section markers ────────────────────────────────────────────
		if (trimmed.find("--- Completed Courses ---") != std::string::npos) {
			inRecordsSection = true;
			std::cout << "[load] entered records section at line " << lineNum << std::endl;
			continue;
		}
		if (trimmed.find("=== Student Academic Record ===") != std::string::npos) {
			continue;
		}

		// ── Header fields (before the records section) ─────────────────
		if (!inRecordsSection) {
			if (trimmed.rfind("Name:", 0) == 0) {
				student.setName(trim(trimmed.substr(5)));
				std::cout << "[load] name set to '" << student.getName() << "'" << std::endl;
				anythingLoaded = true;
			}
			else if (trimmed.rfind("Major:", 0) == 0) {
				student.setMajor(trim(trimmed.substr(6)));
				std::cout << "[load] major set to '" << student.getMajor() << "'" << std::endl;
				anythingLoaded = true;
			}
			continue;
		}

		// ── Course record line ─────────────────────────────────────────
		std::cout << "[load] parsing record line " << lineNum << ": " << trimmed << std::endl;

		std::vector<std::string> parts;
		size_t pos = 0;
		while (true) {
			size_t next = trimmed.find(" | ", pos);
			if (next == std::string::npos) {
				parts.push_back(trim(trimmed.substr(pos)));
				break;
			}
			parts.push_back(trim(trimmed.substr(pos, next - pos)));
			pos = next + 3;
		}

		std::cout << "[load]   parsed " << parts.size() << " fields" << std::endl;
		if (parts.size() < 5) {
			std::cout << "[load]   SKIPPED — not enough fields" << std::endl;
			continue;
		}

		std::string code = parts[0];
		std::string grade = parts[2];
		std::string semester = parts[4];
		bool        completed = (parts.size() >= 6) ? (parts[5] == "Completed")
			: true;

		if (!g_catalog) {
			std::cout << "[load]   SKIPPED — catalog pointer null" << std::endl;
			continue;
		}

		Course found;
		bool   courseFound = false;
		for (const Course& c : *g_catalog) {
			if (c.getCourseCode() == code) {
				found = c;
				courseFound = true;
				break;
			}
		}
		if (!courseFound) {
			std::cout << "[load]   SKIPPED — course code '" << code << "' not in catalog" << std::endl;
			continue;
		}

		AcademicRecord record(found, semester, grade, completed);
		student.addRecord(record);
		std::cout << "[load]   ADDED " << code << " (" << grade << ")" << std::endl;
		anythingLoaded = true;
	}

	infile.close();
	return anythingLoaded;
}

// ───────────────────────────────────────────────────────────────────────
//  DISMISSED ALERTS
//  Each alert generated by DegreeAudit::run() is a fresh Alert object
//  built from current student state. There is no identity that survives
//  a re-run, so dismissals must be tracked separately by fingerprint.
//
//  A fingerprint is "type|courseCode|message" — type alone isn't unique
//  (multiple PREREQ_NOT_MET alerts at once), and courseCode can be
//  empty for category-level alerts so message is included as a tiebreak.
//
//  The set lives in memory and persists to dismissed_alerts.txt so
//  dismissals survive server restarts. One fingerprint per line.
// ───────────────────────────────────────────────────────────────────────
static std::set<std::string> g_dismissedAlerts;

// Converts an AlertType enum value to its string name.
// Mirrors Alert::alertTypeToString() which is private inside the class.
static std::string alertTypeName(AlertType t) {
	if (t == AlertType::GPA_WARNING)       return "GPA_WARNING";
	if (t == AlertType::PREREQ_NOT_MET)    return "PREREQ_NOT_MET";
	if (t == AlertType::MANDATORY_NOT_MET) return "MANDATORY_NOT_MET";
	if (t == AlertType::HOURS_NOT_MET)     return "HOURS_NOT_MET";
	if (t == AlertType::ELIGIBLE_COURSE)   return "ELIGIBLE_COURSE";
	if (t == AlertType::COURSE_REUSED)     return "COURSE_REUSED";
	return "UNKNOWN";
}

// Builds the fingerprint string used as the set key.
// Format: studentName|type|courseCode|message
//
// The student name is included so dismissals don't leak across
// students. If a future user named "Bob" fails the same prerequisite
// check that "Alex" already dismissed, Bob still sees the alert.
//
// Same format whether building from an Alert object server-side or
// from a JSON request body client-side — both paths must produce the
// exact same string for the dismissed-set check to match.
static std::string alertFingerprint(const std::string& studentName,
	const std::string& type,
	const std::string& courseCode,
	const std::string& message) {
	return studentName + "|" + type + "|" + courseCode + "|" + message;
}
static std::string alertFingerprint(const std::string& studentName,
	const Alert& a) {
	return alertFingerprint(studentName,
		alertTypeName(a.getType()),
		a.getCourseCode(),
		a.getMessage());
}

// Reads dismissed_alerts.txt at startup and populates the in-memory set.
// Missing file is fine — just means no alerts have been dismissed yet.
static void loadDismissedAlerts() {
	std::ifstream infile("dismissed_alerts.txt");
	if (!infile.is_open()) return;
	std::string line;
	while (std::getline(infile, line)) {
		std::string trimmed = trim(line);
		if (!trimmed.empty()) g_dismissedAlerts.insert(trimmed);
	}
}

// Writes the full dismissed set to disk after each addition.
// One fingerprint per line — easy to inspect and clear by hand.
static void saveDismissedAlerts() {
	std::ofstream outfile("dismissed_alerts.txt");
	if (!outfile.is_open()) return;
	for (const std::string& fp : g_dismissedAlerts) {
		outfile << fp << std::endl;
	}
}

// ───────────────────────────────────────────────────────────────────────
//  MAIN — SERVER ENTRY POINT
// ───────────────────────────────────────────────────────────────────────
int main() {
	// ── Initialize core objects ────────────────────────────────────────
	std::vector<Course> catalog = Course::loadFromFile("Schedule_with_Credits.csv");
	g_catalog = &catalog;

	DegreePlan  plan("Computer Science");
	Student     student;
	DegreeAudit audit;

	// Load student from disk if a save file exists.
	// loadStudentFromFile now restores name, major, AND all course
	// records — so refreshing the page after a server restart shows
	// the same data the user had last session.
	bool studentLoaded = loadStudentFromFile(student);

	// Load any previously dismissed alerts so they stay dismissed
	// across server restarts.
	loadDismissedAlerts();

	std::cout << "Catalog loaded: " << catalog.size() << " courses" << std::endl;
	std::cout << "Student loaded: " << (studentLoaded ? "yes" : "no") << std::endl;
	if (studentLoaded) {
		std::cout << "  Name: " << student.getName() << std::endl;
		std::cout << "  Major: " << student.getMajor() << std::endl;
		std::cout << "  Records: " << student.getRecords().size() << std::endl;
	}

	// ── Start HTTP server ──────────────────────────────────────────────
	httplib::Server svr;

	svr.set_default_headers({
		{"Access-Control-Allow-Origin",  "*"},
		{"Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS"},
		{"Access-Control-Allow-Headers", "Content-Type"}
		});

	svr.Options(".*", [](const httplib::Request&, httplib::Response& res) {
		res.status = 200;
		});

	// ── ROUTE: GET /status ─────────────────────────────────────────────
	svr.Get("/status", [&](const httplib::Request&, httplib::Response& res) {
		std::string json = "{";
		json += "\"studentLoaded\":" + std::string(studentLoaded ? "true" : "false");
		json += "}";
		res.set_content(json, "application/json");
		});

	// ── ROUTE: GET /student ────────────────────────────────────────────
	svr.Get("/student", [&](const httplib::Request&, httplib::Response& res) {
		res.set_content(student.toJson(), "application/json");
		});

	// ── ROUTE: POST /student ───────────────────────────────────────────
	// Creates a brand-new student record. This is the "first-time setup"
	// or "fresh start" flow — any state tied to a previous student
	// must be wiped here so the new student doesn't inherit it.
	svr.Post("/student", [&](const httplib::Request& req,
		httplib::Response& res) {
			std::string name = extractJson(req.body, "name");
			std::string major = extractJson(req.body, "major");

			if (name.empty() || major.empty()) {
				res.status = 400;
				res.set_content("{\"error\":\"Name and major are required.\"}",
					"application/json");
				return;
			}

			student.setName(name);
			student.setMajor(major);
			studentLoaded = true;

			// A new student starts with a clean alert slate. Without
			// this, alerts the previous student dismissed would stay
			// hidden for the new one — so they'd never see a real
			// "GPA below 2.0" or "Prereq not met" warning that they
			// happen to share with the old account.
			g_dismissedAlerts.clear();
			saveDismissedAlerts();

			// Auto-save so the new student persists across restarts
			// without the user having to click Save.
			saveStudentToFile(student);

			res.set_content(student.toJson(), "application/json");
		});

	// ── ROUTE: PUT /student ────────────────────────────────────────────
	svr.Put("/student", [&](const httplib::Request& req,
		httplib::Response& res) {
			std::string name = extractJson(req.body, "name");
			std::string major = extractJson(req.body, "major");

			if (!name.empty())  student.setName(name);
			if (!major.empty()) student.setMajor(major);

			saveStudentToFile(student);
			res.set_content(student.toJson(), "application/json");
		});

	// ── ROUTE: GET /courses ────────────────────────────────────────────
	svr.Get("/courses", [&](const httplib::Request&, httplib::Response& res) {
		std::string json = "[";
		for (size_t i = 0; i < catalog.size(); i++) {
			json += catalog[i].toJson();
			if (i < catalog.size() - 1) json += ",";
		}
		json += "]";
		res.set_content(json, "application/json");
		});

	// ── ROUTE: GET /audit ──────────────────────────────────────────────
	svr.Get("/audit", [&](const httplib::Request&, httplib::Response& res) {
		audit.run(student, plan);
		res.set_content(audit.toJson(), "application/json");
		});

	// ── ROUTE: POST /record ────────────────────────────────────────────
	svr.Post("/record", [&](const httplib::Request& req,
		httplib::Response& res) {
			std::string courseCode = extractJson(req.body, "courseCode");
			std::string grade = extractJson(req.body, "grade");
			std::string semester = extractJson(req.body, "semester");
			std::string completedStr = extractJson(req.body, "completed");
			bool completed = (completedStr == "true");

			Course found;
			bool courseFound = false;
			for (const Course& c : catalog) {
				if (c.getCourseCode() == courseCode) {
					found = c;
					courseFound = true;
					break;
				}
			}

			if (!courseFound) {
				res.status = 404;
				res.set_content("{\"error\":\"Course not found in catalog.\"}",
					"application/json");
				return;
			}

			// ── Time-conflict check ──────────────────────────────────────
			// Reject the registration if the new course's section
			// overlaps with any in-progress (non-completed) record on
			// the student's schedule. Completed past courses can't
			// conflict with anything new.
			if (!completed) {
				for (const AcademicRecord& existing : student.getRecords()) {
					if (existing.isCompleted()) continue;
					if (sectionsConflict(found.getSection(),
						existing.getCourse().getSection())) {
						res.status = 409; // 409 Conflict — semantically correct
						std::string msg =
							"{\"error\":\"Time conflict with " +
							existing.getCourseCode() +
							" (" + existing.getCourse().getName() + ").\"}";
						res.set_content(msg, "application/json");
						return;
					}
				}
			}

			AcademicRecord record(found, semester, grade, completed);
			student.addRecord(record);

			// Auto-save after every record change so progress persists.
			saveStudentToFile(student);

			res.set_content(student.toJson(), "application/json");
		});

	// ── ROUTE: PUT /record ─────────────────────────────────────────────
	svr.Put("/record", [&](const httplib::Request& req,
		httplib::Response& res) {
			std::string courseCode = extractJson(req.body, "courseCode");
			std::string newGrade = extractJson(req.body, "grade");

			if (courseCode.empty() || newGrade.empty()) {
				res.status = 400;
				res.set_content("{\"error\":\"courseCode and grade are required.\"}",
					"application/json");
				return;
			}

			student.updateRecord(courseCode, newGrade);
			saveStudentToFile(student);
			res.set_content(student.toJson(), "application/json");
		});

	// ── ROUTE: DELETE /record ──────────────────────────────────────────
	svr.Delete("/record", [&](const httplib::Request& req,
		httplib::Response& res) {
			std::string courseCode = extractJson(req.body, "courseCode");

			if (courseCode.empty()) {
				res.status = 400;
				res.set_content("{\"error\":\"courseCode is required.\"}",
					"application/json");
				return;
			}

			student.removeRecord(courseCode);
			saveStudentToFile(student);
			res.set_content(student.toJson(), "application/json");
		});

	// ── ROUTE: POST /flex ──────────────────────────────────────────────
	svr.Post("/flex", [&](const httplib::Request& req,
		httplib::Response& res) {
			std::string courseCode = extractJson(req.body, "courseCode");
			std::string categoryCode = extractJson(req.body, "categoryCode");

			if (courseCode.empty() || categoryCode.empty()) {
				res.status = 400;
				res.set_content("{\"error\":\"courseCode and categoryCode are required.\"}",
					"application/json");
				return;
			}

			audit.run(student, plan);
			bool success = audit.assignCourseToFlex(courseCode, categoryCode, student);

			if (!success) {
				res.status = 400;
				res.set_content("{\"error\":\"Assignment failed. Course may already be used.\"}",
					"application/json");
				return;
			}

			res.set_content(audit.toJson(), "application/json");
		});

	// ── ROUTE: GET /alerts ─────────────────────────────────────────────
	// Returns active alerts after filtering out any whose fingerprint
	// is in the dismissed set. This is what makes dismissals survive
	// re-runs of the audit — the alert objects get regenerated each
	// time but anything matching a stored fingerprint is hidden.
	svr.Get("/alerts", [&](const httplib::Request&, httplib::Response& res) {
		audit.run(student, plan);
		const std::vector<Alert>& alerts = audit.getAlerts();

		std::string json = "[";
		bool first = true;
		for (size_t i = 0; i < alerts.size(); i++) {
			if (g_dismissedAlerts.count(alertFingerprint(student.getName(), alerts[i]))) continue;
			if (!first) json += ",";
			json += alerts[i].toJson();
			first = false;
		}
		json += "]";

		res.set_content(json, "application/json");
		});

	// ── ROUTE: POST /alerts/dismiss ────────────────────────────────────
	// Adds an alert fingerprint to the dismissed set and persists.
	// Body must contain "type", "courseCode", and "message" — these
	// are what alerts.html sends from the alert object it received.
	svr.Post("/alerts/dismiss", [&](const httplib::Request& req,
		httplib::Response& res) {
			std::string type = extractJson(req.body, "type");
			std::string courseCode = extractJson(req.body, "courseCode");
			std::string message = extractJson(req.body, "message");

			if (type.empty() && message.empty()) {
				res.status = 400;
				res.set_content("{\"error\":\"type or message required\"}",
					"application/json");
				return;
			}

			std::string fp = alertFingerprint(student.getName(), type, courseCode, message);
			g_dismissedAlerts.insert(fp);
			saveDismissedAlerts();

			res.set_content("{\"dismissed\":true}", "application/json");
		});

	// ── ROUTE: POST /alerts/restore ────────────────────────────────────
	// Clears the dismissed set entirely. Useful for testing or letting
	// the user un-dismiss everything from a future "Show all" button.
	svr.Post("/alerts/restore", [&](const httplib::Request&,
		httplib::Response& res) {
			g_dismissedAlerts.clear();
			saveDismissedAlerts();
			res.set_content("{\"restored\":true}", "application/json");
		});

	// ── ROUTE: POST /save ──────────────────────────────────────────────
	svr.Post("/save", [&](const httplib::Request&, httplib::Response& res) {
		saveStudentToFile(student);
		res.set_content("{\"saved\":true}", "application/json");
		});

	// ── Launch ─────────────────────────────────────────────────────────
	svr.set_mount_point("/", "./temp");
	system("start http://localhost:8080/home.html");
	std::cout << "Server running at http://localhost:8080/  Press Ctrl+C to stop." << std::endl;
	svr.listen("localhost", 8080);

	return 0;
}