#include "Course.h"
#include <sstream>
#include <fstream>

// ═══════════════════════════════════════════════════════════════════════
//  COURSE.CPP
//  Implements the Course class — a single course offering loaded from
//  the school's CSV. Now reads all 8 columns from the CSV including
//  meeting days, start/end times, and an optional room number.
//  These are passed to the Section constructor so every Course carries
//  its full scheduling details alongside its catalog data.
// ═══════════════════════════════════════════════════════════════════════

// ───────────────────────────────────────────────────────────────────────
//  CONSTRUCTORS
// ───────────────────────────────────────────────────────────────────────

// Default constructor.
// Initializes an empty course with safe default values.
// Used as a fallback when fromCSV() encounters a malformed line.
Course::Course()
	: crn(""), subject(""), code(""), name(""), creditHours(0),
	  courseCode(""), section() {
}

// Parameterized constructor.
// Builds a course from its component parts including a fully-built Section.
// courseCode is computed automatically from subject + code
// so it never gets out of sync with either field.
Course::Course(const std::string& crn, const std::string& subject,
	const std::string& code, const std::string& name,
	int creditHours, const Section& section)
	: crn(crn), subject(subject), code(code), name(name),
	  creditHours(creditHours), courseCode(subject + code),
	  section(section) {
}

// ───────────────────────────────────────────────────────────────────────
//  GETTERS
// ───────────────────────────────────────────────────────────────────────

std::string Course::getCrn()         const { return crn; }
std::string Course::getSubject()     const { return subject; }
std::string Course::getCode()        const { return code; }
std::string Course::getName()        const { return name; }
int         Course::getCreditHours() const { return creditHours; }
std::string Course::getCourseCode()  const { return courseCode; }

// Returns the Section object by const reference.
// Avoids copying the vector of meeting days on every call.
const Section& Course::getSection() const {
	return section;
}

// ───────────────────────────────────────────────────────────────────────
//  CSV LOADING
// ───────────────────────────────────────────────────────────────────────

// Parses one CSV line and returns a Course object.
// Now reads up to 9 columns. The first 5 are catalog data; cols 5–7
// are scheduling data passed to Section; col 8 is optional room number.
//
// Expected CSV column layout:
//   0: crn          e.g. "11543"
//   1: subject      e.g. "CMPS"
//   2: code         e.g. "1044"
//   3: name         e.g. "Computer Science I"
//   4: creditHours  e.g. "4"
//   5: days         e.g. "MWF"
//   6: startTime    e.g. "0800am"
//   7: endTime      e.g. "0920am"
//   8: roomNumber   e.g. "Bolin 101"  (optional)
//   9: professor    e.g. "Dr. Smith"  (optional)
//
// Returns an empty Course if the line has fewer than 5 fields.
Course Course::fromCSV(const std::string& csvLine) {
	std::stringstream ss(csvLine);
	std::string token;
	std::vector<std::string> fields;

	while (std::getline(ss, token, ',')) {
		// Trim leading/trailing whitespace from each field.
		size_t start = token.find_first_not_of(" \t\r\n");
		size_t end   = token.find_last_not_of(" \t\r\n");
		if (start != std::string::npos)
			fields.push_back(token.substr(start, end - start + 1));
		else
			fields.push_back("");
	}

	// Safety guard — need at least catalog columns 0–4.
	if (fields.size() < 5) return Course();

	std::string crn      = fields[0];
	std::string subject  = fields[1];
	std::string code     = fields[2];
	std::string name     = fields[3];
	int creditHours      = 0;
	try { creditHours = std::stoi(fields[4]); } catch (...) {}

	// Read scheduling columns — default to empty string if absent.
	std::string rawDays  = fields.size() > 5 ? fields[5] : "";
	std::string rawStart = fields.size() > 6 ? fields[6] : "";
	std::string rawEnd   = fields.size() > 7 ? fields[7] : "";
	std::string room     = fields.size() > 8 ? fields[8] : "";
	std::string prof     = fields.size() > 9 ? fields[9] : "";

	Section sec(prof, rawDays, rawStart, rawEnd, room);
	return Course(crn, subject, code, name, creditHours, sec);
}

// Reads the entire courses CSV file and returns a vector of all courses.
// Opens the file with ifstream, reads line by line, calls fromCSV()
// on each non-empty line, and collects results into a vector.
// Called once at server startup — catalog is loaded into memory
// and reused for all requests, never re-read from disk.
//
// Note: if your CSV has a header row, add a single getline() call
// before the loop to skip it.
std::vector<Course> Course::loadFromFile(const std::string& filename) {
	std::vector<Course> courses;
	std::ifstream infile(filename);
	std::string line;

	while (std::getline(infile, line)) {
		if (!line.empty()) {
			courses.push_back(fromCSV(line));
		}
	}

	return courses;
}

// ───────────────────────────────────────────────────────────────────────
//  SERIALIZATION
// ───────────────────────────────────────────────────────────────────────

// Converts this Course to a JSON string for the frontend.
// The Section object is serialized inline as a nested "section" key.
// This means every GET /courses and every AcademicRecord response
// automatically includes the full scheduling details.
//
// Example output:
// {
//   "crn":         "11543",
//   "subject":     "CMPS",
//   "code":        "1044",
//   "name":        "Computer Science I",
//   "creditHours": 4,
//   "courseCode":  "CMPS1044",
//   "section": {
//     "professor":   "Dr. Smith",
//     "meetingDays": ["Mon","Wed","Fri"],
//     "startTime":   "8:00 AM",
//     "endTime":     "9:20 AM",
//     "roomNumber":  "Bolin 101"
//   }
// }
std::string Course::toJson() const {
	std::string json = "{";
	json += "\"crn\":\"" + crn + "\",";
	json += "\"subject\":\"" + subject + "\",";
	json += "\"code\":\"" + code + "\",";
	json += "\"name\":\"" + name + "\",";
	json += "\"creditHours\":" + std::to_string(creditHours) + ",";
	json += "\"courseCode\":\"" + courseCode + "\",";

	// Nest the full section object — no trailing comma after this.
	json += "\"section\":" + section.toJson();

	json += "}";
	return json;
}
