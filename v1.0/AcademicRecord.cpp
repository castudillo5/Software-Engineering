#include "AcademicRecord.h"

// ═══════════════════════════════════════════════════════════════════════
//  ACADEMICRECORD.CPP
//  Implements AcademicRecord — a single row in a student's course
//  history. Stores the course taken, semester, grade received, and
//  completion status. Provides grade-to-GPA conversion which is the
//  backbone of Student::calculateGPA().
// ═══════════════════════════════════════════════════════════════════════

// ───────────────────────────────────────────────────────────────────────
//  CONSTRUCTORS
// ───────────────────────────────────────────────────────────────────────

// Default constructor.
// Initializes an empty record with safe defaults.
// Course default-constructs to empty, semester and grade to empty
// strings, and completed to false since nothing has been finished yet.
AcademicRecord::AcademicRecord()
	: course(), semester(""), grade(""), completed(false) {
}

// Parameterized constructor.
// Builds a fully populated record in one step.
// All objects passed by const reference to avoid unnecessary copies.
AcademicRecord::AcademicRecord(const Course& course, const std::string& semester,
	const std::string& grade, bool completed)
	: course(course), semester(semester), grade(grade), completed(completed) {
}

// ───────────────────────────────────────────────────────────────────────
//  GETTERS
// ───────────────────────────────────────────────────────────────────────

// Returns the full Course object for this record.
// Gives access to course name, code, credit hours, and prereqs.
Course AcademicRecord::getCourse() const {
	return course;
}

// Returns the semester this course was taken (e.g. "Spring 2025").
std::string AcademicRecord::getSemester() const {
	return semester;
}

// Returns the letter grade for this record (e.g. "B+", "A-", "C").
std::string AcademicRecord::getGrade() const {
	return grade;
}

// Returns true if the student has completed this course.
// Returns false if they are currently enrolled but not yet finished.
bool AcademicRecord::isCompleted() const {
	return completed;
}

// ───────────────────────────────────────────────────────────────────────
//  SETTERS
// ───────────────────────────────────────────────────────────────────────

// Updates the grade for this record.
// Called by Student::updateRecord() when a student corrects or
// improves their grade. Student::calculateGPA() fires immediately
// after this to keep the GPA current.
void AcademicRecord::setGrade(const std::string& grade) {
	this->grade = grade;
}

// Updates the completion status of this record.
// Called when a student marks an in-progress course as finished.
// Only completed courses count toward GPA and degree requirements.
void AcademicRecord::setCompleted(bool completed) {
	this->completed = completed;
}

// ───────────────────────────────────────────────────────────────────────
//  UTILITY
// ───────────────────────────────────────────────────────────────────────

// Converts the stored letter grade to its GPA numeric equivalent.
// Uses MSU's standard grading scale.
// Called by Student::calculateGPA() for every record in the vector.
// Returns 0.0 for any unrecognized grade as a safe fallback.
//
// Scale:
//   A  = 4.0 | A- = 3.7
//   B+ = 3.3 | B  = 3.0 | B- = 2.7
//   C+ = 2.3 | C  = 2.0 | C- = 1.7
//   D+ = 1.3 | D  = 1.0 | D- = 0.7
//   F  = 0.0
double AcademicRecord::getGradePoints() const {
	if (grade == "A")  return 4.0;
	if (grade == "A-") return 3.7;
	if (grade == "B+") return 3.3;
	if (grade == "B")  return 3.0;
	if (grade == "B-") return 2.7;
	if (grade == "C+") return 2.3;
	if (grade == "C")  return 2.0;
	if (grade == "C-") return 1.7;
	if (grade == "D+") return 1.3;
	if (grade == "D")  return 1.0;
	if (grade == "D-") return 0.7;
	if (grade == "F")  return 0.0;

	// Unrecognized grade — return 0.0 as a safe default.
	// Future improvement: throw an exception or log a warning.
	return 0.0;
}

// Returns the credit hours for this course.
// Shortcut so callers don't need to chain getCourse().getCreditHours().
// Used directly by Student::calculateGPA() in the weighted average.
int AcademicRecord::getCreditHours() const {
	return course.getCreditHours();
}

// Returns the course code for this record (e.g. "CMPS1044").
// Shortcut so callers don't need to chain getCourse().getCourseCode().
// Used by Student::removeRecord() and Student::updateRecord()
// to locate the correct record by course code.
std::string AcademicRecord::getCourseCode() const {
	return course.getCourseCode();
}

// ───────────────────────────────────────────────────────────────────────
//  SERIALIZATION
//  Converts this record into a JSON string for the frontend.
//  Called by Student::toJson() when building the records array.
//  gradePoints and creditHours are included so the frontend
//  never has to recalculate them — it just reads and displays.
// ───────────────────────────────────────────────────────────────────────

// Example output:
// {
//   "courseCode":  "CMPS1044",
//   "courseName":  "Computer Science I",
//   "semester":    "Spring 2025",
//   "grade":       "B+",
//   "gradePoints": 3.3,
//   "creditHours": 4,
//   "completed":   true
// }
std::string AcademicRecord::toJson() const {
	std::string json = "{";

	// Nest the full course object so the frontend has all course details.
	json += "\"course\":" + course.toJson() + ",";
	json += "\"semester\":\"" + semester + "\",";
	json += "\"grade\":\"" + grade + "\",";

	// Pre-computed values — saves the frontend from doing the math.
	json += "\"gradePoints\":" + std::to_string(getGradePoints()) + ",";
	json += "\"creditHours\":" + std::to_string(getCreditHours()) + ",";

	// Boolean — JSON uses lowercase true/false not "true"/"false" string.
	json += "\"completed\":" + std::string(completed ? "true" : "false");

	json += "}";
	return json;
}