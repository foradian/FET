/*
File generate_pre.h
*/

/*
Copyright 2007 Lalescu Liviu.

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

#ifndef GENERATE_PRE_H
#define GENERATE_PRE_H

#include "timetable_defs.h"
#include "timetable.h"

#include "matrix.h"

#include <QSet>
#include <QHash>

#include <QTextStream>

bool compareFunctionGeneratePre(int i, int j);

extern int permutation[MAX_ACTIVITIES]; //the permutation matrix to obtain activities in
//decreasing difficulty order

bool processTimeSpaceConstraints(QTextStream* initialOrderStream=NULL);


////////BEGIN BASIC TIME CONSTRAINTS
//extern bool activitiesConflicting[MAX_ACTIVITIES][MAX_ACTIVITIES];
//extern qint8 activitiesConflictingPercentage[MAX_ACTIVITIES][MAX_ACTIVITIES]; //-1 for not conflicting,
extern Matrix2D<qint8> activitiesConflictingPercentage; //-1 for not conflicting,
//a value >=0 equal with the weight of the maximum weightPercentage of a basic time constraint

bool computeActivitiesConflictingPercentage();
//void computeActivitiesConflicting();

void sortActivities(const QHash<int, int> & reprSameStartingTime, const QHash<int, QSet<int> > & reprSameActivitiesSet, QTextStream* initialOrderStream=NULL);
////////END   BASIC TIME CONSTRAINTS


////////BEGIN MIN DAYS TIME CONSTRAINTS
extern Matrix1D<QList<int> > minDaysListOfActivities;
extern Matrix1D<QList<int> > minDaysListOfMinDays;
extern Matrix1D<QList<double> > minDaysListOfWeightPercentages;
extern Matrix1D<QList<bool> > minDaysListOfConsecutiveIfSameDay;

bool computeMinDays();
////////END   MIN DAYS TIME CONSTRAINTS


////////BEGIN MAX DAYS TIME CONSTRAINTS
extern Matrix1D<QList<int> > maxDaysListOfActivities;
extern Matrix1D<QList<int> > maxDaysListOfMaxDays;
extern Matrix1D<QList<double> > maxDaysListOfWeightPercentages;

bool computeMaxDays();
////////END   MAX DAYS TIME CONSTRAINTS


////////BEGIN MIN GAPS between activities TIME CONSTRAINTS
extern Matrix1D<QList<int> > minGapsBetweenActivitiesListOfActivities;
extern Matrix1D<QList<int> > minGapsBetweenActivitiesListOfMinGaps;
extern Matrix1D<QList<double> > minGapsBetweenActivitiesListOfWeightPercentages;

bool computeMinGapsBetweenActivities();
////////END	 MIN GAPS between activities TIME CONSTRAINTS


////////BEGIN st. not available, tch not avail., break, activity preferred time,
////////activity preferred times, activities preferred times
//percentage of allowed time, -1 if no restriction
//extern double notAllowedTimesPercentages[MAX_ACTIVITIES][MAX_HOURS_PER_WEEK];
extern Matrix2D<double> notAllowedTimesPercentages;

//break, which is not considered gap, false means no break, true means 100% break
//break can only be 100% or none
//extern bool breakDayHour[MAX_DAYS_PER_WEEK][MAX_HOURS_PER_DAY];
extern Matrix2D<bool> breakDayHour;

//students set not available, which is not considered gap, false means available, true means 100% not available
//students set not available can only be 100% or none
//extern bool subgroupNotAvailableDayHour[MAX_TOTAL_SUBGROUPS][MAX_DAYS_PER_WEEK][MAX_HOURS_PER_DAY];
extern Matrix3D<bool> subgroupNotAvailableDayHour;

//teacher not available, which is not considered gap, false means available, true means 100% not available
//teacher not available can only be 100% or none
//extern bool teacherNotAvailableDayHour[MAX_TEACHERS][MAX_DAYS_PER_WEEK][MAX_HOURS_PER_DAY];
extern Matrix3D<bool> teacherNotAvailableDayHour;

bool computeNotAllowedTimesPercentages();
////////END   st. not available, tch not avail., break, activity preferred time,
////////activity preferred time, activities preferred times


////////BEGIN students no gaps and early
bool computeNHoursPerSubgroup();
bool computeSubgroupsEarlyAndMaxGapsPercentages();

extern double subgroupsEarlyMaxBeginningsAtSecondHourPercentage[MAX_TOTAL_SUBGROUPS];
extern int subgroupsEarlyMaxBeginningsAtSecondHourMaxBeginnings[MAX_TOTAL_SUBGROUPS];
extern double subgroupsMaxGapsPerWeekPercentage[MAX_TOTAL_SUBGROUPS];
extern int subgroupsMaxGapsPerWeekMaxGaps[MAX_TOTAL_SUBGROUPS];

extern int nHoursPerSubgroup[MAX_TOTAL_SUBGROUPS]; //used also for students min hours daily

//max gaps per day (not perfect!!!)
bool computeSubgroupsMaxGapsPerDayPercentages();

extern double subgroupsMaxGapsPerDayPercentage[MAX_TOTAL_SUBGROUPS];
extern int subgroupsMaxGapsPerDayMaxGaps[MAX_TOTAL_SUBGROUPS];
extern bool haveStudentsMaxGapsPerDay;
////////END   students no gaps and early


////////BEGIN teachers max days per week
//activities indices (in 0..gt.rules.nInternalActivities-1) for each teacher
extern int teachersMaxDaysPerWeekMaxDays[MAX_TEACHERS]; //-1 for not existing
extern double teachersMaxDaysPerWeekWeightPercentages[MAX_TEACHERS]; //-1 for not existing
//it is practically better to use the variable below and to put it exactly like in generate.cpp,
//the order of activities changes
///extern QList<int> teacherActivitiesOfTheDay[MAX_TEACHERS][MAX_DAYS_PER_WEEK];
extern Matrix1D<QList<int> > teachersWithMaxDaysPerWeekForActivities;

bool computeMaxDaysPerWeekForTeachers();
////////END   teachers max days per week


////////BEGIN teachers max gaps per week and per day
//extern QList<int> activitiesForTeachers[MAX_TEACHERS];
extern double teachersMaxGapsPerWeekPercentage[MAX_TEACHERS];
extern int teachersMaxGapsPerWeekMaxGaps[MAX_TEACHERS];

extern double teachersMaxGapsPerDayPercentage[MAX_TEACHERS];
extern int teachersMaxGapsPerDayMaxGaps[MAX_TEACHERS];

extern int nHoursPerTeacher[MAX_TEACHERS];
bool computeNHoursPerTeacher();
bool computeTeachersMaxGapsPerWeekPercentage();
bool computeTeachersMaxGapsPerDayPercentage();
////////END   teachers max gaps per week and per day


////////BEGIN activities same starting time
extern Matrix1D<QList<int> > activitiesSameStartingTimeActivities;
extern Matrix1D<QList<double> > activitiesSameStartingTimePercentages;
bool computeActivitiesSameStartingTime(QHash<int, int> & reprSameStartingTime, QHash<int, QSet<int> > & reprSameActivitiesSet);
////////END   activities same starting time


////////BEGIN activities same starting hour
extern Matrix1D<QList<int> > activitiesSameStartingHourActivities;
extern Matrix1D<QList<double> > activitiesSameStartingHourPercentages;
void computeActivitiesSameStartingHour();
////////END   activities same starting hour


////////BEGIN activities same starting day
extern Matrix1D<QList<int> > activitiesSameStartingDayActivities;
extern Matrix1D<QList<double> > activitiesSameStartingDayPercentages;
void computeActivitiesSameStartingDay();
////////END   activities same starting hour


////////BEGIN activities not overlapping
extern Matrix1D<QList<int> > activitiesNotOverlappingActivities;
extern Matrix1D<QList<double> > activitiesNotOverlappingPercentages;
void computeActivitiesNotOverlapping();
////////END   activities not overlapping


////////BEGIN teacher(s) max hours daily
//extern int teachersGapsPerDay[MAX_TEACHERS][MAX_DAYS_PER_WEEK];
//extern int teachersNHoursPerDay[MAX_TEACHERS][MAX_DAYS_PER_WEEK];

extern double teachersMaxHoursDailyPercentages1[MAX_TEACHERS];
extern int teachersMaxHoursDailyMaxHours1[MAX_TEACHERS];
//extern int teachersRealGapsPerDay1[MAX_TEACHERS][MAX_DAYS_PER_WEEK];

extern double teachersMaxHoursDailyPercentages2[MAX_TEACHERS];
extern int teachersMaxHoursDailyMaxHours2[MAX_TEACHERS];
//extern int teachersRealGapsPerDay2[MAX_TEACHERS][MAX_DAYS_PER_WEEK];

bool computeTeachersMaxHoursDaily();
////////END   teacher(s) max hours daily


////////BEGIN teacher(s) max hours continuously
extern double teachersMaxHoursContinuouslyPercentages1[MAX_TEACHERS];
extern int teachersMaxHoursContinuouslyMaxHours1[MAX_TEACHERS];

extern double teachersMaxHoursContinuouslyPercentages2[MAX_TEACHERS];
extern int teachersMaxHoursContinuouslyMaxHours2[MAX_TEACHERS];

bool computeTeachersMaxHoursContinuously();
////////END   teacher(s) max hours continuously


///////BEGIN teacher(s) activity tag max hours daily
extern bool haveTeachersActivityTagMaxHoursDaily;

extern Matrix1D<QList<int> > teachersActivityTagMaxHoursDailyMaxHours;
extern Matrix1D<QList<int> > teachersActivityTagMaxHoursDailyActivityTag;
extern Matrix1D<QList<double> > teachersActivityTagMaxHoursDailyPercentage;
//(formerly arrays of size MAX_TEACHERS)

bool computeTeachersActivityTagMaxHoursDaily();
///////END   teacher(s) activity tag max hours daily


///////BEGIN teacher(s) activity tag max hours continuously
extern bool haveTeachersActivityTagMaxHoursContinuously;

extern Matrix1D<QList<int> > teachersActivityTagMaxHoursContinuouslyMaxHours;
extern Matrix1D<QList<int> > teachersActivityTagMaxHoursContinuouslyActivityTag;
extern Matrix1D<QList<double> > teachersActivityTagMaxHoursContinuouslyPercentage;
//(formerly arrays of size MAX_TEACHERS)

bool computeTeachersActivityTagMaxHoursContinuously();
///////END   teacher(s) activity tag max hours continuously


////////BEGIN teacher(s) min hours daily
extern double teachersMinHoursDailyPercentages[MAX_TEACHERS];
extern int teachersMinHoursDailyMinHours[MAX_TEACHERS];

bool computeTeachersMinHoursDaily();
////////END   teacher(s) min hours daily


////////BEGIN teacher(s) min days per week
extern double teachersMinDaysPerWeekPercentages[MAX_TEACHERS];
extern int teachersMinDaysPerWeekMinDays[MAX_TEACHERS];

bool computeTeachersMinDaysPerWeek();
////////END   teacher(s) min days per week


////////BEGIN students (set) max hours daily
extern double subgroupsMaxHoursDailyPercentages1[MAX_TOTAL_SUBGROUPS];
extern int subgroupsMaxHoursDailyMaxHours1[MAX_TOTAL_SUBGROUPS];

extern double subgroupsMaxHoursDailyPercentages2[MAX_TOTAL_SUBGROUPS];
extern int subgroupsMaxHoursDailyMaxHours2[MAX_TOTAL_SUBGROUPS];

bool computeSubgroupsMaxHoursDaily();
////////END   students (set) max hours daily


////////BEGIN students (set) max hours continuously
extern double subgroupsMaxHoursContinuouslyPercentages1[MAX_TOTAL_SUBGROUPS];
extern int subgroupsMaxHoursContinuouslyMaxHours1[MAX_TOTAL_SUBGROUPS];

extern double subgroupsMaxHoursContinuouslyPercentages2[MAX_TOTAL_SUBGROUPS];
extern int subgroupsMaxHoursContinuouslyMaxHours2[MAX_TOTAL_SUBGROUPS];

bool computeStudentsMaxHoursContinuously();
////////END   students (set) max hours continuously


///////BEGIN students (set) activity tag max hours daily
extern bool haveStudentsActivityTagMaxHoursDaily;

extern Matrix1D<QList<int> > subgroupsActivityTagMaxHoursDailyMaxHours;
extern Matrix1D<QList<int> > subgroupsActivityTagMaxHoursDailyActivityTag;
extern Matrix1D<QList<double> > subgroupsActivityTagMaxHoursDailyPercentage;
//(formerly arrays of size MAX_TOTAL_SUBGROUPS)

bool computeStudentsActivityTagMaxHoursDaily();
///////END   students (set) activity tag max hours daily


///////BEGIN students (set) activity tag max hours continuously
extern bool haveStudentsActivityTagMaxHoursContinuously;

extern Matrix1D<QList<int> > subgroupsActivityTagMaxHoursContinuouslyMaxHours;
extern Matrix1D<QList<int> > subgroupsActivityTagMaxHoursContinuouslyActivityTag;
extern Matrix1D<QList<double> > subgroupsActivityTagMaxHoursContinuouslyPercentage;
//(formerly arrays of size MAX_TOTAL_SUBGROUPS)

bool computeStudentsActivityTagMaxHoursContinuously();
///////END   students (set) activity tag max hours continuously


////////BEGIN students (set) min hours daily
extern double subgroupsMinHoursDailyPercentages[MAX_TOTAL_SUBGROUPS];
extern int subgroupsMinHoursDailyMinHours[MAX_TOTAL_SUBGROUPS];
extern bool subgroupsMinHoursDailyAllowEmptyDays[MAX_TOTAL_SUBGROUPS];
bool computeSubgroupsMinHoursDaily();
////////END   students (set) min hours daily


//////////////BEGIN 2 activities consecutive
//index represents the first activity, value in array represents the second activity
extern Matrix1D<QList<double> > constrTwoActivitiesConsecutivePercentages;
extern Matrix1D<QList<int> > constrTwoActivitiesConsecutiveActivities;
void computeConstrTwoActivitiesConsecutive();

//index represents the second activity, value in array represents the first activity
extern Matrix1D<QList<double> > inverseConstrTwoActivitiesConsecutivePercentages;
extern Matrix1D<QList<int> > inverseConstrTwoActivitiesConsecutiveActivities;
//////////////END   2 activities consecutive


//////////////BEGIN 2 activities grouped
//index represents the first activity, value in array represents the second activity
extern Matrix1D<QList<double> > constrTwoActivitiesGroupedPercentages;
extern Matrix1D<QList<int> > constrTwoActivitiesGroupedActivities;
void computeConstrTwoActivitiesGrouped();


//////////////BEGIN 3 activities grouped
//index represents the first activity, value in array represents the second and third activities
extern Matrix1D<QList<double> > constrThreeActivitiesGroupedPercentages;
extern Matrix1D<QList<QPair<int, int> > > constrThreeActivitiesGroupedActivities;
void computeConstrThreeActivitiesGrouped();


//////////////BEGIN 2 activities ordered
//index represents the first activity, value in array represents the second activity
extern Matrix1D<QList<double> > constrTwoActivitiesOrderedPercentages;
extern Matrix1D<QList<int> > constrTwoActivitiesOrderedActivities;
void computeConstrTwoActivitiesOrdered();

//index represents the second activity, value in array represents the first activity
extern Matrix1D<QList<double> > inverseConstrTwoActivitiesOrderedPercentages;
extern Matrix1D<QList<int> > inverseConstrTwoActivitiesOrderedActivities;
//////////////END   2 activities ordered


////////////BEGIN activity ends students day
extern double activityEndsStudentsDayPercentages[MAX_ACTIVITIES]; //-1 for not existing
bool computeActivityEndsStudentsDayPercentages();
extern bool haveActivityEndsStudentsDay;
////////////END   activity ends students day


bool checkMinDays100Percent();
bool checkMinDaysConsecutiveIfSameDay();


///////BEGIN teachers interval max days per week
extern double teachersIntervalMaxDaysPerWeekPercentages1[MAX_TEACHERS];
extern int teachersIntervalMaxDaysPerWeekMaxDays1[MAX_TEACHERS];
extern int teachersIntervalMaxDaysPerWeekIntervalStart1[MAX_TEACHERS];
extern int teachersIntervalMaxDaysPerWeekIntervalEnd1[MAX_TEACHERS];

extern double teachersIntervalMaxDaysPerWeekPercentages2[MAX_TEACHERS];
extern int teachersIntervalMaxDaysPerWeekMaxDays2[MAX_TEACHERS];
extern int teachersIntervalMaxDaysPerWeekIntervalStart2[MAX_TEACHERS];
extern int teachersIntervalMaxDaysPerWeekIntervalEnd2[MAX_TEACHERS];

extern double teachersIntervalMaxDaysPerWeekPercentages3[MAX_TEACHERS];
extern int teachersIntervalMaxDaysPerWeekMaxDays3[MAX_TEACHERS];
extern int teachersIntervalMaxDaysPerWeekIntervalStart3[MAX_TEACHERS];
extern int teachersIntervalMaxDaysPerWeekIntervalEnd3[MAX_TEACHERS];

bool computeTeachersIntervalMaxDaysPerWeek();
///////END   teachers interval max days per week


///////BEGIN subgroups interval max days per week
extern double subgroupsIntervalMaxDaysPerWeekPercentages1[MAX_TOTAL_SUBGROUPS];
extern int subgroupsIntervalMaxDaysPerWeekMaxDays1[MAX_TOTAL_SUBGROUPS];
extern int subgroupsIntervalMaxDaysPerWeekIntervalStart1[MAX_TOTAL_SUBGROUPS];
extern int subgroupsIntervalMaxDaysPerWeekIntervalEnd1[MAX_TOTAL_SUBGROUPS];

extern double subgroupsIntervalMaxDaysPerWeekPercentages2[MAX_TOTAL_SUBGROUPS];
extern int subgroupsIntervalMaxDaysPerWeekMaxDays2[MAX_TOTAL_SUBGROUPS];
extern int subgroupsIntervalMaxDaysPerWeekIntervalStart2[MAX_TOTAL_SUBGROUPS];
extern int subgroupsIntervalMaxDaysPerWeekIntervalEnd2[MAX_TOTAL_SUBGROUPS];

extern double subgroupsIntervalMaxDaysPerWeekPercentages3[MAX_TOTAL_SUBGROUPS];
extern int subgroupsIntervalMaxDaysPerWeekMaxDays3[MAX_TOTAL_SUBGROUPS];
extern int subgroupsIntervalMaxDaysPerWeekIntervalStart3[MAX_TOTAL_SUBGROUPS];
extern int subgroupsIntervalMaxDaysPerWeekIntervalEnd3[MAX_TOTAL_SUBGROUPS];

bool computeSubgroupsIntervalMaxDaysPerWeek();
///////END   subgroups interval max days per week


////////BEGIN rooms
bool computeBasicSpace();

//extern double notAllowedRoomTimePercentages[MAX_ROOMS][MAX_HOURS_PER_WEEK]; //-1 for available
extern Matrix2D<double> notAllowedRoomTimePercentages; //-1 for available
bool computeNotAllowedRoomTimePercentages();

class PreferredRoomsItem{
public:
	double percentage;
	QSet<int> preferredRooms;
};

extern Matrix1D<QList<PreferredRoomsItem> > activitiesPreferredRoomsList;
extern bool unspecifiedPreferredRoom[MAX_ACTIVITIES];

extern Matrix1D<QList<int> > activitiesHomeRoomsHomeRooms;
extern double activitiesHomeRoomsPercentage[MAX_ACTIVITIES];
extern bool unspecifiedHomeRoom[MAX_ACTIVITIES];

bool computeActivitiesRoomsPreferences();
////////END   rooms


////////BEGIN buildings
extern double maxBuildingChangesPerDayForStudentsPercentages[MAX_TOTAL_SUBGROUPS];
extern int maxBuildingChangesPerDayForStudentsMaxChanges[MAX_TOTAL_SUBGROUPS];
bool computeMaxBuildingChangesPerDayForStudents();

extern double minGapsBetweenBuildingChangesForStudentsPercentages[MAX_TOTAL_SUBGROUPS];
extern int minGapsBetweenBuildingChangesForStudentsMinGaps[MAX_TOTAL_SUBGROUPS];
bool computeMinGapsBetweenBuildingChangesForStudents();

extern double maxBuildingChangesPerDayForTeachersPercentages[MAX_TEACHERS];
extern int maxBuildingChangesPerDayForTeachersMaxChanges[MAX_TEACHERS];
bool computeMaxBuildingChangesPerDayForTeachers();

extern double minGapsBetweenBuildingChangesForTeachersPercentages[MAX_TEACHERS];
extern int minGapsBetweenBuildingChangesForTeachersMinGaps[MAX_TEACHERS];
bool computeMinGapsBetweenBuildingChangesForTeachers();

extern double maxBuildingChangesPerWeekForStudentsPercentages[MAX_TOTAL_SUBGROUPS];
extern int maxBuildingChangesPerWeekForStudentsMaxChanges[MAX_TOTAL_SUBGROUPS];
bool computeMaxBuildingChangesPerWeekForStudents();

extern double maxBuildingChangesPerWeekForTeachersPercentages[MAX_TEACHERS];
extern int maxBuildingChangesPerWeekForTeachersMaxChanges[MAX_TEACHERS];
bool computeMaxBuildingChangesPerWeekForTeachers();
////////END   buildings


extern Matrix1D<QList<int> > mustComputeTimetableSubgroups;
extern Matrix1D<QList<int> > mustComputeTimetableTeachers;
extern bool mustComputeTimetableSubgroup[MAX_TOTAL_SUBGROUPS];
extern bool mustComputeTimetableTeacher[MAX_TEACHERS];


bool homeRoomsAreOk();


extern bool fixedTimeActivity[MAX_ACTIVITIES]; //this is in fact time fixed, not both time and space
extern bool fixedSpaceActivity[MAX_ACTIVITIES];
bool computeFixedActivities();

class GeneratePreTranslate: public QObject{
	Q_OBJECT
};


#endif
