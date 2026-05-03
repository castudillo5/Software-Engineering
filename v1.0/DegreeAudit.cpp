#include "DegreeAudit.h"

// ═══════════════════════════════════════════════════════════════════════
//  DEGREEAUDIT.CPP
//  Implements DegreeAudit — the referee of the advising system.
//  All audit logic is unchanged from the original.
//  The only change is in toJson(): a "categories" array is now appended
//  to the JSON output. home.html iterates over audit.categories to
//  render the requirement progress bars and course checklists.
// ═══════════════════════════════════════════════════════════════════════

// ───────────────────────────────────────────────────────────────────────
//  CONSTRUCTOR
// ───────────────────────────────────────────────────────────────────────

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
	return 0.0;
}

// ───────────────────────────────────────────────────────────────────────
//  PRIVATE HELPER — COMPLETION CHECK
// ───────────────────────────────────────────────────────────────────────

bool DegreeAudit::hasCompletedWithGrade(const Student& student,
	const std::string& courseCode,
	const std::string& minGrade) const {
	for (const AcademicRecord& record : student.getRecords()) {
		if (record.getCourseCode() == courseCode && record.isCompleted()) {
			if (minGrade.empty()) return true;
			return gradeToPoints(record.getGrade()) >= gradeToPoints(minGrade);
		}
	}
	return false;
}

// ───────────────────────────────────────────────────────────────────────
//  CORE METHOD
// ───────────────────────────────────────────────────────────────────────

void DegreeAudit::run(const Student& student, const DegreePlan& plan) {
	fulfilled.clear();
	unfulfilled.clear();
	categoryProgress.clear();
	flexProgress.clear();
	eligibleCourses.clear();
	alerts.clear();

	// Cache pointers so toJson() can look up approved course lists
	// and check per-course completion status when emitting categories.
	lastStudent = &student;
	lastPlan = &plan;

	checkRigidCourses(student, plan);
	checkCategories(student, plan);
	checkFlexCategories(student, plan);
	checkPrereqs(student, plan);
	findEligibleCourses(student, plan);
	calculateCompletion(student);

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

void DegreeAudit::checkRigidCourses(const Student& student, const DegreePlan& plan) {
	for (const std::string& courseCode : plan.getRigidCourses()) {
		bool completed = false;
		for (const AcademicRecord& record : student.getRecords()) {
			if (record.getCourseCode() == courseCode && record.isCompleted()) {
				completed = true;
				break;
			}
		}
		if (completed)
			fulfilled.push_back(courseCode);
		else
			unfulfilled.push_back(courseCode);
	}
}

// ───────────────────────────────────────────────────────────────────────
//  CHECK 2 — CATEGORY REQUIREMENTS
// ───────────────────────────────────────────────────────────────────────

void DegreeAudit::checkCategories(const Student& student, const DegreePlan& plan) {
	for (const CategoryRequirement& cat : plan.getCategoryRequirements()) {
		CategoryProgress progress;
		progress.categoryName = cat.categoryName;
		progress.requiredHours = cat.requiredCreditHours;
		progress.completedHours = 0;
		progress.satisfied = false;

		for (const std::string& mandatory : cat.mandatoryCourses) {
			if (!hasCompletedWithGrade(student, mandatory, ""))
				progress.missingMandatory.push_back(mandatory);
		}

		for (const std::string& elective : cat.electiveCourses) {
			for (const AcademicRecord& record : student.getRecords()) {
				if (record.getCourseCode() == elective && record.isCompleted()) {
					progress.completedHours += record.getCreditHours();
					break;
				}
			}
		}

		progress.satisfied = progress.missingMandatory.empty() &&
			progress.completedHours >= progress.requiredHours;

		for (const std::string& missing : progress.missingMandatory) {
			alerts.push_back(Alert(
				AlertType::MANDATORY_NOT_MET,
				"Required course not completed for " + cat.categoryName + ": " + missing,
				missing
			));
		}

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

void DegreeAudit::checkFlexCategories(const Student& student, const DegreePlan& plan) {
	for (const FlexCategory& flex : plan.getFlexCategories()) {
		FlexProgress progress;
		progress.categoryName = flex.categoryName;
		progress.categoryCode = flex.categoryCode;
		progress.requiredHours = flex.requiredCreditHours;
		progress.completedHours = 0;
		progress.satisfied = false;
		progress.satisfiedByCourse = "";

		for (const AcademicRecord& record : student.getRecords()) {
			if (!record.isCompleted()) continue;
			const std::string& code = record.getCourseCode();
			bool approved = false;
			for (const std::string& approvedCode : flex.approvedCourses) {
				if (approvedCode == code) { approved = true; break; }
			}
			if (!approved) continue;
			if (usedCourses.count(code)) continue;

			progress.completedHours += record.getCreditHours();
			progress.satisfiedByCourse = code;
			usedCourses.insert(code);
			if (progress.completedHours >= progress.requiredHours) break;
		}

		progress.satisfied = progress.completedHours >= progress.requiredHours;

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

void DegreeAudit::checkPrereqs(const Student& student, const DegreePlan& plan) {
	for (const std::string& courseCode : plan.getRigidCourses()) {
		if (!plan.hasPrereq(courseCode)) continue;

		// Only flag prereq problems for courses the student is ACTUALLY
		// trying to take (i.e. has a record for — enrolled or in-progress).
		// Without this guard, every future degree course would generate
		// a PREREQ_NOT_MET alert just because the staircase hasn't been
		// climbed yet, which floods the alerts page with noise.
		bool studentAttempting = false;
		for (const AcademicRecord& rec : student.getRecords()) {
			if (rec.getCourseCode() == courseCode) {
				studentAttempting = true;
				break;
			}
		}
		if (!studentAttempting) continue;

		// Already completed — no prereq alert needed.
		if (hasCompletedWithGrade(student, courseCode, "")) continue;

		const PrereqRule& rule = plan.getPrereqRule(courseCode);
		bool prereqMet = false;

		if (rule.isOr) {
			for (const std::string& prereq : rule.courseCodes) {
				if (hasCompletedWithGrade(student, prereq, rule.minimumGrade)) {
					prereqMet = true;
					break;
				}
			}
		}
		else {
			prereqMet = true;
			for (const std::string& prereq : rule.courseCodes) {
				if (!hasCompletedWithGrade(student, prereq, rule.minimumGrade)) {
					prereqMet = false;
					break;
				}
			}
		}

		if (!prereqMet) {
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

void DegreeAudit::findEligibleCourses(const Student& student, const DegreePlan& plan) {
	for (const std::string& courseCode : plan.getRigidCourses()) {
		if (hasCompletedWithGrade(student, courseCode, "")) continue;
		bool eligible = false;

		if (!plan.hasPrereq(courseCode)) {
			eligible = true;
		}
		else {
			const PrereqRule& rule = plan.getPrereqRule(courseCode);
			if (rule.isOr) {
				for (const std::string& prereq : rule.courseCodes) {
					if (hasCompletedWithGrade(student, prereq, rule.minimumGrade)) {
						eligible = true;
						break;
					}
				}
			}
			else {
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
			// Add to the eligible list so the registration page can use
			// it for recommendations, but DO NOT generate an alert.
			// The registration page already surfaces these via the
			// "Recommended for You" section, so an alert is redundant.
			eligibleCourses.push_back(courseCode);
		}
	}
}

// ───────────────────────────────────────────────────────────────────────
//  CHECK 6 — COMPLETION PERCENTAGE
// ───────────────────────────────────────────────────────────────────────

void DegreeAudit::calculateCompletion(const Student& student) {
	const double totalRequired = 120.0;
	double completed = static_cast<double>(student.getTotalCreditHours());
	completionPercentage = std::min((completed / totalRequired) * 100.0, 100.0);
}

// ───────────────────────────────────────────────────────────────────────
//  FLEX CATEGORY ASSIGNMENT
// ───────────────────────────────────────────────────────────────────────

bool DegreeAudit::assignCourseToFlex(const std::string& courseCode,
	const std::string& categoryCode,
	const Student& student) {
	if (usedCourses.count(courseCode)) {
		alerts.push_back(Alert(
			AlertType::COURSE_REUSED,
			"Course " + courseCode + " has already been used for another requirement.",
			courseCode
		));
		return false;
	}

	for (FlexProgress& progress : flexProgress) {
		if (progress.categoryCode != categoryCode) continue;
		usedCourses.insert(courseCode);
		progress.satisfiedByCourse = courseCode;
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

const std::vector<std::string>& DegreeAudit::getFulfilled()        const { return fulfilled; }
const std::vector<std::string>& DegreeAudit::getUnfulfilled()      const { return unfulfilled; }
const std::vector<CategoryProgress>& DegreeAudit::getCategoryProgress() const { return categoryProgress; }
const std::vector<FlexProgress>& DegreeAudit::getFlexProgress()     const { return flexProgress; }
const std::vector<std::string>& DegreeAudit::getEligibleCourses()  const { return eligibleCourses; }
const std::vector<Alert>& DegreeAudit::getAlerts()           const { return alerts; }
double                                DegreeAudit::getCompletionPercentage() const { return completionPercentage; }

// ───────────────────────────────────────────────────────────────────────
//  SERIALIZATION
//
//  All original fields are preserved (fulfilled, unfulfilled,
//  categoryProgress, flexProgress, eligibleCourses, alerts,
//  completionPercentage).
//
//  NEW: "categories" array — the shape home.html iterates over.
//  Each entry has:
//    name      — display name of the requirement group
//    total     — number of courses required in this group
//    completed — number the student has finished
//    courses   — array of { code, name, completed, inProgress }
//
//  Three groups are always emitted:
//    1. "Core Requirements"  — all rigid courses
//    2. "Category Electives" — categoryProgress entries
//    3. "Gen Ed Requirements"— flexProgress entries
// ───────────────────────────────────────────────────────────────────────
std::string DegreeAudit::toJson() const {
	std::string json = "{";

	// ── fulfilled ──────────────────────────────────────────────────────
	json += "\"fulfilled\":[";
	for (size_t i = 0; i < fulfilled.size(); i++) {
		json += "\"" + fulfilled[i] + "\"";
		if (i < fulfilled.size() - 1) json += ",";
	}
	json += "],";

	// ── unfulfilled ────────────────────────────────────────────────────
	json += "\"unfulfilled\":[";
	for (size_t i = 0; i < unfulfilled.size(); i++) {
		json += "\"" + unfulfilled[i] + "\"";
		if (i < unfulfilled.size() - 1) json += ",";
	}
	json += "],";

	// ── categoryProgress ──────────────────────────────────────────────
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

	// ── flexProgress ──────────────────────────────────────────────────
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

	// ── eligibleCourses ───────────────────────────────────────────────
	json += "\"eligibleCourses\":[";
	for (size_t i = 0; i < eligibleCourses.size(); i++) {
		json += "\"" + eligibleCourses[i] + "\"";
		if (i < eligibleCourses.size() - 1) json += ",";
	}
	json += "],";

	// ── alerts ────────────────────────────────────────────────────────
	json += "\"alerts\":[";
	for (size_t i = 0; i < alerts.size(); i++) {
		json += alerts[i].toJson();
		if (i < alerts.size() - 1) json += ",";
	}
	json += "],";

	// ── completionPercentage ──────────────────────────────────────────
	json += "\"completionPercentage\":" + std::to_string(completionPercentage) + ",";

	// ── categories — the shape home.html iterates over ─────────────────
	// Group 1: Core Requirements — one entry per rigid course,
	// status derived from the fulfilled / unfulfilled lists.
	// Group 2: Category Electives — one entry per CategoryProgress.
	// Group 3: Gen Ed Requirements — one entry per FlexProgress.
	//
	// Each entry shape:
	// {
	//   "name":      string,
	//   "total":     int,      (number of courses / hours required)
	//   "completed": int,      (number of courses / hours done)
	//   "courses": [
	//     { "code": string, "name": string,
	//       "completed": bool, "inProgress": bool }
	//   ]
	// }
	json += "\"categories\":[";

	// ── Group 1: Core Requirements ─────────────────────────────────────
	{
		int totalCourses = (int)(fulfilled.size() + unfulfilled.size());
		int doneCourses = (int)fulfilled.size();

		json += "{";
		json += "\"name\":\"Core Requirements\",";
		json += "\"total\":" + std::to_string(totalCourses) + ",";
		json += "\"completed\":" + std::to_string(doneCourses) + ",";
		json += "\"courses\":[";

		size_t idx = 0;
		// Fulfilled courses — completed = true
		for (const std::string& code : fulfilled) {
			json += "{\"code\":\"" + code + "\",\"name\":\"\","
				"\"completed\":true,\"inProgress\":false}";
			if (idx < fulfilled.size() - 1 || !unfulfilled.empty()) json += ",";
			idx++;
		}
		// Unfulfilled courses — completed = false, inProgress = false
		for (size_t i = 0; i < unfulfilled.size(); i++) {
			json += "{\"code\":\"" + unfulfilled[i] + "\",\"name\":\"\","
				"\"completed\":false,\"inProgress\":false}";
			if (i < unfulfilled.size() - 1) json += ",";
		}
		json += "]}";
	}

	// ── Group 2: Category Electives ───────────────────────────────────
	// For each CategoryRequirement, emit the full list of mandatory +
	// elective courses with each one's status (completed / in-progress /
	// remaining) derived from the student's records. Without this the
	// home-page dropdown for these categories would be empty.
	if (lastPlan && lastStudent) {
		const auto& reqs = lastPlan->getCategoryRequirements();
		const auto& records = lastStudent->getRecords();

		for (size_t r = 0; r < reqs.size(); r++) {
			const CategoryRequirement& cat = reqs[r];

			// Find matching CategoryProgress for the totals.
			int total = cat.requiredCreditHours;
			int done = 0;
			for (const CategoryProgress& cp : categoryProgress) {
				if (cp.categoryName == cat.categoryName) {
					done = cp.completedHours;
					break;
				}
			}

			json += ",{";
			json += "\"name\":\"" + cat.categoryName + "\",";
			json += "\"total\":" + std::to_string(total) + ",";
			json += "\"completed\":" + std::to_string(done) + ",";
			json += "\"courses\":[";

			// Build the full approved course list — mandatory first
			// then electives — and emit each with its status.
			std::vector<std::string> allCodes = cat.mandatoryCourses;
			for (const std::string& e : cat.electiveCourses) {
				allCodes.push_back(e);
			}

			for (size_t i = 0; i < allCodes.size(); i++) {
				const std::string& code = allCodes[i];

				bool isCompleted = false;
				bool isInProgress = false;
				for (const AcademicRecord& rec : records) {
					if (rec.getCourseCode() == code) {
						if (rec.isCompleted()) isCompleted = true;
						else                   isInProgress = true;
						break;
					}
				}

				json += "{\"code\":\"" + code + "\",\"name\":\"\",";
				json += "\"completed\":" + std::string(isCompleted ? "true" : "false") + ",";
				json += "\"inProgress\":" + std::string(isInProgress ? "true" : "false");
				json += "}";
				if (i < allCodes.size() - 1) json += ",";
			}

			json += "]}";
		}
	}

	// ── Group 3: Gen Ed Requirements ──────────────────────────────────
	// Same pattern as Group 2 but for FlexCategory entries — emit every
	// approved course in the category with its status. The student picks
	// from this list to satisfy the category, so the dropdown needs to
	// show all options even when none have been taken yet.
	if (lastPlan && lastStudent) {
		const auto& flexes = lastPlan->getFlexCategories();
		const auto& records = lastStudent->getRecords();

		for (size_t f = 0; f < flexes.size(); f++) {
			const FlexCategory& flex = flexes[f];

			int total = flex.requiredCreditHours;
			int done = 0;
			for (const FlexProgress& fp : flexProgress) {
				if (fp.categoryCode == flex.categoryCode) {
					done = fp.completedHours;
					break;
				}
			}

			json += ",{";
			json += "\"name\":\"" + flex.categoryName + " (" + flex.categoryCode + ")\",";
			json += "\"total\":" + std::to_string(total) + ",";
			json += "\"completed\":" + std::to_string(done) + ",";
			json += "\"courses\":[";

			for (size_t i = 0; i < flex.approvedCourses.size(); i++) {
				const std::string& code = flex.approvedCourses[i];

				bool isCompleted = false;
				bool isInProgress = false;
				for (const AcademicRecord& rec : records) {
					if (rec.getCourseCode() == code) {
						if (rec.isCompleted()) isCompleted = true;
						else                   isInProgress = true;
						break;
					}
				}

				json += "{\"code\":\"" + code + "\",\"name\":\"\",";
				json += "\"completed\":" + std::string(isCompleted ? "true" : "false") + ",";
				json += "\"inProgress\":" + std::string(isInProgress ? "true" : "false");
				json += "}";
				if (i < flex.approvedCourses.size() - 1) json += ",";
			}

			json += "]}";
		}
	}

	json += "]"; // end categories

	json += "}";
	return json;
}