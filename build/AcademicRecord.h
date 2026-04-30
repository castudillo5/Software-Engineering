#pragma once
#include <string>
#include "Course.h"

// ═══════════════════════════════════════════════════════════════════════
//  ACADEMICRECORD.H
//  Represents a single row in a student's course history.
//  Each record ties a Course to the student's performance in it —
//  the semester they took it, the grade they received, and whether
//  they have completed it. This class is the foundation of GPA
//  calculation and degree audit progress tracking.
// ═══════════════════════════════════════════════════════════════════════

class AcademicRecord {
private:
	// The full course object this record is associated with.
	// Gives direct access to course code, name, and credit hours
	// without needing to look them up separately.
	Course course;

	// The semester this course was taken
	// Used for display purposes on the frontend.
	// for maybe future features like sorting or filtering records by semester
	std::string semester;

	// The letter grade the student received (e.g. "A", "B+", "C-").
	// Stored as a string  not char  because plus/minus grades
	// like "B+" are two characters and char cannot hold them.
	std::string grade;

	// Whether the student has finished this course.
	// False means not completed, true means completed.
	// Only completed courses count toward GPA and degree requirements.
	bool completed;

public:
	// ───────────────────────────────────────────────────────────────────
	//  CONSTRUCTORS
	// ───────────────────────────────────────────────────────────────────

	// Default constructor.
	AcademicRecord();

	// Parameterized constructor.
	AcademicRecord(const Course& course, const std::string& semester,
		const std::string& grade, bool completed);

	// ───────────────────────────────────────────────────────────────────
	//  GETTERS
	//  All marked const they promise not to modify the record.
	// ───────────────────────────────────────────────────────────────────

	// Returns the full Course object associated with this record.
	Course getCourse() const;

	// Returns the semester this course was taken
	std::string getSemester() const;

	// Returns the letter grade for this record
	std::string getGrade() const;

	// Returns true if the student has completed this course.
	// Returns false if not
	bool isCompleted() const;

	// ───────────────────────────────────────────────────────────────────
	//  SETTERS
	// ───────────────────────────────────────────────────────────────────

	// Updates the grade for this record.
	// Called by Student::updateRecord() when a student corrects
	// or improves their grade entry. Automatically triggers a
	// GPA recalculation through the Student class.
	void setGrade(const std::string& grade);

	// Updates the completion status of this record.
	// Called when a student marks an in-progress course as finished.
	void setCompleted(bool completed);

	// ───────────────────────────────────────────────────────────────────
	//  UTILITY
	//  Convenience methods that let Student and DegreeAudit access
	//  common data without chaining calls like record.getCourse().X()
	// ───────────────────────────────────────────────────────────────────

	// Converts the letter grade to its GPA numeric equivalent.
	// Examples: "A" → 4.0, "B+" → 3.3, "C-" → 1.7, "F" → 0.0
	// Called by Student::calculateGPA() for every record.
	double getGradePoints() const;

	// Returns the credit hours for this course.
	// Shortcut for getCourse().getCreditHours().
	// Used by Student::calculateGPA() for weighted GPA calculation.
	int getCreditHours() const;

	// Returns the course code for this record (e.g. "CMPS1044").
	// Shortcut for getCourse().getCourseCode().
	// Used by Student::removeRecord() and Student::updateRecord()
	// to find the right record by course code.
	std::string getCourseCode() const;

	// ───────────────────────────────────────────────────────────────────
	//  SERIALIZATION
	// ───────────────────────────────────────────────────────────────────

	// Converts this record into a JSON string for the frontend.
	// Called by Student::toJson() when building the records array.
	// Example output:
	// {
	//   "courseCode": "CMPS1044",
	//   "courseName": "Computer Science I",
	//   "semester":   "Fall 2024",
	//   "grade":      "B+",
	//   "gradePoints": 3.3,
	//   "creditHours": 4,
	//   "completed":  true
	// }
	std::string toJson() const;
};