#include "Course.h"
#include <sstream>
#include <fstream>

// ═══════════════════════════════════════════════════════════════════════
//  COURSE.CPP
//  Implements the Course class — a single course offering loaded from
//  the school's CSV. Course is a pure data class — it holds exactly
//  what comes from the CSV file and nothing more. Prerequisite rules
//  and degree requirements are handled separately by DegreePlan, keeping
//  Course simple and focused on course catalog data only.
// ═══════════════════════════════════════════════════════════════════════

// ───────────────────────────────────────────────────────────────────────
//  CONSTRUCTORS
// ───────────────────────────────────────────────────────────────────────

// Default constructor.
// Initializes an empty course with safe default values.
// Used as a fallback when fromCSV() encounters a malformed line.
Course::Course()
	: crn(""), subject(""), code(""), name(""), creditHours(0), courseCode("") {
}

// Parameterized constructor.
// Builds a course from its component parts.
// courseCode is computed automatically from subject + code
// so it never gets out of sync with either field.
Course::Course(const std::string& crn, const std::string& subject,
	const std::string& code, const std::string& name,
	int creditHours)
	: crn(crn), subject(subject), code(code), name(name),
	creditHours(creditHours), courseCode(subject + code) {
}

// ───────────────────────────────────────────────────────────────────────
//  GETTERS
// ───────────────────────────────────────────────────────────────────────

// Returns the Course Registration Number for this section.
std::string Course::getCrn() const {
	return crn;
}

// Returns the department subject code (e.g. "CMPS").
std::string Course::getSubject() const {
	return subject;
}

// Returns the course number (e.g. "1044").
std::string Course::getCode() const {
	return code;
}

// Returns the full course title (e.g. "Computer Science I").
std::string Course::getName() const {
	return name;
}

// Returns the number of credit hours for this course.
// Used by AcademicRecord and DegreeAudit to compute GPA and degree progress.
// Added manually to the CSV before loading since it is not in the original file.
int Course::getCreditHours() const {
	return creditHours;
}

// Returns the combined course code (e.g. "CMPS1044").
// Built automatically from subject + code in the constructor.
// Used as the primary identifier throughout the entire system.
std::string Course::getCourseCode() const {
	return courseCode;
}

// ───────────────────────────────────────────────────────────────────────
//  CSV LOADING
// ───────────────────────────────────────────────────────────────────────

// Parses one CSV line and returns a Course object.
// Uses stringstream to split the line by commas into a fields vector.
// Only columns 0-4 are used — days, start time, and end time are ignored.
//
// Expected CSV format:
// crn, subject, code, name, creditHours, days, startTime, endTime
//  0      1      2     3        4          5       6          7
//
// Returns an empty Course if the line has fewer than 5 fields —
// guards against malformed rows or stray blank lines in the file.
Course Course::fromCSV(const std::string& csvLine) {
	std::stringstream ss(csvLine);
	std::string token;
	std::vector<std::string> fields;

	// Split the line by commas and collect each field.
	while (std::getline(ss, token, ',')) {
		fields.push_back(token);
	}

	// Safety guard — if the row is malformed return an empty course.
	// Prevents out-of-bounds crash on fields[4] and below.
	if (fields.size() < 5) return Course();

	std::string crn = fields[0];
	std::string subject = fields[1];
	std::string code = fields[2];
	std::string name = fields[3];

	// stoi converts the credit hours string to an integer.
	// Example: "4" → 4
	int creditHours = std::stoi(fields[4]);

	// fields[5], [6], [7] = days, start time, end time — intentionally ignored.
	return Course(crn, subject, code, name, creditHours);
}

// Reads the entire courses CSV file and returns a vector of all courses.
// Opens the file with ifstream, reads line by line, calls fromCSV()
// on each non-empty line, and collects results into a vector.
// Called once at server startup — catalog is loaded into memory
// and reused for all requests, never re-read from disk.
//
// Note: assumes the CSV has no header row. If a header is added
// later, skip the first line with a single getline() before the loop.
std::vector<Course> Course::loadFromFile(const std::string& filename) {
	std::vector<Course> courses;
	std::ifstream infile(filename);
	std::string line;

	// Read one line at a time until end of file.
	while (std::getline(infile, line)) {
		// Skip blank lines that may appear between rows.
		if (!line.empty()) {
			courses.push_back(fromCSV(line));
		}
	}

	return courses;
}

// ───────────────────────────────────────────────────────────────────────
//  SERIALIZATION
//  Converts this Course object into a JSON string for the frontend.
//  Called by AcademicRecord::toJson() when serializing records,
//  and by the server when responding to GET /courses requests.
//
//  Example output:
//  {
//    "crn":         "11543",
//    "subject":     "CMPS",
//    "code":        "1044",
//    "name":        "Computer Science I",
//    "creditHours": 4,
//    "courseCode":  "CMPS1044"
//  }
// ───────────────────────────────────────────────────────────────────────
std::string Course::toJson() const {
	std::string json = "{";
	json += "\"crn\":\"" + crn + "\",";
	json += "\"subject\":\"" + subject + "\",";
	json += "\"code\":\"" + code + "\",";
	json += "\"name\":\"" + name + "\",";
	json += "\"creditHours\":" + std::to_string(creditHours) + ",";

	// courseCode is last — no trailing comma.
	json += "\"courseCode\":\"" + courseCode + "\"";
	json += "}";
	return json;
}