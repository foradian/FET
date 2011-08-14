/*
File timetableexport.cpp
*/

/***************************************************************************
                          timetableexport.cpp  -  description
                          -------------------
    begin                : Tue Apr 22 2003
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

//**********************************************************************************************************************/
//August 2007
//XHTML generation code modified by Volker Dirr (timetabling.de) from old html generation code
//(old code by Liviu Lalescu)
//added features: - xhtml 1.0 strict valide
//                - colspan and rowspan using
//                - times vertical
//                - table of contents with hyperlinks
//                - css and JavaScript support
//                - print rooms timetable
//                - TIMETABLE_HTML_LEVEL
//                - print groups and years timetable
//                - print subjects timetable
//                - print teachers free periods timetable
//                - print all activities timetable
//                - index html file
//                - print daily timetable
//                - print activities with same starting time


#include "timetable_defs.h"
#include "timetable.h"
#include "timetableexport.h"
#include "solution.h"

#include "matrix.h"

#include <iostream>
#include <fstream>
using namespace std;

#include <QString>
#include <QTextStream>
#include <QFile>

#include <QList>

#include <QHash>

#include <QDesktopWidget>

#include <QMessageBox>

#include <QLocale>
#include <QTime>
#include <QDate>

#include <QDir>

//Represents the current status of the simulation - running or stopped.
extern bool simulation_running;

 bool students_schedule_ready;
 bool teachers_schedule_ready;
 bool rooms_schedule_ready;

Solution best_solution;
extern bool LANGUAGE_STYLE_RIGHT_TO_LEFT;
extern QString LANGUAGE_FOR_HTML;

extern Timetable gt;
/*extern qint16 teachers_timetable_weekly[MAX_TEACHERS][MAX_DAYS_PER_WEEK][MAX_HOURS_PER_DAY];
extern qint16 students_timetable_weekly[MAX_TOTAL_SUBGROUPS][MAX_DAYS_PER_WEEK][MAX_HOURS_PER_DAY];
extern qint16 rooms_timetable_weekly[MAX_ROOMS][MAX_DAYS_PER_WEEK][MAX_HOURS_PER_DAY];*/
extern Matrix3D<qint16> teachers_timetable_weekly;
extern Matrix3D<qint16> students_timetable_weekly;
extern Matrix3D<qint16> rooms_timetable_weekly;

//extern QList<qint16> teachers_free_periods_timetable_weekly[TEACHERS_FREE_PERIODS_N_CATEGORIES][MAX_DAYS_PER_WEEK][MAX_HOURS_PER_DAY];
extern Matrix3D<QList<qint16> > teachers_free_periods_timetable_weekly;

//extern bool breakDayHour[MAX_DAYS_PER_WEEK][MAX_HOURS_PER_DAY];
extern Matrix2D<bool> breakDayHour;
/*extern bool teacherNotAvailableDayHour[MAX_TEACHERS][MAX_DAYS_PER_WEEK][MAX_HOURS_PER_DAY];
extern double notAllowedRoomTimePercentages[MAX_ROOMS][MAX_HOURS_PER_WEEK];
extern bool subgroupNotAvailableDayHour[MAX_TOTAL_SUBGROUPS][MAX_DAYS_PER_WEEK][MAX_HOURS_PER_DAY];*/
extern Matrix3D<bool> teacherNotAvailableDayHour;
extern Matrix2D<double> notAllowedRoomTimePercentages;
extern Matrix3D<bool> subgroupNotAvailableDayHour;

QList<int> activitiesForCurrentSubject[MAX_DAYS_PER_WEEK][MAX_HOURS_PER_DAY];

QList<int> activitiesAtTime[MAX_DAYS_PER_WEEK][MAX_HOURS_PER_DAY];

 Rules rules2;


const QString STRING_EMPTY_SLOT="---";

const QString STRING_SEVERAL_ACTIVITIES_IN_LESS_DETAILED_TABLES="???";

const QString STRING_NOT_AVAILABLE_TIME_SLOT="-x-";

const QString STRING_BREAK_SLOT="-X-";


//this hashs are needed to get the IDs for html and css in timetableexport and statistics
QHash<QString, QString> hashSubjectIDsTimetable;
QHash<QString, QString> hashActivityTagIDsTimetable;
QHash<QString, QString> hashStudentIDsTimetable;
QHash<QString, QString> hashTeacherIDsTimetable;
QHash<QString, QString> hashRoomIDsTimetable;
QHash<QString, QString> hashDayIDsTimetable;

//this hash is needed to care about sctivities with same starting time
QHash<int, QList<int> >activitiesWithSameStartingTime;

//Now the filenames of the output files are following (for xml and all html tables)
const QString SUBGROUPS_TIMETABLE_FILENAME_XML="subgroups.xml";
const QString TEACHERS_TIMETABLE_FILENAME_XML="teachers.xml";
const QString ACTIVITIES_TIMETABLE_FILENAME_XML="activities.xml";
const QString ROOMS_TIMETABLE_FILENAME_XML="rooms.xml";

const QString CONFLICTS_FILENAME="soft_conflicts.txt";
const QString INDEX_HTML="index.html";
const QString STYLESHEET_CSS="stylesheet.css";

const QString SUBGROUPS_TIMETABLE_DAYS_HORIZONTAL_FILENAME_HTML="subgroups_days_horizontal.html";
const QString SUBGROUPS_TIMETABLE_DAYS_VERTICAL_FILENAME_HTML="subgroups_days_vertical.html";
const QString SUBGROUPS_TIMETABLE_TIME_HORIZONTAL_FILENAME_HTML="subgroups_time_horizontal.html";
const QString SUBGROUPS_TIMETABLE_TIME_VERTICAL_FILENAME_HTML="subgroups_time_vertical.html";

const QString GROUPS_TIMETABLE_DAYS_HORIZONTAL_FILENAME_HTML="groups_days_horizontal.html";
const QString GROUPS_TIMETABLE_DAYS_VERTICAL_FILENAME_HTML="groups_days_vertical.html";
const QString GROUPS_TIMETABLE_TIME_HORIZONTAL_FILENAME_HTML="groups_time_horizontal.html";
const QString GROUPS_TIMETABLE_TIME_VERTICAL_FILENAME_HTML="groups_time_vertical.html";

const QString YEARS_TIMETABLE_DAYS_HORIZONTAL_FILENAME_HTML="years_days_horizontal.html";
const QString YEARS_TIMETABLE_DAYS_VERTICAL_FILENAME_HTML="years_days_vertical.html";
const QString YEARS_TIMETABLE_TIME_HORIZONTAL_FILENAME_HTML="years_time_horizontal.html";
const QString YEARS_TIMETABLE_TIME_VERTICAL_FILENAME_HTML="years_time_vertical.html";

const QString TEACHERS_TIMETABLE_DAYS_HORIZONTAL_FILENAME_HTML="teachers_days_horizontal.html";
const QString TEACHERS_TIMETABLE_DAYS_VERTICAL_FILENAME_HTML="teachers_days_vertical.html";
const QString TEACHERS_TIMETABLE_TIME_HORIZONTAL_FILENAME_HTML="teachers_time_horizontal.html";
const QString TEACHERS_TIMETABLE_TIME_VERTICAL_FILENAME_HTML="teachers_time_vertical.html";

const QString ROOMS_TIMETABLE_DAYS_HORIZONTAL_FILENAME_HTML="rooms_days_horizontal.html";
const QString ROOMS_TIMETABLE_DAYS_VERTICAL_FILENAME_HTML="rooms_days_vertical.html";
const QString ROOMS_TIMETABLE_TIME_HORIZONTAL_FILENAME_HTML="rooms_time_horizontal.html";
const QString ROOMS_TIMETABLE_TIME_VERTICAL_FILENAME_HTML="rooms_time_vertical.html";

const QString SUBJECTS_TIMETABLE_DAYS_HORIZONTAL_FILENAME_HTML="subjects_days_horizontal.html";
const QString SUBJECTS_TIMETABLE_DAYS_VERTICAL_FILENAME_HTML="subjects_days_vertical.html";
const QString SUBJECTS_TIMETABLE_TIME_HORIZONTAL_FILENAME_HTML="subjects_time_horizontal.html";
const QString SUBJECTS_TIMETABLE_TIME_VERTICAL_FILENAME_HTML="subjects_time_vertical.html";

const QString ALL_ACTIVITIES_TIMETABLE_DAYS_HORIZONTAL_FILENAME_HTML="activities_days_horizontal.html";
const QString ALL_ACTIVITIES_TIMETABLE_DAYS_VERTICAL_FILENAME_HTML="activities_days_vertical.html";
const QString ALL_ACTIVITIES_TIMETABLE_TIME_HORIZONTAL_FILENAME_HTML="activities_time_horizontal.html";
const QString ALL_ACTIVITIES_TIMETABLE_TIME_VERTICAL_FILENAME_HTML="activities_time_vertical.html";

const QString TEACHERS_FREE_PERIODS_TIMETABLE_DAYS_HORIZONTAL_FILENAME_HTML="teachers_free_periods_days_horizontal.html";
const QString TEACHERS_FREE_PERIODS_TIMETABLE_DAYS_VERTICAL_FILENAME_HTML="teachers_free_periods_days_vertical.html";

const QString MULTIPLE_TIMETABLE_DATA_RESULTS_FILE="data_and_timetable.fet";

//now the XML tags used for identification of the output file (is that comment correct? it's the old comment)
const QString STUDENTS_TIMETABLE_TAG="Students_Timetable";
const QString TEACHERS_TIMETABLE_TAG="Teachers_Timetable";
const QString ACTIVITIES_TIMETABLE_TAG="Activities_Timetable";
const QString ROOMS_TIMETABLE_TAG="Rooms_Timetable";


const QString RANDOM_SEED_FILENAME="random_seed.txt";


extern int XX;
extern int YY;

TimetableExport::TimetableExport()
{
}

TimetableExport::~TimetableExport()
{
}


void TimetableExport::getStudentsTimetable(Solution &c){
	//assert(gt.timePopulation.initialized);
	assert(gt.rules.initialized && gt.rules.internalStructureComputed);

	//assert(c.HFitness()==0); - for perfect solutions
	c.getSubgroupsTimetable(gt.rules, students_timetable_weekly);
	best_solution.copy(gt.rules, c);
	students_schedule_ready=true;
}

void TimetableExport::getTeachersTimetable(Solution &c){
	//assert(gt.timePopulation.initialized);
	assert(gt.rules.initialized && gt.rules.internalStructureComputed);

	//assert(c.HFitness()==0); - for perfect solutions
	//c.getTeachersTimetable(gt.rules, teachers_timetable_weekly);
	c.getTeachersTimetable(gt.rules, teachers_timetable_weekly, teachers_free_periods_timetable_weekly);
	best_solution.copy(gt.rules, c);
	teachers_schedule_ready=true;
}

void TimetableExport::getRoomsTimetable(Solution &c){
	//assert(gt.timePopulation.initialized);
	assert(gt.rules.initialized && gt.rules.internalStructureComputed);

	//assert(c.HFitness()==0); - for perfect solutions
	c.getRoomsTimetable(gt.rules, rooms_timetable_weekly);
	best_solution.copy(gt.rules, c);
	rooms_schedule_ready=true;
}


void TimetableExport::writeSimulationResults(){
	QDir dir;
	
	QString OUTPUT_DIR_TIMETABLES=OUTPUT_DIR+FILE_SEP+"timetables";
	
	OUTPUT_DIR_TIMETABLES.append(FILE_SEP);
	if(INPUT_FILENAME_XML=="")
		OUTPUT_DIR_TIMETABLES.append("unnamed");
	else{
		OUTPUT_DIR_TIMETABLES.append(INPUT_FILENAME_XML.right(INPUT_FILENAME_XML.length()-INPUT_FILENAME_XML.findRev(FILE_SEP)-1));
		if(OUTPUT_DIR_TIMETABLES.right(4)==".fet")
			OUTPUT_DIR_TIMETABLES=OUTPUT_DIR_TIMETABLES.left(OUTPUT_DIR_TIMETABLES.length()-4);
		else if(INPUT_FILENAME_XML!="")
			cout<<"Minor problem - input file does not end in .fet extension - might be a problem when saving the timetables"<<" (file:"<<__FILE__<<", line:"<<__LINE__<<")"<<endl;
	}
	OUTPUT_DIR_TIMETABLES.append("-single");
	
	//make sure that the output directory exists
	if(!dir.exists(OUTPUT_DIR_TIMETABLES))
		dir.mkpath(OUTPUT_DIR_TIMETABLES);

	assert(gt.rules.initialized && gt.rules.internalStructureComputed);
	//assert(gt.timePopulation.initialized);
	assert(students_schedule_ready && teachers_schedule_ready && rooms_schedule_ready);
	assert(TIMETABLE_HTML_LEVEL>=0);
	assert(TIMETABLE_HTML_LEVEL<=6);

	computeHashForIDsTimetable();
	computeActivitiesAtTime();
	computeActivitiesWithSameStartingTime();

	QString s;
	QString bar;
	if(INPUT_FILENAME_XML=="")
		bar="";
	else
		bar="_";
	QString s2=INPUT_FILENAME_XML.right(INPUT_FILENAME_XML.length()-INPUT_FILENAME_XML.findRev(FILE_SEP)-1);
	if(s2.right(4)==".fet")
		s2=s2.left(s2.length()-4);
	//else if(INPUT_FILENAME_XML!="")
	//	cout<<"Minor problem - input file does not end in .fet extension - might be a problem when saving the timetables"<<" (file:"<<__FILE__<<", line:"<<__LINE__<<")"<<endl;
	
	//now write the solution in xml files
	//subgroups
	s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+SUBGROUPS_TIMETABLE_FILENAME_XML;
	writeSubgroupsTimetableXml(s);
	//teachers
	s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+TEACHERS_TIMETABLE_FILENAME_XML;
	writeTeachersTimetableXml(s);
	//activities
	s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+ACTIVITIES_TIMETABLE_FILENAME_XML;
	writeActivitiesTimetableXml(s);

	//now get the time. TODO: maybe write it in xml too? so do it a few lines earlier!
	QDate dat=QDate::currentDate();
	QTime tim=QTime::currentTime();
	QLocale loc(FET_LANGUAGE);
	QString sTime=loc.toString(dat, QLocale::ShortFormat)+" "+loc.toString(tim, QLocale::ShortFormat);

	//now get the number of placed activities. TODO: maybe write it in xml too? so do it a few lines earlier!
	int na=0;
	for(int i=0; i<gt.rules.nInternalActivities; i++)
		if(best_solution.times[i]!=UNALLOCATED_TIME)
			na++;
			
	int na2=0;
	for(int i=0; i<gt.rules.nInternalActivities; i++)
		if(best_solution.rooms[i]!=UNALLOCATED_SPACE)
			na2++;
	
	if(na==gt.rules.nInternalActivities && na==na2){
		s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+MULTIPLE_TIMETABLE_DATA_RESULTS_FILE;
		cout<<"Since simulation is complete, FET will write also the timetable data file"<<endl;
		writeTimetableDataFile(s);
	}
	
	//write the conflicts in txt mode
	s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+CONFLICTS_FILENAME;
	writeConflictsTxt(s, sTime, na);
	
	//now write the solution in html files
	if(TIMETABLE_HTML_LEVEL>=1){
		s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+STYLESHEET_CSS;
		writeStylesheetCss(s, sTime, na);
	}
	
	//indexHtml
	s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+INDEX_HTML;
	writeIndexHtml(s, sTime, na);
	
	//subgroups
	s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+SUBGROUPS_TIMETABLE_DAYS_HORIZONTAL_FILENAME_HTML;
	writeSubgroupsTimetableDaysHorizontalHtml(s, sTime, na);
	s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+SUBGROUPS_TIMETABLE_DAYS_VERTICAL_FILENAME_HTML;
	writeSubgroupsTimetableDaysVerticalHtml(s, sTime, na);
	if(!DIVIDE_HTML_TIMETABLES_WITH_TIME_AXIS_BY_DAYS){
		s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+SUBGROUPS_TIMETABLE_TIME_HORIZONTAL_FILENAME_HTML;
		writeSubgroupsTimetableTimeHorizontalHtml(s, sTime, na);
		s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+SUBGROUPS_TIMETABLE_TIME_VERTICAL_FILENAME_HTML;
		writeSubgroupsTimetableTimeVerticalHtml(s, sTime, na);
	} else {
		s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+SUBGROUPS_TIMETABLE_TIME_HORIZONTAL_FILENAME_HTML;
		writeSubgroupsTimetableTimeHorizontalDailyHtml(s, sTime, na);
		s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+SUBGROUPS_TIMETABLE_TIME_VERTICAL_FILENAME_HTML;
		writeSubgroupsTimetableTimeVerticalDailyHtml(s, sTime, na);
	}
	//groups
	s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+GROUPS_TIMETABLE_DAYS_HORIZONTAL_FILENAME_HTML;
	writeGroupsTimetableDaysHorizontalHtml(s, sTime, na);
	s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+GROUPS_TIMETABLE_DAYS_VERTICAL_FILENAME_HTML;
	writeGroupsTimetableDaysVerticalHtml(s, sTime, na);
	if(!DIVIDE_HTML_TIMETABLES_WITH_TIME_AXIS_BY_DAYS){
		s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+GROUPS_TIMETABLE_TIME_HORIZONTAL_FILENAME_HTML;
		writeGroupsTimetableTimeHorizontalHtml(s, sTime, na);
		s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+GROUPS_TIMETABLE_TIME_VERTICAL_FILENAME_HTML;
		writeGroupsTimetableTimeVerticalHtml(s, sTime, na);
	} else {
		s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+GROUPS_TIMETABLE_TIME_HORIZONTAL_FILENAME_HTML;
		writeGroupsTimetableTimeHorizontalDailyHtml(s, sTime, na);
		s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+GROUPS_TIMETABLE_TIME_VERTICAL_FILENAME_HTML;
		writeGroupsTimetableTimeVerticalDailyHtml(s, sTime, na);
	}
	//years
	s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+YEARS_TIMETABLE_DAYS_HORIZONTAL_FILENAME_HTML;
	writeYearsTimetableDaysHorizontalHtml(s, sTime, na);
	s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+YEARS_TIMETABLE_DAYS_VERTICAL_FILENAME_HTML;
	writeYearsTimetableDaysVerticalHtml(s, sTime, na);
	if(!DIVIDE_HTML_TIMETABLES_WITH_TIME_AXIS_BY_DAYS){
		s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+YEARS_TIMETABLE_TIME_HORIZONTAL_FILENAME_HTML;
		writeYearsTimetableTimeHorizontalHtml(s, sTime, na);
		s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+YEARS_TIMETABLE_TIME_VERTICAL_FILENAME_HTML;
		writeYearsTimetableTimeVerticalHtml(s, sTime, na);
	} else {
		s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+YEARS_TIMETABLE_TIME_HORIZONTAL_FILENAME_HTML;
		writeYearsTimetableTimeHorizontalDailyHtml(s, sTime, na);
		s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+YEARS_TIMETABLE_TIME_VERTICAL_FILENAME_HTML;
		writeYearsTimetableTimeVerticalDailyHtml(s, sTime, na);
	}
	//teachers
	s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+TEACHERS_TIMETABLE_DAYS_HORIZONTAL_FILENAME_HTML;
	writeTeachersTimetableDaysHorizontalHtml(s, sTime, na);
	s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+TEACHERS_TIMETABLE_DAYS_VERTICAL_FILENAME_HTML;
	writeTeachersTimetableDaysVerticalHtml(s, sTime, na);
	if(!DIVIDE_HTML_TIMETABLES_WITH_TIME_AXIS_BY_DAYS){
		s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+TEACHERS_TIMETABLE_TIME_HORIZONTAL_FILENAME_HTML;
		writeTeachersTimetableTimeHorizontalHtml(s, sTime, na);
		s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+TEACHERS_TIMETABLE_TIME_VERTICAL_FILENAME_HTML;
		writeTeachersTimetableTimeVerticalHtml(s, sTime, na);
	} else {
		s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+TEACHERS_TIMETABLE_TIME_HORIZONTAL_FILENAME_HTML;
		writeTeachersTimetableTimeHorizontalDailyHtml(s, sTime, na);
		s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+TEACHERS_TIMETABLE_TIME_VERTICAL_FILENAME_HTML;
		writeTeachersTimetableTimeVerticalDailyHtml(s, sTime, na);
	}
	//rooms
	s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+ROOMS_TIMETABLE_DAYS_HORIZONTAL_FILENAME_HTML;
	writeRoomsTimetableDaysHorizontalHtml(s, sTime, na);
	s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+ROOMS_TIMETABLE_DAYS_VERTICAL_FILENAME_HTML;
	writeRoomsTimetableDaysVerticalHtml(s, sTime, na);
	if(!DIVIDE_HTML_TIMETABLES_WITH_TIME_AXIS_BY_DAYS){
		s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+ROOMS_TIMETABLE_TIME_HORIZONTAL_FILENAME_HTML;
		writeRoomsTimetableTimeHorizontalHtml(s, sTime, na);
		s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+ROOMS_TIMETABLE_TIME_VERTICAL_FILENAME_HTML;
		writeRoomsTimetableTimeVerticalHtml(s, sTime, na);
	} else {
		s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+ROOMS_TIMETABLE_TIME_HORIZONTAL_FILENAME_HTML;
		writeRoomsTimetableTimeHorizontalDailyHtml(s, sTime, na);
		s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+ROOMS_TIMETABLE_TIME_VERTICAL_FILENAME_HTML;
		writeRoomsTimetableTimeVerticalDailyHtml(s, sTime, na);
	}
	//subjects
	s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+SUBJECTS_TIMETABLE_DAYS_HORIZONTAL_FILENAME_HTML;
	writeSubjectsTimetableDaysHorizontalHtml(s, sTime, na);
	s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+SUBJECTS_TIMETABLE_DAYS_VERTICAL_FILENAME_HTML;
	writeSubjectsTimetableDaysVerticalHtml(s, sTime, na);
	if(!DIVIDE_HTML_TIMETABLES_WITH_TIME_AXIS_BY_DAYS){
		s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+SUBJECTS_TIMETABLE_TIME_HORIZONTAL_FILENAME_HTML;
		writeSubjectsTimetableTimeHorizontalHtml(s, sTime, na);
		s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+SUBJECTS_TIMETABLE_TIME_VERTICAL_FILENAME_HTML;
		writeSubjectsTimetableTimeVerticalHtml(s, sTime, na);
	} else {
		s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+SUBJECTS_TIMETABLE_TIME_HORIZONTAL_FILENAME_HTML;
		writeSubjectsTimetableTimeHorizontalDailyHtml(s, sTime, na);
		s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+SUBJECTS_TIMETABLE_TIME_VERTICAL_FILENAME_HTML;
		writeSubjectsTimetableTimeVerticalDailyHtml(s, sTime, na);
	}
	//all activities
	s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+ALL_ACTIVITIES_TIMETABLE_DAYS_HORIZONTAL_FILENAME_HTML;
	writeAllActivitiesTimetableDaysHorizontalHtml(s, sTime, na);
	s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+ALL_ACTIVITIES_TIMETABLE_DAYS_VERTICAL_FILENAME_HTML;
	writeAllActivitiesTimetableDaysVerticalHtml(s, sTime, na);
	if(!DIVIDE_HTML_TIMETABLES_WITH_TIME_AXIS_BY_DAYS){
		s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+ALL_ACTIVITIES_TIMETABLE_TIME_HORIZONTAL_FILENAME_HTML;
		writeAllActivitiesTimetableTimeHorizontalHtml(s, sTime, na);
		s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+ALL_ACTIVITIES_TIMETABLE_TIME_VERTICAL_FILENAME_HTML;
		writeAllActivitiesTimetableTimeVerticalHtml(s, sTime, na);
	} else {
		s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+ALL_ACTIVITIES_TIMETABLE_TIME_HORIZONTAL_FILENAME_HTML;
		writeAllActivitiesTimetableTimeHorizontalDailyHtml(s, sTime, na);
		s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+ALL_ACTIVITIES_TIMETABLE_TIME_VERTICAL_FILENAME_HTML;
		writeAllActivitiesTimetableTimeVerticalDailyHtml(s, sTime, na);
	}
	//teachers free periods
	s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+TEACHERS_FREE_PERIODS_TIMETABLE_DAYS_HORIZONTAL_FILENAME_HTML;
	writeTeachersFreePeriodsTimetableDaysHorizontalHtml(s, sTime, na);
	s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+TEACHERS_FREE_PERIODS_TIMETABLE_DAYS_VERTICAL_FILENAME_HTML;
	writeTeachersFreePeriodsTimetableDaysVerticalHtml(s, sTime, na);

	hashSubjectIDsTimetable.clear();
	hashActivityTagIDsTimetable.clear();
	hashStudentIDsTimetable.clear();
	hashTeacherIDsTimetable.clear();
	hashRoomIDsTimetable.clear();
	hashDayIDsTimetable.clear();

	cout<<"Writing simulation results to disk completed successfully"<<endl;
}

void TimetableExport::writeHighestStageResults(){
	QDir dir;
	
	QString OUTPUT_DIR_TIMETABLES=OUTPUT_DIR+FILE_SEP+"timetables";
	
	OUTPUT_DIR_TIMETABLES.append(FILE_SEP);
	if(INPUT_FILENAME_XML=="")
		OUTPUT_DIR_TIMETABLES.append("unnamed");
	else{
		OUTPUT_DIR_TIMETABLES.append(INPUT_FILENAME_XML.right(INPUT_FILENAME_XML.length()-INPUT_FILENAME_XML.findRev(FILE_SEP)-1));
		if(OUTPUT_DIR_TIMETABLES.right(4)==".fet")
			OUTPUT_DIR_TIMETABLES=OUTPUT_DIR_TIMETABLES.left(OUTPUT_DIR_TIMETABLES.length()-4);
		else if(INPUT_FILENAME_XML!="")
			cout<<"Minor problem - input file does not end in .fet extension - might be a problem when saving the timetables"<<" (file:"<<__FILE__<<", line:"<<__LINE__<<")"<<endl;
	}
	OUTPUT_DIR_TIMETABLES.append("-highest");
	
	//make sure that the output directory exists
	if(!dir.exists(OUTPUT_DIR_TIMETABLES))
		dir.mkpath(OUTPUT_DIR_TIMETABLES);

	assert(gt.rules.initialized && gt.rules.internalStructureComputed);
	//assert(gt.timePopulation.initialized);
	assert(students_schedule_ready && teachers_schedule_ready && rooms_schedule_ready);
	assert(TIMETABLE_HTML_LEVEL>=0);
	assert(TIMETABLE_HTML_LEVEL<=6);

	computeHashForIDsTimetable();
	computeActivitiesAtTime();
	computeActivitiesWithSameStartingTime();

	QString s;
	QString bar;
	if(INPUT_FILENAME_XML=="")
		bar="";
	else
		bar="_";
	QString s2=INPUT_FILENAME_XML.right(INPUT_FILENAME_XML.length()-INPUT_FILENAME_XML.findRev(FILE_SEP)-1);
	if(s2.right(4)==".fet")
		s2=s2.left(s2.length()-4);
	//else if(INPUT_FILENAME_XML!="")
	//	cout<<"Minor problem - input file does not end in .fet extension - might be a problem when saving the timetables"<<" (file:"<<__FILE__<<", line:"<<__LINE__<<")"<<endl;
	
	//now write the solution in xml files
	//subgroups
	s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+SUBGROUPS_TIMETABLE_FILENAME_XML;
	writeSubgroupsTimetableXml(s);
	//teachers
	s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+TEACHERS_TIMETABLE_FILENAME_XML;
	writeTeachersTimetableXml(s);
	//activities
	s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+ACTIVITIES_TIMETABLE_FILENAME_XML;
	writeActivitiesTimetableXml(s);

	//now get the time. TODO: maybe write it in xml too? so do it a few lines earlier!
	QDate dat=QDate::currentDate();
	QTime tim=QTime::currentTime();
	QLocale loc(FET_LANGUAGE);
	QString sTime=loc.toString(dat, QLocale::ShortFormat)+" "+loc.toString(tim, QLocale::ShortFormat);

	//now get the number of placed activities. TODO: maybe write it in xml too? so do it a few lines earlier!
	int na=0;
	for(int i=0; i<gt.rules.nInternalActivities; i++)
		if(best_solution.times[i]!=UNALLOCATED_TIME)
			na++;
			
	int na2=0;
	for(int i=0; i<gt.rules.nInternalActivities; i++)
		if(best_solution.rooms[i]!=UNALLOCATED_SPACE)
			na2++;
	
	if(na==gt.rules.nInternalActivities && na==na2){
		s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+MULTIPLE_TIMETABLE_DATA_RESULTS_FILE;
		cout<<"Since simulation is complete, FET will write also the timetable data file"<<endl;
		writeTimetableDataFile(s);
	}
	
	//write the conflicts in txt mode
	s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+CONFLICTS_FILENAME;
	writeConflictsTxt(s, sTime, na);
	
	//now write the solution in html files
	if(TIMETABLE_HTML_LEVEL>=1){
		s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+STYLESHEET_CSS;
		writeStylesheetCss(s, sTime, na);
	}
	
	//indexHtml
	s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+INDEX_HTML;
	writeIndexHtml(s, sTime, na);
	
	//subgroups
	s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+SUBGROUPS_TIMETABLE_DAYS_HORIZONTAL_FILENAME_HTML;
	writeSubgroupsTimetableDaysHorizontalHtml(s, sTime, na);
	s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+SUBGROUPS_TIMETABLE_DAYS_VERTICAL_FILENAME_HTML;
	writeSubgroupsTimetableDaysVerticalHtml(s, sTime, na);
	if(!DIVIDE_HTML_TIMETABLES_WITH_TIME_AXIS_BY_DAYS){
		s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+SUBGROUPS_TIMETABLE_TIME_HORIZONTAL_FILENAME_HTML;
		writeSubgroupsTimetableTimeHorizontalHtml(s, sTime, na);
		s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+SUBGROUPS_TIMETABLE_TIME_VERTICAL_FILENAME_HTML;
		writeSubgroupsTimetableTimeVerticalHtml(s, sTime, na);
	} else {
		s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+SUBGROUPS_TIMETABLE_TIME_HORIZONTAL_FILENAME_HTML;
		writeSubgroupsTimetableTimeHorizontalDailyHtml(s, sTime, na);
		s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+SUBGROUPS_TIMETABLE_TIME_VERTICAL_FILENAME_HTML;
		writeSubgroupsTimetableTimeVerticalDailyHtml(s, sTime, na);
	}
	//groups
	s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+GROUPS_TIMETABLE_DAYS_HORIZONTAL_FILENAME_HTML;
	writeGroupsTimetableDaysHorizontalHtml(s, sTime, na);
	s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+GROUPS_TIMETABLE_DAYS_VERTICAL_FILENAME_HTML;
	writeGroupsTimetableDaysVerticalHtml(s, sTime, na);
	if(!DIVIDE_HTML_TIMETABLES_WITH_TIME_AXIS_BY_DAYS){
		s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+GROUPS_TIMETABLE_TIME_HORIZONTAL_FILENAME_HTML;
		writeGroupsTimetableTimeHorizontalHtml(s, sTime, na);
		s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+GROUPS_TIMETABLE_TIME_VERTICAL_FILENAME_HTML;
		writeGroupsTimetableTimeVerticalHtml(s, sTime, na);
	} else {
		s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+GROUPS_TIMETABLE_TIME_HORIZONTAL_FILENAME_HTML;
		writeGroupsTimetableTimeHorizontalDailyHtml(s, sTime, na);
		s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+GROUPS_TIMETABLE_TIME_VERTICAL_FILENAME_HTML;
		writeGroupsTimetableTimeVerticalDailyHtml(s, sTime, na);
	}
	//years
	s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+YEARS_TIMETABLE_DAYS_HORIZONTAL_FILENAME_HTML;
	writeYearsTimetableDaysHorizontalHtml(s, sTime, na);
	s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+YEARS_TIMETABLE_DAYS_VERTICAL_FILENAME_HTML;
	writeYearsTimetableDaysVerticalHtml(s, sTime, na);
	if(!DIVIDE_HTML_TIMETABLES_WITH_TIME_AXIS_BY_DAYS){
		s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+YEARS_TIMETABLE_TIME_HORIZONTAL_FILENAME_HTML;
		writeYearsTimetableTimeHorizontalHtml(s, sTime, na);
		s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+YEARS_TIMETABLE_TIME_VERTICAL_FILENAME_HTML;
		writeYearsTimetableTimeVerticalHtml(s, sTime, na);
	} else {
		s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+YEARS_TIMETABLE_TIME_HORIZONTAL_FILENAME_HTML;
		writeYearsTimetableTimeHorizontalDailyHtml(s, sTime, na);
		s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+YEARS_TIMETABLE_TIME_VERTICAL_FILENAME_HTML;
		writeYearsTimetableTimeVerticalDailyHtml(s, sTime, na);
	}
	//teachers
	s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+TEACHERS_TIMETABLE_DAYS_HORIZONTAL_FILENAME_HTML;
	writeTeachersTimetableDaysHorizontalHtml(s, sTime, na);
	s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+TEACHERS_TIMETABLE_DAYS_VERTICAL_FILENAME_HTML;
	writeTeachersTimetableDaysVerticalHtml(s, sTime, na);
	if(!DIVIDE_HTML_TIMETABLES_WITH_TIME_AXIS_BY_DAYS){
		s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+TEACHERS_TIMETABLE_TIME_HORIZONTAL_FILENAME_HTML;
		writeTeachersTimetableTimeHorizontalHtml(s, sTime, na);
		s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+TEACHERS_TIMETABLE_TIME_VERTICAL_FILENAME_HTML;
		writeTeachersTimetableTimeVerticalHtml(s, sTime, na);
	} else {
		s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+TEACHERS_TIMETABLE_TIME_HORIZONTAL_FILENAME_HTML;
		writeTeachersTimetableTimeHorizontalDailyHtml(s, sTime, na);
		s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+TEACHERS_TIMETABLE_TIME_VERTICAL_FILENAME_HTML;
		writeTeachersTimetableTimeVerticalDailyHtml(s, sTime, na);
	}
	//rooms
	s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+ROOMS_TIMETABLE_DAYS_HORIZONTAL_FILENAME_HTML;
	writeRoomsTimetableDaysHorizontalHtml(s, sTime, na);
	s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+ROOMS_TIMETABLE_DAYS_VERTICAL_FILENAME_HTML;
	writeRoomsTimetableDaysVerticalHtml(s, sTime, na);
	if(!DIVIDE_HTML_TIMETABLES_WITH_TIME_AXIS_BY_DAYS){
		s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+ROOMS_TIMETABLE_TIME_HORIZONTAL_FILENAME_HTML;
		writeRoomsTimetableTimeHorizontalHtml(s, sTime, na);
		s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+ROOMS_TIMETABLE_TIME_VERTICAL_FILENAME_HTML;
		writeRoomsTimetableTimeVerticalHtml(s, sTime, na);
	} else {
		s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+ROOMS_TIMETABLE_TIME_HORIZONTAL_FILENAME_HTML;
		writeRoomsTimetableTimeHorizontalDailyHtml(s, sTime, na);
		s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+ROOMS_TIMETABLE_TIME_VERTICAL_FILENAME_HTML;
		writeRoomsTimetableTimeVerticalDailyHtml(s, sTime, na);
	}
	//subjects
	s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+SUBJECTS_TIMETABLE_DAYS_HORIZONTAL_FILENAME_HTML;
	writeSubjectsTimetableDaysHorizontalHtml(s, sTime, na);
	s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+SUBJECTS_TIMETABLE_DAYS_VERTICAL_FILENAME_HTML;
	writeSubjectsTimetableDaysVerticalHtml(s, sTime, na);
	if(!DIVIDE_HTML_TIMETABLES_WITH_TIME_AXIS_BY_DAYS){
		s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+SUBJECTS_TIMETABLE_TIME_HORIZONTAL_FILENAME_HTML;
		writeSubjectsTimetableTimeHorizontalHtml(s, sTime, na);
		s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+SUBJECTS_TIMETABLE_TIME_VERTICAL_FILENAME_HTML;
		writeSubjectsTimetableTimeVerticalHtml(s, sTime, na);
	} else {
		s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+SUBJECTS_TIMETABLE_TIME_HORIZONTAL_FILENAME_HTML;
		writeSubjectsTimetableTimeHorizontalDailyHtml(s, sTime, na);
		s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+SUBJECTS_TIMETABLE_TIME_VERTICAL_FILENAME_HTML;
		writeSubjectsTimetableTimeVerticalDailyHtml(s, sTime, na);
	}
	//all activities
	s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+ALL_ACTIVITIES_TIMETABLE_DAYS_HORIZONTAL_FILENAME_HTML;
	writeAllActivitiesTimetableDaysHorizontalHtml(s, sTime, na);
	s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+ALL_ACTIVITIES_TIMETABLE_DAYS_VERTICAL_FILENAME_HTML;
	writeAllActivitiesTimetableDaysVerticalHtml(s, sTime, na);
	if(!DIVIDE_HTML_TIMETABLES_WITH_TIME_AXIS_BY_DAYS){
		s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+ALL_ACTIVITIES_TIMETABLE_TIME_HORIZONTAL_FILENAME_HTML;
		writeAllActivitiesTimetableTimeHorizontalHtml(s, sTime, na);
		s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+ALL_ACTIVITIES_TIMETABLE_TIME_VERTICAL_FILENAME_HTML;
		writeAllActivitiesTimetableTimeVerticalHtml(s, sTime, na);
	} else {
		s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+ALL_ACTIVITIES_TIMETABLE_TIME_HORIZONTAL_FILENAME_HTML;
		writeAllActivitiesTimetableTimeHorizontalDailyHtml(s, sTime, na);
		s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+ALL_ACTIVITIES_TIMETABLE_TIME_VERTICAL_FILENAME_HTML;
		writeAllActivitiesTimetableTimeVerticalDailyHtml(s, sTime, na);
	}
	//teachers free periods
	s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+TEACHERS_FREE_PERIODS_TIMETABLE_DAYS_HORIZONTAL_FILENAME_HTML;
	writeTeachersFreePeriodsTimetableDaysHorizontalHtml(s, sTime, na);
	s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+TEACHERS_FREE_PERIODS_TIMETABLE_DAYS_VERTICAL_FILENAME_HTML;
	writeTeachersFreePeriodsTimetableDaysVerticalHtml(s, sTime, na);

	hashSubjectIDsTimetable.clear();
	hashActivityTagIDsTimetable.clear();
	hashStudentIDsTimetable.clear();
	hashTeacherIDsTimetable.clear();
	hashRoomIDsTimetable.clear();
	hashDayIDsTimetable.clear();

	cout<<"Writing highest stage results to disk completed successfully"<<endl;
}



void TimetableExport::writeRandomSeed()
{
	QDir dir;
	
	QString OUTPUT_DIR_TIMETABLES=OUTPUT_DIR+FILE_SEP+"timetables";
	
	OUTPUT_DIR_TIMETABLES.append(FILE_SEP);
	if(INPUT_FILENAME_XML=="")
		OUTPUT_DIR_TIMETABLES.append("unnamed");
	else{
		OUTPUT_DIR_TIMETABLES.append(INPUT_FILENAME_XML.right(INPUT_FILENAME_XML.length()-INPUT_FILENAME_XML.findRev(FILE_SEP)-1));
		if(OUTPUT_DIR_TIMETABLES.right(4)==".fet")
			OUTPUT_DIR_TIMETABLES=OUTPUT_DIR_TIMETABLES.left(OUTPUT_DIR_TIMETABLES.length()-4);
		else if(INPUT_FILENAME_XML!="")
			cout<<"Minor problem - input file does not end in .fet extension - might be a problem when saving the timetables"<<" (file:"<<__FILE__<<", line:"<<__LINE__<<")"<<endl;
	}
	OUTPUT_DIR_TIMETABLES.append("-single");
	
	//make sure that the output directory exists
	if(!dir.exists(OUTPUT_DIR_TIMETABLES))
		dir.mkpath(OUTPUT_DIR_TIMETABLES);

	QString s;
	QString bar;
	if(INPUT_FILENAME_XML=="")
		bar="";
	else
		bar="_";
	QString s2=INPUT_FILENAME_XML.right(INPUT_FILENAME_XML.length()-INPUT_FILENAME_XML.findRev(FILE_SEP)-1);
	if(s2.right(4)==".fet")
		s2=s2.left(s2.length()-4);

	s=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+bar+RANDOM_SEED_FILENAME;
	
	writeRandomSeedFile(s);
}

void TimetableExport::writeRandomSeedFile(const QString& filename)
{
	QString s=filename;

	QFile file(s);
	if(!file.open(QIODevice::WriteOnly)){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Cannot open file %1 for writing. Please check your disk's free space. Saving of %1 aborted.").arg(s));
		return;
		assert(0);
	}
	QTextStream tos(&file);
	tos.setCodec("UTF-8");
	tos.setGenerateByteOrderMark(true);

	QDate dat=QDate::currentDate();
	QTime tim=QTime::currentTime();
	QLocale loc(FET_LANGUAGE);
	QString sTime=loc.toString(dat, QLocale::ShortFormat)+" "+loc.toString(tim, QLocale::ShortFormat);
	
	tos<<tr("Generation started on: %1", "%1 is the time").arg(sTime);
	tos<<endl<<endl;
	tos<<tr("Random seed at the start of generation is: X=%1, Y=%2", "The random seed has two components, X and Y").arg(XX).arg(YY);
	tos<<endl<<endl;
	tos<<tr("This file was automatically generated by FET %1.").arg(FET_VERSION);
	tos<<endl;
	
	if(file.error()>0){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Writing %1 gave error code %2, which means saving is compromised. Please check your disk's free space.").arg(s).arg(file.error()));
	}
	file.close();
}



void TimetableExport::writeTimetableDataFile(const QString& filename){
	if(!students_schedule_ready || !teachers_schedule_ready || !rooms_schedule_ready){
		QMessageBox::critical(NULL, tr("FET - Critical"), tr("Timetable not generated - cannot save it - this should not happen (please report bug)"));
		return;	
	}

	Solution* tc=&best_solution;

	for(int ai=0; ai<gt.rules.nInternalActivities; ai++){
		//Activity* act=&gt.rules.internalActivitiesList[ai];
		int time=tc->times[ai];
		if(time==UNALLOCATED_TIME){
			QMessageBox::critical(NULL, tr("FET - Critical"), tr("Incomplete timetable - this should not happen - please report bug"));
			return;	
		}
		
		int ri=tc->rooms[ai];
		if(ri==UNALLOCATED_SPACE){
			QMessageBox::critical(NULL, tr("FET - Critical"), tr("Incomplete timetable - this should not happen - please report bug"));
			return;	
		}
	}
	
	rules2.initialized=true;
	
	rules2.institutionName=gt.rules.institutionName;
	rules2.comments=gt.rules.comments;
	
	rules2.nHoursPerDay=gt.rules.nHoursPerDay;
	for(int i=0; i<gt.rules.nHoursPerDay; i++)
		rules2.hoursOfTheDay[i]=gt.rules.hoursOfTheDay[i];

	rules2.nDaysPerWeek=gt.rules.nDaysPerWeek;
	for(int i=0; i<gt.rules.nDaysPerWeek; i++)
		rules2.daysOfTheWeek[i]=gt.rules.daysOfTheWeek[i];
		
	rules2.yearsList=gt.rules.yearsList;
	
	rules2.teachersList=gt.rules.teachersList;
	
	rules2.subjectsList=gt.rules.subjectsList;
	
	rules2.activityTagsList=gt.rules.activityTagsList;

	rules2.activitiesList=gt.rules.activitiesList;

	rules2.buildingsList=gt.rules.buildingsList;

	rules2.roomsList=gt.rules.roomsList;

	rules2.timeConstraintsList=gt.rules.timeConstraintsList;
	
	rules2.spaceConstraintsList=gt.rules.spaceConstraintsList;


	//add locking constraints
	TimeConstraintsList lockTimeConstraintsList;
	SpaceConstraintsList lockSpaceConstraintsList;


	bool report=false;
	
	int addedTime=0, duplicatesTime=0;
	int addedSpace=0, duplicatesSpace=0;

	//lock selected activities
	for(int ai=0; ai<gt.rules.nInternalActivities; ai++){
		Activity* act=&gt.rules.internalActivitiesList[ai];
		int time=tc->times[ai];
		if(time>=0 && time<gt.rules.nDaysPerWeek*gt.rules.nHoursPerDay){
			int hour=time/gt.rules.nDaysPerWeek;
			int day=time%gt.rules.nDaysPerWeek;

			ConstraintActivityPreferredStartingTime* ctr=new ConstraintActivityPreferredStartingTime(100.0, act->id, day, hour, false); //permanently locked is false
			bool t=rules2.addTimeConstraint(ctr);
						
			if(t){
				addedTime++;
				lockTimeConstraintsList.append(ctr);
			}
			else
				duplicatesTime++;

			QString s;
						
			if(t)
				s=tr("Added the following constraint to saved file:")+"\n"+ctr->getDetailedDescription(gt.rules);
			else{
				s=tr("Constraint\n%1 NOT added to saved file - duplicate").arg(ctr->getDetailedDescription(gt.rules));
				delete ctr;
			}
						
			if(report){
				int k;
				if(t)
					k=QMessageBox::information(NULL, tr("FET information"), s,
				 	 tr("Skip information"), tr("See next"), QString(), 1, 0 );
				else
					k=QMessageBox::warning(NULL, tr("FET warning"), s,
				 	 tr("Skip information"), tr("See next"), QString(), 1, 0 );
		 		if(k==0)
					report=false;
			}
		}
					
		int ri=tc->rooms[ai];
		if(ri!=UNALLOCATED_SPACE && ri!=UNSPECIFIED_ROOM && ri>=0 && ri<gt.rules.nInternalRooms){
			ConstraintActivityPreferredRoom* ctr=new ConstraintActivityPreferredRoom(100, act->id, (gt.rules.internalRoomsList[ri])->name, false); //permanently locked is false
			bool t=rules2.addSpaceConstraint(ctr);

			QString s;
						
			if(t){
				addedSpace++;
				lockSpaceConstraintsList.append(ctr);
			}
			else
				duplicatesSpace++;

			if(t)
				s=tr("Added the following constraint to saved file:")+"\n"+ctr->getDetailedDescription(gt.rules);
			else{
				s=tr("Constraint\n%1 NOT added to saved file - duplicate").arg(ctr->getDetailedDescription(gt.rules));
				delete ctr;
			}
						
			if(report){
				int k;
				if(t)
					k=QMessageBox::information(NULL, tr("FET information"), s,
				 	 tr("Skip information"), tr("See next"), QString(), 1, 0 );
				else
					k=QMessageBox::warning(NULL, tr("FET warning"), s,
					 tr("Skip information"), tr("See next"), QString(), 1, 0 );
				if(k==0)
					report=false;
			}
		}
	}

	//QMessageBox::information(NULL, tr("FET information"), tr("Added %1 locking time constraints and %2 locking space constraints to saved file,"
	// " ignored %3 activities which were already fixed in time and %4 activities which were already fixed in space").arg(addedTime).arg(addedSpace).arg(duplicatesTime).arg(duplicatesSpace));
		
	bool result=rules2.write(filename);
	
	while(!lockTimeConstraintsList.isEmpty())
		delete lockTimeConstraintsList.takeFirst();
	while(!lockSpaceConstraintsList.isEmpty())
		delete lockSpaceConstraintsList.takeFirst();

	//if(result)	
	//	QMessageBox::information(NULL, tr("FET information"),
	//		tr("File saved successfully. You can see it on the hard disk. Current data file remained untouched (of locking constraints),"
	//		" so you can save it also, or generate different timetables."));

	rules2.nHoursPerDay=0;
	rules2.nDaysPerWeek=0;

	rules2.yearsList.clear();
	
	rules2.teachersList.clear();
	
	rules2.subjectsList.clear();
	
	rules2.activityTagsList.clear();

	rules2.activitiesList.clear();

	rules2.buildingsList.clear();

	rules2.roomsList.clear();

	rules2.timeConstraintsList.clear();
	
	rules2.spaceConstraintsList.clear();
	
	if(!result){
		QMessageBox::critical(NULL, tr("FET critical"), tr("Could not save the data + timetable file on the hard disk - maybe hard disk is full"));
	}
}


void TimetableExport::writeSimulationResults(int n){
	QDir dir;
	
	QString OUTPUT_DIR_TIMETABLES=OUTPUT_DIR+FILE_SEP+"timetables";

	//make sure that the output directory exists
	if(!dir.exists(OUTPUT_DIR_TIMETABLES))
		dir.mkpath(OUTPUT_DIR_TIMETABLES);

	assert(gt.rules.initialized && gt.rules.internalStructureComputed);
	//assert(gt.timePopulation.initialized);
	assert(students_schedule_ready && teachers_schedule_ready && rooms_schedule_ready);
	assert(TIMETABLE_HTML_LEVEL>=0);
	assert(TIMETABLE_HTML_LEVEL<=6);

	computeHashForIDsTimetable();
	computeActivitiesAtTime();
	computeActivitiesWithSameStartingTime();


	QString s;
	QString s2=INPUT_FILENAME_XML.right(INPUT_FILENAME_XML.length()-INPUT_FILENAME_XML.findRev(FILE_SEP)-1);
	if(s2.right(4)==".fet")
		s2=s2.left(s2.length()-4);
	else if(INPUT_FILENAME_XML!="")
		cout<<"Minor problem - input file does not end in .fet extension - might be a problem when saving the timetables"<<" (file:"<<__FILE__<<", line:"<<__LINE__<<")"<<endl;
	
	QString destDir=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+"-multi";
	
	if(!dir.exists(destDir))
		dir.mkpath(destDir);
		
	QString finalDestDir=destDir+FILE_SEP+QString::number(n);

	if(!dir.exists(finalDestDir))
		dir.mkpath(finalDestDir);
		
	finalDestDir+=FILE_SEP;


	QString s3=INPUT_FILENAME_XML.right(INPUT_FILENAME_XML.length()-INPUT_FILENAME_XML.findRev(FILE_SEP)-1);

	if(s3.right(4)==".fet")
		s3=s3.left(s3.length()-4);
	//else if(INPUT_FILENAME_XML!="")
	//	cout<<"Minor problem - input file does not end in .fet extension - might be a problem when saving the timetables"<<" (file:"<<__FILE__<<", line:"<<__LINE__<<")"<<endl;


	finalDestDir+=s3+"_";

	
	//write data+timetable in .fet format
	writeTimetableDataFile(finalDestDir+MULTIPLE_TIMETABLE_DATA_RESULTS_FILE);

	//now write the solution in xml files
	//subgroups
	s=finalDestDir+SUBGROUPS_TIMETABLE_FILENAME_XML;
	writeSubgroupsTimetableXml(s);
	//teachers
	s=finalDestDir+TEACHERS_TIMETABLE_FILENAME_XML;
	writeTeachersTimetableXml(s);
	//activities
	s=finalDestDir+ACTIVITIES_TIMETABLE_FILENAME_XML;
	writeActivitiesTimetableXml(s);

	//now get the time. TODO: maybe write it in xml too? so do it a few lines earlier!
	QDate dat=QDate::currentDate();
	QTime tim=QTime::currentTime();
	QLocale loc(FET_LANGUAGE);
	QString sTime=loc.toString(dat, QLocale::ShortFormat)+" "+loc.toString(tim, QLocale::ShortFormat);

	//now get the number of placed activities. TODO: maybe write it in xml too? so do it a few lines earlier!
	int na=0;
	for(int i=0; i<gt.rules.nInternalActivities; i++)
		if(best_solution.times[i]!=UNALLOCATED_TIME)
			na++;
	
	//write the conflicts in txt mode
	s=finalDestDir+CONFLICTS_FILENAME;
	writeConflictsTxt(s, sTime, na);
	
	//now write the solution in html files
	if(TIMETABLE_HTML_LEVEL>=1){
		s=finalDestDir+STYLESHEET_CSS;
		writeStylesheetCss(s, sTime, na);
	}
	//indexHtml
	s=finalDestDir+INDEX_HTML;
	writeIndexHtml(s, sTime, na);
	//subgroups
	s=finalDestDir+SUBGROUPS_TIMETABLE_DAYS_HORIZONTAL_FILENAME_HTML;
	writeSubgroupsTimetableDaysHorizontalHtml(s, sTime, na);
	s=finalDestDir+SUBGROUPS_TIMETABLE_DAYS_VERTICAL_FILENAME_HTML;
	writeSubgroupsTimetableDaysVerticalHtml(s, sTime, na);
	if(!DIVIDE_HTML_TIMETABLES_WITH_TIME_AXIS_BY_DAYS){
		s=finalDestDir+SUBGROUPS_TIMETABLE_TIME_HORIZONTAL_FILENAME_HTML;
		writeSubgroupsTimetableTimeHorizontalHtml(s, sTime, na);
		s=finalDestDir+SUBGROUPS_TIMETABLE_TIME_VERTICAL_FILENAME_HTML;
		writeSubgroupsTimetableTimeVerticalHtml(s, sTime, na);
	} else {
		s=finalDestDir+SUBGROUPS_TIMETABLE_TIME_HORIZONTAL_FILENAME_HTML;
		writeSubgroupsTimetableTimeHorizontalDailyHtml(s, sTime, na);
		s=finalDestDir+SUBGROUPS_TIMETABLE_TIME_VERTICAL_FILENAME_HTML;
		writeSubgroupsTimetableTimeVerticalDailyHtml(s, sTime, na);
	}
	//groups
	s=finalDestDir+GROUPS_TIMETABLE_DAYS_HORIZONTAL_FILENAME_HTML;
	writeGroupsTimetableDaysHorizontalHtml(s, sTime, na);
	s=finalDestDir+GROUPS_TIMETABLE_DAYS_VERTICAL_FILENAME_HTML;
	writeGroupsTimetableDaysVerticalHtml(s, sTime, na);
	if(!DIVIDE_HTML_TIMETABLES_WITH_TIME_AXIS_BY_DAYS){
		s=finalDestDir+GROUPS_TIMETABLE_TIME_HORIZONTAL_FILENAME_HTML;
		writeGroupsTimetableTimeHorizontalHtml(s, sTime, na);
		s=finalDestDir+GROUPS_TIMETABLE_TIME_VERTICAL_FILENAME_HTML;
		writeGroupsTimetableTimeVerticalHtml(s, sTime, na);
	} else {
		s=finalDestDir+GROUPS_TIMETABLE_TIME_HORIZONTAL_FILENAME_HTML;
		writeGroupsTimetableTimeHorizontalDailyHtml(s, sTime, na);
		s=finalDestDir+GROUPS_TIMETABLE_TIME_VERTICAL_FILENAME_HTML;
		writeGroupsTimetableTimeVerticalDailyHtml(s, sTime, na);
	}
	//years
	s=finalDestDir+YEARS_TIMETABLE_DAYS_HORIZONTAL_FILENAME_HTML;
	writeYearsTimetableDaysHorizontalHtml(s, sTime, na);
	s=finalDestDir+YEARS_TIMETABLE_DAYS_VERTICAL_FILENAME_HTML;
	writeYearsTimetableDaysVerticalHtml(s, sTime, na);
	if(!DIVIDE_HTML_TIMETABLES_WITH_TIME_AXIS_BY_DAYS){
		s=finalDestDir+YEARS_TIMETABLE_TIME_HORIZONTAL_FILENAME_HTML;
		writeYearsTimetableTimeHorizontalHtml(s, sTime, na);
		s=finalDestDir+YEARS_TIMETABLE_TIME_VERTICAL_FILENAME_HTML;
		writeYearsTimetableTimeVerticalHtml(s, sTime, na);
	} else {
		s=finalDestDir+YEARS_TIMETABLE_TIME_HORIZONTAL_FILENAME_HTML;
		writeYearsTimetableTimeHorizontalDailyHtml(s, sTime, na);
		s=finalDestDir+YEARS_TIMETABLE_TIME_VERTICAL_FILENAME_HTML;
		writeYearsTimetableTimeVerticalDailyHtml(s, sTime, na);
	}
	//teachers
	s=finalDestDir+TEACHERS_TIMETABLE_DAYS_HORIZONTAL_FILENAME_HTML;
	writeTeachersTimetableDaysHorizontalHtml(s, sTime, na);
	s=finalDestDir+TEACHERS_TIMETABLE_DAYS_VERTICAL_FILENAME_HTML;
	writeTeachersTimetableDaysVerticalHtml(s, sTime, na);
	if(!DIVIDE_HTML_TIMETABLES_WITH_TIME_AXIS_BY_DAYS){
		s=finalDestDir+TEACHERS_TIMETABLE_TIME_HORIZONTAL_FILENAME_HTML;
		writeTeachersTimetableTimeHorizontalHtml(s, sTime, na);
		s=finalDestDir+TEACHERS_TIMETABLE_TIME_VERTICAL_FILENAME_HTML;
		writeTeachersTimetableTimeVerticalHtml(s, sTime, na);
	} else {
		s=finalDestDir+TEACHERS_TIMETABLE_TIME_HORIZONTAL_FILENAME_HTML;
		writeTeachersTimetableTimeHorizontalDailyHtml(s, sTime, na);
		s=finalDestDir+TEACHERS_TIMETABLE_TIME_VERTICAL_FILENAME_HTML;
		writeTeachersTimetableTimeVerticalDailyHtml(s, sTime, na);
	}
	//rooms
	s=finalDestDir+ROOMS_TIMETABLE_DAYS_HORIZONTAL_FILENAME_HTML;
	writeRoomsTimetableDaysHorizontalHtml(s, sTime, na);
	s=finalDestDir+ROOMS_TIMETABLE_DAYS_VERTICAL_FILENAME_HTML;
	writeRoomsTimetableDaysVerticalHtml(s, sTime, na);
	if(!DIVIDE_HTML_TIMETABLES_WITH_TIME_AXIS_BY_DAYS){
		s=finalDestDir+ROOMS_TIMETABLE_TIME_HORIZONTAL_FILENAME_HTML;
		writeRoomsTimetableTimeHorizontalHtml(s, sTime, na);
		s=finalDestDir+ROOMS_TIMETABLE_TIME_VERTICAL_FILENAME_HTML;
		writeRoomsTimetableTimeVerticalHtml(s, sTime, na);
	} else {
		s=finalDestDir+ROOMS_TIMETABLE_TIME_HORIZONTAL_FILENAME_HTML;
		writeRoomsTimetableTimeHorizontalDailyHtml(s, sTime, na);
		s=finalDestDir+ROOMS_TIMETABLE_TIME_VERTICAL_FILENAME_HTML;
		writeRoomsTimetableTimeVerticalDailyHtml(s, sTime, na);
	}
	//subjects
	s=finalDestDir+SUBJECTS_TIMETABLE_DAYS_HORIZONTAL_FILENAME_HTML;
	writeSubjectsTimetableDaysHorizontalHtml(s, sTime, na);
	s=finalDestDir+SUBJECTS_TIMETABLE_DAYS_VERTICAL_FILENAME_HTML;
	writeSubjectsTimetableDaysVerticalHtml(s, sTime, na);
	if(!DIVIDE_HTML_TIMETABLES_WITH_TIME_AXIS_BY_DAYS){
		s=finalDestDir+SUBJECTS_TIMETABLE_TIME_HORIZONTAL_FILENAME_HTML;
		writeSubjectsTimetableTimeHorizontalHtml(s, sTime, na);
		s=finalDestDir+SUBJECTS_TIMETABLE_TIME_VERTICAL_FILENAME_HTML;
		writeSubjectsTimetableTimeVerticalHtml(s, sTime, na);
	} else {
		s=finalDestDir+SUBJECTS_TIMETABLE_TIME_HORIZONTAL_FILENAME_HTML;
		writeSubjectsTimetableTimeHorizontalDailyHtml(s, sTime, na);
		s=finalDestDir+SUBJECTS_TIMETABLE_TIME_VERTICAL_FILENAME_HTML;
		writeSubjectsTimetableTimeVerticalDailyHtml(s, sTime, na);
	}
	//all activities
	s=finalDestDir+ALL_ACTIVITIES_TIMETABLE_DAYS_HORIZONTAL_FILENAME_HTML;
	writeAllActivitiesTimetableDaysHorizontalHtml(s, sTime, na);
	s=finalDestDir+ALL_ACTIVITIES_TIMETABLE_DAYS_VERTICAL_FILENAME_HTML;
	writeAllActivitiesTimetableDaysVerticalHtml(s, sTime, na);
	if(!DIVIDE_HTML_TIMETABLES_WITH_TIME_AXIS_BY_DAYS){
		s=finalDestDir+ALL_ACTIVITIES_TIMETABLE_TIME_HORIZONTAL_FILENAME_HTML;
		writeAllActivitiesTimetableTimeHorizontalHtml(s, sTime, na);
		s=finalDestDir+ALL_ACTIVITIES_TIMETABLE_TIME_VERTICAL_FILENAME_HTML;
		writeAllActivitiesTimetableTimeVerticalHtml(s, sTime, na);
	} else {
		s=finalDestDir+ALL_ACTIVITIES_TIMETABLE_TIME_HORIZONTAL_FILENAME_HTML;
		writeAllActivitiesTimetableTimeHorizontalDailyHtml(s, sTime, na);
		s=finalDestDir+ALL_ACTIVITIES_TIMETABLE_TIME_VERTICAL_FILENAME_HTML;
		writeAllActivitiesTimetableTimeVerticalDailyHtml(s, sTime, na);
	}
	//teachers free periods
	s=finalDestDir+TEACHERS_FREE_PERIODS_TIMETABLE_DAYS_HORIZONTAL_FILENAME_HTML;
	writeTeachersFreePeriodsTimetableDaysHorizontalHtml(s, sTime, na);
	s=finalDestDir+TEACHERS_FREE_PERIODS_TIMETABLE_DAYS_VERTICAL_FILENAME_HTML;
	writeTeachersFreePeriodsTimetableDaysVerticalHtml(s, sTime, na);
	
	hashSubjectIDsTimetable.clear();
	hashActivityTagIDsTimetable.clear();
	hashStudentIDsTimetable.clear();
	hashTeacherIDsTimetable.clear();
	hashRoomIDsTimetable.clear();
	hashDayIDsTimetable.clear();

	cout<<"Writing multiple simulation results to disk completed successfully"<<endl;
}




void TimetableExport::writeRandomSeed(int n){
	QDir dir;
	
	QString OUTPUT_DIR_TIMETABLES=OUTPUT_DIR+FILE_SEP+"timetables";

	//make sure that the output directory exists
	if(!dir.exists(OUTPUT_DIR_TIMETABLES))
		dir.mkpath(OUTPUT_DIR_TIMETABLES);

	QString s;
	QString s2=INPUT_FILENAME_XML.right(INPUT_FILENAME_XML.length()-INPUT_FILENAME_XML.findRev(FILE_SEP)-1);
	if(s2.right(4)==".fet")
		s2=s2.left(s2.length()-4);
	else if(INPUT_FILENAME_XML!="")
		cout<<"Minor problem - input file does not end in .fet extension - might be a problem when saving the timetables"<<" (file:"<<__FILE__<<", line:"<<__LINE__<<")"<<endl;
	
	QString destDir=OUTPUT_DIR_TIMETABLES+FILE_SEP+s2+"-multi";
	
	if(!dir.exists(destDir))
		dir.mkpath(destDir);
		
	QString finalDestDir=destDir+FILE_SEP+QString::number(n);

	if(!dir.exists(finalDestDir))
		dir.mkpath(finalDestDir);
		
	finalDestDir+=FILE_SEP;

	QString s3=INPUT_FILENAME_XML.right(INPUT_FILENAME_XML.length()-INPUT_FILENAME_XML.findRev(FILE_SEP)-1);

	if(s3.right(4)==".fet")
		s3=s3.left(s3.length()-4);
	//else if(INPUT_FILENAME_XML!="")
	//	cout<<"Minor problem - input file does not end in .fet extension - might be a problem when saving the timetables"<<" (file:"<<__FILE__<<", line:"<<__LINE__<<")"<<endl;

	finalDestDir+=s3+"_";
	
	s=finalDestDir+RANDOM_SEED_FILENAME;

	writeRandomSeedFile(s);
}



void TimetableExport::writeSimulationResultsCommandLine(const QString& outputDirectory){ //outputDirectory contains trailing FILE_SEP
	QString add=INPUT_FILENAME_XML.right(INPUT_FILENAME_XML.length()-INPUT_FILENAME_XML.findRev(FILE_SEP)-1);
	if(add.right(4)==".fet")
		add=add.left(add.length()-4);
	else if(INPUT_FILENAME_XML!="")
		cout<<"Minor problem - input file does not end in .fet extension - might be a problem when saving the timetables"<<" (file:"<<__FILE__<<", line:"<<__LINE__<<")"<<endl;

	if(add!="")
		add.append("_");

/////////

	assert(gt.rules.initialized && gt.rules.internalStructureComputed);
	assert(students_schedule_ready && teachers_schedule_ready && rooms_schedule_ready);
	assert(TIMETABLE_HTML_LEVEL>=0);
	assert(TIMETABLE_HTML_LEVEL<=6);

	computeHashForIDsTimetable();
	computeActivitiesAtTime();
	computeActivitiesWithSameStartingTime();


	TimetableExport::writeSubgroupsTimetableXml(outputDirectory+add+SUBGROUPS_TIMETABLE_FILENAME_XML);
	TimetableExport::writeTeachersTimetableXml(outputDirectory+add+TEACHERS_TIMETABLE_FILENAME_XML);
	TimetableExport::writeActivitiesTimetableXml(outputDirectory+add+ACTIVITIES_TIMETABLE_FILENAME_XML);
			
	//get the time
	QDate dat=QDate::currentDate();
	QTime tim=QTime::currentTime();
	QLocale loc(FET_LANGUAGE);
	QString sTime=loc.toString(dat, QLocale::ShortFormat)+" "+loc.toString(tim, QLocale::ShortFormat);
							
	//now get the number of placed activities. TODO: maybe write it in xml too? so do it a few lines earlier!
	int na=0;
	for(int i=0; i<gt.rules.nInternalActivities; i++)
		if(best_solution.times[i]!=UNALLOCATED_TIME)
			na++;

///////
	int na2=0;
	for(int i=0; i<gt.rules.nInternalActivities; i++)
		if(best_solution.rooms[i]!=UNALLOCATED_SPACE)
			na2++;
	
	if(na==gt.rules.nInternalActivities && na==na2){
		QString s=outputDirectory+add+MULTIPLE_TIMETABLE_DATA_RESULTS_FILE;
		cout<<"Since simulation is complete, FET will write also the timetable data file"<<endl;
		writeTimetableDataFile(s);
	}
///////
	
														//write the conflicts in txt mode
	QString s=add+CONFLICTS_FILENAME;
	s.prepend(outputDirectory);
	TimetableExport::writeConflictsTxt(s, sTime, na);
	
	//now write the solution in html files
	if(TIMETABLE_HTML_LEVEL>=1){
		s=add+STYLESHEET_CSS;
		s.prepend(outputDirectory);
		TimetableExport::writeStylesheetCss(s, sTime, na);
	}
	//indexHtml
	s=add+INDEX_HTML;
	s.prepend(outputDirectory);
	writeIndexHtml(s, sTime, na);
	//subgroups
	s=add+SUBGROUPS_TIMETABLE_DAYS_HORIZONTAL_FILENAME_HTML;
	s.prepend(outputDirectory);
	TimetableExport::writeSubgroupsTimetableDaysHorizontalHtml(s, sTime, na);
	s=add+SUBGROUPS_TIMETABLE_DAYS_VERTICAL_FILENAME_HTML;
	s.prepend(outputDirectory);
	TimetableExport::writeSubgroupsTimetableDaysVerticalHtml(s, sTime, na);
	if(!DIVIDE_HTML_TIMETABLES_WITH_TIME_AXIS_BY_DAYS){
		s=add+SUBGROUPS_TIMETABLE_TIME_HORIZONTAL_FILENAME_HTML;
		s.prepend(outputDirectory);
		TimetableExport::writeSubgroupsTimetableTimeHorizontalHtml(s, sTime, na);
		s=add+SUBGROUPS_TIMETABLE_TIME_VERTICAL_FILENAME_HTML;
		s.prepend(outputDirectory);
		TimetableExport::writeSubgroupsTimetableTimeVerticalHtml(s, sTime, na);
	} else {
		s=add+SUBGROUPS_TIMETABLE_TIME_HORIZONTAL_FILENAME_HTML;
		s.prepend(outputDirectory);
		TimetableExport::writeSubgroupsTimetableTimeHorizontalDailyHtml(s, sTime, na);
		s=add+SUBGROUPS_TIMETABLE_TIME_VERTICAL_FILENAME_HTML;
		s.prepend(outputDirectory);
		TimetableExport::writeSubgroupsTimetableTimeVerticalDailyHtml(s, sTime, na);
	}
	//groups
	s=add+GROUPS_TIMETABLE_DAYS_HORIZONTAL_FILENAME_HTML;
	s.prepend(outputDirectory);
	TimetableExport::writeGroupsTimetableDaysHorizontalHtml(s, sTime, na);
	s=add+GROUPS_TIMETABLE_DAYS_VERTICAL_FILENAME_HTML;
	s.prepend(outputDirectory);
	TimetableExport::writeGroupsTimetableDaysVerticalHtml(s, sTime, na);
	if(!DIVIDE_HTML_TIMETABLES_WITH_TIME_AXIS_BY_DAYS){
		s=add+GROUPS_TIMETABLE_TIME_HORIZONTAL_FILENAME_HTML;
		s.prepend(outputDirectory);
		TimetableExport::writeGroupsTimetableTimeHorizontalHtml(s, sTime, na);
		s=add+GROUPS_TIMETABLE_TIME_VERTICAL_FILENAME_HTML;
		s.prepend(outputDirectory);
		TimetableExport::writeGroupsTimetableTimeVerticalHtml(s, sTime, na);
	} else {
		s=add+GROUPS_TIMETABLE_TIME_HORIZONTAL_FILENAME_HTML;
		s.prepend(outputDirectory);
		TimetableExport::writeGroupsTimetableTimeHorizontalDailyHtml(s, sTime, na);
		s=add+GROUPS_TIMETABLE_TIME_VERTICAL_FILENAME_HTML;
		s.prepend(outputDirectory);
		TimetableExport::writeGroupsTimetableTimeVerticalDailyHtml(s, sTime, na);
	}
	//years
	s=add+YEARS_TIMETABLE_DAYS_HORIZONTAL_FILENAME_HTML;
	s.prepend(outputDirectory);
	TimetableExport::writeYearsTimetableDaysHorizontalHtml(s, sTime, na);
	s=add+YEARS_TIMETABLE_DAYS_VERTICAL_FILENAME_HTML;
	s.prepend(outputDirectory);
	TimetableExport::writeYearsTimetableDaysVerticalHtml(s, sTime, na);
	if(!DIVIDE_HTML_TIMETABLES_WITH_TIME_AXIS_BY_DAYS){
		s=add+YEARS_TIMETABLE_TIME_HORIZONTAL_FILENAME_HTML;
		s.prepend(outputDirectory);
		TimetableExport::writeYearsTimetableTimeHorizontalHtml(s, sTime, na);
		s=add+YEARS_TIMETABLE_TIME_VERTICAL_FILENAME_HTML;
		s.prepend(outputDirectory);
		TimetableExport::writeYearsTimetableTimeVerticalHtml(s, sTime, na);
	} else {
		s=add+YEARS_TIMETABLE_TIME_HORIZONTAL_FILENAME_HTML;
		s.prepend(outputDirectory);
		TimetableExport::writeYearsTimetableTimeHorizontalDailyHtml(s, sTime, na);
		s=add+YEARS_TIMETABLE_TIME_VERTICAL_FILENAME_HTML;
		s.prepend(outputDirectory);
		TimetableExport::writeYearsTimetableTimeVerticalDailyHtml(s, sTime, na);
	}
	//teachers
	s=add+TEACHERS_TIMETABLE_DAYS_HORIZONTAL_FILENAME_HTML;
	s.prepend(outputDirectory);
	TimetableExport::writeTeachersTimetableDaysHorizontalHtml(s, sTime, na);
	s=add+TEACHERS_TIMETABLE_DAYS_VERTICAL_FILENAME_HTML;
	s.prepend(outputDirectory);
	TimetableExport::writeTeachersTimetableDaysVerticalHtml(s, sTime, na);
	if(!DIVIDE_HTML_TIMETABLES_WITH_TIME_AXIS_BY_DAYS){
		s=add+TEACHERS_TIMETABLE_TIME_HORIZONTAL_FILENAME_HTML;
		s.prepend(outputDirectory);
		TimetableExport::writeTeachersTimetableTimeHorizontalHtml(s, sTime, na);
		s=add+TEACHERS_TIMETABLE_TIME_VERTICAL_FILENAME_HTML;
		s.prepend(outputDirectory);
		TimetableExport::writeTeachersTimetableTimeVerticalHtml(s, sTime, na);
	} else {
		s=add+TEACHERS_TIMETABLE_TIME_HORIZONTAL_FILENAME_HTML;
		s.prepend(outputDirectory);
		TimetableExport::writeTeachersTimetableTimeHorizontalDailyHtml(s, sTime, na);
		s=add+TEACHERS_TIMETABLE_TIME_VERTICAL_FILENAME_HTML;
		s.prepend(outputDirectory);
		TimetableExport::writeTeachersTimetableTimeVerticalDailyHtml(s, sTime, na);
	}
	//rooms
	s=add+ROOMS_TIMETABLE_DAYS_HORIZONTAL_FILENAME_HTML;
	s.prepend(outputDirectory);
	TimetableExport::writeRoomsTimetableDaysHorizontalHtml(s, sTime, na);
	s=add+ROOMS_TIMETABLE_DAYS_VERTICAL_FILENAME_HTML;
	s.prepend(outputDirectory);
	TimetableExport::writeRoomsTimetableDaysVerticalHtml(s, sTime, na);
	if(!DIVIDE_HTML_TIMETABLES_WITH_TIME_AXIS_BY_DAYS){
		s=add+ROOMS_TIMETABLE_TIME_HORIZONTAL_FILENAME_HTML;
		s.prepend(outputDirectory);
		TimetableExport::writeRoomsTimetableTimeHorizontalHtml(s, sTime, na);
		s=add+ROOMS_TIMETABLE_TIME_VERTICAL_FILENAME_HTML;
		s.prepend(outputDirectory);
		TimetableExport::writeRoomsTimetableTimeVerticalHtml(s, sTime, na);
	} else {
		s=add+ROOMS_TIMETABLE_TIME_HORIZONTAL_FILENAME_HTML;
		s.prepend(outputDirectory);
		TimetableExport::writeRoomsTimetableTimeHorizontalDailyHtml(s, sTime, na);
		s=add+ROOMS_TIMETABLE_TIME_VERTICAL_FILENAME_HTML;
		s.prepend(outputDirectory);
		TimetableExport::writeRoomsTimetableTimeVerticalDailyHtml(s, sTime, na);
	}
	//subjects
	s=add+SUBJECTS_TIMETABLE_DAYS_HORIZONTAL_FILENAME_HTML;
	s.prepend(outputDirectory);
	TimetableExport::writeSubjectsTimetableDaysHorizontalHtml(s, sTime, na);
	s=add+SUBJECTS_TIMETABLE_DAYS_VERTICAL_FILENAME_HTML;
	s.prepend(outputDirectory);
	TimetableExport::writeSubjectsTimetableDaysVerticalHtml(s, sTime, na);
	if(!DIVIDE_HTML_TIMETABLES_WITH_TIME_AXIS_BY_DAYS){
		s=add+SUBJECTS_TIMETABLE_TIME_HORIZONTAL_FILENAME_HTML;
		s.prepend(outputDirectory);
		TimetableExport::writeSubjectsTimetableTimeHorizontalHtml(s, sTime, na);
		s=add+SUBJECTS_TIMETABLE_TIME_VERTICAL_FILENAME_HTML;
		s.prepend(outputDirectory);
		TimetableExport::writeSubjectsTimetableTimeVerticalHtml(s, sTime, na);
	} else {
		s=add+SUBJECTS_TIMETABLE_TIME_HORIZONTAL_FILENAME_HTML;
		s.prepend(outputDirectory);
		TimetableExport::writeSubjectsTimetableTimeHorizontalDailyHtml(s, sTime, na);
		s=add+SUBJECTS_TIMETABLE_TIME_VERTICAL_FILENAME_HTML;
		s.prepend(outputDirectory);
		TimetableExport::writeSubjectsTimetableTimeVerticalDailyHtml(s, sTime, na);
	}
	//all activities
	s=add+ALL_ACTIVITIES_TIMETABLE_DAYS_HORIZONTAL_FILENAME_HTML;
	s.prepend(outputDirectory);
	TimetableExport::writeAllActivitiesTimetableDaysHorizontalHtml(s, sTime, na);
	s=add+ALL_ACTIVITIES_TIMETABLE_DAYS_VERTICAL_FILENAME_HTML;
	s.prepend(outputDirectory);
	TimetableExport::writeAllActivitiesTimetableDaysVerticalHtml(s, sTime, na);
	if(!DIVIDE_HTML_TIMETABLES_WITH_TIME_AXIS_BY_DAYS){
		s=add+ALL_ACTIVITIES_TIMETABLE_TIME_HORIZONTAL_FILENAME_HTML;
		s.prepend(outputDirectory);
		TimetableExport::writeAllActivitiesTimetableTimeHorizontalHtml(s, sTime, na);
		s=add+ALL_ACTIVITIES_TIMETABLE_TIME_VERTICAL_FILENAME_HTML;
		s.prepend(outputDirectory);
		TimetableExport::writeAllActivitiesTimetableTimeVerticalHtml(s, sTime, na);
	} else {
		s=add+ALL_ACTIVITIES_TIMETABLE_TIME_HORIZONTAL_FILENAME_HTML;
		s.prepend(outputDirectory);
		TimetableExport::writeAllActivitiesTimetableTimeHorizontalDailyHtml(s, sTime, na);
		s=add+ALL_ACTIVITIES_TIMETABLE_TIME_VERTICAL_FILENAME_HTML;
		s.prepend(outputDirectory);
		TimetableExport::writeAllActivitiesTimetableTimeVerticalDailyHtml(s, sTime, na);
	}
	//teachers free periods
	s=add+TEACHERS_FREE_PERIODS_TIMETABLE_DAYS_HORIZONTAL_FILENAME_HTML;
	s.prepend(outputDirectory);
	TimetableExport::writeTeachersFreePeriodsTimetableDaysHorizontalHtml(s, sTime, na);
	s=add+TEACHERS_FREE_PERIODS_TIMETABLE_DAYS_VERTICAL_FILENAME_HTML;
	s.prepend(outputDirectory);
	TimetableExport::writeTeachersFreePeriodsTimetableDaysVerticalHtml(s, sTime, na);

	hashSubjectIDsTimetable.clear();
	hashActivityTagIDsTimetable.clear();
	hashStudentIDsTimetable.clear();
	hashTeacherIDsTimetable.clear();
	hashRoomIDsTimetable.clear();
	hashDayIDsTimetable.clear();
}








void TimetableExport::writeRandomSeedCommandLine(const QString& outputDirectory){ //outputDirectory contains trailing FILE_SEP
	QString add=INPUT_FILENAME_XML.right(INPUT_FILENAME_XML.length()-INPUT_FILENAME_XML.findRev(FILE_SEP)-1);
	if(add.right(4)==".fet")
		add=add.left(add.length()-4);
	else if(INPUT_FILENAME_XML!="")
		cout<<"Minor problem - input file does not end in .fet extension - might be a problem when saving the timetables"<<" (file:"<<__FILE__<<", line:"<<__LINE__<<")"<<endl;

	if(add!="")
		add.append("_");

	QString s=add+RANDOM_SEED_FILENAME;
	s.prepend(outputDirectory);
	
	writeRandomSeedFile(s);
}






//modified by Volker Dirr (timetabling.de) from old code by Liviu Lalescu
void TimetableExport::writeConflictsTxt(const QString& filename, const QString& saveTime, int placedActivities){
	assert(gt.rules.initialized && gt.rules.internalStructureComputed);
	//assert(gt.timePopulation.initialized);
	assert(students_schedule_ready && teachers_schedule_ready && rooms_schedule_ready);

	QFile file(filename);
	if(!file.open(QIODevice::WriteOnly)){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Cannot open file %1 for writing. Please check your disk's free space. Saving of %1 aborted.").arg(filename));
		return;
		assert(0);
	}
	QTextStream tos(&file);
	tos.setCodec("UTF-8");
	tos.setGenerateByteOrderMark(true);
	
	if(placedActivities==gt.rules.nInternalActivities){
		QString tt=INPUT_FILENAME_XML.right(INPUT_FILENAME_XML.length()-INPUT_FILENAME_XML.findRev(FILE_SEP)-1);
		if(INPUT_FILENAME_XML=="")
			tt=tr("unnamed");
		tos<<TimetableExport::tr("Soft conflicts of %1", "%1 is the file name").arg(tt);
		tos<<"\n";
		tos<<TimetableExport::tr("Generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"\n\n";

		tos<<TimetableExport::tr("Total soft conflicts:")<<QString(" ")<<best_solution.conflictsTotal<<endl<<endl;
		tos<<TimetableExport::tr("Soft conflicts list (in decreasing order):")<<endl<<endl;
		foreach(QString t, best_solution.conflictsDescriptionList)
			tos<<t<<endl;
		tos<<endl<<TimetableExport::tr("End of file.")<<"\n";
	}
	else{
		QString tt=INPUT_FILENAME_XML.right(INPUT_FILENAME_XML.length()-INPUT_FILENAME_XML.findRev(FILE_SEP)-1);
		if(INPUT_FILENAME_XML=="")
			tt=tr("unnamed");
		tos<<TimetableExport::tr("Conflicts of %1").arg(tt);
		tos<<"\n";
		tos<<TimetableExport::tr("Warning! Only %1 out of %2 activities placed!").arg(placedActivities).arg(gt.rules.nInternalActivities)<<"\n";
		tos<<TimetableExport::tr("Generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"\n\n";

		tos<<TimetableExport::tr("Total conflicts:")<<QString(" ")<<best_solution.conflictsTotal<<endl<<endl;
		tos<<TimetableExport::tr("Conflicts list (in decreasing order):")<<endl<<endl;
		foreach(QString t, best_solution.conflictsDescriptionList)
			tos<<t<<endl;
		tos<<endl<<TimetableExport::tr("End of file.")<<"\n";
	}
	
	if(file.error()>0){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Writing %1 gave error code %2, which means saving is compromised. Please check your disk's free space.").arg(filename).arg(file.error()));
	}
	file.close();
}


void TimetableExport::writeSubgroupsTimetableXml(const QString& xmlfilename){
	assert(gt.rules.initialized && gt.rules.internalStructureComputed);
	//assert(gt.timePopulation.initialized);
	assert(students_schedule_ready && teachers_schedule_ready && rooms_schedule_ready);

	//Now we print the results to an XML file
	QFile file(xmlfilename);
	if(!file.open(QIODevice::WriteOnly)){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Cannot open file %1 for writing. Please check your disk's free space. Saving of %1 aborted.").arg(xmlfilename));
		return;
		assert(0);
	}
	QTextStream tos(&file);
	tos.setCodec("UTF-8");
	tos.setGenerateByteOrderMark(true);
	tos<<"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n\n";
	tos<<"<"<<protect(STUDENTS_TIMETABLE_TAG)<<">\n";

	for(int subgroup=0; subgroup<gt.rules.nInternalSubgroups; subgroup++){
		tos<<"\n";
		tos<< "  <Subgroup name=\"";
		QString subgroup_name = gt.rules.internalSubgroupsList[subgroup]->name;
		tos<< protect(subgroup_name) << "\">\n";

		for(int k=0; k<gt.rules.nDaysPerWeek; k++){
			tos<<"   <Day name=\""<<protect(gt.rules.daysOfTheWeek[k])<<"\">\n";
			for(int j=0; j<gt.rules.nHoursPerDay; j++){
				tos << "    <Hour name=\"" << protect(gt.rules.hoursOfTheDay[j]) << "\">\n";
				tos<<"     ";
				int ai=students_timetable_weekly[subgroup][k][j]; //activity index
				if(ai!=UNALLOCATED_ACTIVITY){
					//Activity* act=gt.rules.activitiesList.at(ai);
					Activity* act=&gt.rules.internalActivitiesList[ai];
					for(QStringList::Iterator it=act->teachersNames.begin(); it!=act->teachersNames.end(); it++)
						tos<<" <Teacher name=\""<<protect(*it)<<"\"></Teacher>";
					tos<<"<Subject name=\""<<protect(act->subjectName)<<"\"></Subject>";
					foreach(QString atn, act->activityTagsNames)
						tos<<"<Activity_Tag name=\""<<protect(atn)<<"\"></Activity_Tag>";

					int r=best_solution.rooms[ai];
					if(r!=UNALLOCATED_SPACE && r!=UNSPECIFIED_ROOM){
						tos<<"<Room name=\""<<protect(gt.rules.internalRoomsList[r]->name)<<"\"></Room>";
					}
				}
				tos<<"\n";
				tos << "    </Hour>\n";
			}
			tos<<"   </Day>\n";
		}
		tos<<"  </Subgroup>\n";
	}

	tos<<"\n";
	tos << "</" << protect(STUDENTS_TIMETABLE_TAG) << ">\n";

	if(file.error()>0){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Writing %1 gave error code %2, which means saving is compromised. Please check your disk's free space.").arg(xmlfilename).arg(file.error()));
	}
	file.close();
}


void TimetableExport::writeTeachersTimetableXml(const QString& xmlfilename){
	assert(gt.rules.initialized && gt.rules.internalStructureComputed);
	//assert(gt.timePopulation.initialized);
	assert(students_schedule_ready && teachers_schedule_ready && rooms_schedule_ready);

	//Writing the timetable in xml format
	QFile file(xmlfilename);
	if(!file.open(QIODevice::WriteOnly)){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Cannot open file %1 for writing. Please check your disk's free space. Saving of %1 aborted.").arg(xmlfilename));
		return;
		assert(0);
	}
	QTextStream tos(&file);
	tos.setCodec("UTF-8");
	tos.setGenerateByteOrderMark(true);
	tos<<"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n\n";
	tos << "<" << protect(TEACHERS_TIMETABLE_TAG) << ">\n";

	for(int i=0; i<gt.rules.nInternalTeachers; i++){
		tos<<"\n";
		tos << "  <Teacher name=\"" << protect(gt.rules.internalTeachersList[i]->name) << "\">\n";
		for(int k=0; k<gt.rules.nDaysPerWeek; k++){
			tos << "   <Day name=\"" << protect(gt.rules.daysOfTheWeek[k]) << "\">\n";
			for(int j=0; j<gt.rules.nHoursPerDay; j++){
				tos << "    <Hour name=\"" << protect(gt.rules.hoursOfTheDay[j]) << "\">\n";

				tos<<"     ";
				int ai=teachers_timetable_weekly[i][k][j]; //activity index
				//Activity* act=gt.rules.activitiesList.at(ai);
				if(ai!=UNALLOCATED_ACTIVITY){
					Activity* act=&gt.rules.internalActivitiesList[ai];
					tos<<"<Subject name=\""<<protect(act->subjectName)<<"\"></Subject>";
					foreach(QString atn, act->activityTagsNames)
						tos<<"<Activity_Tag name=\""<<protect(atn)<<"\"></Activity_Tag>";
					for(QStringList::Iterator it=act->studentsNames.begin(); it!=act->studentsNames.end(); it++)
						tos << "<Students name=\"" << protect(*it) << "\"></Students>";

					int r=best_solution.rooms[ai];
					if(r!=UNALLOCATED_SPACE && r!=UNSPECIFIED_ROOM){
						tos<<"<Room name=\""<<protect(gt.rules.internalRoomsList[r]->name)<<"\"></Room>";
					}
				}
				tos<<"\n";
				tos << "    </Hour>\n";
			}
			tos << "   </Day>\n";
		}
		tos<<"  </Teacher>\n";
	}

	tos<<"\n";
	tos << "</" << protect(TEACHERS_TIMETABLE_TAG) << ">\n";

	if(file.error()>0){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Writing %1 gave error code %2, which means saving is compromised. Please check your disk's free space.").arg(xmlfilename).arg(file.error()));
	}
	file.close();
}


void TimetableExport::writeActivitiesTimetableXml(const QString& xmlfilename){
	assert(gt.rules.initialized && gt.rules.internalStructureComputed);
	//assert(gt.timePopulation.initialized);
	assert(students_schedule_ready && teachers_schedule_ready && rooms_schedule_ready);

	//Writing the timetable in xml format
	QFile file(xmlfilename);
	if(!file.open(QIODevice::WriteOnly)){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Cannot open file %1 for writing. Please check your disk's free space. Saving of %1 aborted.").arg(xmlfilename));
		return;
		assert(0);
	}
	QTextStream tos(&file);
	tos.setCodec("UTF-8");
	tos.setGenerateByteOrderMark(true);
	tos<<"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n\n";
	tos << "<" << protect(ACTIVITIES_TIMETABLE_TAG) << ">\n";
	
	tos<<"\n";
	
	for(int i=0; i<gt.rules.nInternalActivities; i++){
		tos<<"<Activity>"<<endl;
		
		tos<<"	<Id>"<<gt.rules.internalActivitiesList[i].id<<"</Id>"<<endl;
		
		QString day="";
		if(best_solution.times[i]!=UNALLOCATED_TIME){
			int d=best_solution.times[i]%gt.rules.nDaysPerWeek;
			day=gt.rules.daysOfTheWeek[d];
		}
		tos<<"	<Day>"<<protect(day)<<"</Day>"<<endl;
		
		QString hour="";
		if(best_solution.times[i]!=UNALLOCATED_TIME){
			int h=best_solution.times[i]/gt.rules.nDaysPerWeek;
			hour=gt.rules.hoursOfTheDay[h];
		}
		tos<<"	<Hour>"<<protect(hour)<<"</Hour>"<<endl;
		
		QString room="";
		if(best_solution.rooms[i]!=UNALLOCATED_SPACE && best_solution.rooms[i]!=UNSPECIFIED_ROOM){
			int r=best_solution.rooms[i];
			room=gt.rules.internalRoomsList[r]->name;
		}
		tos<<"	<Room>"<<protect(room)<<"</Room>"<<endl;
		
		tos<<"</Activity>"<<endl;
	}

	tos<<"\n";
	tos << "</" << protect(ACTIVITIES_TIMETABLE_TAG) << ">\n";

	if(file.error()>0){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Writing %1 gave error code %2, which means saving is compromised. Please check your disk's free space.").arg(xmlfilename).arg(file.error()));
	}
	file.close();
}


// writing the index html file by Volker Dirr.
void TimetableExport::writeIndexHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities){
	assert(gt.rules.initialized && gt.rules.internalStructureComputed);
	//assert(gt.timePopulation.initialized);
	assert(students_schedule_ready && teachers_schedule_ready && rooms_schedule_ready);

	//Now we print the results to an HTML file
	QFile file(htmlfilename);
	if(!file.open(QIODevice::WriteOnly)){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Cannot open file %1 for writing. Please check your disk's free space. Saving of %1 aborted.").arg(htmlfilename));
		return;
		assert(0);
	}
	QTextStream tos(&file);
	tos.setCodec("UTF-8");
	tos.setGenerateByteOrderMark(true);

	tos<<writeHead(false, placedActivities, true);

	QString bar;
	QString s2="";
	if(INPUT_FILENAME_XML=="")
		bar="";
	else{
		bar="_";
		s2=INPUT_FILENAME_XML.right(INPUT_FILENAME_XML.length()-INPUT_FILENAME_XML.findRev(FILE_SEP)-1);

		if(s2.right(4)==".fet")
			s2=s2.left(s2.length()-4);
		//else if(INPUT_FILENAME_XML!="")
		//	cout<<"Minor problem - input file does not end in .fet extension - might be a problem when saving the timetables"<<" (file:"<<__FILE__<<", line:"<<__LINE__<<")"<<endl;
	}
	tos<<"    <p>\n";

	tos<<"      <a href=\""<<s2+bar+CONFLICTS_FILENAME<<"\">"<<tr("View the soft conflicts list.")<<"</a><br />\n";
//	tos<<"    </p>\n";
//	tos<<"    <p>\n";

	QString tmp1="<a href=\""+s2+bar+SUBGROUPS_TIMETABLE_FILENAME_XML+"\">"+tr("subgroups")+"</a>";
	QString tmp2="<a href=\""+s2+bar+TEACHERS_TIMETABLE_FILENAME_XML+"\">"+tr("teachers")+"</a>";
	QString tmp3="<a href=\""+s2+bar+ACTIVITIES_TIMETABLE_FILENAME_XML+"\">"+tr("activities")+"</a>";
	QString tmp4=TimetableExport::tr("View XML: %1, %2, %3.", "%1, %2 and %3 are three files in XML format, subgroups, teachers and activities timetables. The user can click on one file to view it").arg(tmp1).arg(tmp2).arg(tmp3);
	tos<<"      "<<tmp4<<"\n";

	tos<<"    </p>\n\n";

	tos<<"    <table border=\"1\">\n";

	tos<<"      <caption>"<<protect2(gt.rules.institutionName)<<"</caption>\n";

	tos<<"      <thead>\n        <tr><td rowspan=\"2\"></td><th colspan=\"4\">"+tr("Timetables")+"</th></tr>\n";
	tos<<"        <tr>\n          <!-- span -->\n";
	tos<<"          <th>"+tr("Days Horizontal")+"</th><th>"+tr("Days Vertical")+"</th><th>"+tr("Time Horizontal")+"</th><th>"+tr("Time Vertical")+"</th>";
	tos<<"        </tr>\n";
	tos<<"      </thead>\n";
	tos<<"      <tbody>\n";

	tos<<"        <tr>\n";
	tos<<"          <th>"+tr("Subgroups")+"</th>\n";
	tos<<"          <td><a href=\""<<s2+bar+SUBGROUPS_TIMETABLE_DAYS_HORIZONTAL_FILENAME_HTML<<"\">"+tr("view")+"</a></td>\n";
	tos<<"          <td><a href=\""<<s2+bar+SUBGROUPS_TIMETABLE_DAYS_VERTICAL_FILENAME_HTML<<"\">"+tr("view")+"</a></td>\n";
	tos<<"          <td><a href=\""<<s2+bar+SUBGROUPS_TIMETABLE_TIME_HORIZONTAL_FILENAME_HTML<<"\">"+tr("view")+"</a></td>\n";
	tos<<"          <td><a href=\""<<s2+bar+SUBGROUPS_TIMETABLE_TIME_VERTICAL_FILENAME_HTML<<"\">"+tr("view")+"</a></td>\n";
	tos<<"        </tr>\n";
	tos<<"        <tr>\n";
	tos<<"          <th>"+tr("Groups")+"</th>\n";
	tos<<"          <td><a href=\""<<s2+bar+GROUPS_TIMETABLE_DAYS_HORIZONTAL_FILENAME_HTML<<"\">"+tr("view")+"</a></td>\n";
	tos<<"          <td><a href=\""<<s2+bar+GROUPS_TIMETABLE_DAYS_VERTICAL_FILENAME_HTML<<"\">"+tr("view")+"</a></td>\n";
	tos<<"          <td><a href=\""<<s2+bar+GROUPS_TIMETABLE_TIME_HORIZONTAL_FILENAME_HTML<<"\">"+tr("view")+"</a></td>\n";
	tos<<"          <td><a href=\""<<s2+bar+GROUPS_TIMETABLE_TIME_VERTICAL_FILENAME_HTML<<"\">"+tr("view")+"</a></td>\n";
	tos<<"        </tr>\n";
	tos<<"        <tr>\n";
	tos<<"          <th>"+tr("Years")+"</th>\n";
	tos<<"          <td><a href=\""<<s2+bar+YEARS_TIMETABLE_DAYS_HORIZONTAL_FILENAME_HTML<<"\">"+tr("view")+"</a></td>\n";
	tos<<"          <td><a href=\""<<s2+bar+YEARS_TIMETABLE_DAYS_VERTICAL_FILENAME_HTML<<"\">"+tr("view")+"</a></td>\n";
	tos<<"          <td><a href=\""<<s2+bar+YEARS_TIMETABLE_TIME_HORIZONTAL_FILENAME_HTML<<"\">"+tr("view")+"</a></td>\n";
	tos<<"          <td><a href=\""<<s2+bar+YEARS_TIMETABLE_TIME_VERTICAL_FILENAME_HTML<<"\">"+tr("view")+"</a></td>\n";
	tos<<"        </tr>\n";
	tos<<"        <tr>\n";
	tos<<"          <th>"+tr("Teachers")+"</th>\n";
	tos<<"          <td><a href=\""<<s2+bar+TEACHERS_TIMETABLE_DAYS_HORIZONTAL_FILENAME_HTML<<"\">"+tr("view")+"</a></td>\n";
	tos<<"          <td><a href=\""<<s2+bar+TEACHERS_TIMETABLE_DAYS_VERTICAL_FILENAME_HTML<<"\">"+tr("view")+"</a></td>\n";
	tos<<"          <td><a href=\""<<s2+bar+TEACHERS_TIMETABLE_TIME_HORIZONTAL_FILENAME_HTML<<"\">"+tr("view")+"</a></td>\n";
	tos<<"          <td><a href=\""<<s2+bar+TEACHERS_TIMETABLE_TIME_VERTICAL_FILENAME_HTML<<"\">"+tr("view")+"</a></td>\n";
	tos<<"        </tr>\n";
	tos<<"        <tr>\n";
	tos<<"          <th>"+tr("Teachers Free Periods")+"</th>\n";
	tos<<"          <td><a href=\""<<s2+bar+TEACHERS_FREE_PERIODS_TIMETABLE_DAYS_HORIZONTAL_FILENAME_HTML<<"\">"+tr("view")+"</a></td>\n";
	tos<<"          <td><a href=\""<<s2+bar+TEACHERS_FREE_PERIODS_TIMETABLE_DAYS_VERTICAL_FILENAME_HTML<<"\">"+tr("view")+"</a></td>\n";
	tos<<"          <td>"<<protect2(STRING_EMPTY_SLOT)<<"</td>\n";
	tos<<"          <td>"<<protect2(STRING_EMPTY_SLOT)<<"</td>\n";
	tos<<"        </tr>\n";
	tos<<"        <tr>\n";
	tos<<"          <th>"+tr("Rooms")+"</th>\n";
	tos<<"          <td><a href=\""<<s2+bar+ROOMS_TIMETABLE_DAYS_HORIZONTAL_FILENAME_HTML<<"\">"+tr("view")+"</a></td>\n";
	tos<<"          <td><a href=\""<<s2+bar+ROOMS_TIMETABLE_DAYS_VERTICAL_FILENAME_HTML<<"\">"+tr("view")+"</a></td>\n";
	tos<<"          <td><a href=\""<<s2+bar+ROOMS_TIMETABLE_TIME_HORIZONTAL_FILENAME_HTML<<"\">"+tr("view")+"</a></td>\n";
	tos<<"          <td><a href=\""<<s2+bar+ROOMS_TIMETABLE_TIME_VERTICAL_FILENAME_HTML<<"\">"+tr("view")+"</a></td>\n";
	tos<<"        </tr>\n";
	tos<<"        <tr>\n";
	tos<<"          <th>"+tr("Subjects")+"</th>\n";
	tos<<"          <td><a href=\""<<s2+bar+SUBJECTS_TIMETABLE_DAYS_HORIZONTAL_FILENAME_HTML<<"\">"+tr("view")+"</a></td>\n";
	tos<<"          <td><a href=\""<<s2+bar+SUBJECTS_TIMETABLE_DAYS_VERTICAL_FILENAME_HTML<<"\">"+tr("view")+"</a></td>\n";
	tos<<"          <td><a href=\""<<s2+bar+SUBJECTS_TIMETABLE_TIME_HORIZONTAL_FILENAME_HTML<<"\">"+tr("view")+"</a></td>\n";
	tos<<"          <td><a href=\""<<s2+bar+SUBJECTS_TIMETABLE_TIME_VERTICAL_FILENAME_HTML<<"\">"+tr("view")+"</a></td>\n";
	tos<<"        </tr>\n";
	tos<<"        <tr>\n";
	tos<<"          <th>"+tr("Activities")+"</th>\n";
	tos<<"          <td><a href=\""<<s2+bar+ALL_ACTIVITIES_TIMETABLE_DAYS_HORIZONTAL_FILENAME_HTML<<"\">"+tr("view")+"</a></td>\n";
	tos<<"          <td><a href=\""<<s2+bar+ALL_ACTIVITIES_TIMETABLE_DAYS_VERTICAL_FILENAME_HTML<<"\">"+tr("view")+"</a></td>\n";
	tos<<"          <td><a href=\""<<s2+bar+ALL_ACTIVITIES_TIMETABLE_TIME_HORIZONTAL_FILENAME_HTML<<"\">"+tr("view")+"</a></td>\n";
	tos<<"          <td><a href=\""<<s2+bar+ALL_ACTIVITIES_TIMETABLE_TIME_VERTICAL_FILENAME_HTML<<"\">"+tr("view")+"</a></td>\n";
	tos<<"        </tr>\n";
	//workaround begin. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
	tos<<"      <tr class=\"foot\"><td></td><td colspan=\"4\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr>\n";
	//workaround end.
	tos<<"      </tbody>\n";
	tos<<"    </table>\n";

	tos<<"  </body>\n</html>\n";

	if(file.error()>0){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Writing %1 gave error code %2, which means saving is compromised. Please check your disk's free space.").arg(htmlfilename).arg(file.error()));
	}
	file.close();
}

// writing the stylesheet in css format to a file by Volker Dirr.
void TimetableExport::writeStylesheetCss(const QString& htmlfilename, const QString& saveTime, int placedActivities){
	assert(gt.rules.initialized && gt.rules.internalStructureComputed);
	//assert(gt.timePopulation.initialized);
	assert(students_schedule_ready && teachers_schedule_ready && rooms_schedule_ready);

	//get used students	//TODO: do it the same way in statistics.cpp
	QSet<QString> usedStudents;
	for(int i=0; i<gt.rules.nInternalActivities; i++){
		foreach(QString st, gt.rules.internalActivitiesList[i].studentsNames){
			if(!usedStudents.contains(st))
				usedStudents<<st;
		}
	}

	//Now we print the results to an CSS file
	QFile file(htmlfilename);
	if(!file.open(QIODevice::WriteOnly)){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Cannot open file %1 for writing. Please check your disk's free space. Saving of %1 aborted.").arg(htmlfilename));
		return;
		assert(0);
	}
	QTextStream tos(&file);
	tos.setCodec("UTF-8");
	tos.setGenerateByteOrderMark(true);
	
	tos<<"@charset \"utf-8\";"<<"\n\n";

	QString tt=INPUT_FILENAME_XML.right(INPUT_FILENAME_XML.length()-INPUT_FILENAME_XML.findRev(FILE_SEP)-1);
	if(INPUT_FILENAME_XML=="")
		tt=tr("unnamed");
	tos<<"/* "<<TimetableExport::tr("CSS Stylesheet of %1", "%1 is the file name").arg(tt);
	tos<<"\n";
	if(placedActivities!=gt.rules.nInternalActivities)
		tos<<"   "<<TimetableExport::tr("Warning! Only %1 out of %2 activities placed!").arg(placedActivities).arg(gt.rules.nInternalActivities)<<"\n";
	tos<<"   "<<TimetableExport::tr("Stylesheet generated with FET %1 on %2", "%1 is FET version, %2 is date and time").arg(FET_VERSION).arg(saveTime)<<" */\n\n";

	tos<<"/* "<<TimetableExport::tr("To do a page-break only after every second timetable, delete \"page-break-before: always;\" in \"table.even_table\".",
		"Please keep fields in quotes as they are, untranslated.")<<" */\n";
	tos<<"/* "<<TimetableExport::tr("To hide an element just write the following phrase into the element: %1 (without quotes).",
		"%1 is a short phrase beginning and ending with quotes, and we want the user to be able to add it, but without quotes").arg("\"display:none;\"")<<" */\n\n";
	tos<<"p.back {\n  margin-top: 4ex;\n  margin-bottom: 5ex;\n}\n\n";
	tos<<"table {\n  text-align: center;\n}\n\n";
	tos<<"table.odd_table {\n  page-break-before: always;\n}\n\n";
	tos<<"table.even_table {\n  page-break-before: always;\n}\n\n";
	tos<<"table.detailed {\n  margin-left:auto; margin-right:auto;\n  text-align: center;\n  border: 0px;\n  border-spacing: 0;\n  border-collapse: collapse;\n}\n\n";
	tos<<"caption {\n\n}\n\n";
	tos<<"thead {\n\n}\n\n";
	
	//workaround begin. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
	tos<<"/* "<<TimetableExport::tr("OpenOffice.org imports the \"tfoot\" incorrectly. So we use \"tr.foot\" instead of \"tfoot\".",
		"Please keep tfoot and tr.foot untranslated, as they are in the original English phrase")<<"\n";
	tos<<"   "<<TimetableExport::tr("See the link %1 for more details.").arg("http://www.openoffice.org/issues/show_bug.cgi?id=82600")<<"\n";

	tos<<"tfoot {\n\n}*/\n\n";
	tos<<"tr.foot {\n\n}\n\n";
	//workaround end
	tos<<"tbody {\n\n}\n\n";
	tos<<"th {\n\n}\n\n";
	tos<<"td {\n\n}\n\n";
	tos<<"td.detailed {\n  border: 1px dashed silver;\n  border-bottom: 0;\n  border-top: 0;\n}\n\n";
	if(TIMETABLE_HTML_LEVEL>=2){
		tos<<"th.xAxis {\n/*width: 8em; */\n}\n\n";
		tos<<"th.yAxis {\n  height: 8ex;\n}\n\n";
	}
	if(TIMETABLE_HTML_LEVEL>=4){ // must be written before LEVEL 3, because LEVEL 3 should have higher priority
		for(int i=0; i<gt.rules.nInternalSubjects; i++){
			tos << "span.s_"<<hashSubjectIDsTimetable.value(gt.rules.internalSubjectsList[i]->name)<<" { /* subject "<<gt.rules.internalSubjectsList[i]->name<<" */\n\n}\n\n";
		}
		for(int i=0; i<gt.rules.nInternalActivityTags; i++){
			tos << "span.at_"<<hashActivityTagIDsTimetable.value(gt.rules.internalActivityTagsList[i]->name)<<" { /* activity tag "<<gt.rules.internalActivityTagsList[i]->name<<" */\n\n}\n\n";
		}
		for(int i=0; i<gt.rules.augmentedYearsList.size(); i++){
			StudentsYear* sty=gt.rules.augmentedYearsList[i];
			if(usedStudents.contains(sty->name))
				tos << "span.ss_"<<hashStudentIDsTimetable.value(sty->name)<<" { /* students set "<<sty->name<<" */\n\n}\n\n";
			for(int j=0; j<sty->groupsList.size(); j++){
				StudentsGroup* stg=sty->groupsList[j];
				if(usedStudents.contains(stg->name))
					tos << "span.ss_"<<hashStudentIDsTimetable.value(stg->name)<<" { /* students set "<<stg->name<<" */\n\n}\n\n";
				for(int k=0; k<stg->subgroupsList.size(); k++){
					StudentsSubgroup* sts=stg->subgroupsList[k];
					if(usedStudents.contains(sts->name))
						tos << "span.ss_"<<hashStudentIDsTimetable.value(sts->name)<<" { /* students set "<<sts->name<<" */\n\n}\n\n";
				}
			}
		}
		for(int i=0; i<gt.rules.nInternalTeachers; i++){
			tos << "span.t_"<<hashTeacherIDsTimetable.value(gt.rules.internalTeachersList[i]->name)<<" { /* teacher "<<gt.rules.internalTeachersList[i]->name<<" */\n\n}\n\n";
		}
		for(int room=0; room<gt.rules.nInternalRooms; room++){
			tos << "span.r_"<<hashRoomIDsTimetable.value(gt.rules.internalRoomsList[room]->name)<<" { /* room "<<gt.rules.internalRoomsList[room]->name<<" */\n\n}\n\n";
		}
	}
	if(TIMETABLE_HTML_LEVEL>=3){
		tos<<"span.subject {\n\n}\n\n";
		tos<<"span.activitytag {\n\n}\n\n";

		tos<<"span.empty {\n  color: gray;\n}\n\n";
		tos<<"td.empty {\n  border-color:silver;\n  border-right-style:none;\n  border-bottom-style:none;\n  border-left-style:dotted;\n  border-top-style:dotted;\n}\n\n";

		tos<<"span.notAvailable {\n  color: gray;\n}\n\n";
		tos<<"td.notAvailable {\n  border-color:silver;\n  border-right-style:none;\n  border-bottom-style:none;\n  border-left-style:dotted;\n  border-top-style:dotted;\n}\n\n";

		tos<<"span.break {\n  color: gray;\n}\n\n";
		tos<<"td.break {\n  border-color:silver;\n  border-right-style:none;\n  border-bottom-style:none;\n  border-left-style:dotted;\n  border-top-style:dotted;\n}\n\n";

		tos<<"td.student, div.student {\n\n}\n\n";
		tos<<"td.teacher, div.teacher {\n\n}\n\n";
		tos<<"td.room, div.room {\n\n}\n\n";
		tos<<"tr.line0 {\n  font-size: smaller;\n}\n\n";
		tos<<"tr.line1, div.line1 {\n\n}\n\n";
		tos<<"tr.line2, div.line2 {\n  font-size: smaller;\n  color: gray;\n}\n\n";
		tos<<"tr.line3, div.line3 {\n  font-size: smaller;\n  color: silver;\n}\n\n";
	}
	if(TIMETABLE_HTML_LEVEL==6){
		tos<<endl<<"/* "<<TimetableExport::tr("Be careful. You might get mutual and ambiguous styles. CSS means that the last definition will be used.")<<" */\n\n";
		for(int i=0; i<gt.rules.nInternalSubjects; i++){
			tos << "td.s_"<<hashSubjectIDsTimetable.value(gt.rules.internalSubjectsList[i]->name)<<" { /* subject "<<gt.rules.internalSubjectsList[i]->name<<" */\n\n}\n\n";
		}
		for(int i=0; i<gt.rules.nInternalActivityTags; i++){
			tos << "td.at_"<<hashActivityTagIDsTimetable.value(gt.rules.internalActivityTagsList[i]->name)<<" { /* activity tag "<<gt.rules.internalActivityTagsList[i]->name<<" */\n\n}\n\n";
		}
		for(int i=0; i<gt.rules.augmentedYearsList.size(); i++){
			StudentsYear* sty=gt.rules.augmentedYearsList[i];
			if(usedStudents.contains(sty->name))
				tos << "td.ss_"<<hashStudentIDsTimetable.value(sty->name)<<" { /* students set "<<sty->name<<" */\n\n}\n\n";
			for(int j=0; j<sty->groupsList.size(); j++){
				StudentsGroup* stg=sty->groupsList[j];
				if(usedStudents.contains(stg->name))
					tos << "td.ss_"<<hashStudentIDsTimetable.value(stg->name)<<" { /* students set "<<stg->name<<" */\n\n}\n\n";
				for(int k=0; k<stg->subgroupsList.size(); k++){
					StudentsSubgroup* sts=stg->subgroupsList[k];
					if(usedStudents.contains(sts->name))
						tos << "td.ss_"<<hashStudentIDsTimetable.value(sts->name)<<" { /* students set "<<sts->name<<" */\n\n}\n\n";
				}
			}
		}
		for(int i=0; i<gt.rules.nInternalTeachers; i++){
			tos << "td.t_"<<hashTeacherIDsTimetable.value(gt.rules.internalTeachersList[i]->name)<<" { /* teacher "<<gt.rules.internalTeachersList[i]->name<<" */\n\n}\n\n";
		}

		//not included yet
		//for(int room=0; room<gt.rules.nInternalRooms; room++){
		//	tos << "span.r_"<<hashRoomIDsTimetable.value(gt.rules.internalRoomsList[room]->name)<<" { /* room "<<gt.rules.internalRoomsList[room]->name<<" */\n\n}\n\n";
		//}
	}
	tos<<endl<<"/* "<<TimetableExport::tr("Style the teachers free periods")<<" */\n\n";
	if(TIMETABLE_HTML_LEVEL>=2){
		tos<<"div.DESCRIPTION {\n  text-align: left;\n  font-size: smaller;\n}\n\n";
	}
	if(TIMETABLE_HTML_LEVEL>=3){
		tos<<"div.TEACHER_HAS_SINGLE_GAP {\n  color: black;\n}\n\n";
		tos<<"div.TEACHER_HAS_BORDER_GAP {\n  color: gray;\n}\n\n";
		tos<<"div.TEACHER_HAS_BIG_GAP {\n  color: silver;\n}\n\n";
		tos<<"div.TEACHER_MUST_COME_EARLIER {\n  color: purple;\n}\n\n";
		tos<<"div.TEACHER_MUST_COME_MUCH_EARLIER {\n  font-size: smaller;\n  color: fuchsia;\n}\n\n";
		tos<<"div.TEACHER_MUST_STAY_LONGER {\n  color: teal;\n}\n\n";
		tos<<"div.TEACHER_MUST_STAY_MUCH_LONGER {\n  font-size: smaller;\n  color: aqua;\n}\n\n";
		tos<<"div.TEACHER_HAS_A_FREE_DAY {\n  font-size: smaller;\n  color: red;\n}\n\n";
		tos<<"div.TEACHER_IS_NOT_AVAILABLE {\n  font-size: smaller;\n  color: olive;\n}\n\n";
	}
	tos<<endl<<"/* "<<TimetableExport::tr("End of file.")<<" */\n";

	if(file.error()>0){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Writing %1 gave error code %2, which means saving is compromised. Please check your disk's free space.").arg(htmlfilename).arg(file.error()));
	}
	file.close();
}


//XHTML generation code modified by Volker Dirr (timetabling.de) from old html generation code
//(old code by Liviu Lalescu)
void TimetableExport::writeSubgroupsTimetableDaysHorizontalHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities){
	assert(gt.rules.initialized && gt.rules.internalStructureComputed);
	//assert(gt.timePopulation.initialized);
	assert(students_schedule_ready && teachers_schedule_ready && rooms_schedule_ready);

	//Now we print the results to an HTML file
	QFile file(htmlfilename);
	if(!file.open(QIODevice::WriteOnly)){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Cannot open file %1 for writing. Please check your disk's free space. Saving of %1 aborted.").arg(htmlfilename));
		return;
		assert(0);
	}
	QTextStream tos(&file);
	tos.setCodec("UTF-8");
	tos.setGenerateByteOrderMark(true);

	tos<<writeHead(true, placedActivities, true);
	
	tos<<"    <p><strong>"<<TimetableExport::tr("Table of contents")<<"</strong></p>\n";
	tos<<"    <ul>\n";
	for(int i=0; i<gt.rules.augmentedYearsList.size(); i++){
		StudentsYear* sty=gt.rules.augmentedYearsList[i];
		tos<<"      <li>\n        "<<TimetableExport::tr("Year")<<" "<<protect2(sty->name)<<"\n        <ul>\n";
		for(int j=0; j<sty->groupsList.size(); j++){
			StudentsGroup* stg=sty->groupsList[j];
			tos<<"          <li>\n            "<<TimetableExport::tr("Group")<<" "<<protect2(stg->name)<<": \n";
			for(int k=0; k<stg->subgroupsList.size(); k++){
				StudentsSubgroup* sts=stg->subgroupsList[k];
				tos<<"              <a href=\""<<"#table_"<<hashStudentIDsTimetable.value(sts->name)<<"\">"<<protect2(sts->name)<<"</a>\n";
			}
			tos<<"          </li>\n";
		}
		tos<<"        </ul>\n      </li>\n";
	}
	tos<<"    </ul>\n    <p>&nbsp;</p>\n\n";

	for(int subgroup=0; subgroup<gt.rules.nInternalSubgroups; subgroup++){
		QString subgroup_name = gt.rules.internalSubgroupsList[subgroup]->name;
		tos<<"    <table id=\"table_"<<hashStudentIDsTimetable.value(subgroup_name)<<"\" border=\"1\"";
		if(subgroup%2==0) tos<<" class=\"odd_table\"";
		else tos<<" class=\"even_table\"";
		tos<<">\n";
				
		tos<<"      <caption>"<<protect2(gt.rules.institutionName)<<"</caption>\n";

		tos<<"      <thead>\n        <tr><td rowspan=\"2\"></td><th colspan=\""<<gt.rules.nDaysPerWeek<<"\">"<<protect2(subgroup_name)<<"</th></tr>\n";
		tos<<"        <tr>\n          <!-- span -->\n";
		for(int j=0; j<gt.rules.nDaysPerWeek; j++){
			if(TIMETABLE_HTML_LEVEL>=2)
				tos<<"          <th class=\"xAxis\">";
			else
				tos<<"          <th>";
			tos<<protect2(gt.rules.daysOfTheWeek[j])<<"</th>\n";
		}
		tos<<"        </tr>\n";
		tos<<"      </thead>\n";
		/*workaround. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
		tos<<"      <tfoot><tr><td></td><td colspan=\""<<gt.rules.nDaysPerWeek<<"\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr></tfoot>\n";
		*/
		tos<<"      <tbody>\n";
		for(int j=0; j<gt.rules.nHoursPerDay; j++){
			tos<<"        <tr>\n";
			if(TIMETABLE_HTML_LEVEL>=2)
				tos<<"          <th class=\"yAxis\">";
			else
				tos<<"          <th>";
			tos<<protect2(gt.rules.hoursOfTheDay[j])<<"</th>\n";
			for(int k=0; k<gt.rules.nDaysPerWeek; k++){
				QList<int> allActivities;
				allActivities.clear();
				allActivities<<students_timetable_weekly[subgroup][k][j];
				bool activitiesWithSameStartingtime=addActivitiesWithSameStartingTime(allActivities, j);
				if(allActivities.size()==1 && !activitiesWithSameStartingtime){  // because i am using colspan or rowspan!!!
					tos<<writeActivityStudents(students_timetable_weekly[subgroup][k][j], k, j, subgroupNotAvailableDayHour[subgroup][k][j], false, true);
				} else{
					tos<<writeActivitiesStudents(allActivities);
				}
			}
			tos<<"        </tr>\n";
		}
		//workaround begin. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
		tos<<"        <tr class=\"foot\"><td></td><td colspan=\""<<gt.rules.nDaysPerWeek<<"\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr>\n";
		//workaround end.
		tos<<"      </tbody>\n";
		tos<<"    </table>\n\n";
		tos<<"    <p class=\"back\"><a href=\""<<"#top\">"<<TimetableExport::tr("back to the top")<<"</a></p>\n\n";
	}
	tos<<"  </body>\n</html>\n";

	if(file.error()>0){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Writing %1 gave error code %2, which means saving is compromised. Please check your disk's free space.").arg(htmlfilename).arg(file.error()));
	}
	file.close();
}


//XHTML generation code modified by Volker Dirr (timetabling.de) from old html generation code
//(old code by Liviu Lalescu)
void TimetableExport::writeSubgroupsTimetableDaysVerticalHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities){
	assert(gt.rules.initialized && gt.rules.internalStructureComputed);
	//assert(gt.timePopulation.initialized);
	assert(students_schedule_ready && teachers_schedule_ready && rooms_schedule_ready);

	//Now we print the results to an HTML file
	QFile file(htmlfilename);
	if(!file.open(QIODevice::WriteOnly)){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Cannot open file %1 for writing. Please check your disk's free space. Saving of %1 aborted.").arg(htmlfilename));
		return;
		assert(0);
	}
	QTextStream tos(&file);
	tos.setCodec("UTF-8");
	tos.setGenerateByteOrderMark(true);

	tos<<writeHead(true, placedActivities, true);
		
	tos<<"    <p><strong>"<<TimetableExport::tr("Table of contents")<<"</strong></p>\n";
	tos<<"    <ul>\n";
	for(int i=0; i<gt.rules.augmentedYearsList.size(); i++){
		StudentsYear* sty=gt.rules.augmentedYearsList[i];
		tos<<"      <li>\n        "<<TimetableExport::tr("Year")<<" "<<protect2(sty->name)<<"\n        <ul>\n";
		for(int j=0; j<sty->groupsList.size(); j++){
			StudentsGroup* stg=sty->groupsList[j];
			tos<<"          <li>\n            "<<TimetableExport::tr("Group")<<" "<<protect2(stg->name)<<": \n";
			for(int k=0; k<stg->subgroupsList.size(); k++){
				StudentsSubgroup* sts=stg->subgroupsList[k];
				tos<<"              <a href=\""<<"#table_"<<hashStudentIDsTimetable.value(sts->name)<<"\">"<<protect2(sts->name)<<"</a>\n";
			}
			tos<<"          </li>\n";
		}
		tos<<"        </ul>\n      </li>\n";
	}
	tos<<"    </ul>\n    <p>&nbsp;</p>\n";

	for(int subgroup=0; subgroup<gt.rules.nInternalSubgroups; subgroup++){
		QString subgroup_name = gt.rules.internalSubgroupsList[subgroup]->name;
		tos<<"    <table id=\"table_"<<hashStudentIDsTimetable.value(subgroup_name)<<"\" border=\"1\"";
		if(subgroup%2==0) tos<<" class=\"odd_table\"";
		else tos<<" class=\"even_table\"";
		tos<<">\n";
		
		tos<<"      <caption>"<<protect2(gt.rules.institutionName)<<"</caption>\n";

		tos<<"      <thead>\n        <tr><td rowspan=\"2\"></td><th colspan=\""<<gt.rules.nHoursPerDay<<"\">"<<protect2(subgroup_name)<<"</th></tr>\n";
		tos<<"        <tr>\n          <!-- span -->\n";
		for(int j=0; j<gt.rules.nHoursPerDay; j++){
			if(TIMETABLE_HTML_LEVEL>=2)
				tos<<"          <th class=\"xAxis\">";
			else
				tos<<"          <th>";
			tos<<protect2(gt.rules.hoursOfTheDay[j])<<"</th>\n";
		}
		tos<<"        </tr>\n";
		tos<<"      </thead>\n";
		/*workaround. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
		tos<<"      <tfoot><tr><td></td><td colspan=\""<<gt.rules.nHoursPerDay<<"\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr></tfoot>\n";
		*/
		tos<<"      <tbody>\n";
		for(int k=0; k<gt.rules.nDaysPerWeek; k++){
			tos<<"        <tr>\n";
			if(TIMETABLE_HTML_LEVEL>=2)
				tos<<"          <th class=\"yAxis\">";
			else
				tos<<"          <th>";
			tos<<protect2(gt.rules.daysOfTheWeek[k])<<"</th>\n";

			for(int j=0; j<gt.rules.nHoursPerDay; j++){
				QList<int> allActivities;
				allActivities.clear();
				allActivities<<students_timetable_weekly[subgroup][k][j];
				bool activitiesWithSameStartingtime=addActivitiesWithSameStartingTime(allActivities, j);
				if(allActivities.size()==1 && !activitiesWithSameStartingtime){  // because i am using colspan or rowspan!!!
					tos<<writeActivityStudents(students_timetable_weekly[subgroup][k][j], k, j, subgroupNotAvailableDayHour[subgroup][k][j], true, false);
				} else{
					tos<<writeActivitiesStudents(allActivities);
				}
			}
			tos<<"        </tr>\n";
		}
		//workaround begin. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
		tos<<"        <tr class=\"foot\"><td></td><td colspan=\""<<gt.rules.nHoursPerDay<<"\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr>\n";
		//workaround end.
		tos<<"      </tbody>\n";
		tos<<"    </table>\n\n";
		tos<<"    <p class=\"back\"><a href=\""<<"#top\">"<<TimetableExport::tr("back to the top")<<"</a></p>\n\n";
	}

	tos<<"  </body>\n</html>\n";

	if(file.error()>0){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Writing %1 gave error code %2, which means saving is compromised. Please check your disk's free space.").arg(htmlfilename).arg(file.error()));
	}
	file.close();
}


//XHTML generation code by Volker Dirr (http://timetabling.de/)
void TimetableExport::writeSubgroupsTimetableTimeVerticalHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities){
	assert(gt.rules.initialized && gt.rules.internalStructureComputed);
	//assert(gt.timePopulation.initialized);
	assert(students_schedule_ready && teachers_schedule_ready && rooms_schedule_ready);

	//Now we print the results to an HTML file
	QFile file(htmlfilename);
	if(!file.open(QIODevice::WriteOnly)){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Cannot open file %1 for writing. Please check your disk's free space. Saving of %1 aborted.").arg(htmlfilename));
		return;
		assert(0);
	}
	QTextStream tos(&file);
	tos.setCodec("UTF-8");
	tos.setGenerateByteOrderMark(true);

	tos<<writeHead(true, placedActivities, false);
	
	tos<<"    <table border=\"1\">\n";
	tos<<"      <caption>"<<protect2(gt.rules.institutionName)<<"</caption>\n";

	tos<<"      <thead>\n        <tr><td colspan=\"2\"></td>";
	for(int i=0; i<gt.rules.nInternalSubgroups; i++){
		if(TIMETABLE_HTML_LEVEL>=2)
			tos<<"          <th class=\"xAxis\">";
		else
			tos<<"          <th>";
		tos << gt.rules.internalSubgroupsList[i]->name << "</th>";
	}
	tos<<"</tr>\n      </thead>\n";
	/*workaround. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
	tos<<"      <tfoot><tr><td colspan=\"2\"></td><td colspan=\""<<gt.rules.nInternalSubgroups<<"\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr></tfoot>\n";
	*/
	tos<<"      <tbody>\n";
	for(int k=0; k<gt.rules.nDaysPerWeek; k++){
		for(int j=0; j<gt.rules.nHoursPerDay; j++){
			tos << "        <tr>\n";
			if(j==0)
				tos << "        <th rowspan=\"" << gt.rules.nHoursPerDay  << "\">" << protect2vert(gt.rules.daysOfTheWeek[k]) << "</th>\n";
			else tos <<"          <!-- span -->\n";

			if(TIMETABLE_HTML_LEVEL>=2)
				tos<<"          <th class=\"yAxis\">";
			else
				tos<<"          <th>";
			tos << protect2(gt.rules.hoursOfTheDay[j]) << "</th>\n";
			for(int subgroup=0; subgroup<gt.rules.nInternalSubgroups; subgroup++){
				QList<int> allActivities;
				allActivities.clear();
				allActivities<<students_timetable_weekly[subgroup][k][j];
				bool activitiesWithSameStartingtime=addActivitiesWithSameStartingTime(allActivities, j);
				if(allActivities.size()==1 && !activitiesWithSameStartingtime){  // because i am using colspan or rowspan!!!
					tos<<writeActivityStudents(students_timetable_weekly[subgroup][k][j], k, j, subgroupNotAvailableDayHour[subgroup][k][j], false, true);
				} else{
					tos<<writeActivitiesStudents(allActivities);
				}
			}
			tos<<"        </tr>\n";
		}
	}
	//workaround begin. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
	tos<<"      <tr class=\"foot\"><td colspan=\"2\"></td><td colspan=\""<<gt.rules.nInternalSubgroups<<"\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr>\n";
	//workaround end.
	tos << "      </tbody>\n    </table>\n";
	tos << "  </body>\n</html>\n";

	if(file.error()>0){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Writing %1 gave error code %2, which means saving is compromised. Please check your disk's free space.").arg(htmlfilename).arg(file.error()));
	}	
	file.close();
}


//XHTML generation code modified by Volker Dirr (timetabling.de) from old html generation code
//(old code by Liviu Lalescu)
void TimetableExport::writeSubgroupsTimetableTimeHorizontalHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities){
	assert(gt.rules.initialized && gt.rules.internalStructureComputed);
	//assert(gt.timePopulation.initialized);
	assert(students_schedule_ready && teachers_schedule_ready && rooms_schedule_ready);

	//Now we print the results to an HTML file
	QFile file(htmlfilename);
	if(!file.open(QIODevice::WriteOnly)){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Cannot open file %1 for writing. Please check your disk's free space. Saving of %1 aborted.").arg(htmlfilename));
		return;
		assert(0);
	}
	QTextStream tos(&file);
	tos.setCodec("UTF-8");
	tos.setGenerateByteOrderMark(true);

	tos<<writeHead(true, placedActivities, false);
	
	tos<<"    <table border=\"1\">\n";
	tos<<"      <caption>"<<protect2(gt.rules.institutionName)<<"</caption>\n";

	tos<<"      <thead>\n        <tr><td rowspan=\"2\"></td>";
	for(int k=0; k<gt.rules.nDaysPerWeek; k++)
		tos << "<th colspan=\"" << gt.rules.nHoursPerDay <<"\">" << protect2(gt.rules.daysOfTheWeek[k]) << "</th>";
	tos<<"        </tr>\n";
	tos<<"        <tr>\n          <!-- span -->\n";
	for(int k=0; k<gt.rules.nDaysPerWeek; k++)
		for(int j=0; j<gt.rules.nHoursPerDay; j++){
			if(TIMETABLE_HTML_LEVEL>=2)
				tos<<"          <th class=\"xAxis\">";
			else
				tos<<"          <th>";
			tos << protect2(gt.rules.hoursOfTheDay[j]) << "</th>\n";
		}
	tos<<"        </tr>\n";
	tos<<"      </thead>\n";
	/*workaround. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
	tos<<"      <tfoot><tr><td></td><td colspan=\""<<gt.rules.nHoursPerDay*gt.rules.nDaysPerWeek<<"\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr></tfoot>\n";
	*/
	tos<<"      <tbody>\n";
	for(int subgroup=0; subgroup<gt.rules.nInternalSubgroups; subgroup++){
		tos << "        <tr>\n";
		if(TIMETABLE_HTML_LEVEL>=2)
			tos<<"          <th class=\"yAxis\">";
		else
			tos<<"          <th>";
		tos << gt.rules.internalSubgroupsList[subgroup]->name << "</th>\n";
		for(int k=0; k<gt.rules.nDaysPerWeek; k++){
			for(int j=0; j<gt.rules.nHoursPerDay; j++){
				QList<int> allActivities;
				allActivities.clear();
				allActivities<<students_timetable_weekly[subgroup][k][j];
				bool activitiesWithSameStartingtime=addActivitiesWithSameStartingTime(allActivities, j);
				if(allActivities.size()==1 && !activitiesWithSameStartingtime){  // because i am using colspan or rowspan!!!
					tos<<writeActivityStudents(students_timetable_weekly[subgroup][k][j], k, j, subgroupNotAvailableDayHour[subgroup][k][j], true, false);
				} else{
					tos<<writeActivitiesStudents(allActivities);
				}
			}
		}
		tos<<"        </tr>\n";
	}
	//workaround begin. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
	tos<<"      <tr class=\"foot\"><td></td><td colspan=\""<<gt.rules.nHoursPerDay*gt.rules.nDaysPerWeek<<"\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr>\n";
	//workaround end.
	tos << "      </tbody>\n    </table>\n";
	tos << "  </body>\n</html>\n";

	if(file.error()>0){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Writing %1 gave error code %2, which means saving is compromised. Please check your disk's free space.").arg(htmlfilename).arg(file.error()));
	}
	file.close();
}


// by Volker Dirr
void TimetableExport::writeSubgroupsTimetableTimeVerticalDailyHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities){
	assert(gt.rules.initialized && gt.rules.internalStructureComputed);
	//assert(gt.timePopulation.initialized);
	assert(students_schedule_ready && teachers_schedule_ready && rooms_schedule_ready);

	//Now we print the results to an HTML file
	QFile file(htmlfilename);
	if(!file.open(QIODevice::WriteOnly)){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Cannot open file %1 for writing. Please check your disk's free space. Saving of %1 aborted.").arg(htmlfilename));
		return;
		assert(0);
	}
	QTextStream tos(&file);
	tos.setCodec("UTF-8");
	tos.setGenerateByteOrderMark(true);

	tos<<writeHead(true, placedActivities, true);
	tos<<writeTOCDays(false);

	for(int k=0; k<gt.rules.nDaysPerWeek; k++){
		tos<<"    <table id=\"table_"<<hashDayIDsTimetable.value(gt.rules.daysOfTheWeek[k])<<"\" border=\"1\">\n";
		tos<<"      <caption>"<<protect2(gt.rules.institutionName)<<"</caption>\n";
	
		tos<<"      <thead>\n        <tr><td colspan=\"2\"></td>";
		for(int subgroup=0; subgroup<gt.rules.nInternalSubgroups; subgroup++){
			if(TIMETABLE_HTML_LEVEL>=2)
				tos<<"          <th class=\"xAxis\">";
			else
				tos<<"          <th>";
			tos << gt.rules.internalSubgroupsList[subgroup]->name << "</th>";
		}
		tos<<"</tr>\n      </thead>\n";
		/*workaround. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
		tos<<"      <tfoot><tr><td colspan=\"2\"></td><td colspan=\""<<gt.rules.nInternalSubgroups<<"\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr></tfoot>\n";
		*/
		tos<<"      <tbody>\n";

		for(int j=0; j<gt.rules.nHoursPerDay; j++){
			tos << "        <tr>\n";
			if(j==0)
				tos << "        <th rowspan=\"" << gt.rules.nHoursPerDay  << "\">" << protect2vert(gt.rules.daysOfTheWeek[k]) << "</th>\n";
			else tos <<"          <!-- span -->\n";

			if(TIMETABLE_HTML_LEVEL>=2)
				tos<<"          <th class=\"yAxis\">";
			else
				tos<<"          <th>";
			tos << protect2(gt.rules.hoursOfTheDay[j]) << "</th>\n";
			for(int subgroup=0; subgroup<gt.rules.nInternalSubgroups; subgroup++){
				QList<int> allActivities;
				allActivities.clear();
				allActivities<<students_timetable_weekly[subgroup][k][j];
				bool activitiesWithSameStartingtime=addActivitiesWithSameStartingTime(allActivities, j);
				if(allActivities.size()==1 && !activitiesWithSameStartingtime){  // because i am using colspan or rowspan!!!
					tos<<writeActivityStudents(students_timetable_weekly[subgroup][k][j], k, j, subgroupNotAvailableDayHour[subgroup][k][j], false, true);
				} else{
					tos<<writeActivitiesStudents(allActivities);
				}
			}
			tos<<"        </tr>\n";
		}
		//workaround begin. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
		tos<<"        <tr class=\"foot\"><td colspan=\"2\"></td><td colspan=\""<<gt.rules.nInternalSubgroups<<"\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr>\n";
		//workaround end.
		tos<<"      </tbody>\n";
		tos<<"    </table>\n\n";
		tos<<"    <p class=\"back\"><a href=\""<<"#top\">"<<TimetableExport::tr("back to the top")<<"</a></p>\n\n";
	}

	tos << "  </body>\n</html>\n";

	if(file.error()>0){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Writing %1 gave error code %2, which means saving is compromised. Please check your disk's free space.").arg(htmlfilename).arg(file.error()));
	}	
	file.close();
} 



// by Volker Dirr
void TimetableExport::writeSubgroupsTimetableTimeHorizontalDailyHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities){
	assert(gt.rules.initialized && gt.rules.internalStructureComputed);
	//assert(gt.timePopulation.initialized);
	assert(students_schedule_ready && teachers_schedule_ready && rooms_schedule_ready);

	//Now we print the results to an HTML file
	QFile file(htmlfilename);
	if(!file.open(QIODevice::WriteOnly)){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Cannot open file %1 for writing. Please check your disk's free space. Saving of %1 aborted.").arg(htmlfilename));
		return;
		assert(0);
	}
	QTextStream tos(&file);
	tos.setCodec("UTF-8");
	tos.setGenerateByteOrderMark(true);

	tos<<writeHead(true, placedActivities, true);
	tos<<writeTOCDays(false);
	
	for(int k=0; k<gt.rules.nDaysPerWeek; k++){
		tos<<"    <table id=\"table_"<<hashDayIDsTimetable.value(gt.rules.daysOfTheWeek[k])<<"\" border=\"1\">\n";
		tos<<"      <caption>"<<protect2(gt.rules.institutionName)<<"</caption>\n";
	
		tos<<"      <thead>\n        <tr><td rowspan=\"2\"></td>";
		tos << "<th colspan=\"" << gt.rules.nHoursPerDay <<"\">" << protect2(gt.rules.daysOfTheWeek[k]) << "</th>";
		tos<<"        </tr>\n";
		tos<<"        <tr>\n          <!-- span -->\n";
		for(int j=0; j<gt.rules.nHoursPerDay; j++){
			if(TIMETABLE_HTML_LEVEL>=2)
				tos<<"          <th class=\"xAxis\">";
			else
				tos<<"          <th>";
			tos << protect2(gt.rules.hoursOfTheDay[j]) << "</th>\n";
			}
		tos<<"        </tr>\n";
		tos<<"      </thead>\n";
		/*workaround. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
		tos<<"      <tfoot><tr><td></td><td colspan=\""<<gt.rules.nHoursPerDay<<"\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr></tfoot>\n";
		*/
		tos<<"      <tbody>\n";
		for(int subgroup=0; subgroup<gt.rules.nInternalSubgroups; subgroup++){
			tos << "        <tr>\n";
			if(TIMETABLE_HTML_LEVEL>=2)
				tos<<"          <th class=\"yAxis\">";
			else
				tos<<"          <th>";
			tos << gt.rules.internalSubgroupsList[subgroup]->name << "</th>\n";
			for(int j=0; j<gt.rules.nHoursPerDay; j++){
				QList<int> allActivities;
				allActivities.clear();
				allActivities<<students_timetable_weekly[subgroup][k][j];
				bool activitiesWithSameStartingtime=addActivitiesWithSameStartingTime(allActivities, j);
				if(allActivities.size()==1 && !activitiesWithSameStartingtime){  // because i am using colspan or rowspan!!!
					tos<<writeActivityStudents(students_timetable_weekly[subgroup][k][j], k, j, subgroupNotAvailableDayHour[subgroup][k][j], true, false);
				} else{
					tos<<writeActivitiesStudents(allActivities);
				}
			}
			tos<<"        </tr>\n";
		}
		//workaround begin. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
		tos<<"        <tr class=\"foot\"><td></td><td colspan=\""<<gt.rules.nHoursPerDay<<"\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr>\n";
		//workaround end.
		tos<<"      </tbody>\n";
		tos<<"    </table>\n\n";
		tos<<"    <p class=\"back\"><a href=\""<<"#top\">"<<TimetableExport::tr("back to the top")<<"</a></p>\n\n";
	}
	tos << "  </body>\n</html>\n";

	if(file.error()>0){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Writing %1 gave error code %2, which means saving is compromised. Please check your disk's free space.").arg(htmlfilename).arg(file.error()));
	}
	file.close();
}


//Now print the groups

//XHTML generation code by Volker Dirr (http://timetabling.de/)
void TimetableExport::writeGroupsTimetableDaysHorizontalHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities){
	assert(gt.rules.initialized && gt.rules.internalStructureComputed);
	//assert(gt.timePopulation.initialized);
	assert(students_schedule_ready && teachers_schedule_ready && rooms_schedule_ready);

	//Now we print the results to an HTML file
	QFile file(htmlfilename);
	if(!file.open(QIODevice::WriteOnly)){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Cannot open file %1 for writing. Please check your disk's free space. Saving of %1 aborted.").arg(htmlfilename));
		return;
		assert(0);
	}
	QTextStream tos(&file);
	tos.setCodec("UTF-8");
	tos.setGenerateByteOrderMark(true);

	tos<<writeHead(true, placedActivities, true);
		
	tos<<"    <p><strong>"<<TimetableExport::tr("Table of contents")<<"</strong></p>\n";
	tos<<"    <ul>\n";
	for(int i=0; i<gt.rules.augmentedYearsList.size(); i++){
		StudentsYear* sty=gt.rules.augmentedYearsList[i];
		tos<<"      <li>\n        "<<TimetableExport::tr("Year")<<" "<<protect2(sty->name)<<"\n        <ul>\n";
		for(int j=0; j<sty->groupsList.size(); j++){
			StudentsGroup* stg=sty->groupsList[j];
			tos<<"          <li>\n            "<<TimetableExport::tr("Group");
			tos<<" <a href=\""<<"#table_"<<hashStudentIDsTimetable.value(stg->name)<<"_DETAILED\">"<<protect2(stg->name)<<" ("<<tr("Detailed")<<")</a> /";
			tos<<" <a href=\""<<"#table_"<<hashStudentIDsTimetable.value(stg->name)<<"\">"<<protect2(stg->name)<<" ("<<tr("Less detailed")<<")</a>\n";
			tos<<"          </li>\n";
		}
		tos<<"        </ul>\n      </li>\n";
	}
	tos<<"    </ul>\n    <p>&nbsp;</p>\n\n";

	bool PRINT_DETAILED=true;
	do{
		int group=0;
		for(int i=0; i<gt.rules.augmentedYearsList.size(); i++){
			StudentsYear* sty=gt.rules.augmentedYearsList[i];
			for(int g=0; g<sty->groupsList.size(); g++){
				StudentsGroup* stg=sty->groupsList[g];
				tos<<"    <table id=\"table_"<<hashStudentIDsTimetable.value(stg->name);
				if(PRINT_DETAILED==true) tos<<"_DETAILED";
				tos<<"\" border=\"1\"";
				if(group%2==0) tos<<" class=\"odd_table\"";
				else tos<<" class=\"even_table\"";
				tos<<">\n";
						
				tos<<"      <caption>"<<protect2(gt.rules.institutionName)<<"</caption>\n";

				tos<<"      <thead>\n        <tr><td rowspan=\"2\"></td><th colspan=\""<<gt.rules.nDaysPerWeek<<"\">"<<protect2(stg->name)<<"</th></tr>\n";
				tos<<"        <tr>\n          <!-- span -->\n";
				for(int j=0; j<gt.rules.nDaysPerWeek; j++){
					if(TIMETABLE_HTML_LEVEL>=2)
						tos<<"          <th class=\"xAxis\">";
					else
						tos<<"          <th>";
					tos<<protect2(gt.rules.daysOfTheWeek[j])<<"</th>\n";
				}
				tos<<"        </tr>\n";
				tos<<"      </thead>\n";
				/*workaround. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
				tos<<"      <tfoot><tr><td></td><td colspan=\""<<gt.rules.nDaysPerWeek<<"\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr></tfoot>\n";
				*/
				tos<<"      <tbody>\n";
				for(int j=0; j<gt.rules.nHoursPerDay; j++){
					tos<<"        <tr>\n";
					if(TIMETABLE_HTML_LEVEL>=2)
						tos<<"          <th class=\"yAxis\">";
					else
						tos<<"          <th>";
					tos<<protect2(gt.rules.hoursOfTheDay[j])<<"</th>\n";
					for(int k=0; k<gt.rules.nDaysPerWeek; k++){
						QList<int> allActivities;
						allActivities.clear();
						bool isNotAvailable=true;
						for(int sg=0; sg<stg->subgroupsList.size(); sg++){
							StudentsSubgroup* sts=stg->subgroupsList[sg];
							//old code by Volker
							/*
							for(int subgroup=0; subgroup<gt.rules.nInternalSubgroups; subgroup++){
								QString subgroup_name = gt.rules.internalSubgroupsList[subgroup]->name;
								if(subgroup_name==sts->name)
							*/
							//speed improvement by Liviu
							int subgroup=sts->indexInInternalSubgroupsList;
									if(!(allActivities.contains(students_timetable_weekly[subgroup][k][j])))
												allActivities<<students_timetable_weekly[subgroup][k][j];
							//}
							if(!subgroupNotAvailableDayHour[subgroup][k][j])
								isNotAvailable=false;
						}
						assert(!allActivities.isEmpty());
						bool activitiesWithSameStartingtime=addActivitiesWithSameStartingTime(allActivities, j);
						if(allActivities.size()==1 && !activitiesWithSameStartingtime){  // because i am using colspan or rowspan!!!
							tos<<writeActivityStudents(allActivities[0], k, j, isNotAvailable, false, true);
						} else{
							if(PRINT_DETAILED==false) tos<<"          <td>"<<protect2(STRING_SEVERAL_ACTIVITIES_IN_LESS_DETAILED_TABLES)<<"</td>\n";
							else{
								tos<<writeActivitiesStudents(allActivities);
							}
						}
					}
					tos<<"        </tr>\n";
				}
				//workaround begin. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
				tos<<"        <tr class=\"foot\"><td></td><td colspan=\""<<gt.rules.nDaysPerWeek<<"\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr>\n";
				//workaround end.
				tos<<"      </tbody>\n";
				tos<<"    </table>\n\n";
				tos<<"    <p class=\"back\"><a href=\""<<"#top\">"<<TimetableExport::tr("back to the top")<<"</a></p>\n\n";
				group++;
			}
		}
		if(PRINT_DETAILED==true) PRINT_DETAILED=false;
		else PRINT_DETAILED=true;
	} while(PRINT_DETAILED!=true);
	tos<<"  </body>\n</html>\n";

	if(file.error()>0){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Writing %1 gave error code %2, which means saving is compromised. Please check your disk's free space.").arg(htmlfilename).arg(file.error()));
	}
	file.close();
}


//XHTML generation code by Volker Dirr (http://timetabling.de/)
void TimetableExport::writeGroupsTimetableDaysVerticalHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities){
	assert(gt.rules.initialized && gt.rules.internalStructureComputed);
	//assert(gt.timePopulation.initialized);
	assert(students_schedule_ready && teachers_schedule_ready && rooms_schedule_ready);

	//Now we print the results to an HTML file
	QFile file(htmlfilename);
	if(!file.open(QIODevice::WriteOnly)){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Cannot open file %1 for writing. Please check your disk's free space. Saving of %1 aborted.").arg(htmlfilename));
		return;
		assert(0);
	}
	QTextStream tos(&file);
	tos.setCodec("UTF-8");
	tos.setGenerateByteOrderMark(true);

	tos<<writeHead(true, placedActivities, true);
		
	tos<<"    <p><strong>"<<TimetableExport::tr("Table of contents")<<"</strong></p>\n";
	tos<<"    <ul>\n";
	for(int i=0; i<gt.rules.augmentedYearsList.size(); i++){
		StudentsYear* sty=gt.rules.augmentedYearsList[i];
		tos<<"      <li>\n        "<<TimetableExport::tr("Year")<<" "<<protect2(sty->name)<<"\n        <ul>\n";
		for(int j=0; j<sty->groupsList.size(); j++){
			StudentsGroup* stg=sty->groupsList[j];
			tos<<"          <li>\n            "<<TimetableExport::tr("Group");
			tos<<" <a href=\""<<"#table_"<<hashStudentIDsTimetable.value(stg->name)<<"_DETAILED\">"<<protect2(stg->name)<<" ("<<tr("Detailed")<<")</a> /";
			tos<<" <a href=\""<<"#table_"<<hashStudentIDsTimetable.value(stg->name)<<"\">"<<protect2(stg->name)<<" ("<<tr("Less detailed")<<")</a>\n";
			tos<<"          </li>\n";
		}
		tos<<"        </ul>\n      </li>\n";
	}
	tos<<"    </ul>\n    <p>&nbsp;</p>\n";

	bool PRINT_DETAILED=true;
	do{
		int group=0;
		for(int i=0; i<gt.rules.augmentedYearsList.size(); i++){
			StudentsYear* sty=gt.rules.augmentedYearsList[i];
			for(int g=0; g<sty->groupsList.size(); g++){
				StudentsGroup* stg=sty->groupsList[g];
				tos<<"    <table id=\"table_"<<hashStudentIDsTimetable.value(stg->name);
				if(PRINT_DETAILED==true) tos<<"_DETAILED";
				tos<<"\" border=\"1\"";
				if(group%2==0) tos<<" class=\"odd_table\"";
				else tos<<" class=\"even_table\"";
				tos<<">\n";
						
				tos<<"      <caption>"<<protect2(gt.rules.institutionName)<<"</caption>\n";

				tos<<"      <thead>\n        <tr><td rowspan=\"2\"></td><th colspan=\""<<gt.rules.nHoursPerDay<<"\">"<<protect2(stg->name)<<"</th></tr>\n";
				tos<<"        <tr>\n          <!-- span -->\n";
				for(int j=0; j<gt.rules.nHoursPerDay; j++){
					if(TIMETABLE_HTML_LEVEL>=2)
						tos<<"          <th class=\"xAxis\">";
					else
						tos<<"          <th>";
					tos<<protect2(gt.rules.hoursOfTheDay[j])<<"</th>\n";
				}
				tos<<"        </tr>\n";
				tos<<"      </thead>\n";
				/*workaround. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
				tos<<"      <tfoot><tr><td></td><td colspan=\""<<gt.rules.nHoursPerDay<<"\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr></tfoot>\n";
				*/
				tos<<"      <tbody>\n";
				for(int k=0; k<gt.rules.nDaysPerWeek; k++){
					tos<<"        <tr>\n";
					if(TIMETABLE_HTML_LEVEL>=2)
						tos<<"          <th class=\"yAxis\">";
					else
						tos<<"          <th>";
					tos<<protect2(gt.rules.daysOfTheWeek[k])<<"</th>\n";
					for(int j=0; j<gt.rules.nHoursPerDay; j++){
						QList<int> allActivities;
						allActivities.clear();
						bool isNotAvailable=true;
						for(int sg=0; sg<stg->subgroupsList.size(); sg++){
							StudentsSubgroup* sts=stg->subgroupsList[sg];
							//old code by Volker
							/*
							for(int subgroup=0; subgroup<gt.rules.nInternalSubgroups; subgroup++){
								QString subgroup_name = gt.rules.internalSubgroupsList[subgroup]->name;
								if(subgroup_name==sts->name)
							*/
							//speed improvement by Liviu
							int subgroup=sts->indexInInternalSubgroupsList;
									if(!(allActivities.contains(students_timetable_weekly[subgroup][k][j])))
												allActivities<<students_timetable_weekly[subgroup][k][j];
							//}
							if(!subgroupNotAvailableDayHour[subgroup][k][j])
								isNotAvailable=false;
						}
						assert(!allActivities.isEmpty());
						bool activitiesWithSameStartingtime=addActivitiesWithSameStartingTime(allActivities, j);
						if(allActivities.size()==1 && !activitiesWithSameStartingtime){  // because i am using colspan or rowspan!!!
							tos<<writeActivityStudents(allActivities[0], k, j, isNotAvailable, true, false);
						} else{
							if(PRINT_DETAILED==false) tos<<"          <td>"<<protect2(STRING_SEVERAL_ACTIVITIES_IN_LESS_DETAILED_TABLES)<<"</td>\n";
							else{
								tos<<writeActivitiesStudents(allActivities);
							}
						}
					}
					tos<<"        </tr>\n";
				}
				//workaround begin. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
				tos<<"        <tr class=\"foot\"><td></td><td colspan=\""<<gt.rules.nHoursPerDay<<"\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr>\n";
				//workaround end.
				tos<<"      </tbody>\n";
				tos<<"    </table>\n\n";
				tos<<"    <p class=\"back\"><a href=\""<<"#top\">"<<TimetableExport::tr("back to the top")<<"</a></p>\n\n";
				group++;
			}
		}
		if(PRINT_DETAILED==true) PRINT_DETAILED=false;
		else PRINT_DETAILED=true;
	} while(PRINT_DETAILED!=true);
	tos<<"  </body>\n</html>\n";

	if(file.error()>0){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Writing %1 gave error code %2, which means saving is compromised. Please check your disk's free space.").arg(htmlfilename).arg(file.error()));
	}
	file.close();
}


//XHTML generation code by Volker Dirr (http://timetabling.de/)
void TimetableExport::writeGroupsTimetableTimeVerticalHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities){
	assert(gt.rules.initialized && gt.rules.internalStructureComputed);
	//assert(gt.timePopulation.initialized);
	assert(students_schedule_ready && teachers_schedule_ready && rooms_schedule_ready);

	//Now we print the results to an HTML file
	QFile file(htmlfilename);
	if(!file.open(QIODevice::WriteOnly)){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Cannot open file %1 for writing. Please check your disk's free space. Saving of %1 aborted.").arg(htmlfilename));
		return;
		assert(0);
	}
	QTextStream tos(&file);
	tos.setCodec("UTF-8");
	tos.setGenerateByteOrderMark(true);

	tos<<writeHead(true, placedActivities, false);
	
	bool PRINT_DETAILED=true;
	do{
		tos<<"    <table id=\"table";
		if(PRINT_DETAILED==false)
			tos<<"_LESS";
		tos<<"_DETAILED";
		tos<<"\" border=\"1\">\n";

		tos<<"      <caption>"<<protect2(gt.rules.institutionName)<<"</caption>\n";

		tos<<"      <thead>\n        <tr><td colspan=\"2\"></td>";
		int group=0;
		for(int i=0; i<gt.rules.augmentedYearsList.size(); i++){
			StudentsYear* sty=gt.rules.augmentedYearsList[i];
			for(int g=0; g<sty->groupsList.size(); g++){
				group++;
				StudentsGroup* stg=sty->groupsList[g];
				if(TIMETABLE_HTML_LEVEL>=2)
					tos<<"          <th class=\"xAxis\">";
				else
					tos<<"          <th>";
				tos << stg->name << "</th>";
			}
		}
		
		tos<<"</tr>\n      </thead>\n";
		/*workaround. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
		tos<<"      <tfoot><tr><td colspan=\"2\"></td><td colspan=\""<<group<<"\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr></tfoot>\n";
		*/
		tos<<"      <tbody>\n";
		for(int k=0; k<gt.rules.nDaysPerWeek; k++){
			for(int j=0; j<gt.rules.nHoursPerDay; j++){
				tos << "        <tr>\n";
				if(j==0)
					tos << "        <th rowspan=\"" << gt.rules.nHoursPerDay  << "\">" << protect2vert(gt.rules.daysOfTheWeek[k]) << "</th>\n";
				else tos <<"          <!-- span -->\n";
				if(TIMETABLE_HTML_LEVEL>=2)
					tos<<"          <th class=\"yAxis\">";
				else
					tos<<"          <th>";
				tos << protect2(gt.rules.hoursOfTheDay[j]) << "</th>\n";
				for(int i=0; i<gt.rules.augmentedYearsList.size(); i++){
					StudentsYear* sty=gt.rules.augmentedYearsList[i];
					for(int g=0; g<sty->groupsList.size(); g++){
						StudentsGroup* stg=sty->groupsList[g];
						QList<int> allActivities;
						allActivities.clear();
						bool isNotAvailable=true;
						for(int sg=0; sg<stg->subgroupsList.size(); sg++){
							StudentsSubgroup* sts=stg->subgroupsList[sg];
							//old code by Volker
							/*
							for(int subgroup=0; subgroup<gt.rules.nInternalSubgroups; subgroup++){
								QString subgroup_name = gt.rules.internalSubgroupsList[subgroup]->name;
								if(subgroup_name==sts->name)
							*/
							//speed improvement by Liviu
							int subgroup=sts->indexInInternalSubgroupsList;
									if(!(allActivities.contains(students_timetable_weekly[subgroup][k][j])))
												allActivities<<students_timetable_weekly[subgroup][k][j];
							//}
							if(!subgroupNotAvailableDayHour[subgroup][k][j])
								isNotAvailable=false;
						}
						assert(!allActivities.isEmpty());
						bool activitiesWithSameStartingtime=addActivitiesWithSameStartingTime(allActivities, j);
						if(allActivities.size()==1 && !activitiesWithSameStartingtime){  // because i am using colspan or rowspan!!!
							tos<<writeActivityStudents(allActivities[0], k, j, isNotAvailable, false, true);
						} else{
							if(PRINT_DETAILED==false) tos<<"          <td>"<<protect2(STRING_SEVERAL_ACTIVITIES_IN_LESS_DETAILED_TABLES)<<"</td>\n";
							else{
								tos<<writeActivitiesStudents(allActivities);
							}
						}
					}
				}
				tos<<"        </tr>\n";
			}
		}
		//workaround begin. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
		tos<<"        <tr class=\"foot\"><td colspan=\"2\"></td><td colspan=\""<<group<<"\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr>\n";
		//workaround end.
		tos<<"      </tbody>\n";
		tos<<"    </table>\n\n";
		tos<<"    <p class=\"back\"><a href=\""<<"#top\">"<<TimetableExport::tr("back to the top")<<"</a></p>\n\n";
		if(PRINT_DETAILED==true) PRINT_DETAILED=false;
		else PRINT_DETAILED=true;
	} while(PRINT_DETAILED!=true);
	tos << "  </body>\n</html>\n";

	if(file.error()>0){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Writing %1 gave error code %2, which means saving is compromised. Please check your disk's free space.").arg(htmlfilename).arg(file.error()));
	}
	file.close();
}

//XHTML generation code by Volker Dirr (http://timetabling.de/)
void TimetableExport::writeGroupsTimetableTimeHorizontalHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities){
	assert(gt.rules.initialized && gt.rules.internalStructureComputed);
	//assert(gt.timePopulation.initialized);
	assert(students_schedule_ready && teachers_schedule_ready && rooms_schedule_ready);

	//Now we print the results to an HTML file
	QFile file(htmlfilename);
	if(!file.open(QIODevice::WriteOnly)){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Cannot open file %1 for writing. Please check your disk's free space. Saving of %1 aborted.").arg(htmlfilename));
		return;
		assert(0);
	}
	QTextStream tos(&file);
	tos.setCodec("UTF-8");
	tos.setGenerateByteOrderMark(true);

	tos<<writeHead(true, placedActivities, false);
	
	bool PRINT_DETAILED=true;
	do{
		tos<<"    <table id=\"table";
		if(PRINT_DETAILED==false)
			tos<<"_LESS";
		tos<<"_DETAILED";
		tos<<"\" border=\"1\">\n";

		tos<<"      <caption>"<<protect2(gt.rules.institutionName)<<"</caption>\n";

		tos<<"      <thead>\n        <tr><td rowspan=\"2\"></td>";
		for(int k=0; k<gt.rules.nDaysPerWeek; k++)
			tos << "<th colspan=\"" << gt.rules.nHoursPerDay <<"\">" << protect2(gt.rules.daysOfTheWeek[k]) << "</th>";
		tos <<"</tr>\n";
		tos<<"        <tr>\n          <!-- span -->\n";
		for(int k=0; k<gt.rules.nDaysPerWeek; k++)
			for(int j=0; j<gt.rules.nHoursPerDay; j++){
				if(TIMETABLE_HTML_LEVEL>=2)
					tos<<"          <th class=\"xAxis\">";
				else
					tos<<"          <th>";
				tos << protect2(gt.rules.hoursOfTheDay[j]) << "</th>\n";
			}
		tos<<"        </tr>\n";
		tos<<"      </thead>\n";
		/*workaround. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
		tos<<"      <tfoot><tr><td colspan=\"2\"></td><td colspan=\""<<gt.rules.nDaysPerWeek*gt.rules.nHoursPerDay<<"\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr></tfoot>\n";
		*/
		tos<<"      <tbody>\n";
		
		for(int i=0; i<gt.rules.augmentedYearsList.size(); i++){
			StudentsYear* sty=gt.rules.augmentedYearsList[i];
			for(int g=0; g<sty->groupsList.size(); g++){
				StudentsGroup* stg=sty->groupsList[g];
				tos << "        <tr>\n";
				if(TIMETABLE_HTML_LEVEL>=2)
					tos<<"          <th class=\"yAxis\">";
				else
					tos<<"          <th>";
				tos << protect2(stg->name) << "</th>\n";
				for(int k=0; k<gt.rules.nDaysPerWeek; k++){
					for(int j=0; j<gt.rules.nHoursPerDay; j++){
						QList<int> allActivities;
						allActivities.clear();
						bool isNotAvailable=true;
						for(int sg=0; sg<stg->subgroupsList.size(); sg++){
							StudentsSubgroup* sts=stg->subgroupsList[sg];
							//old code by Volker
							/*
							for(int subgroup=0; subgroup<gt.rules.nInternalSubgroups; subgroup++){
								QString subgroup_name = gt.rules.internalSubgroupsList[subgroup]->name;
								if(subgroup_name==sts->name)
							*/
							//speed improvement by Liviu
							int subgroup=sts->indexInInternalSubgroupsList;
									if(!(allActivities.contains(students_timetable_weekly[subgroup][k][j])))
												allActivities<<students_timetable_weekly[subgroup][k][j];
							//}
							if(!subgroupNotAvailableDayHour[subgroup][k][j])
								isNotAvailable=false;
						}
						assert(!allActivities.isEmpty());
						bool activitiesWithSameStartingtime=addActivitiesWithSameStartingTime(allActivities, j);
						if(allActivities.size()==1 && !activitiesWithSameStartingtime){  // because i am using colspan or rowspan!!!
							tos<<writeActivityStudents(allActivities[0], k, j, isNotAvailable, true, false);
						} else{
							if(PRINT_DETAILED==false) tos<<"          <td>"<<protect2(STRING_SEVERAL_ACTIVITIES_IN_LESS_DETAILED_TABLES)<<"</td>\n";
							else{
								tos<<writeActivitiesStudents(allActivities);
							}
						}
					}
				}
				tos<<"        </tr>\n";
			}
		}
		//workaround begin. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
		tos<<"        <tr class=\"foot\"><td></td><td colspan=\""<<gt.rules.nDaysPerWeek*gt.rules.nHoursPerDay<<"\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr>\n";
		//workaround end.
		tos<<"      </tbody>\n";
		tos<<"    </table>\n\n";
		tos<<"    <p class=\"back\"><a href=\""<<"#top\">"<<TimetableExport::tr("back to the top")<<"</a></p>\n\n";
		if(PRINT_DETAILED==true) PRINT_DETAILED=false;
		else PRINT_DETAILED=true;
	} while(PRINT_DETAILED!=true);
	tos << "  </body>\n</html>\n";

	if(file.error()>0){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Writing %1 gave error code %2, which means saving is compromised. Please check your disk's free space.").arg(htmlfilename).arg(file.error()));
	}
	file.close();
}


//XHTML generation code by Volker Dirr (http://timetabling.de/)
void TimetableExport::writeGroupsTimetableTimeVerticalDailyHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities){
	assert(gt.rules.initialized && gt.rules.internalStructureComputed);
	//assert(gt.timePopulation.initialized);
	assert(students_schedule_ready && teachers_schedule_ready && rooms_schedule_ready);

	//Now we print the results to an HTML file
	QFile file(htmlfilename);
	if(!file.open(QIODevice::WriteOnly)){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Cannot open file %1 for writing. Please check your disk's free space. Saving of %1 aborted.").arg(htmlfilename));
		return;
		assert(0);
	}
	QTextStream tos(&file);
	tos.setCodec("UTF-8");
	tos.setGenerateByteOrderMark(true);

	tos<<writeHead(true, placedActivities, true);
	tos<<writeTOCDays(true);

	bool PRINT_DETAILED=true;
	do{
		for(int k=0; k<gt.rules.nDaysPerWeek; k++){
			tos<<"    <table id=\"table_"<<hashDayIDsTimetable.value(gt.rules.daysOfTheWeek[k]);
			if(PRINT_DETAILED==true) tos<<"_DETAILED";
			tos<<"\" border=\"1\">\n";
	
			tos<<"      <caption>"<<protect2(gt.rules.institutionName)<<"</caption>\n";
	
			tos<<"      <thead>\n        <tr><td colspan=\"2\"></td>";
			int group=0;
			for(int i=0; i<gt.rules.augmentedYearsList.size(); i++){
				StudentsYear* sty=gt.rules.augmentedYearsList[i];
				for(int g=0; g<sty->groupsList.size(); g++){
					group++;
					StudentsGroup* stg=sty->groupsList[g];
					if(TIMETABLE_HTML_LEVEL>=2)
						tos<<"          <th class=\"xAxis\">";
					else
						tos<<"          <th>";
					tos << stg->name << "</th>";
				}
			}
			
			tos<<"</tr>\n      </thead>\n";
			/*workaround. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
			tos<<"      <tfoot><tr><td colspan=\"2\"></td><td colspan=\""<<group<<"\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr></tfoot>\n";
			*/
			tos<<"      <tbody>\n";

			for(int j=0; j<gt.rules.nHoursPerDay; j++){
				tos << "        <tr>\n";
				if(j==0)
					tos << "        <th rowspan=\"" << gt.rules.nHoursPerDay  << "\">" << protect2vert(gt.rules.daysOfTheWeek[k]) << "</th>\n";
				else tos <<"          <!-- span -->\n";
				if(TIMETABLE_HTML_LEVEL>=2)
					tos<<"          <th class=\"yAxis\">";
				else
					tos<<"          <th>";
				tos << protect2(gt.rules.hoursOfTheDay[j]) << "</th>\n";
				for(int i=0; i<gt.rules.augmentedYearsList.size(); i++){
					StudentsYear* sty=gt.rules.augmentedYearsList[i];
					for(int g=0; g<sty->groupsList.size(); g++){
						StudentsGroup* stg=sty->groupsList[g];
						QList<int> allActivities;
						allActivities.clear();
						bool isNotAvailable=true;
						for(int sg=0; sg<stg->subgroupsList.size(); sg++){
							StudentsSubgroup* sts=stg->subgroupsList[sg];
							//old code by Volker
							/*
							for(int subgroup=0; subgroup<gt.rules.nInternalSubgroups; subgroup++){
								QString subgroup_name = gt.rules.internalSubgroupsList[subgroup]->name;
								if(subgroup_name==sts->name)
							*/
							//speed improvement by Liviu
							int subgroup=sts->indexInInternalSubgroupsList;
									if(!(allActivities.contains(students_timetable_weekly[subgroup][k][j])))
												allActivities<<students_timetable_weekly[subgroup][k][j];
							//}
							if(!subgroupNotAvailableDayHour[subgroup][k][j])
								isNotAvailable=false;
						}
						assert(!allActivities.isEmpty());
						bool activitiesWithSameStartingtime=addActivitiesWithSameStartingTime(allActivities, j);
						if(allActivities.size()==1 && !activitiesWithSameStartingtime){  // because i am using colspan or rowspan!!!
							tos<<writeActivityStudents(allActivities[0], k, j, isNotAvailable, false, true);
						} else{
							if(PRINT_DETAILED==false) tos<<"          <td>"<<protect2(STRING_SEVERAL_ACTIVITIES_IN_LESS_DETAILED_TABLES)<<"</td>\n";
							else{
								tos<<writeActivitiesStudents(allActivities);
							}
						}
					}
				}
				tos<<"        </tr>\n";
			}
			//workaround begin. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
			tos<<"        <tr class=\"foot\"><td colspan=\"2\"></td><td colspan=\""<<group<<"\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr>\n";
			//workaround end.
			tos<<"      </tbody>\n";
			tos<<"    </table>\n\n";
			tos<<"    <p class=\"back\"><a href=\""<<"#top\">"<<TimetableExport::tr("back to the top")<<"</a></p>\n\n";
		}
		if(PRINT_DETAILED==true) PRINT_DETAILED=false;
		else PRINT_DETAILED=true;
	} while(PRINT_DETAILED!=true);
	tos << "  </body>\n</html>\n";

	if(file.error()>0){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Writing %1 gave error code %2, which means saving is compromised. Please check your disk's free space.").arg(htmlfilename).arg(file.error()));
	}
	file.close();
}

//XHTML generation code by Volker Dirr (http://timetabling.de/)
void TimetableExport::writeGroupsTimetableTimeHorizontalDailyHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities){
	assert(gt.rules.initialized && gt.rules.internalStructureComputed);
	//assert(gt.timePopulation.initialized);
	assert(students_schedule_ready && teachers_schedule_ready && rooms_schedule_ready);

	//Now we print the results to an HTML file
	QFile file(htmlfilename);
	if(!file.open(QIODevice::WriteOnly)){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Cannot open file %1 for writing. Please check your disk's free space. Saving of %1 aborted.").arg(htmlfilename));
		return;
		assert(0);
	}
	QTextStream tos(&file);
	tos.setCodec("UTF-8");
	tos.setGenerateByteOrderMark(true);

	tos<<writeHead(true, placedActivities, true);
	tos<<writeTOCDays(true);
	
	bool PRINT_DETAILED=true;
	do{
		for(int k=0; k<gt.rules.nDaysPerWeek; k++){
			tos<<"    <table id=\"table_"<<hashDayIDsTimetable.value(gt.rules.daysOfTheWeek[k]);
			if(PRINT_DETAILED==true) tos<<"_DETAILED";
			tos<<"\" border=\"1\">\n";
	
			tos<<"      <caption>"<<protect2(gt.rules.institutionName)<<"</caption>\n";
	
			tos<<"      <thead>\n        <tr><td rowspan=\"2\"></td>";
			tos << "<th colspan=\"" << gt.rules.nHoursPerDay <<"\">" << protect2(gt.rules.daysOfTheWeek[k]) << "</th>";
			tos <<"</tr>\n";
			tos<<"        <tr>\n          <!-- span -->\n";
			for(int j=0; j<gt.rules.nHoursPerDay; j++){
				if(TIMETABLE_HTML_LEVEL>=2)
					tos<<"          <th class=\"xAxis\">";
				else
					tos<<"          <th>";
				tos << protect2(gt.rules.hoursOfTheDay[j]) << "</th>\n";
			}
			tos<<"        </tr>\n";
			tos<<"      </thead>\n";
			/*workaround. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
			tos<<"      <tfoot><tr><td colspan=\"2\"></td><td colspan=\""<<gt.rules.nHoursPerDay<<"\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr></tfoot>\n";
			*/
			tos<<"      <tbody>\n";
			
			for(int i=0; i<gt.rules.augmentedYearsList.size(); i++){
				StudentsYear* sty=gt.rules.augmentedYearsList[i];
				for(int g=0; g<sty->groupsList.size(); g++){
					StudentsGroup* stg=sty->groupsList[g];
					tos << "        <tr>\n";
					if(TIMETABLE_HTML_LEVEL>=2)
						tos<<"          <th class=\"yAxis\">";
					else
						tos<<"          <th>";
					tos << protect2(stg->name) << "</th>\n";
				
					for(int j=0; j<gt.rules.nHoursPerDay; j++){
						QList<int> allActivities;
						allActivities.clear();
						bool isNotAvailable=true;
						for(int sg=0; sg<stg->subgroupsList.size(); sg++){
							StudentsSubgroup* sts=stg->subgroupsList[sg];
							//old code by Volker
							/*
							for(int subgroup=0; subgroup<gt.rules.nInternalSubgroups; subgroup++){
								QString subgroup_name = gt.rules.internalSubgroupsList[subgroup]->name;
								if(subgroup_name==sts->name)
							*/
							//speed improvement by Liviu
							int subgroup=sts->indexInInternalSubgroupsList;
									if(!(allActivities.contains(students_timetable_weekly[subgroup][k][j])))
												allActivities<<students_timetable_weekly[subgroup][k][j];
							//}
							if(!subgroupNotAvailableDayHour[subgroup][k][j])
								isNotAvailable=false;
						}
						assert(!allActivities.isEmpty());
						bool activitiesWithSameStartingtime=addActivitiesWithSameStartingTime(allActivities, j);
						if(allActivities.size()==1 && !activitiesWithSameStartingtime){  // because i am using colspan or rowspan!!!
							tos<<writeActivityStudents(allActivities[0], k, j, isNotAvailable, true, false);
						} else{
							if(PRINT_DETAILED==false) tos<<"          <td>"<<protect2(STRING_SEVERAL_ACTIVITIES_IN_LESS_DETAILED_TABLES)<<"</td>\n";
							else{
								tos<<writeActivitiesStudents(allActivities);
							}
						}
					}
					tos<<"        </tr>\n";
				}
			}
			//workaround begin. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
			tos<<"        <tr class=\"foot\"><td></td><td colspan=\""<<gt.rules.nHoursPerDay<<"\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr>\n";
			//workaround end.
			tos<<"      </tbody>\n";
			tos<<"    </table>\n\n";
			tos<<"    <p class=\"back\"><a href=\""<<"#top\">"<<TimetableExport::tr("back to the top")<<"</a></p>\n\n";
		}
		if(PRINT_DETAILED==true) PRINT_DETAILED=false;
		else PRINT_DETAILED=true;
	} while(PRINT_DETAILED!=true);
	tos << "  </body>\n</html>\n";

	if(file.error()>0){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Writing %1 gave error code %2, which means saving is compromised. Please check your disk's free space.").arg(htmlfilename).arg(file.error()));
	}
	file.close();
}


//Now print the years

//XHTML generation code by Volker Dirr (http://timetabling.de/)
void TimetableExport::writeYearsTimetableDaysHorizontalHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities){
	assert(gt.rules.initialized && gt.rules.internalStructureComputed);
	//assert(gt.timePopulation.initialized);
	assert(students_schedule_ready && teachers_schedule_ready && rooms_schedule_ready);

	//Now we print the results to an HTML file
	QFile file(htmlfilename);
	if(!file.open(QIODevice::WriteOnly)){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Cannot open file %1 for writing. Please check your disk's free space. Saving of %1 aborted.").arg(htmlfilename));
		return;
		assert(0);
	}
	QTextStream tos(&file);
	tos.setCodec("UTF-8");
	tos.setGenerateByteOrderMark(true);

	tos<<writeHead(true, placedActivities, true);
		
	tos<<"    <p><strong>"<<TimetableExport::tr("Table of contents")<<"</strong></p>\n";
	tos<<"    <ul>\n";
	for(int i=0; i<gt.rules.augmentedYearsList.size(); i++){
		StudentsYear* sty=gt.rules.augmentedYearsList[i];
		tos<<"      <li>\n        "<<TimetableExport::tr("Year");
		tos<<" <a href=\""<<"#table_"<<hashStudentIDsTimetable.value(sty->name)<<"_DETAILED\">"<<protect2(sty->name)<<" ("<<tr("Detailed")<<")</a> /";
		tos<<" <a href=\""<<"#table_"<<hashStudentIDsTimetable.value(sty->name)<<"\">"<<protect2(sty->name)<<" ("<<tr("Less detailed")<<")</a>\n";
		tos<<"      </li>\n";
	}
	tos<<"    </ul>\n    <p>&nbsp;</p>\n\n";

	bool PRINT_DETAILED=true;
	do{
		for(int year=0; year<gt.rules.augmentedYearsList.size(); year++){
			StudentsYear* sty=gt.rules.augmentedYearsList[year];
				tos<<"    <table id=\"table_"<<hashStudentIDsTimetable.value(sty->name);
				if(PRINT_DETAILED==true) tos<<"_DETAILED";
				tos<<"\" border=\"1\"";
				if(year%2==0)  tos<<" class=\"odd_table\"";
				else tos<<" class=\"even_table\"";
				tos<<">\n";
						
				tos<<"      <caption>"<<protect2(gt.rules.institutionName)<<"</caption>\n";

				tos<<"      <thead>\n        <tr><td rowspan=\"2\"></td><th colspan=\""<<gt.rules.nDaysPerWeek<<"\">"<<protect2(sty->name)<<"</th></tr>\n";
				tos<<"        <tr>\n          <!-- span -->\n";
				for(int j=0; j<gt.rules.nDaysPerWeek; j++){
					if(TIMETABLE_HTML_LEVEL>=2)
						tos<<"          <th class=\"xAxis\">";
					else
						tos<<"          <th>";
					tos<<protect2(gt.rules.daysOfTheWeek[j])<<"</th>\n";
				}
				tos<<"        </tr>\n";
				tos<<"      </thead>\n";
				/*workaround. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
				tos<<"      <tfoot><tr><td></td><td colspan=\""<<gt.rules.nDaysPerWeek<<"\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr></tfoot>\n";
				*/
				tos<<"      <tbody>\n";
				for(int j=0; j<gt.rules.nHoursPerDay; j++){
					tos<<"        <tr>\n";
					if(TIMETABLE_HTML_LEVEL>=2)
						tos<<"          <th class=\"yAxis\">";
					else
						tos<<"          <th>";
					tos<<protect2(gt.rules.hoursOfTheDay[j])<<"</th>\n";
					for(int k=0; k<gt.rules.nDaysPerWeek; k++){
						QList<int> allActivities;
						allActivities.clear();
						bool isNotAvailable=true;
						for(int g=0; g<sty->groupsList.size(); g++){
							StudentsGroup* stg=sty->groupsList[g];
							for(int sg=0; sg<stg->subgroupsList.size(); sg++){
								StudentsSubgroup* sts=stg->subgroupsList[sg];
								//old code by Volker
								/*
								for(int subgroup=0; subgroup<gt.rules.nInternalSubgroups; subgroup++){
									QString subgroup_name = gt.rules.internalSubgroupsList[subgroup]->name;
									if(subgroup_name==sts->name)
								*/
								//speed improvement by Liviu
								int subgroup=sts->indexInInternalSubgroupsList;
										if(!(allActivities.contains(students_timetable_weekly[subgroup][k][j])))
													allActivities<<students_timetable_weekly[subgroup][k][j];
								//}
								if(!subgroupNotAvailableDayHour[subgroup][k][j])
									isNotAvailable=false;
							}
						}
						assert(!allActivities.isEmpty());
						bool activitiesWithSameStartingtime=addActivitiesWithSameStartingTime(allActivities, j);
						if(allActivities.size()==1 && !activitiesWithSameStartingtime){  // because i am using colspan or rowspan!!!
							tos<<writeActivityStudents(allActivities[0], k, j, isNotAvailable, false, true);
						} else{
							if(PRINT_DETAILED==false) tos<<"          <td>"<<protect2(STRING_SEVERAL_ACTIVITIES_IN_LESS_DETAILED_TABLES)<<"</td>\n";
							else{
								tos<<writeActivitiesStudents(allActivities);
							}
						}
					}
					tos<<"        </tr>\n";
				}
				//workaround begin. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
				tos<<"        <tr class=\"foot\"><td></td><td colspan=\""<<gt.rules.nDaysPerWeek<<"\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr>\n";
				//workaround end.
				tos<<"      </tbody>\n";
				tos<<"    </table>\n\n";
				tos<<"    <p class=\"back\"><a href=\""<<"#top\">"<<TimetableExport::tr("back to the top")<<"</a></p>\n\n";
		}
		if(PRINT_DETAILED==true) PRINT_DETAILED=false;
		else PRINT_DETAILED=true;
	} while(PRINT_DETAILED!=true);
	tos<<"  </body>\n</html>\n";

	if(file.error()>0){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Writing %1 gave error code %2, which means saving is compromised. Please check your disk's free space.").arg(htmlfilename).arg(file.error()));
	}
	file.close();
}


//XHTML generation code by Volker Dirr (http://timetabling.de/)
void TimetableExport::writeYearsTimetableDaysVerticalHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities){
	assert(gt.rules.initialized && gt.rules.internalStructureComputed);
	//assert(gt.timePopulation.initialized);
	assert(students_schedule_ready && teachers_schedule_ready && rooms_schedule_ready);

	//Now we print the results to an HTML file
	QFile file(htmlfilename);
	if(!file.open(QIODevice::WriteOnly)){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Cannot open file %1 for writing. Please check your disk's free space. Saving of %1 aborted.").arg(htmlfilename));
		return;
		assert(0);
	}
	QTextStream tos(&file);
	tos.setCodec("UTF-8");
	tos.setGenerateByteOrderMark(true);

	tos<<writeHead(true, placedActivities, true);
		
	tos<<"    <p><strong>"<<TimetableExport::tr("Table of contents")<<"</strong></p>\n";
	tos<<"    <ul>\n";
	for(int i=0; i<gt.rules.augmentedYearsList.size(); i++){
		StudentsYear* sty=gt.rules.augmentedYearsList[i];
		tos<<"      <li>\n        "<<TimetableExport::tr("Year");
		tos<<" <a href=\""<<"#table_"<<hashStudentIDsTimetable.value(sty->name)<<"_DETAILED\">"<<protect2(sty->name)<<" ("<<tr("Detailed")<<")</a> /";
		tos<<" <a href=\""<<"#table_"<<hashStudentIDsTimetable.value(sty->name)<<"\">"<<protect2(sty->name)<<" ("<<tr("Less detailed")<<")</a>\n";
		tos<<"      </li>\n";
	}
	tos<<"    </ul>\n    <p>&nbsp;</p>\n\n";

	bool PRINT_DETAILED=true;
	do{
		for(int year=0; year<gt.rules.augmentedYearsList.size(); year++){
				StudentsYear* sty=gt.rules.augmentedYearsList[year];
				tos<<"    <table id=\"table_"<<hashStudentIDsTimetable.value(sty->name);
				if(PRINT_DETAILED==true) tos<<"_DETAILED";
				tos<<"\" border=\"1\"";
				if(year%2==0)  tos<<" class=\"odd_table\"";
				else tos<<" class=\"even_table\"";
				tos<<">\n";
						
				tos<<"      <caption>"<<protect2(gt.rules.institutionName)<<"</caption>\n";

				tos<<"      <thead>\n        <tr><td rowspan=\"2\"></td><th colspan=\""<<gt.rules.nHoursPerDay<<"\">"<<protect2(sty->name)<<"</th></tr>\n";
				tos<<"        <tr>\n          <!-- span -->\n";
				for(int j=0; j<gt.rules.nHoursPerDay; j++){
					if(TIMETABLE_HTML_LEVEL>=2)
						tos<<"          <th class=\"xAxis\">";
					else
						tos<<"          <th>";
					tos<<protect2(gt.rules.hoursOfTheDay[j])<<"</th>\n";
				}
				tos<<"        </tr>\n";
				tos<<"      </thead>\n";
				/*workaround. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
				tos<<"      <tfoot><tr><td></td><td colspan=\""<<gt.rules.nHoursPerDay<<"\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr></tfoot>\n";
				*/
				tos<<"      <tbody>\n";
				for(int k=0; k<gt.rules.nDaysPerWeek; k++){
					tos<<"        <tr>\n";
					if(TIMETABLE_HTML_LEVEL>=2)
						tos<<"          <th class=\"yAxis\">";
					else
						tos<<"          <th>";
					tos<<protect2(gt.rules.daysOfTheWeek[k])<<"</th>\n";

					for(int j=0; j<gt.rules.nHoursPerDay; j++){
						QList<int> allActivities;
						allActivities.clear();
						bool isNotAvailable=true;
						for(int g=0; g<sty->groupsList.size(); g++){
							StudentsGroup* stg=sty->groupsList[g];
							for(int sg=0; sg<stg->subgroupsList.size(); sg++){
								StudentsSubgroup* sts=stg->subgroupsList[sg];
								//old code by Volker
								/*
								for(int subgroup=0; subgroup<gt.rules.nInternalSubgroups; subgroup++){
									QString subgroup_name = gt.rules.internalSubgroupsList[subgroup]->name;
									if(subgroup_name==sts->name)
								*/
								//speed improvement by Liviu
								int subgroup=sts->indexInInternalSubgroupsList;
										if(!(allActivities.contains(students_timetable_weekly[subgroup][k][j])))
													allActivities<<students_timetable_weekly[subgroup][k][j];
								//}
								if(!subgroupNotAvailableDayHour[subgroup][k][j])
									isNotAvailable=false;
							}
						}
						assert(!allActivities.isEmpty());
						bool activitiesWithSameStartingtime=addActivitiesWithSameStartingTime(allActivities, j);
						if(allActivities.size()==1 && !activitiesWithSameStartingtime){  // because i am using colspan or rowspan!!!
							tos<<writeActivityStudents(allActivities[0], k, j, isNotAvailable, true, false);
						} else{
							if(PRINT_DETAILED==false) tos<<"          <td>"<<protect2(STRING_SEVERAL_ACTIVITIES_IN_LESS_DETAILED_TABLES)<<"</td>\n";
							else{
								tos<<writeActivitiesStudents(allActivities);
							}
						}
					}
					tos<<"        </tr>\n";
				}
				//workaround begin. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
				tos<<"        <tr class=\"foot\"><td></td><td colspan=\""<<gt.rules.nHoursPerDay<<"\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr>\n";
				//workaround end.
				tos<<"      </tbody>\n";
				tos<<"    </table>\n\n";
				tos<<"    <p class=\"back\"><a href=\""<<"#top\">"<<TimetableExport::tr("back to the top")<<"</a></p>\n\n";
		}
		if(PRINT_DETAILED==true) PRINT_DETAILED=false;
		else PRINT_DETAILED=true;
	} while(PRINT_DETAILED!=true);
	tos<<"  </body>\n</html>\n";

	if(file.error()>0){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Writing %1 gave error code %2, which means saving is compromised. Please check your disk's free space.").arg(htmlfilename).arg(file.error()));
	}
	file.close();
}


//XHTML generation code by Volker Dirr (http://timetabling.de/)
void TimetableExport::writeYearsTimetableTimeVerticalHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities){
	assert(gt.rules.initialized && gt.rules.internalStructureComputed);
	//assert(gt.timePopulation.initialized);
	assert(students_schedule_ready && teachers_schedule_ready && rooms_schedule_ready);

	//Now we print the results to an HTML file
	QFile file(htmlfilename);
	if(!file.open(QIODevice::WriteOnly)){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Cannot open file %1 for writing. Please check your disk's free space. Saving of %1 aborted.").arg(htmlfilename));
		return;
		assert(0);
	}
	QTextStream tos(&file);
	tos.setCodec("UTF-8");
	tos.setGenerateByteOrderMark(true);

	tos<<writeHead(true, placedActivities, false);
	
	bool PRINT_DETAILED=true;
	do{
		tos<<"    <table id=\"table";
		if(PRINT_DETAILED==false)
			tos<<"_LESS";
		tos<<"_DETAILED";
		tos<<"\" border=\"1\">\n";

		tos<<"      <caption>"<<protect2(gt.rules.institutionName)<<"</caption>\n";

		tos<<"      <thead>\n        <tr><td colspan=\"2\"></td>";
		for(int i=0; i<gt.rules.augmentedYearsList.size(); i++){
			StudentsYear* sty=gt.rules.augmentedYearsList[i];
				if(TIMETABLE_HTML_LEVEL>=2)
					tos<<"          <th class=\"xAxis\">";
				else
					tos<<"          <th>";
				tos << sty->name << "</th>";
		}
		
		tos<<"</tr>\n      </thead>\n";
		/*workaround. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
		tos<<"      <tfoot><tr><td colspan=\"2\"></td><td colspan=\""<<gt.rules.augmentedYearsList.size()<<"\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr></tfoot>\n";
		*/
		tos<<"      <tbody>\n";
		for(int k=0; k<gt.rules.nDaysPerWeek; k++){
			for(int j=0; j<gt.rules.nHoursPerDay; j++){
				tos << "        <tr>\n";
				if(j==0)
					tos << "        <th rowspan=\"" << gt.rules.nHoursPerDay  << "\">" << protect2vert(gt.rules.daysOfTheWeek[k]) << "</th>\n";
				else tos <<"          <!-- span -->\n";
				if(TIMETABLE_HTML_LEVEL>=2)
					tos<<"          <th class=\"yAxis\">";
				else
					tos<<"          <th>";
				tos << protect2(gt.rules.hoursOfTheDay[j]) << "</th>\n";
				for(int year=0; year<gt.rules.augmentedYearsList.size(); year++){
						StudentsYear* sty=gt.rules.augmentedYearsList[year];
						QList<int> allActivities;
						allActivities.clear();
						bool isNotAvailable=true;
						for(int g=0; g<sty->groupsList.size(); g++){
							StudentsGroup* stg=sty->groupsList[g];
							for(int sg=0; sg<stg->subgroupsList.size(); sg++){
								StudentsSubgroup* sts=stg->subgroupsList[sg];
								//old code by Volker
								/*
								for(int subgroup=0; subgroup<gt.rules.nInternalSubgroups; subgroup++){
									QString subgroup_name = gt.rules.internalSubgroupsList[subgroup]->name;
									if(subgroup_name==sts->name)
								*/
								//speed improvement by Liviu
								int subgroup=sts->indexInInternalSubgroupsList;
										if(!(allActivities.contains(students_timetable_weekly[subgroup][k][j])))
													allActivities<<students_timetable_weekly[subgroup][k][j];
								//}
								if(!subgroupNotAvailableDayHour[subgroup][k][j])
									isNotAvailable=false;
							}
						}
						assert(!allActivities.isEmpty());
						bool activitiesWithSameStartingtime=addActivitiesWithSameStartingTime(allActivities, j);
						if(allActivities.size()==1 && !activitiesWithSameStartingtime){  // because i am using colspan or rowspan!!!
							tos<<writeActivityStudents(allActivities[0], k, j, isNotAvailable, false, true);
						} else{
							if(PRINT_DETAILED==false) tos<<"          <td>"<<protect2(STRING_SEVERAL_ACTIVITIES_IN_LESS_DETAILED_TABLES)<<"</td>\n";
							else{
								tos<<writeActivitiesStudents(allActivities);
							}
					}
				}
				tos<<"        </tr>\n";
			}
		}
		//workaround begin. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
		tos<<"        <tr class=\"foot\"><td colspan=\"2\"></td><td colspan=\""<<gt.rules.augmentedYearsList.size()<<"\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr>\n";
		//workaround end.
		tos<<"      </tbody>\n";
		tos<<"    </table>\n\n";
		tos<<"    <p class=\"back\"><a href=\""<<"#top\">"<<TimetableExport::tr("back to the top")<<"</a></p>\n\n";
		if(PRINT_DETAILED==true) PRINT_DETAILED=false;
		else PRINT_DETAILED=true;
	} while(PRINT_DETAILED!=true);
	tos << "  </body>\n</html>\n";

	if(file.error()>0){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Writing %1 gave error code %2, which means saving is compromised. Please check your disk's free space.").arg(htmlfilename).arg(file.error()));
	}
	file.close();
}

//XHTML generation code by Volker Dirr (http://timetabling.de/)
void TimetableExport::writeYearsTimetableTimeHorizontalHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities){
	assert(gt.rules.initialized && gt.rules.internalStructureComputed);
	//assert(gt.timePopulation.initialized);
	assert(students_schedule_ready && teachers_schedule_ready && rooms_schedule_ready);

	//Now we print the results to an HTML file
	QFile file(htmlfilename);
	if(!file.open(QIODevice::WriteOnly)){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Cannot open file %1 for writing. Please check your disk's free space. Saving of %1 aborted.").arg(htmlfilename));
		return;
		assert(0);
	}
	QTextStream tos(&file);
	tos.setCodec("UTF-8");
	tos.setGenerateByteOrderMark(true);

	tos<<writeHead(true, placedActivities, false);
	
	bool PRINT_DETAILED=true;
	do{
		tos<<"    <table id=\"table";
		if(PRINT_DETAILED==false)
			tos<<"_LESS";
		tos<<"_DETAILED";
		tos<<"\" border=\"1\">\n";

		tos<<"      <caption>"<<protect2(gt.rules.institutionName)<<"</caption>\n";

		tos<<"      <thead>\n        <tr><td rowspan=\"2\"></td>";
		for(int k=0; k<gt.rules.nDaysPerWeek; k++)
			tos << "<th colspan=\"" << gt.rules.nHoursPerDay <<"\">" << protect2(gt.rules.daysOfTheWeek[k]) << "</th>";
		tos <<"</tr>\n";
		tos<<"        <tr>\n          <!-- span -->\n";
		for(int k=0; k<gt.rules.nDaysPerWeek; k++)
			for(int j=0; j<gt.rules.nHoursPerDay; j++){
				if(TIMETABLE_HTML_LEVEL>=2)
					tos<<"          <th class=\"xAxis\">";
				else
					tos<<"          <th>";
				tos << protect2(gt.rules.hoursOfTheDay[j]) << "</th>\n";
			}
		tos<<"        </tr>\n";
		tos<<"      </thead>\n";
		/*workaround. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
		tos<<"      <tfoot><tr><td colspan=\"2\"></td><td colspan=\""<<gt.rules.nInternalSubgroups<<"\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr></tfoot>\n";
		*/
		tos<<"      <tbody>\n";
		
		for(int year=0; year<gt.rules.augmentedYearsList.size(); year++){
			StudentsYear* sty=gt.rules.augmentedYearsList[year];
				tos << "        <tr>\n";
				if(TIMETABLE_HTML_LEVEL>=2)
					tos<<"          <th class=\"yAxis\">";
				else
					tos<<"          <th>";
				tos << protect2(sty->name) << "</th>\n";
				for(int k=0; k<gt.rules.nDaysPerWeek; k++){
					for(int j=0; j<gt.rules.nHoursPerDay; j++){
						QList<int> allActivities;
						allActivities.clear();
						bool isNotAvailable=true;
						for(int g=0; g<sty->groupsList.size(); g++){
							StudentsGroup* stg=sty->groupsList[g];
							for(int sg=0; sg<stg->subgroupsList.size(); sg++){
								StudentsSubgroup* sts=stg->subgroupsList[sg];
								//old code by Volker
								/*
								for(int subgroup=0; subgroup<gt.rules.nInternalSubgroups; subgroup++){
									QString subgroup_name = gt.rules.internalSubgroupsList[subgroup]->name;
									if(subgroup_name==sts->name)
								*/
								//speed improvement by Liviu
								int subgroup=sts->indexInInternalSubgroupsList;
										if(!(allActivities.contains(students_timetable_weekly[subgroup][k][j])))
													allActivities<<students_timetable_weekly[subgroup][k][j];
								//}
								if(!subgroupNotAvailableDayHour[subgroup][k][j])
									isNotAvailable=false;
							}
						}
						assert(!allActivities.isEmpty());
						bool activitiesWithSameStartingtime=addActivitiesWithSameStartingTime(allActivities, j);
						if(allActivities.size()==1 && !activitiesWithSameStartingtime){  // because i am using colspan or rowspan!!!
							tos<<writeActivityStudents(allActivities[0], k, j, isNotAvailable, true, false);
						} else{
							if(PRINT_DETAILED==false) tos<<"          <td>"<<protect2(STRING_SEVERAL_ACTIVITIES_IN_LESS_DETAILED_TABLES)<<"</td>\n";
							else{
								tos<<writeActivitiesStudents(allActivities);
							}
						}
					}
				}
				tos<<"        </tr>\n";
		}
		//workaround begin. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
		tos<<"        <tr class=\"foot\"><td></td><td colspan=\""<<gt.rules.nHoursPerDay*gt.rules.nDaysPerWeek<<"\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr>\n";
		//workaround end.
		tos<<"      </tbody>\n";
		tos<<"    </table>\n\n";
		tos<<"    <p class=\"back\"><a href=\""<<"#top\">"<<TimetableExport::tr("back to the top")<<"</a></p>\n\n";
		if(PRINT_DETAILED==true) PRINT_DETAILED=false;
		else PRINT_DETAILED=true;
	} while(PRINT_DETAILED!=true);
	tos << "  </body>\n</html>\n";

	if(file.error()>0){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Writing %1 gave error code %2, which means saving is compromised. Please check your disk's free space.").arg(htmlfilename).arg(file.error()));
	}
	file.close();
}


//XHTML generation code by Volker Dirr (http://timetabling.de/)
void TimetableExport::writeYearsTimetableTimeVerticalDailyHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities){
	assert(gt.rules.initialized && gt.rules.internalStructureComputed);
	//assert(gt.timePopulation.initialized);
	assert(students_schedule_ready && teachers_schedule_ready && rooms_schedule_ready);

	//Now we print the results to an HTML file
	QFile file(htmlfilename);
	if(!file.open(QIODevice::WriteOnly)){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Cannot open file %1 for writing. Please check your disk's free space. Saving of %1 aborted.").arg(htmlfilename));
		return;
		assert(0);
	}
	QTextStream tos(&file);
	tos.setCodec("UTF-8");
	tos.setGenerateByteOrderMark(true);

	tos<<writeHead(true, placedActivities, true);
	tos<<writeTOCDays(true);

	bool PRINT_DETAILED=true;
	do{
		for(int k=0; k<gt.rules.nDaysPerWeek; k++){
			tos<<"    <table id=\"table_"<<hashDayIDsTimetable.value(gt.rules.daysOfTheWeek[k]);
			if(PRINT_DETAILED==true) tos<<"_DETAILED";
			tos<<"\" border=\"1\">\n";
	
			tos<<"      <caption>"<<protect2(gt.rules.institutionName)<<"</caption>\n";
	
			tos<<"      <thead>\n        <tr><td colspan=\"2\"></td>";
			for(int i=0; i<gt.rules.augmentedYearsList.size(); i++){
				StudentsYear* sty=gt.rules.augmentedYearsList[i];
					if(TIMETABLE_HTML_LEVEL>=2)
						tos<<"          <th class=\"xAxis\">";
					else
						tos<<"          <th>";
					tos << sty->name << "</th>";
			}
			
			tos<<"</tr>\n      </thead>\n";
			/*workaround. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
			tos<<"      <tfoot><tr><td colspan=\"2\"></td><td colspan=\""<<gt.rules.augmentedYearsList.size()<<"\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr></tfoot>\n";
			*/
			tos<<"      <tbody>\n";

			for(int j=0; j<gt.rules.nHoursPerDay; j++){
				tos << "        <tr>\n";
				if(j==0)
					tos << "        <th rowspan=\"" << gt.rules.nHoursPerDay  << "\">" << protect2vert(gt.rules.daysOfTheWeek[k]) << "</th>\n";
				else tos <<"          <!-- span -->\n";
				if(TIMETABLE_HTML_LEVEL>=2)
					tos<<"          <th class=\"yAxis\">";
				else
					tos<<"          <th>";
				tos << protect2(gt.rules.hoursOfTheDay[j]) << "</th>\n";
				for(int year=0; year<gt.rules.augmentedYearsList.size(); year++){
						StudentsYear* sty=gt.rules.augmentedYearsList[year];
						QList<int> allActivities;
						allActivities.clear();
						bool isNotAvailable=true;
						for(int g=0; g<sty->groupsList.size(); g++){
							StudentsGroup* stg=sty->groupsList[g];
							for(int sg=0; sg<stg->subgroupsList.size(); sg++){
								StudentsSubgroup* sts=stg->subgroupsList[sg];
								//old code by Volker
								/*
								for(int subgroup=0; subgroup<gt.rules.nInternalSubgroups; subgroup++){
									QString subgroup_name = gt.rules.internalSubgroupsList[subgroup]->name;
									if(subgroup_name==sts->name)
								*/
								//speed improvement by Liviu
								int subgroup=sts->indexInInternalSubgroupsList;
										if(!(allActivities.contains(students_timetable_weekly[subgroup][k][j])))
													allActivities<<students_timetable_weekly[subgroup][k][j];
								//}
								if(!subgroupNotAvailableDayHour[subgroup][k][j])
									isNotAvailable=false;
							}
						}
						assert(!allActivities.isEmpty());
						bool activitiesWithSameStartingtime=addActivitiesWithSameStartingTime(allActivities, j);
						if(allActivities.size()==1 && !activitiesWithSameStartingtime){  // because i am using colspan or rowspan!!!
							tos<<writeActivityStudents(allActivities[0], k, j, isNotAvailable, false, true);
						} else{
							if(PRINT_DETAILED==false) tos<<"          <td>"<<protect2(STRING_SEVERAL_ACTIVITIES_IN_LESS_DETAILED_TABLES)<<"</td>\n";
							else{
								tos<<writeActivitiesStudents(allActivities);
							}
					}
				}
				tos<<"        </tr>\n";
			}
			//workaround begin. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
			tos<<"        <tr class=\"foot\"><td colspan=\"2\"></td><td colspan=\""<<gt.rules.augmentedYearsList.size()<<"\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr>\n";
			//workaround end.
			tos<<"      </tbody>\n";
			tos<<"    </table>\n\n";
			tos<<"    <p class=\"back\"><a href=\""<<"#top\">"<<TimetableExport::tr("back to the top")<<"</a></p>\n\n";
		}
		if(PRINT_DETAILED==true) PRINT_DETAILED=false;
		else PRINT_DETAILED=true;
	} while(PRINT_DETAILED!=true);
	tos << "  </body>\n</html>\n";

	if(file.error()>0){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Writing %1 gave error code %2, which means saving is compromised. Please check your disk's free space.").arg(htmlfilename).arg(file.error()));
	}
	file.close();
}

//XHTML generation code by Volker Dirr (http://timetabling.de/)
void TimetableExport::writeYearsTimetableTimeHorizontalDailyHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities){
	assert(gt.rules.initialized && gt.rules.internalStructureComputed);
	//assert(gt.timePopulation.initialized);
	assert(students_schedule_ready && teachers_schedule_ready && rooms_schedule_ready);

	//Now we print the results to an HTML file
	QFile file(htmlfilename);
	if(!file.open(QIODevice::WriteOnly)){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Cannot open file %1 for writing. Please check your disk's free space. Saving of %1 aborted.").arg(htmlfilename));
		return;
		assert(0);
	}
	QTextStream tos(&file);
	tos.setCodec("UTF-8");
	tos.setGenerateByteOrderMark(true);

	tos<<writeHead(true, placedActivities, true);
	tos<<writeTOCDays(true);
	
	bool PRINT_DETAILED=true;
	do{
		for(int k=0; k<gt.rules.nDaysPerWeek; k++){
			tos<<"    <table id=\"table_"<<hashDayIDsTimetable.value(gt.rules.daysOfTheWeek[k]);
			if(PRINT_DETAILED==true) tos<<"_DETAILED";
			tos<<"\" border=\"1\">\n";
	
			tos<<"      <caption>"<<protect2(gt.rules.institutionName)<<"</caption>\n";
	
			tos<<"      <thead>\n        <tr><td rowspan=\"2\"></td>";

			tos << "<th colspan=\"" << gt.rules.nHoursPerDay <<"\">" << protect2(gt.rules.daysOfTheWeek[k]) << "</th>";
			tos <<"</tr>\n";
			tos<<"        <tr>\n          <!-- span -->\n";
			for(int j=0; j<gt.rules.nHoursPerDay; j++){
				if(TIMETABLE_HTML_LEVEL>=2)
					tos<<"          <th class=\"xAxis\">";
				else
					tos<<"          <th>";
				tos << protect2(gt.rules.hoursOfTheDay[j]) << "</th>\n";
			}
			tos<<"        </tr>\n";
			tos<<"      </thead>\n";
			/*workaround. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
			tos<<"      <tfoot><tr><td colspan=\"2\"></td><td colspan=\""<<gt.rules.nHoursPerDay<<"\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr></tfoot>\n";
			*/
			tos<<"      <tbody>\n";
			
			for(int year=0; year<gt.rules.augmentedYearsList.size(); year++){
				StudentsYear* sty=gt.rules.augmentedYearsList[year];
					tos << "        <tr>\n";
					if(TIMETABLE_HTML_LEVEL>=2)
						tos<<"          <th class=\"yAxis\">";
					else
						tos<<"          <th>";
					tos << protect2(sty->name) << "</th>\n";

					for(int j=0; j<gt.rules.nHoursPerDay; j++){
						QList<int> allActivities;
						allActivities.clear();
						bool isNotAvailable=true;
						for(int g=0; g<sty->groupsList.size(); g++){
							StudentsGroup* stg=sty->groupsList[g];
							for(int sg=0; sg<stg->subgroupsList.size(); sg++){
								StudentsSubgroup* sts=stg->subgroupsList[sg];
								//old code by Volker
								/*
								for(int subgroup=0; subgroup<gt.rules.nInternalSubgroups; subgroup++){
									QString subgroup_name = gt.rules.internalSubgroupsList[subgroup]->name;
									if(subgroup_name==sts->name)
								*/
								//speed improvement by Liviu
								int subgroup=sts->indexInInternalSubgroupsList;
										if(!(allActivities.contains(students_timetable_weekly[subgroup][k][j])))
													allActivities<<students_timetable_weekly[subgroup][k][j];
								//}
								if(!subgroupNotAvailableDayHour[subgroup][k][j])
									isNotAvailable=false;
							}
						}
						assert(!allActivities.isEmpty());
						bool activitiesWithSameStartingtime=addActivitiesWithSameStartingTime(allActivities, j);
						if(allActivities.size()==1 && !activitiesWithSameStartingtime){  // because i am using colspan or rowspan!!!
							tos<<writeActivityStudents(allActivities[0], k, j, isNotAvailable, true, false);
						} else{
							if(PRINT_DETAILED==false) tos<<"          <td>"<<protect2(STRING_SEVERAL_ACTIVITIES_IN_LESS_DETAILED_TABLES)<<"</td>\n";
							else{
								tos<<writeActivitiesStudents(allActivities);
							}
						}
					}
					tos<<"        </tr>\n";
				}
			//workaround begin. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
			tos<<"        <tr class=\"foot\"><td></td><td colspan=\""<<gt.rules.nHoursPerDay<<"\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr>\n";
			//workaround end.
			tos<<"      </tbody>\n";
			tos<<"    </table>\n\n";
			tos<<"    <p class=\"back\"><a href=\""<<"#top\">"<<TimetableExport::tr("back to the top")<<"</a></p>\n\n";
		}
		if(PRINT_DETAILED==true) PRINT_DETAILED=false;
		else PRINT_DETAILED=true;
	} while(PRINT_DETAILED!=true);
	tos << "  </body>\n</html>\n";

	if(file.error()>0){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Writing %1 gave error code %2, which means saving is compromised. Please check your disk's free space.").arg(htmlfilename).arg(file.error()));
	}
	file.close();
}


//***********************************************************************************************************************************************************************************************

//Now print all activities

//XHTML generation code by Volker Dirr (http://timetabling.de/)
void TimetableExport::writeAllActivitiesTimetableDaysHorizontalHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities){
	assert(gt.rules.initialized && gt.rules.internalStructureComputed);
	//assert(gt.timePopulation.initialized);
	assert(students_schedule_ready && teachers_schedule_ready && rooms_schedule_ready);

	//Now we print the results to an HTML file
	QFile file(htmlfilename);
	if(!file.open(QIODevice::WriteOnly)){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Cannot open file %1 for writing. Please check your disk's free space. Saving of %1 aborted.").arg(htmlfilename));
		return;
		assert(0);
	}
	QTextStream tos(&file);
	tos.setCodec("UTF-8");
	tos.setGenerateByteOrderMark(true);

	tos<<writeHead(true, placedActivities, true);

	tos<<"    <table border=\"1\">\n";	
	tos<<"      <caption>"<<protect2(gt.rules.institutionName)<<"</caption>\n";
	tos<<"      <thead>\n        <tr><td rowspan=\"2\"></td><th colspan=\""<<gt.rules.nDaysPerWeek<<"\">"<<tr("All Activities")<<"</th></tr>\n";
	tos<<"        <tr>\n          <!-- span -->\n";
	for(int j=0; j<gt.rules.nDaysPerWeek; j++){
		if(TIMETABLE_HTML_LEVEL>=2)
			tos<<"          <th class=\"xAxis\">";
		else
			tos<<"          <th>";
		tos<<protect2(gt.rules.daysOfTheWeek[j])<<"</th>\n";
	}
	tos<<"        </tr>\n";
	tos<<"      </thead>\n";
	/*workaround. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
	tos<<"      <tfoot><tr><td></td><td colspan=\""<<gt.rules.nDaysPerWeek<<"\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr></tfoot>\n";
	*/
	tos<<"      <tbody>\n";
	for(int j=0; j<gt.rules.nHoursPerDay; j++){
		tos<<"        <tr>\n";
		if(TIMETABLE_HTML_LEVEL>=2)
			tos<<"          <th class=\"yAxis\">";
		else
			tos<<"          <th>";
		tos<<protect2(gt.rules.hoursOfTheDay[j])<<"</th>\n";
		for(int k=0; k<gt.rules.nDaysPerWeek; k++){
			if(activitiesAtTime[k][j].isEmpty()){
				if(breakDayHour[k][j] && PRINT_BREAK_TIME_SLOTS){
					tos<<writeBreakSlot("");
				} else {
					tos<<writeEmpty();
				}
			} else {
				tos<<writeActivitiesStudents(activitiesAtTime[k][j]);
			}
		}
		tos<<"        </tr>\n";
	}
	//workaround begin. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
	tos<<"      <tr class=\"foot\"><td></td><td colspan=\""<<gt.rules.nDaysPerWeek<<"\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr>\n";
	//workaround end.
	tos<<"      </tbody>\n";
	tos<<"    </table>\n";
	tos<<"  </body>\n</html>\n";

	if(file.error()>0){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Writing %1 gave error code %2, which means saving is compromised. Please check your disk's free space.").arg(htmlfilename).arg(file.error()));
	}
	file.close();
}


//XHTML generation code by Volker Dirr (http://timetabling.de/)
void TimetableExport::writeAllActivitiesTimetableDaysVerticalHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities){
	assert(gt.rules.initialized && gt.rules.internalStructureComputed);
	//assert(gt.timePopulation.initialized);
	assert(students_schedule_ready && teachers_schedule_ready && rooms_schedule_ready);

	//Now we print the results to an HTML file
	QFile file(htmlfilename);
	if(!file.open(QIODevice::WriteOnly)){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Cannot open file %1 for writing. Please check your disk's free space. Saving of %1 aborted.").arg(htmlfilename));
		return;
		assert(0);
	}
	QTextStream tos(&file);
	tos.setCodec("UTF-8");
	tos.setGenerateByteOrderMark(true);

	tos<<writeHead(true, placedActivities, true);

	tos<<"    <table border=\"1\">\n";
	tos<<"      <caption>"<<protect2(gt.rules.institutionName)<<"</caption>\n";
	tos<<"      <thead>\n        <tr><td rowspan=\"2\"></td><th colspan=\""<<gt.rules.nHoursPerDay<<"\">"<<tr("All Activities")<<"</th></tr>\n";
	tos<<"        <tr>\n          <!-- span -->\n";
	for(int j=0; j<gt.rules.nHoursPerDay; j++){
		if(TIMETABLE_HTML_LEVEL>=2)
			tos<<"          <th class=\"xAxis\">";
		else
			tos<<"          <th>";
		tos<<protect2(gt.rules.hoursOfTheDay[j])<<"</th>\n";
	}
	tos<<"        </tr>\n";
	tos<<"      </thead>\n";
	/*workaround. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
	tos<<"      <tfoot><tr><td></td><td colspan=\""<<gt.rules.nHoursPerDay<<"\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr></tfoot>\n";
	*/
	tos<<"      <tbody>\n";
	for(int k=0; k<gt.rules.nDaysPerWeek; k++){
		tos<<"        <tr>\n";
		if(TIMETABLE_HTML_LEVEL>=2)
			tos<<"          <th class=\"yAxis\">";
		else
			tos<<"          <th>";
		tos<<protect2(gt.rules.daysOfTheWeek[k])<<"</th>\n";
		for(int j=0; j<gt.rules.nHoursPerDay; j++){
			if(activitiesAtTime[k][j].isEmpty()){
				if(breakDayHour[k][j] && PRINT_BREAK_TIME_SLOTS){
					tos<<writeBreakSlot("");
				} else {
					tos<<writeEmpty();
				}
			} else {
				tos<<writeActivitiesStudents(activitiesAtTime[k][j]);
			}
		}
		tos<<"        </tr>\n";
	}
	//workaround begin. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
	tos<<"      <tr class=\"foot\"><td></td><td colspan=\""<<gt.rules.nHoursPerDay<<"\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr>\n";
	//workaround end.
	tos<<"      </tbody>\n";
	tos<<"    </table>\n";
	tos<<"  </body>\n</html>\n";

	if(file.error()>0){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Writing %1 gave error code %2, which means saving is compromised. Please check your disk's free space.").arg(htmlfilename).arg(file.error()));
	}
	file.close();
}


//XHTML generation code by Volker Dirr (http://timetabling.de/)
void TimetableExport::writeAllActivitiesTimetableTimeVerticalHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities){
	assert(gt.rules.initialized && gt.rules.internalStructureComputed);
	//assert(gt.timePopulation.initialized);
	assert(students_schedule_ready && teachers_schedule_ready && rooms_schedule_ready);

	//Now we print the results to an HTML file
	QFile file(htmlfilename);
	if(!file.open(QIODevice::WriteOnly)){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Cannot open file %1 for writing. Please check your disk's free space. Saving of %1 aborted.").arg(htmlfilename));
		return;
		assert(0);
	}
	QTextStream tos(&file);
	tos.setCodec("UTF-8");
	tos.setGenerateByteOrderMark(true);

	tos<<writeHead(true, placedActivities, false);

	tos<<"    <table border=\"1\">\n";
	tos<<"      <caption>"<<protect2(gt.rules.institutionName)<<"</caption>\n";
	tos<<"      <thead>\n        <tr><td colspan=\"2\"></td>";
	if(TIMETABLE_HTML_LEVEL>=2)
		tos<<"          <th class=\"xAxis\">";
	else
		tos<<"          <th>";
	tos << tr("All Activities"); //Liviu
	tos<<"</th></tr>\n      </thead>\n";
	/*workaround. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
	tos<<"      <tfoot><tr><td colspan=\"2\"></td><td>"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr></tfoot>\n";
	*/
	tos<<"      <tbody>\n";
	for(int k=0; k<gt.rules.nDaysPerWeek; k++){
		for(int j=0; j<gt.rules.nHoursPerDay; j++){
			tos << "        <tr>\n";
			if(j==0)
				tos << "        <th rowspan=\"" << gt.rules.nHoursPerDay  << "\">" << protect2vert(gt.rules.daysOfTheWeek[k]) << "</th>\n";
			else tos <<"          <!-- span -->\n";
			if(TIMETABLE_HTML_LEVEL>=2)
				tos<<"          <th class=\"yAxis\">";
			else
				tos<<"          <th>";
			tos << protect2(gt.rules.hoursOfTheDay[j]) << "</th>\n";
			if(activitiesAtTime[k][j].isEmpty()){
				if(breakDayHour[k][j] && PRINT_BREAK_TIME_SLOTS){
					tos<<writeBreakSlot("");
				} else {
					tos<<writeEmpty();
				}
			} else {
				tos<<writeActivitiesStudents(activitiesAtTime[k][j]);
			}
			tos<<"        </tr>\n";
		}
	}
	//workaround begin. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
	tos<<"      <tr class=\"foot\"><td colspan=\"2\"></td><td>"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr>\n";
	//workaround end.
	tos<<"      </tbody>\n";
	tos<<"    </table>\n";
	tos<<"  </body>\n</html>\n";

	if(file.error()>0){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Writing %1 gave error code %2, which means saving is compromised. Please check your disk's free space.").arg(htmlfilename).arg(file.error()));
	}
	file.close();
}

//XHTML generation code by Volker Dirr (http://timetabling.de/)
void TimetableExport::writeAllActivitiesTimetableTimeHorizontalHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities){
	assert(gt.rules.initialized && gt.rules.internalStructureComputed);
	//assert(gt.timePopulation.initialized);
	assert(students_schedule_ready && teachers_schedule_ready && rooms_schedule_ready);

	//Now we print the results to an HTML file
	QFile file(htmlfilename);
	if(!file.open(QIODevice::WriteOnly)){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Cannot open file %1 for writing. Please check your disk's free space. Saving of %1 aborted.").arg(htmlfilename));
		return;
		assert(0);
	}
	QTextStream tos(&file);
	tos.setCodec("UTF-8");
	tos.setGenerateByteOrderMark(true);

	tos<<writeHead(true, placedActivities, false);

	tos<<"    <table border=\"1\">\n";
	tos<<"      <caption>"<<protect2(gt.rules.institutionName)<<"</caption>\n";
	tos<<"      <thead>\n        <tr><td rowspan=\"2\"></td>";
	for(int k=0; k<gt.rules.nDaysPerWeek; k++)
		tos << "<th colspan=\"" << gt.rules.nHoursPerDay <<"\">" << protect2(gt.rules.daysOfTheWeek[k]) << "</th>";
	tos <<"</tr>\n";
	tos<<"        <tr>\n          <!-- span -->\n";
	for(int k=0; k<gt.rules.nDaysPerWeek; k++)
		for(int j=0; j<gt.rules.nHoursPerDay; j++){
			if(TIMETABLE_HTML_LEVEL>=2)
				tos<<"          <th class=\"xAxis\">";
			else
				tos<<"          <th>";
			tos << protect2(gt.rules.hoursOfTheDay[j]) << "</th>\n";
		}
	tos<<"        </tr>\n";
	tos<<"      </thead>\n";
	/*workaround. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
	tos<<"      <tfoot><tr><td colspan=\"2\"></td><td colspan=\""<<gt.rules.nHoursPerDay*gt.rules.nDaysPerWeek<<"\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr></tfoot>\n";
	*/
	tos<<"      <tbody>\n";
		
	tos << "        <tr>\n";
	if(TIMETABLE_HTML_LEVEL>=2)
		tos<<"          <th class=\"yAxis\">";
	else
		tos<<"          <th>";
	tos << tr("All Activities") << "</th>\n";
	for(int k=0; k<gt.rules.nDaysPerWeek; k++){
		for(int j=0; j<gt.rules.nHoursPerDay; j++){
			if(activitiesAtTime[k][j].isEmpty()){
				if(breakDayHour[k][j] && PRINT_BREAK_TIME_SLOTS){
					tos<<writeBreakSlot("");
				} else {
					tos<<writeEmpty();
				}
			} else {
				tos<<writeActivitiesStudents(activitiesAtTime[k][j]);
			}
		}
	}
	tos<<"        </tr>\n";
	//workaround begin. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
	tos<<"      <tr class=\"foot\"><td></td><td colspan=\""<<gt.rules.nHoursPerDay*gt.rules.nDaysPerWeek<<"\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr>\n";
	//workaround end.
	tos<<"      </tbody>\n";
	tos<<"    </table>\n";
	tos<<"  </body>\n</html>\n";

	if(file.error()>0){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Writing %1 gave error code %2, which means saving is compromised. Please check your disk's free space.").arg(htmlfilename).arg(file.error()));
	}
	file.close();
}


//XHTML generation code by Volker Dirr (http://timetabling.de/)
void TimetableExport::writeAllActivitiesTimetableTimeVerticalDailyHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities){
	assert(gt.rules.initialized && gt.rules.internalStructureComputed);
	//assert(gt.timePopulation.initialized);
	assert(students_schedule_ready && teachers_schedule_ready && rooms_schedule_ready);

	//Now we print the results to an HTML file
	QFile file(htmlfilename);
	if(!file.open(QIODevice::WriteOnly)){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Cannot open file %1 for writing. Please check your disk's free space. Saving of %1 aborted.").arg(htmlfilename));
		return;
		assert(0);
	}
	QTextStream tos(&file);
	tos.setCodec("UTF-8");
	tos.setGenerateByteOrderMark(true);

	tos<<writeHead(true, placedActivities, true);
	tos<<writeTOCDays(false);

	for(int k=0; k<gt.rules.nDaysPerWeek; k++){
		tos<<"    <table id=\"table_"<<hashDayIDsTimetable.value(gt.rules.daysOfTheWeek[k])<<"\" border=\"1\">\n";
		tos<<"      <caption>"<<protect2(gt.rules.institutionName)<<"</caption>\n";
		tos<<"      <thead>\n        <tr><td colspan=\"2\"></td>";
		if(TIMETABLE_HTML_LEVEL>=2)
			tos<<"          <th class=\"xAxis\">";
		else
			tos<<"          <th>";
		tos << tr("All Activities"); //Liviu
		tos<<"</th></tr>\n      </thead>\n";
		/*workaround. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
		tos<<"      <tfoot><tr><td colspan=\"2\"></td><td>"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr></tfoot>\n";
		*/
		tos<<"      <tbody>\n";

		for(int j=0; j<gt.rules.nHoursPerDay; j++){
			tos << "        <tr>\n";
			if(j==0)
				tos << "        <th rowspan=\"" << gt.rules.nHoursPerDay  << "\">" << protect2vert(gt.rules.daysOfTheWeek[k]) << "</th>\n";
			else tos <<"          <!-- span -->\n";
			if(TIMETABLE_HTML_LEVEL>=2)
				tos<<"          <th class=\"yAxis\">";
			else
				tos<<"          <th>";
			tos << protect2(gt.rules.hoursOfTheDay[j]) << "</th>\n";
			if(activitiesAtTime[k][j].isEmpty()){
				if(breakDayHour[k][j] && PRINT_BREAK_TIME_SLOTS){
					tos<<writeBreakSlot("");
				} else {
					tos<<writeEmpty();
				}
			} else {
				tos<<writeActivitiesStudents(activitiesAtTime[k][j]);
			}
			tos<<"        </tr>\n";
		}
		//workaround begin. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
		tos<<"        <tr class=\"foot\"><td colspan=\"2\"></td><td>"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr>\n";
		//workaround end.
		tos<<"      </tbody>\n";
		tos<<"    </table>\n\n";
		tos<<"    <p class=\"back\"><a href=\""<<"#top\">"<<TimetableExport::tr("back to the top")<<"</a></p>\n\n";
	}
	tos<<"  </body>\n</html>\n";

	if(file.error()>0){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Writing %1 gave error code %2, which means saving is compromised. Please check your disk's free space.").arg(htmlfilename).arg(file.error()));
	}
	file.close();
}

//XHTML generation code by Volker Dirr (http://timetabling.de/)
void TimetableExport::writeAllActivitiesTimetableTimeHorizontalDailyHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities){
	assert(gt.rules.initialized && gt.rules.internalStructureComputed);
	//assert(gt.timePopulation.initialized);
	assert(students_schedule_ready && teachers_schedule_ready && rooms_schedule_ready);

	//Now we print the results to an HTML file
	QFile file(htmlfilename);
	if(!file.open(QIODevice::WriteOnly)){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Cannot open file %1 for writing. Please check your disk's free space. Saving of %1 aborted.").arg(htmlfilename));
		return;
		assert(0);
	}
	QTextStream tos(&file);
	tos.setCodec("UTF-8");
	tos.setGenerateByteOrderMark(true);

	tos<<writeHead(true, placedActivities, true);
	tos<<writeTOCDays(false);

	for(int k=0; k<gt.rules.nDaysPerWeek; k++){
		tos<<"    <table id=\"table_"<<hashDayIDsTimetable.value(gt.rules.daysOfTheWeek[k])<<"\" border=\"1\">\n";
		tos<<"      <caption>"<<protect2(gt.rules.institutionName)<<"</caption>\n";
		tos<<"      <thead>\n        <tr><td rowspan=\"2\"></td>";
		tos << "<th colspan=\"" << gt.rules.nHoursPerDay <<"\">" << protect2(gt.rules.daysOfTheWeek[k]) << "</th>";
		tos <<"</tr>\n";
		tos<<"        <tr>\n          <!-- span -->\n";
		for(int j=0; j<gt.rules.nHoursPerDay; j++){
			if(TIMETABLE_HTML_LEVEL>=2)
				tos<<"          <th class=\"xAxis\">";
			else
				tos<<"          <th>";
			tos << protect2(gt.rules.hoursOfTheDay[j]) << "</th>\n";
		}
		tos<<"        </tr>\n";
		tos<<"      </thead>\n";
		/*workaround. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
		tos<<"      <tfoot><tr><td colspan=\"2\"></td><td colspan=\""<<gt.rules.nHoursPerDay<<"\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr></tfoot>\n";
		*/
		tos<<"      <tbody>\n";
			
		tos << "        <tr>\n";
		if(TIMETABLE_HTML_LEVEL>=2)
			tos<<"          <th class=\"yAxis\">";
		else
			tos<<"          <th>";
		tos << tr("All Activities") << "</th>\n";
		for(int j=0; j<gt.rules.nHoursPerDay; j++){
			if(activitiesAtTime[k][j].isEmpty()){
				if(breakDayHour[k][j] && PRINT_BREAK_TIME_SLOTS){
					tos<<writeBreakSlot("");
				} else {
					tos<<writeEmpty();
				}
			} else {
				tos<<writeActivitiesStudents(activitiesAtTime[k][j]);
			}
		}
		tos<<"        </tr>\n";
		//workaround begin. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
		tos<<"        <tr class=\"foot\"><td></td><td colspan=\""<<gt.rules.nHoursPerDay<<"\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr>\n";
		//workaround end.
		tos<<"      </tbody>\n";
		tos<<"    </table>\n\n";
		tos<<"    <p class=\"back\"><a href=\""<<"#top\">"<<TimetableExport::tr("back to the top")<<"</a></p>\n\n";
	}

	tos<<"  </body>\n</html>\n";

	if(file.error()>0){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Writing %1 gave error code %2, which means saving is compromised. Please check your disk's free space.").arg(htmlfilename).arg(file.error()));
	}
	file.close();
}


//************************************************************************************************************************************************************************************************

// Now print the teachers

//XHTML generation code modified by Volker Dirr (timetabling.de) from old html generation code
//(old code by Liviu Lalescu)
void TimetableExport::writeTeachersTimetableDaysHorizontalHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities){
	assert(gt.rules.initialized && gt.rules.internalStructureComputed);
	//assert(gt.timePopulation.initialized);
	assert(students_schedule_ready && teachers_schedule_ready && rooms_schedule_ready);

	//Now we print the results to an HTML file
	QFile file(htmlfilename);
	if(!file.open(QIODevice::WriteOnly)){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Cannot open file %1 for writing. Please check your disk's free space. Saving of %1 aborted.").arg(htmlfilename));
		return;
		assert(0);
	}
	QTextStream tos(&file);
	tos.setCodec("UTF-8");
	tos.setGenerateByteOrderMark(true);
	
	tos<<writeHead(true, placedActivities, true);
		
	tos<<"    <p><strong>"<<TimetableExport::tr("Table of contents")<<"</strong></p>\n";
	tos<<"    <ul>\n";
	for(int teacher=0; teacher<gt.rules.nInternalTeachers; teacher++){
		QString teacher_name = gt.rules.internalTeachersList[teacher]->name;
		tos<<"      <li><a href=\""<<"#table_"<<hashTeacherIDsTimetable.value(teacher_name)<<"\">"<<protect2(teacher_name)<<"</a></li>\n";
	}
	tos<<"    </ul>\n    <p>&nbsp;</p>\n\n";

	for(int teacher=0; teacher<gt.rules.nInternalTeachers; teacher++){
		QString teacher_name = gt.rules.internalTeachersList[teacher]->name;
		tos<<"    <table id=\"table_"<<hashTeacherIDsTimetable.value(teacher_name)<<"\" border=\"1\"";
		if(teacher%2==0)  tos<<" class=\"odd_table\"";
		else tos<<" class=\"even_table\"";
		tos<<">\n";
		
		tos<<"      <caption>"<<protect2(gt.rules.institutionName)<<"</caption>\n";

		tos<<"      <thead>\n        <tr><td rowspan=\"2\"></td><th colspan=\""<<gt.rules.nDaysPerWeek<<"\">"<<protect2(teacher_name)<<"</th></tr>\n";
		tos<<"        <tr>\n          <!-- span -->\n";
		for(int j=0; j<gt.rules.nDaysPerWeek; j++){
			if(TIMETABLE_HTML_LEVEL>=2)
				tos<<"          <th class=\"xAxis\">";
			else
				tos<<"          <th>";
			tos<<protect2(gt.rules.daysOfTheWeek[j])<<"</th>\n";
		}
		tos<<"        </tr>\n";
		tos<<"      </thead>\n";
		/*workaround. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
		tos<<"      <tfoot><tr><td></td><td colspan=\""<<gt.rules.nDaysPerWeek<<"\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr></tfoot>\n";
		*/
		tos<<"      <tbody>\n";
		for(int j=0; j<gt.rules.nHoursPerDay; j++){
			tos<<"        <tr>\n";
			if(TIMETABLE_HTML_LEVEL>=2)
				tos<<"          <th class=\"yAxis\">";
			else
				tos<<"          <th>";
			tos<<protect2(gt.rules.hoursOfTheDay[j])<<"</th>\n";
			for(int k=0; k<gt.rules.nDaysPerWeek; k++){
				QList<int> allActivities;
				allActivities.clear();
				allActivities<<teachers_timetable_weekly[teacher][k][j];
				bool activitiesWithSameStartingtime=addActivitiesWithSameStartingTime(allActivities, j);
				if(allActivities.size()==1 && !activitiesWithSameStartingtime){  // because i am using colspan or rowspan!!!
					tos<<writeActivityTeacher(teacher, k, j, false, true);
				} else{
					tos<<writeActivitiesTeachers(allActivities);
				}
			}
			tos<<"        </tr>\n";
		}
		//workaround begin. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
		tos<<"        <tr class=\"foot\"><td></td><td colspan=\""<<gt.rules.nDaysPerWeek<<"\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr>\n";
		//workaround end.
		tos<<"      </tbody>\n";
		tos<<"    </table>\n\n";
		tos<<"    <p class=\"back\"><a href=\""<<"#top\">"<<TimetableExport::tr("back to the top")<<"</a></p>\n\n";
	}
	tos<<"  </body>\n</html>\n";

	if(file.error()>0){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Writing %1 gave error code %2, which means saving is compromised. Please check your disk's free space.").arg(htmlfilename).arg(file.error()));
	}
	file.close();
}

//XHTML generation code modified by Volker Dirr (timetabling.de) from old html generation code
//(old code by Liviu Lalescu)
void TimetableExport::writeTeachersTimetableDaysVerticalHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities){
	assert(gt.rules.initialized && gt.rules.internalStructureComputed);
	//assert(gt.timePopulation.initialized);
	assert(students_schedule_ready && teachers_schedule_ready && rooms_schedule_ready);

	//Now we print the results to an HTML file
	QFile file(htmlfilename);
	if(!file.open(QIODevice::WriteOnly)){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Cannot open file %1 for writing. Please check your disk's free space. Saving of %1 aborted.").arg(htmlfilename));
		return;
		assert(0);
	}
	QTextStream tos(&file);
	tos.setCodec("UTF-8");
	tos.setGenerateByteOrderMark(true);

	tos<<writeHead(true, placedActivities, true); 
		
	tos<<"    <p><strong>"<<TimetableExport::tr("Table of contents")<<"</strong></p>\n";
	tos<<"    <ul>\n";
	for(int teacher=0; teacher<gt.rules.nInternalTeachers; teacher++){
		QString teacher_name = gt.rules.internalTeachersList[teacher]->name;
		tos<<"      <li><a href=\""<<"#table_"<<hashTeacherIDsTimetable.value(teacher_name)<<"\">"<<protect2(teacher_name)<<"</a></li>\n";
	}
	tos<<"    </ul>\n    <p>&nbsp;</p>\n\n";

	for(int teacher=0; teacher<gt.rules.nInternalTeachers; teacher++){
		QString teacher_name = gt.rules.internalTeachersList[teacher]->name;
		tos<<"    <table id=\"table_"<<hashTeacherIDsTimetable.value(teacher_name)<<"\" border=\"1\"";
		if(teacher%2==0) tos<<" class=\"odd_table\"";
		else tos<<" class=\"even_table\"";
		tos<<">\n";
		
		tos<<"      <caption>"<<protect2(gt.rules.institutionName)<<"</caption>\n";

		tos<<"      <thead>\n";
		tos<<"        <tr><td rowspan=\"2\"></td><th colspan=\""<<gt.rules.nHoursPerDay<<"\">"<<protect2(teacher_name)<<"</th></tr>\n";
		tos<<"        <tr>\n          <!-- span -->\n";
		for(int j=0; j<gt.rules.nHoursPerDay; j++){
			if(TIMETABLE_HTML_LEVEL>=2)
				tos<<"          <th class=\"xAxis\">";
			else
				tos<<"          <th>";
			tos<<protect2(gt.rules.hoursOfTheDay[j])<<"</th>\n";
		}
		tos<<"        </tr>\n";
		tos<<"      </thead>\n";
		/*workaround. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
		tos<<"      <tfoot><tr><td></td><td colspan=\""<<gt.rules.nHoursPerDay<<"\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr></tfoot>\n";
		*/
		tos<<"      <tbody>\n";
		for(int k=0; k<gt.rules.nDaysPerWeek; k++){
			tos<<"        <tr>\n";
			if(TIMETABLE_HTML_LEVEL>=2)
				tos<<"          <th class=\"yAxis\">";
			else
				tos<<"          <th>";
			tos<<protect2(gt.rules.daysOfTheWeek[k])<<"</th>\n";
			for(int j=0; j<gt.rules.nHoursPerDay; j++){
				QList<int> allActivities;
				allActivities.clear();
				allActivities<<teachers_timetable_weekly[teacher][k][j];
				bool activitiesWithSameStartingtime=addActivitiesWithSameStartingTime(allActivities, j);
				if(allActivities.size()==1 && !activitiesWithSameStartingtime){  // because i am using colspan or rowspan!!!
					tos<<writeActivityTeacher(teacher, k, j, true, false);
				} else{
					tos<<writeActivitiesTeachers(allActivities);
				}
			}
			tos<<"        </tr>\n";
		}
		//workaround begin. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
		tos<<"        <tr class=\"foot\"><td></td><td colspan=\""<<gt.rules.nHoursPerDay<<"\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr>\n";
		//workaround end.
		tos<<"      </tbody>\n";
		tos<<"    </table>\n\n";
		tos<<"    <p class=\"back\"><a href=\""<<"#top\">"<<TimetableExport::tr("back to the top")<<"</a></p>\n\n";
	}
	tos<<"  </body>\n</html>\n";

	if(file.error()>0){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Writing %1 gave error code %2, which means saving is compromised. Please check your disk's free space.").arg(htmlfilename).arg(file.error()));
	}
	file.close();
}

//XHTML generation code by Volker Dirr (http://timetabling.de/)
void TimetableExport::writeTeachersTimetableTimeVerticalHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities){
	assert(gt.rules.initialized && gt.rules.internalStructureComputed);
	//assert(gt.timePopulation.initialized);
	assert(students_schedule_ready && teachers_schedule_ready && rooms_schedule_ready);

	//Now we print the results to an HTML file
	QFile file(htmlfilename);
	if(!file.open(QIODevice::WriteOnly)){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Cannot open file %1 for writing. Please check your disk's free space. Saving of %1 aborted.").arg(htmlfilename));
		return;
		assert(0);
	}
	QTextStream tos(&file);
	tos.setCodec("UTF-8");
	tos.setGenerateByteOrderMark(true);

	tos<<writeHead(true, placedActivities, false);
	
	tos<<"    <table border=\"1\">\n";
	tos<<"      <caption>"<<protect2(gt.rules.institutionName)<<"</caption>\n";

	tos<<"      <thead>\n        <tr><td colspan=\"2\"></td>";
	for(int i=0; i<gt.rules.nInternalTeachers; i++){
		if(TIMETABLE_HTML_LEVEL>=2)
			tos<<"          <th class=\"xAxis\">";
		else
			tos<<"          <th>";
		tos << gt.rules.internalTeachersList[i]->name << "</th>";
	}
	tos<<"</tr>\n      </thead>\n";
	/*workaround. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
	tos<<"      <tfoot><tr><td colspan=\"2\"></td><td colspan=\""<<gt.rules.nInternalTeachers<<"\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr></tfoot>\n";
	*/
	tos<<"      <tbody>\n";
	for(int k=0; k<gt.rules.nDaysPerWeek; k++){
		for(int j=0; j<gt.rules.nHoursPerDay; j++){
			tos << "        <tr>\n";
			if(j==0)
				tos << "        <th rowspan=\"" << gt.rules.nHoursPerDay  << "\">" << protect2vert(gt.rules.daysOfTheWeek[k]) << "</th>\n";
			else tos <<"          <!-- span -->\n";
			if(TIMETABLE_HTML_LEVEL>=2)
				tos<<"          <th class=\"yAxis\">";
			else
				tos<<"          <th>";
			tos << protect2(gt.rules.hoursOfTheDay[j]) << "</th>\n";
			for(int teacher=0; teacher<gt.rules.nInternalTeachers; teacher++){
				QList<int> allActivities;
				allActivities.clear();
				allActivities<<teachers_timetable_weekly[teacher][k][j];
				bool activitiesWithSameStartingtime=addActivitiesWithSameStartingTime(allActivities, j);
				if(allActivities.size()==1 && !activitiesWithSameStartingtime){  // because i am using colspan or rowspan!!!
					tos<<writeActivityTeacher(teacher, k, j, false, true);
				} else {
					tos<<writeActivitiesTeachers(allActivities);
				}
			}
			tos<<"        </tr>\n";
		}
	}
	//workaround begin. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
	tos<<"      <tr class=\"foot\"><td colspan=\"2\"></td><td colspan=\""<<gt.rules.nInternalTeachers<<"\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr>\n";
	//workaround end.
	tos << "      </tbody>\n    </table>\n";
	tos << "  </body>\n</html>\n";

	if(file.error()>0){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Writing %1 gave error code %2, which means saving is compromised. Please check your disk's free space.").arg(htmlfilename).arg(file.error()));
	}
	file.close();
}

//XHTML generation code modified by Volker Dirr (timetabling.de) from old html generation code
//(old code by Liviu Lalescu)
void TimetableExport::writeTeachersTimetableTimeHorizontalHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities){
	assert(gt.rules.initialized && gt.rules.internalStructureComputed);
	//assert(gt.timePopulation.initialized);
	assert(students_schedule_ready && teachers_schedule_ready && rooms_schedule_ready);

	//Now we print the results to an HTML file
	QFile file(htmlfilename);
	if(!file.open(QIODevice::WriteOnly)){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Cannot open file %1 for writing. Please check your disk's free space. Saving of %1 aborted.").arg(htmlfilename));
		return;
		assert(0);
	}
	QTextStream tos(&file);
	tos.setCodec("UTF-8");
	tos.setGenerateByteOrderMark(true);

	tos<<writeHead(true, placedActivities, false);
	
	tos<<"    <table border=\"1\">\n";
	tos<<"      <caption>"<<protect2(gt.rules.institutionName)<<"</caption>\n";

	tos<<"      <thead>\n        <tr><td rowspan=\"2\"></td>";
	for(int k=0; k<gt.rules.nDaysPerWeek; k++)
		tos << "<th colspan=\"" << gt.rules.nHoursPerDay <<"\">" << protect2(gt.rules.daysOfTheWeek[k]) << "</th>";
	tos <<"</tr>\n";
	tos<<"        <tr>\n          <!-- span -->\n";
	for(int k=0; k<gt.rules.nDaysPerWeek; k++)
		for(int j=0; j<gt.rules.nHoursPerDay; j++){
			if(TIMETABLE_HTML_LEVEL>=2)
				tos<<"          <th class=\"xAxis\">";
			else
				tos<<"          <th>";
			tos << protect2(gt.rules.hoursOfTheDay[j]) << "</th>\n";
		}
	tos<<"        </tr>\n";
	tos<<"      </thead>\n";
	/*workaround. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
	tos<<"      <tfoot><tr><td></td><td colspan=\""<<gt.rules.nHoursPerDay*gt.rules.nDaysPerWeek<<"\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr></tfoot>\n";
	*/
	tos<<"      <tbody>\n";
	for(int teacher=0; teacher<gt.rules.nInternalTeachers; teacher++){
		tos << "        <tr>\n";
		if(TIMETABLE_HTML_LEVEL>=2)
			tos<<"          <th class=\"yAxis\">";
		else
			tos<<"          <th>";
		tos << gt.rules.internalTeachersList[teacher]->name << "</th>\n";
		for(int k=0; k<gt.rules.nDaysPerWeek; k++){
			for(int j=0; j<gt.rules.nHoursPerDay; j++){
				QList<int> allActivities;
				allActivities.clear();
				allActivities<<teachers_timetable_weekly[teacher][k][j];
				bool activitiesWithSameStartingtime=addActivitiesWithSameStartingTime(allActivities, j);
				if(allActivities.size()==1 && !activitiesWithSameStartingtime){  // because i am using colspan or rowspan!!!
					tos<<writeActivityTeacher(teacher, k, j, true, false);
				} else {
					tos<<writeActivitiesTeachers(allActivities);
				}
			}
		}
		tos<<"        </tr>\n";
	}
	//workaround begin. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
	tos<<"      <tr class=\"foot\"><td></td><td colspan=\""<<gt.rules.nHoursPerDay*gt.rules.nDaysPerWeek<<"\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr>\n";
	//workaround end.
	tos << "      </tbody>\n    </table>\n";
	tos << "  </body>\n</html>\n";

	if(file.error()>0){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Writing %1 gave error code %2, which means saving is compromised. Please check your disk's free space.").arg(htmlfilename).arg(file.error()));
	}
	file.close();
}


//by Volker Dirr
void TimetableExport::writeTeachersTimetableTimeVerticalDailyHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities){
	assert(gt.rules.initialized && gt.rules.internalStructureComputed);
	//assert(gt.timePopulation.initialized);
	assert(students_schedule_ready && teachers_schedule_ready && rooms_schedule_ready);

	//Now we print the results to an HTML file
	QFile file(htmlfilename);
	if(!file.open(QIODevice::WriteOnly)){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Cannot open file %1 for writing. Please check your disk's free space. Saving of %1 aborted.").arg(htmlfilename));
		return;
		assert(0);
	}
	QTextStream tos(&file);
	tos.setCodec("UTF-8");
	tos.setGenerateByteOrderMark(true);

	tos<<writeHead(true, placedActivities, true);
	tos<<writeTOCDays(false);

	for(int k=0; k<gt.rules.nDaysPerWeek; k++){
		tos<<"    <table id=\"table_"<<hashDayIDsTimetable.value(gt.rules.daysOfTheWeek[k])<<"\" border=\"1\">\n";
		tos<<"      <caption>"<<protect2(gt.rules.institutionName)<<"</caption>\n";
	
		tos<<"      <thead>\n        <tr><td colspan=\"2\"></td>";
		for(int i=0; i<gt.rules.nInternalTeachers; i++){
			if(TIMETABLE_HTML_LEVEL>=2)
				tos<<"          <th class=\"xAxis\">";
			else
				tos<<"          <th>";
			tos << gt.rules.internalTeachersList[i]->name << "</th>";
		}
		tos<<"</tr>\n      </thead>\n";
		/*workaround. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
		tos<<"      <tfoot><tr><td colspan=\"2\"></td><td colspan=\""<<gt.rules.nInternalTeachers<<"\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr></tfoot>\n";
		*/
		tos<<"      <tbody>\n";

		for(int j=0; j<gt.rules.nHoursPerDay; j++){
			tos << "        <tr>\n";
			if(j==0)
				tos << "        <th rowspan=\"" << gt.rules.nHoursPerDay  << "\">" << protect2vert(gt.rules.daysOfTheWeek[k]) << "</th>\n";
			else tos <<"          <!-- span -->\n";
			if(TIMETABLE_HTML_LEVEL>=2)
				tos<<"          <th class=\"yAxis\">";
			else
				tos<<"          <th>";
			tos << protect2(gt.rules.hoursOfTheDay[j]) << "</th>\n";
			for(int teacher=0; teacher<gt.rules.nInternalTeachers; teacher++){
				QList<int> allActivities;
				allActivities.clear();
				allActivities<<teachers_timetable_weekly[teacher][k][j];
				bool activitiesWithSameStartingtime=addActivitiesWithSameStartingTime(allActivities, j);
				if(allActivities.size()==1 && !activitiesWithSameStartingtime){  // because i am using colspan or rowspan!!!
					tos<<writeActivityTeacher(teacher, k, j, false, true);
				} else {
					tos<<writeActivitiesTeachers(allActivities);
				}
			}
			tos<<"        </tr>\n";
		}
		//workaround begin. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
		tos<<"        <tr class=\"foot\"><td colspan=\"2\"></td><td colspan=\""<<gt.rules.nInternalTeachers<<"\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr>\n";
		//workaround end.
		tos<<"      </tbody>\n";
		tos<<"    </table>\n\n";
		tos<<"    <p class=\"back\"><a href=\""<<"#top\">"<<TimetableExport::tr("back to the top")<<"</a></p>\n\n";
	}
	tos << "  </body>\n</html>\n";

	if(file.error()>0){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Writing %1 gave error code %2, which means saving is compromised. Please check your disk's free space.").arg(htmlfilename).arg(file.error()));
	}
	file.close();
}

//by Volker Dirr
void TimetableExport::writeTeachersTimetableTimeHorizontalDailyHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities){
	assert(gt.rules.initialized && gt.rules.internalStructureComputed);
	//assert(gt.timePopulation.initialized);
	assert(students_schedule_ready && teachers_schedule_ready && rooms_schedule_ready);

	//Now we print the results to an HTML file
	QFile file(htmlfilename);
	if(!file.open(QIODevice::WriteOnly)){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Cannot open file %1 for writing. Please check your disk's free space. Saving of %1 aborted.").arg(htmlfilename));
		return;
		assert(0);
	}
	QTextStream tos(&file);
	tos.setCodec("UTF-8");
	tos.setGenerateByteOrderMark(true);

	tos<<writeHead(true, placedActivities, true);
	tos<<writeTOCDays(false);

	for(int k=0; k<gt.rules.nDaysPerWeek; k++){
		tos<<"    <table id=\"table_"<<hashDayIDsTimetable.value(gt.rules.daysOfTheWeek[k])<<"\" border=\"1\">\n";
		tos<<"      <caption>"<<protect2(gt.rules.institutionName)<<"</caption>\n";
	
		tos<<"      <thead>\n        <tr><td rowspan=\"2\"></td>";
		tos << "<th colspan=\"" << gt.rules.nHoursPerDay <<"\">" << protect2(gt.rules.daysOfTheWeek[k]) << "</th>";
		tos <<"</tr>\n";
		tos<<"        <tr>\n          <!-- span -->\n";
		for(int j=0; j<gt.rules.nHoursPerDay; j++){
			if(TIMETABLE_HTML_LEVEL>=2)
				tos<<"          <th class=\"xAxis\">";
			else
				tos<<"          <th>";
			tos << protect2(gt.rules.hoursOfTheDay[j]) << "</th>\n";
		}
		tos<<"        </tr>\n";
		tos<<"      </thead>\n";
		/*workaround. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
		tos<<"      <tfoot><tr><td></td><td colspan=\""<<gt.rules.nHoursPerDay<<"\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr></tfoot>\n";
		*/
		tos<<"      <tbody>\n";
		for(int teacher=0; teacher<gt.rules.nInternalTeachers; teacher++){
			tos << "        <tr>\n";
			if(TIMETABLE_HTML_LEVEL>=2)
				tos<<"          <th class=\"yAxis\">";
			else
				tos<<"          <th>";
			tos << gt.rules.internalTeachersList[teacher]->name << "</th>\n";
		
			for(int j=0; j<gt.rules.nHoursPerDay; j++){
				QList<int> allActivities;
				allActivities.clear();
				allActivities<<teachers_timetable_weekly[teacher][k][j];
				bool activitiesWithSameStartingtime=addActivitiesWithSameStartingTime(allActivities, j);
				if(allActivities.size()==1 && !activitiesWithSameStartingtime){  // because i am using colspan or rowspan!!!
					tos<<writeActivityTeacher(teacher, k, j, true, false);
				} else {
					tos<<writeActivitiesTeachers(allActivities);
				}
			}
			tos<<"        </tr>\n";
		}
		//workaround begin. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
		tos<<"        <tr class=\"foot\"><td></td><td colspan=\""<<gt.rules.nHoursPerDay<<"\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr>\n";
		//workaround end.
		tos<<"      </tbody>\n";
		tos<<"    </table>\n\n";
		tos<<"    <p class=\"back\"><a href=\""<<"#top\">"<<TimetableExport::tr("back to the top")<<"</a></p>\n\n";
	}

	tos << "  </body>\n</html>\n";

	if(file.error()>0){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Writing %1 gave error code %2, which means saving is compromised. Please check your disk's free space.").arg(htmlfilename).arg(file.error()));
	}
	file.close();
}

// -----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

//writing the rooms' timetable html format to a file by Volker Dirr
void TimetableExport::writeRoomsTimetableDaysHorizontalHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities){
	assert(gt.rules.initialized && gt.rules.internalStructureComputed);
	//assert(gt.timePopulation.initialized);
	assert(students_schedule_ready && teachers_schedule_ready && rooms_schedule_ready);

	//Now we print the results to an HTML file
	QFile file(htmlfilename);
	if(!file.open(QIODevice::WriteOnly)){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Cannot open file %1 for writing. Please check your disk's free space. Saving of %1 aborted.").arg(htmlfilename));
		return;
		assert(0);
	}
	QTextStream tos(&file);
	tos.setCodec("UTF-8");
	tos.setGenerateByteOrderMark(true);

	tos<<writeHead(true, placedActivities, true);	
	
	if(gt.rules.nInternalRooms==0)
		tos<<"    <h1>"<<TimetableExport::tr("No rooms recorded in FET for %1.", "%1 is the institution name").arg(protect2(gt.rules.institutionName))<<"</h1>\n";
	else {
		tos<<"    <p><strong>"<<TimetableExport::tr("Table of contents")<<"</strong></p>\n";
		tos<<"    <ul>\n";
		for(int room=0; room<gt.rules.nInternalRooms; room++){
			QString room_name = gt.rules.internalRoomsList[room]->name;
			tos<<"      <li><a href=\""<<"#table_"<<hashRoomIDsTimetable.value(room_name)<<"\">"<<protect2(room_name)<<"</a></li>\n";
		}
		tos<<"    </ul>\n    <p>&nbsp;</p>\n\n";

		for(int room=0; room<gt.rules.nInternalRooms; room++){
			QString room_name = gt.rules.internalRoomsList[room]->name;
			tos<<"    <table id=\"table_"<<hashRoomIDsTimetable.value(room_name)<<"\" border=\"1\"";
			if(room%2==0)  tos<<" class=\"odd_table\"";
			else tos<<" class=\"even_table\"";
			tos<<">\n";
		
			tos<<"      <caption>"<<protect2(gt.rules.institutionName)<<"</caption>\n";

			tos<<"      <thead>\n        <tr><td rowspan=\"2\"></td><th colspan=\""<<gt.rules.nDaysPerWeek<<"\">"<<protect2(room_name)<<"</th></tr>\n";
			tos<<"        <tr>\n          <!-- span -->\n";
			for(int j=0; j<gt.rules.nDaysPerWeek; j++){
				if(TIMETABLE_HTML_LEVEL>=2)
					tos<<"          <th class=\"xAxis\">";
				else
					tos<<"          <th>";
				tos<<protect2(gt.rules.daysOfTheWeek[j])<<"</th>\n";
			}
			tos<<"        </tr>\n";
			tos<<"      </thead>\n";
			/*workaround. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
			tos<<"      <tfoot><tr><td></td><td colspan=\""<<gt.rules.nDaysPerWeek<<"\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr></tfoot>\n";
			*/
			tos<<"      <tbody>\n";
			for(int j=0; j<gt.rules.nHoursPerDay; j++){
				tos<<"        <tr>\n";
				if(TIMETABLE_HTML_LEVEL>=2)
					tos<<"          <th class=\"yAxis\">";
				else
					tos<<"          <th>";
				tos<<protect2(gt.rules.hoursOfTheDay[j])<<"</th>\n";
				for(int k=0; k<gt.rules.nDaysPerWeek; k++){
					QList<int> allActivities;
					allActivities.clear();
					allActivities<<rooms_timetable_weekly[room][k][j];
					bool activitiesWithSameStartingtime=addActivitiesWithSameStartingTime(allActivities, j);
					if(allActivities.size()==1 && !activitiesWithSameStartingtime){  // because i am using colspan or rowspan!!!
						tos<<writeActivityRoom(room, k, j, false, true);
					} else {
						tos<<writeActivitiesRooms(allActivities);
					}
				}
				tos<<"        </tr>\n";
			}
			//workaround begin. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
			tos<<"        <tr class=\"foot\"><td></td><td colspan=\""<<gt.rules.nDaysPerWeek<<"\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr>\n";
			//workaround end.
			tos<<"      </tbody>\n";
			tos<<"    </table>\n\n";
			tos<<"    <p class=\"back\"><a href=\""<<"#top\">"<<TimetableExport::tr("back to the top")<<"</a></p>\n\n";
		}
	}
	tos<<"  </body>\n</html>\n";

	if(file.error()>0){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Writing %1 gave error code %2, which means saving is compromised. Please check your disk's free space.").arg(htmlfilename).arg(file.error()));
	}
	file.close();
}

//writing the rooms' timetable html format to a file by Volker Dirr
void TimetableExport::writeRoomsTimetableDaysVerticalHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities){
	assert(gt.rules.initialized && gt.rules.internalStructureComputed);
	//assert(gt.timePopulation.initialized);
	assert(students_schedule_ready && teachers_schedule_ready && rooms_schedule_ready);

	//Now we print the results to an HTML file
	QFile file(htmlfilename);
	if(!file.open(QIODevice::WriteOnly)){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Cannot open file %1 for writing. Please check your disk's free space. Saving of %1 aborted.").arg(htmlfilename));
		return;

		assert(0);
	}
	QTextStream tos(&file);
	tos.setCodec("UTF-8");
	tos.setGenerateByteOrderMark(true);

	tos<<writeHead(true, placedActivities, true);
		
	if(gt.rules.nInternalRooms==0)
		tos<<"    <h1>"<<TimetableExport::tr("No rooms recorded in FET for %1.", "%1 is the institution name").arg(protect2(gt.rules.institutionName))<<"</h1>\n";
	else {
		tos<<"    <p><strong>"<<TimetableExport::tr("Table of contents")<<"</strong></p>\n";
		tos<<"    <ul>\n";
		for(int room=0; room<gt.rules.nInternalRooms; room++){
			QString room_name = gt.rules.internalRoomsList[room]->name;
			tos<<"      <li><a href=\""<<"#table_"<<hashRoomIDsTimetable.value(room_name)<<"\">"<<protect2(room_name)<<"</a></li>\n";
		}
		tos<<"    </ul>\n    <p>&nbsp;</p>\n\n";

		for(int room=0; room<gt.rules.nInternalRooms; room++){
			QString room_name = gt.rules.internalRoomsList[room]->name;
			tos<<"    <table id=\"table_"<<hashRoomIDsTimetable.value(room_name)<<"\" border=\"1\"";
			if(room%2==0)  tos<<" class=\"odd_table\"";
			else tos<<" class=\"even_table\"";
			tos<<">\n";
			
			tos<<"      <caption>"<<protect2(gt.rules.institutionName)<<"</caption>\n";

			tos<<"      <thead>\n";
			tos<<"        <tr><td rowspan=\"2\"></td><th colspan=\""<<gt.rules.nHoursPerDay<<"\">"<<protect2(room_name)<<"</th></tr>\n";
			tos<<"        <tr>\n          <!-- span -->\n";
			for(int j=0; j<gt.rules.nHoursPerDay; j++){
				if(TIMETABLE_HTML_LEVEL>=2)
					tos<<"          <th class=\"xAxis\">";
					else
						tos<<"          <th>";
					tos<<protect2(gt.rules.hoursOfTheDay[j])<<"</th>\n";
			}
			tos<<"        </tr>\n";
			tos<<"      </thead>\n";
			/*workaround. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
			tos<<"      <tfoot><tr><td></td><td colspan=\""<<gt.rules.nHoursPerDay<<"\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr></tfoot>\n";
			*/
			tos<<"      <tbody>\n";
			for(int k=0; k<gt.rules.nDaysPerWeek; k++){
				tos<<"        <tr>\n";
				if(TIMETABLE_HTML_LEVEL>=2)
					tos<<"          <th class=\"yAxis\">";
				else
					tos<<"          <th>";
				tos<<protect2(gt.rules.daysOfTheWeek[k])<<"</th>\n";
				for(int j=0; j<gt.rules.nHoursPerDay; j++){
					QList<int> allActivities;
					allActivities.clear();
					allActivities<<rooms_timetable_weekly[room][k][j];
					bool activitiesWithSameStartingtime=addActivitiesWithSameStartingTime(allActivities, j);
					if(allActivities.size()==1 && !activitiesWithSameStartingtime){  // because i am using colspan or rowspan!!!
						tos<<writeActivityRoom(room, k, j, true, false);
					} else {
						tos<<writeActivitiesRooms(allActivities);
					}
				}
				tos<<"        </tr>\n";
			}
			//workaround begin. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
			tos<<"        <tr class=\"foot\"><td></td><td colspan=\""<<gt.rules.nHoursPerDay<<"\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr>\n";
			//workaround end.
			tos<<"      </tbody>\n";
			tos<<"    </table>\n\n";
			tos<<"    <p class=\"back\"><a href=\""<<"#top\">"<<TimetableExport::tr("back to the top")<<"</a></p>\n\n";
		}
	}
	tos<<"  </body>\n</html>\n";

	if(file.error()>0){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Writing %1 gave error code %2, which means saving is compromised. Please check your disk's free space.").arg(htmlfilename).arg(file.error()));
	}
	file.close();
}


//writing the rooms' timetable html format to a file by Volker Dirr
void TimetableExport::writeRoomsTimetableTimeVerticalHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities){
	assert(gt.rules.initialized && gt.rules.internalStructureComputed);
	//assert(gt.timePopulation.initialized);
	assert(students_schedule_ready && teachers_schedule_ready && rooms_schedule_ready);

	//Now we print the results to an HTML file
	QFile file(htmlfilename);
	if(!file.open(QIODevice::WriteOnly)){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Cannot open file %1 for writing. Please check your disk's free space. Saving of %1 aborted.").arg(htmlfilename));
		return;
		assert(0);
	}
	QTextStream tos(&file);
	tos.setCodec("UTF-8");
	tos.setGenerateByteOrderMark(true);

	tos<<writeHead(true, placedActivities, false);
	
	if(gt.rules.nInternalRooms==0)
		tos<<"    <h1>"<<TimetableExport::tr("No rooms recorded in FET for %1.", "%1 is the institution name").arg(protect2(gt.rules.institutionName))<<"</h1>\n";
	else {
		tos<<"    <table border=\"1\">\n";
		tos<<"      <caption>"<<protect2(gt.rules.institutionName)<<"</caption>\n";

		tos<<"      <thead>\n        <tr><td colspan=\"2\"></td>";
		for(int i=0; i<gt.rules.nInternalRooms; i++){
			if(TIMETABLE_HTML_LEVEL>=2)
				tos<<"          <th class=\"xAxis\">";
			else
				tos<<"          <th>";
			tos << gt.rules.internalRoomsList[i]->name << "</th>";
		}
		tos<<"</tr>\n      </thead>\n";
		/*workaround. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
		tos<<"      <tfoot><tr><td colspan=\"2\"></td><td colspan=\""<<gt.rules.nInternalRooms<<"\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr></tfoot>\n";
		*/
		tos<<"      <tbody>\n";
		for(int k=0; k<gt.rules.nDaysPerWeek; k++){
			for(int j=0; j<gt.rules.nHoursPerDay; j++){
				tos << "        <tr>\n";
				if(j==0)
					tos << "        <th rowspan=\"" << gt.rules.nHoursPerDay  << "\">" << protect2vert(gt.rules.daysOfTheWeek[k]) << "</th>\n";
				else tos <<"          <!-- span -->\n";
				if(TIMETABLE_HTML_LEVEL>=2)
					tos<<"          <th class=\"yAxis\">";
				else
					tos<<"          <th>";
				tos << protect2(gt.rules.hoursOfTheDay[j]) << "</th>\n";
				for(int room=0; room<gt.rules.nInternalRooms; room++){
					QList<int> allActivities;
					allActivities.clear();
					allActivities<<rooms_timetable_weekly[room][k][j];
					bool activitiesWithSameStartingtime=addActivitiesWithSameStartingTime(allActivities, j);
					if(allActivities.size()==1 && !activitiesWithSameStartingtime){  // because i am using colspan or rowspan!!!
						tos<<writeActivityRoom(room, k, j, false, true);
					} else {
						tos<<writeActivitiesRooms(allActivities);
					}
				}
				tos<<"        </tr>\n";
			}
		}
		//workaround begin. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
		tos<<"      <tr class=\"foot\"><td colspan=\"2\"></td><td colspan=\""<<gt.rules.nInternalRooms<<"\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr>\n";
		//workaround end.
		tos << "      </tbody>\n    </table>\n";
	}
	tos << "  </body>\n</html>\n";

	if(file.error()>0){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Writing %1 gave error code %2, which means saving is compromised. Please check your disk's free space.").arg(htmlfilename).arg(file.error()));
	}
	file.close();
}


// writing the rooms' timetable html format to a file by Volker Dirr
void TimetableExport::writeRoomsTimetableTimeHorizontalHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities){
	assert(gt.rules.initialized && gt.rules.internalStructureComputed);
	//assert(gt.timePopulation.initialized);
	assert(students_schedule_ready && teachers_schedule_ready && rooms_schedule_ready);

	//Now we print the results to an HTML file
	QFile file(htmlfilename);
	if(!file.open(QIODevice::WriteOnly)){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Cannot open file %1 for writing. Please check your disk's free space. Saving of %1 aborted.").arg(htmlfilename));
		return;
		assert(0);
	}
	QTextStream tos(&file);
	tos.setCodec("UTF-8");
	tos.setGenerateByteOrderMark(true);

	tos<<writeHead(true, placedActivities, false);
	
	if(gt.rules.nInternalRooms==0)
		tos<<"    <h1>"<<TimetableExport::tr("No rooms recorded in FET for %1.", "%1 is the institution name").arg(protect2(gt.rules.institutionName))<<"</h1>\n";
	else {
		tos<<"    <table border=\"1\">\n";
		tos<<"      <caption>"<<protect2(gt.rules.institutionName)<<"</caption>\n";

		tos<<"      <thead>\n        <tr><td rowspan=\"2\"></td>";
		for(int k=0; k<gt.rules.nDaysPerWeek; k++)
			tos << "<th colspan=\"" << gt.rules.nHoursPerDay <<"\">" << protect2(gt.rules.daysOfTheWeek[k]) << "</th>";
		tos <<"</tr>\n";
		tos<<"        <tr>\n          <!-- span -->\n";
		for(int k=0; k<gt.rules.nDaysPerWeek; k++)
			for(int j=0; j<gt.rules.nHoursPerDay; j++){
				if(TIMETABLE_HTML_LEVEL>=2)
					tos<<"          <th class=\"xAxis\">";
				else
					tos<<"          <th>";
				tos << protect2(gt.rules.hoursOfTheDay[j]) << "</th>\n";
			}
		tos<<"        </tr>\n";
		tos<<"      </thead>\n";
		/*workaround. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
		tos<<"      <tfoot><tr><td></td><td colspan=\""<<gt.rules.nHoursPerDay*gt.rules.nDaysPerWeek<<"\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr></tfoot>\n";
		*/
		tos<<"      <tbody>\n";
		for(int room=0; room<gt.rules.nInternalRooms; room++){
			tos << "        <tr>\n";
			if(TIMETABLE_HTML_LEVEL>=2)
				tos<<"          <th class=\"yAxis\">";
			else
				tos<<"          <th>";
			tos << gt.rules.internalRoomsList[room]->name << "</th>\n";
			for(int k=0; k<gt.rules.nDaysPerWeek; k++){
				for(int j=0; j<gt.rules.nHoursPerDay; j++){
					QList<int> allActivities;
					allActivities.clear();
					allActivities<<rooms_timetable_weekly[room][k][j];
					bool activitiesWithSameStartingtime=addActivitiesWithSameStartingTime(allActivities, j);
					if(allActivities.size()==1 && !activitiesWithSameStartingtime){  // because i am using colspan or rowspan!!!
						tos<<writeActivityRoom(room, k, j, true, false);
					} else {
						tos<<writeActivitiesRooms(allActivities);
					}
				}
			}
			tos<<"        </tr>\n";
		}
		//workaround begin. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
		tos<<"      <tr class=\"foot\"><td></td><td colspan=\""<<gt.rules.nHoursPerDay*gt.rules.nDaysPerWeek<<"\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr>\n";
		//workaround end.
		tos << "      </tbody>\n    </table>\n";
	}
	tos << "  </body>\n</html>\n";

	if(file.error()>0){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Writing %1 gave error code %2, which means saving is compromised. Please check your disk's free space.").arg(htmlfilename).arg(file.error()));
	}
	file.close();
}


//by Volker Dirr
void TimetableExport::writeRoomsTimetableTimeVerticalDailyHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities){
	assert(gt.rules.initialized && gt.rules.internalStructureComputed);
	//assert(gt.timePopulation.initialized);
	assert(students_schedule_ready && teachers_schedule_ready && rooms_schedule_ready);

	//Now we print the results to an HTML file
	QFile file(htmlfilename);
	if(!file.open(QIODevice::WriteOnly)){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Cannot open file %1 for writing. Please check your disk's free space. Saving of %1 aborted.").arg(htmlfilename));
		return;
		assert(0);
	}
	QTextStream tos(&file);
	tos.setCodec("UTF-8");
	tos.setGenerateByteOrderMark(true);

	tos<<writeHead(true, placedActivities, true);
	tos<<writeTOCDays(false);
	
	if(gt.rules.nInternalRooms==0)
		tos<<"    <h1>"<<TimetableExport::tr("No rooms recorded in FET for %1.", "%1 is the institution name").arg(protect2(gt.rules.institutionName))<<"</h1>\n";
	else {
		for(int k=0; k<gt.rules.nDaysPerWeek; k++){
			tos<<"    <table id=\"table_"<<hashDayIDsTimetable.value(gt.rules.daysOfTheWeek[k])<<"\" border=\"1\">\n";
			tos<<"      <caption>"<<protect2(gt.rules.institutionName)<<"</caption>\n";
	
			tos<<"      <thead>\n        <tr><td colspan=\"2\"></td>";
			for(int i=0; i<gt.rules.nInternalRooms; i++){
				if(TIMETABLE_HTML_LEVEL>=2)
					tos<<"          <th class=\"xAxis\">";
				else
					tos<<"          <th>";
				tos << gt.rules.internalRoomsList[i]->name << "</th>";
			}
			tos<<"</tr>\n      </thead>\n";
			/*workaround. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
			tos<<"      <tfoot><tr><td colspan=\"2\"></td><td colspan=\""<<gt.rules.nInternalRooms<<"\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr></tfoot>\n";
			*/
			tos<<"      <tbody>\n";
		
			for(int j=0; j<gt.rules.nHoursPerDay; j++){
				tos << "        <tr>\n";
				if(j==0)
					tos << "        <th rowspan=\"" << gt.rules.nHoursPerDay  << "\">" << protect2vert(gt.rules.daysOfTheWeek[k]) << "</th>\n";
				else tos <<"          <!-- span -->\n";
				if(TIMETABLE_HTML_LEVEL>=2)
					tos<<"          <th class=\"yAxis\">";
				else
					tos<<"          <th>";
				tos << protect2(gt.rules.hoursOfTheDay[j]) << "</th>\n";
				for(int room=0; room<gt.rules.nInternalRooms; room++){
					QList<int> allActivities;
					allActivities.clear();
					allActivities<<rooms_timetable_weekly[room][k][j];
					bool activitiesWithSameStartingtime=addActivitiesWithSameStartingTime(allActivities, j);
					if(allActivities.size()==1 && !activitiesWithSameStartingtime){  // because i am using colspan or rowspan!!!
						tos<<writeActivityRoom(room, k, j, false, true);
					} else {
						tos<<writeActivitiesRooms(allActivities);
					}
				}
				tos<<"        </tr>\n";
			}
			//workaround begin. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
			tos<<"        <tr class=\"foot\"><td colspan=\"2\"></td><td colspan=\""<<gt.rules.nInternalRooms<<"\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr>\n";
			//workaround end.
			tos<<"      </tbody>\n";
			tos<<"    </table>\n\n";
			tos<<"    <p class=\"back\"><a href=\""<<"#top\">"<<TimetableExport::tr("back to the top")<<"</a></p>\n\n";
		}
	}
	tos << "  </body>\n</html>\n";

	if(file.error()>0){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Writing %1 gave error code %2, which means saving is compromised. Please check your disk's free space.").arg(htmlfilename).arg(file.error()));
	}
	file.close();
}


//by Volker Dirr
void TimetableExport::writeRoomsTimetableTimeHorizontalDailyHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities){
	assert(gt.rules.initialized && gt.rules.internalStructureComputed);
	//assert(gt.timePopulation.initialized);
	assert(students_schedule_ready && teachers_schedule_ready && rooms_schedule_ready);

	//Now we print the results to an HTML file
	QFile file(htmlfilename);
	if(!file.open(QIODevice::WriteOnly)){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Cannot open file %1 for writing. Please check your disk's free space. Saving of %1 aborted.").arg(htmlfilename));
		return;
		assert(0);
	}
	QTextStream tos(&file);
	tos.setCodec("UTF-8");
	tos.setGenerateByteOrderMark(true);

	tos<<writeHead(true, placedActivities, true);
	tos<<writeTOCDays(false);
	
	if(gt.rules.nInternalRooms==0)
		tos<<"    <h1>"<<TimetableExport::tr("No rooms recorded in FET for %1.", "%1 is the institution name").arg(protect2(gt.rules.institutionName))<<"</h1>\n";
	else {
		for(int k=0; k<gt.rules.nDaysPerWeek; k++){
			tos<<"    <table id=\"table_"<<hashDayIDsTimetable.value(gt.rules.daysOfTheWeek[k])<<"\" border=\"1\">\n";
			tos<<"      <caption>"<<protect2(gt.rules.institutionName)<<"</caption>\n";
	
			tos<<"      <thead>\n        <tr><td rowspan=\"2\"></td>";
			tos << "<th colspan=\"" << gt.rules.nHoursPerDay <<"\">" << protect2(gt.rules.daysOfTheWeek[k]) << "</th>";
			tos <<"</tr>\n";
			tos<<"        <tr>\n          <!-- span -->\n";
			for(int j=0; j<gt.rules.nHoursPerDay; j++){
				if(TIMETABLE_HTML_LEVEL>=2)
					tos<<"          <th class=\"xAxis\">";
				else
					tos<<"          <th>";
				tos << protect2(gt.rules.hoursOfTheDay[j]) << "</th>\n";
			}
			tos<<"        </tr>\n";
			tos<<"      </thead>\n";
			/*workaround. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
			tos<<"      <tfoot><tr><td></td><td colspan=\""<<gt.rules.nHoursPerDay<<"\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr></tfoot>\n";
			*/
			tos<<"      <tbody>\n";
			for(int room=0; room<gt.rules.nInternalRooms; room++){
				tos << "        <tr>\n";
				if(TIMETABLE_HTML_LEVEL>=2)
					tos<<"          <th class=\"yAxis\">";
				else
					tos<<"          <th>";
				tos << gt.rules.internalRoomsList[room]->name << "</th>\n";
				for(int j=0; j<gt.rules.nHoursPerDay; j++){
					QList<int> allActivities;
					allActivities.clear();
					allActivities<<rooms_timetable_weekly[room][k][j];
					bool activitiesWithSameStartingtime=addActivitiesWithSameStartingTime(allActivities, j);
					if(allActivities.size()==1 && !activitiesWithSameStartingtime){  // because i am using colspan or rowspan!!!
						tos<<writeActivityRoom(room, k, j, true, false);
					} else {
						tos<<writeActivitiesRooms(allActivities);
					}
				}
				tos<<"        </tr>\n";
			}
			//workaround begin. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
			tos<<"        <tr class=\"foot\"><td></td><td colspan=\""<<gt.rules.nHoursPerDay<<"\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr>\n";
			//workaround end.
			tos<<"      </tbody>\n";
			tos<<"    </table>\n\n";
			tos<<"    <p class=\"back\"><a href=\""<<"#top\">"<<TimetableExport::tr("back to the top")<<"</a></p>\n\n";
		}

	}
	tos << "  </body>\n</html>\n";

	if(file.error()>0){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Writing %1 gave error code %2, which means saving is compromised. Please check your disk's free space.").arg(htmlfilename).arg(file.error()));
	}
	file.close();
}


//Now print the subjects ----------------------------------------------------------------------------------------------------------------------------------------------------------------------

//XHTML generation code by Volker Dirr (http://timetabling.de/)
void TimetableExport::writeSubjectsTimetableDaysHorizontalHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities){
	assert(gt.rules.initialized && gt.rules.internalStructureComputed);
	//assert(gt.timePopulation.initialized);
	assert(students_schedule_ready && teachers_schedule_ready && rooms_schedule_ready);

	//Now we print the results to an HTML file
	QFile file(htmlfilename);
	if(!file.open(QIODevice::WriteOnly)){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Cannot open file %1 for writing. Please check your disk's free space. Saving of %1 aborted.").arg(htmlfilename));
		return;
		assert(0);
	}
	QTextStream tos(&file);
	tos.setCodec("UTF-8");
	tos.setGenerateByteOrderMark(true);

	tos<<writeHead(true, placedActivities, true);

	tos<<"    <p><strong>"<<TimetableExport::tr("Table of contents")<<"</strong></p>\n";
	tos<<"    <ul>\n";
	for(int i=0; i<gt.rules.nInternalSubjects; i++){
		tos<<"      <li>\n        "<<TimetableExport::tr("Subject");
		tos<<" <a href=\""<<"#table_"<<hashSubjectIDsTimetable.value(gt.rules.internalSubjectsList[i]->name)<<"\">"<<gt.rules.internalSubjectsList[i]->name<<"</a>\n";
		tos<<"      </li>\n";
	}
	tos<<"    </ul>\n    <p>&nbsp;</p>\n\n";


	for(int subject=0; subject<gt.rules.nInternalSubjects; subject++){
		///////Liviu here
		for(int d=0; d<gt.rules.nDaysPerWeek; d++)
			for(int h=0; h<gt.rules.nHoursPerDay; h++)
				activitiesForCurrentSubject[d][h].clear();
		foreach(int ai, gt.rules.activitiesForSubject[subject])
			if(best_solution.times[ai]!=UNALLOCATED_TIME){
				int d=best_solution.times[ai]%gt.rules.nDaysPerWeek;
				int h=best_solution.times[ai]/gt.rules.nDaysPerWeek;
				Activity* act=&gt.rules.internalActivitiesList[ai];
				for(int dd=0; dd < act->duration && h+dd < gt.rules.nHoursPerDay; dd++)
					activitiesForCurrentSubject[d][h+dd].append(ai);
			}
		///////end Liviu
	
		tos<<"    <table id=\"table_"<<hashSubjectIDsTimetable.value(gt.rules.internalSubjectsList[subject]->name);
		tos<<"\" border=\"1\"";
		if(subject%2==0)  tos<<" class=\"odd_table\"";
		else tos<<" class=\"even_table\"";
		tos<<">\n";

		tos<<"      <caption>"<<protect2(gt.rules.institutionName)<<"</caption>\n";

		tos<<"      <thead>\n        <tr><td rowspan=\"2\"></td><th colspan=\""<<gt.rules.nDaysPerWeek<<"\">"<<protect2(gt.rules.internalSubjectsList[subject]->name)<<"</th></tr>\n";
		tos<<"        <tr>\n          <!-- span -->\n";
		for(int j=0; j<gt.rules.nDaysPerWeek; j++){
			if(TIMETABLE_HTML_LEVEL>=2)
				tos<<"          <th class=\"xAxis\">";
			else
				tos<<"          <th>";
			tos<<protect2(gt.rules.daysOfTheWeek[j])<<"</th>\n";
		}
		tos<<"        </tr>\n";
		tos<<"      </thead>\n";
		/*workaround. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
		tos<<"      <tfoot><tr><td></td><td colspan=\""<<gt.rules.nDaysPerWeek<<"\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr></tfoot>\n";
		*/
		tos<<"      <tbody>\n";
		for(int j=0; j<gt.rules.nHoursPerDay; j++){
			tos<<"        <tr>\n";
			if(TIMETABLE_HTML_LEVEL>=2)
				tos<<"          <th class=\"yAxis\">";
			else
				tos<<"          <th>";
			tos<<protect2(gt.rules.hoursOfTheDay[j])<<"</th>\n";
			for(int k=0; k<gt.rules.nDaysPerWeek; k++){
				QList<int> allActivities;
				
				//Liviu:
				allActivities=activitiesForCurrentSubject[k][j];
				
				/*
				allActivities.clear();
				//Now get the activitiy ids. I don't run through the InternalActivitiesList, even that is faster. I run through subgroupsList, because by that the activites are sorted by that in the html-table.
				for(int subgroup=0; subgroup<gt.rules.nInternalSubgroups; subgroup++){
					if(students_timetable_weekly[subgroup][k][j]!=UNALLOCATED_ACTIVITY){
						Activity* act=&gt.rules.internalActivitiesList[students_timetable_weekly[subgroup][k][j]];
						if(act->subjectName==gt.rules.internalSubjectsList[subject]->name)
							if(!(allActivities.contains(students_timetable_weekly[subgroup][k][j]))){
								allActivities<<students_timetable_weekly[subgroup][k][j];
							}
					}
				}
				//Now run through the teachers timetable, because activities without a students set are still missing.
				for(int teacher=0; teacher<gt.rules.nInternalTeachers; teacher++){
					if(teachers_timetable_weekly[teacher][k][j]!=UNALLOCATED_ACTIVITY){
						Activity* act=&gt.rules.internalActivitiesList[teachers_timetable_weekly[teacher][k][j]];
						if(act->subjectName==gt.rules.internalSubjectsList[subject]->name)
							if(!(allActivities.contains(teachers_timetable_weekly[teacher][k][j]))){
								assert(act->studentsNames.isEmpty());
								allActivities<<teachers_timetable_weekly[teacher][k][j];
							}
					}
				}*/
				addActivitiesWithSameStartingTime(allActivities, j);
				tos<<writeActivitiesSubjects(allActivities);
			}
			tos<<"        </tr>\n";
		}
		//workaround begin. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
		tos<<"        <tr class=\"foot\"><td></td><td colspan=\""<<gt.rules.nDaysPerWeek<<"\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr>\n";
		//workaround end.
		tos<<"      </tbody>\n";
		tos<<"    </table>\n\n";
		tos<<"    <p class=\"back\"><a href=\""<<"#top\">"<<TimetableExport::tr("back to the top")<<"</a></p>\n\n";
	}
	tos<<"  </body>\n</html>\n";

	if(file.error()>0){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Writing %1 gave error code %2, which means saving is compromised. Please check your disk's free space.").arg(htmlfilename).arg(file.error()));
	}
	file.close();
}


//XHTML generation code by Volker Dirr (http://timetabling.de/)
void TimetableExport::writeSubjectsTimetableDaysVerticalHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities){
	assert(gt.rules.initialized && gt.rules.internalStructureComputed);
	//assert(gt.timePopulation.initialized);
	assert(students_schedule_ready && teachers_schedule_ready && rooms_schedule_ready);

	//Now we print the results to an HTML file
	QFile file(htmlfilename);
	if(!file.open(QIODevice::WriteOnly)){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Cannot open file %1 for writing. Please check your disk's free space. Saving of %1 aborted.").arg(htmlfilename));
		return;
		assert(0);
	}
	QTextStream tos(&file);
	tos.setCodec("UTF-8");
	tos.setGenerateByteOrderMark(true);

	tos<<writeHead(true, placedActivities, true);

	tos<<"    <p><strong>"<<TimetableExport::tr("Table of contents")<<"</strong></p>\n";
	tos<<"    <ul>\n";
	for(int i=0; i<gt.rules.nInternalSubjects; i++){
		tos<<"      <li>\n        "<<TimetableExport::tr("Subject");
		tos<<" <a href=\""<<"#table_"<<hashSubjectIDsTimetable.value(gt.rules.internalSubjectsList[i]->name)<<"\">"<<gt.rules.internalSubjectsList[i]->name<<"</a>\n";
		tos<<"      </li>\n";
	}
	tos<<"    </ul>\n    <p>&nbsp;</p>\n\n";

	for(int subject=0; subject<gt.rules.nInternalSubjects; subject++){
		///////Liviu here
		for(int d=0; d<gt.rules.nDaysPerWeek; d++)
			for(int h=0; h<gt.rules.nHoursPerDay; h++)
				activitiesForCurrentSubject[d][h].clear();
		foreach(int ai, gt.rules.activitiesForSubject[subject])
			if(best_solution.times[ai]!=UNALLOCATED_TIME){
				int d=best_solution.times[ai]%gt.rules.nDaysPerWeek;
				int h=best_solution.times[ai]/gt.rules.nDaysPerWeek;
				Activity* act=&gt.rules.internalActivitiesList[ai];
				for(int dd=0; dd < act->duration && h+dd < gt.rules.nHoursPerDay; dd++)
					activitiesForCurrentSubject[d][h+dd].append(ai);
			}
		///////end Liviu

		tos<<"    <table id=\"table_"<<hashSubjectIDsTimetable.value(gt.rules.internalSubjectsList[subject]->name);
		tos<<"\" border=\"1\"";
		if(subject%2==0) tos<<" class=\"odd_table\"";
		else tos<<" class=\"even_table\"";
		tos<<">\n";

		tos<<"      <caption>"<<protect2(gt.rules.institutionName)<<"</caption>\n";

		tos<<"      <thead>\n        <tr><td rowspan=\"2\"></td><th colspan=\""<<gt.rules.nHoursPerDay<<"\">"<<protect2(gt.rules.internalSubjectsList[subject]->name)<<"</th></tr>\n";
		tos<<"        <tr>\n          <!-- span -->\n";
		for(int j=0; j<gt.rules.nHoursPerDay; j++){
			if(TIMETABLE_HTML_LEVEL>=2)
				tos<<"          <th class=\"xAxis\">";
			else
				tos<<"          <th>";
			tos<<protect2(gt.rules.hoursOfTheDay[j])<<"</th>\n";
		}
		tos<<"        </tr>\n";
		tos<<"      </thead>\n";
		/*workaround. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
		tos<<"      <tfoot><tr><td></td><td colspan=\""<<gt.rules.nHoursPerDay<<"\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr></tfoot>\n";
		*/
		tos<<"      <tbody>\n";
		for(int k=0; k<gt.rules.nDaysPerWeek; k++){
			tos<<"        <tr>\n";
			if(TIMETABLE_HTML_LEVEL>=2)
				tos<<"          <th class=\"yAxis\">";
			else
				tos<<"          <th>";
			tos<<protect2(gt.rules.daysOfTheWeek[k])<<"</th>\n";
			for(int j=0; j<gt.rules.nHoursPerDay; j++){
				QList<int> allActivities;

				//Liviu:
				allActivities=activitiesForCurrentSubject[k][j];

				/*
				allActivities.clear();
				//Now get the activitiy ids. I don't run through the InternalActivitiesList, even that is faster. I run through subgroupsList, because by that the activites are sorted by that in the html-table.
				for(int subgroup=0; subgroup<gt.rules.nInternalSubgroups; subgroup++){
					if(students_timetable_weekly[subgroup][k][j]!=UNALLOCATED_ACTIVITY){
						Activity* act=&gt.rules.internalActivitiesList[students_timetable_weekly[subgroup][k][j]];
						if(act->subjectName==gt.rules.internalSubjectsList[subject]->name)
							if(!(allActivities.contains(students_timetable_weekly[subgroup][k][j]))){
								allActivities<<students_timetable_weekly[subgroup][k][j];
							}
					}
				}
				//Now run through the teachers timetable, because activities without a students set are still missing.
				for(int teacher=0; teacher<gt.rules.nInternalTeachers; teacher++){
					if(teachers_timetable_weekly[teacher][k][j]!=UNALLOCATED_ACTIVITY){
						Activity* act=&gt.rules.internalActivitiesList[teachers_timetable_weekly[teacher][k][j]];
						if(act->subjectName==gt.rules.internalSubjectsList[subject]->name)
							if(!(allActivities.contains(teachers_timetable_weekly[teacher][k][j]))){
								assert(act->studentsNames.isEmpty());
								allActivities<<teachers_timetable_weekly[teacher][k][j];
							}
					}
				}
				*/
				addActivitiesWithSameStartingTime(allActivities, j);
				tos<<writeActivitiesSubjects(allActivities);
			}
		tos<<"        </tr>\n";
		}
		//workaround begin. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
		tos<<"        <tr class=\"foot\"><td></td><td colspan=\""<<gt.rules.nHoursPerDay<<"\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr>\n";
		//workaround end.
		tos<<"      </tbody>\n";
		tos<<"    </table>\n\n";
		tos<<"    <p class=\"back\"><a href=\""<<"#top\">"<<TimetableExport::tr("back to the top")<<"</a></p>\n\n";
	}
	tos << "  </body>\n</html>\n";

	if(file.error()>0){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Writing %1 gave error code %2, which means saving is compromised. Please check your disk's free space.").arg(htmlfilename).arg(file.error()));
	}
	file.close();
}


//XHTML generation code by Volker Dirr (http://timetabling.de/)
void TimetableExport::writeSubjectsTimetableTimeVerticalHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities){
	assert(gt.rules.initialized && gt.rules.internalStructureComputed);
	//assert(gt.timePopulation.initialized);
	assert(students_schedule_ready && teachers_schedule_ready && rooms_schedule_ready);

	//Now we print the results to an HTML file
	QFile file(htmlfilename);
	if(!file.open(QIODevice::WriteOnly)){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Cannot open file %1 for writing. Please check your disk's free space. Saving of %1 aborted.").arg(htmlfilename));
		return;
		assert(0);
	}
	QTextStream tos(&file);
	tos.setCodec("UTF-8");
	tos.setGenerateByteOrderMark(true);

	tos<<writeHead(true, placedActivities, false);

	tos<<"    <table id=\"table\" border=\"1\">\n";

	tos<<"      <caption>"<<protect2(gt.rules.institutionName)<<"</caption>\n";

	tos<<"      <thead>\n        <tr><td colspan=\"2\"></td>";
	for(int i=0; i<gt.rules.nInternalSubjects; i++){
			if(TIMETABLE_HTML_LEVEL>=2)
				tos<<"          <th class=\"xAxis\">";
			else
				tos<<"          <th>";
			tos << gt.rules.internalSubjectsList[i]->name << "</th>";
	}
		
	tos<<"</tr>\n      </thead>\n";
	/*workaround. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
	tos<<"      <tfoot><tr><td colspan=\"2\"></td><td colspan=\""<<gt.rules.internalSubjectsList.size()<<"\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr></tfoot>\n";
	*/
	tos<<"      <tbody>\n";

	//LIVIU										//maybe TODO: write a function for this
	//already computed
	/*for(int k=0; k<gt.rules.nDaysPerWeek; k++)
		for(int j=0; j<gt.rules.nHoursPerDay; j++)
			activitiesAtTime[k][j].clear();
	for(int i=0; i<gt.rules.nInternalActivities; i++) {
		Activity* act=&gt.rules.internalActivitiesList[i];
		if(best_solution.times[i]!=UNALLOCATED_TIME) {
			int hour=best_solution.times[i]/gt.rules.nDaysPerWeek;
			int day=best_solution.times[i]%gt.rules.nDaysPerWeek;
			for(int dd=0; dd < act->duration && hour+dd < gt.rules.nHoursPerDay; dd++)
				activitiesAtTime[day][hour+dd].append(i);
		}
	}*/
	///////

	for(int k=0; k<gt.rules.nDaysPerWeek; k++){
		for(int j=0; j<gt.rules.nHoursPerDay; j++){
			tos << "        <tr>\n";
			if(j==0)
				tos << "        <th rowspan=\"" << gt.rules.nHoursPerDay  << "\">" << protect2vert(gt.rules.daysOfTheWeek[k]) << "</th>\n";
			else tos <<"          <!-- span -->\n";
			if(TIMETABLE_HTML_LEVEL>=2)
				tos<<"          <th class=\"yAxis\">";
			else
				tos<<"          <th>";
			tos << protect2(gt.rules.hoursOfTheDay[j]) << "</th>\n";


			for(int subject=0; subject<gt.rules.nInternalSubjects; subject++){
				QList<int> allActivities;
				allActivities.clear();
				
				foreach(int ai, gt.rules.activitiesForSubject[subject])
					if(activitiesAtTime[k][j].contains(ai)){
						assert(!allActivities.contains(ai));
						allActivities.append(ai);
					}
				
				/* //Now get the activitiy ids. I don't run through the InternalActivitiesList, even that is faster. I run through subgroupsList, because by that the activites are sorted by that in the html-table.
				for(int subgroup=0; subgroup<gt.rules.nInternalSubgroups; subgroup++){
					if(students_timetable_weekly[subgroup][k][j]!=UNALLOCATED_ACTIVITY){
						Activity* act=&gt.rules.internalActivitiesList[students_timetable_weekly[subgroup][k][j]];
						if(act->subjectName==gt.rules.internalSubjectsList[subject]->name)
							if(!(allActivities.contains(students_timetable_weekly[subgroup][k][j]))){
								allActivities<<students_timetable_weekly[subgroup][k][j];
							}
					}
				}
				//Now run through the teachers timetable, because activities without a students set are still missing.
				for(int teacher=0; teacher<gt.rules.nInternalTeachers; teacher++){
					if(teachers_timetable_weekly[teacher][k][j]!=UNALLOCATED_ACTIVITY){
						Activity* act=&gt.rules.internalActivitiesList[teachers_timetable_weekly[teacher][k][j]];
						if(act->subjectName==gt.rules.internalSubjectsList[subject]->name)
							if(!(allActivities.contains(teachers_timetable_weekly[teacher][k][j]))){
								assert(act->studentsNames.isEmpty());
								allActivities<<teachers_timetable_weekly[teacher][k][j];
							}
					}
				}*/
				addActivitiesWithSameStartingTime(allActivities, j);
				tos<<writeActivitiesSubjects(allActivities);
			}
			tos<<"        </tr>\n";
		}
	}
	//workaround begin. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
	tos<<"        <tr class=\"foot\"><td colspan=\"2\"></td><td colspan=\""<<gt.rules.nInternalSubjects<<"\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr>\n";
	//workaround end.
	tos<<"      </tbody>\n    </table>\n";
	tos << "  </body>\n</html>\n";

	if(file.error()>0){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Writing %1 gave error code %2, which means saving is compromised. Please check your disk's free space.").arg(htmlfilename).arg(file.error()));
	}
	file.close();
}

//XHTML generation code by Volker Dirr (http://timetabling.de/)
void TimetableExport::writeSubjectsTimetableTimeHorizontalHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities){
	assert(gt.rules.initialized && gt.rules.internalStructureComputed);
	//assert(gt.timePopulation.initialized);
	assert(students_schedule_ready && teachers_schedule_ready && rooms_schedule_ready);

	//Now we print the results to an HTML file
	QFile file(htmlfilename);
	if(!file.open(QIODevice::WriteOnly)){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Cannot open file %1 for writing. Please check your disk's free space. Saving of %1 aborted.").arg(htmlfilename));
		return;
		assert(0);
	}
	QTextStream tos(&file);
	tos.setCodec("UTF-8");
	tos.setGenerateByteOrderMark(true);

	tos<<writeHead(true, placedActivities, false);

	tos<<"    <table id=\"table\" border=\"1\">\n";

	tos<<"      <caption>"<<protect2(gt.rules.institutionName)<<"</caption>\n";

	tos<<"      <thead>\n        <tr><td rowspan=\"2\"></td>";

	for(int k=0; k<gt.rules.nDaysPerWeek; k++)
		tos << "<th colspan=\"" << gt.rules.nHoursPerDay <<"\">" << protect2(gt.rules.daysOfTheWeek[k]) << "</th>";
	tos <<"</tr>\n";
	tos<<"        <tr>\n          <!-- span -->\n";
	for(int k=0; k<gt.rules.nDaysPerWeek; k++)
		for(int j=0; j<gt.rules.nHoursPerDay; j++){
			if(TIMETABLE_HTML_LEVEL>=2)
				tos<<"          <th class=\"xAxis\">";
			else
				tos<<"          <th>";
			tos << protect2(gt.rules.hoursOfTheDay[j]) << "</th>\n";
		}
	tos<<"        </tr>\n";
	tos<<"      </thead>\n";
	/*workaround. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
	tos<<"      <tfoot><tr><td colspan=\"2\"></td><td colspan=\""<<gt.rules.nInternalSubgroups<<"\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr></tfoot>\n";
	*/
	tos<<"      <tbody>\n";
	for(int subject=0; subject<gt.rules.nInternalSubjects; subject++){
		tos << "        <tr>\n";
		if(TIMETABLE_HTML_LEVEL>=2)
			tos << "        <th class=\"yAxis\">" << protect2(gt.rules.internalSubjectsList[subject]->name) << "</th>\n";
		else
			tos << "        <th>" << protect2(gt.rules.internalSubjectsList[subject]->name) << "</th>\n";

		///////Liviu here
		for(int d=0; d<gt.rules.nDaysPerWeek; d++)
			for(int h=0; h<gt.rules.nHoursPerDay; h++)
				activitiesForCurrentSubject[d][h].clear();
		foreach(int ai, gt.rules.activitiesForSubject[subject])
			if(best_solution.times[ai]!=UNALLOCATED_TIME){
				int d=best_solution.times[ai]%gt.rules.nDaysPerWeek;
				int h=best_solution.times[ai]/gt.rules.nDaysPerWeek;
				Activity* act=&gt.rules.internalActivitiesList[ai];
				for(int dd=0; dd < act->duration && h+dd < gt.rules.nHoursPerDay; dd++)
					activitiesForCurrentSubject[d][h+dd].append(ai);
			}
		///////end Liviu

		for(int k=0; k<gt.rules.nDaysPerWeek; k++){
			for(int j=0; j<gt.rules.nHoursPerDay; j++){
				QList<int> allActivities;

				allActivities=activitiesForCurrentSubject[k][j];


				/*allActivities.clear();
				//Now get the activitiy ids. I don't run through the InternalActivitiesList, even that is faster. I run through subgroupsList, because by that the activites are sorted by that in the html-table.
				for(int subgroup=0; subgroup<gt.rules.nInternalSubgroups; subgroup++){
					if(students_timetable_weekly[subgroup][k][j]!=UNALLOCATED_ACTIVITY){
						Activity* act=&gt.rules.internalActivitiesList[students_timetable_weekly[subgroup][k][j]];
						if(act->subjectName==gt.rules.internalSubjectsList[subject]->name)
							if(!(allActivities.contains(students_timetable_weekly[subgroup][k][j]))){
								allActivities<<students_timetable_weekly[subgroup][k][j];
							}
					}
				}
				//Now run through the teachers timetable, because activities without a students set are still missing.
				for(int teacher=0; teacher<gt.rules.nInternalTeachers; teacher++){
					if(teachers_timetable_weekly[teacher][k][j]!=UNALLOCATED_ACTIVITY){
						Activity* act=&gt.rules.internalActivitiesList[teachers_timetable_weekly[teacher][k][j]];
						if(act->subjectName==gt.rules.internalSubjectsList[subject]->name)
							if(!(allActivities.contains(teachers_timetable_weekly[teacher][k][j]))){
								assert(act->studentsNames.isEmpty());
								allActivities<<teachers_timetable_weekly[teacher][k][j];
							}
					}
				}*/
				addActivitiesWithSameStartingTime(allActivities, j);
				tos<<writeActivitiesSubjects(allActivities);
			}
		}
		tos<<"        </tr>\n";
	}
	//workaround begin. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
	tos<<"        <tr class=\"foot\"><td></td><td colspan=\""<<gt.rules.nHoursPerDay*gt.rules.nDaysPerWeek<<"\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr>\n";
	//workaround end.
	tos<<"      </tbody>\n    </table>\n";
	tos << "  </body>\n</html>\n";

	if(file.error()>0){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Writing %1 gave error code %2, which means saving is compromised. Please check your disk's free space.").arg(htmlfilename).arg(file.error()));
	}
	file.close();
}


//XHTML generation code by Volker Dirr (http://timetabling.de/)
void TimetableExport::writeSubjectsTimetableTimeVerticalDailyHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities){
	assert(gt.rules.initialized && gt.rules.internalStructureComputed);
	//assert(gt.timePopulation.initialized);
	assert(students_schedule_ready && teachers_schedule_ready && rooms_schedule_ready);

	//Now we print the results to an HTML file
	QFile file(htmlfilename);
	if(!file.open(QIODevice::WriteOnly)){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Cannot open file %1 for writing. Please check your disk's free space. Saving of %1 aborted.").arg(htmlfilename));
		return;
		assert(0);
	}
	QTextStream tos(&file);
	tos.setCodec("UTF-8");
	tos.setGenerateByteOrderMark(true);

	tos<<writeHead(true, placedActivities, true);
	tos<<writeTOCDays(false);

	//LIVIU										//maybe TODO: write a function for this
	//already computed
	/*for(int k=0; k<gt.rules.nDaysPerWeek; k++)
		for(int j=0; j<gt.rules.nHoursPerDay; j++)
			activitiesAtTime[k][j].clear();
	for(int i=0; i<gt.rules.nInternalActivities; i++) {
		Activity* act=&gt.rules.internalActivitiesList[i];
		if(best_solution.times[i]!=UNALLOCATED_TIME) {
			int hour=best_solution.times[i]/gt.rules.nDaysPerWeek;
			int day=best_solution.times[i]%gt.rules.nDaysPerWeek;
			for(int dd=0; dd < act->duration && hour+dd < gt.rules.nHoursPerDay; dd++)
				activitiesAtTime[day][hour+dd].append(i);
		}
	}*/
	///////


	for(int k=0; k<gt.rules.nDaysPerWeek; k++){
		tos<<"    <table id=\"table_"<<hashDayIDsTimetable.value(gt.rules.daysOfTheWeek[k])<<"\" border=\"1\">\n";
		tos<<"      <caption>"<<protect2(gt.rules.institutionName)<<"</caption>\n";
		tos<<"      <thead>\n        <tr><td colspan=\"2\"></td>";
		for(int i=0; i<gt.rules.nInternalSubjects; i++){
				if(TIMETABLE_HTML_LEVEL>=2)
					tos<<"          <th class=\"xAxis\">";
				else
					tos<<"          <th>";
				tos << gt.rules.internalSubjectsList[i]->name << "</th>";
		}
		tos<<"</tr>\n      </thead>\n";
		/*workaround. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
		tos<<"      <tfoot><tr><td colspan=\"2\"></td><td colspan=\""<<gt.rules.internalSubjectsList.size()<<"\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr></tfoot>\n";
		*/
		tos<<"      <tbody>\n";
		for(int j=0; j<gt.rules.nHoursPerDay; j++){
			tos << "        <tr>\n";
			if(j==0)
				tos << "        <th rowspan=\"" << gt.rules.nHoursPerDay  << "\">" << protect2vert(gt.rules.daysOfTheWeek[k]) << "</th>\n";
			else tos <<"          <!-- span -->\n";
			if(TIMETABLE_HTML_LEVEL>=2)
				tos<<"          <th class=\"yAxis\">";
			else
				tos<<"          <th>";
			tos << protect2(gt.rules.hoursOfTheDay[j]) << "</th>\n";


			for(int subject=0; subject<gt.rules.nInternalSubjects; subject++){
				QList<int> allActivities;
				allActivities.clear();
				
				foreach(int ai, gt.rules.activitiesForSubject[subject])
					if(activitiesAtTime[k][j].contains(ai)){
						assert(!allActivities.contains(ai));
						allActivities.append(ai);
					}
				
				/*//Now get the activitiy ids. I don't run through the InternalActivitiesList, even that is faster. I run through subgroupsList, because by that the activites are sorted by that in the html-table.
				for(int subgroup=0; subgroup<gt.rules.nInternalSubgroups; subgroup++){
					if(students_timetable_weekly[subgroup][k][j]!=UNALLOCATED_ACTIVITY){
						Activity* act=&gt.rules.internalActivitiesList[students_timetable_weekly[subgroup][k][j]];
						if(act->subjectName==gt.rules.internalSubjectsList[subject]->name)
							if(!(allActivities.contains(students_timetable_weekly[subgroup][k][j]))){
								allActivities<<students_timetable_weekly[subgroup][k][j];
							}
					}
				}
				//Now run through the teachers timetable, because activities without a students set are still missing.
				for(int teacher=0; teacher<gt.rules.nInternalTeachers; teacher++){
					if(teachers_timetable_weekly[teacher][k][j]!=UNALLOCATED_ACTIVITY){
						Activity* act=&gt.rules.internalActivitiesList[teachers_timetable_weekly[teacher][k][j]];
						if(act->subjectName==gt.rules.internalSubjectsList[subject]->name)
							if(!(allActivities.contains(teachers_timetable_weekly[teacher][k][j]))){
								assert(act->studentsNames.isEmpty());
								allActivities<<teachers_timetable_weekly[teacher][k][j];
							}
					}
				}*/
				addActivitiesWithSameStartingTime(allActivities, j);
				tos<<writeActivitiesSubjects(allActivities);
			}
			tos<<"        </tr>\n";
		}
		//workaround begin. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
		tos<<"        <tr class=\"foot\"><td colspan=\"2\"></td><td colspan=\""<<gt.rules.nInternalSubjects<<"\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr>\n";
		//workaround end.
		tos<<"      </tbody>\n";
		tos<<"    </table>\n\n";
		tos<<"    <p class=\"back\"><a href=\""<<"#top\">"<<TimetableExport::tr("back to the top")<<"</a></p>\n\n";
	}

	tos << "  </body>\n</html>\n";

	if(file.error()>0){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Writing %1 gave error code %2, which means saving is compromised. Please check your disk's free space.").arg(htmlfilename).arg(file.error()));
	}
	file.close();
}

//XHTML generation code by Volker Dirr (http://timetabling.de/)
void TimetableExport::writeSubjectsTimetableTimeHorizontalDailyHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities){
	assert(gt.rules.initialized && gt.rules.internalStructureComputed);
	//assert(gt.timePopulation.initialized);
	assert(students_schedule_ready && teachers_schedule_ready && rooms_schedule_ready);

	//Now we print the results to an HTML file
	QFile file(htmlfilename);
	if(!file.open(QIODevice::WriteOnly)){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Cannot open file %1 for writing. Please check your disk's free space. Saving of %1 aborted.").arg(htmlfilename));
		return;
		assert(0);
	}
	QTextStream tos(&file);
	tos.setCodec("UTF-8");
	tos.setGenerateByteOrderMark(true);

	tos<<writeHead(true, placedActivities, true);
	tos<<writeTOCDays(false);

	for(int k=0; k<gt.rules.nDaysPerWeek; k++){
		tos<<"    <table id=\"table_"<<hashDayIDsTimetable.value(gt.rules.daysOfTheWeek[k])<<"\" border=\"1\">\n";
		tos<<"      <caption>"<<protect2(gt.rules.institutionName)<<"</caption>\n";
		tos<<"      <thead>\n        <tr><td rowspan=\"2\"></td>";
	
		tos << "<th colspan=\"" << gt.rules.nHoursPerDay <<"\">" << protect2(gt.rules.daysOfTheWeek[k]) << "</th>";
		tos <<"</tr>\n";
		tos<<"        <tr>\n          <!-- span -->\n";
		for(int j=0; j<gt.rules.nHoursPerDay; j++){
			if(TIMETABLE_HTML_LEVEL>=2)
				tos<<"          <th class=\"xAxis\">";
			else
				tos<<"          <th>";
			tos << protect2(gt.rules.hoursOfTheDay[j]) << "</th>\n";
		}
		tos<<"        </tr>\n";
		tos<<"      </thead>\n";
		/*workaround. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
		tos<<"      <tfoot><tr><td colspan=\"2\"></td><td colspan=\""<<gt.rules.nHoursPerDay<<"\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr></tfoot>\n";
		*/
		tos<<"      <tbody>\n";
		for(int subject=0; subject<gt.rules.nInternalSubjects; subject++){
			tos << "        <tr>\n";
			if(TIMETABLE_HTML_LEVEL>=2)
				tos << "        <th class=\"yAxis\">" << protect2(gt.rules.internalSubjectsList[subject]->name) << "</th>\n";
			else
				tos << "        <th>" << protect2(gt.rules.internalSubjectsList[subject]->name) << "</th>\n";
	
			///////Liviu here
			for(int d=0; d<gt.rules.nDaysPerWeek; d++)
				for(int h=0; h<gt.rules.nHoursPerDay; h++)
					activitiesForCurrentSubject[d][h].clear();
			foreach(int ai, gt.rules.activitiesForSubject[subject])
				if(best_solution.times[ai]!=UNALLOCATED_TIME){
					int d=best_solution.times[ai]%gt.rules.nDaysPerWeek;
					int h=best_solution.times[ai]/gt.rules.nDaysPerWeek;
					Activity* act=&gt.rules.internalActivitiesList[ai];
					for(int dd=0; dd < act->duration && h+dd < gt.rules.nHoursPerDay; dd++)
						activitiesForCurrentSubject[d][h+dd].append(ai);
				}
			///////end Liviu

			for(int j=0; j<gt.rules.nHoursPerDay; j++){
				QList<int> allActivities;

				allActivities=activitiesForCurrentSubject[k][j];


				/*allActivities.clear();
				//Now get the activitiy ids. I don't run through the InternalActivitiesList, even that is faster. I run through subgroupsList, because by that the activites are sorted by that in the html-table.
				for(int subgroup=0; subgroup<gt.rules.nInternalSubgroups; subgroup++){
					if(students_timetable_weekly[subgroup][k][j]!=UNALLOCATED_ACTIVITY){
						Activity* act=&gt.rules.internalActivitiesList[students_timetable_weekly[subgroup][k][j]];
						if(act->subjectName==gt.rules.internalSubjectsList[subject]->name)
							if(!(allActivities.contains(students_timetable_weekly[subgroup][k][j]))){
								allActivities<<students_timetable_weekly[subgroup][k][j];
							}
					}
				}
				//Now run through the teachers timetable, because activities without a students set are still missing.
				for(int teacher=0; teacher<gt.rules.nInternalTeachers; teacher++){
					if(teachers_timetable_weekly[teacher][k][j]!=UNALLOCATED_ACTIVITY){
						Activity* act=&gt.rules.internalActivitiesList[teachers_timetable_weekly[teacher][k][j]];
						if(act->subjectName==gt.rules.internalSubjectsList[subject]->name)
							if(!(allActivities.contains(teachers_timetable_weekly[teacher][k][j]))){
								assert(act->studentsNames.isEmpty());
								allActivities<<teachers_timetable_weekly[teacher][k][j];
							}
					}
				}*/
				addActivitiesWithSameStartingTime(allActivities, j);
				tos<<writeActivitiesSubjects(allActivities);
			}
			tos<<"        </tr>\n";
		}
		//workaround begin. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
		tos<<"        <tr class=\"foot\"><td></td><td colspan=\""<<gt.rules.nHoursPerDay<<"\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr>\n";
		//workaround end.
		tos<<"      </tbody>\n";
		tos<<"    </table>\n\n";
		tos<<"    <p class=\"back\"><a href=\""<<"#top\">"<<TimetableExport::tr("back to the top")<<"</a></p>\n\n";
	}
	tos << "  </body>\n</html>\n";

	if(file.error()>0){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Writing %1 gave error code %2, which means saving is compromised. Please check your disk's free space.").arg(htmlfilename).arg(file.error()));
	}
	file.close();
}


// Now print the teachers free periods. Code by Volker Dirr (http://timetabling.de/)
// ---------------------------------------------------------------------------------
void TimetableExport::writeTeachersFreePeriodsTimetableDaysHorizontalHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities){
	assert(gt.rules.initialized && gt.rules.internalStructureComputed);
	//assert(gt.timePopulation.initialized);
	assert(students_schedule_ready && teachers_schedule_ready && rooms_schedule_ready);

	//Now we print the results to an HTML file
	QFile file(htmlfilename);
	if(!file.open(QIODevice::WriteOnly)){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Cannot open file %1 for writing. Please check your disk's free space. Saving of %1 aborted.").arg(htmlfilename));
		return;
		assert(0);
	}
	QTextStream tos(&file);
	tos.setCodec("UTF-8");
	tos.setGenerateByteOrderMark(true);

	tos<<writeHead(true, placedActivities, true);

	tos<<"    <ul>\n";
	tos<<"      <li><a href=\""<<"#table_DETAILED\">"<<TimetableExport::tr("Teachers' Free Periods")<<" ("<<tr("Detailed")<<")</a></li>\n";
	tos<<"      <li><a href=\""<<"#table_LESS_DETAILED\">"<<TimetableExport::tr("Teachers' Free Periods")<<" ("<<tr("Less detailed")<<")</a></li>\n";
	tos<<"    </ul>\n    <p>&nbsp;</p>\n\n";

	tos<<"    <div class=\"TEACHER_HAS_SINGLE_GAP\">"<<TimetableExport::tr("Teacher has a single gap")<<"</div>\n";
	tos<<"    <div class=\"TEACHER_HAS_BORDER_GAP\">"<<TimetableExport::tr("Teacher has a border gap")<<"</div>\n";
	tos<<"    <div class=\"TEACHER_HAS_BIG_GAP\">"<<TimetableExport::tr("Teacher has a big gap")<<"</div>\n";
	tos<<"    <div class=\"TEACHER_MUST_COME_EARLIER\">"<<TimetableExport::tr("Teacher must come earlier")<<"</div>\n";
	tos<<"    <div class=\"TEACHER_MUST_COME_MUCH_EARLIER\">"<<TimetableExport::tr("Teacher must come much earlier")<<"</div>\n";
	tos<<"    <div class=\"TEACHER_MUST_STAY_LONGER\">"<<TimetableExport::tr("Teacher must stay longer")<<"</div>\n";
	tos<<"    <div class=\"TEACHER_MUST_STAY_MUCH_LONGER\">"<<TimetableExport::tr("Teacher must stay much longer")<<"</div>\n";
	tos<<"    <div class=\"TEACHER_HAS_A_FREE_DAY\">"<<TimetableExport::tr("Teacher has a free day")<<"</div>\n";
	tos<<"    <div class=\"TEACHER_IS_NOT_AVAILABLE\">"<<TimetableExport::tr("Teacher is not available")<<"</div>\n";

	tos<<"    <p>&nbsp;</p>\n\n";
	
	bool PRINT_DETAILED=true;
	do{
		if(PRINT_DETAILED==true)
			tos<<"    <table id=\"table_DETAILED\" border=\"1\">\n";
		else
			tos<<"    <table id=\"table_LESS_DETAILED\" border=\"1\">\n";
		
		tos<<"      <caption>"<<protect2(gt.rules.institutionName)<<"</caption>\n";

		if(PRINT_DETAILED==true)
			tos<<"      <thead>\n        <tr><td rowspan=\"2\"></td><th colspan=\""<<gt.rules.nDaysPerWeek<<"\">"<<TimetableExport::tr("Teachers' Free Periods")<<" ("<<tr("Detailed")<<")</th></tr>\n";
		else	tos<<"      <thead>\n        <tr><td rowspan=\"2\"></td><th colspan=\""<<gt.rules.nDaysPerWeek<<"\">"<<TimetableExport::tr("Teachers' Free Periods")<<" ("<<tr("Less detailed")<<")</th></tr>\n";

		tos<<"        <tr>\n          <!-- span -->\n";
		for(int j=0; j<gt.rules.nDaysPerWeek; j++){
			if(TIMETABLE_HTML_LEVEL>=2)
				tos<<"          <th class=\"xAxis\">";
			else
				tos<<"          <th>";
			tos<<protect2(gt.rules.daysOfTheWeek[j])<<"</th>\n";
		}
		tos<<"        </tr>\n";
		tos<<"      </thead>\n";
		/*workaround. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
		tos<<"      <tfoot><tr><td></td><td colspan=\""<<gt.rules.nDaysPerWeek<<"\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr></tfoot>\n";
		*/
		tos<<"      <tbody>\n";
		for(int j=0; j<gt.rules.nHoursPerDay; j++){
			tos<<"        <tr>\n";
			if(TIMETABLE_HTML_LEVEL>=2)
				tos<<"          <th class=\"yAxis\">";
			else
				tos<<"          <th>";
			tos<<protect2(gt.rules.hoursOfTheDay[j])<<"</th>\n";
			for(int k=0; k<gt.rules.nDaysPerWeek; k++){
				bool empty_slot;
				empty_slot=true;
				for(int tfp=0; tfp<TEACHERS_FREE_PERIODS_N_CATEGORIES; tfp++){
					if(teachers_free_periods_timetable_weekly[tfp][k][j].size()>0){
						empty_slot=false;
					}
					if(PRINT_DETAILED==false&&tfp>=TEACHER_MUST_COME_EARLIER) break;
				}
				if(!empty_slot) tos<<"          <td>";
				for(int tfp=0; tfp<TEACHERS_FREE_PERIODS_N_CATEGORIES; tfp++){
					if(teachers_free_periods_timetable_weekly[tfp][k][j].size()>0){
						if(TIMETABLE_HTML_LEVEL>=2)
							tos<<"<div class=\"DESCRIPTION\">";
						switch(tfp){
							case TEACHER_HAS_SINGLE_GAP		: tos<<TimetableExport::tr("Single gap"); break;
							case TEACHER_HAS_BORDER_GAP		: tos<<TimetableExport::tr("Border gap"); break;
							case TEACHER_HAS_BIG_GAP		: tos<<TimetableExport::tr("Big gap"); break;
							case TEACHER_MUST_COME_EARLIER		: tos<<TimetableExport::tr("Must come earlier"); break;
							case TEACHER_MUST_STAY_LONGER		: tos<<TimetableExport::tr("Must stay longer"); break;
							case TEACHER_MUST_COME_MUCH_EARLIER	: tos<<TimetableExport::tr("Must come much earlier"); break;
							case TEACHER_MUST_STAY_MUCH_LONGER	: tos<<TimetableExport::tr("Must stay much longer"); break;
							case TEACHER_HAS_A_FREE_DAY		: tos<<TimetableExport::tr("Free day"); break;
							case TEACHER_IS_NOT_AVAILABLE		: tos<<TimetableExport::tr("Not available"); break;
							default: assert(0==1); break;
						}
						if(TIMETABLE_HTML_LEVEL>=2)
							tos<<":</div>";
						else tos<<":<br />";
						if(TIMETABLE_HTML_LEVEL>=3)
							switch(tfp){
								case TEACHER_HAS_SINGLE_GAP		: tos<<"<div class=\"TEACHER_HAS_SINGLE_GAP\">"; break;
								case TEACHER_HAS_BORDER_GAP		: tos<<"<div class=\"TEACHER_HAS_BORDER_GAP\">"; break;
								case TEACHER_HAS_BIG_GAP		: tos<<"<div class=\"TEACHER_HAS_BIG_GAP\">"; break;
								case TEACHER_MUST_COME_EARLIER		: tos<<"<div class=\"TEACHER_MUST_COME_EARLIER\">"; break;
								case TEACHER_MUST_STAY_LONGER		: tos<<"<div class=\"TEACHER_MUST_STAY_LONGER\">"; break;
								case TEACHER_MUST_COME_MUCH_EARLIER	: tos<<"<div class=\"TEACHER_MUST_COME_MUCH_EARLIER\">"; break;
								case TEACHER_MUST_STAY_MUCH_LONGER	: tos<<"<div class=\"TEACHER_MUST_STAY_MUCH_LONGER\">"; break;
								case TEACHER_HAS_A_FREE_DAY		: tos<<"<div class=\"TEACHER_HAS_A_FREE_DAY\">"; break;
								case TEACHER_IS_NOT_AVAILABLE		: tos<<"<div class=\"TEACHER_IS_NOT_AVAILABLE\">"; break;
								default: assert(0==1); break;
							}
						for(int t=0; t<teachers_free_periods_timetable_weekly[tfp][k][j].size(); t++){
							QString teacher_name = gt.rules.internalTeachersList[teachers_free_periods_timetable_weekly[tfp][k][j].at(t)]->name;
								switch(TIMETABLE_HTML_LEVEL){
									case 4 : tos<<"<span class=\"t_"<<hashTeacherIDsTimetable.value(teacher_name)<<"\">"<<protect2(teacher_name)<<"</span>"; break;
									case 5 : ;
									case 6 : tos<<"<span class=\"t_"<<hashTeacherIDsTimetable.value(teacher_name)<<"\" onmouseover=\"highlight('t_"<<hashTeacherIDsTimetable.value(teacher_name)<<"')\">"<<protect2(teacher_name)<<"</span>"; break;
									default: tos<<protect2(teacher_name); break;
								}
							tos<<"<br />";
						}
						if(TIMETABLE_HTML_LEVEL>=3)
							tos<<"</div>";
					}
					if(PRINT_DETAILED==false&&tfp>=TEACHER_MUST_COME_EARLIER) break;
				}
				if(!empty_slot){
					tos<<"</td>\n";
				} else {
					tos<<writeEmpty();
				}
			}
			tos<<"        </tr>\n";
		}
		//workaround begin. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
		tos<<"        <tr class=\"foot\"><td></td><td colspan=\""<<gt.rules.nDaysPerWeek<<"\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr>\n";
		//workaround end.
		tos<<"      </tbody>\n";
		tos<<"    </table>\n\n";
		tos<<"    <p class=\"back\"><a href=\""<<"#top\">"<<TimetableExport::tr("back to the top")<<"</a></p>\n\n";
		if(PRINT_DETAILED==true) PRINT_DETAILED=false;
		else PRINT_DETAILED=true;
	} while(PRINT_DETAILED!=true);
	tos<<"  </body>\n</html>\n";

	if(file.error()>0){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Writing %1 gave error code %2, which means saving is compromised. Please check your disk's free space.").arg(htmlfilename).arg(file.error()));
	}
	file.close();
}

//XHTML generation code by Volker Dirr (http://timetabling.de/)
void TimetableExport::writeTeachersFreePeriodsTimetableDaysVerticalHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities){
	assert(gt.rules.initialized && gt.rules.internalStructureComputed);
	//assert(gt.timePopulation.initialized);
	assert(students_schedule_ready && teachers_schedule_ready && rooms_schedule_ready);

	//Now we print the results to an HTML file
	QFile file(htmlfilename);
	if(!file.open(QIODevice::WriteOnly)){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Cannot open file %1 for writing. Please check your disk's free space. Saving of %1 aborted.").arg(htmlfilename));
		return;
		assert(0);
	}
	QTextStream tos(&file);
	tos.setCodec("UTF-8");
	tos.setGenerateByteOrderMark(true);

	tos<<writeHead(true, placedActivities, true);

	tos<<"    <ul>\n";
	tos<<"      <li><a href=\""<<"#table_DETAILED\">"<<TimetableExport::tr("Teachers' Free Periods")<<" ("<<tr("Detailed")<<")</a></li>\n";
	tos<<"      <li><a href=\""<<"#table_LESS_DETAILED\">"<<TimetableExport::tr("Teachers' Free Periods")<<" ("<<tr("Less detailed")<<")</a></li>\n";
	tos<<"    </ul>\n    <p>&nbsp;</p>\n\n";

	tos<<"    <div class=\"TEACHER_HAS_SINGLE_GAP\">"<<TimetableExport::tr("Teacher has a single gap")<<"</div>\n";
	tos<<"    <div class=\"TEACHER_HAS_BORDER_GAP\">"<<TimetableExport::tr("Teacher has a border gap")<<"</div>\n";
	tos<<"    <div class=\"TEACHER_HAS_BIG_GAP\">"<<TimetableExport::tr("Teacher has a big gap")<<"</div>\n";
	tos<<"    <div class=\"TEACHER_MUST_COME_EARLIER\">"<<TimetableExport::tr("Teacher must come earlier")<<"</div>\n";
	tos<<"    <div class=\"TEACHER_MUST_COME_MUCH_EARLIER\">"<<TimetableExport::tr("Teacher must come much earlier")<<"</div>\n";
	tos<<"    <div class=\"TEACHER_MUST_STAY_LONGER\">"<<TimetableExport::tr("Teacher must stay longer")<<"</div>\n";
	tos<<"    <div class=\"TEACHER_MUST_STAY_MUCH_LONGER\">"<<TimetableExport::tr("Teacher must stay much longer")<<"</div>\n";
	tos<<"    <div class=\"TEACHER_HAS_A_FREE_DAY\">"<<TimetableExport::tr("Teacher has a free day")<<"</div>\n";
	tos<<"    <div class=\"TEACHER_IS_NOT_AVAILABLE\">"<<TimetableExport::tr("Teacher is not available")<<"</div>\n";

	tos<<"    <p>&nbsp;</p>\n\n";

	bool PRINT_DETAILED=true;
	do{
		if(PRINT_DETAILED==true)
			tos<<"    <table id=\"table_DETAILED\" border=\"1\">\n";
		else
			tos<<"    <table id=\"table_LESS_DETAILED\" border=\"1\">\n";
		
		tos<<"      <caption>"<<protect2(gt.rules.institutionName)<<"</caption>\n";

		if(PRINT_DETAILED==true)
			tos<<"      <thead>\n        <tr><td rowspan=\"2\"></td><th colspan=\""<<gt.rules.nHoursPerDay<<"\">"<<TimetableExport::tr("Teachers' Free Periods")<<" ("<<tr("Detailed")<<")</th></tr>\n";
		else	tos<<"      <thead>\n        <tr><td rowspan=\"2\"></td><th colspan=\""<<gt.rules.nHoursPerDay<<"\">"<<TimetableExport::tr("Teachers' Free Periods")<<" ("<<tr("Less detailed")<<")</th></tr>\n";

		tos<<"        <tr>\n          <!-- span -->\n";
		for(int j=0; j<gt.rules.nHoursPerDay; j++){
			if(TIMETABLE_HTML_LEVEL>=2)
				tos<<"          <th class=\"xAxis\">";
			else
				tos<<"          <th>";
			tos<<protect2(gt.rules.hoursOfTheDay[j])<<"</th>\n";
		}
		tos<<"        </tr>\n";
		tos<<"      </thead>\n";
		/*workaround. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
		tos<<"      <tfoot><tr><td></td><td colspan=\""<<gt.rules.nHoursPerDay<<"\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr></tfoot>\n";
		*/
		tos<<"      <tbody>\n";
		for(int k=0; k<gt.rules.nDaysPerWeek; k++){
			tos<<"        <tr>\n";
			if(TIMETABLE_HTML_LEVEL>=2)
				tos<<"          <th class=\"yAxis\">";
			else
				tos<<"          <th>";
			tos<<protect2(gt.rules.daysOfTheWeek[k])<<"</th>\n";
			for(int j=0; j<gt.rules.nHoursPerDay; j++){
				bool empty_slot;
				empty_slot=true;
				for(int tfp=0; tfp<TEACHERS_FREE_PERIODS_N_CATEGORIES; tfp++){
					if(teachers_free_periods_timetable_weekly[tfp][k][j].size()>0){
						empty_slot=false;
					}
					if(PRINT_DETAILED==false&&tfp>=TEACHER_MUST_COME_EARLIER) break;
				}
				if(!empty_slot) tos<<"          <td>";
				for(int tfp=0; tfp<TEACHERS_FREE_PERIODS_N_CATEGORIES; tfp++){
					if(teachers_free_periods_timetable_weekly[tfp][k][j].size()>0){
						if(TIMETABLE_HTML_LEVEL>=2)
							tos<<"<div class=\"DESCRIPTION\">";
						switch(tfp){
							case TEACHER_HAS_SINGLE_GAP		: tos<<TimetableExport::tr("Single gap"); break;
							case TEACHER_HAS_BORDER_GAP		: tos<<TimetableExport::tr("Border gap"); break;
							case TEACHER_HAS_BIG_GAP		: tos<<TimetableExport::tr("Big gap"); break;
							case TEACHER_MUST_COME_EARLIER		: tos<<TimetableExport::tr("Must come earlier"); break;
							case TEACHER_MUST_STAY_LONGER		: tos<<TimetableExport::tr("Must stay longer"); break;
							case TEACHER_MUST_COME_MUCH_EARLIER	: tos<<TimetableExport::tr("Must come much earlier"); break;
							case TEACHER_MUST_STAY_MUCH_LONGER	: tos<<TimetableExport::tr("Must stay much longer"); break;
							case TEACHER_HAS_A_FREE_DAY		: tos<<TimetableExport::tr("Free day"); break;
							case TEACHER_IS_NOT_AVAILABLE		: tos<<TimetableExport::tr("Not available"); break;
							default: assert(0==1); break;
						}
						if(TIMETABLE_HTML_LEVEL>=2)
							tos<<":</div>";
						else tos<<":<br />";
						if(TIMETABLE_HTML_LEVEL>=3)
							switch(tfp){
								case TEACHER_HAS_SINGLE_GAP		: tos<<"<div class=\"TEACHER_HAS_SINGLE_GAP\">"; break;
								case TEACHER_HAS_BORDER_GAP		: tos<<"<div class=\"TEACHER_HAS_BORDER_GAP\">"; break;
								case TEACHER_HAS_BIG_GAP		: tos<<"<div class=\"TEACHER_HAS_BIG_GAP\">"; break;
								case TEACHER_MUST_COME_EARLIER		: tos<<"<div class=\"TEACHER_MUST_COME_EARLIER\">"; break;
								case TEACHER_MUST_STAY_LONGER		: tos<<"<div class=\"TEACHER_MUST_STAY_LONGER\">"; break;
								case TEACHER_MUST_COME_MUCH_EARLIER	: tos<<"<div class=\"TEACHER_MUST_COME_MUCH_EARLIER\">"; break;
								case TEACHER_MUST_STAY_MUCH_LONGER	: tos<<"<div class=\"TEACHER_MUST_STAY_MUCH_LONGER\">"; break;
								case TEACHER_HAS_A_FREE_DAY		: tos<<"<div class=\"TEACHER_HAS_A_FREE_DAY\">"; break;
								case TEACHER_IS_NOT_AVAILABLE		: tos<<"<div class=\"TEACHER_IS_NOT_AVAILABLE\">"; break;
								default: assert(0==1); break;
							}
						for(int t=0; t<teachers_free_periods_timetable_weekly[tfp][k][j].size(); t++){
							QString teacher_name = gt.rules.internalTeachersList[teachers_free_periods_timetable_weekly[tfp][k][j].at(t)]->name;
								switch(TIMETABLE_HTML_LEVEL){
									case 4 : tos<<"<span class=\"t_"<<hashTeacherIDsTimetable.value(teacher_name)<<"\">"<<protect2(teacher_name)<<"</span>"; break;
									case 5 : ;
									case 6 : tos<<"<span class=\"t_"<<hashTeacherIDsTimetable.value(teacher_name)<<"\" onmouseover=\"highlight('t_"<<hashTeacherIDsTimetable.value(teacher_name)<<"')\">"<<protect2(teacher_name)<<"</span>"; break;
									default: tos<<protect2(teacher_name); break;
								}
							tos<<"<br />";
						}
						if(TIMETABLE_HTML_LEVEL>=3)
							tos<<"</div>";
					}
					if(PRINT_DETAILED==false&&tfp>=TEACHER_MUST_COME_EARLIER) break;
				}
				if(!empty_slot){
					tos<<"</td>\n";
				} else
					tos<<writeEmpty();
			}
			tos<<"        </tr>\n";
		}
		//workaround begin. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
		tos<<"        <tr class=\"foot\"><td></td><td colspan=\""<<gt.rules.nHoursPerDay<<"\">"<<TimetableExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr>\n";
		//workaround end.
		tos<<"      </tbody>\n";
		tos<<"    </table>\n\n";
		tos<<"    <p class=\"back\"><a href=\""<<"#top\">"<<TimetableExport::tr("back to the top")<<"</a></p>\n\n";
		if(PRINT_DETAILED==true) PRINT_DETAILED=false;
		else PRINT_DETAILED=true;
	} while(PRINT_DETAILED!=true);
	tos<<"  </body>\n</html>\n";

	if(file.error()>0){
		QMessageBox::critical(NULL, tr("FET critical"),
		 TimetableExport::tr("Writing %1 gave error code %2, which means saving is compromised. Please check your disk's free space.").arg(htmlfilename).arg(file.error()));
	}
	file.close();
}

//------------------------------------------------------------------
//------------------------------------------------------------------

void TimetableExport::computeHashForIDsTimetable(){
// by Volker Dirr

//TODO if an use a relational data base this is unneded, because we can use the primary key id of the database 
//This is very similar to statistics compute hash. so always check it if you change something here!

/*	QSet<QString> usedStudents;
	for(int i=0; i<gt.rules.nInternalActivities; i++){
		foreach(QString st, gt.rules.internalActivitiesList[i].studentsNames){
			if(!usedStudents.contains(st))
				usedStudents<<st;
		}
	}*/
	hashStudentIDsTimetable.clear();
	int cnt=1;
	for(int i=0; i<gt.rules.augmentedYearsList.size(); i++){
		StudentsYear* sty=gt.rules.augmentedYearsList[i];
		//if(usedStudents.contains(sty->name)){
		if(!hashStudentIDsTimetable.contains(sty->name)){
			hashStudentIDsTimetable.insert(sty->name, QString::number(cnt));
			cnt++;
		}
		//}
		for(int j=0; j<sty->groupsList.size(); j++){
			StudentsGroup* stg=sty->groupsList[j];
		//	if(usedStudents.contains(stg->name)){
			if(!hashStudentIDsTimetable.contains(stg->name)){
				hashStudentIDsTimetable.insert(stg->name, QString::number(cnt));
				cnt++;
			}
		//	}
			for(int k=0; k<stg->subgroupsList.size(); k++){
				StudentsSubgroup* sts=stg->subgroupsList[k];
		//		if(usedStudents.contains(sts->name)){
				if(!hashStudentIDsTimetable.contains(sts->name)){
					hashStudentIDsTimetable.insert(sts->name, QString::number(cnt));
					cnt++;
				}
		//		}
			}
		}
	}

	hashSubjectIDsTimetable.clear();
	for(int i=0; i<gt.rules.nInternalSubjects; i++){
		hashSubjectIDsTimetable.insert(gt.rules.internalSubjectsList[i]->name, QString::number(i+1));
	}
	hashActivityTagIDsTimetable.clear();
	for(int i=0; i<gt.rules.nInternalActivityTags; i++){
		hashActivityTagIDsTimetable.insert(gt.rules.internalActivityTagsList[i]->name, QString::number(i+1));
	}
	hashTeacherIDsTimetable.clear();
	for(int i=0; i<gt.rules.nInternalTeachers; i++){
		hashTeacherIDsTimetable.insert(gt.rules.internalTeachersList[i]->name, QString::number(i+1));
	}
	hashRoomIDsTimetable.clear();
	for(int room=0; room<gt.rules.nInternalRooms; room++){
		hashRoomIDsTimetable.insert(gt.rules.internalRoomsList[room]->name, QString::number(room+1));
	}
	hashDayIDsTimetable.clear();
	for(int k=0; k<gt.rules.nDaysPerWeek; k++){
		hashDayIDsTimetable.insert(gt.rules.daysOfTheWeek[k], QString::number(k+1));
	}
}


void TimetableExport::computeActivitiesAtTime(){		// by Liviu Lalescu
	for(int k=0; k<gt.rules.nDaysPerWeek; k++)
		for(int j=0; j<gt.rules.nHoursPerDay; j++)
			activitiesAtTime[k][j].clear();
	for(int i=0; i<gt.rules.nInternalActivities; i++) {		//maybe TODO: maybe it is better to do this sorted by students or teachers?
		Activity* act=&gt.rules.internalActivitiesList[i];
		if(best_solution.times[i]!=UNALLOCATED_TIME) {
			int hour=best_solution.times[i]/gt.rules.nDaysPerWeek;
			int day=best_solution.times[i]%gt.rules.nDaysPerWeek;
			for(int dd=0; dd < act->duration && hour+dd < gt.rules.nHoursPerDay; dd++)
				activitiesAtTime[day][hour+dd].append(i);
		}
	}
}


void TimetableExport::computeActivitiesWithSameStartingTime(){
// by Volker Dirr
	activitiesWithSameStartingTime.clear();

	if(PRINT_ACTIVITIES_WITH_SAME_STARTING_TIME){
		for(int i=0; i<gt.rules.nInternalTimeConstraints; i++){
			TimeConstraint* tc=gt.rules.internalTimeConstraintsList[i];
			if(tc->type==CONSTRAINT_ACTIVITIES_SAME_STARTING_TIME){ //not needed anymore:  && tc->weightPercentage==100
				ConstraintActivitiesSameStartingTime* c=(ConstraintActivitiesSameStartingTime*) tc;
				for(int a=0; a<c->_n_activities; a++){
					//speed improvement
					QList<int> & tmpList=activitiesWithSameStartingTime[c->_activities[a]];
					for(int b=0; b<c->_n_activities; b++){
						if(a!=b){
							if(best_solution.times[c->_activities[a]]==best_solution.times[c->_activities[b]]){ 	//because constraint is maybe not with 100% weight and failed
								if(!tmpList.contains(c->_activities[b])){
									tmpList<<c->_activities[b];
								}
							}
						}
					}
					/*
					QList<int> tmpList;
					if(activitiesWithSameStartingTime.contains(c->_activities[a]))
						tmpList=activitiesWithSameStartingTime.value(c->_activities[a]);
					for(int b=0; b<c->_n_activities; b++){
						if(a!=b){
							if(best_solution.times[c->_activities[a]]==best_solution.times[c->_activities[b]]){ 	//because constraint is maybe not with 100% weight and failed
								if(!tmpList.contains(c->_activities[b])){
									tmpList<<c->_activities[b];
								}
							}
						}
					}
					activitiesWithSameStartingTime.insert(c->_activities[a], tmpList);
					*/
				}
			}
		}
	}
}

bool TimetableExport::addActivitiesWithSameStartingTime(QList<int>& allActivities, int hour){
// by Volker Dirr
	if(PRINT_ACTIVITIES_WITH_SAME_STARTING_TIME){
		bool activitiesWithSameStartingtime=false;
		QList<int> allActivitiesNew;
		foreach(int tmpAct, allActivities){
			allActivitiesNew<<tmpAct;
			if(activitiesWithSameStartingTime.contains(tmpAct)){
				QList<int> sameTimeList=activitiesWithSameStartingTime.value(tmpAct);
				foreach(int sameTimeAct, sameTimeList){
					if(!allActivitiesNew.contains(sameTimeAct) && !allActivities.contains(sameTimeAct)){
						if(best_solution.times[sameTimeAct]!=UNALLOCATED_TIME){
							Activity* act=&gt.rules.internalActivitiesList[sameTimeAct];
							assert(best_solution.times[tmpAct]==best_solution.times[sameTimeAct]);//{
								if((best_solution.times[sameTimeAct]/gt.rules.nDaysPerWeek+(act->duration-1))>=hour){
									allActivitiesNew<<sameTimeAct;
								}
								activitiesWithSameStartingtime=true; //don't add this line in previous if command because of activities with different duration!
							//}
						}
					}
				}
			}
		}
		//allActivities.clear();
		allActivities=allActivitiesNew;
		//allActivitiesNew.clear();
		return activitiesWithSameStartingtime;
	}
	else
		return false;
}


QString TimetableExport::writeHead(bool java, int placedActivities, bool printInstitution){
// by Volker Dirr
	QString tmp;
	tmp+="<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\"\n";
	tmp+="  \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n\n";

	if(LANGUAGE_STYLE_RIGHT_TO_LEFT==false)
		tmp+="<html xmlns=\"http://www.w3.org/1999/xhtml\" lang=\""+LANGUAGE_FOR_HTML+"\" xml:lang=\""+LANGUAGE_FOR_HTML+"\">\n";
	else
		tmp+="<html xmlns=\"http://www.w3.org/1999/xhtml\" lang=\""+LANGUAGE_FOR_HTML+"\" xml:lang=\""+LANGUAGE_FOR_HTML+"\" dir=\"rtl\">\n";
	tmp+="  <head>\n";
	tmp+="    <title>"+protect2(gt.rules.institutionName)+"</title>\n";
	tmp+="    <meta http-equiv=\"Content-Type\" content=\"text/html;charset=utf-8\" />\n";
	if(TIMETABLE_HTML_LEVEL>=1){
		QString cssfilename=INPUT_FILENAME_XML.right(INPUT_FILENAME_XML.length()-INPUT_FILENAME_XML.findRev(FILE_SEP)-1);
		
		if(cssfilename.right(4)==".fet")
			cssfilename=cssfilename.left(cssfilename.length()-4);
		//else if(INPUT_FILENAME_XML!="")
		//	cout<<"Minor problem - input file does not end in .fet extension - might be a problem when saving the timetables"<<" (file:"<<__FILE__<<", line:"<<__LINE__<<")"<<endl;
		
		cssfilename+="_"+STYLESHEET_CSS;
		if(INPUT_FILENAME_XML=="")
			cssfilename=STYLESHEET_CSS;
		tmp+="    <link rel=\"stylesheet\" media=\"all\" href=\""+cssfilename+"\" type=\"text/css\" />\n";
	}
	if(java){
		if(TIMETABLE_HTML_LEVEL>=5){  // the following JavaScript code is pretty similar to an example of Les Richardson ( http://richtech.ca/openadmin/index.html )
			tmp+="    <meta http-equiv=\"Content-Script-Type\" content=\"text/javascript\" />\n";
			tmp+="    <script type=\"text/javascript\">\n";
			tmp+="      function highlight(classval) {\n";
			tmp+="        var spans = document.getElementsByTagName('span');\n";
			tmp+="        for(var i=0;spans.length>i;i++) {\n";
			tmp+="          if (spans[i].className == classval) {\n";
			tmp+="            spans[i].style.backgroundColor = 'lime';\n";
			tmp+="          } else {\n";
			tmp+="            spans[i].style.backgroundColor = 'white';\n";
			tmp+="          }\n";
			tmp+="        }\n";
			tmp+="      }\n";
			tmp+="    </script>\n";
		}
	}
	tmp+="  </head>\n\n";
	tmp+="  <body id=\"top\">\n";
	if(placedActivities!=gt.rules.nInternalActivities)
		tmp+="    <h1>"+TimetableExport::tr("Warning! Only %1 out of %2 activities placed!").arg(placedActivities).arg(gt.rules.nInternalActivities)+"</h1>\n";
	if(printInstitution){
		tmp+="    <table>\n      <tr align=\"left\" valign=\"top\">\n        <th>"+TimetableExport::tr("Institution name")+":</th>\n        <td>"+protect2(gt.rules.institutionName)+"</td>\n      </tr>\n    </table>\n";
		tmp+="    <table>\n      <tr align=\"left\" valign=\"top\">\n        <th>"+TimetableExport::tr("Comments")+":</th>\n        <td>"+protect2(gt.rules.comments).replace(QString("\n"), QString("<br />\n"))+"</td>\n      </tr>\n    </table>\n";
	}
	return tmp;
}


QString TimetableExport::writeTOCDays(bool detailed){
// by Volker Dirr
	QString tmp;
	tmp+="    <p><strong>"+TimetableExport::tr("Table of contents")+"</strong></p>\n";
	tmp+="    <ul>\n";
	for(int k=0; k<gt.rules.nDaysPerWeek; k++){
		tmp+="      <li>\n        ";
		if(detailed){
			tmp+=" <a href=\"";
			tmp+="#table_"+hashDayIDsTimetable.value(gt.rules.daysOfTheWeek[k])+"_DETAILED\">"+protect2(gt.rules.daysOfTheWeek[k])+" ("+tr("Detailed")+")</a> /";
			tmp+=" <a href=\"";
			tmp+="#table_"+hashDayIDsTimetable.value(gt.rules.daysOfTheWeek[k])+"\">"+protect2(gt.rules.daysOfTheWeek[k])+" ("+tr("Less detailed")+")</a>\n";
		} else{
			tmp+=" <a href=\"";
			tmp+="#table_"+hashDayIDsTimetable.value(gt.rules.daysOfTheWeek[k])+"\">"+protect2(gt.rules.daysOfTheWeek[k])+"</a>\n";
		}
		tmp+="          </li>\n";
	}
	tmp+="    </ul>\n    <p>&nbsp;</p>\n";
	return tmp;
}


QString TimetableExport::writeStartTagTDofActivities(const Activity* act, bool detailed, bool colspan, bool rowspan){
// by Volker Dirr
	QString tmp;
	assert(!(colspan && rowspan));
	if(detailed)
		assert(!colspan && !rowspan);
	else
		tmp+="          ";
	tmp+="<td";
	if(rowspan && act->duration>1)
		tmp+=" rowspan=\""+QString::number(act->duration)+"\"";
	if(colspan && act->duration>1)
		tmp+=" colspan=\""+QString::number(act->duration)+"\"";
	if(TIMETABLE_HTML_LEVEL==6){
		tmp+=" class=\"";
		if(act->subjectName.size()>0){
			tmp+="s_"+hashSubjectIDsTimetable.value(act->subjectName);
		}
		if(act->activityTagsNames.size()>0){
			foreach(QString atn, act->activityTagsNames)
				 tmp+=" at_"+hashActivityTagIDsTimetable.value(atn);
		}
		if(act->studentsNames.size()>0){
			foreach(QString st, act->studentsNames)
				tmp+=" ss_"+hashStudentIDsTimetable.value(st);
		}
		if(act->teachersNames.size()>0){
			foreach(QString t, act->teachersNames)
				tmp+=" t_"+hashTeacherIDsTimetable.value(t);
		}
		//i need ai for this!!! so i need a parameter ai?! //TODO
/*		int r=best_solution.rooms[ai];
		if(r!=UNALLOCATED_SPACE && r!=UNSPECIFIED_ROOM){
			tmp+=" room_"+protect2id(gt.rules.internalRoomsList[r]->name);
		}*/
		if(detailed)
			tmp+=" detailed";
		tmp+="\"";
	}
	if(detailed && TIMETABLE_HTML_LEVEL>=1 && TIMETABLE_HTML_LEVEL<=5)
		tmp+=" class=\"detailed\"";
	tmp+=">";
	return tmp;
}


// by Volker Dirr
QString TimetableExport::writeSubjectAndActivityTags(const Activity* act, const QString& startTag, const QString& startTagAttribute, bool activityTagsOnly){
	QString tmp;
	if(act->subjectName.size()>0||act->activityTagsNames.size()>0){
		if(startTag=="div" && TIMETABLE_HTML_LEVEL>=3)
			tmp+="<"+startTag+startTagAttribute+">";
		if(act->subjectName.size()>0 && !activityTagsOnly){
			switch(TIMETABLE_HTML_LEVEL){
				case 3 : tmp+="<span class=\"subject\">"+protect2(act->subjectName)+"</span>"; break;
				case 4 : tmp+="<span class=\"subject\"><span class=\"s_"+hashSubjectIDsTimetable.value(act->subjectName)+"\">"+protect2(act->subjectName)+"</span></span>"; break;
				case 5 : ;
				case 6 : tmp+="<span class=\"subject\"><span class=\"s_"+hashSubjectIDsTimetable.value(act->subjectName)+"\" onmouseover=\"highlight('s_"+hashSubjectIDsTimetable.value(act->subjectName)+"')\">"+protect2(act->subjectName)+"</span></span>"; break;
				default: tmp+=protect2(act->subjectName); break;
			}
			if(act->activityTagsNames.size()>0){
				tmp+=" ";
			}
		}
		if(act->activityTagsNames.size()>0){
			if(TIMETABLE_HTML_LEVEL>=3){
				tmp+="<span class=\"activitytag\">";
			}
			foreach(QString atn, act->activityTagsNames){
				switch(TIMETABLE_HTML_LEVEL){
					case 3 : tmp+=protect2(atn); break;
					case 4 : tmp+="<span class=\"at_"+hashActivityTagIDsTimetable.value(atn)+"\">"+protect2(atn)+"</span>"; break;
					case 5 : ;
					case 6 : tmp+="<span class=\"at_"+hashActivityTagIDsTimetable.value(atn)+"\" onmouseover=\"highlight('at_"+hashActivityTagIDsTimetable.value(atn)+"')\">"+protect2(atn)+"</span>"; break;
					default: tmp+=protect2(atn); break;
				}
				tmp+=", ";
			}
			tmp.remove(tmp.size()-2, 2);
			if(TIMETABLE_HTML_LEVEL>=3){
				tmp+="</span>";
			}
		}
		if(startTag=="div"){
			if(TIMETABLE_HTML_LEVEL>=3)
				tmp+="</div>";
			else tmp+="<br />";
		}
	}
	return tmp;
}


QString TimetableExport::writeStudents(const Activity* act, const QString& startTag, const QString& startTagAttribute){
// by Volker Dirr
	QString tmp;
	if(act->studentsNames.size()>0){
		if(startTag=="div" && TIMETABLE_HTML_LEVEL>=3)
			tmp+="<"+startTag+startTagAttribute+">";
		foreach(QString st, act->studentsNames){
			switch(TIMETABLE_HTML_LEVEL){
				case 4 : tmp+="<span class=\"ss_"+hashStudentIDsTimetable.value(st)+"\">"+protect2(st)+"</span>"; break;
				case 5 : ;
				case 6 : tmp+="<span class=\"ss_"+hashStudentIDsTimetable.value(st)+"\" onmouseover=\"highlight('ss_"+hashStudentIDsTimetable.value(st)+"')\">"+protect2(st)+"</span>"; break;
				default: tmp+=protect2(st); break;
			}
			tmp+=", ";
		}
		tmp.remove(tmp.size()-2, 2);
		if(startTag=="div"){
			if(TIMETABLE_HTML_LEVEL>=3)
				tmp+="</div>";
			else tmp+="<br />";
		}
	}
	return tmp;
}


QString TimetableExport::writeTeachers(const Activity* act, const QString& startTag, const QString& startTagAttribute){
// by Volker Dirr
	QString tmp;
	if(act->teachersNames.size()>0){
		if(startTag=="div" && TIMETABLE_HTML_LEVEL>=3)
			tmp+="<"+startTag+startTagAttribute+">";
		foreach(QString t, act->teachersNames){
			switch(TIMETABLE_HTML_LEVEL){
				case 4 : tmp+="<span class=\"t_"+hashTeacherIDsTimetable.value(t)+"\">"+protect2(t)+"</span>"; break;
				case 5 : ;
				case 6 : tmp+="<span class=\"t_"+hashTeacherIDsTimetable.value(t)+"\" onmouseover=\"highlight('t_"+hashTeacherIDsTimetable.value(t)+"')\">"+protect2(t)+"</span>"; break;
				default: tmp+=protect2(t); break;
			}
			tmp+=", ";
		}
		tmp.remove(tmp.size()-2, 2);
		if(startTag=="div"){
			if(TIMETABLE_HTML_LEVEL>=3)
				tmp+="</div>";
			else tmp+="<br />";
		}
	}
	return tmp;
}


QString TimetableExport::writeRoom(int ai, const QString& startTag, const QString& startTagAttribute){
// by Volker Dirr
	QString tmp;
	int r=best_solution.rooms[ai];
	if(r!=UNALLOCATED_SPACE && r!=UNSPECIFIED_ROOM){
		if(startTag=="div" && TIMETABLE_HTML_LEVEL>=3)
			tmp+="<"+startTag+startTagAttribute+">";
		switch(TIMETABLE_HTML_LEVEL){
			case 4 : tmp+="<span class=\"r_"+hashRoomIDsTimetable.value(gt.rules.internalRoomsList[r]->name)+"\">"+protect2(gt.rules.internalRoomsList[r]->name)+"</span>"; break;
			case 5 : ;
			case 6 : tmp+="<span class=\"r_"+hashRoomIDsTimetable.value(gt.rules.internalRoomsList[r]->name)+"\" onmouseover=\"highlight('r_"+hashRoomIDsTimetable.value(gt.rules.internalRoomsList[r]->name)+"')\">"+protect2(gt.rules.internalRoomsList[r]->name)+"</span>"; break;
			default: tmp+=protect2(gt.rules.internalRoomsList[r]->name); break;
		}
		if(startTag=="div"){
			if(TIMETABLE_HTML_LEVEL>=3)
				tmp+="</div>";
			else tmp+="<br />";
		}
	}
	return tmp;
}


QString TimetableExport::writeNotAvailableSlot(const QString& weight){
// by Volker Dirr
	QString tmp;
	//weight=" "+weight;
	switch(TIMETABLE_HTML_LEVEL){
		case 3 : ;
		case 4 : tmp="          <td class=\"notAvailable\"><span class=\"notAvailable\">"+protect2(STRING_NOT_AVAILABLE_TIME_SLOT)+weight+"</span></td>\n"; break;
		case 5 : ;
		case 6 : tmp="          <td class=\"notAvailable\"><span class=\"notAvailable\" onmouseover=\"highlight('notAvailable')\">"+protect2(STRING_NOT_AVAILABLE_TIME_SLOT)+weight+"</span></td>\n"; break;
		default: tmp="          <td>"+protect2(STRING_NOT_AVAILABLE_TIME_SLOT)+weight+"</td>\n";
	}
	return tmp;
}

QString TimetableExport::writeBreakSlot(const QString& weight){
// by Volker Dirr
	QString tmp;
	//weight=" "+weight;
	switch(TIMETABLE_HTML_LEVEL){
		case 3 : ;
		case 4 : tmp="          <td class=\"break\"><span class=\"break\">"+protect2(STRING_BREAK_SLOT)+weight+"</span></td>\n"; break;
		case 5 : ;
		case 6 : tmp="          <td class=\"break\"><span class=\"break\" onmouseover=\"highlight('break')\">"+protect2(STRING_BREAK_SLOT)+weight+"</span></td>\n"; break;
		default: tmp="          <td>"+protect2(STRING_BREAK_SLOT)+weight+"</td>\n";
	}
	return tmp;
}

QString TimetableExport::writeEmpty(){
// by Volker Dirr
	QString tmp;
	switch(TIMETABLE_HTML_LEVEL){
		case 3 : ;
		case 4 : tmp="          <td class=\"empty\"><span class=\"empty\">"+protect2(STRING_EMPTY_SLOT)+"</span></td>\n"; break;
		case 5 : ;
		case 6 : tmp="          <td class=\"empty\"><span class=\"empty\" onmouseover=\"highlight('empty')\">"+protect2(STRING_EMPTY_SLOT)+"</span></td>\n"; break;
		default: tmp="          <td>"+protect2(STRING_EMPTY_SLOT)+"</td>\n";
	}
	return tmp;
}


QString TimetableExport::writeActivityStudents(int ai, int day, int hour, bool notAvailable, bool colspan, bool rowspan){
//by Volker Dirr
	QString tmp;
	int currentTime=day+gt.rules.nDaysPerWeek*hour;
	if(ai!=UNALLOCATED_ACTIVITY){
		if(best_solution.times[ai]==currentTime){
			Activity* act=&gt.rules.internalActivitiesList[ai];
			tmp+=writeStartTagTDofActivities(act, false, colspan, rowspan);
			tmp+=writeSubjectAndActivityTags(act, "div", " class=\"line1\"", false);
			tmp+=writeTeachers(act, "div", " class=\"teacher line2\"");
			tmp+=writeRoom(ai, "div", " class=\"room line3\"");
			tmp+="</td>\n";
		} else
			tmp+="          <!-- span -->\n";
	} else {
		if(notAvailable && PRINT_NOT_AVAILABLE_TIME_SLOTS){
			tmp+=writeNotAvailableSlot("");
		}
		else if(breakDayHour[day][hour] && PRINT_BREAK_TIME_SLOTS){
			tmp+=writeBreakSlot("");
		}
		else{
			tmp+=writeEmpty();
		}
	}
	return tmp;
}


QString TimetableExport::writeActivitiesStudents(const QList<int>& allActivities){
//by Volker Dirr
	QString tmp;
	if(TIMETABLE_HTML_LEVEL>=1)
		tmp+="          <td><table class=\"detailed\">";
	else
		tmp+="          <td><table>";
	if(TIMETABLE_HTML_LEVEL>=3)
		tmp+="<tr class=\"student line0\">";
	else	tmp+="<tr>";
	for(int a=0; a<allActivities.size(); a++){
		int ai=allActivities[a];
		if(ai!=UNALLOCATED_ACTIVITY){
			Activity* act=&gt.rules.internalActivitiesList[ai];
			tmp+=writeStartTagTDofActivities(act, true, false, false)+writeStudents(act, "", "")+"</td>";
		}
	}
	tmp+="</tr>";
	if(TIMETABLE_HTML_LEVEL>=3)
		tmp+="<tr class=\"line1\">";
	else	tmp+="<tr>";
	for(int a=0; a<allActivities.size(); a++){
		int ai=allActivities[a];
		if(ai!=UNALLOCATED_ACTIVITY){
			Activity* act=&gt.rules.internalActivitiesList[ai];
			tmp+=writeStartTagTDofActivities(act, true, false, false)+writeSubjectAndActivityTags(act, "", "", false)+"</td>";
		}
	}
	tmp+="</tr>";
	if(TIMETABLE_HTML_LEVEL>=3)
		tmp+="<tr class=\"teacher line2\">";
	else	tmp+="<tr>";
	for(int a=0; a<allActivities.size(); a++){
		int ai=allActivities[a];
		if(ai!=UNALLOCATED_ACTIVITY){
			Activity* act=&gt.rules.internalActivitiesList[ai];
			tmp+=writeStartTagTDofActivities(act, true, false, false)+writeTeachers(act, "", "")+"</td>";
		}
	}
	tmp+="</tr>";
	if(TIMETABLE_HTML_LEVEL>=3)
		tmp+="<tr class=\"room line3\">";
	else	tmp+="<tr>";
	for(int a=0; a<allActivities.size(); a++){
		int ai=allActivities[a];
		if(ai!=UNALLOCATED_ACTIVITY){
			Activity* act=&gt.rules.internalActivitiesList[ai];
			tmp+=writeStartTagTDofActivities(act, true, false, false)+writeRoom(ai, "", "")+"</td>";
		}
	}
	tmp+="</tr>";
	tmp+="</table></td>\n";
	return tmp;
}


QString TimetableExport::writeActivityTeacher(int teacher, int day, int hour, bool colspan, bool rowspan){
//by Volker Dirr
	QString tmp;
	int ai=teachers_timetable_weekly[teacher][day][hour];
	int currentTime=day+gt.rules.nDaysPerWeek*hour;
	if(ai!=UNALLOCATED_ACTIVITY){
		if(best_solution.times[ai]==currentTime){
			Activity* act=&gt.rules.internalActivitiesList[ai];
			tmp+=writeStartTagTDofActivities(act, false, colspan, rowspan);
			tmp+=writeStudents(act, "div", " class=\"student line1\"");
			tmp+=writeSubjectAndActivityTags(act, "div", " class=\"line2\"", false);
			tmp+=writeRoom(ai, "div", " class=\"room line3\"");
			tmp+="</td>\n";
		} else
			tmp+="          <!-- span -->\n";
	} else {
		if(teacherNotAvailableDayHour[teacher][day][hour] && PRINT_NOT_AVAILABLE_TIME_SLOTS){
			tmp+=writeNotAvailableSlot("");
		}
		else if(breakDayHour[day][hour] && PRINT_BREAK_TIME_SLOTS){
			tmp+=writeBreakSlot("");
		}
		else{
			tmp+=writeEmpty();
		}
	}
	return tmp;
}


QString TimetableExport::writeActivitiesTeachers(const QList<int>& allActivities){
//by Volker Dirr
	QString tmp;
	if(TIMETABLE_HTML_LEVEL>=1)
		tmp+="          <td><table class=\"detailed\">";
	else
		tmp+="          <td><table>";
	if(TIMETABLE_HTML_LEVEL>=3)
		tmp+="<tr class=\"teacher line0\">";
	else	tmp+="<tr>";
	for(int a=0; a<allActivities.size(); a++){
		int ai=allActivities[a];
		if(ai!=UNALLOCATED_ACTIVITY){
			Activity* act=&gt.rules.internalActivitiesList[ai];
			tmp+=writeStartTagTDofActivities(act, true, false, false)+writeTeachers(act, "", "")+"</td>";
		}
	}
	tmp+="</tr>";
	if(TIMETABLE_HTML_LEVEL>=3)
		tmp+="<tr class=\"student line1\">";
	else	tmp+="<tr>";
	for(int a=0; a<allActivities.size(); a++){
		int ai=allActivities[a];
		if(ai!=UNALLOCATED_ACTIVITY){
			Activity* act=&gt.rules.internalActivitiesList[ai];
			tmp+=writeStartTagTDofActivities(act, true, false, false)+writeStudents(act, "", "")+"</td>";
		}
	}
	tmp+="</tr>";
	if(TIMETABLE_HTML_LEVEL>=3)
		tmp+="<tr class=\"line2\">";
	else	tmp+="<tr>";
	for(int a=0; a<allActivities.size(); a++){
		int ai=allActivities[a];
		if(ai!=UNALLOCATED_ACTIVITY){
			Activity* act=&gt.rules.internalActivitiesList[ai];
			tmp+=writeStartTagTDofActivities(act, true, false, false)+writeSubjectAndActivityTags(act, "", "", false)+"</td>";
		}
	}
	tmp+="</tr>";

	if(TIMETABLE_HTML_LEVEL>=3)
		tmp+="<tr class=\"room line3\">";
	else	tmp+="<tr>";
	for(int a=0; a<allActivities.size(); a++){
		int ai=allActivities[a];
		if(ai!=UNALLOCATED_ACTIVITY){
			Activity* act=&gt.rules.internalActivitiesList[ai];
			tmp+=writeStartTagTDofActivities(act, true, false, false)+writeRoom(ai, "", "")+"</td>";
		}
	}
	tmp+="</tr>";
	tmp+="</table></td>\n";
	return tmp;
}


QString TimetableExport::writeActivityRoom(int room, int day, int hour, bool colspan, bool rowspan){
//by Volker Dirr
	QString tmp;
	int ai=rooms_timetable_weekly[room][day][hour];
	int currentTime=day+gt.rules.nDaysPerWeek*hour;
	if(ai!=UNALLOCATED_ACTIVITY){
		if(best_solution.times[ai]==currentTime){
			Activity* act=&gt.rules.internalActivitiesList[ai];
			tmp+=writeStartTagTDofActivities(act, false, colspan, rowspan);
			tmp+=writeStudents(act, "div", " class=\"student line1\"");
			tmp+=writeTeachers(act, "div", " class=\"teacher line2\"");
			tmp+=writeSubjectAndActivityTags(act, "div", " class=\"line3\"", false);
			tmp+="</td>\n";
		} else
			tmp+="          <!-- span -->\n";
	} else {
		if(notAllowedRoomTimePercentages[room][day+hour*gt.rules.nDaysPerWeek]>=0 && PRINT_NOT_AVAILABLE_TIME_SLOTS){
			tmp+=writeNotAvailableSlot("");
		}
		else if(breakDayHour[day][hour] && PRINT_BREAK_TIME_SLOTS){
			tmp+=writeBreakSlot("");
		}
		else{
			tmp+=writeEmpty();
		}
	}
	return tmp;
}


QString TimetableExport::writeActivitiesRooms(const QList<int>& allActivities){
//by Volker Dirr
	QString tmp;
	if(TIMETABLE_HTML_LEVEL>=1)
		tmp+="          <td><table class=\"detailed\">";
	else
		tmp+="          <td><table>";
	if(TIMETABLE_HTML_LEVEL>=3)
		tmp+="<tr class=\"room line0\">";
	else	tmp+="<tr>";
	for(int a=0; a<allActivities.size(); a++){
		int ai=allActivities[a];
		if(ai!=UNALLOCATED_ACTIVITY){
			Activity* act=&gt.rules.internalActivitiesList[ai];
			tmp+=writeStartTagTDofActivities(act, true, false, false)+writeRoom(ai, "", "")+"</td>";
		}
	}
	tmp+="</tr>";
	if(TIMETABLE_HTML_LEVEL>=3)
		tmp+="<tr class=\"student line1\">";
	else	tmp+="<tr>";
	for(int a=0; a<allActivities.size(); a++){
		int ai=allActivities[a];
		if(ai!=UNALLOCATED_ACTIVITY){
			Activity* act=&gt.rules.internalActivitiesList[ai];
			tmp+=writeStartTagTDofActivities(act, true, false, false)+writeStudents(act, "", "")+"</td>";
		}
	}
	tmp+="</tr>";
	if(TIMETABLE_HTML_LEVEL>=3)
		tmp+="<tr class=\"teacher line2\">";
	else	tmp+="<tr>";
	for(int a=0; a<allActivities.size(); a++){
		int ai=allActivities[a];
		if(ai!=UNALLOCATED_ACTIVITY){
			Activity* act=&gt.rules.internalActivitiesList[ai];
			tmp+=writeStartTagTDofActivities(act, true, false, false)+writeTeachers(act, "", "")+"</td>";
		}
	}
	tmp+="</tr>";
	if(TIMETABLE_HTML_LEVEL>=3)
		tmp+="<tr class=\"line3\">";
	else	tmp+="<tr>";
	for(int a=0; a<allActivities.size(); a++){
		int ai=allActivities[a];
		if(ai!=UNALLOCATED_ACTIVITY){
			Activity* act=&gt.rules.internalActivitiesList[ai];
			tmp+=writeStartTagTDofActivities(act, true, false, false)+writeSubjectAndActivityTags(act, "", "", false)+"</td>";
		}
	}
	tmp+="</tr>";

	tmp+="</table></td>\n";
	return tmp;
}


QString TimetableExport::writeActivitiesSubjects(const QList<int>& allActivities){
//by Volker Dirr
	QString tmp;
	if(allActivities.isEmpty()){
		tmp+=writeEmpty();
	} else {
		if(TIMETABLE_HTML_LEVEL>=1)
			tmp+="          <td><table class=\"detailed\">";
		else
						tmp+="          <td><table>";
		if(TIMETABLE_HTML_LEVEL>=3)
			tmp+="<tr class=\"line0 activitytag\">";
		else	tmp+="<tr>";
		for(int a=0; a<allActivities.size(); a++){
			Activity* act=&gt.rules.internalActivitiesList[allActivities[a]];
			tmp+=writeStartTagTDofActivities(act, true, false, false)+writeSubjectAndActivityTags(act, "", "", true)+"</td>";
		}
		tmp+="</tr>";
		if(TIMETABLE_HTML_LEVEL>=3)
			tmp+="<tr class=\"student line1\">";
		else	tmp+="<tr>";
		for(int a=0; a<allActivities.size(); a++){
			Activity* act=&gt.rules.internalActivitiesList[allActivities[a]];
			tmp+=writeStartTagTDofActivities(act, true, false, false)+writeStudents(act, "", "")+"</td>";	
		}
		tmp+="</tr>";
		if(TIMETABLE_HTML_LEVEL>=3)
			tmp+="<tr class=\"teacher line2\">";
		else	tmp+="<tr>";
		for(int a=0; a<allActivities.size(); a++){
			Activity* act=&gt.rules.internalActivitiesList[allActivities[a]];
			tmp+=writeStartTagTDofActivities(act, true, false, false)+writeTeachers(act, "", "")+"</td>";
		}
		tmp+="</tr>";
		if(TIMETABLE_HTML_LEVEL>=3)
			tmp+="<tr class=\"room line3\">";
		else	tmp+="<tr>";
		for(int a=0; a<allActivities.size(); a++){
			int ai=allActivities[a];
			Activity* act=&gt.rules.internalActivitiesList[ai];
			tmp+=writeStartTagTDofActivities(act, true, false, false)+writeRoom(ai, "", "")+"</td>";
		}
		tmp+="</tr>";
		tmp+="</table></td>\n";
	}
	return tmp;
}
