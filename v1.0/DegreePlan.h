#pragma once
#include <string>
#include <vector>
#include <map>
#include "Structs.h"

// ═══════════════════════════════════════════════════════════════════════
//  DEGREEPLAN.H
//  Represents the complete rulebook for a degree program.
//  Hardcodes all requirements for the CS degree at MSU Texas —
//  rigid required courses, category requirements, flex gen ed
//  categories, prerequisite staircase rules, and advanced elective
//  hour requirements. DegreePlan is the same for every CS student.
//  Per-student progress is tracked separately in DegreeAudit.
// ═══════════════════════════════════════════════════════════════════════

class DegreePlan {
private:
	// The name of the major this plan represents.
	// Example: "Computer Science"
	std::string major;

	// Courses the student MUST complete — no substitutions, no skipping.
	// Includes all core CMPS, mandatory Math, Physics, and Gen Ed courses.
	// Example: "CMPS1044", "MATH1534", "PHYS1624", "HIST1133"
	std::vector<std::string> rigidCourses;

	// Structured requirements that have both mandatory and elective options.
	// Example: Math category — mandatory MATH1534 + MATH1634,
	// then pick one of MATH2753, MATH3533, or MATH4243.
	std::vector<CategoryRequirement> categoryRequirements;

	// Flexible gen ed categories where student picks from an approved list.
	// Example: Creative Arts (050N) — pick any 3hr approved course.
	// Student assigns a course through the frontend.
	std::vector<FlexCategory> flexCategories;

	// Prerequisite staircase rules — one entry per course that has a prereq.
	// Key   = course the student wants to take (e.g. "CMPS3013")
	// Value = the rule they must satisfy first (e.g. CMPS2143 with C or better)
	std::map<std::string, PrereqRule> prereqRules;

	// Total credit hours of 4000-level CMPS courses required
	// beyond the core mandatory CMPS courses.
	// Example: 15 hrs of advanced CMPS electives.
	int advancedElectiveHours;

	// ───────────────────────────────────────────────────────────────────
	//  PRIVATE LOADER
	//  Called by the constructor to hardcode all CS degree requirements.
	//  Separated into its own method to keep the constructor clean.
	// ───────────────────────────────────────────────────────────────────

	// Populates all private fields with the CS degree requirements.
	// Hardcodes rigid courses, category rules, flex categories,
	// prereq staircase, and advanced elective hour requirement.
	void loadCSDegreePlan();

public:
	// ───────────────────────────────────────────────────────────────────
	//  CONSTRUCTOR
	// ───────────────────────────────────────────────────────────────────

	// Builds the degree plan for the given major.
	// Currently only supports "Computer Science".
	// Calls loadCSDegreePlan() internally to populate all rules.
	DegreePlan(const std::string& major);

	// ───────────────────────────────────────────────────────────────────
	//  GETTERS
	//  All const — the rulebook never changes after construction.
	// ───────────────────────────────────────────────────────────────────

	// Returns the name of the major this plan covers.
	std::string getMajor() const;

	// Returns the list of courses every CS student must complete.
	const std::vector<std::string>& getRigidCourses() const;

	// Returns the structured category requirements (Math, Physics etc).
	const std::vector<CategoryRequirement>& getCategoryRequirements() const;

	// Returns the flexible gen ed categories with their approved lists.
	const std::vector<FlexCategory>& getFlexCategories() const;

	// Returns the full prereq rules map.
	const std::map<std::string, PrereqRule>& getPrereqRules() const;

	// Returns the number of advanced elective hours required.
	int getAdvancedElectiveHours() const;

	// ───────────────────────────────────────────────────────────────────
	//  LOOKUP
	// ───────────────────────────────────────────────────────────────────

	// Returns true if the given course has a prerequisite rule.
	// Used by DegreeAudit before checking prereq satisfaction.
	bool hasPrereq(const std::string& courseCode) const;

	// Returns the PrereqRule for the given course.
	// Call hasPrereq() first — throws if no rule exists.
	PrereqRule getPrereqRule(const std::string& courseCode) const;
};