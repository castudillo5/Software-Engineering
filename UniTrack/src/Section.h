#pragma once
#include <string>
#include <vector>

// ═══════════════════════════════════════════════════════════════════════
//  SECTION.H
//  Represents the meeting details for one section of a Course.
//  Holds the professor, meeting days, start/end times, and room number.
//  Course owns one Section object. Section data comes from the CSV
//  columns that Course::fromCSV() previously discarded (cols 5–7).
//  Section::toJson() is included in Course::toJson() so the frontend
//  can populate the schedule calendar and registration detail rows.
// ═══════════════════════════════════════════════════════════════════════

class Section {
private:
	// Faculty name for this section.
	// Stored as empty string if not provided in the CSV.
	std::string professor;

	// Days of the week this section meets.
	// Parsed from the raw day string (e.g. "MWF" → ["Mon","Wed","Fri"]).
	// Stored as full day names so the frontend calendar can match them
	// directly against column headers without a second parse step.
	std::vector<std::string> meetingDays;

	// Start time as a display string (e.g. "8:00 AM").
	// Converted from the raw CSV format (e.g. "0800am") in parseTime().
	std::string startTime;

	// End time as a display string (e.g. "9:20 AM").
	std::string endTime;

	// Room number / building where this section meets.
	// Example: "Bolin 101"
	std::string roomNumber;

	// ── Private helpers ────────────────────────────────────────────────

	// Parses a raw CSV day string into a vector of full day names.
	// "MWF" → ["Mon","Wed","Fri"]
	// "TR"  → ["Tue","Thu"]
	static std::vector<std::string> parseDays(const std::string& raw);

	// Converts a raw time string like "0800am" or "0920am" to "8:00 AM".
	static std::string parseTime(const std::string& raw);

public:
	// ───────────────────────────────────────────────────────────────────
	//  CONSTRUCTORS
	// ───────────────────────────────────────────────────────────────────

	// Default constructor — all fields empty.
	Section();

	// Parameterized constructor.
	// Called by Course::fromCSV() after parsing columns 5–7.
	Section(const std::string& professor,
		const std::string& rawDays,
		const std::string& rawStart,
		const std::string& rawEnd,
		const std::string& roomNumber);

	// ───────────────────────────────────────────────────────────────────
	//  GETTERS
	// ───────────────────────────────────────────────────────────────────

	std::string              getProfessor()   const;
	std::vector<std::string> getMeetingDays() const;
	std::string              getStartTime()   const;
	std::string              getEndTime()     const;
	std::string              getRoomNumber()  const;

	// ───────────────────────────────────────────────────────────────────
	//  SERIALIZATION
	// ───────────────────────────────────────────────────────────────────

	// Converts this section to a JSON object.
	// Called by Course::toJson() — nested inside the course object.
	// Example output:
	// {
	//   "professor":   "Dr. Smith",
	//   "meetingDays": ["Mon","Wed","Fri"],
	//   "startTime":   "8:00 AM",
	//   "endTime":     "9:20 AM",
	//   "roomNumber":  "Bolin 101"
	// }
	std::string toJson() const;
};
