/*
File rules.h

Copyright 2002-2003 Lalescu Liviu.

This file is part of FET.

FET is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

FET is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with FET; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef RULES_H
#define RULES_H

#include "timetable_defs.h"
#include "timeconstraint.h"
#include "spaceconstraint.h"
#include "activity.h"
#include "studentsset.h"
#include "teacher.h"
#include "subject.h"
#include "activitytag.h"
#include "room.h"
#include "building.h"

#include "matrix.h"

#include <QSet>

#include <QString>

#include <QCoreApplication>

class FakeString
/*
fake string, so that the output log is not too large
*/
{
public:
	FakeString();

	void operator=(const QString& other);
	void operator=(const char* str);
	void operator+=(const QString& other);
	void operator+=(const char* str);
};

class QDomElement;

/**
This class contains all the information regarding
the institution: teachers, students, activities, constraints, etc.
*/
class Rules{
	Q_DECLARE_TR_FUNCTIONS(Rules)

public:
	/**
	The name of the institution
	*/
	QString institutionName;
	
	/**
	The comments
	*/
	QString comments;

	/**
	The number of hours per day
	*/
	int nHoursPerDay;

	/**
	The number of days per week
	*/
	int nDaysPerWeek;

	/**
	The days of the week (names)
	*/
	QString daysOfTheWeek[MAX_DAYS_PER_WEEK];

	/**
	The hours of the day (names). This includes also the last hour (+1)
	*/
	QString hoursOfTheDay[MAX_HOURS_PER_DAY+1];

	/**
	The number of hours per week
	*/
	int nHoursPerWeek;

	/**
	The list of teachers
	*/
	TeachersList teachersList;

	/**
	The list of subjects
	*/
	SubjectsList subjectsList;

	/**
	The list of activity tags
	*/
	ActivityTagsList activityTagsList;

	/**
	The list of students (groups and subgroups included).
	Remember that every identifier (year, group or subgroup) must be UNIQUE.
	*/
	StudentsYearsList yearsList;

	/**
	The list of activities
	*/
	ActivitiesList activitiesList;

	/**
	The list of rooms
	*/
	RoomsList roomsList;

	/**
	The list of buildings
	*/
	BuildingsList buildingsList;

	/**
	The list of time constraints
	*/
	TimeConstraintsList timeConstraintsList;

	/**
	The list of space constraints
	*/
	SpaceConstraintsList spaceConstraintsList;
	
	/*
	The following variables contain redundant data and are used internally
	*/
	////////////////////////////////////////////////////////////////////////
	int nInternalTeachers;
	Matrix1D<Teacher*> internalTeachersList;

	int nInternalSubjects;
	Matrix1D<Subject*> internalSubjectsList;

	int nInternalActivityTags;
	Matrix1D<ActivityTag*> internalActivityTagsList;

	int nInternalSubgroups;
	Matrix1D<StudentsSubgroup*> internalSubgroupsList;
	
	StudentsYearsList augmentedYearsList;

	/**
	Here will be only the active activities.
	
	For speed, I used here not pointers, but static copies.
	*/
	int nInternalActivities;
	Matrix1D<Activity> internalActivitiesList;
	
	QSet<int> inactiveActivities;
	
	Matrix1D<QList<int> > activitiesForSubject;

	int nInternalRooms;
	Matrix1D<Room*> internalRoomsList;

	int nInternalBuildings;
	Matrix1D<Building*> internalBuildingsList;

	int nInternalTimeConstraints;
	Matrix1D<TimeConstraint*> internalTimeConstraintsList;

	int nInternalSpaceConstraints;
	Matrix1D<SpaceConstraint*> internalSpaceConstraintsList;

	/*
	///////////////////////////////////////////////////////////////////////
	*/

	/**
	True if the rules have been initialized in some way (new or loaded).
	*/
	bool initialized;

	/**
	True if the internal structure was computed.
	*/
	bool internalStructureComputed;

	/**
	Initializes the rules (empty)
	*/
	void init();

	/**
	Internal structure initializer.
	<p>
	After any modification of the activities or students or teachers
	or constraints, you need to call this subroutine
	*/
	bool computeInternalStructure();

	/**
	Terminator - basically clears the memory for the constraints.
	*/
	void kill();

	Rules();

	~Rules();
	
	void setInstitutionName(const QString& newInstitutionName);
	
	void setComments(const QString& newComments);

	/**
	Adds a new teacher
	(if not already in the list).
	Returns false/true (unsuccessful/successful).
	*/
	bool addTeacher(Teacher* teacher);

	/*when reading rules, faster*/
	bool addTeacherFast(Teacher* teacher);

	/**
	Returns the index of this teacher in the teachersList,
	or -1 for inexistent teacher.
	*/
	int searchTeacher(const QString& teacherName);

	/**
	Removes this teacher and all related activities and constraints.
	It returns false on failure. If successful, returns true.
	*/
	bool removeTeacher(const QString& teacherName);

	/**
	Modifies (renames) this teacher and takes care of all related activities and constraints.
	Returns true on success, false on failure (if not found)
	*/
	bool modifyTeacher(const QString& initialTeacherName, const QString& finalTeacherName);

	/**
	A function to sort the teachers alphabetically
	*/
	void sortTeachersAlphabetically();

	/**
	Adds a new subject
	(if not already in the list).
	Returns false/true (unsuccessful/successful).
	*/
	bool addSubject(Subject* subject);

	/*
	When reading rules, faster
	*/
	bool addSubjectFast(Subject* subject);

	/**
	Returns the index of this subject in the subjectsList,
	or -1 if not found.
	*/
	int searchSubject(const QString& subjectName);

	/**
	Removes this subject and all related activities and constraints.
	It returns false on failure.
	If successful, returns true.
	*/
	bool removeSubject(const QString& subjectName);

	/**
	Modifies (renames) this subject and takes care of all related activities and constraints.
	Returns true on success, false on failure (if not found)
	*/
	bool modifySubject(const QString& initialSubjectName, const QString& finalSubjectName);

	/**
	A function to sort the subjects alphabetically
	*/
	void sortSubjectsAlphabetically();

	/**
	Adds a new activity tag to the list of activity tags
	(if not already in the list).
	Returns false/true (unsuccessful/successful).
	*/
	bool addActivityTag(ActivityTag* activityTag);

	/*
	When reading rules, faster
	*/
	bool addActivityTagFast(ActivityTag* activityTag);

	/**
	Returns the index of this activity tag in the activityTagsList,
	or -1 if not found.
	*/
	int searchActivityTag(const QString& activityTagName);

	/**
	Removes this activity tag. In the list of activities, the activity tag will 
	be removed from all activities which posess it.
	It returns false on failure.
	If successful, returns true.
	*/
	bool removeActivityTag(const QString& activityTagName);

	/**
	Modifies (renames) this activity tag and takes care of all related activities.
	Returns true on success, false on failure (if not found)
	*/
	bool modifyActivityTag(const QString& initialActivityTagName, const QString& finalActivityTagName);

	/**
	A function to sort the activity tags alphabetically
	*/
	void sortActivityTagsAlphabetically();

	/**
	Returns a pointer to the structure containing this student set
	(year, group or subgroup) or NULL.
	*/
	StudentsSet* searchStudentsSet(const QString& setName);
	
	StudentsSet* searchAugmentedStudentsSet(const QString& setName);
	
	/**
	True if the students sets contain one common subgroup.
	This function is used in constraints isRelatedToStudentsSet
	*/
	bool setsShareStudents(const QString& studentsSet1, const QString& studentsSet2);

	/**
	Adds a new year of study to the academic structure
	*/
	bool addYear(StudentsYear* year);
	
	/*
	When reading rules, faster
	*/
	bool addYearFast(StudentsYear* year);

	bool removeYear(const QString& yearName);

	/**
	Returns -1 if not found or the index of this year in the years list
	*/
	int searchYear(const QString& yearName);

	int searchAugmentedYear(const QString& yearName);

	/**
	Modifies this students year (name, number of students) and takes care of all related 
	activities and constraints.	Returns true on success, false on failure (if not found)
	*/
	bool modifyYear(const QString& initialYearName, const QString& finalYearName, int finalNumberOfStudents);
	
	/**
	A function to sort the years alphabetically
	*/
	void sortYearsAlphabetically();

	/**
	Adds a new group in a certain year of study to the academic structure
	*/
	bool addGroup(const QString& yearName, StudentsGroup* group);
	
	/*
	When reading rules, faster
	*/
	bool addGroupFast(StudentsYear* year, StudentsGroup* group);

	bool removeGroup(const QString& yearName, const QString& groupName);

	/**
	Returns -1 if not found or the index of this group in the groups list
	of this year.
	*/
	int searchGroup(const QString& yearName, const QString& groupName);

	int searchAugmentedGroup(const QString& yearName, const QString& groupName);

	/**
	Modifies this students group (name, number of students) and takes care of all related 
	activities and constraints.	Returns true on success, false on failure (if not found)
	*/
	bool modifyGroup(const QString& yearName, const QString& initialGroupName, const QString& finalGroupName, int finalNumberOfStudents);
	
	/**
	A function to sort the groups of this year alphabetically
	*/
	void sortGroupsAlphabetically(const QString& yearName);

	/**
	Adds a new subgroup to a certain group in a certain year of study to
	the academic structure
	*/
	bool addSubgroup(const QString& yearName, const QString& groupName, StudentsSubgroup* subgroup);

	/*
	When reading rules, faster
	*/
	bool addSubgroupFast(StudentsYear* year, StudentsGroup* group, StudentsSubgroup* subgroup);

	bool removeSubgroup(const QString& yearName, const QString& groupName, const QString& subgroupName);

	/**
	Returns -1 if not found or the index of the subgroup in the list of subgroups of this group
	*/
	int searchSubgroup(const QString& yearName, const QString& groupName, const QString& subgroupName);

	int searchAugmentedSubgroup(const QString& yearName, const QString& groupName, const QString& subgroupName);

	/**
	Modifies this students subgroup (name, number of students) and takes care of all related 
	activities and constraints.	Returns true on success, false on failure (if not found)
	*/
	bool modifySubgroup(const QString& yearName, const QString& groupName, const QString& initialSubgroupName, const QString& finalSubgroupName, int finalNumberOfStudents);
	
	/**
	A function to sort the subgroups of this group alphabetically
	*/
	void sortSubgroupsAlphabetically(const QString& yearName, const QString& groupName);
	
	/**
	Adds a new indivisible activity (not split) to the list of activities.
	(It can add a subactivity of a split activity)
	Returns true if successful or false if the maximum
	number of activities was reached.
	*/
	bool addSimpleActivity(
		int _id,
		int _activityGroupId,
		const QStringList& _teachersNames,
		const QString& _subjectName,
		const QStringList& _activityTagsNames,
		const QStringList& _studentsNames,
		int _duration, /*duration, in hours*/
		int _totalDuration,
		bool _active,
		bool _computeNTotalStudents,
		int _nTotalStudents);

	/*
	Faster, when reading rules (no need to recompute the number of students in activity constructor
	*/
	bool addSimpleActivityRulesFast(
		int _id,
		int _activityGroupId,
		const QStringList& _teachersNames,
		const QString& _subjectName,
		const QStringList& _activityTagsNames,
		const QStringList& _studentsNames,
		int _duration, /*duration, in hours*/
		int _totalDuration,
		bool _active,
		bool _computeNTotalStudents,
		int _nTotalStudents,
		int _computedNumberOfStudents);

	/**
	Adds a new split activity to the list of activities.
	Returns true if successful or false if the maximum
	number of activities was reached.
	If _minDayDistance>0, there will automatically added a compulsory
	ConstraintMinDaysBetweenActivities.
	*/
	bool addSplitActivity(
		int _firstActivityId,
		int _activityGroupId,
		const QStringList& _teachersNames,
		const QString& _subjectName,
		const QStringList& _activityTagsNames,
		const QStringList& _studentsNames,
		int _nSplits,
		int _totalDuration,
		int _durations[],
		bool _active[],
		int _minDayDistance,
		double _weightPercentage,
		bool _consecutiveIfSameDay,
		bool _computeNTotalStudents,
		int _nTotalStudents);

	/**
	Removes only the activity with this id.
	*/
	void removeActivity(int _id);

	/**
	If _activityGroupId==0, then this is a non-split activity
	(if >0, then this is a single sub-activity from a split activity.
	Removes this activity from the list of activities.
	For split activities, it removes all the sub-activities that are contained in it.
	*/
	void removeActivity(int _id, int _activityGroupId);
	
	/**
	A function to modify the information of a certain activity.
	If this is a sub-activity of a split activity,
	all the sub-activities will be modified.
	*/
	void modifyActivity(
		int _id, 
		int _activityGroupId, 
		const QStringList& _teachersNames,
		const QString& _subjectName, 
		const QStringList& _activityTagsNames, 
		const QStringList& _studentsNames,
	 	int _nSplits,
		int _totalDuration,
		int _durations[],
		bool _active[],
		bool _computeNTotalStudents,
		int nTotalStudents);

	void modifySubactivity(
		int _id, 
		int _activityGroupId, 
		const QStringList& _teachersNames,
		const QString& _subjectName, 
		const QStringList& _activityTagsNames, 
		const QStringList& _studentsNames,
		int _duration,
		bool _active,
		bool _computeNTotalStudents,
		int nTotalStudents);

	/**
	Adds a new room (already allocated).
	Returns true on success, false for already existing rooms (same name).
	*/
	bool addRoom(Room* rm);

	/*
	Faster, when reading
	*/
	bool addRoomFast(Room* rm);

	/**
	Returns -1 if not found or the index in the rooms list if found.
	*/
	int searchRoom(const QString& roomName);

	/**
	Removes the room with this name.
	Returns true on success, false on failure (not found).
	*/
	bool removeRoom(const QString& roomName);
	
	/**
	Modifies this room and takes care of all related constraints.
	Returns true on success, false on failure (if not found)
	*/
	bool modifyRoom(const QString& initialRoomName, const QString& finalRoomName, const QString& building, int capacity);

	/**
	A function to sort the room alphabetically, by name
	*/
	void sortRoomsAlphabetically();

	/**
	Adds a new building (already allocated).
	Returns true on success, false for already existing buildings (same name).
	*/
	bool addBuilding(Building* rm);

	/*
	Faster, when reading
	*/
	bool addBuildingFast(Building* rm);

	/**
	Returns -1 if not found or the index in the buildings list if found.
	*/
	int searchBuilding(const QString& buildingName);

	/**
	Removes the building with this name.
	Returns true on success, false on failure (not found).
	*/
	bool removeBuilding(const QString& buildingName);
	
	/**
	Modifies this building and takes care of all related constraints.
	Returns true on success, false on failure (if not found)
	*/
	bool modifyBuilding(const QString& initialBuildingName, const QString& finalBuildingName);

	/**
	A function to sort the buildings alphabetically, by name
	*/
	void sortBuildingsAlphabetically();

	/**
	Adds a new time constraint (already allocated).
	Returns true on success, false for already existing constraints.
	*/
	bool addTimeConstraint(TimeConstraint* ctr);

	/**
	Removes this time constraint.
	Returns true on success, false on failure (not found).
	*/
	bool removeTimeConstraint(TimeConstraint* ctr);

	/**
	Adds a new space constraint (already allocated).
	Returns true on success, false for already existing constraints.
	*/
	bool addSpaceConstraint(SpaceConstraint* ctr);

	/**
	Removes this space constraint.
	Returns true on success, false on failure (not found).
	*/
	bool removeSpaceConstraint(SpaceConstraint* ctr);

	/**
	Reads the rules from the xml input file "filename".
	Returns true on success, false on failure (inexistent file or wrong format)
	*/
	bool read(const QString& filename, bool commandLine=false, QString commandLineDirectory=QString());

	/**
	Write the rules to the xml input file "inputfile".
	*/
	bool write(const QString& filename);
	
	int activateTeacher(const QString& teacherName);
	
	int activateStudents(const QString& studentsName);
	
	int activateSubject(const QString& subjectName);
	
	int activateActivityTag(const QString& activityTagName);

	int deactivateTeacher(const QString& teacherName);
	
	int deactivateStudents(const QString& studentsName);
	
	int deactivateSubject(const QString& subjectName);
	
	int deactivateActivityTag(const QString& activityTagName);
	
private:
	TimeConstraint* readBasicCompulsoryTime(const QDomElement& elem3, FakeString& xmlReadingLog);
	TimeConstraint* readTeacherNotAvailable(const QDomElement& elem3, FakeString& xmlReadingLog);
	TimeConstraint* readTeacherNotAvailableTimes(const QDomElement& elem3, FakeString& xmlReadingLog);
	TimeConstraint* readTeacherMaxDaysPerWeek(const QDomElement& elem3, FakeString& xmlReadingLog);
	TimeConstraint* readTeachersMaxDaysPerWeek(const QDomElement& elem3, FakeString& xmlReadingLog);

	TimeConstraint* readTeacherMinDaysPerWeek(const QDomElement& elem3, FakeString& xmlReadingLog);
	TimeConstraint* readTeachersMinDaysPerWeek(const QDomElement& elem3, FakeString& xmlReadingLog);

	TimeConstraint* readTeacherIntervalMaxDaysPerWeek(const QDomElement& elem3, FakeString& xmlReadingLog);
	TimeConstraint* readTeachersIntervalMaxDaysPerWeek(const QDomElement& elem3, FakeString& xmlReadingLog);
	TimeConstraint* readStudentsSetIntervalMaxDaysPerWeek(const QDomElement& elem3, FakeString& xmlReadingLog);
	TimeConstraint* readStudentsIntervalMaxDaysPerWeek(const QDomElement& elem3, FakeString& xmlReadingLog);
	TimeConstraint* readStudentsSetNotAvailable(const QDomElement& elem3, FakeString& xmlReadingLog);
	TimeConstraint* readStudentsSetNotAvailableTimes(const QDomElement& elem3, FakeString& xmlReadingLog);
	TimeConstraint* readMinNDaysBetweenActivities(const QDomElement& elem3, FakeString& xmlReadingLog);
	TimeConstraint* readMinDaysBetweenActivities(const QDomElement& elem3, FakeString& xmlReadingLog);
	TimeConstraint* readMaxDaysBetweenActivities(const QDomElement& elem3, FakeString& xmlReadingLog);
	TimeConstraint* readMinGapsBetweenActivities(const QDomElement& elem3, FakeString& xmlReadingLog);
	TimeConstraint* readActivitiesNotOverlapping(const QDomElement& elem3, FakeString& xmlReadingLog);
	TimeConstraint* readActivitiesSameStartingTime(const QDomElement& elem3, FakeString& xmlReadingLog);
	TimeConstraint* readActivitiesSameStartingHour(const QDomElement& elem3, FakeString& xmlReadingLog);
	TimeConstraint* readActivitiesSameStartingDay(const QDomElement& elem3, FakeString& xmlReadingLog);
	TimeConstraint* readTeachersMaxHoursDaily(const QDomElement& elem3, FakeString& xmlReadingLog);
	TimeConstraint* readTeacherMaxHoursDaily(const QDomElement& elem3, FakeString& xmlReadingLog);
	TimeConstraint* readTeachersMaxHoursContinuously(const QDomElement& elem3, FakeString& xmlReadingLog);
	TimeConstraint* readTeacherMaxHoursContinuously(const QDomElement& elem3, FakeString& xmlReadingLog);
	TimeConstraint* readTeacherActivityTagMaxHoursContinuously(const QDomElement& elem3, FakeString& xmlReadingLog);
	TimeConstraint* readTeachersActivityTagMaxHoursContinuously(const QDomElement& elem3, FakeString& xmlReadingLog);

	TimeConstraint* readTeacherActivityTagMaxHoursDaily(const QDomElement& elem3, FakeString& xmlReadingLog);
	TimeConstraint* readTeachersActivityTagMaxHoursDaily(const QDomElement& elem3, FakeString& xmlReadingLog);

	TimeConstraint* readTeachersMinHoursDaily(const QDomElement& elem3, FakeString& xmlReadingLog);
	TimeConstraint* readTeacherMinHoursDaily(const QDomElement& elem3, FakeString& xmlReadingLog);
	TimeConstraint* readStudentsMaxHoursDaily(const QDomElement& elem3, FakeString& xmlReadingLog);
	TimeConstraint* readStudentsSetMaxHoursDaily(const QDomElement& elem3, FakeString& xmlReadingLog);
	TimeConstraint* readStudentsMaxHoursContinuously(const QDomElement& elem3, FakeString& xmlReadingLog);
	TimeConstraint* readStudentsSetMaxHoursContinuously(const QDomElement& elem3, FakeString& xmlReadingLog);
	TimeConstraint* readStudentsSetActivityTagMaxHoursContinuously(const QDomElement& elem3, FakeString& xmlReadingLog);
	TimeConstraint* readStudentsActivityTagMaxHoursContinuously(const QDomElement& elem3, FakeString& xmlReadingLog);

	TimeConstraint* readStudentsSetActivityTagMaxHoursDaily(const QDomElement& elem3, FakeString& xmlReadingLog);
	TimeConstraint* readStudentsActivityTagMaxHoursDaily(const QDomElement& elem3, FakeString& xmlReadingLog);

	TimeConstraint* readStudentsMinHoursDaily(const QDomElement& elem3, FakeString& xmlReadingLog);
	TimeConstraint* readStudentsSetMinHoursDaily(const QDomElement& elem3, FakeString& xmlReadingLog);

	TimeConstraint* readActivityPreferredTime(const QDomElement& elem3, FakeString& xmlReadingLog,
		bool& reportUnspecifiedPermanentlyLockedTime, bool& reportUnspecifiedDayOrHourPreferredStartingTime);
	TimeConstraint* readActivityPreferredStartingTime(const QDomElement& elem3, FakeString& xmlReadingLog,
		bool& reportUnspecifiedPermanentlyLockedTime, bool& reportUnspecifiedDayOrHourPreferredStartingTime);

	TimeConstraint* readActivityEndsStudentsDay(const QDomElement& elem3, FakeString& xmlReadingLog);
	TimeConstraint* readActivitiesEndStudentsDay(const QDomElement& elem3, FakeString& xmlReadingLog);
	
	/*old, with 2 and 3*/
	TimeConstraint* read2ActivitiesConsecutive(const QDomElement& elem3, FakeString& xmlReadingLog);
	TimeConstraint* read2ActivitiesGrouped(const QDomElement& elem3, FakeString& xmlReadingLog);
	TimeConstraint* read3ActivitiesGrouped(const QDomElement& elem3, FakeString& xmlReadingLog);
	TimeConstraint* read2ActivitiesOrdered(const QDomElement& elem3, FakeString& xmlReadingLog);
	/*end old*/
	
	TimeConstraint* readTwoActivitiesConsecutive(const QDomElement& elem3, FakeString& xmlReadingLog);
	TimeConstraint* readTwoActivitiesGrouped(const QDomElement& elem3, FakeString& xmlReadingLog);
	TimeConstraint* readThreeActivitiesGrouped(const QDomElement& elem3, FakeString& xmlReadingLog);
	TimeConstraint* readTwoActivitiesOrdered(const QDomElement& elem3, FakeString& xmlReadingLog);
	
	TimeConstraint* readActivityPreferredTimes(const QDomElement& elem3, FakeString& xmlReadingLog);
	TimeConstraint* readActivityPreferredTimeSlots(const QDomElement& elem3, FakeString& xmlReadingLog);
	TimeConstraint* readActivityPreferredStartingTimes(const QDomElement& elem3, FakeString& xmlReadingLog);
	
	TimeConstraint* readBreak(const QDomElement& elem3, FakeString& xmlReadingLog);
	TimeConstraint* readBreakTimes(const QDomElement& elem3, FakeString& xmlReadingLog);
	
	TimeConstraint* readTeachersNoGaps(const QDomElement& elem3, FakeString& xmlReadingLog);
	TimeConstraint* readTeachersMaxGapsPerWeek(const QDomElement& elem3, FakeString& xmlReadingLog);
	TimeConstraint* readTeacherMaxGapsPerWeek(const QDomElement& elem3, FakeString& xmlReadingLog);
	TimeConstraint* readTeachersMaxGapsPerDay(const QDomElement& elem3, FakeString& xmlReadingLog);
	TimeConstraint* readTeacherMaxGapsPerDay(const QDomElement& elem3, FakeString& xmlReadingLog);
	
	TimeConstraint* readStudentsNoGaps(const QDomElement& elem3, FakeString& xmlReadingLog);
	TimeConstraint* readStudentsSetNoGaps(const QDomElement& elem3, FakeString& xmlReadingLog);
	TimeConstraint* readStudentsMaxGapsPerWeek(const QDomElement& elem3, FakeString& xmlReadingLog);
	TimeConstraint* readStudentsSetMaxGapsPerWeek(const QDomElement& elem3, FakeString& xmlReadingLog);

	TimeConstraint* readStudentsMaxGapsPerDay(const QDomElement& elem3, FakeString& xmlReadingLog);
	TimeConstraint* readStudentsSetMaxGapsPerDay(const QDomElement& elem3, FakeString& xmlReadingLog);

	TimeConstraint* readStudentsEarly(const QDomElement& elem3, FakeString& xmlReadingLog);
	TimeConstraint* readStudentsEarlyMaxBeginningsAtSecondHour(const QDomElement& elem3, FakeString& xmlReadingLog);
	TimeConstraint* readStudentsSetEarly(const QDomElement& elem3, FakeString& xmlReadingLog);
	TimeConstraint* readStudentsSetEarlyMaxBeginningsAtSecondHour(const QDomElement& elem3, FakeString& xmlReadingLog);

	TimeConstraint* readActivitiesPreferredTimes(const QDomElement& elem3, FakeString& xmlReadingLog);
	TimeConstraint* readActivitiesPreferredTimeSlots(const QDomElement& elem3, FakeString& xmlReadingLog);
	TimeConstraint* readActivitiesPreferredStartingTimes(const QDomElement& elem3, FakeString& xmlReadingLog);

	TimeConstraint* readSubactivitiesPreferredTimeSlots(const QDomElement& elem3, FakeString& xmlReadingLog);
	TimeConstraint* readSubactivitiesPreferredStartingTimes(const QDomElement& elem3, FakeString& xmlReadingLog);


	SpaceConstraint* readBasicCompulsorySpace(const QDomElement& elem3, FakeString& xmlReadingLog);
	SpaceConstraint* readRoomNotAvailable(const QDomElement& elem3, FakeString& xmlReadingLog);
	SpaceConstraint* readRoomNotAvailableTimes(const QDomElement& elem3, FakeString& xmlReadingLog);
	SpaceConstraint* readActivityPreferredRoom(const QDomElement& elem3, FakeString& xmlReadingLog,
		bool& reportUnspecifiedPermanentlyLockedSpace);
	SpaceConstraint* readActivityPreferredRooms(const QDomElement& elem3, FakeString& xmlReadingLog);
	SpaceConstraint* readSubjectPreferredRoom(const QDomElement& elem3, FakeString& xmlReadingLog);
	SpaceConstraint* readSubjectPreferredRooms(const QDomElement& elem3, FakeString& xmlReadingLog);
	SpaceConstraint* readSubjectSubjectTagPreferredRoom(const QDomElement& elem3, FakeString& xmlReadingLog);
	SpaceConstraint* readSubjectSubjectTagPreferredRooms(const QDomElement& elem3, FakeString& xmlReadingLog);
	SpaceConstraint* readSubjectActivityTagPreferredRoom(const QDomElement& elem3, FakeString& xmlReadingLog);
	SpaceConstraint* readSubjectActivityTagPreferredRooms(const QDomElement& elem3, FakeString& xmlReadingLog);
	SpaceConstraint* readActivityTagPreferredRoom(const QDomElement& elem3, FakeString& xmlReadingLog);
	SpaceConstraint* readActivityTagPreferredRooms(const QDomElement& elem3, FakeString& xmlReadingLog);

	SpaceConstraint* readStudentsSetHomeRoom(const QDomElement& elem3, FakeString& xmlReadingLog);
	SpaceConstraint* readStudentsSetHomeRooms(const QDomElement& elem3, FakeString& xmlReadingLog);
	SpaceConstraint* readTeacherHomeRoom(const QDomElement& elem3, FakeString& xmlReadingLog);
	SpaceConstraint* readTeacherHomeRooms(const QDomElement& elem3, FakeString& xmlReadingLog);

	SpaceConstraint* readTeacherMaxBuildingChangesPerDay(const QDomElement& elem3, FakeString& xmlReadingLog);
	SpaceConstraint* readTeachersMaxBuildingChangesPerDay(const QDomElement& elem3, FakeString& xmlReadingLog);
	SpaceConstraint* readTeacherMaxBuildingChangesPerWeek(const QDomElement& elem3, FakeString& xmlReadingLog);
	SpaceConstraint* readTeachersMaxBuildingChangesPerWeek(const QDomElement& elem3, FakeString& xmlReadingLog);
	SpaceConstraint* readTeacherMinGapsBetweenBuildingChanges(const QDomElement& elem3, FakeString& xmlReadingLog);
	SpaceConstraint* readTeachersMinGapsBetweenBuildingChanges(const QDomElement& elem3, FakeString& xmlReadingLog);

	SpaceConstraint* readStudentsSetMaxBuildingChangesPerDay(const QDomElement& elem3, FakeString& xmlReadingLog);
	SpaceConstraint* readStudentsMaxBuildingChangesPerDay(const QDomElement& elem3, FakeString& xmlReadingLog);
	SpaceConstraint* readStudentsSetMaxBuildingChangesPerWeek(const QDomElement& elem3, FakeString& xmlReadingLog);
	SpaceConstraint* readStudentsMaxBuildingChangesPerWeek(const QDomElement& elem3, FakeString& xmlReadingLog);
	SpaceConstraint* readStudentsSetMinGapsBetweenBuildingChanges(const QDomElement& elem3, FakeString& xmlReadingLog);
	SpaceConstraint* readStudentsMinGapsBetweenBuildingChanges(const QDomElement& elem3, FakeString& xmlReadingLog);
};

#endif
