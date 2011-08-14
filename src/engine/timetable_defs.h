/*
File timetable_defs.h
*/

/***************************************************************************
                          timetable_defs.h  -  description
                             -------------------
    begin                : Sat Mar 15 2003
    copyright            : (C) 2003 by Lalescu Liviu
    email                : Please see http://lalescu.ro/liviu/ for details about contacting Liviu Lalescu (in particular, you can find here the e-mail address)
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#ifndef TIMETABLE_DEFS_H
#define TIMETABLE_DEFS_H

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>


#include <QString>

class QWidget;


/**
The version number
*/
extern const QString FET_VERSION;

/**
The language
*/
extern QString FET_LANGUAGE;

extern bool LANGUAGE_STYLE_RIGHT_TO_LEFT;

extern QString LANGUAGE_FOR_HTML;

/**
Timetable html css javaScript Level, added by Volker Dirr
*/
extern int TIMETABLE_HTML_LEVEL;

extern bool PRINT_NOT_AVAILABLE_TIME_SLOTS;

extern bool PRINT_BREAK_TIME_SLOTS;

extern bool DIVIDE_HTML_TIMETABLES_WITH_TIME_AXIS_BY_DAYS;

extern bool PRINT_ACTIVITIES_WITH_SAME_STARTING_TIME;

extern bool USE_GUI_COLORS;

extern bool SHOW_SHORTCUTS_ON_MAIN_WINDOW;

extern bool ENABLE_ACTIVITY_TAG_MAX_HOURS_DAILY;
extern bool ENABLE_STUDENTS_MAX_GAPS_PER_DAY;

extern bool SHOW_WARNING_FOR_NOT_PERFECT_CONSTRAINTS;

extern bool ENABLE_STUDENTS_MIN_HOURS_DAILY_WITH_ALLOW_EMPTY_DAYS;
extern bool SHOW_WARNING_FOR_STUDENTS_MIN_HOURS_DAILY_WITH_ALLOW_EMPTY_DAYS;

extern bool CONFIRM_ACTIVITY_PLANNING;
extern bool CONFIRM_SPREAD_ACTIVITIES;
extern bool CONFIRM_REMOVE_REDUNDANT;

/**
The maximum total number of different subgroups of students
*/
const int MAX_TOTAL_SUBGROUPS=30000;//MAX_YEARS*MAX_GROUPS_PER_YEAR*MAX_SUBGROUPS_PER_GROUP;

const int MAX_ROOM_CAPACITY=30000;


/**
The maximum number of different teachers
*/
const int MAX_TEACHERS=6000;

/**
The maximum number of different subjects
*/
const int MAX_SUBJECTS=6000;

/**
The maximum number of activities
IMPORTANT: must be qint16 (max 32767), because we are using qint16 for each activity index and for unallocated activity = max_activities
*/
const int MAX_ACTIVITIES=30000;

//if you want to increase this, you also need to modify the add/modify activity dialogs, to permit larger values
//(add more pages in the subactivities tab).
const int MAX_SPLIT_OF_AN_ACTIVITY=35;

/**
The maximum number of rooms
IMPORTANT: max_rooms+1 must be qint16 (max 32766 for max_rooms), because we are using qint16 for each room index and for unallocated space = max_rooms and for unspecified room = max_rooms+1
*/
const int MAX_ROOMS=6000;

/**
The maximum number of buildings
*/
const int MAX_BUILDINGS=6000;

/**
This constant represents an unallocated activity
*/
const qint16 UNALLOCATED_ACTIVITY = MAX_ACTIVITIES;

/**
The maximum number of working hours per day.
IMPORTANT: max hours per day * max days per week = max hours per week must be qint16 (max 32767),
because each time is qint16 and unallocated time is qint16
*/
const int MAX_HOURS_PER_DAY=60;

/**
The maximum number of working days per week.
IMPORTANT: max hours per day * max days per week = max hours per week must be qint16 (max 32767)
because each time is qint16 and unallocated time is qint16
*/
const int MAX_DAYS_PER_WEEK=35;

/**
The predefined names of the days of the week
*/
extern const QString PREDEFINED_DAYS_OF_THE_WEEK[];

/**
The maximum number of working hours in a week.
<p>
hours in a week are arranged like this:
           Mo Tu We Th Fr
0-1        0  1  2  3  4
1-2        5  6  7  8  9
2-3        10 11 12 13 14
3-4        15 16 17 18 19
4-5        20 21 22 23 24
5-6        25 26 27 28 29 etc.

IMPORTANT: max hours per day * max days per week = max hours per week must be qint16 (max 32767)
because each time is qint16 and unallocated time is qint16
*/
const int MAX_HOURS_PER_WEEK = MAX_HOURS_PER_DAY * MAX_DAYS_PER_WEEK;

/**
This constant represents unallocated time for an activity
*/
const qint16 UNALLOCATED_TIME = MAX_HOURS_PER_WEEK;

/**
This constant represents unallocated space for an activity
*/
const qint16 UNALLOCATED_SPACE = MAX_ROOMS;

const qint16 UNSPECIFIED_ROOM = MAX_ROOMS+1;


/**
The maximum number of time constraints
*/
const int MAX_TIME_CONSTRAINTS = 60000;

/**
The maximum number of space constraints
*/
const int MAX_SPACE_CONSTRAINTS = 60000;

/**
The maximum number of preferred times that can be considered
in this kind of constraint
*/
//const int MAX_N_CONSTRAINT_ACTIVITY_PREFERRED_TIME_SLOTS = MAX_HOURS_PER_WEEK;

/**
The maximum number of preferred times that can be considered
in this kind of constraint
*/
//const int MAX_N_CONSTRAINT_ACTIVITIES_PREFERRED_TIME_SLOTS = MAX_HOURS_PER_WEEK;

//const int MAX_N_CONSTRAINT_ACTIVITY_PREFERRED_STARTING_TIMES = MAX_HOURS_PER_WEEK;
//const int MAX_N_CONSTRAINT_ACTIVITIES_PREFERRED_STARTING_TIMES = MAX_HOURS_PER_WEEK;

//const int MAX_N_CONSTRAINT_SUBACTIVITIES_PREFERRED_TIME_SLOTS = MAX_HOURS_PER_WEEK;
//const int MAX_N_CONSTRAINT_SUBACTIVITIES_PREFERRED_STARTING_TIMES = MAX_HOURS_PER_WEEK;


/**
File and directory separator
*/
extern const QString FILE_SEP;


/**
The timetable's rules input file name
*/
extern QString INPUT_FILENAME_XML;

/**
The working directory
*/
extern QString WORKING_DIRECTORY;

/**
The import directory
*/
extern QString IMPORT_DIRECTORY;


//OUTPUT FILES

/**
The output directory. Please be careful when editing it,
because the functions add a FILE_SEP sign at the end of it
and then the name of a file. If you make OUTPUT_DIR="",
there will be problems.
*/
extern QString OUTPUT_DIR;
/*#ifdef Q_OS_WIN
const QString OUTPUT_DIR="results";
#else
const QString OUTPUT_DIR=QDir::homeDirPath()+"/fet-results";
#endif*/


/**
A log file explaining how the xml input file was parsed
*/
extern const QString XML_PARSING_LOG_FILENAME;


/**
A function used in xml saving
*/
QString protect(const QString& str);

/**
A function used in html saving
*/
QString protect2(const QString& str);

/**
A function used in html saving
*/
QString protect2vert(const QString& str);

/**
A function used in html saving
*/
QString protect2id(const QString& str);

/**
A function used in html saving
*/
QString protect2java(const QString& str);

/**
A function used in iCal saving
*/
QString protect3(const QString& str);

/**
This constants represents free periods of a teacher in the teachers free periods timetable
*/
const qint16 TEACHER_HAS_SINGLE_GAP =0;
const qint16 TEACHER_HAS_BORDER_GAP =1;
const qint16 TEACHER_HAS_BIG_GAP =2;

const qint16 TEACHER_MUST_COME_EARLIER =4;
const qint16 TEACHER_MUST_COME_MUCH_EARLIER =6;

const qint16 TEACHER_MUST_STAY_LONGER =3;
const qint16 TEACHER_MUST_STAY_MUCH_LONGER =5;		// BE CAREFULL, I just print into LESS_DETAILED timetable, if it's smaller then TEACHER_MUST_STAY_MUCH_LONGER

const qint16 TEACHER_HAS_A_FREE_DAY =7;

const qint16 TEACHER_IS_NOT_AVAILABLE =8;

const int TEACHERS_FREE_PERIODS_N_CATEGORIES=9;



extern int checkForUpdates;

extern QString internetVersion;

//class Widget;
void centerWidgetOnScreen(QWidget* widget);
//int maxScreenWidth(QWidget* widget);
int maxRecommendedWidth(QWidget* widget);


//for random Knuth - from Knuth TAOCP Vol. 2 Seminumerical Algorithms section 3.6
//these numbers are really important - please do not change them, NEVER!!!
//if you want, write a new random number generator routine, with other name
//I think I found a minor error in Knuth TAOCP, the author said: if(Z<=0) then Z+=MM,
//but I think it should be: if(Z<=0) then Z+=MM-1.
//extern int XX;
//extern int YY;
const int MM=2147483647;
const int AA=48271;
const int QQ=44488;
const int RR=3399;

const int MMM=2147483399;
const int AAA=40692;
const int QQQ=52774;
const int RRR=3791;


void initRandomKnuth();
int randomKnuth1MM1(); //a random between 1 and MM-1
int randomKnuth(int k); //a random between 0 and k-1


#endif
