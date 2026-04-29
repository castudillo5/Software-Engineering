#pragma once
#include <string>
#include <vector>
#include <set>
#include "Structs.h"
#include "Student.h"
#include "DegreePlan.h"
#include "Alert.h"

// ═══════════════════════════════════════════════════════════════════════
//  DEGREEAUDIT.H
//  The referee of the advising system. Reads the DegreePlan rulebook
//  and checks a specific student's records against every requirement.
//  Produces six results: fulfilled courses, unfulfilled courses,
//  category progress, flex progress, eligible next courses, and alerts.
//  Recalculated live every time it is requested — never stored stale.
// ═══════════════════════════════════════════════════════════════════════

class DegreeAudit {
private:
	// ── Results — populated fresh every time run() is called ───────────

	// Rigid courses the student has successfully completed.
	std::vector<std::string> fulfilled;

	// Rigid courses the student still needs to complete.
	std::vector<std::string> unfulfilled;

	// Progress report for each CategoryRequirement (Math, Physics etc).
	std::vector<CategoryProgress> categoryProgress;

	// Progress report for each FlexCategory (gen ed requirements).
	std::vector<FlexProgress> flexProgress;

	// Courses the student is currently eligible to take next.
	// Prereqs satisfied, not yet completed, in their degree plan.
	std::vector<std::string> eligibleCourses;

	// Alerts generated during this audit run.
	// Includes GPA warnings, prereq failures, and recommendations.
	std::vector<Alert> alerts;

	// Percentage of total degree requirements completed.
	// Calculated from completed hours vs total required hours (120).
	double completionPercentage;

	// Tracks which courses have already been assigned to a flex category.
	// Enforces the no-reuse rule — one course satisfies one requirement.
	std::set<std::string> usedCourses;

	// ── Private helpers — each checks one aspect of the degree ─────────

	// Checks which rigid courses are fulfilled vs unfulfilled.
	void checkRigidCourses(const Student& student, const DegreePlan& plan);

	// Checks structured category requirements (Math upper elective).
	void checkCategories(const Student& student, const DegreePlan& plan);

	// Checks flex gen ed categories against assigned courses.
	void checkFlexCategories(const Student& student, const DegreePlan& plan);

	// Checks prerequisite rules for every course in the degree plan.
	// Generates PREREQ_NOT_MET alerts where needed.
	void checkPrereqs(const Student& student, const DegreePlan& plan);

	// Finds courses the student is eligible to take next.
	// Generates ELIGIBLE_COURSE alerts as recommendations.
	void findEligibleCourses(const Student& student, const DegreePlan& plan);

	// Calculates overall degree completion percentage.
	// Based on completed credit hours vs 120 total required.
	void calculateCompletion(const Student& student);

	// Returns true if the student completed the given course
	// with a grade at or above the minimum required grade.
	// Empty minGrade means any completed grade is acceptable.
	bool hasCompletedWithGrade(const Student& student,
		const std::string& courseCode,
		const std::string& minGrade) const;

	// Converts a letter grade string to its numeric GPA equivalent.
	// Used by hasCompletedWithGrade() to compare grades numerically.
	// Static — needs no object data, just converts a string to a number.
	static double gradeToPoints(const std::string& grade);

public:
	// ───────────────────────────────────────────────────────────────────
	//  CONSTRUCTOR
	// ───────────────────────────────────────────────────────────────────

	// Default constructor.
	// Initializes all results to empty — run() populates them.
	DegreeAudit();

	// ───────────────────────────────────────────────────────────────────
	//  CORE METHOD
	// ───────────────────────────────────────────────────────────────────

	// Runs the full degree audit for the given student against the plan.
	// Clears all previous results first then runs all six checks in order:
	//   1. checkRigidCourses()
	//   2. checkCategories()
	//   3. checkFlexCategories()
	//   4. checkPrereqs()
	//   5. findEligibleCourses()
	//   6. calculateCompletion()
	void run(const Student& student, const DegreePlan& plan);

	// ───────────────────────────────────────────────────────────────────
	//  GETTERS
	// ───────────────────────────────────────────────────────────────────

	// Returns the list of rigid courses the student has completed.
	const std::vector<std::string>& getFulfilled() const;

	// Returns the list of rigid courses still needed.
	const std::vector<std::string>& getUnfulfilled() const;

	// Returns progress for each structured category requirement.
	const std::vector<CategoryProgress>& getCategoryProgress() const;

	// Returns progress for each flex gen ed category.
	const std::vector<FlexProgress>& getFlexProgress() const;

	// Returns courses the student is eligible to take next.
	const std::vector<std::string>& getEligibleCourses() const;

	// Returns all alerts generated during the last audit run.
	const std::vector<Alert>& getAlerts() const;

	// Returns the overall degree completion percentage.
	double getCompletionPercentage() const;

	// ───────────────────────────────────────────────────────────────────
	//  FLEX CATEGORY ASSIGNMENT
	// ───────────────────────────────────────────────────────────────────

	// Attempts to assign a course to a flex gen ed category.
	// Enforces two rules:
	//   1. Course must be in the approved list for that category.
	//   2. Course must not already be used for another category.
	// Returns true if assignment succeeded, false if either rule failed.
	// Generates a COURSE_REUSED alert if rule 2 is violated.
	bool assignCourseToFlex(const std::string& courseCode,
		const std::string& categoryCode,
		const Student& student);

	// ───────────────────────────────────────────────────────────────────
	//  SERIALIZATION
	// ───────────────────────────────────────────────────────────────────

	// Converts the full audit result to a JSON string for the frontend.
	// Includes fulfilled, unfulfilled, category progress, flex progress,
	// eligible courses, alerts, and completion percentage.
	std::string toJson() const;
};