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

// ═══════════════════════════════════════════════════════════════════════
//  SERVER.CPP
//  The entry point of the application. Loads the course catalog from
//  CSV, initializes all core objects, starts the cpp-httplib server on
//  localhost:8080, and defines every HTTP route the frontend calls.
//  All data lives in memory — no database. Student data is persisted
//  to a .txt file on request. The server runs until the process ends.
// ═══════════════════════════════════════════════════════════════════════

// ───────────────────────────────────────────────────────────────────────
//  HELPER — PARSE JSON FIELD
//  cpp-httplib has no built-in JSON parser. This helper extracts a
//  single string value from a flat JSON body without a library.
//  Only works for simple flat JSON — not nested objects or arrays.
//  Example: extractJson("{\"name\":\"John\"}", "name") → "John"
// ───────────────────────────────────────────────────────────────────────
std::string extractJson(const std::string& body, const std::string& key) {
	// Build the search pattern: "key":"
	std::string search = "\"" + key + "\":\"";
	size_t start = body.find(search);

	// Key not found — return empty string.
	if (start == std::string::npos) return "";

	// Move past the search pattern to the value.
	start += search.length();

	// Find the closing quote of the value.
	size_t end = body.find("\"", start);
	if (end == std::string::npos) return "";

	return body.substr(start, end - start);
}

// Extracts a numeric value from a JSON body.
// Example: extractJsonInt("{\"credits\":3}", "credits") → 3
int extractJsonInt(const std::string& body, const std::string& key) {
	std::string search = "\"" + key + "\":";
	size_t start = body.find(search);
	if (start == std::string::npos) return 0;
	start += search.length();
	size_t end = body.find_first_of(",}", start);
	if (end == std::string::npos) return 0;
	try {
		return std::stoi(body.substr(start, end - start));
	}
	catch (...) {
		return 0;
	}
}

// ───────────────────────────────────────────────────────────────────────
//  HELPER — SAVE STUDENT TO FILE
//  Writes the student's current data to a human-readable .txt file.
//  Called when the frontend requests a save. Not automatic — student
//  must trigger it. Overwrites the file on each save.
// ───────────────────────────────────────────────────────────────────────
void saveStudentToFile(const Student& student) {
	std::ofstream outfile("student_data.txt");
	if (!outfile.is_open()) return;

	outfile << "=== Student Academic Record ===" << std::endl;
	outfile << "Name:          " << student.getName() << std::endl;
	outfile << "Major:         " << student.getMajor() << std::endl;
	outfile << "GPA:           " << student.getGpa() << std::endl;
	outfile << "Credit Hours:  " << student.getTotalCreditHours() << std::endl;
	outfile << std::endl;
	outfile << "--- Completed Courses ---" << std::endl;

	for (const AcademicRecord& record : student.getRecords()) {
		outfile << record.getCourseCode() << "  "
			<< record.getCourse().getName() << "  "
			<< record.getGrade() << "  "
			<< record.getCreditHours() << " hrs  "
			<< (record.isCompleted() ? "Completed" : "In Progress")
			<< std::endl;
	}

	outfile.close();
}

// ───────────────────────────────────────────────────────────────────────
//  HELPER — LOAD STUDENT FROM FILE
//  Checks if student_data.txt exists and reads basic info from it.
//  Returns true if student data was found and loaded, false if not.
//  If false — server tells frontend to prompt for first-time setup.
// ───────────────────────────────────────────────────────────────────────
bool loadStudentFromFile(Student& student) {
	std::ifstream infile("student_data.txt");
	if (!infile.is_open()) return false;

	std::string line;
	std::string name, major;

	while (std::getline(infile, line)) {
		if (line.substr(0, 6) == "Name: ") {
			name = line.substr(15);
		}
		else if (line.substr(0, 6) == "Major:") {
			major = line.substr(15);
		}
	}

	if (!name.empty() && !major.empty()) {
		student.setName(name);
		student.setMajor(major);
		return true;
	}

	return false;
}

// ───────────────────────────────────────────────────────────────────────
//  MAIN — SERVER ENTRY POINT
// ───────────────────────────────────────────────────────────────────────
int main() {
	// ── Initialize core objects ────────────────────────────────────────

	// Load course catalog from CSV into memory.
	// Done once at startup — never re-read from disk during runtime.
	std::vector<Course> catalog = Course::loadFromFile("courses.csv");

	// Build the CS degree plan — hardcoded rulebook.
	DegreePlan plan("Computer Science");

	// Create the student object — empty until loaded or first-time setup.
	Student student;

	// Create the degree audit — recalculated on every /audit request.
	DegreeAudit audit;

	// Try to load existing student data from file.
	bool studentLoaded = loadStudentFromFile(student);

	// ── Start HTTP server ──────────────────────────────────────────────
	httplib::Server svr;

	// Allow frontend to make requests from the browser.
	// Required for fetch() calls to localhost to work correctly.
	svr.set_default_headers({
		{"Access-Control-Allow-Origin",  "*"},
		{"Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS"},
		{"Access-Control-Allow-Headers", "Content-Type"}
		});

	// Handle preflight OPTIONS requests from the browser.
	svr.Options(".*", [](const httplib::Request&, httplib::Response& res) {
		res.status = 200;
		});

	// ── ROUTE: GET /status ─────────────────────────────────────────────
	// First thing the frontend checks on load.
	// Returns whether student data exists or if first-time setup needed.
	svr.Get("/status", [&](const httplib::Request&, httplib::Response& res) {
		std::string json = "{";
		json += "\"studentLoaded\":" + std::string(studentLoaded ? "true" : "false");
		json += "}";
		res.set_content(json, "application/json");
		});

	// ── ROUTE: GET /student ────────────────────────────────────────────
	// Returns the full student object as JSON.
	// Frontend calls this to populate the dashboard on load.
	svr.Get("/student", [&](const httplib::Request&, httplib::Response& res) {
		res.set_content(student.toJson(), "application/json");
		});

	// ── ROUTE: POST /student ───────────────────────────────────────────
	// Creates a new student on first launch.
	// Frontend sends: { "name": "John Smith", "major": "Computer Science" }
	// Sets studentLoaded to true so /status reflects the change.
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

			res.set_content(student.toJson(), "application/json");
		});

	// ── ROUTE: PUT /student ────────────────────────────────────────────
	// Updates student name or major.
	// Frontend sends: { "name": "...", "major": "..." }
	svr.Put("/student", [&](const httplib::Request& req,
		httplib::Response& res) {
			std::string name = extractJson(req.body, "name");
			std::string major = extractJson(req.body, "major");

			if (!name.empty())  student.setName(name);
			if (!major.empty()) student.setMajor(major);

			res.set_content(student.toJson(), "application/json");
		});

	// ── ROUTE: GET /courses ────────────────────────────────────────────
	// Returns the full course catalog loaded from CSV.
	// Frontend uses this to populate course selection dropdowns.
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
	// Runs the full degree audit and returns results as JSON.
	// Always recalculates fresh — never serves stale audit data.
	svr.Get("/audit", [&](const httplib::Request&, httplib::Response& res) {
		audit.run(student, plan);
		res.set_content(audit.toJson(), "application/json");
		});

	// ── ROUTE: POST /record ────────────────────────────────────────────
	// Adds a new academic record to the student's history.
	// Frontend sends:
	// { "courseCode": "CMPS1044", "grade": "A", "completed": true }
	// Looks up the course from the catalog by course code.
	svr.Post("/record", [&](const httplib::Request& req,
		httplib::Response& res) {
			std::string courseCode = extractJson(req.body, "courseCode");
			std::string grade = extractJson(req.body, "grade");
			std::string semester = extractJson(req.body, "semester");
			std::string completedStr = extractJson(req.body, "completed");
			bool completed = (completedStr == "true");

			// Find the course in the catalog by course code.
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

			// Build the record and add it to the student.
			AcademicRecord record(found, semester, grade, completed);
			student.addRecord(record);

			res.set_content(student.toJson(), "application/json");
		});

	// ── ROUTE: PUT /record ─────────────────────────────────────────────
	// Updates the grade for an existing record.
	// Frontend sends: { "courseCode": "CMPS1044", "grade": "B+" }
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
			res.set_content(student.toJson(), "application/json");
		});

	// ── ROUTE: DELETE /record ──────────────────────────────────────────
	// Removes an academic record by course code.
	// Frontend sends: { "courseCode": "CMPS1044" }
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
			res.set_content(student.toJson(), "application/json");
		});

	// ── ROUTE: POST /flex ──────────────────────────────────────────────
	// Assigns a completed course to a flex gen ed category.
	// Frontend sends: { "courseCode": "MUSC1033", "categoryCode": "050N" }
	// Returns updated audit result so frontend refreshes immediately.
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

			// Run audit first so flexProgress is populated before assignment.
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
	// Returns all current alerts as a JSON array.
	// Runs a fresh audit first so alerts are always current.
	svr.Get("/alerts", [&](const httplib::Request&, httplib::Response& res) {
		audit.run(student, plan);
		const std::vector<Alert>& alerts = audit.getAlerts();

		std::string json = "[";
		for (size_t i = 0; i < alerts.size(); i++) {
			json += alerts[i].toJson();
			if (i < alerts.size() - 1) json += ",";
		}
		json += "]";

		res.set_content(json, "application/json");
		});

	// ── ROUTE: POST /save ──────────────────────────────────────────────
	// Writes student data to student_data.txt.
	// Called when the student clicks save in the frontend.
	svr.Post("/save", [&](const httplib::Request&, httplib::Response& res) {
		saveStudentToFile(student);
		res.set_content("{\"saved\":true}", "application/json");
		});

	// ── Launch ─────────────────────────────────────────────────────────
	// Auto-open the browser so the user doesn't have to type the URL.
	// Windows only — uses the system() call to launch the default browser.
	svr.set_mount_point("/", "./temp");
	system("start http://localhost:8080");
	std::cout << "Server running at http://localhost:8080/  now running... Press Ctrl+C to stop." << std::endl;
	// Block here and serve requests until the process is killed.
	// All objects stay alive in memory for the lifetime of the server.
	svr.listen("localhost", 8080);

	return 0;
}