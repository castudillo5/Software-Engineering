#pragma once
#include <string>
#include <vector>

// ═══════════════════════════════════════════════════════════════════════
//  STRUCTS.H
//  Shared data structures used by both DegreePlan and DegreeAudit.
//
//  DegreePlan uses:
//    PrereqRule          — defines the staircase rule for one course
//    CategoryRequirement — defines a structured requirement like Physics
//    FlexCategory        — defines a gen ed category with approved courses
//
//  DegreeAudit uses:
//    CategoryProgress    — tracks THIS student's progress in a category
//    FlexProgress        — tracks THIS student's flex category completion
//
//  These are plain structs — no methods, no class wrapper.
//  Any file that includes Structs.h can use all of these directly.
// ═══════════════════════════════════════════════════════════════════════

// ───────────────────────────────────────────────────────────────────────
//  PREREQRULE
//  Represents one staircase step in the degree plan.
//  Defines what a student must have completed before taking a course.
//  Example: to take CMPS3013 you need CMPS2143 with a C or better.
// ───────────────────────────────────────────────────────────────────────
struct PrereqRule {
	// One or more course codes that serve as prerequisites.
	// How they combine depends on the isOr flag below.
	// Example: ["CMPS2143"] or ["MATH1023", "MATH1033"]
	std::vector<std::string> courseCodes;

	// Minimum letter grade required in the prerequisite course.
	// Example: "C" means the student must have earned a C or better.
	// Empty string means any passing grade is acceptable.
	std::string minimumGrade;

	// Controls how multiple courseCodes are interpreted.
	// true  = OR logic — student needs ANY ONE of the listed courses.
	// false = AND logic — student needs ALL of the listed courses.
	bool isOr;
};

// ───────────────────────────────────────────────────────────────────────
//  CATEGORYREQUIREMENT
//  Represents a structured degree requirement that has both mandatory
//  specific courses and optional elective courses to fill remaining hours.
//  Example: Physics requires PHYS1624 AND PHYS2644 specifically,
//  with no substitutions allowed.
// ───────────────────────────────────────────────────────────────────────
struct CategoryRequirement {
	// Display name for this requirement.
	// Example: "Physics", "Math"
	std::string categoryName;

	// Total credit hours the student must complete in this category.
	// Example: 8 for Physics (PHYS1624=4hrs + PHYS2644=4hrs)
	int requiredCreditHours;

	// Courses the student MUST take — no substitutions allowed.
	// Example: ["PHYS1624", "PHYS2644"] for Physics requirement.
	std::vector<std::string> mandatoryCourses;

	// Courses the student can choose from to fill any remaining hours.
	// Empty if all hours must come from mandatory courses.
	std::vector<std::string> electiveCourses;
};

// ───────────────────────────────────────────────────────────────────────
//  FLEXCATEGORY
//  Represents a general education requirement where the student
//  picks any approved course to satisfy the category.
//  Example: Creative Arts (050N) — pick any 3hr course from the list.
//  The student assigns a course to this category through the frontend.
//  The system verifies the course is in the approved list and that
//  it has not already been used for another category.
// ───────────────────────────────────────────────────────────────────────
struct FlexCategory {
	// Display name for this gen ed category.
	// Example: "Creative Arts", "Social and Behavioral Sciences"
	std::string categoryName;

	// Official university category code from the degree plan.
	// Example: "050N", "080N", "090A"
	std::string categoryCode;

	// Total credit hours needed to satisfy this category.
	// Usually 3, sometimes 6 (e.g. Lab Science 030N = 6hrs).
	int requiredCreditHours;

	// Complete list of courses approved to satisfy this category.
	// Student must pick from this list — other courses do not count.
	// Example for 050N: ["ART1413", "MCOM2213", "MUSC1033", ...]
	std::vector<std::string> approvedCourses;
};

// ───────────────────────────────────────────────────────────────────────
//  CATEGORYPROGRESS
//  Tracks THIS specific student's progress in a CategoryRequirement.
//  Lives in DegreeAudit — not DegreePlan.
//  DegreePlan holds the rules. DegreeAudit holds the results.
//  Recalculated fresh every time DegreeAudit runs.
// ───────────────────────────────────────────────────────────────────────
struct CategoryProgress {
	// Display name matching the CategoryRequirement being tracked.
	std::string categoryName;

	// Total hours required for this category.
	int requiredHours;

	// Hours the student has completed so far in this category.
	int completedHours;

	// True when completedHours >= requiredHours AND all mandatory
	// courses have been completed.
	bool satisfied;

	// List of mandatory courses the student has not yet completed.
	// Empty when all mandatory courses are done.
	// Example: ["PHYS2644"] if student took PHYS1624 but not PHYS2644.
	std::vector<std::string> missingMandatory;
};

// ───────────────────────────────────────────────────────────────────────
//  FLEXPROGRESS
//  Tracks THIS specific student's progress in a FlexCategory.
//  Lives in DegreeAudit — not DegreePlan.
//  One FlexProgress entry exists per FlexCategory in the degree plan.
//  Recalculated fresh every time DegreeAudit runs.
// ───────────────────────────────────────────────────────────────────────
struct FlexProgress {
	// Display name matching the FlexCategory being tracked.
	std::string categoryName;

	// Official category code matching the FlexCategory.
	std::string categoryCode;

	// Total hours required to satisfy this category.
	int requiredHours;

	// Hours the student has completed toward this category so far.
	int completedHours;

	// True when completedHours >= requiredHours.
	bool satisfied;

	// The course code the student assigned to satisfy this category.
	// Empty string if the student has not yet assigned a course.
	// Example: "MUSC1033" assigned to Creative Arts (050N).
	std::string satisfiedByCourse;
};