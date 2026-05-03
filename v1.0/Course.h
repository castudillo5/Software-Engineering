#pragma once
#include <string>
#include <vector>
#include "Section.h"

// ═══════════════════════════════════════════════════════════════════════
//  COURSE.H
//  Represents a single course offering loaded from the school's CSV.
//  Course is a pure data class — it holds exactly what comes from
//  the CSV file and nothing more. Prerequisite rules and degree
//  requirements are handled separately by DegreePlan, keeping
//  Course simple and focused on course catalog data only.
//
//  Course now owns a Section object that carries the meeting days,
//  times, room, and professor parsed from CSV columns 5–7.
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

	// Meeting details for this section: professor, days, times, room.
	// Populated from CSV columns 5–7 by fromCSV().
	// Previously these columns were discarded — they are now parsed.
	Section section;

public:
	// ───────────────────────────────────────────────────────────────────
	//  CONSTRUCTORS
	// ───────────────────────────────────────────────────────────────────

	// Default constructor — all fields empty/zero.
	Course();

	// Parameterized constructor.
	// courseCode is computed automatically from subject + code.
	// Section is supplied as an already-constructed object.
	Course(const std::string& crn, const std::string& subject,
		const std::string& code, const std::string& name,
		int creditHours, const Section& section);

	// ───────────────────────────────────────────────────────────────────
	//  GETTERS
	// ───────────────────────────────────────────────────────────────────

	std::string getCrn()        const;
	std::string getSubject()    const;
	std::string getCode()       const;
	std::string getName()       const;
	int         getCreditHours() const;
	std::string getCourseCode() const;

	// Returns the Section object for this course.
	// Used by AcademicRecord::toJson() and the schedule calendar.
	const Section& getSection() const;

	// ───────────────────────────────────────────────────────────────────
	//  CSV LOADING
	// ───────────────────────────────────────────────────────────────────

	// Parses one CSV line and returns a Course object.
	// Now reads all 8 columns — cols 5–7 (days, startTime, endTime)
	// are passed to the Section constructor instead of being discarded.
	//
	// Expected CSV format (col indices 0-based):
	//   0:CRN  1:subject  2:code  3:creditHours  4:name
	//   5:days  6:startTime  7:endTime
	//   8:roomNumber (optional)  9:professor (optional)
	//
	// Optional 9th column (index 8) for roomNumber — empty if absent.
	static Course fromCSV(const std::string& csvLine);

	// Reads the entire courses CSV file and returns all courses.
	static std::vector<Course> loadFromFile(const std::string& filename);

	// ───────────────────────────────────────────────────────────────────
	//  SERIALIZATION
	// ───────────────────────────────────────────────────────────────────

	// Converts this course to a JSON string.
	// Now includes the nested "section" object so the frontend gets
	// professor, meetingDays, startTime, endTime, and roomNumber.
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
	std::string toJson() const;
};
