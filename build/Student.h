#pragma once
#include <string>
#include <vector>
#include "AcademicRecord.h"
#include "Alert.h"

class Student {
private:
	// decl
	std::string name;
	std::string major;
	double gpa;
	int totalCreditHours;

	std::vector<AcademicRecord> records;
	std::vector<Alert> alerts;

	void calculateGPA();

public:
	// Constructors
	Student();
	Student(const std::string& name, const std::string& major);

	// Getters
	std::string getName() const;
	std::string getMajor() const;
	double getGpa() const;
	int getTotalCreditHours() const;

	const std::vector<AcademicRecord>& getRecords() const;
	std::vector<AcademicRecord>& getRecords();

	const std::vector<Alert>& getAlerts() const;
	std::vector<Alert>& getAlerts();

	// Setters
	void setName(const std::string& name);
	void setMajor(const std::string& major);

	// Core behaviors
	void addRecord(const AcademicRecord& record);
	void removeRecord(const std::string& courseCode);
	void updateRecord(const std::string& courseCode, const std::string& newGrade);

	void addAlert(const Alert& alert);
	void clearAlerts();

	// Serialization
	std::string toJson() const;
};