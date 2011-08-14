/*
File timetableexport.h
*/

/***************************************************************************
                          timetableexport.h  -  description
                             -------------------
    begin                : Tue Apr 22, 2003
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

#ifndef TIMETABLEEXPORT_H
#define TIMETABLEEXPORT_H

#include "timetable_defs.h"
#include "timetable.h"

#include <QString>

class TimetableExport: public QObject{
	//Q_OBJECT

public:
	TimetableExport();
	~TimetableExport();

	static void getStudentsTimetable(Solution& c);
	static void getTeachersTimetable(Solution& c);
	static void getRoomsTimetable(Solution& c);

	static void writeSimulationResults();
	static void writeHighestStageResults();
	static void writeSimulationResults(int n); //write in a directory with number n (for multiple generation)
	static void writeSimulationResultsCommandLine(const QString& outputDirectory);
	
	static void writeRandomSeed();
	static void writeRandomSeed(int n); //write in a directory with number n (for multiple generation)
	static void writeRandomSeedCommandLine(const QString& outputDirectory);
	static void writeRandomSeedFile(const QString& filename);
	
	static void writeTimetableDataFile(const QString& filename);

private:
	//this function must be called before export html files, because it compute the IDs
	static void computeHashForIDsTimetable();

	//this function must be called before export html files, because it is needed for the allActivities tables
	static void computeActivitiesAtTime();
	
	//this function must be called before export html files, because it is needed to add activities with same starting time (simultanious activities)
	static void computeActivitiesWithSameStartingTime();
	//this function add activities with same starting time into the allActivities list
	static bool addActivitiesWithSameStartingTime(QList<int>& allActivities, int hour);

	//the following functions write the conflicts text and the xml files
	static void writeSubgroupsTimetableXml(const QString& xmlfilename);
	static void writeTeachersTimetableXml(const QString& xmlfilename);
	static void writeActivitiesTimetableXml(const QString& xmlfilename);
	static void writeConflictsTxt(const QString& filename, const QString& saveTime, int placedActivities);

	//the following functions write the css and html timetable files
	static void writeIndexHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities);
	static void writeStylesheetCss(const QString& htmlfilename, const QString& saveTime, int placedActivities);
	static void writeSubgroupsTimetableDaysHorizontalHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities);
	static void writeSubgroupsTimetableDaysVerticalHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities);
	static void writeSubgroupsTimetableTimeHorizontalHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities);
	static void writeSubgroupsTimetableTimeVerticalHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities);
	static void writeSubgroupsTimetableTimeHorizontalDailyHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities);
	static void writeSubgroupsTimetableTimeVerticalDailyHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities);
	static void writeGroupsTimetableDaysHorizontalHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities);
	static void writeGroupsTimetableDaysVerticalHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities);
	static void writeGroupsTimetableTimeHorizontalHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities);
	static void writeGroupsTimetableTimeVerticalHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities);
	static void writeGroupsTimetableTimeHorizontalDailyHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities);
	static void writeGroupsTimetableTimeVerticalDailyHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities);
	static void writeYearsTimetableDaysHorizontalHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities);
	static void writeYearsTimetableDaysVerticalHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities);
	static void writeYearsTimetableTimeHorizontalHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities);
	static void writeYearsTimetableTimeVerticalHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities);
	static void writeYearsTimetableTimeHorizontalDailyHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities);
	static void writeYearsTimetableTimeVerticalDailyHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities);
	static void writeAllActivitiesTimetableDaysHorizontalHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities);
	static void writeAllActivitiesTimetableDaysVerticalHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities);
	static void writeAllActivitiesTimetableTimeHorizontalHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities);
	static void writeAllActivitiesTimetableTimeVerticalHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities);
	static void writeAllActivitiesTimetableTimeHorizontalDailyHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities);
	static void writeAllActivitiesTimetableTimeVerticalDailyHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities);
	static void writeTeachersTimetableDaysHorizontalHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities);
	static void writeTeachersTimetableDaysVerticalHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities);
	static void writeTeachersTimetableTimeHorizontalHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities);
	static void writeTeachersTimetableTimeVerticalHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities);
	static void writeTeachersTimetableTimeHorizontalDailyHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities);
	static void writeTeachersTimetableTimeVerticalDailyHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities);
	static void writeRoomsTimetableDaysHorizontalHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities);
	static void writeRoomsTimetableDaysVerticalHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities);
	static void writeRoomsTimetableTimeHorizontalHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities);
	static void writeRoomsTimetableTimeVerticalHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities);
	static void writeRoomsTimetableTimeHorizontalDailyHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities);
	static void writeRoomsTimetableTimeVerticalDailyHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities);
	static void writeSubjectsTimetableDaysHorizontalHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities);
	static void writeSubjectsTimetableDaysVerticalHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities);
	static void writeSubjectsTimetableTimeHorizontalHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities);
	static void writeSubjectsTimetableTimeVerticalHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities);
	static void writeSubjectsTimetableTimeHorizontalDailyHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities);
	static void writeSubjectsTimetableTimeVerticalDailyHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities);
	static void writeTeachersFreePeriodsTimetableDaysHorizontalHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities);
	static void writeTeachersFreePeriodsTimetableDaysVerticalHtml(const QString& htmlfilename, const QString& saveTime, int placedActivities);

	//the following functions return QStrings, because they are 'only' subfunctions to the writeXxxHtml functions
	static QString writeActivityStudents(int ai, int day, int hour, bool notAvailable, bool colspan, bool rowspan);
	static QString writeActivitiesStudents(const QList<int>& allActivities);
	static QString writeActivityTeacher(int teacher, int day, int hour, bool colspan, bool rowspan);
	static QString writeActivitiesTeachers(const QList<int>& allActivities);
	static QString writeActivityRoom(int room, int day, int hour, bool colspan, bool rowspan);
	static QString writeActivitiesRooms(const QList<int>& allActivities);
	static QString writeActivitiesSubjects(const QList<int>& allActivities);

	//the following functions return QStrings, because they are 'only' subfunctions to the writeActivity-iesXxx functions
	static QString writeHead(bool java, int placedActivities, bool printInstitution);
	static QString writeTOCDays(bool detailed);
	static QString writeStartTagTDofActivities(const Activity* act, bool detailed, bool colspan, bool rowspan);
	static QString writeSubjectAndActivityTags(const Activity* act, const QString& startTag, const QString& startTagAttribute, bool activityTagsOnly);
	static QString writeStudents(const Activity* act, const QString& startTag, const QString& startTagAttribute);
	static QString writeTeachers(const Activity* act, const QString& startTag, const QString& startTagAttribute);
	static QString writeRoom(int ai, const QString& startTag, const QString& startTagAttribute);
	static QString writeNotAvailableSlot(const QString& weight);
	static QString writeBreakSlot(const QString& weight);
	static QString writeEmpty();
};

#endif
