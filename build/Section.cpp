#include "Section.h"
#include <sstream>
#include <iomanip>

// ═══════════════════════════════════════════════════════════════════════
//  SECTION.CPP
//  Implements Section — the meeting details for one course section.
//  Parses raw CSV day/time strings into clean display-ready values.
//  Serializes to JSON so Course::toJson() can nest it directly.
// ═══════════════════════════════════════════════════════════════════════

// ───────────────────────────────────────────────────────────────────────
//  PRIVATE HELPERS
// ───────────────────────────────────────────────────────────────────────

// Parses a raw CSV day string into a vector of full day names.
// The MSU CSV uses single-letter codes: M T W R F
// "R" is Thursday — a common convention in university scheduling.
// "TR" → ["Tue","Thu"]   "MWF" → ["Mon","Wed","Fri"]
// Handles the ambiguity of "T" vs "TR" by checking for "R" after "T".
std::vector<std::string> Section::parseDays(const std::string& raw) {
	std::vector<std::string> days;
	for (size_t i = 0; i < raw.size(); i++) {
		char c = raw[i];
		if      (c == 'M')                      days.push_back("Mon");
		else if (c == 'T' && i + 1 < raw.size() && raw[i+1] == 'R') {
			// "TR" pattern — skip both characters, push Tue and Thu.
			days.push_back("Tue");
			days.push_back("Thu");
			i++; // skip 'R'
		}
		else if (c == 'T')                      days.push_back("Tue");
		else if (c == 'W')                      days.push_back("Wed");
		else if (c == 'R')                      days.push_back("Thu");
		else if (c == 'F')                      days.push_back("Fri");
	}
	return days;
}

// Converts a raw time string from the MSU CSV to a display string.
// Input format: "0800am", "0920am", "0100pm"
// Output format: "8:00 AM", "9:20 AM", "1:00 PM"
// Returns empty string for any input that doesn't match the expected format.
std::string Section::parseTime(const std::string& raw) {
	// Need at least 6 characters: "0800am"
	if (raw.size() < 6) return "";

	// Extract hour and minute digits.
	std::string hourStr = raw.substr(0, 2);
	std::string minStr  = raw.substr(2, 2);
	std::string ampm    = raw.substr(4);   // "am" or "pm"

	int hour = 0, min = 0;
	try {
		hour = std::stoi(hourStr);
		min  = std::stoi(minStr);
	}
	catch (...) {
		return "";
	}

	// Convert to 12-hour clock.
	// 0 hour (midnight) → 12, 13 → 1, etc.
	int displayHour = hour % 12;
	if (displayHour == 0) displayHour = 12;

	// Build the display string.
	// std::to_string(min) won't zero-pad, so we do it manually.
	std::string minDisplay = (min < 10 ? "0" : "") + std::to_string(min);
	std::string suffix     = (ampm == "pm" || ampm == "PM") ? "PM" : "AM";

	return std::to_string(displayHour) + ":" + minDisplay + " " + suffix;
}

// ───────────────────────────────────────────────────────────────────────
//  CONSTRUCTORS
// ───────────────────────────────────────────────────────────────────────

// Default constructor — all fields empty.
// Used when a Course is default-constructed or CSV parsing falls short.
Section::Section()
	: professor(""), meetingDays(), startTime(""), endTime(""), roomNumber("") {
}

// Parameterized constructor.
// Accepts raw CSV strings for days and times and converts them
// to clean display values using the private helper methods.
Section::Section(const std::string& professor,
	const std::string& rawDays,
	const std::string& rawStart,
	const std::string& rawEnd,
	const std::string& roomNumber)
	: professor(professor),
	  meetingDays(parseDays(rawDays)),
	  startTime(parseTime(rawStart)),
	  endTime(parseTime(rawEnd)),
	  roomNumber(roomNumber) {
}

// ───────────────────────────────────────────────────────────────────────
//  GETTERS
// ───────────────────────────────────────────────────────────────────────

std::string Section::getProfessor() const {
	return professor;
}

std::vector<std::string> Section::getMeetingDays() const {
	return meetingDays;
}

std::string Section::getStartTime() const {
	return startTime;
}

std::string Section::getEndTime() const {
	return endTime;
}

std::string Section::getRoomNumber() const {
	return roomNumber;
}

// ───────────────────────────────────────────────────────────────────────
//  SERIALIZATION
// ───────────────────────────────────────────────────────────────────────

// Converts this Section to a JSON object string.
// Called by Course::toJson() — nested as "section": { ... }.
// meetingDays is serialized as a JSON array of strings so the
// schedule.html calendar can iterate over it directly in JS.
//
// Example output:
// {
//   "professor":   "Dr. Smith",
//   "meetingDays": ["Mon","Wed","Fri"],
//   "startTime":   "8:00 AM",
//   "endTime":     "9:20 AM",
//   "roomNumber":  "Bolin 101"
// }
std::string Section::toJson() const {
	std::string json = "{";

	json += "\"professor\":\"" + professor + "\",";

	// meetingDays is a JSON array — iterate and quote each day name.
	json += "\"meetingDays\":[";
	for (size_t i = 0; i < meetingDays.size(); i++) {
		json += "\"" + meetingDays[i] + "\"";
		if (i < meetingDays.size() - 1) json += ",";
	}
	json += "],";

	json += "\"startTime\":\"" + startTime + "\",";
	json += "\"endTime\":\"" + endTime + "\",";
	json += "\"roomNumber\":\"" + roomNumber + "\"";

	json += "}";
	return json;
}
