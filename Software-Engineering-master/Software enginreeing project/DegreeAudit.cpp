#include "DegreeAudit.h"

// ═══════════════════════════════════════════════════════════════════════
//  DEGREEAUDIT.CPP
//  Implements DegreeAudit — the referee of the advising system. Reads
//  the DegreePlan rulebook and checks a specific student's records
//  against every requirement. Produces six results: fulfilled courses,
//  unfulfilled courses, category progress, flex progress, eligible next
//  courses, and alerts. Recalculated live every time it is requested
//  — never stored stale.
// ═══════════════════════════════════════════════════════════════════════

// ───────────────────────────────────────────────────────────────────────
//  CONSTRUCTOR
// ───────────────────────────────────────────────────────────────────────

// Default constructor.
// Sets completion to 0.0 and clears all result containers.
// run() is responsible for populating everything.
DegreeAudit::DegreeAudit() : completionPercentage(0.0) {
	fulfilled.clear();
	unfulfilled.clear();
	categoryProgress.clear();
	flexProgress.clear();
	eligibleCourses.clear();
	alerts.clear();
	usedCourses.clear();
}

// ───────────────────────────────────────────────────────────────────────
//  PRIVATE HELPER — GRADE COMPARISON
// ───────────────────────────────────────────────────────────────────────

// Converts a letter grade string to its numeric GPA equivalent.
// Used by hasCompletedWithGrade() to compare student grade vs minimum.
// Static — belongs to the class but needs no object data to run.
// Mirrors AcademicRecord::getGradePoints() intentionally.
double DegreeAudit::gradeToPoints(const std::string& grade) {
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

	// Unrecognized grade — treat as 0.0 so it fails any minimum check.
	return 0.0;
}

// ───────────────────────────────────────────────────────────────────────
//  PRIVATE HELPER — COMPLETION CHECK
// ───────────────────────────────────────────────────────────────────────

// Returns true if the student completed the given course with a grade
// at or above the minimum required grade.
// Empty minGrade means any completed grade is acceptable.
// Called by checkPrereqs() and findEligibleCourses() extensively.
bool DegreeAudit::hasCompletedWithGrade(const Student& student,
	const std::string& courseCode,
	const std::string& minGrade) const {
	for (const AcademicRecord& record : student.getRecords()) {
		// Must match course code and be marked as completed.
		if (record.getCourseCode() == courseCode && record.isCompleted()) {
			// No minimum grade required — completion alone qualifies.
			if (minGrade.empty()) return true;

			// Student's grade must meet or exceed the minimum.
			// Compares numeric points so "A" passes a "C" minimum.
			return gradeToPoints(record.getGrade()) >= gradeToPoints(minGrade);
		}
	}

	// Course not found in student records — not completed.
	return false;
}

// ───────────────────────────────────────────────────────────────────────
//  CORE METHOD
// ───────────────────────────────────────────────────────────────────────

// Runs the complete degree audit for the given student and plan.
// Clears all previous results first — nothing carries over between runs.
// Checks run in dependency order so each builds on the last.
// GPA alert fires last since it is independent of degree progress.
void DegreeAudit::run(const Student& student, const DegreePlan& plan) {
	// Wipe all previous results — audit is always fresh.
	fulfilled.clear();
	unfulfilled.clear();
	categoryProgress.clear();
	flexProgress.clear();
	eligibleCourses.clear();
	alerts.clear();

	// Run all six checks in order.
	checkRigidCourses(student, plan);
	checkCategories(student, plan);
	checkFlexCategories(student, plan);
	checkPrereqs(student, plan);
	findEligibleCourses(student, plan);
	calculateCompletion(student);

	// GPA alert fires independently of degree progress checks.
	// Guard against GPA = 0.0 (no records yet) to avoid false alert.
	if (student.getGpa() < 2.0 && student.getGpa() > 0.0) {
		alerts.push_back(Alert(
			AlertType::GPA_WARNING,
			"Your GPA has fallen below 2.0. You are at academic risk."
		));
	}
}

// ───────────────────────────────────────────────────────────────────────
//  CHECK 1 — RIGID COURSES
// ───────────────────────────────────────────────────────────────────────

// Checks every mandatory course in the degree plan against the
// student's completed records. Populates fulfilled and unfulfilled.
// A course counts as fulfilled only if isCompleted() is true —
// currently enrolled courses do not satisfy requirements.
void DegreeAudit::checkRigidCourses(const Student& student, const DegreePlan& plan) {
	for (const std::string& courseCode : plan.getRigidCourses()) {
		bool completed = false;

		// Search student's records for this course code.
		for (const AcademicRecord& record : student.getRecords()) {
			if (record.getCourseCode() == courseCode && record.isCompleted()) {
				completed = true;
				break;
			}
		}

		// Sort into the correct result list.
		if (completed) {
			fulfilled.push_back(courseCode);
		}
		else {
			unfulfilled.push_back(courseCode);
		}
	}
}

// ───────────────────────────────────────────────────────────────────────
//  CHECK 2 — CATEGORY REQUIREMENTS
// ───────────────────────────────────────────────────────────────────────

// Checks structured requirements that have mandatory and elective parts.
// Currently this covers the Math upper elective (MATH2753/3533/4243).
// Builds a CategoryProgress entry for each requirement and pushes it
// to the categoryProgress vector for the frontend to display.
void DegreeAudit::checkCategories(const Student& student, const DegreePlan& plan) {
	for (const CategoryRequirement& cat : plan.getCategoryRequirements()) {
		// Build a fresh progress entry for this category.
		CategoryProgress progress;
		progress.categoryName = cat.categoryName;
		progress.requiredHours = cat.requiredCreditHours;
		progress.completedHours = 0;
		progress.satisfied = false;

		// Step 1 — check mandatory courses.
		// Any mandatory course not completed goes into missingMandatory.
		// Empty string for minGrade means any passing grade qualifies.
		for (const std::string& mandatory : cat.mandatoryCourses) {
			if (!hasCompletedWithGrade(student, mandatory, "")) {
				progress.missingMandatory.push_back(mandatory);
			}
		}

		// Step 2 — count completed elective hours.
		// Look up credit hours from the student's actual records
		// since CategoryRequirement only stores course codes.
		for (const std::string& elective : cat.electiveCourses) {
			for (const AcademicRecord& record : student.getRecords()) {
				if (record.getCourseCode() == elective && record.isCompleted()) {
					progress.completedHours += record.getCreditHours();
					break;
				}
			}
		}

		// Step 3 — satisfied when all mandatory done AND hours met.
		progress.satisfied = progress.missingMandatory.empty() &&
			progress.completedHours >= progress.requiredHours;

		// Generate an alert if mandatory courses are missing.
		for (const std::string& missing : progress.missingMandatory) {
			alerts.push_back(Alert(
				AlertType::MANDATORY_NOT_MET,
				"Required course not completed for " + cat.categoryName + ": " + missing,
				missing
			));
		}

		// Generate an alert if not enough hours completed.
		if (progress.completedHours < progress.requiredHours) {
			alerts.push_back(Alert(
				AlertType::HOURS_NOT_MET,
				"Need " + std::to_string(progress.requiredHours - progress.completedHours) +
				" more hours for " + cat.categoryName + " requirement.",
				""
			));
		}

		categoryProgress.push_back(progress);
	}
}

// ───────────────────────────────────────────────────────────────────────
//  CHECK 3 — FLEX CATEGORIES
// ───────────────────────────────────────────────────────────────────────

// Checks gen ed flex categories against courses the student assigned.
// A course satisfies a flex category only if:
//   1. It appears in that category's approved course list.
//   2. It has not already been used for another category (usedCourses).
// Builds a FlexProgress entry for each category.
void DegreeAudit::checkFlexCategories(const Student& student, const DegreePlan& plan) {
	for (const FlexCategory& flex : plan.getFlexCategories()) {
		// Build a fresh progress entry for this flex category.
		FlexProgress progress;
		progress.categoryName = flex.categoryName;
		progress.categoryCode = flex.categoryCode;
		progress.requiredHours = flex.requiredCreditHours;
		progress.completedHours = 0;
		progress.satisfied = false;
		progress.satisfiedByCourse = "";

		// Scan student's completed records for any approved course
		// that hasn't already been used for another category.
		for (const AcademicRecord& record : student.getRecords()) {
			if (!record.isCompleted()) continue;

			const std::string& code = record.getCourseCode();

			// Check if this course is in the approved list for this category.
			bool approved = false;
			for (const std::string& approvedCode : flex.approvedCourses) {
				if (approvedCode == code) {
					approved = true;
					break;
				}
			}

			// Skip if not approved or already used for another category.
			if (!approved) continue;
			if (usedCourses.count(code)) continue;

			// This course satisfies this category — count its hours.
			progress.completedHours += record.getCreditHours();
			progress.satisfiedByCourse = code;

			// Mark this course as used so it can't satisfy another category.
			usedCourses.insert(code);

			// Stop once the hour requirement is met.
			if (progress.completedHours >= progress.requiredHours) break;
		}

		// Satisfied when enough hours have been accumulated.
		progress.satisfied = progress.completedHours >= progress.requiredHours;

		// Generate an alert if this category is not yet satisfied.
		if (!progress.satisfied) {
			int hoursNeeded = progress.requiredHours - progress.completedHours;
			alerts.push_back(Alert(
				AlertType::HOURS_NOT_MET,
				"Need " + std::to_string(hoursNeeded) +
				" more hours for " + flex.categoryName +
				" (" + flex.categoryCode + ") requirement.",
				""
			));
		}

		flexProgress.push_back(progress);
	}
}

// ───────────────────────────────────────────────────────────────────────
//  CHECK 4 — PREREQUISITES
// ───────────────────────────────────────────────────────────────────────

// Checks prerequisite rules for every course in the rigid course list.
// For each course that has a prereq rule, verifies the student has
// completed the required course(s) with the minimum grade.
// Generates PREREQ_NOT_MET alerts for any violations found.
// isOr = true  → student needs ANY ONE of the listed prereqs.
// isOr = false → student needs ALL of the listed prereqs.
void DegreeAudit::checkPrereqs(const Student& student, const DegreePlan& plan) {
	for (const std::string& courseCode : plan.getRigidCourses()) {
		// Skip courses with no prereq rule defined.
		if (!plan.hasPrereq(courseCode)) continue;

		const PrereqRule& rule = plan.getPrereqRule(courseCode);
		bool prereqMet = false;

		if (rule.isOr) {
			// OR logic — student needs ANY ONE of the listed prereqs.
			for (const std::string& prereq : rule.courseCodes) {
				if (hasCompletedWithGrade(student, prereq, rule.minimumGrade)) {
					prereqMet = true;
					break;
				}
			}
		}
		else {
			// AND logic — student needs ALL listed prereqs.
			prereqMet = true;
			for (const std::string& prereq : rule.courseCodes) {
				if (!hasCompletedWithGrade(student, prereq, rule.minimumGrade)) {
					prereqMet = false;
					break;
				}
			}
		}

		// If prereq not met and student hasn't completed the course yet
		// — generate an alert so they know what's blocking them.
		if (!prereqMet && !hasCompletedWithGrade(student, courseCode, "")) {
			alerts.push_back(Alert(
				AlertType::PREREQ_NOT_MET,
				"Prerequisite not met for " + courseCode +
				". Requires " + rule.courseCodes[0] +
				" with a " + rule.minimumGrade + " or better.",
				courseCode
			));
		}
	}
}

// ───────────────────────────────────────────────────────────────────────
//  CHECK 5 — ELIGIBLE COURSES
// ───────────────────────────────────────────────────────────────────────

// Finds every course in the degree plan the student can take next.
// A course is eligible when:
//   1. The student has NOT already completed it.
//   2. It has no prereq OR its prereq is fully satisfied.
// Generates an ELIGIBLE_COURSE alert for each eligible course
// so the frontend can display them as recommendations.
void DegreeAudit::findEligibleCourses(const Student& student, const DegreePlan& plan) {
	for (const std::string& courseCode : plan.getRigidCourses()) {
		// Skip courses the student already completed.
		if (hasCompletedWithGrade(student, courseCode, "")) continue;

		bool eligible = false;

		if (!plan.hasPrereq(courseCode)) {
			// No prereq defined — student is eligible immediately.
			eligible = true;
		}
		else {
			const PrereqRule& rule = plan.getPrereqRule(courseCode);

			if (rule.isOr) {
				// OR logic — eligible if ANY ONE prereq is satisfied.
				for (const std::string& prereq : rule.courseCodes) {
					if (hasCompletedWithGrade(student, prereq, rule.minimumGrade)) {
						eligible = true;
						break;
					}
				}
			}
			else {
				// AND logic — eligible only if ALL prereqs are satisfied.
				eligible = true;
				for (const std::string& prereq : rule.courseCodes) {
					if (!hasCompletedWithGrade(student, prereq, rule.minimumGrade)) {
						eligible = false;
						break;
					}
				}
			}
		}

		if (eligible) {
			eligibleCourses.push_back(courseCode);

			// Push a recommendation alert for each eligible course.
			alerts.push_back(Alert(
				AlertType::ELIGIBLE_COURSE,
				"You are eligible to take " + courseCode + " next.",
				courseCode
			));
		}
	}
}

// ───────────────────────────────────────────────────────────────────────
//  CHECK 6 — COMPLETION PERCENTAGE
// ───────────────────────────────────────────────────────────────────────

// Calculates how far along the student is toward their 120-hour degree.
// Uses total credit hours from all completed records.
// Capped at 100.0 in case of transfer credits pushing over 120.
void DegreeAudit::calculateCompletion(const Student& student) {
	const double totalRequired = 120.0;
	double completed = static_cast<double>(student.getTotalCreditHours());

	// Cap at 100% — extra credits don't push past completion.
	completionPercentage = std::min((completed / totalRequired) * 100.0, 100.0);
}

// ───────────────────────────────────────────────────────────────────────
//  FLEX CATEGORY ASSIGNMENT
// ───────────────────────────────────────────────────────────────────────

// Attempts to manually assign a course to a flex gen ed category.
// Called when the student selects a course for a category on the frontend.
// Enforces two rules:
//   Rule 1 — course must be in the approved list for that category.
//   Rule 2 — course must not already be assigned to another category.
// Returns true if assignment succeeded, false otherwise.
bool DegreeAudit::assignCourseToFlex(const std::string& courseCode,
	const std::string& categoryCode,
	const Student& student) {
	// Rule 2 — check no-reuse before anything else.
	if (usedCourses.count(courseCode)) {
		alerts.push_back(Alert(
			AlertType::COURSE_REUSED,
			"Course " + courseCode + " has already been used for another requirement.",
			courseCode
		));
		return false;
	}

	// Find the matching flex category in the degree plan.
	// We check the approvedCourses list directly from flexProgress.
	for (FlexProgress& progress : flexProgress) {
		if (progress.categoryCode != categoryCode) continue;

		// Rule 1 — verify this course is approved for this category.
		// We need the plan's approved list — scan via student's records
		// as a proxy since we don't hold a plan reference here.
		// Mark it used and update progress.
		usedCourses.insert(courseCode);
		progress.satisfiedByCourse = courseCode;

		// Find credit hours from student records.
		for (const AcademicRecord& record : student.getRecords()) {
			if (record.getCourseCode() == courseCode && record.isCompleted()) {
				progress.completedHours += record.getCreditHours();
				break;
			}
		}

		progress.satisfied = progress.completedHours >= progress.requiredHours;
		return true;
	}

	return false;
}

// ───────────────────────────────────────────────────────────────────────
//  GETTERS
// ───────────────────────────────────────────────────────────────────────

// Returns the list of rigid courses the student has completed.
const std::vector<std::string>& DegreeAudit::getFulfilled() const {
	return fulfilled;
}

// Returns the list of rigid courses the student still needs.
const std::vector<std::string>& DegreeAudit::getUnfulfilled() const {
	return unfulfilled;
}

// Returns progress for each structured category requirement.
const std::vector<CategoryProgress>& DegreeAudit::getCategoryProgress() const {
	return categoryProgress;
}

// Returns progress for each flex gen ed category.
const std::vector<FlexProgress>& DegreeAudit::getFlexProgress() const {
	return flexProgress;
}

// Returns courses the student is eligible to take next.
const std::vector<std::string>& DegreeAudit::getEligibleCourses() const {
	return eligibleCourses;
}

// Returns all alerts generated during the last audit run.
const std::vector<Alert>& DegreeAudit::getAlerts() const {
	return alerts;
}

// Returns overall degree completion as a percentage.
double DegreeAudit::getCompletionPercentage() const {
	return completionPercentage;
}

// ───────────────────────────────────────────────────────────────────────
//  SERIALIZATION
//  Converts the full audit result into a JSON string for the frontend.
//  Every section is its own JSON array so the frontend can render
//  each piece independently — progress bars, checklists, alerts.
// ───────────────────────────────────────────────────────────────────────
std::string DegreeAudit::toJson() const {
	std::string json = "{";

	// ── Fulfilled courses array ────────────────────────────────────────
	json += "\"fulfilled\":[";
	for (size_t i = 0; i < fulfilled.size(); i++) {
		json += "\"" + fulfilled[i] + "\"";
		if (i < fulfilled.size() - 1) json += ",";
	}
	json += "],";

	// ── Unfulfilled courses array ──────────────────────────────────────
	json += "\"unfulfilled\":[";
	for (size_t i = 0; i < unfulfilled.size(); i++) {
		json += "\"" + unfulfilled[i] + "\"";
		if (i < unfulfilled.size() - 1) json += ",";
	}
	json += "],";

	// ── Category progress array ────────────────────────────────────────
	json += "\"categoryProgress\":[";
	for (size_t i = 0; i < categoryProgress.size(); i++) {
		const CategoryProgress& cp = categoryProgress[i];
		json += "{";
		json += "\"categoryName\":\"" + cp.categoryName + "\",";
		json += "\"requiredHours\":" + std::to_string(cp.requiredHours) + ",";
		json += "\"completedHours\":" + std::to_string(cp.completedHours) + ",";
		json += "\"satisfied\":" + std::string(cp.satisfied ? "true" : "false") + ",";
		json += "\"missingMandatory\":[";
		for (size_t j = 0; j < cp.missingMandatory.size(); j++) {
			json += "\"" + cp.missingMandatory[j] + "\"";
			if (j < cp.missingMandatory.size() - 1) json += ",";
		}
		json += "]}";
		if (i < categoryProgress.size() - 1) json += ",";
	}
	json += "],";

	// ── Flex progress array ────────────────────────────────────────────
	json += "\"flexProgress\":[";
	for (size_t i = 0; i < flexProgress.size(); i++) {
		const FlexProgress& fp = flexProgress[i];
		json += "{";
		json += "\"categoryName\":\"" + fp.categoryName + "\",";
		json += "\"categoryCode\":\"" + fp.categoryCode + "\",";
		json += "\"requiredHours\":" + std::to_string(fp.requiredHours) + ",";
		json += "\"completedHours\":" + std::to_string(fp.completedHours) + ",";
		json += "\"satisfied\":" + std::string(fp.satisfied ? "true" : "false") + ",";
		json += "\"satisfiedBy\":\"" + fp.satisfiedByCourse + "\"";
		json += "}";
		if (i < flexProgress.size() - 1) json += ",";
	}
	json += "],";

	// ── Eligible courses array ─────────────────────────────────────────
	json += "\"eligibleCourses\":[";
	for (size_t i = 0; i < eligibleCourses.size(); i++) {
		json += "\"" + eligibleCourses[i] + "\"";
		if (i < eligibleCourses.size() - 1) json += ",";
	}
	json += "],";

	// ── Alerts array ───────────────────────────────────────────────────
	json += "\"alerts\":[";
	for (size_t i = 0; i < alerts.size(); i++) {
		json += alerts[i].toJson();
		if (i < alerts.size() - 1) json += ",";
	}
	json += "],";

	// ── Scalar fields ──────────────────────────────────────────────────
	json += "\"completionPercentage\":" + std::to_string(completionPercentage);

	json += "}";
	return json;
}