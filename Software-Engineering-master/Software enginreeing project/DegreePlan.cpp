#include "DegreePlan.h"

// ═══════════════════════════════════════════════════════════════════════
//  DEGREEPLAN.CPP
//  Implements DegreePlan — the complete rulebook for the CS degree at
//  MSU Texas. Hardcodes all rigid courses, category requirements, flex
//  gen ed categories, prerequisite staircase rules, and advanced elective
//  hour requirements. DegreePlan is the same for every CS student.
//  Per-student progress is tracked separately in DegreeAudit.
// ═══════════════════════════════════════════════════════════════════════

// ───────────────────────────────────────────────────────────────────────
//  CONSTRUCTOR
// ───────────────────────────────────────────────────────────────────────

// Builds the degree plan for the given major.
// Initializes advanced elective hours to 15 — five 3-hour slots
// of upper division CMPS courses beyond the mandatory core.
// Calls loadCSDegreePlan() to populate all rules immediately.
DegreePlan::DegreePlan(const std::string& major)
	: major(major), advancedElectiveHours(15) {
	loadCSDegreePlan();
}

// ───────────────────────────────────────────────────────────────────────
//  DEGREE PLAN LOADER
//  Hardcodes the complete CS degree requirements for MSU Texas.
//  Separated from the constructor to keep it clean and readable.
//  Divided into four sections: rigid courses, category requirements,
//  flex gen ed categories, and prerequisite staircase rules.
// ───────────────────────────────────────────────────────────────────────
void DegreePlan::loadCSDegreePlan() {
	// ── Rigid courses ──────────────────────────────────────────────────
	// Every CS student must complete ALL of these with no substitutions.
	// Order matches the degree map from the MSU Texas catalog.
	rigidCourses = {
		// Core CMPS — the heart of the CS degree
		"CMPS1044", "CMPS1063", "CMPS2084", "CMPS2143",
		"CMPS2433", "CMPS3013", "CMPS3023", "CMPS3233",
		"CMPS4143", "CMPS4103", "CMPS4113", "CMPS4991",

		// Mandatory Math — required before Physics
		"MATH1534", "MATH1634",

		// Mandatory Physics — required for CS engineering track
		"PHYS1624", "PHYS2644",

		// Mandatory Gen Ed — fixed courses with no substitutions
		"ENGL1143",
		"HIST1133", "HIST1233",
		"POLS1333", "POLS1433"
	};

	// ── Category requirements ──────────────────────────────────────────
	// Structured requirements that have both mandatory and elective parts.
	// Student must satisfy the hour requirement using the approved lists.

	// Math upper elective — must take ONE of three options for 3 hours.
	// No mandatory course — any one of the three satisfies the requirement.
	categoryRequirements.push_back({
		"Math Upper Elective",
		3,
		{},
		{"MATH2753", "MATH3533", "MATH4243"}
		});

	// ── Flex categories ────────────────────────────────────────────────
	// General education requirements where the student picks any
	// approved course to satisfy the category hour requirement.
	// The system verifies the course is in the approved list and
	// has not already been used for another category.

	// Communications (010B) — 3 hours, pick one approved course.
	// ENGL1143 satisfies 010A — this is the second communications slot.
	flexCategories.push_back({
		"Communications",
		"010B",
		3,
		{
			"ENGL1153", "ENGL2123", "ENGL2203",
			"MCOM1603", "MCOM2403",
			"SPCH1133", "SPCH2423"
		}
		});

	// Lab Science (030N) — 6 hours total, pick approved science courses.
	// Note: PHYS1624 and PHYS2644 are in rigid courses and do NOT
	// count toward this flex category — they satisfy Physics separately.
	flexCategories.push_back({
		"Lab Science",
		"030N",
		6,
		{
			"BIOL1013", "BIOL1023", "BIOL1103", "BIOL1114",
			"BIOL1133", "BIOL1134", "BIOL1144", "BIOL1214",
			"BIOL1233", "BIOL1234", "BIOL1544",
			"CHEM1103", "CHEM1143", "CHEM1243", "CHEM1303",
			"ENSC1114", "GEOS1134", "GEOS1234",
			"GNSC1104", "GNSC1204",
			"PHYS1144", "PHYS1244", "PHYS1533"
		}
		});

	// Language Philosophy and Culture (040N) — 3 hours.
	// Some courses here also appear in 090A — student can only
	// use each course for one category (no-reuse rule enforced
	// by DegreeAudit using the usedCourses set).
	flexCategories.push_back({
		"Language Philosophy and Culture",
		"040N",
		3,
		{
			"ENGL2413", "ENGL2423", "ENGL2613", "ENGL2623",
			"ENGL2813", "ENGL2823",
			"FREN1134", "GERM1134", "SPAN1134",
			"HIST1333", "HIST1353", "HIST1433", "HIST1453",
			"HUMN2013", "HUMN2023", "HUMN2033", "HUMN2043",
			"PHIL1033", "PHIL1533", "PHIL2033", "PHIL2133",
			"PHIL2213", "PHIL2223", "PHIL2333",
			"PHIL2503", "PHIL2513"
		}
		});

	// Creative Arts (050N) — 3 hours, pick one approved course.
	flexCategories.push_back({
		"Creative Arts",
		"050N",
		3,
		{
			"ART1413", "MCOM2213",
			"MUSC1033", "MUSC1043", "MUSC2733",
			"THEA1103", "THEA1113", "THEA1503", "THEA2423"
		}
		});

	// Social and Behavioral Sciences (080N) — 3 hours.
	flexCategories.push_back({
		"Social and Behavioral Sciences",
		"080N",
		3,
		{
			"COUN2023", "ECON1333", "ECON2333",
			"HSHS1023", "PSYC1103", "SOCL1133"
		}
		});

	// Cultural and Global Understanding (090A) — 3 hours.
	// Several courses overlap with 040N — no-reuse rule applies.
	flexCategories.push_back({
		"Cultural and Global Understanding",
		"090A",
		3,
		{
			"ART1613", "CMPS1023", "EDUC2013",
			"FREN1234", "GERM1234", "GLBS2503",
			"HIST1353", "HIST1453",
			"HSHS1013", "HUMN2013", "HUMN2023",
			"HUMN2033", "HUMN2043",
			"MCOM1233", "MCOM2523",
			"MUSC1043", "MUSC2733",
			"PHIL2503", "PHIL2513",
			"POLS2523", "SOCL2233", "SPAN1234",
			"WGST2503"
		}
		});

	// ── Prerequisite staircase rules ───────────────────────────────────
	// Each entry defines what a student must have completed — and with
	// what minimum grade — before taking the course in the key.
	// isOr = false means AND logic — all listed prereqs must be met.
	// isOr = true  means OR logic  — any one prereq satisfies the rule.

	// CMPS staircase — follows the degree map order exactly.
	// Both CMPS1063 and CMPS2084 branch from CMPS1044 independently.
	prereqRules["CMPS1063"] = { {"CMPS1044"}, "C", false };
	prereqRules["CMPS2084"] = { {"CMPS1044"}, "C", false };
	prereqRules["CMPS2143"] = { {"CMPS1063"}, "C", false };
	prereqRules["CMPS2433"] = { {"CMPS2143"}, "C", false };
	prereqRules["CMPS3013"] = { {"CMPS2143"}, "C", false };
	prereqRules["CMPS3023"] = { {"CMPS2084"}, "C", false };
	prereqRules["CMPS3233"] = { {"CMPS3013"}, "C", false };
	prereqRules["CMPS4143"] = { {"CMPS3013"}, "C", false };
	prereqRules["CMPS4103"] = { {"CMPS3013"}, "C", false };
	prereqRules["CMPS4113"] = { {"CMPS4103"}, "C", false };

	// Math staircase — Calculus I requires Pre-Calculus first.
	prereqRules["MATH1634"] = { {"MATH1534"}, "C", false };

	// Physics staircase — chains through Math then Physics sequence.
	// Physics I requires Calculus I, Physics II requires Physics I.
	prereqRules["PHYS1624"] = { {"MATH1634"}, "C", false };
	prereqRules["PHYS2644"] = { {"PHYS1624"}, "C", false };
}

// ───────────────────────────────────────────────────────────────────────
//  GETTERS
//  All return const references — the rulebook never changes after
//  construction. Returning by reference avoids copying large vectors.
// ───────────────────────────────────────────────────────────────────────

// Returns the name of the major this plan covers.
std::string DegreePlan::getMajor() const {
	return major;
}

// Returns the list of courses every CS student must complete.
const std::vector<std::string>& DegreePlan::getRigidCourses() const {
	return rigidCourses;
}

// Returns the structured category requirements (Math upper elective).
const std::vector<CategoryRequirement>& DegreePlan::getCategoryRequirements() const {
	return categoryRequirements;
}

// Returns the flexible gen ed categories with their approved lists.
const std::vector<FlexCategory>& DegreePlan::getFlexCategories() const {
	return flexCategories;
}

// Returns the full prerequisite rules map.
const std::map<std::string, PrereqRule>& DegreePlan::getPrereqRules() const {
	return prereqRules;
}

// Returns the number of advanced CMPS elective hours required.
int DegreePlan::getAdvancedElectiveHours() const {
	return advancedElectiveHours;
}

// ───────────────────────────────────────────────────────────────────────
//  LOOKUP
// ───────────────────────────────────────────────────────────────────────

// Returns true if the given course has a prerequisite rule defined.
// Uses map::count() which returns 1 if key exists, 0 if not.
// Always call this before getPrereqRule() to avoid exceptions.
bool DegreePlan::hasPrereq(const std::string& courseCode) const {
	return prereqRules.count(courseCode) > 0;
}

// Returns the PrereqRule for the given course code.
// Uses map::at() which throws std::out_of_range if key not found.
// Always guard with hasPrereq() before calling this.
PrereqRule DegreePlan::getPrereqRule(const std::string& courseCode) const {
	return prereqRules.at(courseCode);
}