/*
File generate.h
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

#ifndef GENERATE_H
#define GENERATE_H

#include "timetable_defs.h"
#include "solution.h"

#include <QTextStream>

class Activity;

//a probabilistic function to say if we can skip a constraint based on its percentage weight
bool skipRandom(double weightPercentage);

//for sorting slots in ascending order of potential conflicts
bool compareFunctionGenerate(int i, int j);

/**
This class incorporates the routines for time and space allocation of activities
*/
class Generate: public QObject{
//	Q_OBJECT

public:
	Generate();
	~Generate();
	
	inline void addAiToNewTimetable(int ai, const Activity* act, int d, int h);
	inline void removeAiFromNewTimetable(int ai, const Activity* act, int d, int h);
	
	inline void getTchTimetable(int tch, const QList<int>& conflActivities);
	inline void getSbgTimetable(int sbg, const QList<int>& conflActivities);
	
	inline void removeAi2FromTchTimetable(int ai2);
	inline void removeAi2FromSbgTimetable(int ai2);

	inline void updateTeachersNHoursGaps(Activity* act, int ai, int d);
	inline void updateSubgroupsNHoursGaps(Activity* act, int ai, int d);
	
	inline void updateTchNHoursGaps(int tch, int d);
	inline void updateSbgNHoursGaps(int sbg, int d);
	
	inline void tchGetNHoursGaps(int tch);
	inline void teacherGetNHoursGaps(int tch);
	inline bool teacherRemoveAnActivityFromBeginOrEnd(int tch, int level, int ai, QList<int>& conflActivities, int& nConflActivities, int& removedActivity);
	inline bool teacherRemoveAnActivityFromAnywhere(int tch, int level, int ai, QList<int>& conflActivities, int& nConflActivities, int& removedActivity);
	inline bool teacherRemoveAnActivityFromBeginOrEndCertainDay(int tch, int d2, int level, int ai, QList<int>& conflActivities, int& nConflActivities, int& removedActivity);
	inline bool teacherRemoveAnActivityFromAnywhereCertainDay(int tch, int d2, int level, int ai, QList<int>& conflActivities, int& nConflActivities, int& removedActivity);

	inline bool teacherRemoveAnActivityFromAnywhereCertainDayCertainActivityTag(int tch, int d2, int actTag, int level, int ai, QList<int>& conflActivities, int& nConflActivities, int& removedActivity);

	inline void sbgGetNHoursGaps(int sbg);
	inline void subgroupGetNHoursGaps(int sbg);
	inline bool subgroupRemoveAnActivityFromBegin(int sbg, int level, int ai, QList<int>& conflActivities, int& nConflActivities, int& removedActivity);
	inline bool subgroupRemoveAnActivityFromEnd(int sbg, int level, int ai, QList<int>& conflActivities, int& nConflActivities, int& removedActivity);
	inline bool subgroupRemoveAnActivityFromBeginOrEnd(int sbg, int level, int ai, QList<int>& conflActivities, int& nConflActivities, int& removedActivity);
	inline bool subgroupRemoveAnActivityFromAnywhere(int sbg, int level, int ai, QList<int>& conflActivities, int& nConflActivities, int& removedActivity);
	inline bool subgroupRemoveAnActivityFromBeginCertainDay(int sbg, int d2, int level, int ai, QList<int>& conflActivities, int& nConflActivities, int& removedActivity);
	inline bool subgroupRemoveAnActivityFromEndCertainDay(int sbg, int d2, int level, int ai, QList<int>& conflActivities, int& nConflActivities, int& removedActivity);
	inline bool subgroupRemoveAnActivityFromAnywhereCertainDay(int sbg, int d2, int level, int ai, QList<int>& conflActivities, int& nConflActivities, int& removedActivity);

	inline bool subgroupRemoveAnActivityFromAnywhereCertainDayCertainActivityTag(int sbg, int d2, int actTag, int level, int ai, QList<int>& conflActivities, int& nConflActivities, int& removedActivity);
	
	//only one out of sbg and tch is >=0, other one is -1
	inline bool checkBuildingChanges(int sbg, int tch, const QList<int>& globalConflActivities, int rm, int level, const Activity* act, int ai, int d, int h, QList<int>& tmp_list);
	inline bool chooseRoom(const QList<int>& listOfRooms, const QList<int>& globalConflActivities, int level, const Activity* act, int ai, int d, int h, int& roomSlot, int& selectedSlot, QList<int>& localConflActivities);
	inline bool getHomeRoom(const QList<int>& globalConflActivities, int level, const Activity* act, int ai, int d, int h, int& roomSlot, int& selectedSlot, QList<int>& localConflActivities);
	inline bool getPreferredRoom(const QList<int>& globalConflActivities, int level, const Activity* act, int ai, int d, int h, int& roomSlot, int& selectedSlot, QList<int>& localConflActivities, bool& canBeUnspecifiedPreferredRoom);
	inline bool getRoom(int level, const Activity* act, int ai, int d, int h, int& roomSlot, int& selectedSlot, QList<int>& conflActivities, int& nConflActivities);

	Solution c;
	
	int nPlacedActivities;
	
	//difficult activities
	int nDifficultActivities;
	int difficultActivities[MAX_ACTIVITIES];
	
	int searchTime; //seconds
	
	int timeToHighestStage; //seconds
	
	bool abortOptimization;
	
	bool precompute(QTextStream* maxPlacedActivityStream=NULL);
	
	void generate(int maxSeconds, bool& impossible, bool& timeExceeded, bool threaded, QTextStream* maxPlacedActivityStream=NULL);
	
	void moveActivity(int ai, int fromslot, int toslot, int fromroom, int toroom);
	
	void randomSwap(int ai, int level);
	
signals:
	void activityPlaced(int);
	
	void simulationFinished();
	
	void impossibleToSolve();
};

#endif
