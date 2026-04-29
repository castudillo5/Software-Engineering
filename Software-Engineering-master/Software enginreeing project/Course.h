#pragma once
#include <string>
#include <vector>

// ═══════════════════════════════════════════════════════════════════════
//  COURSE.H
//  Represents a single course offering loaded from the school's CSV.
//  Course is a pure data class — it holds exactly what comes from
//  the CSV file and nothing more. Prerequisite rules and degree
//  requirements are handled separately by DegreePlan, keeping
//  Course simple and focused on course catalog data only.
// ═══════════════════════════════════════════════════════════════════════

class Course {
private:
	// Course Registration Number — unique ID for this section.
	// Example: "11543"
	std::string crn;

	// Department subject code.
	// Example: "CMPS", "MATH", "BIOL"
	std::string subject;

	// Course number within the subject.
	// Example: "1044", "1634"
	std::string code;

	// Full course title as listed in the catalog.
	// Example: "Computer Science I"
	std::string name;

	// Number of semester credit hours this course is worth.
	// Added manually to the CSV before loading — not in original file.
	int creditHours;

	// Combined subject + code used as a universal identifier.
	// Example: "CMPS1044" built from "CMPS" + "1044"
	// Used everywhere — prereq checks, degree audit, record lookup.
	std::string courseCode;

public:
	// ───────────────────────────────────────────────────────────────────
	//  CONSTRUCTORS
	// ───────────────────────────────────────────────────────────────────

	// Default constructor.
	// Creates an empty course with safe default values.
	Course();

	// Parameterized constructor.
	// Builds a course from its component parts.
	// courseCode is computed automatically from subject + code.
	Course(const std::string& crn, const std::string& subject,
		const std::string& code, const std::string& name,
		int creditHours);

	// ───────────────────────────────────────────────────────────────────
	//  GETTERS
	// ───────────────────────────────────────────────────────────────────

	// Returns the Course Registration Number for this section.
	std::string getCrn() const;

	// Returns the department subject code (e.g. "CMPS").
	std::string getSubject() const;

	// Returns the course number (e.g. "1044").
	std::string getCode() const;

	// Returns the full course title (e.g. "Computer Science I").
	std::string getName() const;

	// Returns the number of credit hours for this course.
	int getCreditHours() const;

	// Returns the combined course code (e.g. "CMPS1044").
	// This is the primary identifier used throughout the system.
	std::string getCourseCode() const;

	// ───────────────────────────────────────────────────────────────────
	//  CSV LOADING
	//  Both methods are static — they belong to the class itself,
	//  not to any specific Course object. You call them like this:
	//  Course::fromCSV(line) and Course::loadFromFile("courses.csv")
	//  without needing a Course object to exist first.
	// ───────────────────────────────────────────────────────────────────

	// Parses one CSV line and returns a Course object.
	// Extracts columns 0-4, ignores days and time columns.
	// Example input:
	// "11543,CMPS,1044,Computer Science I,4,MW,0800am,0920am"
	static Course fromCSV(const std::string& csvLine);

	// Reads the entire courses CSV file and returns all courses.
	// Opens the file with ifstream, reads line by line, calls
	// fromCSV() on each line, and collects results into a vector.
	// Called once at server startup — catalog loaded into memory
	// and reused for all requests, never re-read from disk.
	static std::vector<Course> loadFromFile(const std::string& filename);

	// ───────────────────────────────────────────────────────────────────
	//  SERIALIZATION
	// ───────────────────────────────────────────────────────────────────

	// Converts this course to a JSON string for the frontend.
	// Called by AcademicRecord::toJson() when serializing records,
	// and by the server when responding to GET /courses requests.
	// Example output:
	// {
	//   "crn":         "11543",
	//   "subject":     "CMPS",
	//   "code":        "1044",
	//   "name":        "Computer Science I",
	//   "creditHours": 4,
	//   "courseCode":  "CMPS1044"
	// }
	std::string toJson() const;
};