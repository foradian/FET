/*
File rules.cpp
*/

/*
Copyright 2002, 2003 Lalescu Liviu.

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

#include "timetable_defs.h"
#include "rules.h"

#include <QDir>

#include <iostream>
using namespace std;

#include <QTextStream>
#include <QFile>
#include <QFileInfo>

#include <QDate>
#include <QTime>
#include <QLocale>

#include <QString>

#include <QtXml>

#include <QString>
#include <QTranslator>
#include <QMessageBox>

#include <QtAlgorithms>

#include <QSet>
#include <QHash>

#include <QApplication>
#include <QProgressDialog>

#include <QRegExp>


#include "lockunlock.h"

static bool toSkipTime[MAX_TIME_CONSTRAINTS];
static bool toSkipSpace[MAX_SPACE_CONSTRAINTS];

extern QApplication* pqapplication;

extern bool students_schedule_ready;
extern bool rooms_schedule_ready;
extern bool teachers_schedule_ready;


FakeString::FakeString()
{
}

void FakeString::operator=(const QString& other)
{
	Q_UNUSED(other);
}

void FakeString::operator=(const char* str)
{
	Q_UNUSED(str);
}

void FakeString::operator+=(const QString& other)
{
	Q_UNUSED(other);
}

void FakeString::operator+=(const char* str)
{
	Q_UNUSED(str);
}


void Rules::init() //initializes the rules (empty, but with default hours and days)
{
	//defaults
	this->nHoursPerDay=12;
	this->hoursOfTheDay[0]="08:00";
	this->hoursOfTheDay[1]="09:00";
	this->hoursOfTheDay[2]="10:00";
	this->hoursOfTheDay[3]="11:00";
	this->hoursOfTheDay[4]="12:00";
	this->hoursOfTheDay[5]="13:00";
	this->hoursOfTheDay[6]="14:00";
	this->hoursOfTheDay[7]="15:00";
	this->hoursOfTheDay[8]="16:00";
	this->hoursOfTheDay[9]="17:00";
	this->hoursOfTheDay[10]="18:00";
	this->hoursOfTheDay[11]="19:00";
	this->hoursOfTheDay[12]="20:00";

	this->nDaysPerWeek=5;
	this->daysOfTheWeek[0] = tr("Monday");
	this->daysOfTheWeek[1] = tr("Tuesday");
	this->daysOfTheWeek[2] = tr("Wednesday");
	this->daysOfTheWeek[3] = tr("Thursday");
	this->daysOfTheWeek[4] = tr("Friday");
	
	this->institutionName=tr("Default institution");
	this->comments=tr("Default comments");

	this->initialized=true;
}

bool Rules::computeInternalStructure()
{
	//To fix a bug reported by Frans on forum, on 7 May 2010.
	//If user generates, then changes some activities (changes teachers of them), then tries to generate but FET cannot precompute in generate_pre.cpp,
	//then if user views the timetable, the timetable of a teacher contains activities of other teacher.
	//The bug appeared because it is possible to compute internal structure, so internal activities change the teacher, but the timetables remain the same,
	//with the same activities indexes.
	teachers_schedule_ready=false;
	students_schedule_ready=false;
	rooms_schedule_ready=false;

	//The order is important - firstly the teachers, subjects, activity tags and students.
	//After that, the buildings.
	//After that, the rooms.
	//After that, the activities.
	//After that, the time constraints.
	//After that, the space constraints.
	if(this->teachersList.size()>MAX_TEACHERS){
		/*QMessageBox::warning(NULL, tr("FET information"),
		 tr("You have too many teachers.\nPlease talk to the author or increase variable MAX_TEACHERS"));
		return false;
	*/}
	if(this->subjectsList.size()>MAX_SUBJECTS){
		/*QMessageBox::warning(NULL, tr("FET information"),
		 tr("You have too many subjects.\nPlease talk to the author or increase variable MAX_SUBJECTS"));
		return false;
	*/}
	
	//kill augmented students sets
	QList<StudentsYear*> ayears;
	QList<StudentsGroup*> agroups;
	QList<StudentsSubgroup*> asubgroups;
	foreach(StudentsYear* year, augmentedYearsList){
		if(!ayears.contains(year))
			ayears.append(year);
		foreach(StudentsGroup* group, year->groupsList){
			if(!agroups.contains(group))
				agroups.append(group);
			foreach(StudentsSubgroup* subgroup, group->subgroupsList){
				if(!asubgroups.contains(subgroup))
					asubgroups.append(subgroup);
			}
		}
	}
	foreach(StudentsYear* year, ayears){
		assert(year!=NULL);
		delete year;
	}
	foreach(StudentsGroup* group, agroups){
		assert(group!=NULL);
		delete group;
	}
	foreach(StudentsSubgroup* subgroup, asubgroups){
		assert(subgroup!=NULL);
		delete subgroup;
	}	
	augmentedYearsList.clear();
	//////////////////	
	
	//copy list of students sets into augmented list
	QHash<QString, StudentsSet*> augmentedHash;
	
	foreach(StudentsYear* y, yearsList){
		StudentsYear* ay=new StudentsYear();
		ay->name=y->name;
		ay->numberOfStudents=y->numberOfStudents;
		ay->groupsList.clear();
		augmentedYearsList << ay;
		
		assert(!augmentedHash.contains(ay->name));
		augmentedHash.insert(ay->name, ay);
		
		foreach(StudentsGroup* g, y->groupsList){
			if(augmentedHash.contains(g->name)){
				StudentsSet* tmpg=augmentedHash.value(g->name);
				assert(tmpg->type==STUDENTS_GROUP);
				ay->groupsList<<((StudentsGroup*)tmpg);
			}
			else{
				StudentsGroup* ag=new StudentsGroup();
				ag->name=g->name;
				ag->numberOfStudents=g->numberOfStudents;
				ag->subgroupsList.clear();
				ay->groupsList << ag;
				
				assert(!augmentedHash.contains(ag->name));
				augmentedHash.insert(ag->name, ag);
			
				foreach(StudentsSubgroup* s, g->subgroupsList){
					if(augmentedHash.contains(s->name)){
						StudentsSet* tmps=augmentedHash.value(s->name);
						assert(tmps->type==STUDENTS_SUBGROUP);
						ag->subgroupsList<<((StudentsSubgroup*)tmps);
					}
					else{
						StudentsSubgroup* as=new StudentsSubgroup();
						as->name=s->name;
						as->numberOfStudents=s->numberOfStudents;
						ag->subgroupsList << as;
						
						assert(!augmentedHash.contains(as->name));
						augmentedHash.insert(as->name, as);
					}
				}
			}
		}
	}

	/////////
	for(int i=0; i<this->augmentedYearsList.size(); i++){
		StudentsYear* sty=this->augmentedYearsList[i];

		//if this year has no groups, insert something to simulate the whole year
		if(sty->groupsList.count()==0){
			StudentsGroup* tmpGroup = new StudentsGroup();
			tmpGroup->name = sty->name+" "+tr("Automatic Group", "Please keep the translation short. It is used when a year contains no groups and an automatic group "
			 "is added in the year, in the timetable (when viewing the students timetable from FET and also in the html timetables for students groups or subgroups)"
			 ". In the empty year there will be added a group with name = yearName+a space character+your translation of 'Automatic Group'.");
			tmpGroup->numberOfStudents = sty->numberOfStudents;
			sty->groupsList << tmpGroup;
		}
		
		for(int j=0; j<sty->groupsList.size(); j++){
			StudentsGroup* stg=sty->groupsList[j];

			//if this group has no subgroups, insert something to simulate the whole group
			if(stg->subgroupsList.size()==0){
				StudentsSubgroup* tmpSubgroup = new StudentsSubgroup();
				tmpSubgroup->name = stg->name+" "+tr("Automatic Subgroup", "Please keep the translation short. It is used when a group contains no subgroups and an automatic subgroup "
				 "is added in the group, in the timetable (when viewing the students timetable from FET and also in the html timetables for students subgroups)"
				 ". In the empty group there will be added a subgroup with name = groupName+a space character+your translation of 'Automatic Subgroup'.");
				tmpSubgroup->numberOfStudents=stg->numberOfStudents;
				stg->subgroupsList << tmpSubgroup;
			}
		}
	}
	//////////
	
	QSet<QString> allSubgroups;
	
	int tmpNSubgroups;
	for(int i=0; i<this->augmentedYearsList.size(); i++){
		StudentsYear* sty=this->augmentedYearsList.at(i);

		for(int j=0; j<sty->groupsList.size(); j++){
			StudentsGroup* stg=sty->groupsList.at(j);
			
			for(int k=0; k<stg->subgroupsList.size(); k++)
				if(!allSubgroups.contains(stg->subgroupsList.at(k)->name))
					allSubgroups.insert(stg->subgroupsList.at(k)->name);
		}
	}
	tmpNSubgroups=allSubgroups.count();
	if(tmpNSubgroups>MAX_TOTAL_SUBGROUPS){
		/*QMessageBox::warning(NULL, tr("FET information"),
		 tr("You have too many total subgroups.\nPlease talk to the author or increase variable MAX_TOTAL_SUBGROUPS. Currently"
		  " MAX_TOTAL_SUBGROUPS=%1").arg(MAX_TOTAL_SUBGROUPS));
		return false;
	*/}
	this->internalSubgroupsList.resize(tmpNSubgroups);

	int counter=0;
	for(int i=0; i<this->activitiesList.size(); i++){
		Activity* act=this->activitiesList.at(i);
		if(act->active)
			counter++;
	}
	if(counter>MAX_ACTIVITIES){
		/*QMessageBox::warning(NULL, tr("FET information"),
		 tr("You have too many active activities.\nPlease talk to the author or increase variable MAX_ACTIVITIES"));
		return false;
	*/}

	if(this->buildingsList.size()>MAX_BUILDINGS){
		/*QMessageBox::warning(NULL, tr("FET information"),
		 tr("You have too many buildings.\nPlease talk to the author or increase variable MAX_BUILDINGS"));
		return false;
	*/}
	
	if(this->roomsList.size()>MAX_ROOMS){
		/*QMessageBox::warning(NULL, tr("FET information"),
		 tr("You have too many rooms.\nPlease talk to the author or increase variable MAX_ROOMS"));
		return false;
	*/}
	
	if(this->timeConstraintsList.size()>MAX_TIME_CONSTRAINTS){
		/*QMessageBox::warning(NULL, tr("FET information"),
		 tr("You have too many time constraints.\nPlease talk to the author or increase variable MAX_TIME_CONSTRAINTS"));
		return false;
	*/}

	if(this->spaceConstraintsList.size()>MAX_SPACE_CONSTRAINTS){
	/*	QMessageBox::warning(NULL, tr("FET information"),
		 tr("You have too many space constraints.\nPlease talk to the author or increase variable MAX_SPACE_CONSTRAINTS"));
		return false;
	*/}

	assert(this->initialized);

	//days and hours
	assert(this->nHoursPerDay>0);
	assert(this->nDaysPerWeek>0);
	this->nHoursPerWeek=this->nHoursPerDay*this->nDaysPerWeek;

	//teachers
	int i;
	Teacher* tch;
	this->nInternalTeachers=this->teachersList.size();
	assert(this->nInternalTeachers<=MAX_TEACHERS);
	this->internalTeachersList.resize(this->nInternalTeachers);
	for(i=0; i<this->teachersList.size(); i++){
		tch=teachersList[i];
		this->internalTeachersList[i]=tch;
	}
	assert(i==this->nInternalTeachers);

	//subjects
	Subject* sbj;
	this->nInternalSubjects=this->subjectsList.size();
	assert(this->nInternalSubjects<=MAX_SUBJECTS);	
	this->internalSubjectsList.resize(this->nInternalSubjects);
	for(i=0; i<this->subjectsList.size(); i++){
		sbj=this->subjectsList[i];
		this->internalSubjectsList[i]=sbj;
	}
	assert(i==this->nInternalSubjects);

	//activity tags
	ActivityTag* at;
	this->nInternalActivityTags=this->activityTagsList.size();
	this->internalActivityTagsList.resize(this->nInternalActivityTags);
	for(i=0; i<this->activityTagsList.size(); i++){
		at=this->activityTagsList[i];
		this->internalActivityTagsList[i]=at;
	}
	assert(i==this->nInternalActivityTags);

	//students
	this->nInternalSubgroups=0;
	for(int i=0; i<this->augmentedYearsList.size(); i++){
		StudentsYear* sty=this->augmentedYearsList[i];
		
		assert(sty->groupsList.count()>0);

		for(int j=0; j<sty->groupsList.size(); j++){
			StudentsGroup* stg=sty->groupsList[j];
			
			assert(stg->subgroupsList.count()>0);

			for(int k=0; k<stg->subgroupsList.size(); k++){
				StudentsSubgroup* sts=stg->subgroupsList[k];

				bool existing=false;
				for(int i=0; i<this->nInternalSubgroups; i++)
					if(this->internalSubgroupsList[i]->name==sts->name){
						existing=true;
						sts->indexInInternalSubgroupsList=i;
						break;
					}
				if(!existing){
					assert(this->nInternalSubgroups<MAX_TOTAL_SUBGROUPS);
					assert(this->nInternalSubgroups<tmpNSubgroups);
					sts->indexInInternalSubgroupsList=this->nInternalSubgroups;
					this->internalSubgroupsList[this->nInternalSubgroups++]=sts;
				}
			}
		}
	}
	assert(this->nInternalSubgroups==tmpNSubgroups);

	//buildings
	internalBuildingsList.resize(buildingsList.size());
	this->nInternalBuildings=0;
	assert(this->buildingsList.size()<=MAX_BUILDINGS);
	for(int i=0; i<this->buildingsList.size(); i++){
		Building* bu=this->buildingsList[i];
		bu->computeInternalStructure(*this);
	}
	
	for(int i=0; i<this->buildingsList.size(); i++){
		Building* bu=this->buildingsList[i];
		this->internalBuildingsList[this->nInternalBuildings++]=bu;
	}
	assert(this->nInternalBuildings==this->buildingsList.size());

	//rooms
	internalRoomsList.resize(roomsList.size());
	this->nInternalRooms=0;
	assert(this->roomsList.size()<=MAX_ROOMS);
	for(int i=0; i<this->roomsList.size(); i++){
		Room* rm=this->roomsList[i];
		rm->computeInternalStructure(*this);
	}
	
	for(int i=0; i<this->roomsList.size(); i++){
		Room* rm=this->roomsList[i];
		this->internalRoomsList[this->nInternalRooms++]=rm;
	}
	assert(this->nInternalRooms==this->roomsList.size());


	//activities
	int range=0;
	foreach(Activity* act, this->activitiesList)
		if(act->active)
			range++;
	//QProgressDialog progress(NULL);
//	progress.setWindowTitle(tr("Computing internal structure", "Title of a progress dialog"));
//	progress.setLabelText(tr("Processing internally the activities ... please wait"));
//	progress.setRange(0, range);
//	progress.setModal(true);							
	int ttt=0;
		
	Activity* act;
	counter=0;
	
	this->inactiveActivities.clear();
	
	for(int i=0; i<this->activitiesList.size(); i++){
		act=this->activitiesList[i];
		if(act->active){
			//progress.setValue(ttt);
			//pqapplication->processEvents();
			//if(progress.wasCanceled()){
				/*QMessageBox::information(NULL, tr("FET information"), tr("Canceled"));
				return false;
			*///}
			ttt++;

			counter++;
			act->computeInternalStructure(*this);
		}
		else
			inactiveActivities.insert(act->id);
	}
	
//	progress.setValue(range);

	for(int i=0; i<nInternalSubgroups; i++)
		internalSubgroupsList[i]->activitiesForSubgroup.clear();
	for(int i=0; i<nInternalTeachers; i++)
		internalTeachersList[i]->activitiesForTeacher.clear();

	assert(counter<=MAX_ACTIVITIES);
	this->nInternalActivities=counter;
	this->internalActivitiesList.resize(this->nInternalActivities);
	int activei=0;
	for(int ai=0; ai<this->activitiesList.size(); ai++){
		act=this->activitiesList[ai];
		if(act->active){
			this->internalActivitiesList[activei]=*act;
			
			for(int j=0; j<act->iSubgroupsList.count(); j++){
				int k=act->iSubgroupsList.at(j);
				assert(!internalSubgroupsList[k]->activitiesForSubgroup.contains(activei));
				internalSubgroupsList[k]->activitiesForSubgroup.append(activei);
			}
			
			for(int j=0; j<act->iTeachersList.count(); j++){
				int k=act->iTeachersList.at(j);
				assert(!internalTeachersList[k]->activitiesForTeacher.contains(activei));
				internalTeachersList[k]->activitiesForTeacher.append(activei);
			}
			
			activei++;
		}
	}

	//activities list for each subject - used for subjects timetable - in order for students and teachers
	activitiesForSubject.resize(nInternalSubjects);
	for(int sb=0; sb<nInternalSubjects; sb++)
		activitiesForSubject[sb].clear();

	for(int i=0; i<this->augmentedYearsList.size(); i++){
		StudentsYear* sty=this->augmentedYearsList[i];

		for(int j=0; j<sty->groupsList.size(); j++){
			StudentsGroup* stg=sty->groupsList[j];

			for(int k=0; k<stg->subgroupsList.size(); k++){
				StudentsSubgroup* sts=stg->subgroupsList[k];
				
				foreach(int ai, internalSubgroupsList[sts->indexInInternalSubgroupsList]->activitiesForSubgroup)
					if(!activitiesForSubject[internalActivitiesList[ai].subjectIndex].contains(ai))
						activitiesForSubject[internalActivitiesList[ai].subjectIndex].append(ai);
			}
		}
	}
	
	for(int i=0; i<nInternalTeachers; i++){
		foreach(int ai, internalTeachersList[i]->activitiesForTeacher)
			if(!activitiesForSubject[internalActivitiesList[ai].subjectIndex].contains(ai))
				activitiesForSubject[internalActivitiesList[ai].subjectIndex].append(ai);
	}
	/////////////////////////////////////////////////////////////////


	bool ok=true;

	//time constraints
	//progress.reset();
	
	bool skipInactiveTimeConstraints=false;
	
	//bool toSkipTime[MAX_TIME_CONSTRAINTS];

	TimeConstraint* tctr;
	
	int _c=0;
	
	for(int tctrindex=0; tctrindex<this->timeConstraintsList.size(); tctrindex++){
		tctr=this->timeConstraintsList[tctrindex];

		if(tctr->hasInactiveActivities(*this)){
			toSkipTime[tctrindex]=true;
		
			if(!skipInactiveTimeConstraints){
				QString s=tr("The following time constraint is ignored, because it refers to inactive activities:");
				s+="\n";
				s+=tctr->getDetailedDescription(*this);
			        /*int t=LongTextMessageBox::mediumConfirmation(NULL, tr("FET information"), s,
                                 tr("Skip rest"), tr("See next"), QString(),
                                 1, 0 );	

				if(t==0)
					skipInactiveTimeConstraints=true;
			*/}
		}
		else{
			toSkipTime[tctrindex]=false;
			_c++;
		}
	}
	
	internalTimeConstraintsList.resize(_c);
	
	//progress.setLabelText(tr("Processing internally the time constraints ... please wait"));
//	progress.setRange(0, timeConstraintsList.size());
	ttt=0;
		
	assert(this->timeConstraintsList.size()<=MAX_TIME_CONSTRAINTS);
	int tctri=0;
	
	for(int tctrindex=0; tctrindex<this->timeConstraintsList.size(); tctrindex++){
		//progress.setValue(ttt);
		//pqapplication->processEvents();
		//if(progress.wasCanceled()){
			/*QMessageBox::information(NULL, tr("FET information"), tr("Canceled"));
			return false;
		*///}
		ttt++;

		tctr=this->timeConstraintsList[tctrindex];
		
		if(toSkipTime[tctrindex])
			continue;
		
		if(!tctr->computeInternalStructure(*this)){
			//assert(0);
			ok=false;
			continue;
		}
		this->internalTimeConstraintsList[tctri++]=tctr;
	}

	//progress.setValue(timeConstraintsList.size());

	this->nInternalTimeConstraints=tctri;
	cout<<_c<<" time constraints after first pass (after removing inactive ones)"<<endl;
	cout<<"  "<<this->nInternalTimeConstraints<<" time constraints after second pass (after removing wrong ones)"<<endl;
	assert(_c>=this->nInternalTimeConstraints); //because some constraints may have toSkipTime false, but computeInternalStructure also false
	assert(this->nInternalTimeConstraints<=MAX_TIME_CONSTRAINTS);
	
	//space constraints
	//progress.reset();
	
	bool skipInactiveSpaceConstraints=false;
	
	//bool toSkipSpace[MAX_SPACE_CONSTRAINTS];
		
	SpaceConstraint* sctr;
	
	_c=0;

	for(int sctrindex=0; sctrindex<this->spaceConstraintsList.size(); sctrindex++){
		sctr=this->spaceConstraintsList[sctrindex];

		if(sctr->hasInactiveActivities(*this)){
			toSkipSpace[sctrindex]=true;
		
			if(!skipInactiveSpaceConstraints){
				QString s=tr("The following space constraint is ignored, because it refers to inactive activities:");
				s+="\n";
				s+=sctr->getDetailedDescription(*this);
                                 /*int t=LongTextMessageBox::mediumConfirmation(NULL, tr("FET information"), s,
                                 tr("Skip rest"), tr("See next"), QString(),
                                 1, 0 );				

				if(t==0)
					skipInactiveSpaceConstraints=true;
			*/}
		}
		else{
			_c++;
			toSkipSpace[sctrindex]=false;
		}
	}
	
	internalSpaceConstraintsList.resize(_c);
	
	//progress.setLabelText(tr("Processing internally the space constraints ... please wait"));
	//progress.setRange(0, spaceConstraintsList.size());
	ttt=0;
	assert(this->spaceConstraintsList.size()<=MAX_SPACE_CONSTRAINTS);

	int sctri=0;

	for(int sctrindex=0; sctrindex<this->spaceConstraintsList.size(); sctrindex++){
		//progress.setValue(ttt);
		//pqapplication->processEvents();
		//if(progress.wasCanceled()){
			/*QMessageBox::information(NULL, tr("FET information"), tr("Canceled"));
			return false;
		*///}
		ttt++;

		sctr=this->spaceConstraintsList[sctrindex];
	
		if(toSkipSpace[sctrindex])
			continue;
		
		if(!sctr->computeInternalStructure(*this)){
			//assert(0);
			ok=false;
			continue;
		}
		this->internalSpaceConstraintsList[sctri++]=sctr;
	}

//	progress.setValue(spaceConstraintsList.size());

	this->nInternalSpaceConstraints=sctri;
	cout<<_c<<" space constraints after first pass (after removing inactive ones)"<<endl;
	cout<<"  "<<this->nInternalSpaceConstraints<<" space constraints after second pass (after removing wrong ones)"<<endl;
	assert(_c>=this->nInternalSpaceConstraints); //because some constraints may have toSkipSpace false, but computeInternalStructure also false
	assert(this->nInternalSpaceConstraints<=MAX_SPACE_CONSTRAINTS);

	//done.
	this->internalStructureComputed=ok;
	return ok;
}

void Rules::kill() //clears memory for the rules, destroyes them
{
	//Teachers
	while(!teachersList.isEmpty())
		delete teachersList.takeFirst();

	//Subjects
	while(!subjectsList.isEmpty())
		delete subjectsList.takeFirst();

	//Activity tags
	while(!activityTagsList.isEmpty())
		delete activityTagsList.takeFirst();



	//Years
	/*while(!yearsList.isEmpty())
		delete yearsList.takeFirst();*/
		
	//students sets
	QList<StudentsYear*> iyears;
	QList<StudentsGroup*> igroups;
	QList<StudentsSubgroup*> isubgroups;
	foreach(StudentsYear* year, yearsList){
		if(!iyears.contains(year))
			iyears.append(year);
		foreach(StudentsGroup* group, year->groupsList){
			if(!igroups.contains(group))
				igroups.append(group);
			foreach(StudentsSubgroup* subgroup, group->subgroupsList){
				if(!isubgroups.contains(subgroup))
					isubgroups.append(subgroup);
			}
		}
	}
	foreach(StudentsYear* year, iyears){
		assert(year!=NULL);
		delete year;
	}
	foreach(StudentsGroup* group, igroups){
		assert(group!=NULL);
		delete group;
	}
	foreach(StudentsSubgroup* subgroup, isubgroups){
		assert(subgroup!=NULL);
		delete subgroup;
	}	
	yearsList.clear();
	//////////////////	

	//kill augmented students sets
	QList<StudentsYear*> ayears;
	QList<StudentsGroup*> agroups;
	QList<StudentsSubgroup*> asubgroups;
	foreach(StudentsYear* year, augmentedYearsList){
		if(!ayears.contains(year))
			ayears.append(year);
		foreach(StudentsGroup* group, year->groupsList){
			if(!agroups.contains(group))
				agroups.append(group);
			foreach(StudentsSubgroup* subgroup, group->subgroupsList){
				if(!asubgroups.contains(subgroup))
					asubgroups.append(subgroup);
			}
		}
	}
	foreach(StudentsYear* year, ayears){
		assert(year!=NULL);
		delete year;
	}
	foreach(StudentsGroup* group, agroups){
		assert(group!=NULL);
		delete group;
	}
	foreach(StudentsSubgroup* subgroup, asubgroups){
		assert(subgroup!=NULL);
		delete subgroup;
	}	
	augmentedYearsList.clear();
	//////////////////	
		
		

	//Activities
	while(!activitiesList.isEmpty())
		delete activitiesList.takeFirst();

	//Time constraints
	while(!timeConstraintsList.isEmpty())
		delete timeConstraintsList.takeFirst();

	//Space constraints
	while(!spaceConstraintsList.isEmpty())
		delete spaceConstraintsList.takeFirst();

	//Buildings
	while(!buildingsList.isEmpty())
		delete buildingsList.takeFirst();

	//Rooms
	while(!roomsList.isEmpty())
		delete roomsList.takeFirst();

	//done
	this->internalStructureComputed=false;
	this->initialized=false;
}

Rules::Rules()
{
	this->initialized=false;
}

Rules::~Rules()
{
	if(this->initialized)
		this->kill();
}

void Rules::setInstitutionName(const QString& newInstitutionName)
{
	this->institutionName=newInstitutionName;
	this->internalStructureComputed=false;
}

void Rules::setComments(const QString& newComments)
{
	this->comments=newComments;
	this->internalStructureComputed=false;
}

bool Rules::addTeacher(Teacher* teacher)
{
	for(int i=0; i<this->teachersList.size(); i++){
		Teacher* tch=this->teachersList[i];
		if(tch->name==teacher->name)
			return false;
	}
	
	this->internalStructureComputed=false;

	teachers_schedule_ready=false;
	students_schedule_ready=false;
	rooms_schedule_ready=false;

	this->teachersList.append(teacher);
	return true;
}

bool Rules::addTeacherFast(Teacher* teacher)
{
	this->internalStructureComputed=false;

	teachers_schedule_ready=false;
	students_schedule_ready=false;
	rooms_schedule_ready=false;

	this->teachersList.append(teacher);
	return true;
}

int Rules::searchTeacher(const QString& teacherName)
{
	for(int i=0; i<this->teachersList.size(); i++)
		if(this->teachersList.at(i)->name==teacherName)
			return i;

	return -1;
}

bool Rules::removeTeacher(const QString& teacherName)
{
	for(int i=0; i<this->activitiesList.size(); ){
		Activity* act=this->activitiesList[i];
		bool t=act->removeTeacher(teacherName);
		if(t && act->teachersNames.size()==0){
			this->removeActivity(act->id, act->activityGroupId);
			i=0;
			//(You have to be careful, there can be erased more activities here)
		}
		else
			i++;
	}
	
	for(int i=0; i<this->timeConstraintsList.size(); ){
		TimeConstraint* ctr=this->timeConstraintsList[i];
		if(ctr->type==CONSTRAINT_TEACHER_NOT_AVAILABLE_TIMES){
			ConstraintTeacherNotAvailableTimes* crt_constraint=(ConstraintTeacherNotAvailableTimes*)ctr;
			if(teacherName==crt_constraint->teacher)
				this->removeTimeConstraint(ctr); //single constraint removal
			else
				i++;
		}
		else
			i++;
	}

	for(int i=0; i<this->timeConstraintsList.size(); ){
		TimeConstraint* ctr=this->timeConstraintsList[i];
		if(ctr->type==CONSTRAINT_TEACHER_MAX_GAPS_PER_WEEK){
			ConstraintTeacherMaxGapsPerWeek* crt_constraint=(ConstraintTeacherMaxGapsPerWeek*)ctr;
			if(teacherName==crt_constraint->teacherName)
				this->removeTimeConstraint(ctr); //single constraint removal
			else
				i++;
		}
		else
			i++;
	}

	for(int i=0; i<this->timeConstraintsList.size(); ){
		TimeConstraint* ctr=this->timeConstraintsList[i];
		if(ctr->type==CONSTRAINT_TEACHER_MAX_GAPS_PER_DAY){
			ConstraintTeacherMaxGapsPerDay* crt_constraint=(ConstraintTeacherMaxGapsPerDay*)ctr;
			if(teacherName==crt_constraint->teacherName)
				this->removeTimeConstraint(ctr); //single constraint removal
			else
				i++;
		}
		else
			i++;
	}

	for(int i=0; i<this->timeConstraintsList.size(); ){
		TimeConstraint* ctr=this->timeConstraintsList[i];
		if(ctr->type==CONSTRAINT_TEACHER_MAX_HOURS_DAILY){
			ConstraintTeacherMaxHoursDaily* crt_constraint=(ConstraintTeacherMaxHoursDaily*)ctr;
			if(teacherName==crt_constraint->teacherName)
				this->removeTimeConstraint(ctr); //single constraint removal
			else
				i++;
		}
		else
			i++;
	}

	for(int i=0; i<this->timeConstraintsList.size(); ){
		TimeConstraint* ctr=this->timeConstraintsList[i];
		if(ctr->type==CONSTRAINT_TEACHER_MAX_HOURS_CONTINUOUSLY){
			ConstraintTeacherMaxHoursContinuously* crt_constraint=(ConstraintTeacherMaxHoursContinuously*)ctr;
			if(teacherName==crt_constraint->teacherName)
				this->removeTimeConstraint(ctr); //single constraint removal
			else
				i++;
		}
		else
			i++;
	}

	for(int i=0; i<this->timeConstraintsList.size(); ){
		TimeConstraint* ctr=this->timeConstraintsList[i];
		if(ctr->type==CONSTRAINT_TEACHER_ACTIVITY_TAG_MAX_HOURS_CONTINUOUSLY){
			ConstraintTeacherActivityTagMaxHoursContinuously* crt_constraint=(ConstraintTeacherActivityTagMaxHoursContinuously*)ctr;
			if(teacherName==crt_constraint->teacherName)
				this->removeTimeConstraint(ctr); //single constraint removal
			else
				i++;
		}
		else
			i++;
	}

	for(int i=0; i<this->timeConstraintsList.size(); ){
		TimeConstraint* ctr=this->timeConstraintsList[i];
		if(ctr->type==CONSTRAINT_TEACHER_ACTIVITY_TAG_MAX_HOURS_DAILY){
			ConstraintTeacherActivityTagMaxHoursDaily* crt_constraint=(ConstraintTeacherActivityTagMaxHoursDaily*)ctr;
			if(teacherName==crt_constraint->teacherName)
				this->removeTimeConstraint(ctr); //single constraint removal
			else
				i++;
		}
		else
			i++;
	}

	for(int i=0; i<this->timeConstraintsList.size(); ){
		TimeConstraint* ctr=this->timeConstraintsList[i];
		if(ctr->type==CONSTRAINT_TEACHER_MIN_HOURS_DAILY){
			ConstraintTeacherMinHoursDaily* crt_constraint=(ConstraintTeacherMinHoursDaily*)ctr;
			if(teacherName==crt_constraint->teacherName)
				this->removeTimeConstraint(ctr); //single constraint removal
			else
				i++;
		}
		else
			i++;
	}

	for(int i=0; i<this->timeConstraintsList.size(); ){
		TimeConstraint* ctr=this->timeConstraintsList[i];
		if(ctr->type==CONSTRAINT_TEACHER_MAX_DAYS_PER_WEEK){
			ConstraintTeacherMaxDaysPerWeek* crt_constraint=(ConstraintTeacherMaxDaysPerWeek*)ctr;
			if(teacherName==crt_constraint->teacherName)
				this->removeTimeConstraint(ctr); //single constraint removal
			else
				i++;
		}
		else
			i++;
	}

	for(int i=0; i<this->timeConstraintsList.size(); ){
		TimeConstraint* ctr=this->timeConstraintsList[i];
		if(ctr->type==CONSTRAINT_TEACHER_MIN_DAYS_PER_WEEK){
			ConstraintTeacherMinDaysPerWeek* crt_constraint=(ConstraintTeacherMinDaysPerWeek*)ctr;
			if(teacherName==crt_constraint->teacherName)
				this->removeTimeConstraint(ctr); //single constraint removal
			else
				i++;
		}
		else
			i++;
	}

	for(int i=0; i<this->timeConstraintsList.size(); ){
		TimeConstraint* ctr=this->timeConstraintsList[i];
		if(ctr->type==CONSTRAINT_TEACHER_INTERVAL_MAX_DAYS_PER_WEEK){
			ConstraintTeacherIntervalMaxDaysPerWeek* crt_constraint=(ConstraintTeacherIntervalMaxDaysPerWeek*)ctr;
			if(teacherName==crt_constraint->teacherName)
				this->removeTimeConstraint(ctr); //single constraint removal
			else
				i++;
		}
		else
			i++;
	}

	for(int i=0; i<this->timeConstraintsList.size(); ){
		TimeConstraint* ctr=this->timeConstraintsList[i];
		if(ctr->type==CONSTRAINT_ACTIVITIES_PREFERRED_TIME_SLOTS){
			ConstraintActivitiesPreferredTimeSlots* crt_constraint=(ConstraintActivitiesPreferredTimeSlots*)ctr;
			if(teacherName==crt_constraint->p_teacherName)
				this->removeTimeConstraint(ctr); //single constraint removal
			else
				i++;
		}
		else
			i++;
	}

	for(int i=0; i<this->timeConstraintsList.size(); ){
		TimeConstraint* ctr=this->timeConstraintsList[i];
		if(ctr->type==CONSTRAINT_ACTIVITIES_PREFERRED_STARTING_TIMES){
			ConstraintActivitiesPreferredStartingTimes* crt_constraint=(ConstraintActivitiesPreferredStartingTimes*)ctr;
			if(teacherName==crt_constraint->teacherName)
				this->removeTimeConstraint(ctr); //single constraint removal
			else
				i++;
		}
		else
			i++;
	}

	for(int i=0; i<this->timeConstraintsList.size(); ){
		TimeConstraint* ctr=this->timeConstraintsList[i];
		if(ctr->type==CONSTRAINT_ACTIVITIES_END_STUDENTS_DAY){
			ConstraintActivitiesEndStudentsDay* crt_constraint=(ConstraintActivitiesEndStudentsDay*)ctr;
			if(teacherName==crt_constraint->teacherName)
				this->removeTimeConstraint(ctr); //single constraint removal
			else
				i++;
		}
		else
			i++;
	}

	for(int i=0; i<this->timeConstraintsList.size(); ){
		TimeConstraint* ctr=this->timeConstraintsList[i];
		if(ctr->type==CONSTRAINT_SUBACTIVITIES_PREFERRED_TIME_SLOTS){
			ConstraintSubactivitiesPreferredTimeSlots* crt_constraint=(ConstraintSubactivitiesPreferredTimeSlots*)ctr;
			if(teacherName==crt_constraint->p_teacherName)
				this->removeTimeConstraint(ctr); //single constraint removal
			else
				i++;
		}
		else
			i++;
	}

	for(int i=0; i<this->timeConstraintsList.size(); ){
		TimeConstraint* ctr=this->timeConstraintsList[i];
		if(ctr->type==CONSTRAINT_SUBACTIVITIES_PREFERRED_STARTING_TIMES){
			ConstraintSubactivitiesPreferredStartingTimes* crt_constraint=(ConstraintSubactivitiesPreferredStartingTimes*)ctr;
			if(teacherName==crt_constraint->teacherName)
				this->removeTimeConstraint(ctr); //single constraint removal
			else
				i++;
		}
		else
			i++;
	}


	for(int i=0; i<this->spaceConstraintsList.size(); ){
		SpaceConstraint* ctr=this->spaceConstraintsList[i];
		if(ctr->type==CONSTRAINT_TEACHER_HOME_ROOM){
			ConstraintTeacherHomeRoom* crt_constraint=(ConstraintTeacherHomeRoom*)ctr;
			if(teacherName==crt_constraint->teacherName)
				this->removeSpaceConstraint(ctr); //single constraint removal
			else
				i++;
		}
		else
			i++;
	}

	for(int i=0; i<this->spaceConstraintsList.size(); ){
		SpaceConstraint* ctr=this->spaceConstraintsList[i];
		if(ctr->type==CONSTRAINT_TEACHER_HOME_ROOMS){
			ConstraintTeacherHomeRooms* crt_constraint=(ConstraintTeacherHomeRooms*)ctr;
			if(teacherName==crt_constraint->teacherName)
				this->removeSpaceConstraint(ctr); //single constraint removal
			else
				i++;
		}
		else
			i++;
	}

	for(int i=0; i<this->spaceConstraintsList.size(); ){
		SpaceConstraint* ctr=this->spaceConstraintsList[i];
		if(ctr->type==CONSTRAINT_TEACHER_MAX_BUILDING_CHANGES_PER_DAY){
			ConstraintTeacherMaxBuildingChangesPerDay* crt_constraint=(ConstraintTeacherMaxBuildingChangesPerDay*)ctr;
			if(teacherName==crt_constraint->teacherName)
				this->removeSpaceConstraint(ctr); //single constraint removal
			else
				i++;
		}
		else
			i++;
	}
	for(int i=0; i<this->spaceConstraintsList.size(); ){
		SpaceConstraint* ctr=this->spaceConstraintsList[i];
		if(ctr->type==CONSTRAINT_TEACHER_MAX_BUILDING_CHANGES_PER_WEEK){
			ConstraintTeacherMaxBuildingChangesPerWeek* crt_constraint=(ConstraintTeacherMaxBuildingChangesPerWeek*)ctr;
			if(teacherName==crt_constraint->teacherName)
				this->removeSpaceConstraint(ctr); //single constraint removal
			else
				i++;
		}
		else
			i++;
	}
	for(int i=0; i<this->spaceConstraintsList.size(); ){
		SpaceConstraint* ctr=this->spaceConstraintsList[i];
		if(ctr->type==CONSTRAINT_TEACHER_MIN_GAPS_BETWEEN_BUILDING_CHANGES){
			ConstraintTeacherMinGapsBetweenBuildingChanges* crt_constraint=(ConstraintTeacherMinGapsBetweenBuildingChanges*)ctr;
			if(teacherName==crt_constraint->teacherName)
				this->removeSpaceConstraint(ctr); //single constraint removal
			else
				i++;
		}
		else
			i++;
	}



	for(int i=0; i<this->teachersList.size(); i++)
		if(this->teachersList.at(i)->name==teacherName){
			Teacher* tch=this->teachersList[i];
			this->teachersList.removeAt(i);
			delete tch;
			break;
		}
	
	this->internalStructureComputed=false;

	teachers_schedule_ready=false;
	students_schedule_ready=false;
	rooms_schedule_ready=false;

	return true;
}

bool Rules::modifyTeacher(const QString& initialTeacherName, const QString& finalTeacherName)
{
	assert(this->searchTeacher(finalTeacherName)==-1);
	assert(this->searchTeacher(initialTeacherName)>=0);

	//TODO: improve this part
	for(int i=0; i<this->activitiesList.size(); i++)
		this->activitiesList.at(i)->renameTeacher(initialTeacherName, finalTeacherName);

	for(int i=0; i<this->timeConstraintsList.size(); i++){
		TimeConstraint* ctr=this->timeConstraintsList[i];	

		if(ctr->type==CONSTRAINT_TEACHER_NOT_AVAILABLE_TIMES){
			ConstraintTeacherNotAvailableTimes* crt_constraint=(ConstraintTeacherNotAvailableTimes*)ctr;
			if(initialTeacherName == crt_constraint->teacher)
				crt_constraint->teacher=finalTeacherName;
		}
	}

	for(int i=0; i<this->timeConstraintsList.size(); i++){
		TimeConstraint* ctr=this->timeConstraintsList[i];	

		if(ctr->type==CONSTRAINT_TEACHER_MAX_GAPS_PER_WEEK){
			ConstraintTeacherMaxGapsPerWeek* crt_constraint=(ConstraintTeacherMaxGapsPerWeek*)ctr;
			if(initialTeacherName == crt_constraint->teacherName)
				crt_constraint->teacherName=finalTeacherName;
		}
	}

	for(int i=0; i<this->timeConstraintsList.size(); i++){
		TimeConstraint* ctr=this->timeConstraintsList[i];	

		if(ctr->type==CONSTRAINT_TEACHER_MAX_GAPS_PER_DAY){
			ConstraintTeacherMaxGapsPerDay* crt_constraint=(ConstraintTeacherMaxGapsPerDay*)ctr;
			if(initialTeacherName == crt_constraint->teacherName)
				crt_constraint->teacherName=finalTeacherName;
		}
	}

	for(int i=0; i<this->timeConstraintsList.size(); i++){
		TimeConstraint* ctr=this->timeConstraintsList[i];	

		if(ctr->type==CONSTRAINT_TEACHER_MAX_HOURS_DAILY){
			ConstraintTeacherMaxHoursDaily* crt_constraint=(ConstraintTeacherMaxHoursDaily*)ctr;
			if(initialTeacherName == crt_constraint->teacherName)
				crt_constraint->teacherName=finalTeacherName;
		}
	}

	for(int i=0; i<this->timeConstraintsList.size(); i++){
		TimeConstraint* ctr=this->timeConstraintsList[i];	

		if(ctr->type==CONSTRAINT_TEACHER_MAX_HOURS_CONTINUOUSLY){
			ConstraintTeacherMaxHoursContinuously* crt_constraint=(ConstraintTeacherMaxHoursContinuously*)ctr;
			if(initialTeacherName == crt_constraint->teacherName)
				crt_constraint->teacherName=finalTeacherName;
		}
	}

	for(int i=0; i<this->timeConstraintsList.size(); i++){
		TimeConstraint* ctr=this->timeConstraintsList[i];	

		if(ctr->type==CONSTRAINT_TEACHER_ACTIVITY_TAG_MAX_HOURS_CONTINUOUSLY){
			ConstraintTeacherActivityTagMaxHoursContinuously* crt_constraint=(ConstraintTeacherActivityTagMaxHoursContinuously*)ctr;
			if(initialTeacherName == crt_constraint->teacherName)
				crt_constraint->teacherName=finalTeacherName;
		}
	}

	for(int i=0; i<this->timeConstraintsList.size(); i++){
		TimeConstraint* ctr=this->timeConstraintsList[i];	

		if(ctr->type==CONSTRAINT_TEACHER_ACTIVITY_TAG_MAX_HOURS_DAILY){
			ConstraintTeacherActivityTagMaxHoursDaily* crt_constraint=(ConstraintTeacherActivityTagMaxHoursDaily*)ctr;
			if(initialTeacherName == crt_constraint->teacherName)
				crt_constraint->teacherName=finalTeacherName;
		}
	}

	for(int i=0; i<this->timeConstraintsList.size(); i++){
		TimeConstraint* ctr=this->timeConstraintsList[i];	

		if(ctr->type==CONSTRAINT_TEACHER_MIN_HOURS_DAILY){
			ConstraintTeacherMinHoursDaily* crt_constraint=(ConstraintTeacherMinHoursDaily*)ctr;
			if(initialTeacherName == crt_constraint->teacherName)
				crt_constraint->teacherName=finalTeacherName;
		}
	}

	for(int i=0; i<this->timeConstraintsList.size(); i++){
		TimeConstraint* ctr=this->timeConstraintsList[i];	

		if(ctr->type==CONSTRAINT_TEACHER_MAX_DAYS_PER_WEEK){
			ConstraintTeacherMaxDaysPerWeek* crt_constraint=(ConstraintTeacherMaxDaysPerWeek*)ctr;
			if(initialTeacherName == crt_constraint->teacherName)
				crt_constraint->teacherName=finalTeacherName;
		}
	}
	
	for(int i=0; i<this->timeConstraintsList.size(); i++){
		TimeConstraint* ctr=this->timeConstraintsList[i];	

		if(ctr->type==CONSTRAINT_TEACHER_MIN_DAYS_PER_WEEK){
			ConstraintTeacherMinDaysPerWeek* crt_constraint=(ConstraintTeacherMinDaysPerWeek*)ctr;
			if(initialTeacherName == crt_constraint->teacherName)
				crt_constraint->teacherName=finalTeacherName;
		}
	}
	
	for(int i=0; i<this->timeConstraintsList.size(); i++){
		TimeConstraint* ctr=this->timeConstraintsList[i];	

		if(ctr->type==CONSTRAINT_TEACHER_INTERVAL_MAX_DAYS_PER_WEEK){
			ConstraintTeacherIntervalMaxDaysPerWeek* crt_constraint=(ConstraintTeacherIntervalMaxDaysPerWeek*)ctr;
			if(initialTeacherName == crt_constraint->teacherName)
				crt_constraint->teacherName=finalTeacherName;
		}
	}
	
	for(int i=0; i<this->timeConstraintsList.size(); i++){
		TimeConstraint* ctr=this->timeConstraintsList[i];	

		if(ctr->type==CONSTRAINT_ACTIVITIES_PREFERRED_TIME_SLOTS){
			ConstraintActivitiesPreferredTimeSlots* crt_constraint=(ConstraintActivitiesPreferredTimeSlots*)ctr;
			if(initialTeacherName == crt_constraint->p_teacherName)
				crt_constraint->p_teacherName=finalTeacherName;
		}
	}
	
	for(int i=0; i<this->timeConstraintsList.size(); i++){
		TimeConstraint* ctr=this->timeConstraintsList[i];	

		if(ctr->type==CONSTRAINT_ACTIVITIES_PREFERRED_STARTING_TIMES){
			ConstraintActivitiesPreferredStartingTimes* crt_constraint=(ConstraintActivitiesPreferredStartingTimes*)ctr;
			if(initialTeacherName == crt_constraint->teacherName)
				crt_constraint->teacherName=finalTeacherName;
		}
	}
	
	for(int i=0; i<this->timeConstraintsList.size(); i++){
		TimeConstraint* ctr=this->timeConstraintsList[i];	

		if(ctr->type==CONSTRAINT_ACTIVITIES_END_STUDENTS_DAY){
			ConstraintActivitiesEndStudentsDay* crt_constraint=(ConstraintActivitiesEndStudentsDay*)ctr;
			if(initialTeacherName == crt_constraint->teacherName)
				crt_constraint->teacherName=finalTeacherName;
		}
	}
	
	for(int i=0; i<this->timeConstraintsList.size(); i++){
		TimeConstraint* ctr=this->timeConstraintsList[i];	

		if(ctr->type==CONSTRAINT_SUBACTIVITIES_PREFERRED_TIME_SLOTS){
			ConstraintSubactivitiesPreferredTimeSlots* crt_constraint=(ConstraintSubactivitiesPreferredTimeSlots*)ctr;
			if(initialTeacherName == crt_constraint->p_teacherName)
				crt_constraint->p_teacherName=finalTeacherName;
		}
	}
	
	for(int i=0; i<this->timeConstraintsList.size(); i++){
		TimeConstraint* ctr=this->timeConstraintsList[i];	

		if(ctr->type==CONSTRAINT_SUBACTIVITIES_PREFERRED_STARTING_TIMES){
			ConstraintSubactivitiesPreferredStartingTimes* crt_constraint=(ConstraintSubactivitiesPreferredStartingTimes*)ctr;
			if(initialTeacherName == crt_constraint->teacherName)
				crt_constraint->teacherName=finalTeacherName;
		}
	}
	

	for(int i=0; i<this->spaceConstraintsList.size(); i++){
		SpaceConstraint* ctr=this->spaceConstraintsList[i];	

		if(ctr->type==CONSTRAINT_TEACHER_HOME_ROOM){
			ConstraintTeacherHomeRoom* crt_constraint=(ConstraintTeacherHomeRoom*)ctr;
			if(initialTeacherName == crt_constraint->teacherName)
				crt_constraint->teacherName=finalTeacherName;
		}
	}
	
	for(int i=0; i<this->spaceConstraintsList.size(); i++){
		SpaceConstraint* ctr=this->spaceConstraintsList[i];	

		if(ctr->type==CONSTRAINT_TEACHER_HOME_ROOMS){
			ConstraintTeacherHomeRooms* crt_constraint=(ConstraintTeacherHomeRooms*)ctr;
			if(initialTeacherName == crt_constraint->teacherName)
				crt_constraint->teacherName=finalTeacherName;
		}
	}
	
	for(int i=0; i<this->spaceConstraintsList.size(); i++){
		SpaceConstraint* ctr=this->spaceConstraintsList[i];	

		if(ctr->type==CONSTRAINT_TEACHER_MAX_BUILDING_CHANGES_PER_DAY){
			ConstraintTeacherMaxBuildingChangesPerDay* crt_constraint=(ConstraintTeacherMaxBuildingChangesPerDay*)ctr;
			if(initialTeacherName == crt_constraint->teacherName)
				crt_constraint->teacherName=finalTeacherName;
		}
	}
	for(int i=0; i<this->spaceConstraintsList.size(); i++){
		SpaceConstraint* ctr=this->spaceConstraintsList[i];	

		if(ctr->type==CONSTRAINT_TEACHER_MAX_BUILDING_CHANGES_PER_WEEK){
			ConstraintTeacherMaxBuildingChangesPerWeek* crt_constraint=(ConstraintTeacherMaxBuildingChangesPerWeek*)ctr;
			if(initialTeacherName == crt_constraint->teacherName)
				crt_constraint->teacherName=finalTeacherName;
		}
	}
	for(int i=0; i<this->spaceConstraintsList.size(); i++){
		SpaceConstraint* ctr=this->spaceConstraintsList[i];	

		if(ctr->type==CONSTRAINT_TEACHER_MIN_GAPS_BETWEEN_BUILDING_CHANGES){
			ConstraintTeacherMinGapsBetweenBuildingChanges* crt_constraint=(ConstraintTeacherMinGapsBetweenBuildingChanges*)ctr;
			if(initialTeacherName == crt_constraint->teacherName)
				crt_constraint->teacherName=finalTeacherName;
		}
	}
	


	int t=0;
	for(int i=0; i<this->teachersList.size(); i++){
		Teacher* tch=this->teachersList[i];

		if(tch->name==initialTeacherName){
			tch->name=finalTeacherName;
			t++;
		}
	}
	assert(t<=1);

	this->internalStructureComputed=false;

	if(t==0)
		return false;
	else
		return true;
}

void Rules::sortTeachersAlphabetically()
{
	qSort(this->teachersList.begin(), this->teachersList.end(), teachersAscending);

	this->internalStructureComputed=false;
}

bool Rules::addSubject(Subject* subject)
{
	for(int i=0; i<this->subjectsList.size(); i++){
		Subject* sbj=this->subjectsList[i];	
		if(sbj->name==subject->name)
			return false;
	}
	
	this->internalStructureComputed=false;

	teachers_schedule_ready=false;
	students_schedule_ready=false;
	rooms_schedule_ready=false;

	this->subjectsList << subject;
	return true;
}

bool Rules::addSubjectFast(Subject* subject)
{
	this->internalStructureComputed=false;

	teachers_schedule_ready=false;
	students_schedule_ready=false;
	rooms_schedule_ready=false;

	this->subjectsList << subject;
	return true;
}

int Rules::searchSubject(const QString& subjectName)
{
	for(int i=0; i<this->subjectsList.size(); i++)
		if(this->subjectsList.at(i)->name == subjectName)
			return i;

	return -1;
}

bool Rules::removeSubject(const QString& subjectName)
{
	//check the activities first
	for(int i=0; i<this->activitiesList.size(); ){
		Activity* act=this->activitiesList[i];
		if(act->subjectName==subjectName){
			this->removeActivity(act->id, act->activityGroupId);
			i=0;
			//(You have to be careful, there can be erased more activities here)
		}
		else
			i++;
	}
	
	//delete the time constraints related to this subject

	for(int i=0; i<this->timeConstraintsList.size(); ){
		TimeConstraint* ctr=this->timeConstraintsList[i];
		if(ctr->type==CONSTRAINT_ACTIVITIES_PREFERRED_TIME_SLOTS){
			ConstraintActivitiesPreferredTimeSlots* crt_constraint=(ConstraintActivitiesPreferredTimeSlots*)ctr;
			if(subjectName==crt_constraint->p_subjectName)
				this->removeTimeConstraint(ctr); //single constraint removal
			else
				i++;
		}
		else
			i++;
	}

	for(int i=0; i<this->timeConstraintsList.size(); ){
		TimeConstraint* ctr=this->timeConstraintsList[i];
		if(ctr->type==CONSTRAINT_ACTIVITIES_PREFERRED_STARTING_TIMES){
			ConstraintActivitiesPreferredStartingTimes* crt_constraint=(ConstraintActivitiesPreferredStartingTimes*)ctr;
			if(subjectName==crt_constraint->subjectName)
				this->removeTimeConstraint(ctr); //single constraint removal
			else
				i++;
		}
		else
			i++;
	}

	for(int i=0; i<this->timeConstraintsList.size(); ){
		TimeConstraint* ctr=this->timeConstraintsList[i];
		if(ctr->type==CONSTRAINT_ACTIVITIES_END_STUDENTS_DAY){
			ConstraintActivitiesEndStudentsDay* crt_constraint=(ConstraintActivitiesEndStudentsDay*)ctr;
			if(subjectName==crt_constraint->subjectName)
				this->removeTimeConstraint(ctr); //single constraint removal
			else
				i++;
		}
		else
			i++;
	}

	for(int i=0; i<this->timeConstraintsList.size(); ){
		TimeConstraint* ctr=this->timeConstraintsList[i];
		if(ctr->type==CONSTRAINT_SUBACTIVITIES_PREFERRED_TIME_SLOTS){
			ConstraintSubactivitiesPreferredTimeSlots* crt_constraint=(ConstraintSubactivitiesPreferredTimeSlots*)ctr;
			if(subjectName==crt_constraint->p_subjectName)
				this->removeTimeConstraint(ctr); //single constraint removal
			else
				i++;
		}
		else
			i++;
	}

	for(int i=0; i<this->timeConstraintsList.size(); ){
		TimeConstraint* ctr=this->timeConstraintsList[i];
		if(ctr->type==CONSTRAINT_SUBACTIVITIES_PREFERRED_STARTING_TIMES){
			ConstraintSubactivitiesPreferredStartingTimes* crt_constraint=(ConstraintSubactivitiesPreferredStartingTimes*)ctr;
			if(subjectName==crt_constraint->subjectName)
				this->removeTimeConstraint(ctr); //single constraint removal
			else
				i++;
		}
		else
			i++;
	}

	//delete the space constraints related to this subject
	for(int i=0; i<this->spaceConstraintsList.size(); ){
		SpaceConstraint* ctr=this->spaceConstraintsList[i];

		if(ctr->type==CONSTRAINT_SUBJECT_PREFERRED_ROOM){
			ConstraintSubjectPreferredRoom* c=(ConstraintSubjectPreferredRoom*)ctr;

			if(c->subjectName == subjectName)
				this->removeSpaceConstraint(ctr);
			else
				i++;
		}
		else if(ctr->type==CONSTRAINT_SUBJECT_PREFERRED_ROOMS){
			ConstraintSubjectPreferredRooms* c=(ConstraintSubjectPreferredRooms*)ctr;

			if(c->subjectName == subjectName)
				this->removeSpaceConstraint(ctr);
			else
				i++;
		}
		else if(ctr->type==CONSTRAINT_SUBJECT_ACTIVITY_TAG_PREFERRED_ROOM){
			ConstraintSubjectActivityTagPreferredRoom* c=(ConstraintSubjectActivityTagPreferredRoom*)ctr;

			if(c->subjectName == subjectName)
				this->removeSpaceConstraint(ctr);
			else
				i++;
		}
		else if(ctr->type==CONSTRAINT_SUBJECT_ACTIVITY_TAG_PREFERRED_ROOMS){
			ConstraintSubjectActivityTagPreferredRooms* c=(ConstraintSubjectActivityTagPreferredRooms*)ctr;

			if(c->subjectName == subjectName)
				this->removeSpaceConstraint(ctr);
			else
				i++;
		}
		else
			i++;
	}

	//remove the subject from the list
	for(int i=0; i<this->subjectsList.size(); i++)
		if(this->subjectsList[i]->name==subjectName){
			Subject* sbj=this->subjectsList[i];
			this->subjectsList.removeAt(i);
			delete sbj;
			break;
		}
	
	this->internalStructureComputed=false;

	teachers_schedule_ready=false;
	students_schedule_ready=false;
	rooms_schedule_ready=false;

	return true;
}

bool Rules::modifySubject(const QString& initialSubjectName, const QString& finalSubjectName)
{
	assert(this->searchSubject(finalSubjectName)==-1);
	assert(this->searchSubject(initialSubjectName)>=0);

	//check the activities first
	for(int i=0; i<this->activitiesList.size(); i++){
		Activity* act=this->activitiesList[i];

		if( act->subjectName == initialSubjectName)
			act->subjectName=finalSubjectName;
	}
	
	//modify the time constraints related to this subject
	for(int i=0; i<this->timeConstraintsList.size(); i++){
		TimeConstraint* ctr=this->timeConstraintsList[i];

		if(ctr->type==CONSTRAINT_ACTIVITIES_PREFERRED_TIME_SLOTS){
			ConstraintActivitiesPreferredTimeSlots* crt_constraint=(ConstraintActivitiesPreferredTimeSlots*)ctr;
			if(initialSubjectName == crt_constraint->p_subjectName)
				crt_constraint->p_subjectName=finalSubjectName;
		}
	}

	//modify the time constraints related to this subject
	for(int i=0; i<this->timeConstraintsList.size(); i++){
		TimeConstraint* ctr=this->timeConstraintsList[i];

		if(ctr->type==CONSTRAINT_ACTIVITIES_PREFERRED_STARTING_TIMES){
			ConstraintActivitiesPreferredStartingTimes* crt_constraint=(ConstraintActivitiesPreferredStartingTimes*)ctr;
			if(initialSubjectName == crt_constraint->subjectName)
				crt_constraint->subjectName=finalSubjectName;
		}
	}

	//modify the time constraints related to this subject
	for(int i=0; i<this->timeConstraintsList.size(); i++){
		TimeConstraint* ctr=this->timeConstraintsList[i];

		if(ctr->type==CONSTRAINT_ACTIVITIES_END_STUDENTS_DAY){
			ConstraintActivitiesEndStudentsDay* crt_constraint=(ConstraintActivitiesEndStudentsDay*)ctr;
			if(initialSubjectName == crt_constraint->subjectName)
				crt_constraint->subjectName=finalSubjectName;
		}
	}

	//modify the time constraints related to this subject
	for(int i=0; i<this->timeConstraintsList.size(); i++){
		TimeConstraint* ctr=this->timeConstraintsList[i];

		if(ctr->type==CONSTRAINT_SUBACTIVITIES_PREFERRED_TIME_SLOTS){
			ConstraintSubactivitiesPreferredTimeSlots* crt_constraint=(ConstraintSubactivitiesPreferredTimeSlots*)ctr;
			if(initialSubjectName == crt_constraint->p_subjectName)
				crt_constraint->p_subjectName=finalSubjectName;
		}
	}

	//modify the time constraints related to this subject
	for(int i=0; i<this->timeConstraintsList.size(); i++){
		TimeConstraint* ctr=this->timeConstraintsList[i];

		if(ctr->type==CONSTRAINT_SUBACTIVITIES_PREFERRED_STARTING_TIMES){
			ConstraintSubactivitiesPreferredStartingTimes* crt_constraint=(ConstraintSubactivitiesPreferredStartingTimes*)ctr;
			if(initialSubjectName == crt_constraint->subjectName)
				crt_constraint->subjectName=finalSubjectName;
		}
	}

	//modify the space constraints related to this subject
	for(int i=0; i<this->spaceConstraintsList.size(); i++){
		SpaceConstraint* ctr=this->spaceConstraintsList[i];

		if(ctr->type==CONSTRAINT_SUBJECT_PREFERRED_ROOM){
			ConstraintSubjectPreferredRoom* c=(ConstraintSubjectPreferredRoom*)ctr;
			if(c->subjectName == initialSubjectName)
				c->subjectName=finalSubjectName;
		}
		else if(ctr->type==CONSTRAINT_SUBJECT_PREFERRED_ROOMS){
			ConstraintSubjectPreferredRooms* c=(ConstraintSubjectPreferredRooms*)ctr;
			if(c->subjectName == initialSubjectName)
				c->subjectName=finalSubjectName;
		}
		else if(ctr->type==CONSTRAINT_SUBJECT_ACTIVITY_TAG_PREFERRED_ROOM){
			ConstraintSubjectActivityTagPreferredRoom* c=(ConstraintSubjectActivityTagPreferredRoom*)ctr;
			if(c->subjectName == initialSubjectName)
				c->subjectName=finalSubjectName;
		}
		else if(ctr->type==CONSTRAINT_SUBJECT_ACTIVITY_TAG_PREFERRED_ROOMS){
			ConstraintSubjectActivityTagPreferredRooms* c=(ConstraintSubjectActivityTagPreferredRooms*)ctr;
			if(c->subjectName == initialSubjectName)
				c->subjectName=finalSubjectName;
		}
	}


	//rename the subject in the list
	int t=0;
	for(int i=0; i<this->subjectsList.size(); i++){
		Subject* sbj=this->subjectsList[i];

		if(sbj->name==initialSubjectName){
			t++;
			sbj->name=finalSubjectName;
		}
	}
	assert(t<=1);

	this->internalStructureComputed=false;

	return true;
}

void Rules::sortSubjectsAlphabetically()
{
	qSort(this->subjectsList.begin(), this->subjectsList.end(), subjectsAscending);

	this->internalStructureComputed=false;
}

bool Rules::addActivityTag(ActivityTag* activityTag)
{
	for(int i=0; i<this->activityTagsList.size(); i++){
		ActivityTag* sbt=this->activityTagsList[i];

		if(sbt->name==activityTag->name)
			return false;
	}

	this->internalStructureComputed=false;

	teachers_schedule_ready=false;
	students_schedule_ready=false;
	rooms_schedule_ready=false;

	this->activityTagsList << activityTag;
	return true;
}

bool Rules::addActivityTagFast(ActivityTag* activityTag)
{
	this->internalStructureComputed=false;

	teachers_schedule_ready=false;
	students_schedule_ready=false;
	rooms_schedule_ready=false;

	this->activityTagsList << activityTag;
	return true;
}

int Rules::searchActivityTag(const QString& activityTagName)
{
	for(int i=0; i<this->activityTagsList.size(); i++)
		if(this->activityTagsList.at(i)->name==activityTagName)
			return i;

	return -1;
}

bool Rules::removeActivityTag(const QString& activityTagName)
{
	//check the activities first
	for(int i=0; i<this->activitiesList.size(); i++){
		Activity* act=this->activitiesList[i];

		//if( act->activityTagName == activityTagName)
		//	act->activityTagName="";
		if( act->activityTagsNames.contains(activityTagName) )
			act->activityTagsNames.removeAll(activityTagName);
	}
	
	//delete the time constraints related to this activity tag
	for(int i=0; i<this->timeConstraintsList.size(); ){
		TimeConstraint* ctr=this->timeConstraintsList[i];
		
		if(ctr->type==CONSTRAINT_TEACHER_ACTIVITY_TAG_MAX_HOURS_CONTINUOUSLY){
			ConstraintTeacherActivityTagMaxHoursContinuously* crt_constraint=(ConstraintTeacherActivityTagMaxHoursContinuously*)ctr;
			if(activityTagName == crt_constraint->activityTagName)
				this->removeTimeConstraint(ctr); //single constraint removal
			else
				i++;
		}
		else
			i++;
	}
	//delete the time constraints related to this activity tag
	for(int i=0; i<this->timeConstraintsList.size(); ){
		TimeConstraint* ctr=this->timeConstraintsList[i];
		
		if(ctr->type==CONSTRAINT_TEACHER_ACTIVITY_TAG_MAX_HOURS_DAILY){
			ConstraintTeacherActivityTagMaxHoursDaily* crt_constraint=(ConstraintTeacherActivityTagMaxHoursDaily*)ctr;
			if(activityTagName == crt_constraint->activityTagName)
				this->removeTimeConstraint(ctr); //single constraint removal
			else
				i++;
		}
		else
			i++;
	}
	//delete the time constraints related to this activity tag
	for(int i=0; i<this->timeConstraintsList.size(); ){
		TimeConstraint* ctr=this->timeConstraintsList[i];
		
		if(ctr->type==CONSTRAINT_TEACHERS_ACTIVITY_TAG_MAX_HOURS_CONTINUOUSLY){
			ConstraintTeachersActivityTagMaxHoursContinuously* crt_constraint=(ConstraintTeachersActivityTagMaxHoursContinuously*)ctr;
			if(activityTagName == crt_constraint->activityTagName)
				this->removeTimeConstraint(ctr); //single constraint removal
			else
				i++;
		}
		else
			i++;
	}
	//delete the time constraints related to this activity tag
	for(int i=0; i<this->timeConstraintsList.size(); ){
		TimeConstraint* ctr=this->timeConstraintsList[i];
		
		if(ctr->type==CONSTRAINT_TEACHERS_ACTIVITY_TAG_MAX_HOURS_DAILY){
			ConstraintTeachersActivityTagMaxHoursDaily* crt_constraint=(ConstraintTeachersActivityTagMaxHoursDaily*)ctr;
			if(activityTagName == crt_constraint->activityTagName)
				this->removeTimeConstraint(ctr); //single constraint removal
			else
				i++;
		}
		else
			i++;
	}
	//delete the time constraints related to this activity tag
	for(int i=0; i<this->timeConstraintsList.size(); ){
		TimeConstraint* ctr=this->timeConstraintsList[i];
		
		if(ctr->type==CONSTRAINT_STUDENTS_ACTIVITY_TAG_MAX_HOURS_CONTINUOUSLY){
			ConstraintStudentsActivityTagMaxHoursContinuously* crt_constraint=(ConstraintStudentsActivityTagMaxHoursContinuously*)ctr;
			if(activityTagName == crt_constraint->activityTagName)
				this->removeTimeConstraint(ctr); //single constraint removal
			else
				i++;
		}
		else
			i++;
	}
	//delete the time constraints related to this activity tag
	for(int i=0; i<this->timeConstraintsList.size(); ){
		TimeConstraint* ctr=this->timeConstraintsList[i];
		
		if(ctr->type==CONSTRAINT_STUDENTS_ACTIVITY_TAG_MAX_HOURS_DAILY){
			ConstraintStudentsActivityTagMaxHoursDaily* crt_constraint=(ConstraintStudentsActivityTagMaxHoursDaily*)ctr;
			if(activityTagName == crt_constraint->activityTagName)
				this->removeTimeConstraint(ctr); //single constraint removal
			else
				i++;
		}
		else
			i++;
	}
	//delete the time constraints related to this activity tag
	for(int i=0; i<this->timeConstraintsList.size(); ){
		TimeConstraint* ctr=this->timeConstraintsList[i];
		
		if(ctr->type==CONSTRAINT_STUDENTS_SET_ACTIVITY_TAG_MAX_HOURS_CONTINUOUSLY){
			ConstraintStudentsSetActivityTagMaxHoursContinuously* crt_constraint=(ConstraintStudentsSetActivityTagMaxHoursContinuously*)ctr;
			if(activityTagName == crt_constraint->activityTagName)
				this->removeTimeConstraint(ctr); //single constraint removal
			else
				i++;
		}
		else
			i++;
	}
	//delete the time constraints related to this activity tag
	for(int i=0; i<this->timeConstraintsList.size(); ){
		TimeConstraint* ctr=this->timeConstraintsList[i];
		
		if(ctr->type==CONSTRAINT_STUDENTS_SET_ACTIVITY_TAG_MAX_HOURS_DAILY){
			ConstraintStudentsSetActivityTagMaxHoursDaily* crt_constraint=(ConstraintStudentsSetActivityTagMaxHoursDaily*)ctr;
			if(activityTagName == crt_constraint->activityTagName)
				this->removeTimeConstraint(ctr); //single constraint removal
			else
				i++;
		}
		else
			i++;
	}

	//delete the time constraints related to this activity tag
	for(int i=0; i<this->timeConstraintsList.size(); ){
		TimeConstraint* ctr=this->timeConstraintsList[i];
		
		if(ctr->type==CONSTRAINT_ACTIVITIES_PREFERRED_TIME_SLOTS){
			ConstraintActivitiesPreferredTimeSlots* crt_constraint=(ConstraintActivitiesPreferredTimeSlots*)ctr;
			if(activityTagName == crt_constraint->p_activityTagName)
				this->removeTimeConstraint(ctr); //single constraint removal
			else
				i++;
		}
		else
			i++;
	}

	//delete the time constraints related to this activity tag
	for(int i=0; i<this->timeConstraintsList.size(); ){
		TimeConstraint* ctr=this->timeConstraintsList[i];
		
		if(ctr->type==CONSTRAINT_ACTIVITIES_PREFERRED_STARTING_TIMES){
			ConstraintActivitiesPreferredStartingTimes* crt_constraint=(ConstraintActivitiesPreferredStartingTimes*)ctr;
			if(activityTagName == crt_constraint->activityTagName)
				this->removeTimeConstraint(ctr); //single constraint removal
			else
				i++;
		}
		else
			i++;
	}

	//delete the time constraints related to this activity tag
	for(int i=0; i<this->timeConstraintsList.size(); ){
		TimeConstraint* ctr=this->timeConstraintsList[i];
		
		if(ctr->type==CONSTRAINT_ACTIVITIES_END_STUDENTS_DAY){
			ConstraintActivitiesEndStudentsDay* crt_constraint=(ConstraintActivitiesEndStudentsDay*)ctr;
			if(activityTagName == crt_constraint->activityTagName)
				this->removeTimeConstraint(ctr); //single constraint removal
			else
				i++;
		}
		else
			i++;
	}

	//delete the time constraints related to this activity tag
	for(int i=0; i<this->timeConstraintsList.size(); ){
		TimeConstraint* ctr=this->timeConstraintsList[i];
		
		if(ctr->type==CONSTRAINT_SUBACTIVITIES_PREFERRED_TIME_SLOTS){
			ConstraintSubactivitiesPreferredTimeSlots* crt_constraint=(ConstraintSubactivitiesPreferredTimeSlots*)ctr;
			if(activityTagName == crt_constraint->p_activityTagName)
				this->removeTimeConstraint(ctr); //single constraint removal
			else
				i++;
		}
		else
			i++;
	}

	//delete the time constraints related to this activity tag
	for(int i=0; i<this->timeConstraintsList.size(); ){
		TimeConstraint* ctr=this->timeConstraintsList[i];
		
		if(ctr->type==CONSTRAINT_SUBACTIVITIES_PREFERRED_STARTING_TIMES){
			ConstraintSubactivitiesPreferredStartingTimes* crt_constraint=(ConstraintSubactivitiesPreferredStartingTimes*)ctr;
			if(activityTagName == crt_constraint->activityTagName)
				this->removeTimeConstraint(ctr); //single constraint removal
			else
				i++;
		}
		else
			i++;
	}

	//delete the space constraints related to this activity tag
	for(int i=0; i<this->spaceConstraintsList.size(); ){
		SpaceConstraint* ctr=this->spaceConstraintsList[i];
		
		if(ctr->type==CONSTRAINT_SUBJECT_ACTIVITY_TAG_PREFERRED_ROOM){
			ConstraintSubjectActivityTagPreferredRoom* c=(ConstraintSubjectActivityTagPreferredRoom*)ctr;

			if(c->activityTagName == activityTagName)
				this->removeSpaceConstraint(ctr);
			else
				i++;
		}
		else if(ctr->type==CONSTRAINT_SUBJECT_ACTIVITY_TAG_PREFERRED_ROOMS){
			ConstraintSubjectActivityTagPreferredRooms* c=(ConstraintSubjectActivityTagPreferredRooms*)ctr;

			if(c->activityTagName == activityTagName)
				this->removeSpaceConstraint(ctr);
			else
				i++;
		}
		else if(ctr->type==CONSTRAINT_ACTIVITY_TAG_PREFERRED_ROOM){
			ConstraintActivityTagPreferredRoom* c=(ConstraintActivityTagPreferredRoom*)ctr;

			if(c->activityTagName == activityTagName)
				this->removeSpaceConstraint(ctr);
			else
				i++;
		}
		else if(ctr->type==CONSTRAINT_ACTIVITY_TAG_PREFERRED_ROOMS){
			ConstraintActivityTagPreferredRooms* c=(ConstraintActivityTagPreferredRooms*)ctr;

			if(c->activityTagName == activityTagName)
				this->removeSpaceConstraint(ctr);
			else
				i++;
		}
		else
			i++;
	}

	//remove the activity tag from the list
	for(int i=0; i<this->activityTagsList.size(); i++)
		if(this->activityTagsList[i]->name==activityTagName){
			ActivityTag* sbt=this->activityTagsList[i];
			this->activityTagsList.removeAt(i);
			delete sbt;
			break;
		}
	
	this->internalStructureComputed=false;

	teachers_schedule_ready=false;
	students_schedule_ready=false;
	rooms_schedule_ready=false;

	return true;
}

bool Rules::modifyActivityTag(const QString& initialActivityTagName, const QString& finalActivityTagName)
{
	assert(this->searchActivityTag(finalActivityTagName)==-1);
	assert(this->searchActivityTag(initialActivityTagName)>=0);

	//check the activities first
	for(int i=0; i<this->activitiesList.size(); i++){
		Activity* act=this->activitiesList[i];

		//if( act->activityTagName == initialActivityTagName)
		//	act->activityTagName=finalActivityTagName;
		for(int kk=0; kk<act->activityTagsNames.count(); kk++)
			if(act->activityTagsNames.at(kk)==initialActivityTagName)
				act->activityTagsNames[kk]=finalActivityTagName;
	}
	
	//modify the constraints related to this activity tag
	for(int i=0; i<this->timeConstraintsList.size(); i++){
		TimeConstraint* ctr=this->timeConstraintsList[i];
	
		if(ctr->type==CONSTRAINT_TEACHER_ACTIVITY_TAG_MAX_HOURS_CONTINUOUSLY){
			ConstraintTeacherActivityTagMaxHoursContinuously* crt_constraint=(ConstraintTeacherActivityTagMaxHoursContinuously*)ctr;
			if(initialActivityTagName == crt_constraint->activityTagName)
				crt_constraint->activityTagName=finalActivityTagName;
		}
	}
	for(int i=0; i<this->timeConstraintsList.size(); i++){
		TimeConstraint* ctr=this->timeConstraintsList[i];
	
		if(ctr->type==CONSTRAINT_TEACHER_ACTIVITY_TAG_MAX_HOURS_DAILY){
			ConstraintTeacherActivityTagMaxHoursDaily* crt_constraint=(ConstraintTeacherActivityTagMaxHoursDaily*)ctr;
			if(initialActivityTagName == crt_constraint->activityTagName)
				crt_constraint->activityTagName=finalActivityTagName;
		}
	}
	for(int i=0; i<this->timeConstraintsList.size(); i++){
		TimeConstraint* ctr=this->timeConstraintsList[i];
	
		if(ctr->type==CONSTRAINT_TEACHERS_ACTIVITY_TAG_MAX_HOURS_CONTINUOUSLY){
			ConstraintTeachersActivityTagMaxHoursContinuously* crt_constraint=(ConstraintTeachersActivityTagMaxHoursContinuously*)ctr;
			if(initialActivityTagName == crt_constraint->activityTagName)
				crt_constraint->activityTagName=finalActivityTagName;
		}
	}
	for(int i=0; i<this->timeConstraintsList.size(); i++){
		TimeConstraint* ctr=this->timeConstraintsList[i];
	
		if(ctr->type==CONSTRAINT_TEACHERS_ACTIVITY_TAG_MAX_HOURS_DAILY){
			ConstraintTeachersActivityTagMaxHoursDaily* crt_constraint=(ConstraintTeachersActivityTagMaxHoursDaily*)ctr;
			if(initialActivityTagName == crt_constraint->activityTagName)
				crt_constraint->activityTagName=finalActivityTagName;
		}
	}
	for(int i=0; i<this->timeConstraintsList.size(); i++){
		TimeConstraint* ctr=this->timeConstraintsList[i];
	
		if(ctr->type==CONSTRAINT_STUDENTS_ACTIVITY_TAG_MAX_HOURS_CONTINUOUSLY){
			ConstraintStudentsActivityTagMaxHoursContinuously* crt_constraint=(ConstraintStudentsActivityTagMaxHoursContinuously*)ctr;
			if(initialActivityTagName == crt_constraint->activityTagName)
				crt_constraint->activityTagName=finalActivityTagName;
		}
	}
	for(int i=0; i<this->timeConstraintsList.size(); i++){
		TimeConstraint* ctr=this->timeConstraintsList[i];
	
		if(ctr->type==CONSTRAINT_STUDENTS_ACTIVITY_TAG_MAX_HOURS_DAILY){
			ConstraintStudentsActivityTagMaxHoursDaily* crt_constraint=(ConstraintStudentsActivityTagMaxHoursDaily*)ctr;
			if(initialActivityTagName == crt_constraint->activityTagName)
				crt_constraint->activityTagName=finalActivityTagName;
		}
	}
	for(int i=0; i<this->timeConstraintsList.size(); i++){
		TimeConstraint* ctr=this->timeConstraintsList[i];
	
		if(ctr->type==CONSTRAINT_STUDENTS_SET_ACTIVITY_TAG_MAX_HOURS_CONTINUOUSLY){
			ConstraintStudentsSetActivityTagMaxHoursContinuously* crt_constraint=(ConstraintStudentsSetActivityTagMaxHoursContinuously*)ctr;
			if(initialActivityTagName == crt_constraint->activityTagName)
				crt_constraint->activityTagName=finalActivityTagName;
		}
	}
	for(int i=0; i<this->timeConstraintsList.size(); i++){
		TimeConstraint* ctr=this->timeConstraintsList[i];
	
		if(ctr->type==CONSTRAINT_STUDENTS_SET_ACTIVITY_TAG_MAX_HOURS_DAILY){
			ConstraintStudentsSetActivityTagMaxHoursDaily* crt_constraint=(ConstraintStudentsSetActivityTagMaxHoursDaily*)ctr;
			if(initialActivityTagName == crt_constraint->activityTagName)
				crt_constraint->activityTagName=finalActivityTagName;
		}
	}

	//modify the constraints related to this activity tag
	for(int i=0; i<this->timeConstraintsList.size(); i++){
		TimeConstraint* ctr=this->timeConstraintsList[i];
	
		if(ctr->type==CONSTRAINT_ACTIVITIES_PREFERRED_TIME_SLOTS){
			ConstraintActivitiesPreferredTimeSlots* crt_constraint=(ConstraintActivitiesPreferredTimeSlots*)ctr;
			if(initialActivityTagName == crt_constraint->p_activityTagName)
				crt_constraint->p_activityTagName=finalActivityTagName;
		}
	}

	//modify the constraints related to this activity tag
	for(int i=0; i<this->timeConstraintsList.size(); i++){
		TimeConstraint* ctr=this->timeConstraintsList[i];
	
		if(ctr->type==CONSTRAINT_ACTIVITIES_PREFERRED_STARTING_TIMES){
			ConstraintActivitiesPreferredStartingTimes* crt_constraint=(ConstraintActivitiesPreferredStartingTimes*)ctr;
			if(initialActivityTagName == crt_constraint->activityTagName)
				crt_constraint->activityTagName=finalActivityTagName;
		}
	}

	//modify the constraints related to this activity tag
	for(int i=0; i<this->timeConstraintsList.size(); i++){
		TimeConstraint* ctr=this->timeConstraintsList[i];
	
		if(ctr->type==CONSTRAINT_ACTIVITIES_END_STUDENTS_DAY){
			ConstraintActivitiesEndStudentsDay* crt_constraint=(ConstraintActivitiesEndStudentsDay*)ctr;
			if(initialActivityTagName == crt_constraint->activityTagName)
				crt_constraint->activityTagName=finalActivityTagName;
		}
	}

	//modify the constraints related to this activity tag
	for(int i=0; i<this->timeConstraintsList.size(); i++){
		TimeConstraint* ctr=this->timeConstraintsList[i];
	
		if(ctr->type==CONSTRAINT_SUBACTIVITIES_PREFERRED_TIME_SLOTS){
			ConstraintSubactivitiesPreferredTimeSlots* crt_constraint=(ConstraintSubactivitiesPreferredTimeSlots*)ctr;
			if(initialActivityTagName == crt_constraint->p_activityTagName)
				crt_constraint->p_activityTagName=finalActivityTagName;
		}
	}

	//modify the constraints related to this activity tag
	for(int i=0; i<this->timeConstraintsList.size(); i++){
		TimeConstraint* ctr=this->timeConstraintsList[i];
	
		if(ctr->type==CONSTRAINT_SUBACTIVITIES_PREFERRED_STARTING_TIMES){
			ConstraintSubactivitiesPreferredStartingTimes* crt_constraint=(ConstraintSubactivitiesPreferredStartingTimes*)ctr;
			if(initialActivityTagName == crt_constraint->activityTagName)
				crt_constraint->activityTagName=finalActivityTagName;
		}
	}

	//modify the space constraints related to this subject tag
	for(int i=0; i<this->spaceConstraintsList.size(); i++){
		SpaceConstraint* ctr=this->spaceConstraintsList[i];

		if(ctr->type==CONSTRAINT_SUBJECT_ACTIVITY_TAG_PREFERRED_ROOM){
			ConstraintSubjectActivityTagPreferredRoom* c=(ConstraintSubjectActivityTagPreferredRoom*)ctr;
			if(c->activityTagName == initialActivityTagName)
				c->activityTagName=finalActivityTagName;
		}
		else if(ctr->type==CONSTRAINT_SUBJECT_ACTIVITY_TAG_PREFERRED_ROOMS){
			ConstraintSubjectActivityTagPreferredRooms* c=(ConstraintSubjectActivityTagPreferredRooms*)ctr;
			if(c->activityTagName == initialActivityTagName)
				c->activityTagName=finalActivityTagName;
		}
		else if(ctr->type==CONSTRAINT_ACTIVITY_TAG_PREFERRED_ROOM){
			ConstraintActivityTagPreferredRoom* c=(ConstraintActivityTagPreferredRoom*)ctr;
			if(c->activityTagName == initialActivityTagName)
				c->activityTagName=finalActivityTagName;
		}
		else if(ctr->type==CONSTRAINT_ACTIVITY_TAG_PREFERRED_ROOMS){
			ConstraintActivityTagPreferredRooms* c=(ConstraintActivityTagPreferredRooms*)ctr;
			if(c->activityTagName == initialActivityTagName)
				c->activityTagName=finalActivityTagName;
		}
	}

	//rename the activity tag in the list
	int t=0;
	
	for(int i=0; i<this->activityTagsList.size(); i++){
		ActivityTag* sbt=this->activityTagsList[i];

		if(sbt->name==initialActivityTagName){
			t++;
			sbt->name=finalActivityTagName;
		}
	}
	
	assert(t<=1);

	this->internalStructureComputed=false;

	return true;
}

void Rules::sortActivityTagsAlphabetically()
{
	qSort(this->activityTagsList.begin(), this->activityTagsList.end(), activityTagsAscending);

	this->internalStructureComputed=false;
}

bool Rules::setsShareStudents(const QString& studentsSet1, const QString& studentsSet2)
{
	StudentsSet* s1=this->searchStudentsSet(studentsSet1);
	StudentsSet* s2=this->searchStudentsSet(studentsSet2);
	assert(s1!=NULL);
	assert(s2!=NULL);
	
	QSet<QString> downwardSets1;
	
	if(s1->type==STUDENTS_YEAR){
		StudentsYear* year1=(StudentsYear*)s1;
		downwardSets1.insert(year1->name);
		foreach(StudentsGroup* group1, year1->groupsList){
			downwardSets1.insert(group1->name);
			foreach(StudentsSubgroup* subgroup1, group1->subgroupsList)
				downwardSets1.insert(subgroup1->name);
		}
	}
	else if(s1->type==STUDENTS_GROUP){
		StudentsGroup* group1=(StudentsGroup*)s1;
		downwardSets1.insert(group1->name);
		foreach(StudentsSubgroup* subgroup1, group1->subgroupsList)
			downwardSets1.insert(subgroup1->name);
	}
	else if(s1->type==STUDENTS_SUBGROUP){
		StudentsSubgroup* subgroup1=(StudentsSubgroup*)s1;
		downwardSets1.insert(subgroup1->name);
	}
	else
		assert(0);
		
	if(s2->type==STUDENTS_YEAR){
		StudentsYear* year2=(StudentsYear*)s2;
		if(downwardSets1.contains(year2->name))
			return true;
		foreach(StudentsGroup* group2, year2->groupsList){
			if(downwardSets1.contains(group2->name))
				return true;
			foreach(StudentsSubgroup* subgroup2, group2->subgroupsList)
				if(downwardSets1.contains(subgroup2->name))
					return true;
		}
	}
	else if(s2->type==STUDENTS_GROUP){
		StudentsGroup* group2=(StudentsGroup*)s2;
		if(downwardSets1.contains(group2->name))
			return true;
		foreach(StudentsSubgroup* subgroup2, group2->subgroupsList)
			if(downwardSets1.contains(subgroup2->name))
				return true;
	}
	else if(s2->type==STUDENTS_SUBGROUP){
		StudentsSubgroup* subgroup2=(StudentsSubgroup*)s2;
		if(downwardSets1.contains(subgroup2->name))
			return true;
	}
	else
		assert(0);
	
	return false;
	
}

StudentsSet* Rules::searchStudentsSet(const QString& setName)
{
	for(int i=0; i<this->yearsList.size(); i++){
		StudentsYear* sty=this->yearsList[i];
		if(sty->name==setName)
			return sty;
		for(int j=0; j<sty->groupsList.size(); j++){
			StudentsGroup* stg=sty->groupsList[j];
			if(stg->name==setName)
				return stg;
			for(int k=0; k<stg->subgroupsList.size(); k++){
				StudentsSubgroup* sts=stg->subgroupsList[k];
				if(sts->name==setName)
					return sts;
			}
		}
	}
	return NULL;
}

StudentsSet* Rules::searchAugmentedStudentsSet(const QString& setName)
{
	for(int i=0; i<this->augmentedYearsList.size(); i++){
		StudentsYear* sty=this->augmentedYearsList[i];
		if(sty->name==setName)
			return sty;
		for(int j=0; j<sty->groupsList.size(); j++){
			StudentsGroup* stg=sty->groupsList[j];
			if(stg->name==setName)
				return stg;
			for(int k=0; k<stg->subgroupsList.size(); k++){
				StudentsSubgroup* sts=stg->subgroupsList[k];
				if(sts->name==setName)
					return sts;
			}
		}
	}
	return NULL;
}

bool Rules::addYear(StudentsYear* year)
{
	//already existing?
	if(this->searchStudentsSet(year->name)!=NULL)
		return false;
	this->yearsList << year;
	this->internalStructureComputed=false;
	return true;
}

bool Rules::addYearFast(StudentsYear* year)
{
	this->yearsList << year;
	this->internalStructureComputed=false;
	return true;
}

bool Rules::removeYear(const QString& yearName)
{
	StudentsYear* year=NULL;
	foreach(StudentsYear* ty, this->yearsList){
		if(ty->name==yearName){
			year=ty;
			break;
		}
	}

	//StudentsYear* year=(StudentsYear*)searchStudentsSet(yearName);
	assert(year!=NULL);
	int nStudents=year->numberOfStudents;
	while(year->groupsList.size()>0){
		QString groupName=year->groupsList[0]->name;
		this->removeGroup(yearName, groupName);
	}

	for(int i=0; i<this->activitiesList.size(); ){
		Activity* act=this->activitiesList[i];
		bool t=act->removeStudents(*this, yearName, nStudents);
		
		if(t && act->studentsNames.count()==0){
			this->removeActivity(act->id, act->activityGroupId);
			i=0;
			//(You have to be careful, there can be erased more activities here)
		}
		else
			i++;
	}
	
	for(int i=0; i<this->timeConstraintsList.size(); ){
		TimeConstraint* ctr=this->timeConstraintsList[i];
	
		bool erased=false;

		if(ctr->type==CONSTRAINT_STUDENTS_SET_NOT_AVAILABLE_TIMES){
			ConstraintStudentsSetNotAvailableTimes* crt_constraint=(ConstraintStudentsSetNotAvailableTimes*)ctr;
			if(yearName == crt_constraint->students){
				this->removeTimeConstraint(ctr);
				erased=true;
			}
		}
		else if(ctr->type==CONSTRAINT_STUDENTS_SET_MAX_HOURS_DAILY){
			ConstraintStudentsSetMaxHoursDaily* crt_constraint=(ConstraintStudentsSetMaxHoursDaily*)ctr;
			if(yearName == crt_constraint->students){
				this->removeTimeConstraint(ctr);
				erased=true;
			}
		}
		else if(ctr->type==CONSTRAINT_STUDENTS_SET_INTERVAL_MAX_DAYS_PER_WEEK){
			ConstraintStudentsSetIntervalMaxDaysPerWeek* crt_constraint=(ConstraintStudentsSetIntervalMaxDaysPerWeek*)ctr;
			if(yearName == crt_constraint->students){
				this->removeTimeConstraint(ctr);
				erased=true;
			}
		}
		else if(ctr->type==CONSTRAINT_STUDENTS_SET_MAX_HOURS_CONTINUOUSLY){
			ConstraintStudentsSetMaxHoursContinuously* crt_constraint=(ConstraintStudentsSetMaxHoursContinuously*)ctr;
			if(yearName == crt_constraint->students){
				this->removeTimeConstraint(ctr);
				erased=true;
			}
		}
		else if(ctr->type==CONSTRAINT_STUDENTS_SET_ACTIVITY_TAG_MAX_HOURS_CONTINUOUSLY){
			ConstraintStudentsSetActivityTagMaxHoursContinuously* crt_constraint=(ConstraintStudentsSetActivityTagMaxHoursContinuously*)ctr;
			if(yearName == crt_constraint->students){
				this->removeTimeConstraint(ctr);
				erased=true;
			}
		}
		else if(ctr->type==CONSTRAINT_STUDENTS_SET_ACTIVITY_TAG_MAX_HOURS_DAILY){
			ConstraintStudentsSetActivityTagMaxHoursDaily* crt_constraint=(ConstraintStudentsSetActivityTagMaxHoursDaily*)ctr;
			if(yearName == crt_constraint->students){
				this->removeTimeConstraint(ctr);
				erased=true;
			}
		}
		else if(ctr->type==CONSTRAINT_STUDENTS_SET_MIN_HOURS_DAILY){
			ConstraintStudentsSetMinHoursDaily* crt_constraint=(ConstraintStudentsSetMinHoursDaily*)ctr;
			if(yearName == crt_constraint->students){
				this->removeTimeConstraint(ctr);
				erased=true;
			}
		}
		else if(ctr->type==CONSTRAINT_STUDENTS_SET_EARLY_MAX_BEGINNINGS_AT_SECOND_HOUR){
			ConstraintStudentsSetEarlyMaxBeginningsAtSecondHour* crt_constraint=(ConstraintStudentsSetEarlyMaxBeginningsAtSecondHour*)ctr;
			if(yearName == crt_constraint->students){
				this->removeTimeConstraint(ctr);
				erased=true;
			}
		}
		else if(ctr->type==CONSTRAINT_STUDENTS_SET_MAX_GAPS_PER_WEEK){
			ConstraintStudentsSetMaxGapsPerWeek* crt_constraint=(ConstraintStudentsSetMaxGapsPerWeek*)ctr;
			if(yearName == crt_constraint->students){
				this->removeTimeConstraint(ctr);
				erased=true;
			}
		}
		else if(ctr->type==CONSTRAINT_STUDENTS_SET_MAX_GAPS_PER_DAY){
			ConstraintStudentsSetMaxGapsPerDay* crt_constraint=(ConstraintStudentsSetMaxGapsPerDay*)ctr;
			if(yearName == crt_constraint->students){
				this->removeTimeConstraint(ctr);
				erased=true;
			}
		}
		else if(ctr->type==CONSTRAINT_ACTIVITIES_PREFERRED_TIME_SLOTS){
			ConstraintActivitiesPreferredTimeSlots* crt_constraint=(ConstraintActivitiesPreferredTimeSlots*)ctr;
			if(yearName == crt_constraint->p_studentsName){
				this->removeTimeConstraint(ctr);
				erased=true;
			}
		}
		else if(ctr->type==CONSTRAINT_ACTIVITIES_PREFERRED_STARTING_TIMES){
			ConstraintActivitiesPreferredStartingTimes* crt_constraint=(ConstraintActivitiesPreferredStartingTimes*)ctr;
			if(yearName == crt_constraint->studentsName){
				this->removeTimeConstraint(ctr);
				erased=true;
			}
		}
		else if(ctr->type==CONSTRAINT_ACTIVITIES_END_STUDENTS_DAY){
			ConstraintActivitiesEndStudentsDay* crt_constraint=(ConstraintActivitiesEndStudentsDay*)ctr;
			if(yearName == crt_constraint->studentsName){
				this->removeTimeConstraint(ctr);
				erased=true;
			}
		}
		else if(ctr->type==CONSTRAINT_SUBACTIVITIES_PREFERRED_TIME_SLOTS){
			ConstraintSubactivitiesPreferredTimeSlots* crt_constraint=(ConstraintSubactivitiesPreferredTimeSlots*)ctr;
			if(yearName == crt_constraint->p_studentsName){
				this->removeTimeConstraint(ctr);
				erased=true;
			}
		}
		else if(ctr->type==CONSTRAINT_SUBACTIVITIES_PREFERRED_STARTING_TIMES){
			ConstraintSubactivitiesPreferredStartingTimes* crt_constraint=(ConstraintSubactivitiesPreferredStartingTimes*)ctr;
			if(yearName == crt_constraint->studentsName){
				this->removeTimeConstraint(ctr);
				erased=true;
			}
		}

		if(!erased)
			i++;
	}

	for(int i=0; i<this->spaceConstraintsList.size(); ){
		SpaceConstraint* ctr=this->spaceConstraintsList[i];
	
		bool erased=false;

		if(ctr->type==CONSTRAINT_STUDENTS_SET_HOME_ROOM){
			ConstraintStudentsSetHomeRoom* crt_constraint=(ConstraintStudentsSetHomeRoom*)ctr;
			if(yearName == crt_constraint->studentsName){
				this->removeSpaceConstraint(ctr);
				erased=true;
			}
		}
		else if(ctr->type==CONSTRAINT_STUDENTS_SET_HOME_ROOMS){
			ConstraintStudentsSetHomeRooms* crt_constraint=(ConstraintStudentsSetHomeRooms*)ctr;
			if(yearName == crt_constraint->studentsName){
				this->removeSpaceConstraint(ctr);
				erased=true;
			}
		}
		else if(ctr->type==CONSTRAINT_STUDENTS_SET_MAX_BUILDING_CHANGES_PER_DAY){
			ConstraintStudentsSetMaxBuildingChangesPerDay* crt_constraint=(ConstraintStudentsSetMaxBuildingChangesPerDay*)ctr;
			if(yearName == crt_constraint->studentsName){
				this->removeSpaceConstraint(ctr);
				erased=true;
			}
		}
		else if(ctr->type==CONSTRAINT_STUDENTS_SET_MAX_BUILDING_CHANGES_PER_WEEK){
			ConstraintStudentsSetMaxBuildingChangesPerWeek* crt_constraint=(ConstraintStudentsSetMaxBuildingChangesPerWeek*)ctr;
			if(yearName == crt_constraint->studentsName){
				this->removeSpaceConstraint(ctr);
				erased=true;
			}
		}
		else if(ctr->type==CONSTRAINT_STUDENTS_SET_MIN_GAPS_BETWEEN_BUILDING_CHANGES){
			ConstraintStudentsSetMinGapsBetweenBuildingChanges* crt_constraint=(ConstraintStudentsSetMinGapsBetweenBuildingChanges*)ctr;
			if(yearName == crt_constraint->studentsName){
				this->removeSpaceConstraint(ctr);
				erased=true;
			}
		}

		if(!erased)
			i++;
	}

	for(int i=0; i<this->yearsList.size(); i++)
		if(this->yearsList.at(i)->name==yearName){
			delete this->yearsList[i]; //added in version 4.0.2
			this->yearsList.removeAt(i);
			break;
		}

	this->internalStructureComputed=false;
	return true;
}

int Rules::searchYear(const QString& yearName)
{
	for(int i=0; i<this->yearsList.size(); i++)
		if(this->yearsList[i]->name==yearName)
			return i;

	return -1;
}

int Rules::searchAugmentedYear(const QString& yearName)
{
	for(int i=0; i<this->augmentedYearsList.size(); i++)
		if(this->augmentedYearsList[i]->name==yearName)
			return i;

	return -1;
}

bool Rules::modifyYear(const QString& initialYearName, const QString& finalYearName, int finalNumberOfStudents)
{
	StudentsSet* _initialSet=searchStudentsSet(initialYearName);
	assert(_initialSet!=NULL);
	int _initialNumberOfStudents=_initialSet->numberOfStudents;

	QString _initialYearName=initialYearName;

	assert(searchYear(_initialYearName)>=0);
	StudentsSet* _ss=searchStudentsSet(finalYearName);
	assert(_ss==NULL || _initialYearName==finalYearName);

	for(int i=0; i<this->activitiesList.size(); i++){
		Activity* act=this->activitiesList[i];
		act->renameStudents(*this, _initialYearName, finalYearName, _initialNumberOfStudents, finalNumberOfStudents);
	}

	for(int i=0; i<this->timeConstraintsList.size(); i++){
		TimeConstraint* ctr=this->timeConstraintsList[i];

		if(ctr->type==CONSTRAINT_STUDENTS_SET_NOT_AVAILABLE_TIMES){
			ConstraintStudentsSetNotAvailableTimes* crt_constraint=(ConstraintStudentsSetNotAvailableTimes*)ctr;
			if(_initialYearName == crt_constraint->students)
				crt_constraint->students=finalYearName;
		}
		else if(ctr->type==CONSTRAINT_STUDENTS_SET_MAX_HOURS_DAILY){
			ConstraintStudentsSetMaxHoursDaily* crt_constraint=(ConstraintStudentsSetMaxHoursDaily*)ctr;
			if(_initialYearName == crt_constraint->students)
				crt_constraint->students=finalYearName;
		}
		else if(ctr->type==CONSTRAINT_STUDENTS_SET_INTERVAL_MAX_DAYS_PER_WEEK){
			ConstraintStudentsSetIntervalMaxDaysPerWeek* crt_constraint=(ConstraintStudentsSetIntervalMaxDaysPerWeek*)ctr;
			if(_initialYearName == crt_constraint->students)
				crt_constraint->students=finalYearName;
		}
		else if(ctr->type==CONSTRAINT_STUDENTS_SET_MAX_HOURS_CONTINUOUSLY){
			ConstraintStudentsSetMaxHoursContinuously* crt_constraint=(ConstraintStudentsSetMaxHoursContinuously*)ctr;
			if(_initialYearName == crt_constraint->students)
				crt_constraint->students=finalYearName;
		}
		else if(ctr->type==CONSTRAINT_STUDENTS_SET_ACTIVITY_TAG_MAX_HOURS_CONTINUOUSLY){
			ConstraintStudentsSetActivityTagMaxHoursContinuously* crt_constraint=(ConstraintStudentsSetActivityTagMaxHoursContinuously*)ctr;
			if(_initialYearName == crt_constraint->students)
				crt_constraint->students=finalYearName;
		}
		else if(ctr->type==CONSTRAINT_STUDENTS_SET_ACTIVITY_TAG_MAX_HOURS_DAILY){
			ConstraintStudentsSetActivityTagMaxHoursDaily* crt_constraint=(ConstraintStudentsSetActivityTagMaxHoursDaily*)ctr;
			if(_initialYearName == crt_constraint->students)
				crt_constraint->students=finalYearName;
		}
		else if(ctr->type==CONSTRAINT_STUDENTS_SET_MIN_HOURS_DAILY){
			ConstraintStudentsSetMinHoursDaily* crt_constraint=(ConstraintStudentsSetMinHoursDaily*)ctr;
			if(_initialYearName == crt_constraint->students)
				crt_constraint->students=finalYearName;
		}
		else if(ctr->type==CONSTRAINT_STUDENTS_SET_EARLY_MAX_BEGINNINGS_AT_SECOND_HOUR){
			ConstraintStudentsSetEarlyMaxBeginningsAtSecondHour* crt_constraint=(ConstraintStudentsSetEarlyMaxBeginningsAtSecondHour*)ctr;
			if(_initialYearName == crt_constraint->students)
				crt_constraint->students=finalYearName;
		}
		else if(ctr->type==CONSTRAINT_STUDENTS_SET_MAX_GAPS_PER_WEEK){
			ConstraintStudentsSetMaxGapsPerWeek* crt_constraint=(ConstraintStudentsSetMaxGapsPerWeek*)ctr;
			if(_initialYearName == crt_constraint->students)
				crt_constraint->students=finalYearName;
		}
		else if(ctr->type==CONSTRAINT_STUDENTS_SET_MAX_GAPS_PER_DAY){
			ConstraintStudentsSetMaxGapsPerDay* crt_constraint=(ConstraintStudentsSetMaxGapsPerDay*)ctr;
			if(_initialYearName == crt_constraint->students)
				crt_constraint->students=finalYearName;
		}
		else if(ctr->type==CONSTRAINT_ACTIVITIES_PREFERRED_TIME_SLOTS){
			ConstraintActivitiesPreferredTimeSlots* crt_constraint=(ConstraintActivitiesPreferredTimeSlots*)ctr;
			if(_initialYearName == crt_constraint->p_studentsName)
				crt_constraint->p_studentsName=finalYearName;
		}
		else if(ctr->type==CONSTRAINT_ACTIVITIES_PREFERRED_STARTING_TIMES){
			ConstraintActivitiesPreferredStartingTimes* crt_constraint=(ConstraintActivitiesPreferredStartingTimes*)ctr;
			if(_initialYearName == crt_constraint->studentsName)
				crt_constraint->studentsName=finalYearName;
		}
		else if(ctr->type==CONSTRAINT_ACTIVITIES_END_STUDENTS_DAY){
			ConstraintActivitiesEndStudentsDay* crt_constraint=(ConstraintActivitiesEndStudentsDay*)ctr;
			if(_initialYearName == crt_constraint->studentsName)
				crt_constraint->studentsName=finalYearName;
		}
		else if(ctr->type==CONSTRAINT_SUBACTIVITIES_PREFERRED_TIME_SLOTS){
			ConstraintSubactivitiesPreferredTimeSlots* crt_constraint=(ConstraintSubactivitiesPreferredTimeSlots*)ctr;
			if(_initialYearName == crt_constraint->p_studentsName)
				crt_constraint->p_studentsName=finalYearName;
		}
		else if(ctr->type==CONSTRAINT_SUBACTIVITIES_PREFERRED_STARTING_TIMES){
			ConstraintSubactivitiesPreferredStartingTimes* crt_constraint=(ConstraintSubactivitiesPreferredStartingTimes*)ctr;
			if(_initialYearName == crt_constraint->studentsName)
				crt_constraint->studentsName=finalYearName;
		}
	}

	for(int i=0; i<this->spaceConstraintsList.size(); i++){
		SpaceConstraint* ctr=this->spaceConstraintsList[i];

		if(ctr->type==CONSTRAINT_STUDENTS_SET_HOME_ROOM){
			ConstraintStudentsSetHomeRoom* crt_constraint=(ConstraintStudentsSetHomeRoom*)ctr;
			if(_initialYearName == crt_constraint->studentsName)
				crt_constraint->studentsName=finalYearName;
		}
		else if(ctr->type==CONSTRAINT_STUDENTS_SET_HOME_ROOMS){
			ConstraintStudentsSetHomeRooms* crt_constraint=(ConstraintStudentsSetHomeRooms*)ctr;
			if(_initialYearName == crt_constraint->studentsName)
				crt_constraint->studentsName=finalYearName;
		}
		else if(ctr->type==CONSTRAINT_STUDENTS_SET_MAX_BUILDING_CHANGES_PER_DAY){
			ConstraintStudentsSetMaxBuildingChangesPerDay* crt_constraint=(ConstraintStudentsSetMaxBuildingChangesPerDay*)ctr;
			if(_initialYearName == crt_constraint->studentsName)
				crt_constraint->studentsName=finalYearName;
		}
		else if(ctr->type==CONSTRAINT_STUDENTS_SET_MAX_BUILDING_CHANGES_PER_WEEK){
			ConstraintStudentsSetMaxBuildingChangesPerWeek* crt_constraint=(ConstraintStudentsSetMaxBuildingChangesPerWeek*)ctr;
			if(_initialYearName == crt_constraint->studentsName)
				crt_constraint->studentsName=finalYearName;
		}
		else if(ctr->type==CONSTRAINT_STUDENTS_SET_MIN_GAPS_BETWEEN_BUILDING_CHANGES){
			ConstraintStudentsSetMinGapsBetweenBuildingChanges* crt_constraint=(ConstraintStudentsSetMinGapsBetweenBuildingChanges*)ctr;
			if(_initialYearName == crt_constraint->studentsName)
				crt_constraint->studentsName=finalYearName;
		}
	}

	int t=0;
	
	for(int i=0; i<this->yearsList.size(); i++){
		StudentsYear* sty=this->yearsList[i];
	
		if(sty->name==_initialYearName){
			sty->name=finalYearName;
			sty->numberOfStudents=finalNumberOfStudents;
			t++;
			
			/*for(int j=0; j<sty->groupsList.size(); j++){
				StudentsGroup* stg=sty->groupsList[j];

				if(stg->name.right(11)==" WHOLE YEAR" && stg->name.left(stg->name.length()-11)==_initialYearName)
					this->modifyGroup(sty->name, stg->name, sty->name+" WHOLE YEAR", stg->numberOfStudents);
			}*/
		}
	}
		
	assert(t<=1);

	this->internalStructureComputed=false;
	
	if(t==0)
		return false;
	else
		return true;
}

void Rules::sortYearsAlphabetically()
{
	qSort(this->yearsList.begin(), this->yearsList.end(), yearsAscending);

	this->internalStructureComputed=false;
}

bool Rules::addGroup(const QString& yearName, StudentsGroup* group)
{
	StudentsYear* sty=NULL;
	for(int i=0; i<this->yearsList.size(); i++){
		sty=yearsList[i];
		if(sty->name==yearName)
			break;
	}
	assert(sty);
	
	for(int i=0; i<sty->groupsList.size(); i++){
		StudentsGroup* stg=sty->groupsList[i];
		if(stg->name==group->name)
			return false;
	}
	
	sty->groupsList << group; //append

	/*
	foreach(StudentsYear* y, yearsList)
		foreach(StudentsGroup* g, y->groupsList)
			if(g->name==group->name)
				g->numberOfStudents=group->numberOfStudents;*/

	this->internalStructureComputed=false;
	return true;
}

bool Rules::addGroupFast(StudentsYear* year, StudentsGroup* group)
{
	year->groupsList << group; //append

	this->internalStructureComputed=false;
	return true;
}

bool Rules::removeGroup(const QString& yearName, const QString& groupName)
{
	this->internalStructureComputed=false;
	
	StudentsYear* year=NULL;
	foreach(StudentsYear* ty, this->yearsList){
		if(ty->name==yearName){
			year=ty;
			break;
		}
	}
	assert(year!=NULL);
	
	StudentsGroup* group=NULL;
	foreach(StudentsGroup* tg, year->groupsList){
		if(tg->name==groupName){
			group=tg;
			break;
		}
	}

	//StudentsGroup* group=(StudentsGroup*)searchStudentsSet(groupName);
	assert(group!=NULL);
	int nStudents=group->numberOfStudents;
	
	bool stillExistsWithSamePointer=false;
	foreach(StudentsYear* ty, this->yearsList){
		if(ty->name!=yearName){
			foreach(StudentsGroup* tg, ty->groupsList){
				if(tg==group){
					stillExistsWithSamePointer=true;
					break;
				}
			}
		}
		if(stillExistsWithSamePointer)
			break;
	}
	
	if(!stillExistsWithSamePointer){
		while(group->subgroupsList.size()>0){
			QString subgroupName=group->subgroupsList[0]->name;
			this->removeSubgroup(yearName, groupName, subgroupName);
		}
	}

	StudentsYear* sty=NULL;
	for(int i=0; i<this->yearsList.size(); i++){
		sty=yearsList[i];
		if(sty->name==yearName)
			break;
	}
	assert(sty);
	assert(sty==year);

	StudentsGroup* stg=NULL;
	for(int i=0; i<sty->groupsList.size(); i++){
		stg=sty->groupsList[i];
		if(stg->name==groupName){
			sty->groupsList.removeAt(i);
			break;
		}
	}

	if(this->searchStudentsSet(stg->name)!=NULL){
		//group still exists
		
		//with the same pointer??? (leak bug fix on 6 Jan 2010 in FET-5.12.1)
		bool foundSamePointer=false;
		foreach(StudentsYear* year, yearsList){
			foreach(StudentsGroup* group, year->groupsList){
				if(group==stg){
					foundSamePointer=true;
					break;
				}
			}
			if(foundSamePointer)
				break;
		}
		
		if(!foundSamePointer)
			delete stg;
		/////////////////////////////////////////////////////////////////////
		
		return true;
	}

	delete stg;

	for(int i=0; i<this->activitiesList.size(); ){
		Activity* act=this->activitiesList[i];

		bool t=act->removeStudents(*this, groupName, nStudents);
		if(t && act->studentsNames.count()==0){
			this->removeActivity(act->id, act->activityGroupId);
			i=0;
			//(You have to be careful, there can be erased more activities here)
		}
		else
			i++;
	}

	for(int i=0; i<this->timeConstraintsList.size(); ){
		TimeConstraint* ctr=this->timeConstraintsList[i];

		bool erased=false;
		if(ctr->type==CONSTRAINT_STUDENTS_SET_NOT_AVAILABLE_TIMES){
			ConstraintStudentsSetNotAvailableTimes* crt_constraint=(ConstraintStudentsSetNotAvailableTimes*)ctr;
			if(groupName == crt_constraint->students){
				this->removeTimeConstraint(ctr);
				erased=true;
			}
		}
		else if(ctr->type==CONSTRAINT_STUDENTS_SET_MAX_HOURS_DAILY){
			ConstraintStudentsSetMaxHoursDaily* crt_constraint=(ConstraintStudentsSetMaxHoursDaily*)ctr;
			if(groupName == crt_constraint->students){
				this->removeTimeConstraint(ctr);
				erased=true;
			}
		}
		else if(ctr->type==CONSTRAINT_STUDENTS_SET_INTERVAL_MAX_DAYS_PER_WEEK){
			ConstraintStudentsSetIntervalMaxDaysPerWeek* crt_constraint=(ConstraintStudentsSetIntervalMaxDaysPerWeek*)ctr;
			if(groupName == crt_constraint->students){
				this->removeTimeConstraint(ctr);
				erased=true;
			}
		}
		else if(ctr->type==CONSTRAINT_STUDENTS_SET_MAX_HOURS_CONTINUOUSLY){
			ConstraintStudentsSetMaxHoursContinuously* crt_constraint=(ConstraintStudentsSetMaxHoursContinuously*)ctr;
			if(groupName == crt_constraint->students){
				this->removeTimeConstraint(ctr);
				erased=true;
			}
		}
		else if(ctr->type==CONSTRAINT_STUDENTS_SET_ACTIVITY_TAG_MAX_HOURS_CONTINUOUSLY){
			ConstraintStudentsSetActivityTagMaxHoursContinuously* crt_constraint=(ConstraintStudentsSetActivityTagMaxHoursContinuously*)ctr;
			if(groupName == crt_constraint->students){
				this->removeTimeConstraint(ctr);
				erased=true;
			}
		}
		else if(ctr->type==CONSTRAINT_STUDENTS_SET_ACTIVITY_TAG_MAX_HOURS_DAILY){
			ConstraintStudentsSetActivityTagMaxHoursDaily* crt_constraint=(ConstraintStudentsSetActivityTagMaxHoursDaily*)ctr;
			if(groupName == crt_constraint->students){
				this->removeTimeConstraint(ctr);
				erased=true;
			}
		}
		else if(ctr->type==CONSTRAINT_STUDENTS_SET_MIN_HOURS_DAILY){
			ConstraintStudentsSetMinHoursDaily* crt_constraint=(ConstraintStudentsSetMinHoursDaily*)ctr;
			if(groupName == crt_constraint->students){
				this->removeTimeConstraint(ctr);
				erased=true;
			}
		}
		else if(ctr->type==CONSTRAINT_STUDENTS_SET_EARLY_MAX_BEGINNINGS_AT_SECOND_HOUR){
			ConstraintStudentsSetEarlyMaxBeginningsAtSecondHour* crt_constraint=(ConstraintStudentsSetEarlyMaxBeginningsAtSecondHour*)ctr;
			if(groupName == crt_constraint->students){
				this->removeTimeConstraint(ctr);
				erased=true;
			}
		}
		else if(ctr->type==CONSTRAINT_STUDENTS_SET_MAX_GAPS_PER_WEEK){
			ConstraintStudentsSetMaxGapsPerWeek* crt_constraint=(ConstraintStudentsSetMaxGapsPerWeek*)ctr;
			if(groupName == crt_constraint->students){
				this->removeTimeConstraint(ctr);
				erased=true;
			}
		}
		else if(ctr->type==CONSTRAINT_STUDENTS_SET_MAX_GAPS_PER_DAY){
			ConstraintStudentsSetMaxGapsPerDay* crt_constraint=(ConstraintStudentsSetMaxGapsPerDay*)ctr;
			if(groupName == crt_constraint->students){
				this->removeTimeConstraint(ctr);
				erased=true;
			}
		}
		else if(ctr->type==CONSTRAINT_ACTIVITIES_PREFERRED_TIME_SLOTS){
			ConstraintActivitiesPreferredTimeSlots* crt_constraint=(ConstraintActivitiesPreferredTimeSlots*)ctr;
			if(groupName == crt_constraint->p_studentsName){
				this->removeTimeConstraint(ctr);
				erased=true;
			}
		}
		else if(ctr->type==CONSTRAINT_ACTIVITIES_PREFERRED_STARTING_TIMES){
			ConstraintActivitiesPreferredStartingTimes* crt_constraint=(ConstraintActivitiesPreferredStartingTimes*)ctr;
			if(groupName == crt_constraint->studentsName){
				this->removeTimeConstraint(ctr);
				erased=true;
			}
		}
		else if(ctr->type==CONSTRAINT_ACTIVITIES_END_STUDENTS_DAY){
			ConstraintActivitiesEndStudentsDay* crt_constraint=(ConstraintActivitiesEndStudentsDay*)ctr;
			if(groupName == crt_constraint->studentsName){
				this->removeTimeConstraint(ctr);
				erased=true;
			}
		}
		else if(ctr->type==CONSTRAINT_SUBACTIVITIES_PREFERRED_TIME_SLOTS){
			ConstraintSubactivitiesPreferredTimeSlots* crt_constraint=(ConstraintSubactivitiesPreferredTimeSlots*)ctr;
			if(groupName == crt_constraint->p_studentsName){
				this->removeTimeConstraint(ctr);
				erased=true;
			}
		}
		else if(ctr->type==CONSTRAINT_SUBACTIVITIES_PREFERRED_STARTING_TIMES){
			ConstraintSubactivitiesPreferredStartingTimes* crt_constraint=(ConstraintSubactivitiesPreferredStartingTimes*)ctr;
			if(groupName == crt_constraint->studentsName){
				this->removeTimeConstraint(ctr);
				erased=true;
			}
		}

		if(!erased)
			i++;
	}

	for(int i=0; i<this->spaceConstraintsList.size(); ){
		SpaceConstraint* ctr=this->spaceConstraintsList[i];

		bool erased=false;
		if(ctr->type==CONSTRAINT_STUDENTS_SET_HOME_ROOM){
			ConstraintStudentsSetHomeRoom* crt_constraint=(ConstraintStudentsSetHomeRoom*)ctr;
			if(groupName == crt_constraint->studentsName){
				this->removeSpaceConstraint(ctr);
				erased=true;
			}
		}
		else if(ctr->type==CONSTRAINT_STUDENTS_SET_HOME_ROOMS){
			ConstraintStudentsSetHomeRooms* crt_constraint=(ConstraintStudentsSetHomeRooms*)ctr;
			if(groupName == crt_constraint->studentsName){
				this->removeSpaceConstraint(ctr);
				erased=true;
			}
		}
		else if(ctr->type==CONSTRAINT_STUDENTS_SET_MAX_BUILDING_CHANGES_PER_DAY){
			ConstraintStudentsSetMaxBuildingChangesPerDay* crt_constraint=(ConstraintStudentsSetMaxBuildingChangesPerDay*)ctr;
			if(groupName == crt_constraint->studentsName){
				this->removeSpaceConstraint(ctr);
				erased=true;
			}
		}
		else if(ctr->type==CONSTRAINT_STUDENTS_SET_MAX_BUILDING_CHANGES_PER_WEEK){
			ConstraintStudentsSetMaxBuildingChangesPerWeek* crt_constraint=(ConstraintStudentsSetMaxBuildingChangesPerWeek*)ctr;
			if(groupName == crt_constraint->studentsName){
				this->removeSpaceConstraint(ctr);
				erased=true;
			}
		}
		else if(ctr->type==CONSTRAINT_STUDENTS_SET_MIN_GAPS_BETWEEN_BUILDING_CHANGES){
			ConstraintStudentsSetMinGapsBetweenBuildingChanges* crt_constraint=(ConstraintStudentsSetMinGapsBetweenBuildingChanges*)ctr;
			if(groupName == crt_constraint->studentsName){
				this->removeSpaceConstraint(ctr);
				erased=true;
			}
		}

		if(!erased)
			i++;
	}

	return true;
}

int Rules::searchGroup(const QString& yearName, const QString& groupName)
{
	StudentsYear* sty=this->yearsList[this->searchYear(yearName)];
	assert(sty);
	
	for(int i=0; i<sty->groupsList.size(); i++)
		if(sty->groupsList[i]->name==groupName)
			return i;
	
	return -1;
}

int Rules::searchAugmentedGroup(const QString& yearName, const QString& groupName)
{
	StudentsYear* sty=this->augmentedYearsList[this->searchAugmentedYear(yearName)];
	assert(sty);
	
	for(int i=0; i<sty->groupsList.size(); i++)
		if(sty->groupsList[i]->name==groupName)
			return i;
	
	return -1;
}

bool Rules::modifyGroup(const QString& yearName, const QString& initialGroupName, const QString& finalGroupName, int finalNumberOfStudents)
{
	StudentsSet* _initialSet=searchStudentsSet(initialGroupName);
	assert(_initialSet!=NULL);
	int _initialNumberOfStudents=_initialSet->numberOfStudents;

	//cout<<"Begin: initialGroupName=='"<<qPrintableinitialGroupName<<"'"<<endl;
	
	QString _initialGroupName=initialGroupName;

	assert(searchGroup(yearName, _initialGroupName)>=0);
	StudentsSet* _ss=searchStudentsSet(finalGroupName);
	assert(_ss==NULL || _initialGroupName==finalGroupName);

	StudentsYear* sty=NULL;
	for(int i=0; i<this->yearsList.size(); i++){
		sty=this->yearsList[i];
		if(sty->name==yearName)
			break;
	}
	assert(sty);
	
	StudentsGroup* stg=NULL;
	for(int i=0; i<sty->groupsList.size(); i++){
		stg=sty->groupsList[i];
		if(stg->name==_initialGroupName){
			stg->name=finalGroupName;
			stg->numberOfStudents=finalNumberOfStudents;

			break;
		}
	}
	assert(stg);
	
	if(_ss!=NULL){ //In case it only changes the number of students, make the same number of students in all groups with this name
		assert(_initialGroupName==finalGroupName);
		foreach(StudentsYear* year, yearsList)
			foreach(StudentsGroup* group, year->groupsList)
				if(group->name==finalGroupName)
					group->numberOfStudents=finalNumberOfStudents;
	}

	for(int i=0; i<this->activitiesList.size(); i++)
		this->activitiesList[i]->renameStudents(*this, _initialGroupName, finalGroupName, _initialNumberOfStudents, finalNumberOfStudents);
	
	for(int i=0; i<this->timeConstraintsList.size(); i++){
		TimeConstraint* ctr=this->timeConstraintsList[i];

		if(ctr->type==CONSTRAINT_STUDENTS_SET_NOT_AVAILABLE_TIMES){
			ConstraintStudentsSetNotAvailableTimes* crt_constraint=(ConstraintStudentsSetNotAvailableTimes*)ctr;
			if(_initialGroupName == crt_constraint->students)
				crt_constraint->students=finalGroupName;
		}
		else if(ctr->type==CONSTRAINT_STUDENTS_SET_MAX_HOURS_DAILY){
			ConstraintStudentsSetMaxHoursDaily* crt_constraint=(ConstraintStudentsSetMaxHoursDaily*)ctr;
			if(_initialGroupName == crt_constraint->students)
				crt_constraint->students=finalGroupName;
		}
		else if(ctr->type==CONSTRAINT_STUDENTS_SET_INTERVAL_MAX_DAYS_PER_WEEK){
			ConstraintStudentsSetIntervalMaxDaysPerWeek* crt_constraint=(ConstraintStudentsSetIntervalMaxDaysPerWeek*)ctr;
			if(_initialGroupName == crt_constraint->students)
				crt_constraint->students=finalGroupName;
		}
		else if(ctr->type==CONSTRAINT_STUDENTS_SET_MAX_HOURS_CONTINUOUSLY){
			ConstraintStudentsSetMaxHoursContinuously* crt_constraint=(ConstraintStudentsSetMaxHoursContinuously*)ctr;
			if(_initialGroupName == crt_constraint->students)
				crt_constraint->students=finalGroupName;
		}
		else if(ctr->type==CONSTRAINT_STUDENTS_SET_ACTIVITY_TAG_MAX_HOURS_CONTINUOUSLY){
			ConstraintStudentsSetActivityTagMaxHoursContinuously* crt_constraint=(ConstraintStudentsSetActivityTagMaxHoursContinuously*)ctr;
			if(_initialGroupName == crt_constraint->students)
				crt_constraint->students=finalGroupName;
		}
		else if(ctr->type==CONSTRAINT_STUDENTS_SET_ACTIVITY_TAG_MAX_HOURS_DAILY){
			ConstraintStudentsSetActivityTagMaxHoursDaily* crt_constraint=(ConstraintStudentsSetActivityTagMaxHoursDaily*)ctr;
			if(_initialGroupName == crt_constraint->students)
				crt_constraint->students=finalGroupName;
		}
		else if(ctr->type==CONSTRAINT_STUDENTS_SET_MIN_HOURS_DAILY){
			ConstraintStudentsSetMinHoursDaily* crt_constraint=(ConstraintStudentsSetMinHoursDaily*)ctr;
			if(_initialGroupName == crt_constraint->students)
				crt_constraint->students=finalGroupName;
		}
		else if(ctr->type==CONSTRAINT_STUDENTS_SET_EARLY_MAX_BEGINNINGS_AT_SECOND_HOUR){
			ConstraintStudentsSetEarlyMaxBeginningsAtSecondHour* crt_constraint=(ConstraintStudentsSetEarlyMaxBeginningsAtSecondHour*)ctr;
			if(_initialGroupName == crt_constraint->students)
				crt_constraint->students=finalGroupName;
		}
		else if(ctr->type==CONSTRAINT_STUDENTS_SET_MAX_GAPS_PER_WEEK){
			ConstraintStudentsSetMaxGapsPerWeek* crt_constraint=(ConstraintStudentsSetMaxGapsPerWeek*)ctr;
			if(_initialGroupName == crt_constraint->students)
				crt_constraint->students=finalGroupName;
		}
		else if(ctr->type==CONSTRAINT_STUDENTS_SET_MAX_GAPS_PER_DAY){
			ConstraintStudentsSetMaxGapsPerDay* crt_constraint=(ConstraintStudentsSetMaxGapsPerDay*)ctr;
			if(_initialGroupName == crt_constraint->students)
				crt_constraint->students=finalGroupName;
		}
		else if(ctr->type==CONSTRAINT_ACTIVITIES_PREFERRED_TIME_SLOTS){
			ConstraintActivitiesPreferredTimeSlots* crt_constraint=(ConstraintActivitiesPreferredTimeSlots*)ctr;
			if(_initialGroupName == crt_constraint->p_studentsName)
				crt_constraint->p_studentsName=finalGroupName;
		}
		else if(ctr->type==CONSTRAINT_ACTIVITIES_PREFERRED_STARTING_TIMES){
			ConstraintActivitiesPreferredStartingTimes* crt_constraint=(ConstraintActivitiesPreferredStartingTimes*)ctr;
			if(_initialGroupName == crt_constraint->studentsName)
				crt_constraint->studentsName=finalGroupName;
		}
		else if(ctr->type==CONSTRAINT_ACTIVITIES_END_STUDENTS_DAY){
			ConstraintActivitiesEndStudentsDay* crt_constraint=(ConstraintActivitiesEndStudentsDay*)ctr;
			if(_initialGroupName == crt_constraint->studentsName)
				crt_constraint->studentsName=finalGroupName;
		}
		else if(ctr->type==CONSTRAINT_SUBACTIVITIES_PREFERRED_TIME_SLOTS){
			ConstraintSubactivitiesPreferredTimeSlots* crt_constraint=(ConstraintSubactivitiesPreferredTimeSlots*)ctr;
			if(_initialGroupName == crt_constraint->p_studentsName)
				crt_constraint->p_studentsName=finalGroupName;
		}
		else if(ctr->type==CONSTRAINT_SUBACTIVITIES_PREFERRED_STARTING_TIMES){
			ConstraintSubactivitiesPreferredStartingTimes* crt_constraint=(ConstraintSubactivitiesPreferredStartingTimes*)ctr;
			if(_initialGroupName == crt_constraint->studentsName)
				crt_constraint->studentsName=finalGroupName;
		}
	}

	for(int i=0; i<this->spaceConstraintsList.size(); i++){
		SpaceConstraint* ctr=this->spaceConstraintsList[i];

		if(ctr->type==CONSTRAINT_STUDENTS_SET_HOME_ROOM){
			ConstraintStudentsSetHomeRoom* crt_constraint=(ConstraintStudentsSetHomeRoom*)ctr;
			if(_initialGroupName == crt_constraint->studentsName)
				crt_constraint->studentsName=finalGroupName;
		}
		else if(ctr->type==CONSTRAINT_STUDENTS_SET_HOME_ROOMS){
			ConstraintStudentsSetHomeRooms* crt_constraint=(ConstraintStudentsSetHomeRooms*)ctr;
			if(_initialGroupName == crt_constraint->studentsName)
				crt_constraint->studentsName=finalGroupName;
		}
		else if(ctr->type==CONSTRAINT_STUDENTS_SET_MAX_BUILDING_CHANGES_PER_DAY){
			ConstraintStudentsSetMaxBuildingChangesPerDay* crt_constraint=(ConstraintStudentsSetMaxBuildingChangesPerDay*)ctr;
			if(_initialGroupName == crt_constraint->studentsName)
				crt_constraint->studentsName=finalGroupName;
		}
		else if(ctr->type==CONSTRAINT_STUDENTS_SET_MAX_BUILDING_CHANGES_PER_WEEK){
			ConstraintStudentsSetMaxBuildingChangesPerWeek* crt_constraint=(ConstraintStudentsSetMaxBuildingChangesPerWeek*)ctr;
			if(_initialGroupName == crt_constraint->studentsName)
				crt_constraint->studentsName=finalGroupName;
		}
		else if(ctr->type==CONSTRAINT_STUDENTS_SET_MIN_GAPS_BETWEEN_BUILDING_CHANGES){
			ConstraintStudentsSetMinGapsBetweenBuildingChanges* crt_constraint=(ConstraintStudentsSetMinGapsBetweenBuildingChanges*)ctr;
			if(_initialGroupName == crt_constraint->studentsName)
				crt_constraint->studentsName=finalGroupName;
		}
	}

	this->internalStructureComputed=false;

	return true;
}

void Rules::sortGroupsAlphabetically(const QString& yearName)
{
	StudentsYear* sty=this->yearsList[this->searchYear(yearName)];
	assert(sty);

	qSort(sty->groupsList.begin(), sty->groupsList.end(), groupsAscending);

	this->internalStructureComputed=false;
}

bool Rules::addSubgroup(const QString& yearName, const QString& groupName, StudentsSubgroup* subgroup)
{
	StudentsYear* sty=this->yearsList.at(this->searchYear(yearName));
	assert(sty);
	StudentsGroup* stg=sty->groupsList.at(this->searchGroup(yearName, groupName));
	assert(stg);


	for(int i=0; i<stg->subgroupsList.size(); i++){
		StudentsSubgroup* sts=stg->subgroupsList[i];
		if(sts->name==subgroup->name)
			return false;
	}
	
	stg->subgroupsList << subgroup; //append

	/*
	foreach(StudentsYear* y, yearsList)
		foreach(StudentsGroup* g, y->groupsList)
			foreach(StudentsSubgroup* s, g->subgroupsList)
				if(s->name==subgroup->name)
					s->numberOfStudents=subgroup->numberOfStudents;*/

	this->internalStructureComputed=false;
	return true;
}

bool Rules::addSubgroupFast(StudentsYear* year, StudentsGroup* group, StudentsSubgroup* subgroup)
{
	Q_UNUSED(year);

	group->subgroupsList << subgroup; //append

	this->internalStructureComputed=false;
	return true;
}

bool Rules::removeSubgroup(const QString& yearName, const QString& groupName, const QString& subgroupName)
{
	this->internalStructureComputed=false;

	StudentsSubgroup* subgroup=(StudentsSubgroup*)searchStudentsSet(subgroupName);
	assert(subgroup!=NULL);
	int nStudents=subgroup->numberOfStudents;

	StudentsYear* sty=this->yearsList.at(this->searchYear(yearName));
	assert(sty);
	StudentsGroup* stg=sty->groupsList.at(this->searchGroup(yearName, groupName));
	assert(stg);

	StudentsSubgroup* sts=NULL;
	for(int i=0; i<stg->subgroupsList.size(); i++){
		sts=stg->subgroupsList[i];
		if(sts->name==subgroupName){
			stg->subgroupsList.removeAt(i);
			break;
		}
	}
	
	assert(sts!=NULL);

	if(this->searchStudentsSet(sts->name)!=NULL){
		//subgroup still exists, in other group
		
		//with the same pointer??? (leak bug fix on 6 Jan 2010 in FET-5.12.1)
		bool foundSamePointer=false;
		foreach(StudentsYear* year, yearsList){
			foreach(StudentsGroup* group, year->groupsList){
				foreach(StudentsSubgroup* subgroup, group->subgroupsList){
					if(subgroup==sts){
						foundSamePointer=true;
						break;
					}
				}
				if(foundSamePointer)
					break;
			}
			if(foundSamePointer)
				break;
		}
		
		if(!foundSamePointer)
			delete sts;
		/////////////////////////////////////////////////////////////////////
		
		return true;
	}

	delete sts;

	for(int i=0; i<this->activitiesList.size(); ){
		Activity* act=this->activitiesList[i];

		bool t=act->removeStudents(*this, subgroupName, nStudents);
		if(t && act->studentsNames.count()==0){
			this->removeActivity(act->id, act->activityGroupId);
			i=0;
			//(You have to be careful, there can be erased more activities here)
		}
		else
			i++;
	}
	
	for(int i=0; i<this->timeConstraintsList.size(); ){
		TimeConstraint* ctr=this->timeConstraintsList[i];

		bool erased=false;
		if(ctr->type==CONSTRAINT_STUDENTS_SET_NOT_AVAILABLE_TIMES){
			ConstraintStudentsSetNotAvailableTimes* crt_constraint=(ConstraintStudentsSetNotAvailableTimes*)ctr;
			if(subgroupName == crt_constraint->students){
				this->removeTimeConstraint(ctr);
				erased=true;
			}
		}
		else if(ctr->type==CONSTRAINT_STUDENTS_SET_MAX_HOURS_DAILY){
			ConstraintStudentsSetMaxHoursDaily* crt_constraint=(ConstraintStudentsSetMaxHoursDaily*)ctr;
			if(subgroupName == crt_constraint->students){
				this->removeTimeConstraint(ctr);
				erased=true;
			}
		}
		else if(ctr->type==CONSTRAINT_STUDENTS_SET_INTERVAL_MAX_DAYS_PER_WEEK){
			ConstraintStudentsSetIntervalMaxDaysPerWeek* crt_constraint=(ConstraintStudentsSetIntervalMaxDaysPerWeek*)ctr;
			if(subgroupName == crt_constraint->students){
				this->removeTimeConstraint(ctr);
				erased=true;
			}
		}
		else if(ctr->type==CONSTRAINT_STUDENTS_SET_MAX_HOURS_CONTINUOUSLY){
			ConstraintStudentsSetMaxHoursContinuously* crt_constraint=(ConstraintStudentsSetMaxHoursContinuously*)ctr;
			if(subgroupName == crt_constraint->students){
				this->removeTimeConstraint(ctr);
				erased=true;
			}
		}
		else if(ctr->type==CONSTRAINT_STUDENTS_SET_ACTIVITY_TAG_MAX_HOURS_CONTINUOUSLY){
			ConstraintStudentsSetActivityTagMaxHoursContinuously* crt_constraint=(ConstraintStudentsSetActivityTagMaxHoursContinuously*)ctr;
			if(subgroupName == crt_constraint->students){
				this->removeTimeConstraint(ctr);
				erased=true;
			}
		}
		else if(ctr->type==CONSTRAINT_STUDENTS_SET_ACTIVITY_TAG_MAX_HOURS_DAILY){
			ConstraintStudentsSetActivityTagMaxHoursDaily* crt_constraint=(ConstraintStudentsSetActivityTagMaxHoursDaily*)ctr;
			if(subgroupName == crt_constraint->students){
				this->removeTimeConstraint(ctr);
				erased=true;
			}
		}
		else if(ctr->type==CONSTRAINT_STUDENTS_SET_MIN_HOURS_DAILY){
			ConstraintStudentsSetMinHoursDaily* crt_constraint=(ConstraintStudentsSetMinHoursDaily*)ctr;
			if(subgroupName == crt_constraint->students){
				this->removeTimeConstraint(ctr);
				erased=true;
			}
		}
		else if(ctr->type==CONSTRAINT_STUDENTS_SET_EARLY_MAX_BEGINNINGS_AT_SECOND_HOUR){
			ConstraintStudentsSetEarlyMaxBeginningsAtSecondHour* crt_constraint=(ConstraintStudentsSetEarlyMaxBeginningsAtSecondHour*)ctr;
			if(subgroupName == crt_constraint->students){
				this->removeTimeConstraint(ctr);
				erased=true;
			}
		}
		else if(ctr->type==CONSTRAINT_STUDENTS_SET_MAX_GAPS_PER_WEEK){
			ConstraintStudentsSetMaxGapsPerWeek* crt_constraint=(ConstraintStudentsSetMaxGapsPerWeek*)ctr;
			if(subgroupName == crt_constraint->students){
				this->removeTimeConstraint(ctr);
				erased=true;
			}
		}
		else if(ctr->type==CONSTRAINT_STUDENTS_SET_MAX_GAPS_PER_DAY){
			ConstraintStudentsSetMaxGapsPerDay* crt_constraint=(ConstraintStudentsSetMaxGapsPerDay*)ctr;
			if(subgroupName == crt_constraint->students){
				this->removeTimeConstraint(ctr);
				erased=true;
			}
		}
		else if(ctr->type==CONSTRAINT_ACTIVITIES_PREFERRED_TIME_SLOTS){
			ConstraintActivitiesPreferredTimeSlots* crt_constraint=(ConstraintActivitiesPreferredTimeSlots*)ctr;
			if(subgroupName == crt_constraint->p_studentsName){
				this->removeTimeConstraint(ctr);
				erased=true;
			}
		}
		else if(ctr->type==CONSTRAINT_ACTIVITIES_PREFERRED_STARTING_TIMES){
			ConstraintActivitiesPreferredStartingTimes* crt_constraint=(ConstraintActivitiesPreferredStartingTimes*)ctr;
			if(subgroupName == crt_constraint->studentsName){
				this->removeTimeConstraint(ctr);
				erased=true;
			}
		}
		else if(ctr->type==CONSTRAINT_ACTIVITIES_END_STUDENTS_DAY){
			ConstraintActivitiesEndStudentsDay* crt_constraint=(ConstraintActivitiesEndStudentsDay*)ctr;
			if(subgroupName == crt_constraint->studentsName){
				this->removeTimeConstraint(ctr);
				erased=true;
			}
		}
		else if(ctr->type==CONSTRAINT_SUBACTIVITIES_PREFERRED_TIME_SLOTS){
			ConstraintSubactivitiesPreferredTimeSlots* crt_constraint=(ConstraintSubactivitiesPreferredTimeSlots*)ctr;
			if(subgroupName == crt_constraint->p_studentsName){
				this->removeTimeConstraint(ctr);
				erased=true;
			}
		}
		else if(ctr->type==CONSTRAINT_SUBACTIVITIES_PREFERRED_STARTING_TIMES){
			ConstraintSubactivitiesPreferredStartingTimes* crt_constraint=(ConstraintSubactivitiesPreferredStartingTimes*)ctr;
			if(subgroupName == crt_constraint->studentsName){
				this->removeTimeConstraint(ctr);
				erased=true;
			}
		}

		if(!erased)
			i++;
	}
	
	for(int i=0; i<this->spaceConstraintsList.size(); ){
		SpaceConstraint* ctr=this->spaceConstraintsList[i];

		bool erased=false;
		if(ctr->type==CONSTRAINT_STUDENTS_SET_HOME_ROOM){
			ConstraintStudentsSetHomeRoom* crt_constraint=(ConstraintStudentsSetHomeRoom*)ctr;
			if(subgroupName == crt_constraint->studentsName){
				this->removeSpaceConstraint(ctr);
				erased=true;
			}
		}
		else if(ctr->type==CONSTRAINT_STUDENTS_SET_HOME_ROOMS){
			ConstraintStudentsSetHomeRooms* crt_constraint=(ConstraintStudentsSetHomeRooms*)ctr;
			if(subgroupName == crt_constraint->studentsName){
				this->removeSpaceConstraint(ctr);
				erased=true;
			}
		}
		else if(ctr->type==CONSTRAINT_STUDENTS_SET_MAX_BUILDING_CHANGES_PER_DAY){
			ConstraintStudentsSetMaxBuildingChangesPerDay* crt_constraint=(ConstraintStudentsSetMaxBuildingChangesPerDay*)ctr;
			if(subgroupName == crt_constraint->studentsName){
				this->removeSpaceConstraint(ctr);
				erased=true;
			}
		}
		else if(ctr->type==CONSTRAINT_STUDENTS_SET_MAX_BUILDING_CHANGES_PER_WEEK){
			ConstraintStudentsSetMaxBuildingChangesPerWeek* crt_constraint=(ConstraintStudentsSetMaxBuildingChangesPerWeek*)ctr;
			if(subgroupName == crt_constraint->studentsName){
				this->removeSpaceConstraint(ctr);
				erased=true;
			}
		}
		else if(ctr->type==CONSTRAINT_STUDENTS_SET_MIN_GAPS_BETWEEN_BUILDING_CHANGES){
			ConstraintStudentsSetMinGapsBetweenBuildingChanges* crt_constraint=(ConstraintStudentsSetMinGapsBetweenBuildingChanges*)ctr;
			if(subgroupName == crt_constraint->studentsName){
				this->removeSpaceConstraint(ctr);
				erased=true;
			}
		}

		if(!erased)
			i++;
	}

	return true;
}

int Rules::searchSubgroup(const QString& yearName, const QString& groupName, const QString& subgroupName)
{
	StudentsYear* sty=this->yearsList.at(this->searchYear(yearName));
	assert(sty);
	StudentsGroup* stg=sty->groupsList.at(this->searchGroup(yearName, groupName));
	assert(stg);
	
	for(int i=0; i<stg->subgroupsList.size(); i++)
		if(stg->subgroupsList[i]->name==subgroupName)
			return i;
	
	return -1;
}

int Rules::searchAugmentedSubgroup(const QString& yearName, const QString& groupName, const QString& subgroupName)
{
	StudentsYear* sty=this->augmentedYearsList.at(this->searchAugmentedYear(yearName));
	assert(sty);
	StudentsGroup* stg=sty->groupsList.at(this->searchAugmentedGroup(yearName, groupName));
	assert(stg);
	
	for(int i=0; i<stg->subgroupsList.size(); i++)
		if(stg->subgroupsList[i]->name==subgroupName)
			return i;
	
	return -1;
}

bool Rules::modifySubgroup(const QString& yearName, const QString& groupName, const QString& initialSubgroupName, const QString& finalSubgroupName, int finalNumberOfStudents)
{
	StudentsSet* _initialSet=searchStudentsSet(initialSubgroupName);
	assert(_initialSet!=NULL);
	int _initialNumberOfStudents=_initialSet->numberOfStudents;

	QString _initialSubgroupName=initialSubgroupName;

	assert(searchSubgroup(yearName, groupName, _initialSubgroupName)>=0);
	StudentsSet* _ss=searchStudentsSet(finalSubgroupName);
	assert(_ss==NULL || _initialSubgroupName==finalSubgroupName);

	StudentsYear* sty=this->yearsList.at(this->searchYear(yearName));
	assert(sty);
	StudentsGroup* stg=sty->groupsList.at(this->searchGroup(yearName, groupName));
	assert(stg);

	StudentsSubgroup* sts=NULL;
	for(int i=0; i<stg->subgroupsList.size(); i++){
		sts=stg->subgroupsList[i];

		if(sts->name==_initialSubgroupName){
			sts->name=finalSubgroupName;
			sts->numberOfStudents=finalNumberOfStudents;
			break;
		}
	}
	assert(sts);

	if(_ss!=NULL){ //In case it only changes the number of students, make the same number of students in all subgroups with this name
		assert(_initialSubgroupName==finalSubgroupName);
		foreach(StudentsYear* year, yearsList)
			foreach(StudentsGroup* group, year->groupsList)
				foreach(StudentsSubgroup* subgroup, group->subgroupsList)
					if(subgroup->name==finalSubgroupName)
						subgroup->numberOfStudents=finalNumberOfStudents;
	}

	//TODO: improve this part
	for(int i=0; i<this->activitiesList.size(); i++)
		this->activitiesList[i]->renameStudents(*this, _initialSubgroupName, finalSubgroupName, _initialNumberOfStudents, finalNumberOfStudents);

	for(int i=0; i<this->timeConstraintsList.size(); i++){
		TimeConstraint* ctr=this->timeConstraintsList[i];

		if(ctr->type==CONSTRAINT_STUDENTS_SET_NOT_AVAILABLE_TIMES){
			ConstraintStudentsSetNotAvailableTimes* crt_constraint=(ConstraintStudentsSetNotAvailableTimes*)ctr;
			if(_initialSubgroupName == crt_constraint->students)
				crt_constraint->students=finalSubgroupName;
		}
		else if(ctr->type==CONSTRAINT_STUDENTS_SET_MAX_HOURS_DAILY){
			ConstraintStudentsSetMaxHoursDaily* crt_constraint=(ConstraintStudentsSetMaxHoursDaily*)ctr;
			if(_initialSubgroupName == crt_constraint->students)
				crt_constraint->students=finalSubgroupName;
		}
		else if(ctr->type==CONSTRAINT_STUDENTS_SET_INTERVAL_MAX_DAYS_PER_WEEK){
			ConstraintStudentsSetIntervalMaxDaysPerWeek* crt_constraint=(ConstraintStudentsSetIntervalMaxDaysPerWeek*)ctr;
			if(_initialSubgroupName == crt_constraint->students)
				crt_constraint->students=finalSubgroupName;
		}
		else if(ctr->type==CONSTRAINT_STUDENTS_SET_MAX_HOURS_CONTINUOUSLY){
			ConstraintStudentsSetMaxHoursContinuously* crt_constraint=(ConstraintStudentsSetMaxHoursContinuously*)ctr;
			if(_initialSubgroupName == crt_constraint->students)
				crt_constraint->students=finalSubgroupName;
		}
		else if(ctr->type==CONSTRAINT_STUDENTS_SET_ACTIVITY_TAG_MAX_HOURS_CONTINUOUSLY){
			ConstraintStudentsSetActivityTagMaxHoursContinuously* crt_constraint=(ConstraintStudentsSetActivityTagMaxHoursContinuously*)ctr;
			if(_initialSubgroupName == crt_constraint->students)
				crt_constraint->students=finalSubgroupName;
		}
		else if(ctr->type==CONSTRAINT_STUDENTS_SET_ACTIVITY_TAG_MAX_HOURS_DAILY){
			ConstraintStudentsSetActivityTagMaxHoursDaily* crt_constraint=(ConstraintStudentsSetActivityTagMaxHoursDaily*)ctr;
			if(_initialSubgroupName == crt_constraint->students)
				crt_constraint->students=finalSubgroupName;
		}
		else if(ctr->type==CONSTRAINT_STUDENTS_SET_MIN_HOURS_DAILY){
			ConstraintStudentsSetMinHoursDaily* crt_constraint=(ConstraintStudentsSetMinHoursDaily*)ctr;
			if(_initialSubgroupName == crt_constraint->students)
				crt_constraint->students=finalSubgroupName;
		}
		else if(ctr->type==CONSTRAINT_STUDENTS_SET_EARLY_MAX_BEGINNINGS_AT_SECOND_HOUR){
			ConstraintStudentsSetEarlyMaxBeginningsAtSecondHour* crt_constraint=(ConstraintStudentsSetEarlyMaxBeginningsAtSecondHour*)ctr;
			if(_initialSubgroupName == crt_constraint->students)
				crt_constraint->students=finalSubgroupName;
		}
		else if(ctr->type==CONSTRAINT_STUDENTS_SET_MAX_GAPS_PER_WEEK){
			ConstraintStudentsSetMaxGapsPerWeek* crt_constraint=(ConstraintStudentsSetMaxGapsPerWeek*)ctr;
			if(_initialSubgroupName == crt_constraint->students)
				crt_constraint->students=finalSubgroupName;
		}
		else if(ctr->type==CONSTRAINT_STUDENTS_SET_MAX_GAPS_PER_DAY){
			ConstraintStudentsSetMaxGapsPerDay* crt_constraint=(ConstraintStudentsSetMaxGapsPerDay*)ctr;
			if(_initialSubgroupName == crt_constraint->students)
				crt_constraint->students=finalSubgroupName;
		}
		else if(ctr->type==CONSTRAINT_ACTIVITIES_PREFERRED_TIME_SLOTS){
			ConstraintActivitiesPreferredTimeSlots* crt_constraint=(ConstraintActivitiesPreferredTimeSlots*)ctr;
			if(_initialSubgroupName == crt_constraint->p_studentsName)
				crt_constraint->p_studentsName=finalSubgroupName;
		}
		else if(ctr->type==CONSTRAINT_ACTIVITIES_PREFERRED_STARTING_TIMES){
			ConstraintActivitiesPreferredStartingTimes* crt_constraint=(ConstraintActivitiesPreferredStartingTimes*)ctr;
			if(_initialSubgroupName == crt_constraint->studentsName)
				crt_constraint->studentsName=finalSubgroupName;
		}
		else if(ctr->type==CONSTRAINT_ACTIVITIES_END_STUDENTS_DAY){
			ConstraintActivitiesEndStudentsDay* crt_constraint=(ConstraintActivitiesEndStudentsDay*)ctr;
			if(_initialSubgroupName == crt_constraint->studentsName)
				crt_constraint->studentsName=finalSubgroupName;
		}
		else if(ctr->type==CONSTRAINT_SUBACTIVITIES_PREFERRED_TIME_SLOTS){
			ConstraintSubactivitiesPreferredTimeSlots* crt_constraint=(ConstraintSubactivitiesPreferredTimeSlots*)ctr;
			if(_initialSubgroupName == crt_constraint->p_studentsName)
				crt_constraint->p_studentsName=finalSubgroupName;
		}
		else if(ctr->type==CONSTRAINT_SUBACTIVITIES_PREFERRED_STARTING_TIMES){
			ConstraintSubactivitiesPreferredStartingTimes* crt_constraint=(ConstraintSubactivitiesPreferredStartingTimes*)ctr;
			if(_initialSubgroupName == crt_constraint->studentsName)
				crt_constraint->studentsName=finalSubgroupName;
		}
	}

	for(int i=0; i<this->spaceConstraintsList.size(); i++){
		SpaceConstraint* ctr=this->spaceConstraintsList[i];

		if(ctr->type==CONSTRAINT_STUDENTS_SET_HOME_ROOM){
			ConstraintStudentsSetHomeRoom* crt_constraint=(ConstraintStudentsSetHomeRoom*)ctr;
			if(_initialSubgroupName == crt_constraint->studentsName)
				crt_constraint->studentsName=finalSubgroupName;
		}
		else if(ctr->type==CONSTRAINT_STUDENTS_SET_HOME_ROOMS){
			ConstraintStudentsSetHomeRooms* crt_constraint=(ConstraintStudentsSetHomeRooms*)ctr;
			if(_initialSubgroupName == crt_constraint->studentsName)
				crt_constraint->studentsName=finalSubgroupName;
		}
		else if(ctr->type==CONSTRAINT_STUDENTS_SET_MAX_BUILDING_CHANGES_PER_DAY){
			ConstraintStudentsSetMaxBuildingChangesPerDay* crt_constraint=(ConstraintStudentsSetMaxBuildingChangesPerDay*)ctr;
			if(_initialSubgroupName == crt_constraint->studentsName)
				crt_constraint->studentsName=finalSubgroupName;
		}
		else if(ctr->type==CONSTRAINT_STUDENTS_SET_MAX_BUILDING_CHANGES_PER_WEEK){
			ConstraintStudentsSetMaxBuildingChangesPerWeek* crt_constraint=(ConstraintStudentsSetMaxBuildingChangesPerWeek*)ctr;
			if(_initialSubgroupName == crt_constraint->studentsName)
				crt_constraint->studentsName=finalSubgroupName;
		}
		else if(ctr->type==CONSTRAINT_STUDENTS_SET_MIN_GAPS_BETWEEN_BUILDING_CHANGES){
			ConstraintStudentsSetMinGapsBetweenBuildingChanges* crt_constraint=(ConstraintStudentsSetMinGapsBetweenBuildingChanges*)ctr;
			if(_initialSubgroupName == crt_constraint->studentsName)
				crt_constraint->studentsName=finalSubgroupName;
		}
	}

	this->internalStructureComputed=false;

	return true;
}

void Rules::sortSubgroupsAlphabetically(const QString& yearName, const QString& groupName)
{
	StudentsYear* sty=this->yearsList.at(this->searchYear(yearName));
	assert(sty);
	StudentsGroup* stg=sty->groupsList.at(this->searchGroup(yearName, groupName));
	assert(stg);

	qSort(stg->subgroupsList.begin(), stg->subgroupsList.end(), subgroupsAscending);
	
	this->internalStructureComputed=false;
}

bool Rules::addSimpleActivity(
	int _id,
	int _activityGroupId,
	const QStringList& _teachersNames,
	const QString& _subjectName,
	const QStringList& _activityTagsNames,
	const QStringList& _studentsNames,
	int _duration, /*duration, in hours*/
	int _totalDuration,
	//int _parity, /*parity: PARITY_WEEKLY or PARITY_FORTNIGHTLY*/
	bool _active,
	//int _preferredDay,
	//int _preferredHour,
	bool _computeNTotalStudents,
	int _nTotalStudents)
{
	//assert(_parity==PARITY_WEEKLY || _parity==PARITY_FORTNIGHTLY); //weekly or fortnightly

	Activity *act=new Activity(*this, _id, _activityGroupId, _teachersNames, _subjectName, _activityTagsNames,
		_studentsNames, _duration, _totalDuration, /*_parity,*/ _active, _computeNTotalStudents, _nTotalStudents);

	this->activitiesList << act; //append

	/*if(_preferredDay>=0 || _preferredHour>=0){
		TimeConstraint *ctr=new ConstraintActivityPreferredTime(0.0, _id, _preferredDay, _preferredHour);
		bool tmp=this->addTimeConstraint(ctr);
		assert(tmp);
	}*/

	this->internalStructureComputed=false;

	return true;
}

bool Rules::addSimpleActivityRulesFast(
	int _id,
	int _activityGroupId,
	const QStringList& _teachersNames,
	const QString& _subjectName,
	const QStringList& _activityTagsNames,
	const QStringList& _studentsNames,
	int _duration, /*duration, in hours*/
	int _totalDuration,
	//int _parity, /*parity: PARITY_WEEKLY or PARITY_FORTNIGHTLY*/
	bool _active,
	//int _preferredDay,
	//int _preferredHour,
	bool _computeNTotalStudents,
	int _nTotalStudents,
	int _computedNumberOfStudents)
{
	//assert(_parity==PARITY_WEEKLY || _parity==PARITY_FORTNIGHTLY); //weekly or fortnightly

	Activity *act=new Activity(*this, _id, _activityGroupId, _teachersNames, _subjectName, _activityTagsNames,
		_studentsNames, _duration, _totalDuration, /*_parity,*/ _active, _computeNTotalStudents, _nTotalStudents, _computedNumberOfStudents);

	this->activitiesList << act; //append

	/*if(_preferredDay>=0 || _preferredHour>=0){
		TimeConstraint *ctr=new ConstraintActivityPreferredTime(0.0, _id, _preferredDay, _preferredHour);
		bool tmp=this->addTimeConstraint(ctr);
		assert(tmp);
	}*/

	this->internalStructureComputed=false;

	return true;
}

bool Rules::addSplitActivity(
	int _firstActivityId,
	int _activityGroupId,
	const QStringList& _teachersNames,
	const QString& _subjectName,
	const QStringList& _activityTagsNames,
	const QStringList& _studentsNames,
	int _nSplits,
	int _totalDuration,
	int _durations[],
	//int _parities[],
	bool _active[],
	int _minDayDistance,
	double _weightPercentage,
	bool _consecutiveIfSameDay,
	//int _preferredDays[],
	//int _preferredHours[],
	bool _computeNTotalStudents,
	int _nTotalStudents)
{
	assert(_firstActivityId==_activityGroupId);

	QList<int> acts;
	//int acts[MAX_CONSTRAINT_MIN_DAYS_BETWEEN_ACTIVITIES];
	//assert(_nSplits<=MAX_CONSTRAINT_MIN_DAYS_BETWEEN_ACTIVITIES);

	//for(int i=0; i<_nSplits; i++)
	//	assert(_parities[i]==PARITY_WEEKLY || _parities[i]==PARITY_FORTNIGHTLY); //weekly or fortnightly

	acts.clear();
	for(int i=0; i<_nSplits; i++){
		Activity *act;
		if(i==0)
			act=new Activity(*this, _firstActivityId+i, _activityGroupId,
				_teachersNames, _subjectName, _activityTagsNames, _studentsNames,
				_durations[i], _totalDuration, /*_parities[i],*/ _active[i], _computeNTotalStudents, _nTotalStudents);
		else
			act=new Activity(*this, _firstActivityId+i, _activityGroupId,
				_teachersNames, _subjectName, _activityTagsNames, _studentsNames,
				_durations[i], _totalDuration, /*_parities[i],*/ _active[i], _computeNTotalStudents, _nTotalStudents);

		this->activitiesList << act; //append

		acts.append(_firstActivityId+i);
		//acts[i]=_firstActivityId+i;

		/*if(_preferredDays[i]>=0 || _preferredHours[i]>=0){
			TimeConstraint *constr=new ConstraintActivityPreferredTime(0.0, act->id, _preferredDays[i], _preferredHours[i]); //non-compulsory constraint
			bool tmp = this->addTimeConstraint(constr);
			assert(tmp);
		}*/
	}

	if(_minDayDistance>0){
		//TimeConstraint *constr=new ConstraintMinDaysBetweenActivities(1.0, true, _nSplits, acts, _minDayDistance); //compulsory constraint
		TimeConstraint *constr=new ConstraintMinDaysBetweenActivities(_weightPercentage, _consecutiveIfSameDay, _nSplits, acts, _minDayDistance);
		bool tmp=this->addTimeConstraint(constr);
		assert(tmp);
	}

	this->internalStructureComputed=false;

	return true;
}

void Rules::removeActivity(int _id)
{
	bool recomputeTime=false, recomputeSpace=false;

	for(int i=0; i<this->activitiesList.size(); i++){
		Activity* act=this->activitiesList[i];
		if(_id==act->id){
			//removing ConstraintActivityPreferredTime-s referring to this activity
			for(int j=0; j<this->timeConstraintsList.size(); ){
				TimeConstraint* ctr=this->timeConstraintsList[j];
				if(ctr->type==CONSTRAINT_ACTIVITY_PREFERRED_STARTING_TIME){
					ConstraintActivityPreferredStartingTime *apt=(ConstraintActivityPreferredStartingTime*)ctr;
					if(apt->activityId==act->id){
						//cout<<"Removing constraint "<<qPrintable(apt->getDescription(*this))<<endl;
						this->removeTimeConstraint(ctr);
						recomputeTime=true;
					}
					else
						j++;
				}
				else
					j++;
			}
			//removing ConstraintTwoActivitiesConsecutive-s referring to this activity
			for(int j=0; j<this->timeConstraintsList.size(); ){
				TimeConstraint* ctr=this->timeConstraintsList[j];
				if(ctr->type==CONSTRAINT_TWO_ACTIVITIES_CONSECUTIVE){
					ConstraintTwoActivitiesConsecutive *apt=(ConstraintTwoActivitiesConsecutive*)ctr;
					if(apt->firstActivityId==act->id){
						//cout<<"Removing constraint "<<qPrintable(apt->getDescription(*this))<<endl;
						this->removeTimeConstraint(ctr);
					}
					else if(apt->secondActivityId==act->id){
						//cout<<"Removing constraint "<<qPrintable(apt->getDescription(*this))<<endl;
						this->removeTimeConstraint(ctr);
					}
					else
						j++;
				}
				else
					j++;
			}
			//removing ConstraintTwoActivitiesGrouped-s referring to this activity
			for(int j=0; j<this->timeConstraintsList.size(); ){
				TimeConstraint* ctr=this->timeConstraintsList[j];
				if(ctr->type==CONSTRAINT_TWO_ACTIVITIES_GROUPED){
					ConstraintTwoActivitiesGrouped *apt=(ConstraintTwoActivitiesGrouped*)ctr;
					if(apt->firstActivityId==act->id){
						//cout<<"Removing constraint "<<qPrintable(apt->getDescription(*this))<<endl;
						this->removeTimeConstraint(ctr);
					}
					else if(apt->secondActivityId==act->id){
						//cout<<"Removing constraint "<<qPrintable(apt->getDescription(*this))<<endl;
						this->removeTimeConstraint(ctr);
					}
					else
						j++;
				}
				else
					j++;
			}
			//removing ConstraintThreeActivitiesGrouped-s referring to this activity
			for(int j=0; j<this->timeConstraintsList.size(); ){
				TimeConstraint* ctr=this->timeConstraintsList[j];
				if(ctr->type==CONSTRAINT_THREE_ACTIVITIES_GROUPED){
					ConstraintThreeActivitiesGrouped *apt=(ConstraintThreeActivitiesGrouped*)ctr;
					if(apt->firstActivityId==act->id){
						//cout<<"Removing constraint "<<qPrintable(apt->getDescription(*this))<<endl;
						this->removeTimeConstraint(ctr);
					}
					else if(apt->secondActivityId==act->id){
						//cout<<"Removing constraint "<<qPrintable(apt->getDescription(*this))<<endl;
						this->removeTimeConstraint(ctr);
					}
					else if(apt->thirdActivityId==act->id){
						//cout<<"Removing constraint "<<qPrintable(apt->getDescription(*this))<<endl;
						this->removeTimeConstraint(ctr);
					}
					else
						j++;
				}
				else
					j++;
			}
			//removing ConstraintTwoActivitiesOrdered-s referring to this activity
			for(int j=0; j<this->timeConstraintsList.size(); ){
				TimeConstraint* ctr=this->timeConstraintsList[j];
				if(ctr->type==CONSTRAINT_TWO_ACTIVITIES_ORDERED){
					ConstraintTwoActivitiesOrdered *apt=(ConstraintTwoActivitiesOrdered*)ctr;
					if(apt->firstActivityId==act->id){
						//cout<<"Removing constraint "<<qPrintable(apt->getDescription(*this))<<endl;
						this->removeTimeConstraint(ctr);
					}
					else if(apt->secondActivityId==act->id){
						//cout<<"Removing constraint "<<qPrintable(apt->getDescription(*this))<<endl;
						this->removeTimeConstraint(ctr);
					}
					else
						j++;
				}
				else
					j++;
			}
			//removing ConstraintActivityPreferredTimes-s referring to this activity
			for(int j=0; j<this->timeConstraintsList.size(); ){
				TimeConstraint* ctr=this->timeConstraintsList[j];
				if(ctr->type==CONSTRAINT_ACTIVITY_PREFERRED_TIME_SLOTS){
					ConstraintActivityPreferredTimeSlots *apt=(ConstraintActivityPreferredTimeSlots*)ctr;
					if(apt->p_activityId==act->id){
						//cout<<"Removing constraint "<<qPrintable(apt->getDescription(*this))<<endl;
						this->removeTimeConstraint(ctr);
					}
					else
						j++;
				}
				else
					j++;
			}
			//removing ConstraintActivityPreferredStartingTimes-s referring to this activity
			for(int j=0; j<this->timeConstraintsList.size(); ){
				TimeConstraint* ctr=this->timeConstraintsList[j];
				if(ctr->type==CONSTRAINT_ACTIVITY_PREFERRED_STARTING_TIMES){
					ConstraintActivityPreferredStartingTimes *apt=(ConstraintActivityPreferredStartingTimes*)ctr;
					if(apt->activityId==act->id){
						//cout<<"Removing constraint "<<qPrintable(apt->getDescription(*this))<<endl;
						this->removeTimeConstraint(ctr);
					}
					else
						j++;
				}
				else
					j++;
			}
			//removing ConstraintActivityEndsStudentsDay-s referring to this activity
			for(int j=0; j<this->timeConstraintsList.size(); ){
				TimeConstraint* ctr=this->timeConstraintsList[j];
				if(ctr->type==CONSTRAINT_ACTIVITY_ENDS_STUDENTS_DAY){
					ConstraintActivityEndsStudentsDay *apt=(ConstraintActivityEndsStudentsDay*)ctr;
					if(apt->activityId==act->id){
						//cout<<"Removing constraint "<<qPrintable(apt->getDescription(*this))<<endl;
						this->removeTimeConstraint(ctr);
					}
					else
						j++;
				}
				else
					j++;
			}

			//removing ConstraintActivityPreferredRoom-s referring to this activity
			for(int j=0; j<this->spaceConstraintsList.size(); ){
				SpaceConstraint* ctr=this->spaceConstraintsList[j];
				if(ctr->type==CONSTRAINT_ACTIVITY_PREFERRED_ROOM){
					if(((ConstraintActivityPreferredRoom*)ctr)->activityId==act->id){
						this->removeSpaceConstraint(ctr);
						recomputeSpace=true;
					}
					else
						j++;
				}
				else
					j++;
			}
			//removing ConstraintActivityPreferredRooms-s referring to this activity
			for(int j=0; j<this->spaceConstraintsList.size(); ){
				SpaceConstraint* ctr=this->spaceConstraintsList[j];
				if(ctr->type==CONSTRAINT_ACTIVITY_PREFERRED_ROOMS){
					if(((ConstraintActivityPreferredRooms*)ctr)->activityId==act->id)
						this->removeSpaceConstraint(ctr);
					else
						j++;
				}
				else
					j++;
			}

			//remove the activity
			delete this->activitiesList[i];
			this->activitiesList.removeAt(i); 
			break;
		}
	}

	for(int i=0; i<this->timeConstraintsList.size(); ){
		TimeConstraint* ctr=this->timeConstraintsList[i];
		if(ctr->type==CONSTRAINT_MIN_DAYS_BETWEEN_ACTIVITIES){
			((ConstraintMinDaysBetweenActivities*)ctr)->removeUseless(*this);
			if(((ConstraintMinDaysBetweenActivities*)ctr)->n_activities<2)
				this->removeTimeConstraint(ctr);
			else
				i++;
		}
		else
			i++;
	}

	for(int i=0; i<this->timeConstraintsList.size(); ){
		TimeConstraint* ctr=this->timeConstraintsList[i];
		if(ctr->type==CONSTRAINT_MAX_DAYS_BETWEEN_ACTIVITIES){
			((ConstraintMaxDaysBetweenActivities*)ctr)->removeUseless(*this);
			if(((ConstraintMaxDaysBetweenActivities*)ctr)->n_activities<2)
				this->removeTimeConstraint(ctr);
			else
				i++;
		}
		else
			i++;
	}

	for(int i=0; i<this->timeConstraintsList.size(); ){
		TimeConstraint* ctr=this->timeConstraintsList[i];
		if(ctr->type==CONSTRAINT_MIN_GAPS_BETWEEN_ACTIVITIES){
			((ConstraintMinGapsBetweenActivities*)ctr)->removeUseless(*this);
			if(((ConstraintMinGapsBetweenActivities*)ctr)->n_activities<2)
				this->removeTimeConstraint(ctr);
			else
				i++;
		}
		else
			i++;
	}

	for(int i=0; i<this->timeConstraintsList.size(); ){
		TimeConstraint* ctr=this->timeConstraintsList[i];
		if(ctr->type==CONSTRAINT_ACTIVITIES_SAME_STARTING_TIME){
			((ConstraintActivitiesSameStartingTime*)ctr)->removeUseless(*this);
			if(((ConstraintActivitiesSameStartingTime*)ctr)->n_activities<2)
				this->removeTimeConstraint(ctr);
			else
				i++;
		}
		else
			i++;
	}

	for(int i=0; i<this->timeConstraintsList.size(); ){
		TimeConstraint* ctr=this->timeConstraintsList[i];
		if(ctr->type==CONSTRAINT_ACTIVITIES_SAME_STARTING_HOUR){
			((ConstraintActivitiesSameStartingHour*)ctr)->removeUseless(*this);
			if(((ConstraintActivitiesSameStartingHour*)ctr)->n_activities<2)
				this->removeTimeConstraint(ctr);
			else
				i++;
		}
		else
			i++;
	}

	for(int i=0; i<this->timeConstraintsList.size(); ){
		TimeConstraint* ctr=this->timeConstraintsList[i];
		if(ctr->type==CONSTRAINT_ACTIVITIES_SAME_STARTING_DAY){
			((ConstraintActivitiesSameStartingDay*)ctr)->removeUseless(*this);
			if(((ConstraintActivitiesSameStartingDay*)ctr)->n_activities<2)
				this->removeTimeConstraint(ctr);
			else
				i++;
		}
		else
			i++;
	}

	for(int i=0; i<this->timeConstraintsList.size(); ){
		TimeConstraint* ctr=this->timeConstraintsList[i];
		if(ctr->type==CONSTRAINT_ACTIVITIES_NOT_OVERLAPPING){
			((ConstraintActivitiesNotOverlapping*)ctr)->removeUseless(*this);
			if(((ConstraintActivitiesNotOverlapping*)ctr)->n_activities<2)
				this->removeTimeConstraint(ctr);
			else
				i++;
		}
		else
			i++;
	}
	
	if(recomputeTime){
		LockUnlock::computeLockedUnlockedActivitiesOnlyTime();
	}
	if(recomputeSpace){
		LockUnlock::computeLockedUnlockedActivitiesOnlySpace();
	}
	if(recomputeTime || recomputeSpace){
		LockUnlock::increaseCommunicationSpinBox();
	}

	this->internalStructureComputed=false;
}

void Rules::removeActivity(int _id, int _activityGroupId)
{
	bool recomputeTime=false;
	bool recomputeSpace=false;
	
	for(int i=0; i<this->activitiesList.size(); ){
		Activity* act=this->activitiesList[i];

		if(_id==act->id || (_activityGroupId>0 && _activityGroupId==act->activityGroupId)){

			//removing ConstraintActivityPreferredTime-s referring to this activity
			for(int j=0; j<this->timeConstraintsList.size(); ){
				TimeConstraint* ctr=this->timeConstraintsList[j];
				if(ctr->type==CONSTRAINT_ACTIVITY_PREFERRED_STARTING_TIME){
					ConstraintActivityPreferredStartingTime *apt=(ConstraintActivityPreferredStartingTime*)ctr;
					if(apt->activityId==act->id){
						//cout<<"Removing constraint "<<qPrintable(apt->getDescription(*this))<<endl;
						this->removeTimeConstraint(ctr);
						recomputeTime=true;
					}
					else
						j++;
				}
				else
					j++;
			}
			//removing ConstraintTwoActivitiesConsecutive-s referring to this activity
			for(int j=0; j<this->timeConstraintsList.size(); ){
				TimeConstraint* ctr=this->timeConstraintsList[j];
				if(ctr->type==CONSTRAINT_TWO_ACTIVITIES_CONSECUTIVE){
					ConstraintTwoActivitiesConsecutive *apt=(ConstraintTwoActivitiesConsecutive*)ctr;
					if(apt->firstActivityId==act->id){
						//cout<<"Removing constraint "<<qPrintable(apt->getDescription(*this))<<endl;
						this->removeTimeConstraint(ctr);
					}
					else if(apt->secondActivityId==act->id){
						//cout<<"Removing constraint "<<qPrintable(apt->getDescription(*this))<<endl;
						this->removeTimeConstraint(ctr);
					}
					else
						j++;
				}
				else
					j++;
			}
			//removing ConstraintTwoActivitiesGrouped-s referring to this activity
			for(int j=0; j<this->timeConstraintsList.size(); ){
				TimeConstraint* ctr=this->timeConstraintsList[j];
				if(ctr->type==CONSTRAINT_TWO_ACTIVITIES_GROUPED){
					ConstraintTwoActivitiesGrouped *apt=(ConstraintTwoActivitiesGrouped*)ctr;
					if(apt->firstActivityId==act->id){
						//cout<<"Removing constraint "<<qPrintable(apt->getDescription(*this))<<endl;
						this->removeTimeConstraint(ctr);
					}
					else if(apt->secondActivityId==act->id){
						//cout<<"Removing constraint "<<qPrintable(apt->getDescription(*this))<<endl;
						this->removeTimeConstraint(ctr);
					}
					else
						j++;
				}
				else
					j++;
			}
			//removing ConstraintThreeActivitiesGrouped-s referring to this activity
			for(int j=0; j<this->timeConstraintsList.size(); ){
				TimeConstraint* ctr=this->timeConstraintsList[j];
				if(ctr->type==CONSTRAINT_THREE_ACTIVITIES_GROUPED){
					ConstraintThreeActivitiesGrouped *apt=(ConstraintThreeActivitiesGrouped*)ctr;
					if(apt->firstActivityId==act->id){
						//cout<<"Removing constraint "<<qPrintable(apt->getDescription(*this))<<endl;
						this->removeTimeConstraint(ctr);
					}
					else if(apt->secondActivityId==act->id){
						//cout<<"Removing constraint "<<qPrintable(apt->getDescription(*this))<<endl;
						this->removeTimeConstraint(ctr);
					}
					else if(apt->thirdActivityId==act->id){
						//cout<<"Removing constraint "<<qPrintable(apt->getDescription(*this))<<endl;
						this->removeTimeConstraint(ctr);
					}
					else
						j++;
				}
				else
					j++;
			}
			//removing ConstraintTwoActivitiesOrdered-s referring to this activity
			for(int j=0; j<this->timeConstraintsList.size(); ){
				TimeConstraint* ctr=this->timeConstraintsList[j];
				if(ctr->type==CONSTRAINT_TWO_ACTIVITIES_ORDERED){
					ConstraintTwoActivitiesOrdered *apt=(ConstraintTwoActivitiesOrdered*)ctr;
					if(apt->firstActivityId==act->id){
						//cout<<"Removing constraint "<<qPrintable(apt->getDescription(*this))<<endl;
						this->removeTimeConstraint(ctr);
					}
					else if(apt->secondActivityId==act->id){
						//cout<<"Removing constraint "<<qPrintable(apt->getDescription(*this))<<endl;
						this->removeTimeConstraint(ctr);
					}
					else
						j++;
				}
				else
					j++;
			}
			//removing ConstraintActivityPreferredTimes-s referring to this activity
			for(int j=0; j<this->timeConstraintsList.size(); ){
				TimeConstraint* ctr=this->timeConstraintsList[j];
				if(ctr->type==CONSTRAINT_ACTIVITY_PREFERRED_TIME_SLOTS){
					ConstraintActivityPreferredTimeSlots *apt=(ConstraintActivityPreferredTimeSlots*)ctr;
					if(apt->p_activityId==act->id){
						//cout<<"Removing constraint "<<qPrintable(apt->getDescription(*this))<<endl;
						this->removeTimeConstraint(ctr);
					}
					else
						j++;
				}
				else
					j++;
			}
			//removing ConstraintActivityPreferredStartingTimes-s referring to this activity
			for(int j=0; j<this->timeConstraintsList.size(); ){
				TimeConstraint* ctr=this->timeConstraintsList[j];
				if(ctr->type==CONSTRAINT_ACTIVITY_PREFERRED_STARTING_TIMES){
					ConstraintActivityPreferredStartingTimes *apt=(ConstraintActivityPreferredStartingTimes*)ctr;
					if(apt->activityId==act->id){
						//cout<<"Removing constraint "<<qPrintable(apt->getDescription(*this))<<endl;
						this->removeTimeConstraint(ctr);
					}
					else
						j++;
				}
				else
					j++;
			}
			//removing ConstraintActivityEndsStudentsDay-s referring to this activity
			for(int j=0; j<this->timeConstraintsList.size(); ){
				TimeConstraint* ctr=this->timeConstraintsList[j];
				if(ctr->type==CONSTRAINT_ACTIVITY_ENDS_STUDENTS_DAY){
					ConstraintActivityEndsStudentsDay *apt=(ConstraintActivityEndsStudentsDay*)ctr;
					if(apt->activityId==act->id){
						//cout<<"Removing constraint "<<qPrintable(apt->getDescription(*this))<<endl;
						this->removeTimeConstraint(ctr);
					}
					else
						j++;
				}
				else
					j++;
			}

			//removing ConstraintActivityPreferredRoom-s referring to this activity
			for(int j=0; j<this->spaceConstraintsList.size(); ){
				SpaceConstraint* ctr=this->spaceConstraintsList[j];
				if(ctr->type==CONSTRAINT_ACTIVITY_PREFERRED_ROOM){
					if(((ConstraintActivityPreferredRoom*)ctr)->activityId==act->id){
						this->removeSpaceConstraint(ctr);
						recomputeSpace=true;
					}
					else
						j++;
				}
				else
					j++;
			}
			//removing ConstraintActivityPreferredRooms-s referring to this activity
			for(int j=0; j<this->spaceConstraintsList.size(); ){
				SpaceConstraint* ctr=this->spaceConstraintsList[j];
				if(ctr->type==CONSTRAINT_ACTIVITY_PREFERRED_ROOMS){
					if(((ConstraintActivityPreferredRooms*)ctr)->activityId==act->id)
						this->removeSpaceConstraint(ctr);
					else
						j++;
				}
				else
					j++;
			}

			delete this->activitiesList[i];
			this->activitiesList.removeAt(i); //if this is the last activity, then we will make one more comparison above
		}
		else
			i++;
	}

	for(int i=0; i<this->timeConstraintsList.size(); ){
		TimeConstraint* ctr=this->timeConstraintsList[i];
		if(ctr->type==CONSTRAINT_MIN_DAYS_BETWEEN_ACTIVITIES){
			((ConstraintMinDaysBetweenActivities*)ctr)->removeUseless(*this);
			if(((ConstraintMinDaysBetweenActivities*)ctr)->n_activities<2)
				this->removeTimeConstraint(ctr);
			else
				i++;
		}
		else
			i++;
	}

	for(int i=0; i<this->timeConstraintsList.size(); ){
		TimeConstraint* ctr=this->timeConstraintsList[i];
		if(ctr->type==CONSTRAINT_MAX_DAYS_BETWEEN_ACTIVITIES){
			((ConstraintMaxDaysBetweenActivities*)ctr)->removeUseless(*this);
			if(((ConstraintMaxDaysBetweenActivities*)ctr)->n_activities<2)
				this->removeTimeConstraint(ctr);
			else
				i++;
		}
		else
			i++;
	}

	for(int i=0; i<this->timeConstraintsList.size(); ){
		TimeConstraint* ctr=this->timeConstraintsList[i];
		if(ctr->type==CONSTRAINT_MIN_GAPS_BETWEEN_ACTIVITIES){
			((ConstraintMinGapsBetweenActivities*)ctr)->removeUseless(*this);
			if(((ConstraintMinGapsBetweenActivities*)ctr)->n_activities<2)
				this->removeTimeConstraint(ctr);
			else
				i++;
		}
		else
			i++;
	}

	for(int i=0; i<this->timeConstraintsList.size(); ){
		TimeConstraint* ctr=this->timeConstraintsList[i];
		if(ctr->type==CONSTRAINT_ACTIVITIES_SAME_STARTING_TIME){
			((ConstraintActivitiesSameStartingTime*)ctr)->removeUseless(*this);
			if(((ConstraintActivitiesSameStartingTime*)ctr)->n_activities<2)
				this->removeTimeConstraint(ctr);
			else
				i++;
		}
		else
			i++;
	}

	for(int i=0; i<this->timeConstraintsList.size(); ){
		TimeConstraint* ctr=this->timeConstraintsList[i];
		if(ctr->type==CONSTRAINT_ACTIVITIES_SAME_STARTING_HOUR){
			((ConstraintActivitiesSameStartingHour*)ctr)->removeUseless(*this);
			if(((ConstraintActivitiesSameStartingHour*)ctr)->n_activities<2)
				this->removeTimeConstraint(ctr);
			else
				i++;
		}
		else
			i++;
	}

	for(int i=0; i<this->timeConstraintsList.size(); ){
		TimeConstraint* ctr=this->timeConstraintsList[i];
		if(ctr->type==CONSTRAINT_ACTIVITIES_SAME_STARTING_DAY){
			((ConstraintActivitiesSameStartingDay*)ctr)->removeUseless(*this);
			if(((ConstraintActivitiesSameStartingDay*)ctr)->n_activities<2)
				this->removeTimeConstraint(ctr);
			else
				i++;
		}
		else
			i++;
	}

	for(int i=0; i<this->timeConstraintsList.size(); ){
		TimeConstraint* ctr=this->timeConstraintsList[i];
		if(ctr->type==CONSTRAINT_ACTIVITIES_NOT_OVERLAPPING){
			((ConstraintActivitiesNotOverlapping*)ctr)->removeUseless(*this);
			if(((ConstraintActivitiesNotOverlapping*)ctr)->n_activities<2)
				this->removeTimeConstraint(ctr);
			else
				i++;
		}
		else
			i++;
	}

	if(recomputeTime){
		LockUnlock::computeLockedUnlockedActivitiesOnlyTime();
	}
	if(recomputeSpace){
		LockUnlock::computeLockedUnlockedActivitiesOnlySpace();
	}
	if(recomputeTime || recomputeSpace){
		LockUnlock::increaseCommunicationSpinBox();
	}

	this->internalStructureComputed=false;
}

void Rules::modifyActivity(
	int _id,
	int _activityGroupId,
	const QStringList& _teachersNames,
	const QString& _subjectName,
	const QStringList& _activityTagsNames,
	const QStringList& _studentsNames,
	//int _nTotalStudents,
	int _nSplits,
	int _totalDuration,
	int _durations[],
	//int _parities[],
	bool _active[],
	bool _computeNTotalStudents,
	int _nTotalStudents)
{
	int i=0;
	for(int j=0; j<this->activitiesList.size(); j++){
		Activity* act=this->activitiesList[j];
		if((_activityGroupId==0 && act->id==_id) || (_activityGroupId!=0 && act->activityGroupId==_activityGroupId)){
			act->teachersNames=_teachersNames;
			act->subjectName=_subjectName;
			act->activityTagsNames=_activityTagsNames;
			act->studentsNames=_studentsNames;
			act->duration=_durations[i];
			//act->parity=_parities[i];
			act->active=_active[i];
			act->totalDuration=_totalDuration;
			act->computeNTotalStudents=_computeNTotalStudents;
			act->nTotalStudents=_nTotalStudents;
			i++;
		}
	}
		
	assert(i==_nSplits);
	
	this->internalStructureComputed=false;
}

void Rules::modifySubactivity(
	int _id,
	int _activityGroupId,
	const QStringList& _teachersNames,
	const QString& _subjectName,
	const QStringList& _activityTagsNames,
	const QStringList& _studentsNames,
	int _duration,
	bool _active,
	bool _computeNTotalStudents,
	int _nTotalStudents)
{
	QList<Activity*> actsList;
	Activity* crtAct=NULL;
	
	foreach(Activity* act, this->activitiesList){
		if(act->id==_id && act->activityGroupId==_activityGroupId){
			crtAct=act;
			//actsList.append(act);
		}
		else if(act->activityGroupId!=0 && _activityGroupId!=0 && act->activityGroupId==_activityGroupId){
			actsList.append(act);
		}
	}
	
	assert(crtAct!=NULL);
	
	int td=0;
	foreach(Activity* act, actsList)
		td+=act->duration;
	td+=_duration; //crtAct->duration;
	foreach(Activity* act, actsList)
		act->totalDuration=td;

	crtAct->teachersNames=_teachersNames;
	crtAct->subjectName=_subjectName;
	crtAct->activityTagsNames=_activityTagsNames;
	crtAct->studentsNames=_studentsNames;
	crtAct->duration=_duration;
	crtAct->totalDuration=td;
	crtAct->active=_active;
	crtAct->computeNTotalStudents=_computeNTotalStudents;
	crtAct->nTotalStudents=_nTotalStudents;
	
	this->internalStructureComputed=false;
}

bool Rules::addRoom(Room* rm)
{
	if(this->searchRoom(rm->name) >= 0)
		return false;
	this->roomsList << rm; //append
	this->internalStructureComputed=false;

	teachers_schedule_ready=false;
	students_schedule_ready=false;
	rooms_schedule_ready=false;

	return true;
}

bool Rules::addRoomFast(Room* rm)
{
	this->roomsList << rm; //append
	this->internalStructureComputed=false;

	teachers_schedule_ready=false;
	students_schedule_ready=false;
	rooms_schedule_ready=false;

	return true;
}

int Rules::searchRoom(const QString& roomName)
{
	for(int i=0; i<this->roomsList.size(); i++)
		if(this->roomsList[i]->name==roomName)
			return i;
	
	return -1;
}

bool Rules::removeRoom(const QString& roomName)
{
	bool recomputeSpace=false;

	int i=this->searchRoom(roomName);
	if(i<0)
		return false;

	Room* searchedRoom=this->roomsList[i];
	assert(searchedRoom->name==roomName);

	for(int j=0; j<this->spaceConstraintsList.size(); ){
		SpaceConstraint* ctr=this->spaceConstraintsList[j];
		if(ctr->type==CONSTRAINT_ROOM_NOT_AVAILABLE_TIMES){
			ConstraintRoomNotAvailableTimes* crna=(ConstraintRoomNotAvailableTimes*)ctr;
			if(crna->room==roomName)
				this->removeSpaceConstraint(ctr);
			else
				j++;
		}
		else if(ctr->type==CONSTRAINT_ACTIVITY_PREFERRED_ROOM){
			ConstraintActivityPreferredRoom* c=(ConstraintActivityPreferredRoom*)ctr;
			if(c->roomName==roomName){
				this->removeSpaceConstraint(ctr);
				
				recomputeSpace=true;
			}
			else
				j++;
		}
		else if(ctr->type==CONSTRAINT_ACTIVITY_PREFERRED_ROOMS){
			ConstraintActivityPreferredRooms* c=(ConstraintActivityPreferredRooms*)ctr;
			int t=c->roomsNames.remove(roomName);
			assert(t<=1);
			if(t==1 && c->roomsNames.count()==0)
				this->removeSpaceConstraint(ctr);
			else if(t==1 && c->roomsNames.count()==1){
				ConstraintActivityPreferredRoom* c2=new ConstraintActivityPreferredRoom
				 (c->weightPercentage, c->activityId, c->roomsNames.at(0), true); //true means permanently locked

				QMessageBox::information(NULL, tr("FET information"), 
				 tr("The constraint\n%1 will be modified into constraint\n%2 because"
				 " there is only one room left in the constraint")
				 .arg(c->getDetailedDescription(*this))
				 .arg(c2->getDetailedDescription(*this)));

				this->removeSpaceConstraint(ctr);
				this->addSpaceConstraint(c2);
				
				recomputeSpace=true;
			}
			else
				j++;
		}
		else if(ctr->type==CONSTRAINT_STUDENTS_SET_HOME_ROOM){
			ConstraintStudentsSetHomeRoom* c=(ConstraintStudentsSetHomeRoom*)ctr;
			if(c->roomName==roomName)
				this->removeSpaceConstraint(ctr);
			else
				j++;
		}
		else if(ctr->type==CONSTRAINT_STUDENTS_SET_HOME_ROOMS){
			ConstraintStudentsSetHomeRooms* c=(ConstraintStudentsSetHomeRooms*)ctr;
			int t=c->roomsNames.remove(roomName);
			assert(t<=1);
			if(t==1 && c->roomsNames.count()==0)
				this->removeSpaceConstraint(ctr);
			else if(t==1 && c->roomsNames.count()==1){
				ConstraintStudentsSetHomeRoom* c2=new ConstraintStudentsSetHomeRoom
				 (c->weightPercentage, c->studentsName, c->roomsNames.at(0));

				QMessageBox::information(NULL, tr("FET information"), 
				 tr("The constraint\n%1 will be modified into constraint\n%2 because"
				 " there is only one room left in the constraint")
				 .arg(c->getDetailedDescription(*this))
				 .arg(c2->getDetailedDescription(*this)));

				this->removeSpaceConstraint(ctr);
				this->addSpaceConstraint(c2);
			}
			else
				j++;
		}
		else if(ctr->type==CONSTRAINT_TEACHER_HOME_ROOM){
			ConstraintTeacherHomeRoom* c=(ConstraintTeacherHomeRoom*)ctr;
			if(c->roomName==roomName)
				this->removeSpaceConstraint(ctr);
			else
				j++;
		}
		else if(ctr->type==CONSTRAINT_TEACHER_HOME_ROOMS){
			ConstraintTeacherHomeRooms* c=(ConstraintTeacherHomeRooms*)ctr;
			int t=c->roomsNames.remove(roomName);
			assert(t<=1);
			if(t==1 && c->roomsNames.count()==0)
				this->removeSpaceConstraint(ctr);
			else if(t==1 && c->roomsNames.count()==1){
				ConstraintTeacherHomeRoom* c2=new ConstraintTeacherHomeRoom
				 (c->weightPercentage, c->teacherName, c->roomsNames.at(0));

				QMessageBox::information(NULL, tr("FET information"), 
				 tr("The constraint\n%1 will be modified into constraint\n%2 because"
				 " there is only one room left in the constraint")
				 .arg(c->getDetailedDescription(*this))
				 .arg(c2->getDetailedDescription(*this)));

				this->removeSpaceConstraint(ctr);
				this->addSpaceConstraint(c2);
			}
			else
				j++;
		}
		else if(ctr->type==CONSTRAINT_SUBJECT_PREFERRED_ROOM){
			ConstraintSubjectPreferredRoom* c=(ConstraintSubjectPreferredRoom*)ctr;
			if(c->roomName==roomName)
				this->removeSpaceConstraint(ctr);
			else
				j++;
		}
		else if(ctr->type==CONSTRAINT_SUBJECT_PREFERRED_ROOMS){
			ConstraintSubjectPreferredRooms* c=(ConstraintSubjectPreferredRooms*)ctr;
			int t=c->roomsNames.remove(roomName);
			assert(t<=1);
			if(t==1 && c->roomsNames.count()==0)
				this->removeSpaceConstraint(ctr);
			else if(t==1 && c->roomsNames.count()==1){
				ConstraintSubjectPreferredRoom* c2=new ConstraintSubjectPreferredRoom
				 (c->weightPercentage, c->subjectName, c->roomsNames.at(0));

				QMessageBox::information(NULL, tr("FET information"), 
				 tr("The constraint\n%1 will be modified into constraint\n%2 because"
				 " there is only one room left in the constraint")
				 .arg(c->getDetailedDescription(*this))
				 .arg(c2->getDetailedDescription(*this)));

				this->removeSpaceConstraint(ctr);
				this->addSpaceConstraint(c2);
			}
			else
				j++;
		}
		else if(ctr->type==CONSTRAINT_SUBJECT_ACTIVITY_TAG_PREFERRED_ROOM){
			ConstraintSubjectActivityTagPreferredRoom* c=(ConstraintSubjectActivityTagPreferredRoom*)ctr;
			if(c->roomName==roomName)
				this->removeSpaceConstraint(ctr);
			else
				j++;
		}
		else if(ctr->type==CONSTRAINT_SUBJECT_ACTIVITY_TAG_PREFERRED_ROOMS){
			ConstraintSubjectActivityTagPreferredRooms* c=(ConstraintSubjectActivityTagPreferredRooms*)ctr;
			int t=c->roomsNames.remove(roomName);
			assert(t<=1);
			if(t==1 && c->roomsNames.count()==0)
				this->removeSpaceConstraint(ctr);
			else if(t==1 && c->roomsNames.count()==1){
				ConstraintSubjectActivityTagPreferredRoom* c2=new ConstraintSubjectActivityTagPreferredRoom
				 (c->weightPercentage, c->subjectName, c->activityTagName, c->roomsNames.at(0));

				QMessageBox::information(NULL, tr("FET information"), 
				 tr("The constraint\n%1 will be modified into constraint\n%2 because"
				 " there is only one room left in the constraint")
				 .arg(c->getDetailedDescription(*this))
				 .arg(c2->getDetailedDescription(*this)));

				this->removeSpaceConstraint(ctr);
				this->addSpaceConstraint(c2);
			}
			else
				j++;
		}

		else if(ctr->type==CONSTRAINT_ACTIVITY_TAG_PREFERRED_ROOM){
			ConstraintActivityTagPreferredRoom* c=(ConstraintActivityTagPreferredRoom*)ctr;
			if(c->roomName==roomName)
				this->removeSpaceConstraint(ctr);
			else
				j++;
		}
		else if(ctr->type==CONSTRAINT_ACTIVITY_TAG_PREFERRED_ROOMS){
			ConstraintActivityTagPreferredRooms* c=(ConstraintActivityTagPreferredRooms*)ctr;
			int t=c->roomsNames.remove(roomName);
			assert(t<=1);
			if(t==1 && c->roomsNames.count()==0)
				this->removeSpaceConstraint(ctr);
			else if(t==1 && c->roomsNames.count()==1){
				ConstraintActivityTagPreferredRoom* c2=new ConstraintActivityTagPreferredRoom
				 (c->weightPercentage, c->activityTagName, c->roomsNames.at(0));

				QMessageBox::information(NULL, tr("FET information"), 
				 tr("The constraint\n%1 will be modified into constraint\n%2 because"
				 " there is only one room left in the constraint")
				 .arg(c->getDetailedDescription(*this))
				 .arg(c2->getDetailedDescription(*this)));

				this->removeSpaceConstraint(ctr);
				this->addSpaceConstraint(c2);
			}
			else
				j++;
		}

		else
			j++;
	}

	delete this->roomsList[i];
	this->roomsList.removeAt(i);

	if(recomputeSpace){
		LockUnlock::computeLockedUnlockedActivitiesOnlySpace();
		LockUnlock::increaseCommunicationSpinBox();
	}

	this->internalStructureComputed=false;

	teachers_schedule_ready=false;
	students_schedule_ready=false;
	rooms_schedule_ready=false;

	return true;
}

bool Rules::modifyRoom(const QString& initialRoomName, const QString& finalRoomName, const QString& building, int capacity)
{
	int i=this->searchRoom(initialRoomName);
	if(i<0)
		return false;

	Room* searchedRoom=this->roomsList[i];
	assert(searchedRoom->name==initialRoomName);

	for(int j=0; j<this->spaceConstraintsList.size(); ){
		SpaceConstraint* ctr=this->spaceConstraintsList[j];
		if(ctr->type==CONSTRAINT_ROOM_NOT_AVAILABLE_TIMES){
			ConstraintRoomNotAvailableTimes* crna=(ConstraintRoomNotAvailableTimes*)ctr;
			if(crna->room==initialRoomName)
				crna->room=finalRoomName;
			j++;
		}
		else if(ctr->type==CONSTRAINT_ACTIVITY_PREFERRED_ROOM){
			ConstraintActivityPreferredRoom* c=(ConstraintActivityPreferredRoom*)ctr;
			if(c->roomName==initialRoomName)
				c->roomName=finalRoomName;
			j++;
		}
		else if(ctr->type==CONSTRAINT_ACTIVITY_PREFERRED_ROOMS){
			ConstraintActivityPreferredRooms* c=(ConstraintActivityPreferredRooms*)ctr;
			int t=0;
			for(QStringList::Iterator it=c->roomsNames.begin(); it!=c->roomsNames.end(); it++){
				if((*it)==initialRoomName){
					*it=finalRoomName;
					t++;
				}
			}
			assert(t<=1);
			j++;
		}
		else if(ctr->type==CONSTRAINT_STUDENTS_SET_HOME_ROOM){
			ConstraintStudentsSetHomeRoom* c=(ConstraintStudentsSetHomeRoom*)ctr;
			if(c->roomName==initialRoomName)
				c->roomName=finalRoomName;
			j++;
		}
		else if(ctr->type==CONSTRAINT_STUDENTS_SET_HOME_ROOMS){
			ConstraintStudentsSetHomeRooms* c=(ConstraintStudentsSetHomeRooms*)ctr;
			int t=0;
			for(QStringList::Iterator it=c->roomsNames.begin(); it!=c->roomsNames.end(); it++){
				if((*it)==initialRoomName){
					*it=finalRoomName;
					t++;
				}
			}
			assert(t<=1);
			j++;
		}
		else if(ctr->type==CONSTRAINT_TEACHER_HOME_ROOM){
			ConstraintTeacherHomeRoom* c=(ConstraintTeacherHomeRoom*)ctr;
			if(c->roomName==initialRoomName)
				c->roomName=finalRoomName;
			j++;
		}
		else if(ctr->type==CONSTRAINT_TEACHER_HOME_ROOMS){
			ConstraintTeacherHomeRooms* c=(ConstraintTeacherHomeRooms*)ctr;
			int t=0;
			for(QStringList::Iterator it=c->roomsNames.begin(); it!=c->roomsNames.end(); it++){
				if((*it)==initialRoomName){
					*it=finalRoomName;
					t++;
				}
			}
			assert(t<=1);
			j++;
		}
		else if(ctr->type==CONSTRAINT_SUBJECT_PREFERRED_ROOM){
			ConstraintSubjectPreferredRoom* c=(ConstraintSubjectPreferredRoom*)ctr;
			if(c->roomName==initialRoomName)
				c->roomName=finalRoomName;
			j++;
		}
		else if(ctr->type==CONSTRAINT_SUBJECT_PREFERRED_ROOMS){
			ConstraintSubjectPreferredRooms* c=(ConstraintSubjectPreferredRooms*)ctr;
			int t=0;
			for(QStringList::Iterator it=c->roomsNames.begin(); it!=c->roomsNames.end(); it++){
				if((*it)==initialRoomName){
					*it=finalRoomName;
					t++;
				}
			}
			assert(t<=1);
			j++;
		}
		else if(ctr->type==CONSTRAINT_SUBJECT_ACTIVITY_TAG_PREFERRED_ROOM){
			ConstraintSubjectActivityTagPreferredRoom* c=(ConstraintSubjectActivityTagPreferredRoom*)ctr;
			if(c->roomName==initialRoomName)
				c->roomName=finalRoomName;
			j++;
		}
		else if(ctr->type==CONSTRAINT_SUBJECT_ACTIVITY_TAG_PREFERRED_ROOMS){
			ConstraintSubjectActivityTagPreferredRooms* c=(ConstraintSubjectActivityTagPreferredRooms*)ctr;
			int t=0;
			for(QStringList::Iterator it=c->roomsNames.begin(); it!=c->roomsNames.end(); it++){
				if((*it)==initialRoomName){
					*it=finalRoomName;
					t++;
				}
			}
			assert(t<=1);
			j++;
		}

		else if(ctr->type==CONSTRAINT_ACTIVITY_TAG_PREFERRED_ROOM){
			ConstraintActivityTagPreferredRoom* c=(ConstraintActivityTagPreferredRoom*)ctr;
			if(c->roomName==initialRoomName)
				c->roomName=finalRoomName;
			j++;
		}
		else if(ctr->type==CONSTRAINT_ACTIVITY_TAG_PREFERRED_ROOMS){
			ConstraintActivityTagPreferredRooms* c=(ConstraintActivityTagPreferredRooms*)ctr;
			int t=0;
			for(QStringList::Iterator it=c->roomsNames.begin(); it!=c->roomsNames.end(); it++){
				if((*it)==initialRoomName){
					*it=finalRoomName;
					t++;
				}
			}
			assert(t<=1);
			j++;
		}

		else
			j++;
	}

	searchedRoom->name=finalRoomName;
	searchedRoom->building=building;
	searchedRoom->capacity=capacity;

	this->internalStructureComputed=false;
	return true;
}

void Rules::sortRoomsAlphabetically()
{
	qSort(this->roomsList.begin(), this->roomsList.end(), roomsAscending);

	this->internalStructureComputed=false;
}

bool Rules::addBuilding(Building* bu)
{
	if(this->searchBuilding(bu->name) >= 0)
		return false;
	this->buildingsList << bu; //append
	this->internalStructureComputed=false;

	teachers_schedule_ready=false;
	students_schedule_ready=false;
	rooms_schedule_ready=false;

	return true;
}

bool Rules::addBuildingFast(Building* bu)
{
	this->buildingsList << bu; //append
	this->internalStructureComputed=false;

	teachers_schedule_ready=false;
	students_schedule_ready=false;
	rooms_schedule_ready=false;

	return true;
}

int Rules::searchBuilding(const QString& buildingName)
{
	for(int i=0; i<this->buildingsList.size(); i++)
		if(this->buildingsList[i]->name==buildingName)
			return i;
	
	return -1;
}

bool Rules::removeBuilding(const QString& buildingName)
{
	foreach(Room* rm, this->roomsList)
		if(rm->building==buildingName)
			rm->building="";

	int i=this->searchBuilding(buildingName);
	if(i<0)
		return false;

	Building* searchedBuilding=this->buildingsList[i];
	assert(searchedBuilding->name==buildingName);

	delete this->buildingsList[i];
	this->buildingsList.removeAt(i);
	
	this->internalStructureComputed=false;

	teachers_schedule_ready=false;
	students_schedule_ready=false;
	rooms_schedule_ready=false;

	return true;
}

bool Rules::modifyBuilding(const QString& initialBuildingName, const QString& finalBuildingName)
{
	foreach(Room* rm, roomsList)
		if(rm->building==initialBuildingName)
			rm->building=finalBuildingName;

	int i=this->searchBuilding(initialBuildingName);
	if(i<0)
		return false;

	Building* searchedBuilding=this->buildingsList[i];
	assert(searchedBuilding->name==initialBuildingName);
	searchedBuilding->name=finalBuildingName;

	this->internalStructureComputed=false;
	return true;
}

void Rules::sortBuildingsAlphabetically()
{
	qSort(this->buildingsList.begin(), this->buildingsList.end(), buildingsAscending);

	this->internalStructureComputed=false;
}

bool Rules::addTimeConstraint(TimeConstraint *ctr)
{
	bool ok=true;

	//TODO: improve this

	//check if this constraint is already added, for ConstraintActivityPreferredTime
	if(ctr->type==CONSTRAINT_ACTIVITY_PREFERRED_STARTING_TIME){
		int i;
		for(i=0; i<this->timeConstraintsList.size(); i++){
			TimeConstraint* ctr2=this->timeConstraintsList[i];
			if(ctr2->type==CONSTRAINT_ACTIVITY_PREFERRED_STARTING_TIME) 
				if(
				 *((ConstraintActivityPreferredStartingTime*)ctr2)
				 ==
				 *((ConstraintActivityPreferredStartingTime*)ctr)
				)
					break;
		}
				
		if(i<this->timeConstraintsList.size())
			ok=false;
	}

	//check if this constraint is already added, for ConstraintMinDaysBetweenActivities
	else if(ctr->type==CONSTRAINT_MIN_DAYS_BETWEEN_ACTIVITIES){
		int i;
		for(i=0; i<this->timeConstraintsList.size(); i++){
			TimeConstraint* ctr2=this->timeConstraintsList[i];
			if(ctr2->type==CONSTRAINT_MIN_DAYS_BETWEEN_ACTIVITIES)
				if(
				 *((ConstraintMinDaysBetweenActivities*)ctr2)
				 ==
				 *((ConstraintMinDaysBetweenActivities*)ctr)
				 )
					break;
		}

		if(i<this->timeConstraintsList.size())
			ok=false;
	}
	
	else if(ctr->type==CONSTRAINT_STUDENTS_SET_NOT_AVAILABLE_TIMES){
		int i;
		ConstraintStudentsSetNotAvailableTimes* ssna=(ConstraintStudentsSetNotAvailableTimes*)ctr;
		for(i=0; i<this->timeConstraintsList.size(); i++){
			TimeConstraint* ctr2=this->timeConstraintsList[i];
			if(ctr2->type==CONSTRAINT_STUDENTS_SET_NOT_AVAILABLE_TIMES) {
				ConstraintStudentsSetNotAvailableTimes* ssna2=(ConstraintStudentsSetNotAvailableTimes*)ctr2;
				if(ssna->students==ssna2->students)
					break;
			}
		}
				
		if(i<this->timeConstraintsList.size())
			ok=false;
	}
	
	else if(ctr->type==CONSTRAINT_TEACHER_NOT_AVAILABLE_TIMES){
		int i;
		ConstraintTeacherNotAvailableTimes* tna=(ConstraintTeacherNotAvailableTimes*)ctr;
		for(i=0; i<this->timeConstraintsList.size(); i++){
			TimeConstraint* ctr2=this->timeConstraintsList[i];
			if(ctr2->type==CONSTRAINT_TEACHER_NOT_AVAILABLE_TIMES) {
				ConstraintTeacherNotAvailableTimes* tna2=(ConstraintTeacherNotAvailableTimes*)ctr2;
				if(tna->teacher==tna2->teacher)
					break;
			}
		}
				
		if(i<this->timeConstraintsList.size())
			ok=false;
	}
	
	else if(ctr->type==CONSTRAINT_BREAK_TIMES){
		int i;
		for(i=0; i<this->timeConstraintsList.size(); i++){
			TimeConstraint* ctr2=this->timeConstraintsList[i];
			if(ctr2->type==CONSTRAINT_BREAK_TIMES)
				break;
		}
				
		if(i<this->timeConstraintsList.size())
			ok=false;
	}
	
	else if(ctr->type==CONSTRAINT_BASIC_COMPULSORY_TIME){
		int i;
		for(i=0; i<this->timeConstraintsList.size(); i++){
			TimeConstraint* ctr2=this->timeConstraintsList[i];
			if(ctr2->type==CONSTRAINT_BASIC_COMPULSORY_TIME)
				break;
		}
				
		if(i<this->timeConstraintsList.size())
			ok=false;
	}
	
	if(ok){
		this->timeConstraintsList << ctr; //append
		this->internalStructureComputed=false;
		return true;
	}
	else
		return false;
}

bool Rules::removeTimeConstraint(TimeConstraint *ctr)
{
	for(int i=0; i<this->timeConstraintsList.size(); i++)
		if(this->timeConstraintsList[i]==ctr){
			delete ctr;
			this->timeConstraintsList.removeAt(i);
			this->internalStructureComputed=false;

			return true;
		}

	return false;
}

bool Rules::addSpaceConstraint(SpaceConstraint *ctr)
{
	bool ok=true;

	//TODO: check if this constraint is already added...(if any possibility of duplicates)
	if(ctr->type==CONSTRAINT_ACTIVITY_PREFERRED_ROOM){
		int i;
		for(i=0; i<this->spaceConstraintsList.size(); i++){
			SpaceConstraint* ctr2=this->spaceConstraintsList[i];
			if(ctr2->type==CONSTRAINT_ACTIVITY_PREFERRED_ROOM)
				if(
				 *((ConstraintActivityPreferredRoom*)ctr2)
				 ==
				 *((ConstraintActivityPreferredRoom*)ctr)
				)
					break;
		}
		
		if(i<this->spaceConstraintsList.size())
			ok=false;
	}
/*	else if(ctr->type==CONSTRAINT_ROOM_NOT_AVAILABLE_TIMES){
		int i;
		ConstraintRoomNotAvailableTimes* c=(ConstraintRoomNotAvailableTimes*)ctr;
		for(i=0; i<this->spaceConstraintsList.size(); i++){
			SpaceConstraint* ctr2=this->spaceConstraintsList[i];
			if(ctr2->type==CONSTRAINT_ROOM_NOT_AVAILABLE_TIMES){
				ConstraintRoomNotAvailableTimes* c2=(ConstraintRoomNotAvailableTimes*)ctr2;				
				if(c->room==c2->room)
					break;
			}
		}
		
		if(i<this->spaceConstraintsList.size())
			ok=false;
	}*/
	else if(ctr->type==CONSTRAINT_BASIC_COMPULSORY_SPACE){
		int i;
		for(i=0; i<this->spaceConstraintsList.size(); i++){
			SpaceConstraint* ctr2=this->spaceConstraintsList[i];
			if(ctr2->type==CONSTRAINT_BASIC_COMPULSORY_SPACE)
				break;
		}
				
		if(i<this->spaceConstraintsList.size())
			ok=false;
	}

	if(ok){
		this->spaceConstraintsList << ctr; //append
		this->internalStructureComputed=false;
		return true;
	}
	else
		return false;
}

bool Rules::removeSpaceConstraint(SpaceConstraint *ctr)
{
	for(int i=0; i<this->spaceConstraintsList.size(); i++)
		if(this->spaceConstraintsList[i]==ctr){
			delete ctr;
			this->spaceConstraintsList.removeAt(i);
			this->internalStructureComputed=false;

			return true;
		}

	return false;
}

bool Rules::read(const QString& filename, bool commandLine, QString commandLineDirectory) //commandLineDirectory has trailing FILE_SEP
{
	//bool reportWhole=true;

	QFile file(filename);
	if(!file.open(QIODevice::ReadOnly)){
		//cout<<"Could not open file - not existing or in use\n";
		QMessageBox::warning(NULL, tr("FET warning"), tr("Could not open file - not existing or in use"));
		return false;
	}
	//QDomDocument doc("xml_rules");
	QDomDocument doc;
	
	QString errorStr;
	int errorLine;
	int errorColumn;
	
	if(!doc.setContent(&file, true, &errorStr, &errorLine, &errorColumn)){	
		QMessageBox::warning(NULL, tr("FET warning"), 
		 tr("Could not read file - XML parse error at line %1, column %2:\n%3", "The error description is %3")
		 .arg(errorLine)
		 .arg(errorColumn)
		 .arg(errorStr));
	
		file.close();
		return false;
	}
	file.close();

	////////////////////////////////////////

	if(!commandLine){
		//logging part
		QDir dir;
		bool t=true;
		if(!dir.exists(OUTPUT_DIR+FILE_SEP+"logs"))
			t=dir.mkpath(OUTPUT_DIR+FILE_SEP+"logs");
		if(!t){
			QMessageBox::warning(NULL, tr("FET warning"), tr("Cannot create or use directory %1 - cannot continue").arg(QDir::toNativeSeparators(OUTPUT_DIR+FILE_SEP+"logs")));
			return false;
		}
		assert(t);
	}
	else{
		QDir dir;
		bool t=true;
		if(!dir.exists(commandLineDirectory+"logs"))
			t=dir.mkpath(commandLineDirectory+"logs");
		if(!t){
			QMessageBox::warning(NULL, tr("FET warning"), tr("Cannot create or use directory %1 - cannot continue").arg(QDir::toNativeSeparators(commandLineDirectory+"logs")));
			return false;
		}
		assert(t);
	}
	
	FakeString xmlReadingLog;
	xmlReadingLog="";

	QDate dat=QDate::currentDate();
	QTime tim=QTime::currentTime();
	QLocale loc(FET_LANGUAGE);
	QString sTime=loc.toString(dat, QLocale::ShortFormat)+" "+loc.toString(tim, QLocale::ShortFormat);

	QString reducedXmlLog="";
	reducedXmlLog+="Log generated by FET "+FET_VERSION+" on "+sTime+"\n\n";
	QString shortFilename=filename.right(filename.length()-filename.findRev(FILE_SEP)-1);
	reducedXmlLog+="Reading file "+shortFilename+"\n";
	QFileInfo fileinfo(filename);
	reducedXmlLog+="Complete file name, including path: "+QDir::toNativeSeparators(fileinfo.absoluteFilePath())+"\n";
	/*QStringList tl=filename.split(FILE_SEP);
	int tk=0;
	foreach(QString ts, tl){
		tk++;
		QString tts;
		tts.fill(' ', 2*tk);
		reducedXmlLog+=tts;
		reducedXmlLog+=ts+"\n";
	}*/
	reducedXmlLog+="\n";

	QString tmp;
	if(commandLine)
		tmp=commandLineDirectory+"logs"+FILE_SEP+XML_PARSING_LOG_FILENAME;
	else
		tmp=OUTPUT_DIR+FILE_SEP+"logs"+FILE_SEP+XML_PARSING_LOG_FILENAME;
	QFile file2(tmp);
	bool canWriteLogFile=true;
	if(!file2.open(QIODevice::WriteOnly)){
		QString s=tr("FET cannot open the log file %1 for writing. This might mean that you don't"
			" have write permissions in this location. You can continue operation, but you might not be able to save the generated timetables"
			" as html files").arg(QDir::toNativeSeparators(tmp))+
			"\n\n"+tr("A solution is to remove that file (if it exists already) or set its permissions to allow writing")+
			"\n\n"+tr("Please report possible bug");
		QMessageBox::critical(NULL, tr("FET critical"), s);
		canWriteLogFile=false;
	}
	QTextStream logStream;
	if(canWriteLogFile){
		logStream.setDevice(&file2);
		logStream.setCodec("UTF-8");
		logStream.setGenerateByteOrderMark(true);
	}
	
	QDomElement elem1=doc.documentElement();
	xmlReadingLog+=" Found "+elem1.tagName()+" tag\n";
	bool okAbove3_12_17=true;
	bool version5AndAbove=false;
	bool warning=false;
	
	QString file_version;
	
	bool okfetTag;
	if(elem1.tagName()=="fet" || elem1.tagName()=="FET") //the new tag is fet, the old tag - FET
		okfetTag=true;
	else
		okfetTag=false;
	
	if(!okfetTag)
		okAbove3_12_17=false;
	else{
		assert(okAbove3_12_17==true);
		/*QDomDocumentType dt=doc.doctype();
		if(dt.isNull() || dt.name()!="FET")
			okAbove3_12_17=false;
		else*/
		int filev[3], fetv[3];
		
		QDomAttr a=elem1.attributeNode("version");
		
		QString version=a.value();
		file_version=version;

		QRegExp fileVerReCap("^(\\d+)\\.(\\d+)\\.(\\d+)(.*)$");

		int tfile=fileVerReCap.indexIn(file_version);
		filev[0]=filev[1]=filev[2]=-1;
		if(tfile!=0){
			QMessageBox::warning(NULL, tr("FET warning"), tr("File contains a version numbering scheme which"
			" is not matched by v.v.va (3 numbers separated by points, followed by any string a, which may be empty). File will be opened, but you are adviced"
			" to check the version of the .fet file (in the beginning of the file). If this is a FET bug, please report it")+"\n\n"+
			tr("If you are opening a file older than FET format version 5, it will be converted to latest FET data format"));
			cout<<"Opened file version not matched by regexp"<<endl;
		}
		else{
			bool ok;
			filev[0]=fileVerReCap.cap(1).toInt(&ok);
			assert(ok);
			filev[1]=fileVerReCap.cap(2).toInt(&ok);
			assert(ok);
			filev[2]=fileVerReCap.cap(3).toInt(&ok);
			assert(ok);
			cout<<"Opened file version matched by regexp: major="<<filev[0]<<", minor="<<filev[1]<<", revision="<<filev[2];
			cout<<", additional text="<<qPrintable(fileVerReCap.cap(4))<<"."<<endl;
		}
		
		QRegExp fetVerReCap("^(\\d+)\\.(\\d+)\\.(\\d+)(.*)$");

		int tfet=fetVerReCap.indexIn(FET_VERSION);
		fetv[0]=fetv[1]=fetv[2]=-1;
		if(tfet!=0){
			QMessageBox::warning(NULL, tr("FET warning"), tr("FET version does not respect the format v.v.va"
			" (3 numbers separated by points, followed by any string a, which may be empty). This is probably a bug in FET - please report it"));
			cout<<"FET version not matched by regexp"<<endl;
		}
		else{
			bool ok;
			fetv[0]=fetVerReCap.cap(1).toInt(&ok);
			assert(ok);
			fetv[1]=fetVerReCap.cap(2).toInt(&ok);
			assert(ok);
			fetv[2]=fetVerReCap.cap(3).toInt(&ok);
			assert(ok);
			cout<<"FET version matched by regexp: major="<<fetv[0]<<", minor="<<fetv[1]<<", revision="<<fetv[2];
			cout<<", additional text="<<qPrintable(fetVerReCap.cap(4))<<"."<<endl;
		}
		
		if(filev[0]>=0 && fetv[0]>=0 && filev[1]>=0 && fetv[1]>=0 && filev[2]>=0 && fetv[2]>=0){
			if(filev[0]>fetv[0] || (filev[0]==fetv[0] && filev[1]>fetv[1]) || (filev[0]==fetv[0]&&filev[1]==fetv[1]&&filev[2]>fetv[2])){
				warning=true;
			}
		}
		
		if(filev[0]>=5 || (filev[0]==-1 && filev[1]==-1 && filev[2]==-1))
		//if major is >= 5 or major cannot be read
			version5AndAbove=true;
	}
	if(!okAbove3_12_17){
		cout<<"Invalid fet 3.12.17 or above"<<endl;
		file2.close();
		QMessageBox::warning(NULL, tr("FET warning"), tr("File does not have a corresponding beginning tag - it should be %1 or %2. File is incorrect..."
			"it cannot be opened").arg("fet").arg("FET"));
		return false;
	}
	
	if(!version5AndAbove){
		QMessageBox::warning(NULL, tr("FET information"), 
		 tr("Opening older file - it will be converted to latest format, automatically "
		 "assigning weight percentages to constraints and dropping parity for activities. "
		 "You are adviced to make a backup of your old file before saving in new format.\n\n"
		 "Please note that the default weight percentage of constraints min days between activities "
		 "will be 95% (mainly satisfied, not always) and 'force consecutive if same day' will be set to true "
		 "(meaning that if the activities are in the same day, they will be placed continuously, in a bigger duration activity)"
		 "If you want, you can modify this percent to be 100%, manually in the fet input file "
		 "or from the interface"));
	}
	
	if(warning){
		QMessageBox::warning(NULL, tr("FET information"), 
		 tr("Opening a file generated with a newer version than your current FET software ... file will be opened but it is recommended to update your FET software to the latest version")
		 +"\n\n"+tr("Your FET version: %1, file version: %2").arg(FET_VERSION).arg(file_version));
	}
	
	//Clear old rules, initialize new rules
	if(this->initialized)
		this->kill();
	this->init();
	
	this->nHoursPerDay=12;
	this->hoursOfTheDay[0]="08:00";
	this->hoursOfTheDay[1]="09:00";
	this->hoursOfTheDay[2]="10:00";
	this->hoursOfTheDay[3]="11:00";
	this->hoursOfTheDay[4]="12:00";
	this->hoursOfTheDay[5]="13:00";
	this->hoursOfTheDay[6]="14:00";
	this->hoursOfTheDay[7]="15:00";
	this->hoursOfTheDay[8]="16:00";
	this->hoursOfTheDay[9]="17:00";
	this->hoursOfTheDay[10]="18:00";
	this->hoursOfTheDay[11]="19:00";
	//this->hoursOfTheDay[12]="20:00";

	this->nDaysPerWeek=5;
	this->daysOfTheWeek[0] = tr("Monday");
	this->daysOfTheWeek[1] = tr("Tuesday");
	this->daysOfTheWeek[2] = tr("Wednesday");
	this->daysOfTheWeek[3] = tr("Thursday");
	this->daysOfTheWeek[4] = tr("Friday");

	this->institutionName=tr("Default institution");
	this->comments=tr("Default comments");

	bool skipDeprecatedConstraints=false;
	
	bool skipDuplicatedStudentsSets=false;
	
	for(QDomNode node2=elem1.firstChild(); !node2.isNull(); node2=node2.nextSibling()){
		QDomElement elem2=node2.toElement();
		if(elem2.isNull())
			xmlReadingLog+="  Null node here\n";
		xmlReadingLog+="  Found "+elem2.tagName()+" tag\n";
		if(elem2.tagName()=="Institution_Name"){
			this->institutionName=elem2.text();
			xmlReadingLog+="  Found institution name="+this->institutionName+"\n";
			reducedXmlLog+="Read institution name="+this->institutionName+"\n";
		}
		else if(elem2.tagName()=="Comments"){
			this->comments=elem2.text();
			xmlReadingLog+="  Found comments="+this->comments+"\n";
			reducedXmlLog+="Read comments="+this->comments+"\n";
		}
		else if(elem2.tagName()=="Hours_List"){
			int tmp=0;
			for(QDomNode node3=elem2.firstChild(); !node3.isNull(); node3=node3.nextSibling()){
				QDomElement elem3=node3.toElement();
				if(elem3.isNull()){
					xmlReadingLog+="   Null node here\n";
					continue;
				}
				xmlReadingLog+="   Found "+elem3.tagName()+" tag\n";
				if(elem3.tagName()=="Number"){
					this->nHoursPerDay=elem3.text().toInt();
					xmlReadingLog+="   Found the number of hours per day = "+
					 QString::number(this->nHoursPerDay)+"\n";
					reducedXmlLog+="Added "+
					 QString::number(this->nHoursPerDay)+" hours per day\n";
					assert(this->nHoursPerDay>0);
				}
				else if(elem3.tagName()=="Name"){
					this->hoursOfTheDay[tmp]=elem3.text();
					xmlReadingLog+="   Found hour "+this->hoursOfTheDay[tmp]+"\n";
					tmp++;
				}
			}
			//don't do assert tmp == nHoursPerDay, because some older files contain also the end of day hour, so tmp==nHoursPerDay+1 in this case
		}
		else if(elem2.tagName()=="Days_List"){
			int tmp=0;
			for(QDomNode node3=elem2.firstChild(); !node3.isNull(); node3=node3.nextSibling()){
				QDomElement elem3=node3.toElement();
				if(elem3.isNull()){
					xmlReadingLog+="   Null node here\n";
					continue;
				}
				xmlReadingLog+="   Found "+elem3.tagName()+" tag\n";
				if(elem3.tagName()=="Number"){
					this->nDaysPerWeek=elem3.text().toInt();
					xmlReadingLog+="   Found the number of days per week = "+
					 QString::number(this->nDaysPerWeek)+"\n";
					reducedXmlLog+="Added "+
					 QString::number(this->nDaysPerWeek)+" days per week\n";
					assert(this->nDaysPerWeek>0);
				}
				else if(elem3.tagName()=="Name"){
					this->daysOfTheWeek[tmp]=elem3.text();
					xmlReadingLog+="   Found day "+this->daysOfTheWeek[tmp]+"\n";
					tmp++;
				}
			}
			assert(tmp==this->nDaysPerWeek);
		}
		else if(elem2.tagName()=="Teachers_List"){
			QSet<QString> teachersRead;
		
			int tmp=0;
			for(QDomNode node3=elem2.firstChild(); !node3.isNull(); node3=node3.nextSibling()){
				QDomElement elem3=node3.toElement();
				if(elem3.isNull()){
					xmlReadingLog+="   Null node here\n";
					continue;
				}
				xmlReadingLog+="   Found "+elem3.tagName()+" tag\n";
				if(elem3.tagName()=="Teacher"){
					Teacher* teacher=new Teacher();
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Name"){
							teacher->name=elem4.text();
							xmlReadingLog+="    Read teacher name: "+teacher->name+"\n";
						}
					}
					bool tmp2=teachersRead.contains(teacher->name);
					if(tmp2){
						QMessageBox::warning(NULL, tr("FET warning"),
						 tr("Duplicate teacher %1 found - ignoring").arg(teacher->name));
						xmlReadingLog+="   Teacher not added - duplicate\n";
					}
					else{
						teachersRead.insert(teacher->name);
						this->addTeacherFast(teacher);
						tmp++;
						xmlReadingLog+="   Teacher added\n";
					}
				}
			}
			assert(tmp==this->teachersList.size());
			xmlReadingLog+="  Added "+QString::number(tmp)+" teachers\n";
			reducedXmlLog+="Added "+QString::number(tmp)+" teachers\n";
		}
		else if(elem2.tagName()=="Subjects_List"){
			QSet<QString> subjectsRead;
		
			int tmp=0;
			for(QDomNode node3=elem2.firstChild(); !node3.isNull(); node3=node3.nextSibling()){
				QDomElement elem3=node3.toElement();
				if(elem3.isNull()){
					xmlReadingLog+="   Null node here\n";
					continue;
				}
				xmlReadingLog+="   Found "+elem3.tagName()+" tag\n";
				if(elem3.tagName()=="Subject"){
					Subject* subject=new Subject();
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Name"){
							subject->name=elem4.text();
							xmlReadingLog+="    Read subject name: "+subject->name+"\n";
						}
					}
					bool tmp2=subjectsRead.contains(subject->name);
					if(tmp2){
						QMessageBox::warning(NULL, tr("FET warning"),
						 tr("Duplicate subject %1 found - ignoring").arg(subject->name));
						xmlReadingLog+="   Subject not added - duplicate\n";
					}
					else{
						subjectsRead.insert(subject->name);
						this->addSubjectFast(subject);
						tmp++;
						xmlReadingLog+="   Subject inserted\n";
					}
				}
			}
			assert(tmp==this->subjectsList.size());
			xmlReadingLog+="  Added "+QString::number(tmp)+" subjects\n";
			reducedXmlLog+="Added "+QString::number(tmp)+" subjects\n";
		}
		else if(elem2.tagName()=="Subject_Tags_List"){
			QSet<QString> activityTagsRead;
		
			QMessageBox::information(NULL, tr("FET information"), tr("Your file contains subject tags list"
			  ", which is named in versions>=5.5.0 activity tags list"));
		
			int tmp=0;
			for(QDomNode node3=elem2.firstChild(); !node3.isNull(); node3=node3.nextSibling()){
				QDomElement elem3=node3.toElement();
				if(elem3.isNull()){
					xmlReadingLog+="   Null node here\n";
					continue;
				}
				xmlReadingLog+="   Found "+elem3.tagName()+" tag\n";
				if(elem3.tagName()=="Subject_Tag"){
					ActivityTag* activityTag=new ActivityTag();
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Name"){
							activityTag->name=elem4.text();
							xmlReadingLog+="    Read activity tag name: "+activityTag->name+"\n";
						}
					}
					bool tmp2=activityTagsRead.contains(activityTag->name);
					if(tmp2){
						QMessageBox::warning(NULL, tr("FET warning"),
						 tr("Duplicate activity tag %1 found - ignoring").arg(activityTag->name));
						xmlReadingLog+="   Activity tag not added - duplicate\n";
					}
					else{
						activityTagsRead.insert(activityTag->name);
						addActivityTagFast(activityTag);
						tmp++;
						xmlReadingLog+="   Activity tag inserted\n";
					}
				}
			}
			assert(tmp==this->activityTagsList.size());
			xmlReadingLog+="  Added "+QString::number(tmp)+" activity tags\n";
			reducedXmlLog+="Added "+QString::number(tmp)+" activity tags\n";
		}
		else if(elem2.tagName()=="Activity_Tags_List"){
			QSet<QString> activityTagsRead;
		
			int tmp=0;
			for(QDomNode node3=elem2.firstChild(); !node3.isNull(); node3=node3.nextSibling()){
				QDomElement elem3=node3.toElement();
				if(elem3.isNull()){
					xmlReadingLog+="   Null node here\n";
					continue;
				}
				xmlReadingLog+="   Found "+elem3.tagName()+" tag\n";
				if(elem3.tagName()=="Activity_Tag"){
					ActivityTag* activityTag=new ActivityTag();
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Name"){
							activityTag->name=elem4.text();
							xmlReadingLog+="    Read activity tag name: "+activityTag->name+"\n";
						}
					}
					bool tmp2=activityTagsRead.contains(activityTag->name);
					if(tmp2){
						QMessageBox::warning(NULL, tr("FET warning"),
						 tr("Duplicate activity tag %1 found - ignoring").arg(activityTag->name));
						xmlReadingLog+="   Activity tag not added - duplicate\n";
					}
					else{
						activityTagsRead.insert(activityTag->name);
						addActivityTagFast(activityTag);
						tmp++;
						xmlReadingLog+="   Activity tag inserted\n";
					}
				}
			}
			assert(tmp==this->activityTagsList.size());
			xmlReadingLog+="  Added "+QString::number(tmp)+" activity tags\n";
			reducedXmlLog+="Added "+QString::number(tmp)+" activity tags\n";
		}
		else if(elem2.tagName()=="Students_List"){
			QHash<QString, StudentsSet*> studentsHash;
		
			int tsgr=0;
			int tgr=0;
		
			int ny=0;
			for(QDomNode node3=elem2.firstChild(); !node3.isNull(); node3=node3.nextSibling()){
				QDomElement elem3=node3.toElement();
				if(elem3.isNull()){
					xmlReadingLog+="   Null node here\n";
					continue;
				}
				xmlReadingLog+="   Found "+elem3.tagName()+" tag\n";
				if(elem3.tagName()=="Year"){
					StudentsYear* sty=new StudentsYear();
					int ng=0;
					
					QSet<QString> groupsInYear;

					bool tmp2=this->addYearFast(sty);
					assert(tmp2==true);
					ny++;

					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Name"){
							if(!skipDuplicatedStudentsSets){
								QString nn=elem4.text();
								//StudentsSet* ss=this->searchStudentsSet(nn);
								StudentsSet* ss=studentsHash.value(nn, NULL);
								if(ss!=NULL){
									QString str;
									
									if(ss->type==STUDENTS_YEAR)
										str=tr("Trying to add year %1, which is already added as another year - your file will be loaded but probably contains errors, please correct them after loading").arg(nn);
									else if(ss->type==STUDENTS_GROUP)
										str=tr("Trying to add year %1, which is already added as another group - your file will be loaded but probably contains errors, please correct them after loading").arg(nn);
									else if(ss->type==STUDENTS_SUBGROUP)
										str=tr("Trying to add year %1, which is already added as another subgroup - your file will be loaded but probably contains errors, please correct them after loading").arg(nn);
								
									int t=QMessageBox::warning(NULL, tr("FET warning"), str,
									 tr("Skip rest"), tr("See next"), QString(),
									 1, 0 );
				 	
									if(t==0)
										skipDuplicatedStudentsSets=true;
								}
							}						
						
							sty->name=elem4.text();
							if(!studentsHash.contains(sty->name))
								studentsHash.insert(sty->name, sty);
							xmlReadingLog+="    Read year name: "+sty->name+"\n";
						}
						else if(elem4.tagName()=="Number_of_Students"){
							sty->numberOfStudents=elem4.text().toInt();
							xmlReadingLog+="    Read year number of students: "+QString::number(sty->numberOfStudents)+"\n";
						}
						else if(elem4.tagName()=="Group"){
							StudentsGroup* stg=new StudentsGroup();
							int ns=0;

							QSet<QString> subgroupsInGroup;
							
							/*bool tmp4=this->addGroupFast(sty, stg);
							assert(tmp4==true);
							ng++;*/

							for(QDomNode node5=elem4.firstChild(); !node5.isNull(); node5=node5.nextSibling()){
								QDomElement elem5=node5.toElement();
								if(elem5.isNull()){
									xmlReadingLog+="     Null node here\n";
									continue;
								}
								xmlReadingLog+="     Found "+elem5.tagName()+" tag\n";
								if(elem5.tagName()=="Name"){
									if(!skipDuplicatedStudentsSets){
										QString nn=elem5.text();
										StudentsSet* ss=studentsHash.value(nn, NULL);
										if(ss!=NULL){
											QString str;
									
											if(ss->type==STUDENTS_YEAR)
												str=tr("Trying to add group %1, which is already added as another year - your file will be loaded but probably contains errors, please correct them after loading").arg(nn);
											else if(ss->type==STUDENTS_GROUP){
												if(groupsInYear.contains(nn)){
													str=tr("Trying to add group %1 in year %2 but it is already added - your file will be loaded but probably contains errors, please correct them after loading").arg(nn).arg(sty->name);
												}
												else
													str="";
											}
											else if(ss->type==STUDENTS_SUBGROUP)
												str=tr("Trying to add group %1, which is already added as another subgroup - your file will be loaded but probably contains errors, please correct them after loading").arg(nn);
								
											int t=1;
											if(str!=""){
												t=QMessageBox::warning(NULL, tr("FET warning"), str,
												 tr("Skip rest"), tr("See next"), QString(),
												 1, 0 );
											}
				 	
											if(t==0)
												skipDuplicatedStudentsSets=true;
										}
									}
									
									groupsInYear.insert(elem5.text());

									if(studentsHash.contains(elem5.text())){
										delete stg;
										stg=(StudentsGroup*)(studentsHash.value(elem5.text()));

										bool tmp4=this->addGroupFast(sty, stg);
										assert(tmp4==true);
										//ng++;
										break;
									}

									bool tmp4=this->addGroupFast(sty, stg);
									assert(tmp4==true);
									ng++;

									stg->name=elem5.text();
									if(!studentsHash.contains(stg->name))
										studentsHash.insert(stg->name, stg);
									xmlReadingLog+="     Read group name: "+stg->name+"\n";
								}
								else if(elem5.tagName()=="Number_of_Students"){
									stg->numberOfStudents=elem5.text().toInt();
									xmlReadingLog+="     Read group number of students: "+QString::number(stg->numberOfStudents)+"\n";
								}
								else if(elem5.tagName()=="Subgroup"){
									StudentsSubgroup* sts=new StudentsSubgroup();

									/*bool tmp6=this->addSubgroupFast(sty, stg, sts);
									assert(tmp6==true);
									ns++;*/

									for(QDomNode node6=elem5.firstChild(); !node6.isNull(); node6=node6.nextSibling()){
										QDomElement elem6=node6.toElement();
										if(elem6.isNull()){
											xmlReadingLog+="     Null node here\n";
											continue;
										}
										xmlReadingLog+="     Found "+elem6.tagName()+" tag\n";
										if(elem6.tagName()=="Name"){
											if(!skipDuplicatedStudentsSets){
												QString nn=elem6.text();
												StudentsSet* ss=studentsHash.value(nn, NULL);
												if(ss!=NULL){
													QString str;
									
													if(ss->type==STUDENTS_YEAR)
														str=tr("Trying to add subgroup %1, which is already added as another year - your file will be loaded but probably contains errors, please correct them after loading").arg(nn);
													else if(ss->type==STUDENTS_GROUP)
														str=tr("Trying to add subgroup %1, which is already added as another group - your file will be loaded but probably contains errors, please correct them after loading").arg(nn);
													else if(ss->type==STUDENTS_SUBGROUP){
														if(subgroupsInGroup.contains(nn)){
															str=tr("Trying to add subgroup %1 in year %2, group %3 but it is already added - your file will be loaded but probably contains errors, please correct them after loading").arg(nn).arg(sty->name).arg(stg->name);
														}
														else
															str="";
													}
								
													int t=1;
													if(str!=""){
														t=QMessageBox::warning(NULL, tr("FET warning"), str,
														 tr("Skip rest"), tr("See next"), QString(),
														 1, 0 );
													}
						 	
													if(t==0)
														skipDuplicatedStudentsSets=true;
												}
											}
											
											subgroupsInGroup.insert(elem6.text());

											if(studentsHash.contains(elem6.text())){
												delete sts;
												sts=(StudentsSubgroup*)(studentsHash.value(elem6.text()));

												bool tmp6=this->addSubgroupFast(sty, stg, sts);
												assert(tmp6==true);
												//ns++;
												break;
											}

											bool tmp6=this->addSubgroupFast(sty, stg, sts);
											assert(tmp6==true);
											ns++;
											
											sts->name=elem6.text();
											if(!studentsHash.contains(sts->name))
												studentsHash.insert(sts->name, sts);
											xmlReadingLog+="     Read subgroup name: "+sts->name+"\n";
										}
										else if(elem6.tagName()=="Number_of_Students"){
											sts->numberOfStudents=elem6.text().toInt();
											xmlReadingLog+="     Read subgroup number of students: "+QString::number(sts->numberOfStudents)+"\n";
										}
									}
								}
							}
							if(ns == stg->subgroupsList.size()){
								xmlReadingLog+="    Added "+QString::number(ns)+" subgroups\n";
								tsgr+=ns;
							//reducedXmlLog+="		Added "+QString::number(ns)+" subgroups\n";
							}
						}
					}
					if(ng == sty->groupsList.size()){
						xmlReadingLog+="   Added "+QString::number(ng)+" groups\n";
						tgr+=ng;
						//reducedXmlLog+="	Added "+QString::number(ng)+" groups\n";
					}
				}
			}
			xmlReadingLog+="  Added "+QString::number(ny)+" years\n";
			reducedXmlLog+="Added "+QString::number(ny)+" students years\n";
			//reducedXmlLog+="Added "+QString::number(tgr)+" students groups (see note below)\n";
			reducedXmlLog+="Added "+QString::number(tgr)+" students groups\n";
			//reducedXmlLog+="Added "+QString::number(tsgr)+" students subgroups (see note below)\n";
			reducedXmlLog+="Added "+QString::number(tsgr)+" students subgroups\n";
			assert(this->yearsList.size()==ny);

			//BEGIN test for number of students is the same in all sets with the same name
			bool reportWrongNumberOfStudents=true;
			foreach(StudentsYear* year, yearsList){
				assert(studentsHash.contains(year->name));
				StudentsSet* sy=studentsHash.value(year->name);
				if(sy->numberOfStudents!=year->numberOfStudents){
					if(reportWrongNumberOfStudents){
						QString str=tr("Minor problem found and corrected: year %1 has different number of students in two places (%2 and %3)", "%2 and %3 are number of students")
							.arg(year->name).arg(sy->numberOfStudents).arg(year->numberOfStudents)
							+
							"\n\n"+
							tr("Explanation: this is a minor problem, which appears if using overlapping students set, due to a bug in FET previous to version %1."
							" FET will now correct this problem by setting the number of students for this year, in all places where it appears,"
							" to the number that was found in the first appearance (%2). It is advisable to check the number of students for this year.")
							.arg("5.12.1").arg(sy->numberOfStudents);
						int t=QMessageBox::warning(NULL, tr("FET warning"), str,
							 tr("Skip rest"), tr("See next"), QString(),
							 1, 0 );
	
						if(t==0)
							reportWrongNumberOfStudents=false;
					}
					year->numberOfStudents=sy->numberOfStudents;
				}
				
				foreach(StudentsGroup* group, year->groupsList){
					assert(studentsHash.contains(group->name));
					StudentsSet* sg=studentsHash.value(group->name);
					if(sg->numberOfStudents!=group->numberOfStudents){
						if(reportWrongNumberOfStudents){
							QString str=tr("Minor problem found and corrected: group %1 has different number of students in two places (%2 and %3)", "%2 and %3 are number of students")
								.arg(group->name).arg(sg->numberOfStudents).arg(group->numberOfStudents)
								+
								"\n\n"+
								tr("Explanation: this is a minor problem, which appears if using overlapping students set, due to a bug in FET previous to version %1."
								" FET will now correct this problem by setting the number of students for this group, in all places where it appears,"
								" to the number that was found in the first appearance (%2). It is advisable to check the number of students for this group.")
								.arg("5.12.1").arg(sg->numberOfStudents);
							int t=QMessageBox::warning(NULL, tr("FET warning"), str,
								 tr("Skip rest"), tr("See next"), QString(),
								 1, 0 );
		
							if(t==0)
								reportWrongNumberOfStudents=false;
						}
						group->numberOfStudents=sg->numberOfStudents;
					}

					foreach(StudentsSubgroup* subgroup, group->subgroupsList){
						assert(studentsHash.contains(subgroup->name));
						StudentsSet* ss=studentsHash.value(subgroup->name);
						if(ss->numberOfStudents!=subgroup->numberOfStudents){
							if(reportWrongNumberOfStudents){
								QString str=tr("Minor problem found and corrected: subgroup %1 has different number of students in two places (%2 and %3)", "%2 and %3 are number of students")
									.arg(subgroup->name).arg(ss->numberOfStudents).arg(subgroup->numberOfStudents)
									+
									"\n\n"+
									tr("Explanation: this is a minor problem, which appears if using overlapping students set, due to a bug in FET previous to version %1."
									" FET will now correct this problem by setting the number of students for this subgroup, in all places where it appears,"
									" to the number that was found in the first appearance (%2). It is advisable to check the number of students for this subgroup.")
									.arg("5.12.1").arg(ss->numberOfStudents);
								int t=QMessageBox::warning(NULL, tr("FET warning"), str,
									 tr("Skip rest"), tr("See next"), QString(),
									 1, 0 );
			
								if(t==0)
									reportWrongNumberOfStudents=false;
							}
							subgroup->numberOfStudents=ss->numberOfStudents;
						}
					}
				}
			}
			//END test for number of students is the same in all sets with the same name
		}
		else if(elem2.tagName()=="Activities_List"){
			QSet<QString> allTeachers;
			QHash<QString, int> studentsSetsCount;
			QSet<QString> allSubjects;
			QSet<QString> allActivityTags;
			
			foreach(Teacher* tch, this->teachersList)
				allTeachers.insert(tch->name);

			foreach(Subject* sbj, this->subjectsList)
				allSubjects.insert(sbj->name);

			foreach(ActivityTag* at, this->activityTagsList)
				allActivityTags.insert(at->name);

			foreach(StudentsYear* year, this->yearsList){
				if(!studentsSetsCount.contains(year->name))
					studentsSetsCount.insert(year->name, year->numberOfStudents);
				else if(studentsSetsCount.value(year->name)!=year->numberOfStudents){
					cout<<"Mistake: year "<<qPrintable(year->name)<<" appears in more places with different number of students"<<endl;
				}

				foreach(StudentsGroup* group, year->groupsList){
					if(!studentsSetsCount.contains(group->name))
						studentsSetsCount.insert(group->name, group->numberOfStudents);
					else if(studentsSetsCount.value(group->name)!=group->numberOfStudents){
						cout<<"Mistake: group "<<qPrintable(group->name)<<" appears in more places with different number of students"<<endl;
					}
			
					foreach(StudentsSubgroup* subgroup, group->subgroupsList){
						if(!studentsSetsCount.contains(subgroup->name))
							studentsSetsCount.insert(subgroup->name, subgroup->numberOfStudents);
						else if(studentsSetsCount.value(subgroup->name)!=subgroup->numberOfStudents){
							cout<<"Mistake: subgroup "<<qPrintable(subgroup->name)<<" appears in more places with different number of students"<<endl;
						}
					}
				}
			}

			//int nchildrennodes=elem2.childNodes().length();
			
			/*QProgressDialog progress(NULL);
			progress.setLabelText(tr("Loading activities ... please wait"));
			progress.setRange(0, nchildrennodes);
			progress.setModal(true);*/
			//progress.setCancelButton(NULL);
			
			//int ttt=0;
		
			int na=0;
			for(QDomNode node3=elem2.firstChild(); !node3.isNull(); node3=node3.nextSibling()){
			
				/*progress.setValue(ttt);
				pqapplication->processEvents();
				if(progress.wasCanceled()){
					QMessageBox::information(NULL, tr("FET information"), tr("Interrupted - only partial file was loaded"));
					return true;
				}
																				
				ttt++;*/
			
				QDomElement elem3=node3.toElement();
				if(elem3.isNull()){
					xmlReadingLog+="   Null node here\n";
					continue;
				}
				xmlReadingLog+="   Found "+elem3.tagName()+" tag\n";
				if(elem3.tagName()=="Activity"){
					bool correct=true;
				
					QString tn="";
					QStringList tl;
					QString sjn="";
					QString atn="";
					QStringList atl;
					QString stn="";
					QStringList stl;
					//int p=PARITY_NOT_INITIALIZED;
					int td=-1;
					int d=-1;
					int id=-1;
					int gid=-1;
					bool ac=true;
					int nos=-1;
					bool cnos=true;
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="   Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weekly"){
							xmlReadingLog+="    Current activity is weekly - ignoring tag\n";
							//assert(p==PARITY_NOT_INITIALIZED);
							//p=PARITY_WEEKLY;
						}
						//old tag
						else if(elem4.tagName()=="Biweekly"){
							xmlReadingLog+="    Current activity is fortnightly - ignoring tag\n";
							//assert(p==PARITY_NOT_INITIALIZED);
							//p=PARITY_FORTNIGHTLY;
						}
						else if(elem4.tagName()=="Fortnightly"){
							xmlReadingLog+="    Current activity is fortnightly - ignoring tag\n";
							//assert(p==PARITY_NOT_INITIALIZED);
							//p=PARITY_FORTNIGHTLY;
						}
						else if(elem4.tagName()=="Active"){
							if(elem4.text()=="yes" || elem4.text()=="true" || elem4.text()=="1"){
								ac=true;
								xmlReadingLog+="	Current activity is active\n";
							}
							else{
								if(!(elem4.text()=="no" || elem4.text()=="false" || elem4.text()=="0")){
									QMessageBox::warning(NULL, tr("FET warning"),
									 tr("Found activity active tag which is not 'true', 'false', 'yes', 'no', '1' or '0'."
									 " The activity will be considered not active",
									 "Instructions for translators: please leave the 'true', 'false', 'yes' and 'no' fields untranslated, as they are in English"));
								}
								//assert(elem4.text()=="no" || elem4.text()=="false" || elem4.text()=="0");
								ac=false;
								xmlReadingLog+="	Current activity is not active\n";
							}
						}
						else if(elem4.tagName()=="Teacher"){
							tn=elem4.text();
							xmlReadingLog+="    Crt. activity teacher="+tn+"\n";
							tl.append(tn);
							if(!allTeachers.contains(tn))
							//if(this->searchTeacher(tn)<0)
								correct=false;
						}
						else if(elem4.tagName()=="Subject"){
							sjn=elem4.text();
							xmlReadingLog+="    Crt. activity subject="+sjn+"\n";
							if(!allSubjects.contains(sjn))
							//if(this->searchSubject(sjn)<0)
								correct=false;
						}
						else if(elem4.tagName()=="Subject_Tag"){
							atn=elem4.text();
							xmlReadingLog+="    Crt. activity activity_tag="+atn+"\n";
							if(atn!="")
								atl.append(atn);
							if(atn!="" && !allActivityTags.contains(atn))
							//if(atn!="" && this->searchActivityTag(atn)<0)
								correct=false;
						}
						else if(elem4.tagName()=="Activity_Tag"){
							atn=elem4.text();
							xmlReadingLog+="    Crt. activity activity_tag="+atn+"\n";
							if(atn!="")
								atl.append(atn);
							if(atn!="" && !allActivityTags.contains(atn))
							//if(atn!="" && this->searchActivityTag(atn)<0)
								correct=false;
						}
						else if(elem4.tagName()=="Students"){
							stn=elem4.text();
							xmlReadingLog+="    Crt. activity students+="+stn+"\n";
							stl.append(stn);
							if(!studentsSetsCount.contains(stn))
							//if(this->searchStudentsSet(stn)==NULL)
								correct=false;
						}
						else if(elem4.tagName()=="Duration"){
							d=elem4.text().toInt();
							xmlReadingLog+="    Crt. activity duration="+QString::number(d)+"\n";
						}
						else if(elem4.tagName()=="Total_Duration"){
							td=elem4.text().toInt();
							xmlReadingLog+="    Crt. activity total duration="+QString::number(td)+"\n";
						}
						else if(elem4.tagName()=="Id"){
							id=elem4.text().toInt();
							xmlReadingLog+="    Crt. activity id="+QString::number(id)+"\n";
						}
						else if(elem4.tagName()=="Activity_Group_Id"){
							gid=elem4.text().toInt();
							xmlReadingLog+="    Crt. activity group id="+QString::number(gid)+"\n";
						}
						else if(elem4.tagName()=="Number_Of_Students"){
							cnos=false;
							nos=elem4.text().toInt();
							xmlReadingLog+="    Crt. activity number of students="+QString::number(nos)+"\n";
						}
					}
					if(correct){
						assert(id>=0 && gid>=0);
						assert(d>0);
						if(td<0)
							td=d;
						
						if(cnos==true){
							assert(nos==-1);
							int _ns=0;
							foreach(QString _s, stl){
								assert(studentsSetsCount.contains(_s));
								_ns+=studentsSetsCount.value(_s);
							}
							this->addSimpleActivityRulesFast(id, gid, tl, sjn, atl, stl,
								d, td, /*p,*/ ac, /*-1, -1,*/ cnos, nos, _ns);
						}
						else{
							this->addSimpleActivity(id, gid, tl, sjn, atl, stl,
								d, td, /*p,*/ ac, /*-1, -1,*/ cnos, nos);
						}
						na++;
						xmlReadingLog+="   Added the activity\n";
					}
					else{
						xmlReadingLog+="   Activity with id ="+QString::number(id)+" contains invalid data - skipping\n";
						QMessageBox::warning(NULL, tr("FET information"), 
						 tr("Activity with id=%1 contains invalid data - skipping").arg(id));
					}
				}
			}
			xmlReadingLog+="  Added "+QString::number(na)+" activities\n";
			reducedXmlLog+="Added "+QString::number(na)+" activities\n";
		}
		else if(elem2.tagName()=="Equipments_List"){
		 	QMessageBox::warning(NULL, tr("FET warning"),
			 tr("File contains deprecated equipments list - will be ignored\n"));
		}
		else if(elem2.tagName()=="Buildings_List"){
			QSet<QString> buildingsRead;
		
			int tmp=0;
			for(QDomNode node3=elem2.firstChild(); !node3.isNull(); node3=node3.nextSibling()){
				QDomElement elem3=node3.toElement();
				if(elem3.isNull()){
					xmlReadingLog+="   Null node here\n";
					continue;
				}
				xmlReadingLog+="   Found "+elem3.tagName()+" tag\n";
				if(elem3.tagName()=="Building"){
					Building* bu=new Building();
					bu->name="";
					
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Name"){
							bu->name=elem4.text();
							xmlReadingLog+="    Read building name: "+bu->name+"\n";
						}
					}

					bool tmp2=buildingsRead.contains(bu->name);
					if(tmp2){
						QMessageBox::warning(NULL, tr("FET warning"),
						 tr("Duplicate building %1 found - ignoring").arg(bu->name));
						xmlReadingLog+="   Building not added - duplicate\n";
					}
					else{
						buildingsRead.insert(bu->name);
						addBuildingFast(bu);
						tmp++;
						xmlReadingLog+="   Building inserted\n";
					}
				}
			}
			assert(tmp==this->buildingsList.size());
			xmlReadingLog+="  Added "+QString::number(tmp)+" buildings\n";
			reducedXmlLog+="Added "+QString::number(tmp)+" buildings\n";
		}
		else if(elem2.tagName()=="Rooms_List"){
			QSet<QString> roomsRead;
		
			int tmp=0;
			for(QDomNode node3=elem2.firstChild(); !node3.isNull(); node3=node3.nextSibling()){
				QDomElement elem3=node3.toElement();
				if(elem3.isNull()){
					xmlReadingLog+="   Null node here\n";
					continue;
				}
				xmlReadingLog+="   Found "+elem3.tagName()+" tag\n";
				if(elem3.tagName()=="Room"){
					Room* rm=new Room();
					rm->name="";
					rm->capacity=MAX_ROOM_CAPACITY; //infinite, if not specified
					rm->building="";
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Name"){
							rm->name=elem4.text();
							xmlReadingLog+="    Read room name: "+rm->name+"\n";
						}
						else if(elem4.tagName()=="Type"){
							//rm->type=elem4.text();
							xmlReadingLog+="    Ignoring old tag room type:\n";
						}
						else if(elem4.tagName()=="Capacity"){
							rm->capacity=elem4.text().toInt();
							xmlReadingLog+="    Read room capacity: "+QString::number(rm->capacity)+"\n";
						}
						else if(elem4.tagName()=="Equipment"){
							//rm->addEquipment(elem4.text());
							xmlReadingLog+="    Ignoring old tag - room equipment:\n";
						}
						else if(elem4.tagName()=="Building"){
							rm->building=elem4.text();
							xmlReadingLog+="    Read room building:\n"+rm->building;
						}
					}
					bool tmp2=roomsRead.contains(rm->name);
					if(tmp2){
						QMessageBox::warning(NULL, tr("FET warning"),
						 tr("Duplicate room %1 found - ignoring").arg(rm->name));
						xmlReadingLog+="   Room not added - duplicate\n";
					}
					else{
						roomsRead.insert(rm->name);
						addRoomFast(rm);
						tmp++;
						xmlReadingLog+="   Room inserted\n";
					}
				}
			}
			assert(tmp==this->roomsList.size());
			xmlReadingLog+="  Added "+QString::number(tmp)+" rooms\n";
			reducedXmlLog+="Added "+QString::number(tmp)+" rooms\n";
		}
		else if(elem2.tagName()=="Time_Constraints_List"){
			bool reportMaxBeginningsAtSecondHourChange=true;
			bool reportMaxGapsChange=true;
			bool reportStudentsSetNotAvailableChange=true;
			bool reportTeacherNotAvailableChange=true;
			bool reportBreakChange=true;
			
			bool reportActivityPreferredTimeChange=true;
			
			bool reportActivityPreferredTimesChange=true;
			bool reportActivitiesPreferredTimesChange=true;
			
			bool reportUnspecifiedPermanentlyLockedTime=true;
			
			bool reportUnspecifiedDayOrHourPreferredStartingTime=true;
			
#if 0&0&0
			bool reportIncorrectMinDays=true;
#endif
		
			int nc=0;
			TimeConstraint *crt_constraint;
			for(QDomNode node3=elem2.firstChild(); !node3.isNull(); node3=node3.nextSibling()){
				crt_constraint=NULL;
				QDomElement elem3=node3.toElement();
				if(elem3.isNull()){
					xmlReadingLog+="   Null node here\n";
					continue;
				}
				xmlReadingLog+="   Found "+elem3.tagName()+" tag\n";
				if(elem3.tagName()=="ConstraintBasicCompulsoryTime"){
					crt_constraint=readBasicCompulsoryTime(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintTeacherNotAvailable"){
					if(reportTeacherNotAvailableChange){
						int t=QMessageBox::information(NULL, tr("FET information"),
						 tr("File contains constraint teacher not available, which is old (it was improved in FET 5.5.0), and will be converted"
						 " to the similar constraint of this type, constraint teacher not available times (a matrix)."),
						  tr("Skip rest"), tr("See next"), QString(), 1, 0 );
						if(t==0)
							reportTeacherNotAvailableChange=false;
					}

					crt_constraint=readTeacherNotAvailable(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintTeacherNotAvailableTimes"){
					crt_constraint=readTeacherNotAvailableTimes(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintTeacherMaxDaysPerWeek"){
					crt_constraint=readTeacherMaxDaysPerWeek(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintTeachersMaxDaysPerWeek"){
					crt_constraint=readTeachersMaxDaysPerWeek(elem3, xmlReadingLog);
				}

				else if(elem3.tagName()=="ConstraintTeacherMinDaysPerWeek"){
					crt_constraint=readTeacherMinDaysPerWeek(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintTeachersMinDaysPerWeek"){
					crt_constraint=readTeachersMinDaysPerWeek(elem3, xmlReadingLog);
				}

				else if(elem3.tagName()=="ConstraintTeacherIntervalMaxDaysPerWeek"){
					crt_constraint=readTeacherIntervalMaxDaysPerWeek(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintTeachersIntervalMaxDaysPerWeek"){
					crt_constraint=readTeachersIntervalMaxDaysPerWeek(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintStudentsSetIntervalMaxDaysPerWeek"){
					crt_constraint=readStudentsSetIntervalMaxDaysPerWeek(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintStudentsIntervalMaxDaysPerWeek"){
					crt_constraint=readStudentsIntervalMaxDaysPerWeek(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintStudentsSetNotAvailable"){
					if(reportStudentsSetNotAvailableChange){
						int t=QMessageBox::information(NULL, tr("FET information"),
						 tr("File contains constraint students set not available, which is old (it was improved in FET 5.5.0), and will be converted"
						 " to the similar constraint of this type, constraint students set not available times (a matrix)."),
						  tr("Skip rest"), tr("See next"), QString(), 1, 0 );
						if(t==0)
							reportStudentsSetNotAvailableChange=false;
					}

					crt_constraint=readStudentsSetNotAvailable(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintStudentsSetNotAvailableTimes"){
					crt_constraint=readStudentsSetNotAvailableTimes(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintMinNDaysBetweenActivities"){
					crt_constraint=readMinNDaysBetweenActivities(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintMinDaysBetweenActivities"){
					crt_constraint=readMinDaysBetweenActivities(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintMaxDaysBetweenActivities"){
					crt_constraint=readMaxDaysBetweenActivities(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintMinGapsBetweenActivities"){
					crt_constraint=readMinGapsBetweenActivities(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintActivitiesNotOverlapping"){
					crt_constraint=readActivitiesNotOverlapping(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintActivitiesSameStartingTime"){
					crt_constraint=readActivitiesSameStartingTime(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintActivitiesSameStartingHour"){
					crt_constraint=readActivitiesSameStartingHour(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintActivitiesSameStartingDay"){
					crt_constraint=readActivitiesSameStartingDay(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintTeachersMaxHoursDaily"){
					crt_constraint=readTeachersMaxHoursDaily(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintTeacherMaxHoursDaily"){
					crt_constraint=readTeacherMaxHoursDaily(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintTeachersMaxHoursContinuously"){
					crt_constraint=readTeachersMaxHoursContinuously(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintTeacherMaxHoursContinuously"){
					crt_constraint=readTeacherMaxHoursContinuously(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintTeacherActivityTagMaxHoursContinuously"){
					crt_constraint=readTeacherActivityTagMaxHoursContinuously(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintTeachersActivityTagMaxHoursContinuously"){
					crt_constraint=readTeachersActivityTagMaxHoursContinuously(elem3, xmlReadingLog);
				}

				else if(elem3.tagName()=="ConstraintTeacherActivityTagMaxHoursDaily"){
					crt_constraint=readTeacherActivityTagMaxHoursDaily(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintTeachersActivityTagMaxHoursDaily"){
					crt_constraint=readTeachersActivityTagMaxHoursDaily(elem3, xmlReadingLog);
				}

				else if(elem3.tagName()=="ConstraintTeachersMinHoursDaily"){
					crt_constraint=readTeachersMinHoursDaily(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintTeacherMinHoursDaily"){
					crt_constraint=readTeacherMinHoursDaily(elem3, xmlReadingLog);
				}
				else if((elem3.tagName()=="ConstraintTeachersSubgroupsMaxHoursDaily"
				 //TODO: erase the line below. It is only kept for compatibility with older versions
				 || elem3.tagName()=="ConstraintTeachersSubgroupsNoMoreThanXHoursDaily") && !skipDeprecatedConstraints){
					int t=QMessageBox::warning(NULL, tr("FET warning"),
					 tr("File contains deprecated constraint teachers subgroups max hours daily - will be ignored\n"),
					 tr("Skip rest"), tr("See next"), QString(),
					 1, 0 );
													 
					if(t==0)
						skipDeprecatedConstraints=true;
					crt_constraint=NULL;
				}
				else if(elem3.tagName()=="ConstraintStudentsNHoursDaily" && !skipDeprecatedConstraints){
					int t=QMessageBox::warning(NULL, tr("FET warning"),
					 tr("File contains deprecated constraint students n hours daily - will be ignored\n"),
					 tr("Skip rest"), tr("See next"), QString(),
					 1, 0 );
													 
					if(t==0)
						skipDeprecatedConstraints=true;
					crt_constraint=NULL;
				}
				else if(elem3.tagName()=="ConstraintStudentsSetNHoursDaily" && !skipDeprecatedConstraints){
					int t=QMessageBox::warning(NULL, tr("FET warning"),
					 tr("File contains deprecated constraint students set n hours daily - will be ignored\n"),
					 tr("Skip rest"), tr("See next"), QString(),
					 1, 0 );
													 
					if(t==0)
						skipDeprecatedConstraints=true;
					crt_constraint=NULL;
				}
				else if(elem3.tagName()=="ConstraintStudentsMaxHoursDaily"){
					crt_constraint=readStudentsMaxHoursDaily(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintStudentsSetMaxHoursDaily"){
					crt_constraint=readStudentsSetMaxHoursDaily(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintStudentsMaxHoursContinuously"){
					crt_constraint=readStudentsMaxHoursContinuously(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintStudentsSetMaxHoursContinuously"){
					crt_constraint=readStudentsSetMaxHoursContinuously(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintStudentsSetActivityTagMaxHoursContinuously"){
					crt_constraint=readStudentsSetActivityTagMaxHoursContinuously(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintStudentsActivityTagMaxHoursContinuously"){
					crt_constraint=readStudentsActivityTagMaxHoursContinuously(elem3, xmlReadingLog);
				}

				else if(elem3.tagName()=="ConstraintStudentsSetActivityTagMaxHoursDaily"){
					crt_constraint=readStudentsSetActivityTagMaxHoursDaily(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintStudentsActivityTagMaxHoursDaily"){
					crt_constraint=readStudentsActivityTagMaxHoursDaily(elem3, xmlReadingLog);
				}

				else if(elem3.tagName()=="ConstraintStudentsMinHoursDaily"){
					crt_constraint=readStudentsMinHoursDaily(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintStudentsSetMinHoursDaily"){
					crt_constraint=readStudentsSetMinHoursDaily(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintActivityPreferredTime"){
					if(reportActivityPreferredTimeChange){
						int t=QMessageBox::information(NULL, tr("FET information"),
						 tr("File contains old constraint type activity preferred time, which will be converted"
						 " to the newer similar constraint of this type, constraint activity preferred STARTING time."
						 " This improvement is done in versions 5.5.9 and above"),
						  tr("Skip rest"), tr("See next"), QString(), 1, 0 );
						if(t==0)
							reportActivityPreferredTimeChange=false;
					}
					
					crt_constraint=readActivityPreferredTime(elem3, xmlReadingLog,
						reportUnspecifiedPermanentlyLockedTime, reportUnspecifiedDayOrHourPreferredStartingTime);
				}
				
				else if(elem3.tagName()=="ConstraintActivityPreferredStartingTime"){
					crt_constraint=readActivityPreferredStartingTime(elem3, xmlReadingLog,
						reportUnspecifiedPermanentlyLockedTime, reportUnspecifiedDayOrHourPreferredStartingTime);
				}
				else if(elem3.tagName()=="ConstraintActivityEndsStudentsDay"){
					crt_constraint=readActivityEndsStudentsDay(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintActivitiesEndStudentsDay"){
					crt_constraint=readActivitiesEndStudentsDay(elem3, xmlReadingLog);
				}
				//old, with 2 and 3
				else if(elem3.tagName()=="Constraint2ActivitiesConsecutive"){
					crt_constraint=read2ActivitiesConsecutive(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="Constraint2ActivitiesGrouped"){
					crt_constraint=read2ActivitiesGrouped(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="Constraint3ActivitiesGrouped"){
					crt_constraint=read3ActivitiesGrouped(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="Constraint2ActivitiesOrdered"){
					crt_constraint=read2ActivitiesOrdered(elem3, xmlReadingLog);
				}
				//end old
				else if(elem3.tagName()=="ConstraintTwoActivitiesConsecutive"){
					crt_constraint=readTwoActivitiesConsecutive(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintTwoActivitiesGrouped"){
					crt_constraint=readTwoActivitiesGrouped(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintThreeActivitiesGrouped"){
					crt_constraint=readThreeActivitiesGrouped(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintTwoActivitiesOrdered"){
					crt_constraint=readTwoActivitiesOrdered(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintActivityEndsDay" && !skipDeprecatedConstraints ){
					int t=QMessageBox::warning(NULL, tr("FET warning"),
					 tr("File contains deprecated constraint activity ends day - will be ignored\n"),
					 tr("Skip rest"), tr("See next"), QString(),
					 1, 0 );
													 
					if(t==0)
						skipDeprecatedConstraints=true;
					crt_constraint=NULL;
				}
				else if(elem3.tagName()=="ConstraintActivityPreferredTimes"){
					if(reportActivityPreferredTimesChange){
						int t=QMessageBox::information(NULL, tr("FET information"),
						 tr("Your file contains old constraint activity preferred times, which will be converted to"
						 " new equivalent constraint activity preferred starting times. Beginning with FET-5.5.9 it is possible"
						 " to specify: 1. the starting times of an activity (constraint activity preferred starting times)"
						 " or: 2. the accepted time slots (constraint activity preferred time slots)."
						 " If what you need is type 2 of this constraint, you will have to add it by yourself from the interface."),
						  tr("Skip rest"), tr("See next"), QString(), 1, 0 );
						if(t==0)
							reportActivityPreferredTimesChange=false;
					}
					
					crt_constraint=readActivityPreferredTimes(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintActivityPreferredTimeSlots"){
					crt_constraint=readActivityPreferredTimeSlots(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintActivityPreferredStartingTimes"){
					crt_constraint=readActivityPreferredStartingTimes(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintBreak"){
					if(reportBreakChange){
						int t=QMessageBox::information(NULL, tr("FET information"),
						 tr("File contains constraint break, which is old (it was improved in FET 5.5.0), and will be converted"
						 " to the similar constraint of this type, constraint break times (a matrix)."),
						  tr("Skip rest"), tr("See next"), QString(), 1, 0 );
						if(t==0)
							reportBreakChange=false;
					}
					
					crt_constraint=readBreak(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintBreakTimes"){
					crt_constraint=readBreakTimes(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintTeachersNoGaps"){
					crt_constraint=readTeachersNoGaps(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintTeachersMaxGapsPerWeek"){
					crt_constraint=readTeachersMaxGapsPerWeek(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintTeacherMaxGapsPerWeek"){
					crt_constraint=readTeacherMaxGapsPerWeek(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintTeachersMaxGapsPerDay"){
					crt_constraint=readTeachersMaxGapsPerDay(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintTeacherMaxGapsPerDay"){
					crt_constraint=readTeacherMaxGapsPerDay(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintStudentsNoGaps"){
					if(reportMaxGapsChange){
						int t=QMessageBox::information(NULL, tr("FET information"),
						 tr("File contains constraint students no gaps, which is old (it was improved in FET 5.5.0), and will be converted"
						 " to the similar constraint of this type, constraint students max gaps per week,"
						 " with max gaps=0. If you like, you can modify this constraint to allow"
						 " more gaps per week (normally not accepted in schools)"),
						  tr("Skip rest"), tr("See next"), QString(), 1, 0 );
						if(t==0)
							reportMaxGapsChange=false;
					}
					
					crt_constraint=readStudentsNoGaps(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintStudentsSetNoGaps"){
					if(reportMaxGapsChange){
						int t=QMessageBox::information(NULL, tr("FET information"),
						 tr("File contains constraint students set no gaps, which is old (it was improved in FET 5.5.0), and will be converted"
						 " to the similar constraint of this type, constraint students set max gaps per week,"
						 " with max gaps=0. If you like, you can modify this constraint to allow"
						 " more gaps per week (normally not accepted in schools)"),
						  tr("Skip rest"), tr("See next"), QString(), 1, 0 );
						if(t==0)
							reportMaxGapsChange=false;
					}
					
					crt_constraint=readStudentsSetNoGaps(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintStudentsMaxGapsPerWeek"){
					crt_constraint=readStudentsMaxGapsPerWeek(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintStudentsSetMaxGapsPerWeek"){
					crt_constraint=readStudentsSetMaxGapsPerWeek(elem3, xmlReadingLog);
				}

				else if(elem3.tagName()=="ConstraintStudentsMaxGapsPerDay"){
					crt_constraint=readStudentsMaxGapsPerDay(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintStudentsSetMaxGapsPerDay"){
					crt_constraint=readStudentsSetMaxGapsPerDay(elem3, xmlReadingLog);
				}

				else if(elem3.tagName()=="ConstraintStudentsEarly"){
					if(reportMaxBeginningsAtSecondHourChange){
						int t=QMessageBox::information(NULL, tr("FET information"),
						 tr("File contains constraint students early, which is old (it was improved in FET 5.5.0), and will be converted"
						 " to the similar constraint of this type, constraint students early max beginnings at second hour,"
						 " with max beginnings=0. If you like, you can modify this constraint to allow"
						 " more beginnings at second available hour (above 0 - this will make the timetable easier)"),
						  tr("Skip rest"), tr("See next"), QString(), 1, 0 );
 						if(t==0)
							reportMaxBeginningsAtSecondHourChange=false;
					}
					
					crt_constraint=readStudentsEarly(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintStudentsEarlyMaxBeginningsAtSecondHour"){
					crt_constraint=readStudentsEarlyMaxBeginningsAtSecondHour(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintStudentsSetEarly"){
					if(reportMaxBeginningsAtSecondHourChange){
						int t=QMessageBox::information(NULL, tr("FET information"),
						 tr("File contains constraint students set early, which is old (it was improved in FET 5.5.0), and will be converted"
						 " to the similar constraint of this type, constraint students set early max beginnings at second hour,"
						 " with max beginnings=0. If you like, you can modify this constraint to allow"
						 " more beginnings at second available hour (above 0 - this will make the timetable easier)"),
						  tr("Skip rest"), tr("See next"), QString(), 1, 0 );
						if(t==0)
							reportMaxBeginningsAtSecondHourChange=false;
					}
					
					crt_constraint=readStudentsSetEarly(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintStudentsSetEarlyMaxBeginningsAtSecondHour"){
					crt_constraint=readStudentsSetEarlyMaxBeginningsAtSecondHour(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintActivitiesPreferredTimes"){
					if(reportActivitiesPreferredTimesChange){
						int t=QMessageBox::information(NULL, tr("FET information"),
						 tr("Your file contains old constraint activities preferred times, which will be converted to"
						 " new equivalent constraint activities preferred starting times. Beginning with FET-5.5.9 it is possible"
						 " to specify: 1. the starting times of several activities (constraint activities preferred starting times)"
						 " or: 2. the accepted time slots (constraint activities preferred time slots)."
						 " If what you need is type 2 of this constraint, you will have to add it by yourself from the interface."),
						  tr("Skip rest"), tr("See next"), QString(), 1, 0 );
						if(t==0)
							reportActivitiesPreferredTimesChange=false;
					}
					
					crt_constraint=readActivitiesPreferredTimes(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintActivitiesPreferredTimeSlots"){
					crt_constraint=readActivitiesPreferredTimeSlots(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintActivitiesPreferredStartingTimes"){
					crt_constraint=readActivitiesPreferredStartingTimes(elem3, xmlReadingLog);
				}
////////////////
				else if(elem3.tagName()=="ConstraintSubactivitiesPreferredTimeSlots"){
					crt_constraint=readSubactivitiesPreferredTimeSlots(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintSubactivitiesPreferredStartingTimes"){
					crt_constraint=readSubactivitiesPreferredStartingTimes(elem3, xmlReadingLog);
				}
////////////////

				else if(elem3.tagName()=="ConstraintTeachersSubjectTagsMaxHoursContinuously" && !skipDeprecatedConstraints){
					int t=QMessageBox::warning(NULL, tr("FET warning"),
					 tr("File contains deprecated constraint teachers subject tags max hours continuously - will be ignored\n"),
					 tr("Skip rest"), tr("See next"), QString(),
					 1, 0 );
													 
					if(t==0)
						skipDeprecatedConstraints=true;
					crt_constraint=NULL;
				}
				else if(elem3.tagName()=="ConstraintTeachersSubjectTagMaxHoursContinuously" && !skipDeprecatedConstraints){
					int t=QMessageBox::warning(NULL, tr("FET warning"),
					 tr("File contains deprecated constraint teachers subject tag max hours continuously - will be ignored\n"),
					 tr("Skip rest"), tr("See next"), QString(),
					 1, 0 );
													 
					if(t==0)
						skipDeprecatedConstraints=true;
					crt_constraint=NULL;
				}

//corruptConstraintTime:
				//here we skip invalid constraint or add valid one
				if(crt_constraint!=NULL){
					assert(crt_constraint!=NULL);
					bool tmp=this->addTimeConstraint(crt_constraint);
					if(!tmp){
						QMessageBox::warning(NULL, tr("FET information"),
						 tr("Constraint\n%1\nnot added - must be a duplicate").
						 arg(crt_constraint->getDetailedDescription(*this)));
						delete crt_constraint;
					}
					else
						nc++;
				}
			}
			xmlReadingLog+="  Added "+QString::number(nc)+" time constraints\n";
			reducedXmlLog+="Added "+QString::number(nc)+" time constraints\n";
		}
		else if(elem2.tagName()=="Space_Constraints_List"){
			bool reportRoomNotAvailableChange=true;

			bool reportUnspecifiedPermanentlyLockedSpace=true;

			int nc=0;
			SpaceConstraint *crt_constraint;
			
			for(QDomNode node3=elem2.firstChild(); !node3.isNull(); node3=node3.nextSibling()){
				crt_constraint=NULL;
				QDomElement elem3=node3.toElement();
				if(elem3.isNull()){
					xmlReadingLog+="   Null node here\n";
					continue;
				}
				xmlReadingLog+="   Found "+elem3.tagName()+" tag\n";
				if(elem3.tagName()=="ConstraintBasicCompulsorySpace"){
					crt_constraint=readBasicCompulsorySpace(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintRoomNotAvailable"){
					if(reportRoomNotAvailableChange){
						int t=QMessageBox::information(NULL, tr("FET information"),
						 tr("File contains constraint room not available, which is old (it was improved in FET 5.5.0), and will be converted"
						 " to the similar constraint of this type, constraint room not available times (a matrix)."),
						  tr("Skip rest"), tr("See next"), QString(), 1, 0 );
						if(t==0)
							reportRoomNotAvailableChange=false;
					}
					
					crt_constraint=readRoomNotAvailable(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintRoomNotAvailableTimes"){
					crt_constraint=readRoomNotAvailableTimes(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintRoomTypeNotAllowedSubjects" && !skipDeprecatedConstraints){
				
					int t=QMessageBox::warning(NULL, tr("FET warning"),
					 tr("File contains deprecated constraint room type not allowed subjects - will be ignored\n"),
					 tr("Skip rest"), tr("See next"), QString(),
					 1, 0);
					 
					if(t==0)
						skipDeprecatedConstraints=true;
					crt_constraint=NULL;
				}
				else if(elem3.tagName()=="ConstraintSubjectRequiresEquipments" && !skipDeprecatedConstraints){
				
					int t=QMessageBox::warning(NULL, tr("FET warning"),
					 tr("File contains deprecated constraint subject requires equipments - will be ignored\n"),
					 tr("Skip rest"), tr("See next"), QString(),
					 1, 0);
					 
					if(t==0)
						skipDeprecatedConstraints=true;
				
					crt_constraint=NULL;
				}
				else if(elem3.tagName()=="ConstraintSubjectSubjectTagRequireEquipments" && !skipDeprecatedConstraints){
				
					int t=QMessageBox::warning(NULL, tr("FET warning"),
					 tr("File contains deprecated constraint subject tag requires equipments - will be ignored\n"),
					 tr("Skip rest"), tr("See next"), QString(),
					 1, 0);
					 
					if(t==0)
						skipDeprecatedConstraints=true;
					crt_constraint=NULL;
				}
				else if(elem3.tagName()=="ConstraintTeacherRequiresRoom" && !skipDeprecatedConstraints){
				
					int t=QMessageBox::warning(NULL, tr("FET warning"),
					 tr("File contains deprecated constraint teacher requires room - will be ignored\n"),
					 tr("Skip rest"), tr("See next"), QString(),
					 1, 0);
					 
					if(t==0)
						skipDeprecatedConstraints=true;
					crt_constraint=NULL;
				}
				else if(elem3.tagName()=="ConstraintTeacherSubjectRequireRoom" && !skipDeprecatedConstraints){
				
					int t=QMessageBox::warning(NULL, tr("FET warning"),
					 tr("File contains deprecated constraint teacher subject require room - will be ignored\n"),
					 tr("Skip rest"), tr("See next"), QString(),
					 1, 0);
					 
					if(t==0)
						skipDeprecatedConstraints=true;
					crt_constraint=NULL;
				}
				else if(elem3.tagName()=="ConstraintMinimizeNumberOfRoomsForStudents" && !skipDeprecatedConstraints){
				
					int t=QMessageBox::warning(NULL, tr("FET warning"),
					 tr("File contains deprecated constraint minimize number of rooms for students - will be ignored\n"),
					 tr("Skip rest"), tr("See next"), QString(),
					 1, 0);
					 
					if(t==0)
						skipDeprecatedConstraints=true;
					crt_constraint=NULL;
				}
				else if(elem3.tagName()=="ConstraintMinimizeNumberOfRoomsForTeachers" && !skipDeprecatedConstraints){
				
					int t=QMessageBox::warning(NULL, tr("FET warning"),
					 tr("File contains deprecated constraint minimize number of rooms for teachers - will be ignored\n"),
					 tr("Skip rest"), tr("See next"), QString(),
					 1, 0);
					 
					if(t==0)
						skipDeprecatedConstraints=true;
					crt_constraint=NULL;
				}
				else if(elem3.tagName()=="ConstraintActivityPreferredRoom"){
					crt_constraint=readActivityPreferredRoom(elem3, xmlReadingLog, reportUnspecifiedPermanentlyLockedSpace);
				}
				else if(elem3.tagName()=="ConstraintActivityPreferredRooms"){
					crt_constraint=readActivityPreferredRooms(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintActivitiesSameRoom" && !skipDeprecatedConstraints){
				
					int t=QMessageBox::warning(NULL, tr("FET warning"),
					 tr("File contains deprecated constraint activities same room - will be ignored\n"),
					 tr("Skip rest"), tr("See next"), QString(),
					 1, 0);
					 
					if(t==0)
						skipDeprecatedConstraints=true;
					crt_constraint=NULL;
				}
				else if(elem3.tagName()=="ConstraintSubjectPreferredRoom"){
					crt_constraint=readSubjectPreferredRoom(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintSubjectPreferredRooms"){
					crt_constraint=readSubjectPreferredRooms(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintSubjectSubjectTagPreferredRoom"){
					crt_constraint=readSubjectSubjectTagPreferredRoom(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintSubjectSubjectTagPreferredRooms"){
					crt_constraint=readSubjectSubjectTagPreferredRooms(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintSubjectActivityTagPreferredRoom"){
					crt_constraint=readSubjectActivityTagPreferredRoom(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintSubjectActivityTagPreferredRooms"){
					crt_constraint=readSubjectActivityTagPreferredRooms(elem3, xmlReadingLog);
				}
				//added 6 apr 2009
				else if(elem3.tagName()=="ConstraintActivityTagPreferredRoom"){
					crt_constraint=readActivityTagPreferredRoom(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintActivityTagPreferredRooms"){
					crt_constraint=readActivityTagPreferredRooms(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintStudentsSetHomeRoom"){
					crt_constraint=readStudentsSetHomeRoom(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintStudentsSetHomeRooms"){
					crt_constraint=readStudentsSetHomeRooms(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintTeacherHomeRoom"){
					crt_constraint=readTeacherHomeRoom(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintTeacherHomeRooms"){
					crt_constraint=readTeacherHomeRooms(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintMaxBuildingChangesPerDayForTeachers" && !skipDeprecatedConstraints){
				
					int t=QMessageBox::warning(NULL, tr("FET warning"),
					 tr("File contains deprecated constraint max building changes per day for teachers - will be ignored\n"),
					 tr("Skip rest"), tr("See next"), QString(),
					 1, 0);
					 
					if(t==0)
						skipDeprecatedConstraints=true;
					crt_constraint=NULL;
				}
				else if(elem3.tagName()=="ConstraintMaxBuildingChangesPerDayForStudents" && !skipDeprecatedConstraints){
				
					int t=QMessageBox::warning(NULL, tr("FET warning"),
					 tr("File contains deprecated constraint max building changes per day for students - will be ignored\n"),
					 tr("Skip rest"), tr("See next"), QString(),
					 1, 0);
					 
					if(t==0)
						skipDeprecatedConstraints=true;
					crt_constraint=NULL;
				}
				else if(elem3.tagName()=="ConstraintMaxRoomChangesPerDayForTeachers" && !skipDeprecatedConstraints){
				
					int t=QMessageBox::warning(NULL, tr("FET warning"),
					 tr("File contains deprecated constraint max room changes per day for teachers - will be ignored\n"),
					 tr("Skip rest"), tr("See next"), QString(),
					 1, 0);
					 
					if(t==0)
						skipDeprecatedConstraints=true;
					crt_constraint=NULL;
				}
				else if(elem3.tagName()=="ConstraintMaxRoomChangesPerDayForStudents" && !skipDeprecatedConstraints){
				
					int t=QMessageBox::warning(NULL, tr("FET warning"),
					 tr("File contains deprecated constraint max room changes per day for students - will be ignored\n"),
					 tr("Skip rest"), tr("See next"), QString(),
					 1, 0);
					 
					if(t==0)
						skipDeprecatedConstraints=true;

					crt_constraint=NULL;
				}
				else if(elem3.tagName()=="ConstraintTeacherMaxBuildingChangesPerDay"){
					crt_constraint=readTeacherMaxBuildingChangesPerDay(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintTeachersMaxBuildingChangesPerDay"){
					crt_constraint=readTeachersMaxBuildingChangesPerDay(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintTeacherMaxBuildingChangesPerWeek"){
					crt_constraint=readTeacherMaxBuildingChangesPerWeek(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintTeachersMaxBuildingChangesPerWeek"){
					crt_constraint=readTeachersMaxBuildingChangesPerWeek(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintTeacherMinGapsBetweenBuildingChanges"){
					crt_constraint=readTeacherMinGapsBetweenBuildingChanges(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintTeachersMinGapsBetweenBuildingChanges"){
					crt_constraint=readTeachersMinGapsBetweenBuildingChanges(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintStudentsSetMaxBuildingChangesPerDay"){
					crt_constraint=readStudentsSetMaxBuildingChangesPerDay(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintStudentsMaxBuildingChangesPerDay"){
					crt_constraint=readStudentsMaxBuildingChangesPerDay(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintStudentsSetMaxBuildingChangesPerWeek"){
					crt_constraint=readStudentsSetMaxBuildingChangesPerWeek(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintStudentsMaxBuildingChangesPerWeek"){
					crt_constraint=readStudentsMaxBuildingChangesPerWeek(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintStudentsSetMinGapsBetweenBuildingChanges"){
					crt_constraint=readStudentsSetMinGapsBetweenBuildingChanges(elem3, xmlReadingLog);
				}
				else if(elem3.tagName()=="ConstraintStudentsMinGapsBetweenBuildingChanges"){
					crt_constraint=readStudentsMinGapsBetweenBuildingChanges(elem3, xmlReadingLog);
				}

//corruptConstraintSpace:
				//here we skip invalid constraint or add valid one
				if(crt_constraint!=NULL){
					assert(crt_constraint!=NULL);
					
					bool tmp=this->addSpaceConstraint(crt_constraint);
					if(!tmp){
						QMessageBox::warning(NULL, tr("FET information"),
						 tr("Constraint\n%1\nnot added - must be a duplicate").
						 arg(crt_constraint->getDetailedDescription(*this)));
						delete crt_constraint;
					}
					else
						nc++;
				}
			}
			xmlReadingLog+="  Added "+QString::number(nc)+" space constraints\n";
			reducedXmlLog+="Added "+QString::number(nc)+" space constraints\n";
		}
	}

	this->internalStructureComputed=false;
	
	/*reducedXmlLog+="\n";
	reducedXmlLog+="Note: if you have overlapping students sets (years or groups), a group or a subgroup may be counted more than once. "
		"A unique group name is counted once for each year it belongs to and a unique subgroup name is counted once for each year+group it belongs to.\n";*/

	if(canWriteLogFile){
		//logStream<<xmlReadingLog;
		logStream<<reducedXmlLog;
	}

	if(file2.error()>0){
		QMessageBox::critical(NULL, tr("FET critical"),
		 tr("Saving of logging gave error code %1, which means you cannot see the log of reading the file. Please check your disk free space")
		 .arg(file2.error()));
	}

	if(canWriteLogFile)
		file2.close();

	////////////////////////////////////////

	return true;
}

bool Rules::write(const QString& filename)
{
	assert(this->initialized);

	QString s;

	QFile file(filename);
	if(!file.open(QIODevice::WriteOnly)){
		QMessageBox::critical(NULL, tr("FET critical"),
		 tr("Cannot open filename for writing ... please check write permissions of the selected directory or your disk free space. Saving of file aborted"));
		 
		return false;
	
		assert(0);
		exit(1);
	}

	QTextStream tos(&file);
	
	tos.setCodec("UTF-8");
	tos.setGenerateByteOrderMark(true);
	//tos.setEncoding(QTextStream::UnicodeUTF8);
	
	s+="<?xml version=\"1.0\" encoding=\"utf-8\"?>\n\n";

//	s+="<!DOCTYPE FET><FET version=\""+FET_VERSION+"\">\n\n";
	s+="<fet version=\""+FET_VERSION+"\">\n\n";
	
	//the institution name and comments
	s+="<Institution_Name>"+protect(this->institutionName)+"</Institution_Name>\n\n";
	s+="<Comments>"+protect(this->comments)+"</Comments>\n\n";

	//the hours and days
	s+="<Hours_List>\n	<Number>"+QString::number(this->nHoursPerDay)+"</Number>\n";
	for(int i=0; i<this->nHoursPerDay; i++)
		s+="	<Name>"+protect(this->hoursOfTheDay[i])+"</Name>\n";
	s+="</Hours_List>\n\n";
	s+="<Days_List>\n	<Number>"+QString::number(this->nDaysPerWeek)+"</Number>\n";
	for(int i=0; i<this->nDaysPerWeek; i++)
		s+="	<Name>"+protect(this->daysOfTheWeek[i])+"</Name>\n";
	s+="</Days_List>\n\n";

	//students list
	s+="<Students_List>\n";
	for(int i=0; i<this->yearsList.size(); i++){
		StudentsYear* sty=this->yearsList[i];
		s += sty->getXmlDescription();
	}
	s+="</Students_List>\n\n";

	//teachers list
	s += "<Teachers_List>\n";
	for(int i=0; i<this->teachersList.size(); i++){
		Teacher* tch=this->teachersList[i];
		s += tch->getXmlDescription();
	}
	s += "</Teachers_List>\n\n";

	//subjects list
	s += "<Subjects_List>\n";
	for(int i=0; i<this->subjectsList.size(); i++){
		Subject* sbj=this->subjectsList[i];
		s += sbj->getXmlDescription();
	}
	s += "</Subjects_List>\n\n";

	//activity tags list
	s += "<Activity_Tags_List>\n";
	for(int i=0; i<this->activityTagsList.size(); i++){
		ActivityTag* stg=this->activityTagsList[i];
		s += stg->getXmlDescription();
	}
	s += "</Activity_Tags_List>\n\n";

	//activities list
	s += "<Activities_List>\n";
	for(int i=0; i<this->activitiesList.size(); i++){
		Activity* act=this->activitiesList[i];
		s += act->getXmlDescription(*this);
		s += "\n";
	}
	s += "</Activities_List>\n\n";

	//buildings list
	s += "<Buildings_List>\n";
	for(int i=0; i<this->buildingsList.size(); i++){
		Building* bu=this->buildingsList[i];
		s += bu->getXmlDescription();
	}
	s += "</Buildings_List>\n\n";

	//rooms list
	s += "<Rooms_List>\n";
	for(int i=0; i<this->roomsList.size(); i++){
		Room* rm=this->roomsList[i];
		s += rm->getXmlDescription();
	}
	s += "</Rooms_List>\n\n";

	//time constraints list
	s += "<Time_Constraints_List>\n";
	for(int i=0; i<this->timeConstraintsList.size(); i++){
		TimeConstraint* ctr=this->timeConstraintsList[i];
		s += ctr->getXmlDescription(*this);
	}
	s += "</Time_Constraints_List>\n\n";

	//constraints list
	s += "<Space_Constraints_List>\n";
	for(int i=0; i<this->spaceConstraintsList.size(); i++){
		SpaceConstraint* ctr=this->spaceConstraintsList[i];
		s += ctr->getXmlDescription(*this);
	}
	s += "</Space_Constraints_List>\n\n";

//	s+="</FET>\n";
	s+="</fet>\n";

	tos<<s;
	
	if(file.error()>0){
		QMessageBox::critical(NULL, tr("FET critical"),
		 tr("Saved file gave error code %1, which means saving is compromised. Please check your disk free space")
		 .arg(file.error()));
		 
		return false;
	}
	
	return true;
}

int Rules::activateTeacher(const QString& teacherName)
{
	int count=0;
	for(int i=0; i<this->activitiesList.size(); i++){
		Activity* act=this->activitiesList[i];
		if(act->searchTeacher(teacherName)){
			if(!act->active)
				count++;
			act->active=true;
		}
	}

	this->internalStructureComputed=false;
	
	return count;
}

int Rules::activateStudents(const QString& studentsName)
{
	QSet<QString> allSets;
	
	StudentsSet* set=this->searchStudentsSet(studentsName);
	if(set->type==STUDENTS_SUBGROUP)
		allSets.insert(studentsName);
	else if(set->type==STUDENTS_GROUP){
		allSets.insert(studentsName);
		StudentsGroup* g=(StudentsGroup*)set;
		foreach(StudentsSubgroup* s, g->subgroupsList)
			allSets.insert(s->name);
	}
	else if(set->type==STUDENTS_YEAR){
		allSets.insert(studentsName);
		StudentsYear* y=(StudentsYear*)set;
		foreach(StudentsGroup* g, y->groupsList){
			allSets.insert(g->name);
			foreach(StudentsSubgroup* s, g->subgroupsList)
				allSets.insert(s->name);
		}
	}

	int count=0;
	for(int i=0; i<this->activitiesList.size(); i++){
		Activity* act=this->activitiesList[i];
		if(!act->active){
			foreach(QString set, act->studentsNames){
				if(allSets.contains(set)){
					count++;
					act->active=true;
					break;
				}
			}
		}
	}

	this->internalStructureComputed=false;
	
	return count;
}

int Rules::activateSubject(const QString& subjectName)
{
	int count=0;
	for(int i=0; i<this->activitiesList.size(); i++){
		Activity* act=this->activitiesList[i];
		if(act->subjectName==subjectName){
			if(!act->active)
				count++;
			act->active=true;
		}
	}

	this->internalStructureComputed=false;
	
	return count;
}

int Rules::activateActivityTag(const QString& activityTagName)
{
	int count=0;
	for(int i=0; i<this->activitiesList.size(); i++){
		Activity* act=this->activitiesList[i];
		//if(act->activityTagName==activityTagName){
		if(act->activityTagsNames.contains(activityTagName)){
			if(!act->active)
				count++;
			act->active=true;
		}
	}

	this->internalStructureComputed=false;
	
	return count;
}

int Rules::deactivateTeacher(const QString& teacherName)
{
	int count=0;
	for(int i=0; i<this->activitiesList.size(); i++){
		Activity* act=this->activitiesList[i];
		if(act->searchTeacher(teacherName)){
			if(act->active)
				count++;
			act->active=false;
		}
	}

	this->internalStructureComputed=false;
	
	return count;
}

int Rules::deactivateStudents(const QString& studentsName)
{
	QSet<QString> allSets;
	
	StudentsSet* set=this->searchStudentsSet(studentsName);
	if(set->type==STUDENTS_SUBGROUP)
		allSets.insert(studentsName);
	else if(set->type==STUDENTS_GROUP){
		allSets.insert(studentsName);
		StudentsGroup* g=(StudentsGroup*)set;
		foreach(StudentsSubgroup* s, g->subgroupsList)
			allSets.insert(s->name);
	}
	else if(set->type==STUDENTS_YEAR){
		allSets.insert(studentsName);
		StudentsYear* y=(StudentsYear*)set;
		foreach(StudentsGroup* g, y->groupsList){
			allSets.insert(g->name);
			foreach(StudentsSubgroup* s, g->subgroupsList)
				allSets.insert(s->name);
		}
	}

	int count=0;
	for(int i=0; i<this->activitiesList.size(); i++){
		Activity* act=this->activitiesList[i];
		if(act->active){
			foreach(QString set, act->studentsNames){
				if(allSets.contains(set)){
					count++;
					act->active=false;
					break;
				}
			}
		}
	}

	this->internalStructureComputed=false;
	
	return count;
}

int Rules::deactivateSubject(const QString& subjectName)
{
	int count=0;
	for(int i=0; i<this->activitiesList.size(); i++){
		Activity* act=this->activitiesList[i];
		if(act->subjectName==subjectName){
			if(act->active)
				count++;
			act->active=false;
		}
	}

	this->internalStructureComputed=false;
	
	return count;
}

int Rules::deactivateActivityTag(const QString& activityTagName)
{
	int count=0;
	for(int i=0; i<this->activitiesList.size(); i++){
		Activity* act=this->activitiesList[i];
		//if(act->activityTagName==activityTagName){
		if(act->activityTagsNames.contains(activityTagName)){
			if(act->active)
				count++;
			act->active=false;
		}
	}

	this->internalStructureComputed=false;
	
	return count;
}


TimeConstraint* Rules::readBasicCompulsoryTime(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintBasicCompulsoryTime");
					ConstraintBasicCompulsoryTime* cn=new ConstraintBasicCompulsoryTime();
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight"){
							//cn->weight=elem4.text().toDouble();
							xmlReadingLog+="    Ignoring old tag - weight - generating automatic 100% weight percentage\n";
							cn->weightPercentage=100;
						}
						else if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Compulsory"){
							if(elem4.text()=="yes"){
								//cn->compulsory=true;
								xmlReadingLog+="    Ignoring old tag - Current constraint is compulsory\n";
								cn->weightPercentage=100;
							}
							else{
								//cn->compulsory=false;
								xmlReadingLog+="    Old tag - current constraint is not compulsory - making weightPercentage=0%\n";
								cn->weightPercentage=0;
							}
						}
					}
					return cn;
}

TimeConstraint* Rules::readTeacherNotAvailable(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintTeacherNotAvailable");

					QList<int> days;
					QList<int> hours;
					QString teacher;
					double weightPercentage=100;
					int d=-1, h1=-1, h2=-1;
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight_Percentage"){
							weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Read weight percentage="+QString::number(weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Day"){
							for(d=0; d<this->nDaysPerWeek; d++)
								if(this->daysOfTheWeek[d]==elem4.text())
									break;
							if(d>=this->nDaysPerWeek){
								QMessageBox::information(NULL, tr("FET information"), 
								 tr("Constraint TeacherNotAvailable day corrupt for teacher %1, day %2 is inexistent ... ignoring constraint")
								 .arg(teacher)
								 .arg(elem4.text()));
								//cn=NULL;
								
								return NULL;
								//goto corruptConstraintTime;
							}
							assert(d<this->nDaysPerWeek);
							xmlReadingLog+="    Crt. day="+this->daysOfTheWeek[d]+"\n";
						}
						else if(elem4.tagName()=="Start_Hour"){
							for(h1=0; h1 < this->nHoursPerDay; h1++)
								if(this->hoursOfTheDay[h1]==elem4.text())
									break;
							if(h1>=this->nHoursPerDay){
								QMessageBox::information(NULL, tr("FET information"), 
								 tr("Constraint TeacherNotAvailable start hour corrupt for teacher %1, hour %2 is inexistent ... ignoring constraint")
								 .arg(teacher)
								 .arg(elem4.text()));
								//cn=NULL;
								
								return NULL;
								//goto corruptConstraintTime;
							}
							assert(h1>=0 && h1 < this->nHoursPerDay);
							xmlReadingLog+="    Start hour="+this->hoursOfTheDay[h1]+"\n";
						}
						else if(elem4.tagName()=="End_Hour"){
							for(h2=0; h2 < this->nHoursPerDay; h2++)
								if(this->hoursOfTheDay[h2]==elem4.text())
									break;
							if(h2<=0 || h2>this->nHoursPerDay){
								QMessageBox::information(NULL, tr("FET information"), 
								 tr("Constraint TeacherNotAvailable end hour corrupt for teacher %1, hour %2 is inexistent ... ignoring constraint")
								 .arg(teacher)
								 .arg(elem4.text()));
								 
								return NULL;
								//goto corruptConstraintTime;
							}
							assert(h2>0 && h2 <= this->nHoursPerDay);
							xmlReadingLog+="    End hour="+this->hoursOfTheDay[h2]+"\n";
						}
						else if(elem4.tagName()=="Teacher_Name"){
							teacher=elem4.text();
							xmlReadingLog+="    Read teacher name="+teacher+"\n";
						}
					}
					
					assert(weightPercentage>=0);
					assert(d>=0 && h1>=0 && h2>=0);
					
					ConstraintTeacherNotAvailableTimes* cn = NULL;
					
					bool found=false;
					foreach(TimeConstraint* c, this->timeConstraintsList)
						if(c->type==CONSTRAINT_TEACHER_NOT_AVAILABLE_TIMES){
							ConstraintTeacherNotAvailableTimes* tna=(ConstraintTeacherNotAvailableTimes*) c;
							if(tna->teacher==teacher){
								found=true;
								
								for(int hh=h1; hh<h2; hh++){
									int k;
									for(k=0; k<tna->days.count(); k++)
										if(tna->days.at(k)==d && tna->hours.at(k)==hh)
											break;
									if(k==tna->days.count()){
										tna->days.append(d);
										tna->hours.append(hh);
									}
								}
								
								assert(tna->days.count()==tna->hours.count());
							}
						}
					if(!found){
						days.clear();
						hours.clear();
						for(int hh=h1; hh<h2; hh++){
							days.append(d);
							hours.append(hh);
						}
					
						cn=new ConstraintTeacherNotAvailableTimes(weightPercentage, teacher, days, hours);

						return cn;
					}
					else
						return NULL;
}
				
TimeConstraint* Rules::readTeacherNotAvailableTimes(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintTeacherNotAvailableTimes");
					
					ConstraintTeacherNotAvailableTimes* cn=new ConstraintTeacherNotAvailableTimes();
					int nNotAvailableSlots=-1;
					int i=0;
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Read weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}

						else if(elem4.tagName()=="Number_of_Not_Available_Times"){
							nNotAvailableSlots=elem4.text().toInt();
							xmlReadingLog+="    Read number of not available times="+QString::number(nNotAvailableSlots)+"\n";
						}

						else if(elem4.tagName()=="Not_Available_Time"){
							xmlReadingLog+="    Read: not available time\n";
							
							int d=-1;
							int h=-1;

							for(QDomNode node5=elem4.firstChild(); !node5.isNull(); node5=node5.nextSibling()){
								QDomElement elem5=node5.toElement();
								if(elem5.isNull()){
									xmlReadingLog+="    Null node here\n";
									continue;
								}
								xmlReadingLog+="    Found "+elem5.tagName()+" tag\n";
								if(elem5.tagName()=="Day"){
									for(d=0; d<this->nDaysPerWeek; d++)
										if(this->daysOfTheWeek[d]==elem5.text())
											break;

									if(d>=this->nDaysPerWeek){
										QMessageBox::information(NULL, tr("FET information"), 
										 tr("Constraint TeacherNotAvailableTimes day corrupt for teacher %1, day %2 is inexistent ... ignoring constraint")
										 .arg(cn->teacher)
										 .arg(elem5.text()));
										delete cn;
										cn=NULL;
										//goto corruptConstraintTime;
										return NULL;
									}
						
									assert(d<this->nDaysPerWeek);
									xmlReadingLog+="    Day="+this->daysOfTheWeek[d]+"("+QString::number(i)+")"+"\n";
								}
								else if(elem5.tagName()=="Hour"){
									for(h=0; h < this->nHoursPerDay; h++)
										if(this->hoursOfTheDay[h]==elem5.text())
											break;
									
									if(h>=this->nHoursPerDay){
										QMessageBox::information(NULL, tr("FET information"), 
										 tr("Constraint TeacherNotAvailableTimes hour corrupt for teacher %1, hour %2 is inexistent ... ignoring constraint")
										 .arg(cn->teacher)
										 .arg(elem5.text()));
										delete cn;
										cn=NULL;
										//goto corruptConstraintTime;
										return NULL;
									}
									
									assert(h>=0 && h < this->nHoursPerDay);
									xmlReadingLog+="    Hour="+this->hoursOfTheDay[h]+"\n";
								}
							}
							i++;
							
							cn->days.append(d);
							cn->hours.append(h);
						}
						else if(elem4.tagName()=="Teacher"){
							cn->teacher=elem4.text();
							xmlReadingLog+="    Read teacher name="+cn->teacher+"\n";
						}
					}
					assert(i==cn->days.count() && i==cn->hours.count());
					assert(i==nNotAvailableSlots);
					return cn;
}

TimeConstraint* Rules::readTeacherMaxDaysPerWeek(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintTeacherMaxDaysPerWeek");
					
					ConstraintTeacherMaxDaysPerWeek* cn=new ConstraintTeacherMaxDaysPerWeek();
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight"){
							//cn->weight=elem4.text().toDouble();
							xmlReadingLog+="    Ignoring old tag - weight - generating 100% weight percentage\n";
							cn->weightPercentage=100;
						}
						else if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Compulsory"){
							if(elem4.text()=="yes"){
								//cn->compulsory=true;
								xmlReadingLog+="    Ignoring old tag - Current constraint is compulsory\n";
								cn->weightPercentage=100;
							}
							else{
								//cn->compulsory=false;
								xmlReadingLog+="    Old tag - current constraint is not compulsory - making weightPercentage=0%\n";
								cn->weightPercentage=0;
							}
						}
						else if(elem4.tagName()=="Teacher_Name"){
							cn->teacherName=elem4.text();
							xmlReadingLog+="    Read teacher name="+cn->teacherName+"\n";
						}
						else if(elem4.tagName()=="Max_Days_Per_Week"){
							cn->maxDaysPerWeek=elem4.text().toInt();
							if(cn->maxDaysPerWeek<=0 || cn->maxDaysPerWeek>this->nDaysPerWeek){
								QMessageBox::information(NULL, tr("FET information"), 
								 tr("Constraint TeacherMaxDaysPerWeek day corrupt for teacher %1, max days %2 <= 0 or >nDaysPerWeek, ignoring constraint")
								 .arg(cn->teacherName)
								 .arg(elem4.text()));
								delete cn;
								cn=NULL;
								return NULL;
							}
							assert(cn->maxDaysPerWeek>0 && cn->maxDaysPerWeek <= this->nDaysPerWeek);
							xmlReadingLog+="    Max. days per week="+QString::number(cn->maxDaysPerWeek)+"\n";
						}
					}
					return cn;
}

TimeConstraint* Rules::readTeachersMaxDaysPerWeek(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintTeachersMaxDaysPerWeek");
					
					ConstraintTeachersMaxDaysPerWeek* cn=new ConstraintTeachersMaxDaysPerWeek();
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Max_Days_Per_Week"){
							cn->maxDaysPerWeek=elem4.text().toInt();
							if(cn->maxDaysPerWeek<=0 || cn->maxDaysPerWeek>this->nDaysPerWeek){
								QMessageBox::information(NULL, tr("FET information"), 
								 tr("Constraint TeachersMaxDaysPerWeek day corrupt, max days %1 <= 0 or >nDaysPerWeek, ignoring constraint")
								 .arg(elem4.text()));
								delete cn;
								cn=NULL;
								return NULL;
							}
							assert(cn->maxDaysPerWeek>0 && cn->maxDaysPerWeek <= this->nDaysPerWeek);
							xmlReadingLog+="    Max. days per week="+QString::number(cn->maxDaysPerWeek)+"\n";
						}
					}
					return cn;
}


TimeConstraint* Rules::readTeacherMinDaysPerWeek(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintTeacherMinDaysPerWeek");
					
					ConstraintTeacherMinDaysPerWeek* cn=new ConstraintTeacherMinDaysPerWeek();
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Teacher_Name"){
							cn->teacherName=elem4.text();
							xmlReadingLog+="    Read teacher name="+cn->teacherName+"\n";
						}
						else if(elem4.tagName()=="Minimum_Days_Per_Week"){
							cn->minDaysPerWeek=elem4.text().toInt();
							if(cn->minDaysPerWeek<=0 || cn->minDaysPerWeek>this->nDaysPerWeek){
								QMessageBox::information(NULL, tr("FET information"), 
								 tr("Constraint TeacherMinDaysPerWeek day corrupt for teacher %1, min days %2 <= 0 or >nDaysPerWeek, ignoring constraint")
								 .arg(cn->teacherName)
								 .arg(elem4.text()));
								delete cn;
								cn=NULL;
								return NULL;
							}
							assert(cn->minDaysPerWeek>0 && cn->minDaysPerWeek <= this->nDaysPerWeek);
							xmlReadingLog+="    Min. days per week="+QString::number(cn->minDaysPerWeek)+"\n";
						}
					}
					return cn;
}

TimeConstraint* Rules::readTeachersMinDaysPerWeek(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintTeachersMinDaysPerWeek");
					
					ConstraintTeachersMinDaysPerWeek* cn=new ConstraintTeachersMinDaysPerWeek();
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Minimum_Days_Per_Week"){
							cn->minDaysPerWeek=elem4.text().toInt();
							if(cn->minDaysPerWeek<=0 || cn->minDaysPerWeek>this->nDaysPerWeek){
								QMessageBox::information(NULL, tr("FET information"), 
								 tr("Constraint TeachersMinDaysPerWeek day corrupt, min days %1 <= 0 or >nDaysPerWeek, ignoring constraint")
								 .arg(elem4.text()));
								delete cn;
								cn=NULL;
								return NULL;
							}
							assert(cn->minDaysPerWeek>0 && cn->minDaysPerWeek <= this->nDaysPerWeek);
							xmlReadingLog+="    Min. days per week="+QString::number(cn->minDaysPerWeek)+"\n";
						}
					}
					return cn;
}

TimeConstraint* Rules::readTeacherIntervalMaxDaysPerWeek(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintTeacherIntervalMaxDaysPerWeek");
					ConstraintTeacherIntervalMaxDaysPerWeek* cn=new ConstraintTeacherIntervalMaxDaysPerWeek();
					cn->maxDaysPerWeek=this->nDaysPerWeek;
					cn->startHour=this->nHoursPerDay;
					cn->endHour=this->nHoursPerDay;
					int h1, h2;
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight"){
							//cn->weight=elem4.text().toDouble();
							xmlReadingLog+="    Ignoring old tag - weight - generating 100% weight percentage\n";
							cn->weightPercentage=100;
						}
						else if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Compulsory"){
							if(elem4.text()=="yes"){
								//cn->compulsory=true;
								xmlReadingLog+="    Ignoring old tag - Current constraint is compulsory\n";
								cn->weightPercentage=100;
							}
							else{
								//cn->compulsory=false;
								xmlReadingLog+="    Old tag - current constraint is not compulsory - making weightPercentage=0%\n";
								cn->weightPercentage=0;
							}
						}
						else if(elem4.tagName()=="Teacher_Name"){
							cn->teacherName=elem4.text();
							xmlReadingLog+="    Read teacher name="+cn->teacherName+"\n";
						}
						else if(elem4.tagName()=="Max_Days_Per_Week"){
							cn->maxDaysPerWeek=elem4.text().toInt();
							if(cn->maxDaysPerWeek>this->nDaysPerWeek){
								QMessageBox::information(NULL, tr("FET information"), 
								 tr("Constraint TeacherIntervalMaxDaysPerWeek max days corrupt for teacher %1, max days %2 >nDaysPerWeek, constraint added, please correct constraint")
								 .arg(cn->teacherName)
								 .arg(elem4.text()));
								/*delete cn;
								cn=NULL;
								goto corruptConstraintTime;*/
							}
							//assert(cn->maxDaysPerWeek>0 && cn->maxDaysPerWeek <= this->nDaysPerWeek);
							xmlReadingLog+="    Max. days per week="+QString::number(cn->maxDaysPerWeek)+"\n";
						}
						else if(elem4.tagName()=="Interval_Start_Hour"){
							for(h1=0; h1 < this->nHoursPerDay; h1++)
								if(this->hoursOfTheDay[h1]==elem4.text())
									break;
							if(h1>=this->nHoursPerDay){
								QMessageBox::information(NULL, tr("FET information"), 
								 tr("Constraint Teacher interval max days per week start hour corrupt for teacher %1, hour %2 is inexistent ... ignoring constraint")
								 .arg(cn->teacherName)
								 .arg(elem4.text()));
								delete cn;
								//cn=NULL;
								//goto corruptConstraintTime;
								return NULL;
							}
							assert(h1>=0 && h1 < this->nHoursPerDay);
							xmlReadingLog+="    Interval start hour="+this->hoursOfTheDay[h1]+"\n";
							cn->startHour=h1;
						}
						else if(elem4.tagName()=="Interval_End_Hour"){
							if(elem4.text()==""){
								xmlReadingLog+="    Interval end hour void, meaning end of day\n";
								cn->endHour=this->nHoursPerDay;
							}
							else{
								for(h2=0; h2 < this->nHoursPerDay; h2++)
									if(this->hoursOfTheDay[h2]==elem4.text())
										break;
								if(h2>=this->nHoursPerDay){
									QMessageBox::information(NULL, tr("FET information"), 
									 tr("Constraint Teacher interval max days per week end hour corrupt for teacher %1, hour %2 is inexistent (it is also not void, to specify end of the day) ... ignoring constraint")
									 .arg(cn->teacherName)
									 .arg(elem4.text()));
									delete cn;
									//cn=NULL;
									//goto corruptConstraintTime;
									return NULL;
								}
								assert(h2>=0 && h2 < this->nHoursPerDay);
								xmlReadingLog+="    Interval end hour="+this->hoursOfTheDay[h2]+"\n";
								cn->endHour=h2;
							}
						}
					}
					return cn;
}

TimeConstraint* Rules::readTeachersIntervalMaxDaysPerWeek(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintTeachersIntervalMaxDaysPerWeek");
					ConstraintTeachersIntervalMaxDaysPerWeek* cn=new ConstraintTeachersIntervalMaxDaysPerWeek();
					cn->maxDaysPerWeek=this->nDaysPerWeek;
					cn->startHour=this->nHoursPerDay;
					cn->endHour=this->nHoursPerDay;
					int h1, h2;
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight"){
							//cn->weight=elem4.text().toDouble();
							xmlReadingLog+="    Ignoring old tag - weight - generating 100% weight percentage\n";
							cn->weightPercentage=100;
						}
						else if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Compulsory"){
							if(elem4.text()=="yes"){
								//cn->compulsory=true;
								xmlReadingLog+="    Ignoring old tag - Current constraint is compulsory\n";
								cn->weightPercentage=100;
							}
							else{
								//cn->compulsory=false;
								xmlReadingLog+="    Old tag - current constraint is not compulsory - making weightPercentage=0%\n";
								cn->weightPercentage=0;
							}
						}
						/*else if(elem4.tagName()=="Teacher_Name"){
							cn->teacherName=elem4.text();
							xmlReadingLog+="    Read teacher name="+cn->teacherName+"\n";
						}*/
						else if(elem4.tagName()=="Max_Days_Per_Week"){
							cn->maxDaysPerWeek=elem4.text().toInt();
							if(cn->maxDaysPerWeek>this->nDaysPerWeek){
								QMessageBox::information(NULL, tr("FET information"), 
								 tr("Constraint TeachersIntervalMaxDaysPerWeek max days corrupt, max days %2 >nDaysPerWeek, constraint added, please correct constraint")
								 //.arg(cn->teacherName)
								 .arg(elem4.text()));
								/*delete cn;
								cn=NULL;
								goto corruptConstraintTime;*/
							}
							//assert(cn->maxDaysPerWeek>0 && cn->maxDaysPerWeek <= this->nDaysPerWeek);
							xmlReadingLog+="    Max. days per week="+QString::number(cn->maxDaysPerWeek)+"\n";
						}
						else if(elem4.tagName()=="Interval_Start_Hour"){
							for(h1=0; h1 < this->nHoursPerDay; h1++)
								if(this->hoursOfTheDay[h1]==elem4.text())
									break;
							if(h1>=this->nHoursPerDay){
								QMessageBox::information(NULL, tr("FET information"), 
								 tr("Constraint Teachers interval max days per week start hour corrupt because hour %2 is inexistent ... ignoring constraint")
								 //.arg(cn->teacherName)
								 .arg(elem4.text()));
								delete cn;
								//cn=NULL;
								//goto corruptConstraintTime;
								return NULL;
							}
							assert(h1>=0 && h1 < this->nHoursPerDay);
							xmlReadingLog+="    Interval start hour="+this->hoursOfTheDay[h1]+"\n";
							cn->startHour=h1;
						}
						else if(elem4.tagName()=="Interval_End_Hour"){
							if(elem4.text()==""){
								xmlReadingLog+="    Interval end hour void, meaning end of day\n";
								cn->endHour=this->nHoursPerDay;
							}
							else{
								for(h2=0; h2 < this->nHoursPerDay; h2++)
									if(this->hoursOfTheDay[h2]==elem4.text())
										break;
								if(h2>=this->nHoursPerDay){
									QMessageBox::information(NULL, tr("FET information"), 
									 tr("Constraint Teachers interval max days per week end hour corrupt because hour %2 is inexistent (it is also not void, to specify end of the day) ... ignoring constraint")
									 //.arg(cn->teacherName)
									 .arg(elem4.text()));
									delete cn;
									//cn=NULL;
									//goto corruptConstraintTime;
									return NULL;
								}
								assert(h2>=0 && h2 < this->nHoursPerDay);
								xmlReadingLog+="    Interval end hour="+this->hoursOfTheDay[h2]+"\n";
								cn->endHour=h2;
							}
						}
					}
					return cn;
}

TimeConstraint* Rules::readStudentsSetIntervalMaxDaysPerWeek(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintStudentsSetIntervalMaxDaysPerWeek");
					ConstraintStudentsSetIntervalMaxDaysPerWeek* cn=new ConstraintStudentsSetIntervalMaxDaysPerWeek();
					cn->maxDaysPerWeek=this->nDaysPerWeek;
					cn->startHour=this->nHoursPerDay;
					cn->endHour=this->nHoursPerDay;
					int h1, h2;
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight"){
							//cn->weight=elem4.text().toDouble();
							xmlReadingLog+="    Ignoring old tag - weight - generating 100% weight percentage\n";
							cn->weightPercentage=100;
						}
						else if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Compulsory"){
							if(elem4.text()=="yes"){
								//cn->compulsory=true;
								xmlReadingLog+="    Ignoring old tag - Current constraint is compulsory\n";
								cn->weightPercentage=100;
							}
							else{
								//cn->compulsory=false;
								xmlReadingLog+="    Old tag - current constraint is not compulsory - making weightPercentage=0%\n";
								cn->weightPercentage=0;
							}
						}
						else if(elem4.tagName()=="Students"){
							cn->students=elem4.text();
							xmlReadingLog+="    Read students set name="+cn->students+"\n";
						}
						else if(elem4.tagName()=="Max_Days_Per_Week"){
							cn->maxDaysPerWeek=elem4.text().toInt();
							if(cn->maxDaysPerWeek>this->nDaysPerWeek){
								QMessageBox::information(NULL, tr("FET information"), 
								 tr("Constraint StudentsSetIntervalMaxDaysPerWeek max days corrupt for students set %1, max days %2 >nDaysPerWeek, constraint added, please correct constraint")
								 .arg(cn->students)
								 .arg(elem4.text()));
								/*delete cn;
								cn=NULL;
								goto corruptConstraintTime;*/
							}
							//assert(cn->maxDaysPerWeek>0 && cn->maxDaysPerWeek <= this->nDaysPerWeek);
							xmlReadingLog+="    Max. days per week="+QString::number(cn->maxDaysPerWeek)+"\n";
						}
						else if(elem4.tagName()=="Interval_Start_Hour"){
							for(h1=0; h1 < this->nHoursPerDay; h1++)
								if(this->hoursOfTheDay[h1]==elem4.text())
									break;
							if(h1>=this->nHoursPerDay){
								QMessageBox::information(NULL, tr("FET information"), 
								 tr("Constraint Students set interval max days per week start hour corrupt for students %1, hour %2 is inexistent ... ignoring constraint")
								 .arg(cn->students)
								 .arg(elem4.text()));
								delete cn;
								//cn=NULL;
								//goto corruptConstraintTime;
								return NULL;
							}
							assert(h1>=0 && h1 < this->nHoursPerDay);
							xmlReadingLog+="    Interval start hour="+this->hoursOfTheDay[h1]+"\n";
							cn->startHour=h1;
						}
						else if(elem4.tagName()=="Interval_End_Hour"){
							if(elem4.text()==""){
								xmlReadingLog+="    Interval end hour void, meaning end of day\n";
								cn->endHour=this->nHoursPerDay;
							}
							else{
								for(h2=0; h2 < this->nHoursPerDay; h2++)
									if(this->hoursOfTheDay[h2]==elem4.text())
										break;
								if(h2>=this->nHoursPerDay){
									QMessageBox::information(NULL, tr("FET information"), 
									 tr("Constraint Students set interval max days per week end hour corrupt for students %1, hour %2 is inexistent (it is also not void, to specify end of the day) ... ignoring constraint")
									 .arg(cn->students)
									 .arg(elem4.text()));
									delete cn;
									//cn=NULL;
									//goto corruptConstraintTime;
									return NULL;
								}
								assert(h2>=0 && h2 < this->nHoursPerDay);
								xmlReadingLog+="    Interval end hour="+this->hoursOfTheDay[h2]+"\n";
								cn->endHour=h2;
							}
						}
					}
					return cn;
}

TimeConstraint* Rules::readStudentsIntervalMaxDaysPerWeek(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintStudentsIntervalMaxDaysPerWeek");
					ConstraintStudentsIntervalMaxDaysPerWeek* cn=new ConstraintStudentsIntervalMaxDaysPerWeek();
					cn->maxDaysPerWeek=this->nDaysPerWeek;
					cn->startHour=this->nHoursPerDay;
					cn->endHour=this->nHoursPerDay;
					int h1, h2;
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight"){
							//cn->weight=elem4.text().toDouble();
							xmlReadingLog+="    Ignoring old tag - weight - generating 100% weight percentage\n";
							cn->weightPercentage=100;
						}
						else if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Compulsory"){
							if(elem4.text()=="yes"){
								//cn->compulsory=true;
								xmlReadingLog+="    Ignoring old tag - Current constraint is compulsory\n";
								cn->weightPercentage=100;
							}
							else{
								//cn->compulsory=false;
								xmlReadingLog+="    Old tag - current constraint is not compulsory - making weightPercentage=0%\n";
								cn->weightPercentage=0;
							}
						}
						/*else if(elem4.tagName()=="Students"){
							cn->students=elem4.text();
							xmlReadingLog+="    Read students set name="+cn->students+"\n";
						}*/
						else if(elem4.tagName()=="Max_Days_Per_Week"){
							cn->maxDaysPerWeek=elem4.text().toInt();
							if(cn->maxDaysPerWeek>this->nDaysPerWeek){
								QMessageBox::information(NULL, tr("FET information"), 
								 tr("Constraint StudentsIntervalMaxDaysPerWeek max days corrupt: max days %2 >nDaysPerWeek, constraint added, please correct constraint")
								 .arg(elem4.text()));
								/*delete cn;
								cn=NULL;
								goto corruptConstraintTime;*/
							}
							//assert(cn->maxDaysPerWeek>0 && cn->maxDaysPerWeek <= this->nDaysPerWeek);
							xmlReadingLog+="    Max. days per week="+QString::number(cn->maxDaysPerWeek)+"\n";
						}
						else if(elem4.tagName()=="Interval_Start_Hour"){
							for(h1=0; h1 < this->nHoursPerDay; h1++)
								if(this->hoursOfTheDay[h1]==elem4.text())
									break;
							if(h1>=this->nHoursPerDay){
								QMessageBox::information(NULL, tr("FET information"), 
								 tr("Constraint Students interval max days per week start hour corrupt: hour %2 is inexistent ... ignoring constraint")
								 //.arg(cn->students)
								 .arg(elem4.text()));
								delete cn;
								//cn=NULL;
								//goto corruptConstraintTime;
								return NULL;
							}
							assert(h1>=0 && h1 < this->nHoursPerDay);
							xmlReadingLog+="    Interval start hour="+this->hoursOfTheDay[h1]+"\n";
							cn->startHour=h1;
						}
						else if(elem4.tagName()=="Interval_End_Hour"){
							if(elem4.text()==""){
								xmlReadingLog+="    Interval end hour void, meaning end of day\n";
								cn->endHour=this->nHoursPerDay;
							}
							else{
								for(h2=0; h2 < this->nHoursPerDay; h2++)
									if(this->hoursOfTheDay[h2]==elem4.text())
										break;
								if(h2>=this->nHoursPerDay){
									QMessageBox::information(NULL, tr("FET information"), 
									 tr("Constraint Students interval max days per week end hour corrupt: hour %2 is inexistent (it is also not void, to specify end of the day) ... ignoring constraint")
									 //.arg(cn->students)
									 .arg(elem4.text()));
									delete cn;
									//cn=NULL;
									//goto corruptConstraintTime;
									return NULL;
								}
								assert(h2>=0 && h2 < this->nHoursPerDay);
								xmlReadingLog+="    Interval end hour="+this->hoursOfTheDay[h2]+"\n";
								cn->endHour=h2;
							}
						}
					}
					return cn;
}

TimeConstraint* Rules::readStudentsSetNotAvailable(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintStudentsSetNotAvailable");

					//ConstraintStudentsSetNotAvailableTimes* cn=new ConstraintStudentsSetNotAvailableTimes();
					QList<int> days;
					QList<int> hours;
					QString students;
					double weightPercentage=100;
					int d=-1, h1=-1, h2=-1;
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight_Percentage"){
							weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Read weight percentage="+QString::number(weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Day"){
							for(d=0; d<this->nDaysPerWeek; d++)
								if(this->daysOfTheWeek[d]==elem4.text())
									break;
							if(d>=this->nDaysPerWeek){
								QMessageBox::information(NULL, tr("FET information"), 
								 tr("Constraint StudentsSetNotAvailable day corrupt for students %1, day %2 is inexistent ... ignoring constraint")
								 .arg(students)
								 .arg(elem4.text()));
								//cn=NULL;
								//goto corruptConstraintTime;
								return NULL;
							}
							assert(d<this->nDaysPerWeek);
							xmlReadingLog+="    Crt. day="+this->daysOfTheWeek[d]+"\n";
						}
						else if(elem4.tagName()=="Start_Hour"){
							for(h1=0; h1 < this->nHoursPerDay; h1++)
								if(this->hoursOfTheDay[h1]==elem4.text())
									break;
							if(h1>=this->nHoursPerDay){
								QMessageBox::information(NULL, tr("FET information"), 
								 tr("Constraint StudentsSetNotAvailable start hour corrupt for students set %1, hour %2 is inexistent ... ignoring constraint")
								 .arg(students)
								 .arg(elem4.text()));
								//cn=NULL;
								//goto corruptConstraintTime;
								return NULL;
							}
							assert(h1>=0 && h1 < this->nHoursPerDay);
							xmlReadingLog+="    Start hour="+this->hoursOfTheDay[h1]+"\n";
						}
						else if(elem4.tagName()=="End_Hour"){
							for(h2=0; h2 < this->nHoursPerDay; h2++)
								if(this->hoursOfTheDay[h2]==elem4.text())
									break;
							if(h2<=0 || h2>this->nHoursPerDay){
								QMessageBox::information(NULL, tr("FET information"), 
								 tr("Constraint StudentsSetNotAvailable end hour corrupt for students %1, hour %2 is inexistent ... ignoring constraint")
								 .arg(students)
								 .arg(elem4.text()));
								//goto corruptConstraintTime;
								return NULL;
							}
							assert(h2>0 && h2 <= this->nHoursPerDay);
							xmlReadingLog+="    End hour="+this->hoursOfTheDay[h2]+"\n";
						}
						else if(elem4.tagName()=="Students"){
							students=elem4.text();
							xmlReadingLog+="    Read students name="+students+"\n";
						}
					}
					
					assert(weightPercentage>=0);
					assert(d>=0 && h1>=0 && h2>=0);
					
					ConstraintStudentsSetNotAvailableTimes* cn = NULL;
					
					bool found=false;
					foreach(TimeConstraint* c, this->timeConstraintsList)
						if(c->type==CONSTRAINT_STUDENTS_SET_NOT_AVAILABLE_TIMES){
							ConstraintStudentsSetNotAvailableTimes* ssna=(ConstraintStudentsSetNotAvailableTimes*) c;
							if(ssna->students==students){
								found=true;
								
								for(int hh=h1; hh<h2; hh++){
									int k;
									for(k=0; k<ssna->days.count(); k++)
										if(ssna->days.at(k)==d && ssna->hours.at(k)==hh)
											break;
									if(k==ssna->days.count()){
										ssna->days.append(d);
										ssna->hours.append(hh);
									}
								}
								
								assert(ssna->days.count()==ssna->hours.count());
							}
						}
					if(!found){
						days.clear();
						hours.clear();
						for(int hh=h1; hh<h2; hh++){
							days.append(d);
							hours.append(hh);
						}
					
						cn=new ConstraintStudentsSetNotAvailableTimes(weightPercentage, students, days, hours);

						//crt_constraint=cn;
						return cn;
					}
					else
						return NULL;
}

TimeConstraint* Rules::readStudentsSetNotAvailableTimes(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintStudentsSetNotAvailableTimes");
					
					ConstraintStudentsSetNotAvailableTimes* cn=new ConstraintStudentsSetNotAvailableTimes();
					int nNotAvailableSlots=0;
					int i=0;
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Read weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}

						else if(elem4.tagName()=="Number_of_Not_Available_Times"){
							nNotAvailableSlots=elem4.text().toInt();
							xmlReadingLog+="    Read number of not available times="+QString::number(nNotAvailableSlots)+"\n";
						}

						else if(elem4.tagName()=="Not_Available_Time"){
							xmlReadingLog+="    Read: not available time\n";
							
							int d=-1;
							int h=-1;

							for(QDomNode node5=elem4.firstChild(); !node5.isNull(); node5=node5.nextSibling()){
								QDomElement elem5=node5.toElement();
								if(elem5.isNull()){
									xmlReadingLog+="    Null node here\n";
									continue;
								}
								xmlReadingLog+="    Found "+elem5.tagName()+" tag\n";
								if(elem5.tagName()=="Day"){
									for(d=0; d<this->nDaysPerWeek; d++)
										if(this->daysOfTheWeek[d]==elem5.text())
											break;

									if(d>=this->nDaysPerWeek){
										QMessageBox::information(NULL, tr("FET information"), 
										 tr("Constraint StudentsSetNotAvailableTimes day corrupt for students %1, day %2 is inexistent ... ignoring constraint")
										 .arg(cn->students)
										 .arg(elem5.text()));
										delete cn;
										cn=NULL;
										//goto corruptConstraintTime;
										return NULL;
									}
						
									assert(d<this->nDaysPerWeek);
									xmlReadingLog+="    Day="+this->daysOfTheWeek[d]+"("+QString::number(i)+")"+"\n";
								}
								else if(elem5.tagName()=="Hour"){
									for(h=0; h < this->nHoursPerDay; h++)
										if(this->hoursOfTheDay[h]==elem5.text())
											break;
									
									if(h>=this->nHoursPerDay){
										QMessageBox::information(NULL, tr("FET information"), 
										 tr("Constraint StudentsSetNotAvailableTimes hour corrupt for students %1, hour %2 is inexistent ... ignoring constraint")
										 .arg(cn->students)
										 .arg(elem5.text()));
										delete cn;
										cn=NULL;
										//goto corruptConstraintTime;
										return NULL;
									}
									
									assert(h>=0 && h < this->nHoursPerDay);
									xmlReadingLog+="    Hour="+this->hoursOfTheDay[h]+"\n";
								}
							}
							i++;
							
							cn->days.append(d);
							cn->hours.append(h);
						}
						else if(elem4.tagName()=="Students"){
							cn->students=elem4.text();
							xmlReadingLog+="    Read students name="+cn->students+"\n";
						}
					}
					assert(i==cn->days.count() && i==cn->hours.count());
					assert(i==nNotAvailableSlots);
					return cn;
}

TimeConstraint* Rules::readMinNDaysBetweenActivities(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintMinNDaysBetweenActivities");
					
					ConstraintMinDaysBetweenActivities* cn=new ConstraintMinDaysBetweenActivities();
					bool foundCISD=false;
					int n_act=0;
					cn->activitiesId.clear();
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight"){
							//cn->weight=elem4.text().toDouble();
							xmlReadingLog+="    Ignoring old tag - weight - generating weightPercentage=95%\n";
							cn->weightPercentage=95;
						}
						else if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weightPercentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Consecutive_If_Same_Day" || elem4.tagName()=="Adjacent_If_Broken"){
							if(elem4.text()=="yes" || elem4.text()=="true" || elem4.text()=="1"){
								cn->consecutiveIfSameDay=true;
								foundCISD=true;
								xmlReadingLog+="    Current constraint has consecutive if same day=true\n";
							}
							else{
								if(!(elem4.text()=="no" || elem4.text()=="false" || elem4.text()=="0")){
									QMessageBox::warning(NULL, tr("FET warning"),
									 tr("Found constraint min days between activities with tag consecutive if same day"
									 " which is not 'true', 'false', 'yes', 'no', '1' or '0'."
									 " The tag will be considered false",
									 "Instructions for translators: please leave the 'true', 'false', 'yes' and 'no' fields untranslated, as they are in English"));
								}
								//assert(elem4.text()=="no" || elem4.text()=="false" || elem4.text()=="0");
								cn->consecutiveIfSameDay=false;
								foundCISD=true;
								xmlReadingLog+="    Current constraint has consecutive if same day=false\n";
							}
						}
						else if(elem4.tagName()=="Compulsory"){
							if(elem4.text()=="yes"){
								//cn->compulsory=true;
								xmlReadingLog+="    Ignoring old tag - Current constraint is compulsory\n";
								cn->weightPercentage=95;
								cn->consecutiveIfSameDay=true;
								foundCISD=true;
							}
							else{
								//cn->compulsory=false;
								xmlReadingLog+="    Old tag - current constraint is not compulsory - making weightPercentage=0%\n";
								cn->weightPercentage=0;
								cn->consecutiveIfSameDay=false;
								foundCISD=true;
							}
						}
						else if(elem4.tagName()=="Number_of_Activities"){
							cn->n_activities=elem4.text().toInt();
							xmlReadingLog+="    Read n_activities="+QString::number(cn->n_activities)+"\n";
						}
						else if(elem4.tagName()=="Activity_Id"){
							//cn->activitiesId[n_act]=elem4.text().toInt();
							cn->activitiesId.append(elem4.text().toInt());
							assert(n_act==cn->activitiesId.count()-1);
							xmlReadingLog+="    Read activity id="+QString::number(cn->activitiesId[n_act])+"\n";
							n_act++;
						}
						else if(elem4.tagName()=="MinDays"){
							cn->minDays=elem4.text().toInt();
							xmlReadingLog+="    Read MinDays="+QString::number(cn->minDays)+"\n";
						}
					}
					if(!foundCISD){
						xmlReadingLog+="    Could not find consecutive if same day information - making it true\n";
						cn->consecutiveIfSameDay=true;
					}
					assert(n_act==cn->n_activities);
					return cn;
/*
#if 0&0&0
					if(0 && reportIncorrectMinDays && cn->n_activities > this->nDaysPerWeek){
						QString s=tr("You have a constraint min days between activities with more activities than the number of days per week.");
						s+=" ";
						s+=tr("Constraint is:");
						s+="\n";
						s+=crt_constraint->getDescription(*this);
						s+="\n";
						s+=tr("This is a very bad practice from the way the algorithm of generation works (it slows down the generation and makes it harder to find a solution).");
			 			s+="\n\n";
						s+=tr("To improve your file, you are advised to remove the corresponding activities and constraint and add activities again, respecting the following rules:");
			 			s+="\n\n";
						s+=tr("1. If you add 'force consecutive if same day', then couple extra activities in pairs to obtain a number of activities equal to the number of days per week"
			  			 ". Example: 7 activities with duration 1 in a 5 days week, then transform into 5 activities with durations: 2,2,1,1,1 and add a single container activity with these 5 components"
						 " (possibly raising the weight of added constraint min days between activities up to 100%)");
			 			s+="\n\n";

						s+=tr("2. If you don't add 'force consecutive if same day', then add a larger activity splitted into a number of"
						  " activities equal with the number of days per week and the remaining components into other larger splitted activity."
						  " For example, suppose you need to add 7 activities with duration 1 in a 5 days week. Add 2 larger container activities,"
						  " first one splitted into 5 activities with duration 1 and second one splitted into 2 activities with duration 1"
						  " (possibly raising the weight of added constraints min days between activities for each of the 2 containers up to 100%)");
						
					 	int t=QMessageBox::warning(NULL, tr("FET warning"), s,
						 tr("Skip rest"), tr("See next"), QString(),
						 1, 0 );
													 
						if(t==0)
							reportIncorrectMinDays=false;
					}
#endif
*/
}

TimeConstraint* Rules::readMinDaysBetweenActivities(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintMinDaysBetweenActivities");
					
					ConstraintMinDaysBetweenActivities* cn=new ConstraintMinDaysBetweenActivities();
					bool foundCISD=false;
					int n_act=0;
					cn->activitiesId.clear();
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight"){
							//cn->weight=elem4.text().toDouble();
							xmlReadingLog+="    Ignoring old tag - weight - generating weightPercentage=95%\n";
							cn->weightPercentage=95;
						}
						else if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weightPercentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Consecutive_If_Same_Day" || elem4.tagName()=="Adjacent_If_Broken"){
							if(elem4.text()=="yes" || elem4.text()=="true" || elem4.text()=="1"){
								cn->consecutiveIfSameDay=true;
								foundCISD=true;
								xmlReadingLog+="    Current constraint has consecutive if same day=true\n";
							}
							else{
								if(!(elem4.text()=="no" || elem4.text()=="false" || elem4.text()=="0")){
									QMessageBox::warning(NULL, tr("FET warning"),
									 tr("Found constraint min days between activities with tag consecutive if same day"
									 " which is not 'true', 'false', 'yes', 'no', '1' or '0'."
									 " The tag will be considered false",
									 "Instructions for translators: please leave the 'true', 'false', 'yes' and 'no' fields untranslated, as they are in English"));
								}
								//assert(elem4.text()=="no" || elem4.text()=="false" || elem4.text()=="0");
								cn->consecutiveIfSameDay=false;
								foundCISD=true;
								xmlReadingLog+="    Current constraint has consecutive if same day=false\n";
							}
						}
						else if(elem4.tagName()=="Compulsory"){
							if(elem4.text()=="yes"){
								//cn->compulsory=true;
								xmlReadingLog+="    Ignoring old tag - Current constraint is compulsory\n";
								cn->weightPercentage=95;
								cn->consecutiveIfSameDay=true;
								foundCISD=true;
							}
							else{
								//cn->compulsory=false;
								xmlReadingLog+="    Old tag - current constraint is not compulsory - making weightPercentage=0%\n";
								cn->weightPercentage=0;
								cn->consecutiveIfSameDay=false;
								foundCISD=true;
							}
						}
						else if(elem4.tagName()=="Number_of_Activities"){
							cn->n_activities=elem4.text().toInt();
							xmlReadingLog+="    Read n_activities="+QString::number(cn->n_activities)+"\n";
						}
						else if(elem4.tagName()=="Activity_Id"){
							//cn->activitiesId[n_act]=elem4.text().toInt();
							cn->activitiesId.append(elem4.text().toInt());
							assert(n_act==cn->activitiesId.count()-1);
							xmlReadingLog+="    Read activity id="+QString::number(cn->activitiesId[n_act])+"\n";
							n_act++;
						}
						else if(elem4.tagName()=="MinDays"){
							cn->minDays=elem4.text().toInt();
							xmlReadingLog+="    Read MinDays="+QString::number(cn->minDays)+"\n";
						}
					}
					if(!foundCISD){
						xmlReadingLog+="    Could not find consecutive if same day information - making it true\n";
						cn->consecutiveIfSameDay=true;
					}
					assert(n_act==cn->n_activities);
					return cn;
/*
#if 0&0&0
					if(0 && reportIncorrectMinDays && cn->n_activities > this->nDaysPerWeek){
						QString s=tr("You have a constraint min days between activities with more activities than the number of days per week.");
						s+=" ";
						s+=tr("Constraint is:");
						s+="\n";
						s+=crt_constraint->getDescription(*this);
						s+="\n";
						s+=tr("This is a very bad practice from the way the algorithm of generation works (it slows down the generation and makes it harder to find a solution).");
			 			s+="\n\n";
						s+=tr("To improve your file, you are advised to remove the corresponding activities and constraint and add activities again, respecting the following rules:");
			 			s+="\n\n";
						s+=tr("1. If you add 'force consecutive if same day', then couple extra activities in pairs to obtain a number of activities equal to the number of days per week"
			  			 ". Example: 7 activities with duration 1 in a 5 days week, then transform into 5 activities with durations: 2,2,1,1,1 and add a single container activity with these 5 components"
						 " (possibly raising the weight of added constraint min days between activities up to 100%)");
			 			s+="\n\n";

						s+=tr("2. If you don't add 'force consecutive if same day', then add a larger activity splitted into a number of"
						  " activities equal with the number of days per week and the remaining components into other larger splitted activity."
						  " For example, suppose you need to add 7 activities with duration 1 in a 5 days week. Add 2 larger container activities,"
						  " first one splitted into 5 activities with duration 1 and second one splitted into 2 activities with duration 1"
						  " (possibly raising the weight of added constraints min days between activities for each of the 2 containers up to 100%)");
						
					 	int t=QMessageBox::warning(NULL, tr("FET warning"), s,
						 tr("Skip rest"), tr("See next"), QString(),
						 1, 0 );
													 
						if(t==0)
							reportIncorrectMinDays=false;
					}
#endif
*/
}

TimeConstraint* Rules::readMaxDaysBetweenActivities(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintMaxDaysBetweenActivities");
					
					ConstraintMaxDaysBetweenActivities* cn=new ConstraintMaxDaysBetweenActivities();
					int n_act=0;
					cn->activitiesId.clear();
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weightPercentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Number_of_Activities"){
							cn->n_activities=elem4.text().toInt();
							xmlReadingLog+="    Read n_activities="+QString::number(cn->n_activities)+"\n";
						}
						else if(elem4.tagName()=="Activity_Id"){
							cn->activitiesId.append(elem4.text().toInt());
							assert(n_act==cn->activitiesId.count()-1);
							//cn->activitiesId[n_act]=elem4.text().toInt();
							xmlReadingLog+="    Read activity id="+QString::number(cn->activitiesId[n_act])+"\n";
							n_act++;
						}
						else if(elem4.tagName()=="MaxDays"){
							cn->maxDays=elem4.text().toInt();
							xmlReadingLog+="    Read MaxDays="+QString::number(cn->maxDays)+"\n";
						}
					}
					assert(n_act==cn->n_activities);
					return cn;
}

TimeConstraint* Rules::readMinGapsBetweenActivities(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintMinGapsBetweenActivities");
					ConstraintMinGapsBetweenActivities* cn=new ConstraintMinGapsBetweenActivities();
					int n_act=0;
					cn->activitiesId.clear();
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weightPercentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Number_of_Activities"){
							cn->n_activities=elem4.text().toInt();
							xmlReadingLog+="    Read n_activities="+QString::number(cn->n_activities)+"\n";
						}
						else if(elem4.tagName()=="Activity_Id"){
							cn->activitiesId.append(elem4.text().toInt());
							assert(n_act==cn->activitiesId.count()-1);
							xmlReadingLog+="    Read activity id="+QString::number(cn->activitiesId[n_act])+"\n";
							n_act++;
						}
						else if(elem4.tagName()=="MinGaps"){
							cn->minGaps=elem4.text().toInt();
							xmlReadingLog+="    Read MinGaps="+QString::number(cn->minGaps)+"\n";
						}
					}
					assert(n_act==cn->n_activities);
					assert(n_act==cn->activitiesId.count());
					return cn;
}

TimeConstraint* Rules::readActivitiesNotOverlapping(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintActivitiesNotOverlapping");
					ConstraintActivitiesNotOverlapping* cn=new ConstraintActivitiesNotOverlapping();
					int n_act=0;
					cn->activitiesId.clear();
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight"){
							//cn->weight=elem4.text().toDouble();
							xmlReadingLog+="    Ignoring old tag - weight - making weight percentage=100\n";
							cn->weightPercentage=100;
						}
						else if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Compulsory"){
							if(elem4.text()=="yes"){
								//cn->compulsory=true;
								xmlReadingLog+="    Ignoring old tag - Current constraint is compulsory\n";
								cn->weightPercentage=100;
							}
							else{
								//cn->compulsory=false;
								xmlReadingLog+="    Old tag - current constraint is not compulsory - making weightPercentage=0%\n";
								cn->weightPercentage=0;
							}
						}
						else if(elem4.tagName()=="Number_of_Activities"){
							cn->n_activities=elem4.text().toInt();
							xmlReadingLog+="    Read n_activities="+QString::number(cn->n_activities)+"\n";
						}
						else if(elem4.tagName()=="Activity_Id"){
							//cn->activitiesId[n_act]=elem4.text().toInt();
							cn->activitiesId.append(elem4.text().toInt());
							assert(n_act==cn->activitiesId.count()-1);
							xmlReadingLog+="    Read activity id="+QString::number(cn->activitiesId[n_act])+"\n";
							n_act++;
						}
					}
					assert(n_act==cn->n_activities);
					return cn;
}

TimeConstraint* Rules::readActivitiesSameStartingTime(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintActivitiesSameStartingTime");
					ConstraintActivitiesSameStartingTime* cn=new ConstraintActivitiesSameStartingTime();
					int n_act=0;
					cn->activitiesId.clear();
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight"){
							//cn->weight=elem4.text().toDouble();
							xmlReadingLog+="    Ignoring old tag - weight - making weight percentage=100\n";
							cn->weightPercentage=100;
						}
						else if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Compulsory"){
							if(elem4.text()=="yes"){
								//cn->compulsory=true;
								xmlReadingLog+="    Ignoring old tag - Current constraint is compulsory\n";
								cn->weightPercentage=100;
							}
							else{
								//cn->compulsory=false;
								xmlReadingLog+="    Old tag - current constraint is not compulsory - making weightPercentage=0%\n";
								cn->weightPercentage=0;
							}
						}
						else if(elem4.tagName()=="Number_of_Activities"){
							cn->n_activities=elem4.text().toInt();
							xmlReadingLog+="    Read n_activities="+QString::number(cn->n_activities)+"\n";
						}
						else if(elem4.tagName()=="Activity_Id"){
							//cn->activitiesId[n_act]=elem4.text().toInt();
							cn->activitiesId.append(elem4.text().toInt());
							assert(n_act==cn->activitiesId.count()-1);
							xmlReadingLog+="    Read activity id="+QString::number(cn->activitiesId[n_act])+"\n";
							n_act++;
						}
					}
					assert(cn->n_activities==n_act);
					return cn;
}

TimeConstraint* Rules::readActivitiesSameStartingHour(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintActivitiesSameStartingHour");
					ConstraintActivitiesSameStartingHour* cn=new ConstraintActivitiesSameStartingHour();
					int n_act=0;
					cn->activitiesId.clear();
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight"){
							//cn->weight=elem4.text().toDouble();
							xmlReadingLog+="    Ignoring old tag - weight - making weight percentage=100\n";
							cn->weightPercentage=100;
						}
						else if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Compulsory"){
							if(elem4.text()=="yes"){
								//cn->compulsory=true;
								xmlReadingLog+="    Ignoring old tag - Current constraint is compulsory\n";
								cn->weightPercentage=100;
							}
							else{
								//cn->compulsory=false;
								xmlReadingLog+="    Old tag - current constraint is not compulsory - making weightPercentage=0%\n";
								cn->weightPercentage=0;
							}
						}
						else if(elem4.tagName()=="Number_of_Activities"){
							cn->n_activities=elem4.text().toInt();
							xmlReadingLog+="    Read n_activities="+QString::number(cn->n_activities)+"\n";
						}
						else if(elem4.tagName()=="Activity_Id"){
							//cn->activitiesId[n_act]=elem4.text().toInt();
							cn->activitiesId.append(elem4.text().toInt());
							assert(n_act==cn->activitiesId.count()-1);
							xmlReadingLog+="    Read activity id="+QString::number(cn->activitiesId[n_act])+"\n";
							n_act++;
						}
					}
					assert(cn->n_activities==n_act);
					return cn;
}

TimeConstraint* Rules::readActivitiesSameStartingDay(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintActivitiesSameStartingDay");
					ConstraintActivitiesSameStartingDay* cn=new ConstraintActivitiesSameStartingDay();
					int n_act=0;
					cn->activitiesId.clear();
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Number_of_Activities"){
							cn->n_activities=elem4.text().toInt();
							xmlReadingLog+="    Read n_activities="+QString::number(cn->n_activities)+"\n";
						}
						else if(elem4.tagName()=="Activity_Id"){
							//cn->activitiesId[n_act]=elem4.text().toInt();
							cn->activitiesId.append(elem4.text().toInt());
							assert(n_act==cn->activitiesId.count()-1);
							xmlReadingLog+="    Read activity id="+QString::number(cn->activitiesId[n_act])+"\n";
							n_act++;
						}
					}
					assert(cn->n_activities==n_act);
					return cn;
}

TimeConstraint* Rules::readTeachersMaxHoursDaily(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintTeachersMaxHoursDaily");
					ConstraintTeachersMaxHoursDaily* cn=new ConstraintTeachersMaxHoursDaily();
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight"){
							//cn->weight=elem4.text().toDouble();
							xmlReadingLog+="    Ignoring old tag - weight - making weight percentage=100\n";
							cn->weightPercentage=100;
						}
						else if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Compulsory"){
							if(elem4.text()=="yes"){
								//cn->compulsory=true;
								xmlReadingLog+="    Ignoring old tag - Current constraint is compulsory\n";
								cn->weightPercentage=100;
							}
							else{
								//cn->compulsory=false;
								xmlReadingLog+="    Old tag - current constraint is not compulsory - making weightPercentage=0%\n";
								cn->weightPercentage=0;
							}
						}
						else if(elem4.tagName()=="Maximum_Hours_Daily"){
							cn->maxHoursDaily=elem4.text().toInt();
							xmlReadingLog+="    Read maxHoursDaily="+QString::number(cn->maxHoursDaily)+"\n";
						}
					}
					return cn;
}

TimeConstraint* Rules::readTeacherMaxHoursDaily(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintTeacherMaxHoursDaily");
					ConstraintTeacherMaxHoursDaily* cn=new ConstraintTeacherMaxHoursDaily();
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight"){
							//cn->weight=elem4.text().toDouble();
							xmlReadingLog+="    Ignoring old tag - weight - making weight percentage=100\n";
							cn->weightPercentage=100;
						}
						else if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Compulsory"){
							if(elem4.text()=="yes"){
								//cn->compulsory=true;
								xmlReadingLog+="    Ignoring old tag - Current constraint is compulsory\n";
								cn->weightPercentage=100;
							}
							else{
								//cn->compulsory=false;
								xmlReadingLog+="    Old tag - current constraint is not compulsory - making weightPercentage=0%\n";
								cn->weightPercentage=0;
							}
						}
						else if(elem4.tagName()=="Maximum_Hours_Daily"){
							cn->maxHoursDaily=elem4.text().toInt();
							xmlReadingLog+="    Read maxHoursDaily="+QString::number(cn->maxHoursDaily)+"\n";
						}
						else if(elem4.tagName()=="Teacher_Name"){
							cn->teacherName=elem4.text();
							xmlReadingLog+="    Read teacher name="+cn->teacherName+"\n";
						}
					}
					return cn;
}

TimeConstraint* Rules::readTeachersMaxHoursContinuously(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintTeachersMaxHoursContinuously");
					ConstraintTeachersMaxHoursContinuously* cn=new ConstraintTeachersMaxHoursContinuously();
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight"){
							//cn->weight=elem4.text().toDouble();
							xmlReadingLog+="    Ignoring old tag - weight - making weight percentage=100\n";
							cn->weightPercentage=100;
						}
						else if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Compulsory"){
							if(elem4.text()=="yes"){
								//cn->compulsory=true;
								xmlReadingLog+="    Ignoring old tag - Current constraint is compulsory\n";
								cn->weightPercentage=100;
							}
							else{
								//cn->compulsory=false;
								xmlReadingLog+="    Old tag - current constraint is not compulsory - making weightPercentage=0%\n";
								cn->weightPercentage=0;
							}
						}
						else if(elem4.tagName()=="Maximum_Hours_Continuously"){
							cn->maxHoursContinuously=elem4.text().toInt();
							xmlReadingLog+="    Read maxHoursContinuously="+QString::number(cn->maxHoursContinuously)+"\n";
						}
					}
					return cn;
}

TimeConstraint* Rules::readTeacherMaxHoursContinuously(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintTeacherMaxHoursContinuously");
					ConstraintTeacherMaxHoursContinuously* cn=new ConstraintTeacherMaxHoursContinuously();
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight"){
							//cn->weight=elem4.text().toDouble();
							xmlReadingLog+="    Ignoring old tag - weight - making weight percentage=100\n";
							cn->weightPercentage=100;
						}
						else if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Compulsory"){
							if(elem4.text()=="yes"){
								//cn->compulsory=true;
								xmlReadingLog+="    Ignoring old tag - Current constraint is compulsory\n";
								cn->weightPercentage=100;
							}
							else{
								//cn->compulsory=false;
								xmlReadingLog+="    Old tag - current constraint is not compulsory - making weightPercentage=0%\n";
								cn->weightPercentage=0;
							}
						}
						else if(elem4.tagName()=="Maximum_Hours_Continuously"){
							cn->maxHoursContinuously=elem4.text().toInt();
							xmlReadingLog+="    Read maxHoursContinuously="+QString::number(cn->maxHoursContinuously)+"\n";
						}
						else if(elem4.tagName()=="Teacher_Name"){
							cn->teacherName=elem4.text();
							xmlReadingLog+="    Read teacher name="+cn->teacherName+"\n";
						}
					}
					return cn;
}

TimeConstraint* Rules::readTeacherActivityTagMaxHoursContinuously(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintTeacherActivityTagMaxHoursContinuously");
					ConstraintTeacherActivityTagMaxHoursContinuously* cn=new ConstraintTeacherActivityTagMaxHoursContinuously();
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Maximum_Hours_Continuously"){
							cn->maxHoursContinuously=elem4.text().toInt();
							xmlReadingLog+="    Read maxHoursContinuously="+QString::number(cn->maxHoursContinuously)+"\n";
						}
						else if(elem4.tagName()=="Teacher_Name"){
							cn->teacherName=elem4.text();
							xmlReadingLog+="    Read teacher name="+cn->teacherName+"\n";
						}
						else if(elem4.tagName()=="Activity_Tag_Name"){
							cn->activityTagName=elem4.text();
							xmlReadingLog+="    Read activity tag name="+cn->activityTagName+"\n";
						}
					}
					return cn;
}

TimeConstraint* Rules::readTeachersActivityTagMaxHoursContinuously(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintTeachersActivityTagMaxHoursContinuously");
					ConstraintTeachersActivityTagMaxHoursContinuously* cn=new ConstraintTeachersActivityTagMaxHoursContinuously();
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Maximum_Hours_Continuously"){
							cn->maxHoursContinuously=elem4.text().toInt();
							xmlReadingLog+="    Read maxHoursContinuously="+QString::number(cn->maxHoursContinuously)+"\n";
						}
						else if(elem4.tagName()=="Activity_Tag_Name"){
							cn->activityTagName=elem4.text();
							xmlReadingLog+="    Read activity tag name="+cn->activityTagName+"\n";
						}
					}
					return cn;
}

TimeConstraint* Rules::readTeacherActivityTagMaxHoursDaily(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintTeacherActivityTagMaxHoursDaily");
					ConstraintTeacherActivityTagMaxHoursDaily* cn=new ConstraintTeacherActivityTagMaxHoursDaily();
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Maximum_Hours_Daily"){
							cn->maxHoursDaily=elem4.text().toInt();
							xmlReadingLog+="    Read maxHoursDaily="+QString::number(cn->maxHoursDaily)+"\n";
						}
						else if(elem4.tagName()=="Teacher_Name"){
							cn->teacherName=elem4.text();
							xmlReadingLog+="    Read teacher name="+cn->teacherName+"\n";
						}
						else if(elem4.tagName()=="Activity_Tag_Name"){
							cn->activityTagName=elem4.text();
							xmlReadingLog+="    Read activity tag name="+cn->activityTagName+"\n";
						}
					}
					return cn;
}

TimeConstraint* Rules::readTeachersActivityTagMaxHoursDaily(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintTeachersActivityTagMaxHoursDaily");
					ConstraintTeachersActivityTagMaxHoursDaily* cn=new ConstraintTeachersActivityTagMaxHoursDaily();
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Maximum_Hours_Daily"){
							cn->maxHoursDaily=elem4.text().toInt();
							xmlReadingLog+="    Read maxHoursDaily="+QString::number(cn->maxHoursDaily)+"\n";
						}
						else if(elem4.tagName()=="Activity_Tag_Name"){
							cn->activityTagName=elem4.text();
							xmlReadingLog+="    Read activity tag name="+cn->activityTagName+"\n";
						}
					}
					return cn;
}

TimeConstraint* Rules::readTeachersMinHoursDaily(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintTeachersMinHoursDaily");
					ConstraintTeachersMinHoursDaily* cn=new ConstraintTeachersMinHoursDaily();
					cn->allowEmptyDays=true;
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight"){
							//cn->weight=elem4.text().toDouble();
							xmlReadingLog+="    Ignoring old tag - weight - making weight percentage=100\n";
							cn->weightPercentage=100;
						}
						else if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Compulsory"){
							if(elem4.text()=="yes"){
								//cn->compulsory=true;
								xmlReadingLog+="    Ignoring old tag - Current constraint is compulsory\n";
								cn->weightPercentage=100;
							}
							else{
								//cn->compulsory=false;
								xmlReadingLog+="    Old tag - current constraint is not compulsory - making weightPercentage=0%\n";
								cn->weightPercentage=0;
							}
						}
						else if(elem4.tagName()=="Minimum_Hours_Daily"){
							cn->minHoursDaily=elem4.text().toInt();
							xmlReadingLog+="    Read minHoursDaily="+QString::number(cn->minHoursDaily)+"\n";
						}
						else if(elem4.tagName()=="Allow_Empty_Days"){
							if(elem4.text()=="true" || elem4.text()=="1" || elem4.text()=="yes"){
								xmlReadingLog+="    Read allow empty days=true\n";
								cn->allowEmptyDays=true;
							}
							else{
								if(!(elem4.text()=="no" || elem4.text()=="false" || elem4.text()=="0")){
									QMessageBox::warning(NULL, tr("FET warning"),
									 tr("Found constraint teachers min hours daily with tag allow empty days"
									 " which is not 'true', 'false', 'yes', 'no', '1' or '0'."
									 " The tag will be considered false",
									 "Instructions for translators: please leave the 'true', 'false', 'yes' and 'no' fields untranslated, as they are in English"));
								}
								//assert(elem4.text()=="false" || elem4.text()=="0" || elem4.text()=="no");
								xmlReadingLog+="    Read allow empty days=false\n";
								cn->allowEmptyDays=false;
							}
						}
					}
					return cn;
}

TimeConstraint* Rules::readTeacherMinHoursDaily(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintTeacherMinHoursDaily");
					ConstraintTeacherMinHoursDaily* cn=new ConstraintTeacherMinHoursDaily();
					cn->allowEmptyDays=true;
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight"){
							//cn->weight=elem4.text().toDouble();
							xmlReadingLog+="    Ignoring old tag - weight - making weight percentage=100\n";
							cn->weightPercentage=100;
						}
						else if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Compulsory"){
							if(elem4.text()=="yes"){
								//cn->compulsory=true;
								xmlReadingLog+="    Ignoring old tag - Current constraint is compulsory\n";
								cn->weightPercentage=100;
							}
							else{
								//cn->compulsory=false;
								xmlReadingLog+="    Old tag - current constraint is not compulsory - making weightPercentage=0%\n";
								cn->weightPercentage=0;
							}
						}
						else if(elem4.tagName()=="Minimum_Hours_Daily"){
							cn->minHoursDaily=elem4.text().toInt();
							xmlReadingLog+="    Read minHoursDaily="+QString::number(cn->minHoursDaily)+"\n";
						}
						else if(elem4.tagName()=="Teacher_Name"){
							cn->teacherName=elem4.text();
							xmlReadingLog+="    Read teacher name="+cn->teacherName+"\n";
						}
						else if(elem4.tagName()=="Allow_Empty_Days"){
							if(elem4.text()=="true" || elem4.text()=="1" || elem4.text()=="yes"){
								xmlReadingLog+="    Read allow empty days=true\n";
								cn->allowEmptyDays=true;
							}
							else{
								if(!(elem4.text()=="no" || elem4.text()=="false" || elem4.text()=="0")){
									QMessageBox::warning(NULL, tr("FET warning"),
									 tr("Found constraint teacher min hours daily with tag allow empty days"
									 " which is not 'true', 'false', 'yes', 'no', '1' or '0'."
									 " The tag will be considered false",
									 "Instructions for translators: please leave the 'true', 'false', 'yes' and 'no' fields untranslated, as they are in English"));
								}
								//assert(elem4.text()=="false" || elem4.text()=="0" || elem4.text()=="no");
								xmlReadingLog+="    Read allow empty days=false\n";
								cn->allowEmptyDays=false;
							}
						}
					}
					return cn;
}

TimeConstraint* Rules::readStudentsMaxHoursDaily(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintStudentsMaxHoursDaily");
					ConstraintStudentsMaxHoursDaily* cn=new ConstraintStudentsMaxHoursDaily();
					cn->maxHoursDaily=-1;
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight"){
							//cn->weight=elem4.text().toDouble();
							xmlReadingLog+="    Ignoring old tag - weight - making weight percentage=100\n";
							cn->weightPercentage=100;
						}
						else if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Compulsory"){
							if(elem4.text()=="yes"){
								//cn->compulsory=true;
								xmlReadingLog+="    Ignoring old tag - Current constraint is compulsory\n";
								cn->weightPercentage=100;
							}
							else{
								//cn->compulsory=false;
								xmlReadingLog+="    Old tag - current constraint is not compulsory - making weightPercentage=0%\n";
								cn->weightPercentage=0;
							}
						}
						else if(elem4.tagName()=="Maximum_Hours_Daily"){
							cn->maxHoursDaily=elem4.text().toInt();
							xmlReadingLog+="    Read maxHoursDaily="+QString::number(cn->maxHoursDaily)+"\n";
						}
					}
					assert(cn->maxHoursDaily>=0);
					return cn;
}

TimeConstraint* Rules::readStudentsSetMaxHoursDaily(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintStudentsSetMaxHoursDaily");
					ConstraintStudentsSetMaxHoursDaily* cn=new ConstraintStudentsSetMaxHoursDaily();
					cn->maxHoursDaily=-1;
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight"){
							//cn->weight=elem4.text().toDouble();
							xmlReadingLog+="    Ignoring old tag - weight - making weight percentage=100\n";
							cn->weightPercentage=100;
						}
						else if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Compulsory"){
							if(elem4.text()=="yes"){
								//cn->compulsory=true;
								xmlReadingLog+="    Ignoring old tag - Current constraint is compulsory\n";
								cn->weightPercentage=100;
							}
							else{
								//cn->compulsory=false;
								xmlReadingLog+="    Old tag - current constraint is not compulsory - making weightPercentage=0%\n";
								cn->weightPercentage=0;
							}
						}
						else if(elem4.tagName()=="Maximum_Hours_Daily"){
							cn->maxHoursDaily=elem4.text().toInt();
							xmlReadingLog+="    Read maxHoursDaily="+QString::number(cn->maxHoursDaily)+"\n";
						}
						else if(elem4.tagName()=="Students"){
							cn->students=elem4.text();
							xmlReadingLog+="    Read students name="+cn->students+"\n";
						}
					}
					assert(cn->maxHoursDaily>=0);
					return cn;
}

TimeConstraint* Rules::readStudentsMaxHoursContinuously(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintStudentsMaxHoursContinuously");
					ConstraintStudentsMaxHoursContinuously* cn=new ConstraintStudentsMaxHoursContinuously();
					cn->maxHoursContinuously=-1;
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight"){
							//cn->weight=elem4.text().toDouble();
							xmlReadingLog+="    Ignoring old tag - weight - making weight percentage=100\n";
							cn->weightPercentage=100;
						}
						else if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Compulsory"){
							if(elem4.text()=="yes"){
								//cn->compulsory=true;
								xmlReadingLog+="    Ignoring old tag - Current constraint is compulsory\n";
								cn->weightPercentage=100;
							}
							else{
								//cn->compulsory=false;
								xmlReadingLog+="    Old tag - current constraint is not compulsory - making weightPercentage=0%\n";
								cn->weightPercentage=0;
							}
						}
						else if(elem4.tagName()=="Maximum_Hours_Continuously"){
							cn->maxHoursContinuously=elem4.text().toInt();
							xmlReadingLog+="    Read maxHoursContinuously="+QString::number(cn->maxHoursContinuously)+"\n";
						}
					}
					assert(cn->maxHoursContinuously>=0);
					return cn;
}

TimeConstraint* Rules::readStudentsSetMaxHoursContinuously(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintStudentsSetMaxHoursContinuously");
					ConstraintStudentsSetMaxHoursContinuously* cn=new ConstraintStudentsSetMaxHoursContinuously();
					cn->maxHoursContinuously=-1;
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight"){
							//cn->weight=elem4.text().toDouble();
							xmlReadingLog+="    Ignoring old tag - weight - making weight percentage=100\n";
							cn->weightPercentage=100;
						}
						else if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Compulsory"){
							if(elem4.text()=="yes"){
								//cn->compulsory=true;
								xmlReadingLog+="    Ignoring old tag - Current constraint is compulsory\n";
								cn->weightPercentage=100;
							}
							else{
								//cn->compulsory=false;
								xmlReadingLog+="    Old tag - current constraint is not compulsory - making weightPercentage=0%\n";
								cn->weightPercentage=0;
							}
						}
						else if(elem4.tagName()=="Maximum_Hours_Continuously"){
							cn->maxHoursContinuously=elem4.text().toInt();
							xmlReadingLog+="    Read maxHoursContinuously="+QString::number(cn->maxHoursContinuously)+"\n";
						}
						else if(elem4.tagName()=="Students"){
							cn->students=elem4.text();
							xmlReadingLog+="    Read students name="+cn->students+"\n";
						}
					}
					assert(cn->maxHoursContinuously>=0);
					return cn;
}

TimeConstraint* Rules::readStudentsSetActivityTagMaxHoursContinuously(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintStudentsSetActivityTagMaxHoursContinuously");
					ConstraintStudentsSetActivityTagMaxHoursContinuously* cn=new ConstraintStudentsSetActivityTagMaxHoursContinuously();
					cn->maxHoursContinuously=-1;
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Maximum_Hours_Continuously"){
							cn->maxHoursContinuously=elem4.text().toInt();
							xmlReadingLog+="    Read maxHoursContinuously="+QString::number(cn->maxHoursContinuously)+"\n";
						}
						else if(elem4.tagName()=="Students"){
							cn->students=elem4.text();
							xmlReadingLog+="    Read students name="+cn->students+"\n";
						}
						else if(elem4.tagName()=="Activity_Tag"){
							cn->activityTagName=elem4.text();
							xmlReadingLog+="    Read activity tag name="+cn->activityTagName+"\n";
						}
					}
					assert(cn->maxHoursContinuously>=0);
					return cn;
}

TimeConstraint* Rules::readStudentsActivityTagMaxHoursContinuously(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintStudentsActivityTagMaxHoursContinuously");
					ConstraintStudentsActivityTagMaxHoursContinuously* cn=new ConstraintStudentsActivityTagMaxHoursContinuously();
					cn->maxHoursContinuously=-1;
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Maximum_Hours_Continuously"){
							cn->maxHoursContinuously=elem4.text().toInt();
							xmlReadingLog+="    Read maxHoursContinuously="+QString::number(cn->maxHoursContinuously)+"\n";
						}
						else if(elem4.tagName()=="Activity_Tag"){
							cn->activityTagName=elem4.text();
							xmlReadingLog+="    Read activity tag name="+cn->activityTagName+"\n";
						}
					}
					assert(cn->maxHoursContinuously>=0);
					return cn;
}

TimeConstraint* Rules::readStudentsSetActivityTagMaxHoursDaily(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintStudentsSetActivityTagMaxHoursDaily");
					ConstraintStudentsSetActivityTagMaxHoursDaily* cn=new ConstraintStudentsSetActivityTagMaxHoursDaily();
					cn->maxHoursDaily=-1;
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Maximum_Hours_Daily"){
							cn->maxHoursDaily=elem4.text().toInt();
							xmlReadingLog+="    Read maxHoursDaily="+QString::number(cn->maxHoursDaily)+"\n";
						}
						else if(elem4.tagName()=="Students"){
							cn->students=elem4.text();
							xmlReadingLog+="    Read students name="+cn->students+"\n";
						}
						else if(elem4.tagName()=="Activity_Tag"){
							cn->activityTagName=elem4.text();
							xmlReadingLog+="    Read activity tag name="+cn->activityTagName+"\n";
						}
					}
					assert(cn->maxHoursDaily>=0);
					return cn;
}

TimeConstraint* Rules::readStudentsActivityTagMaxHoursDaily(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintStudentsActivityTagMaxHoursDaily");
					ConstraintStudentsActivityTagMaxHoursDaily* cn=new ConstraintStudentsActivityTagMaxHoursDaily();
					cn->maxHoursDaily=-1;
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Maximum_Hours_Daily"){
							cn->maxHoursDaily=elem4.text().toInt();
							xmlReadingLog+="    Read maxHoursDaily="+QString::number(cn->maxHoursDaily)+"\n";
						}
						else if(elem4.tagName()=="Activity_Tag"){
							cn->activityTagName=elem4.text();
							xmlReadingLog+="    Read activity tag name="+cn->activityTagName+"\n";
						}
					}
					assert(cn->maxHoursDaily>=0);
					return cn;
}

TimeConstraint* Rules::readStudentsMinHoursDaily(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintStudentsMinHoursDaily");
					ConstraintStudentsMinHoursDaily* cn=new ConstraintStudentsMinHoursDaily();
					cn->minHoursDaily=-1;
					cn->allowEmptyDays=false;
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight"){
							//cn->weight=elem4.text().toDouble();
							xmlReadingLog+="    Ignoring old tag - weight - making weight percentage=100\n";
							cn->weightPercentage=100;
						}
						else if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Compulsory"){
							if(elem4.text()=="yes"){
								//cn->compulsory=true;
								xmlReadingLog+="    Ignoring old tag - Current constraint is compulsory\n";
								cn->weightPercentage=100;
							}
							else{
								//cn->compulsory=false;
								xmlReadingLog+="    Old tag - current constraint is not compulsory - making weightPercentage=0%\n";
								cn->weightPercentage=0;
							}
						}
						else if(elem4.tagName()=="Minimum_Hours_Daily"){
							cn->minHoursDaily=elem4.text().toInt();
							xmlReadingLog+="    Read minHoursDaily="+QString::number(cn->minHoursDaily)+"\n";
						}
						else if(elem4.tagName()=="Allow_Empty_Days"){
							if(elem4.text()=="true" || elem4.text()=="1" || elem4.text()=="yes"){
								xmlReadingLog+="    Read allow empty days=true\n";
								cn->allowEmptyDays=true;
							}
							else{
								if(!(elem4.text()=="no" || elem4.text()=="false" || elem4.text()=="0")){
									QMessageBox::warning(NULL, tr("FET warning"),
									 tr("Found constraint students min hours daily with tag allow empty days"
									 " which is not 'true', 'false', 'yes', 'no', '1' or '0'."
									 " The tag will be considered false",
									 "Instructions for translators: please leave the 'true', 'false', 'yes' and 'no' fields untranslated, as they are in English"));
								}
								//assert(elem4.text()=="false" || elem4.text()=="0" || elem4.text()=="no");
								xmlReadingLog+="    Read allow empty days=false\n";
								cn->allowEmptyDays=false;
							}
						}
					}
					assert(cn->minHoursDaily>=0);
					return cn;
}

TimeConstraint* Rules::readStudentsSetMinHoursDaily(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintStudentsSetMinHoursDaily");
					ConstraintStudentsSetMinHoursDaily* cn=new ConstraintStudentsSetMinHoursDaily();
					cn->minHoursDaily=-1;
					cn->allowEmptyDays=false;
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight"){
							//cn->weight=elem4.text().toDouble();
							xmlReadingLog+="    Ignoring old tag - weight - making weight percentage=100\n";
							cn->weightPercentage=100;
						}
						else if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Compulsory"){
							if(elem4.text()=="yes"){
								//cn->compulsory=true;
								xmlReadingLog+="    Ignoring old tag - Current constraint is compulsory\n";
								cn->weightPercentage=100;
							}
							else{
								//cn->compulsory=false;
								xmlReadingLog+="    Old tag - current constraint is not compulsory - making weightPercentage=0%\n";
								cn->weightPercentage=0;
							}
						}
						else if(elem4.tagName()=="Minimum_Hours_Daily"){
							cn->minHoursDaily=elem4.text().toInt();
							xmlReadingLog+="    Read minHoursDaily="+QString::number(cn->minHoursDaily)+"\n";
						}
						else if(elem4.tagName()=="Students"){
							cn->students=elem4.text();
							xmlReadingLog+="    Read students name="+cn->students+"\n";
						}
						else if(elem4.tagName()=="Allow_Empty_Days"){
							if(elem4.text()=="true" || elem4.text()=="1" || elem4.text()=="yes"){
								xmlReadingLog+="    Read allow empty days=true\n";
								cn->allowEmptyDays=true;
							}
							else{
								if(!(elem4.text()=="no" || elem4.text()=="false" || elem4.text()=="0")){
									QMessageBox::warning(NULL, tr("FET warning"),
									 tr("Found constraint students set min hours daily with tag allow empty days"
									 " which is not 'true', 'false', 'yes', 'no', '1' or '0'."
									 " The tag will be considered false",
									 "Instructions for translators: please leave the 'true', 'false', 'yes' and 'no' fields untranslated, as they are in English"));
								}
								//assert(elem4.text()=="false" || elem4.text()=="0" || elem4.text()=="no");
								xmlReadingLog+="    Read allow empty days=false\n";
								cn->allowEmptyDays=false;
							}
						}
					}
					assert(cn->minHoursDaily>=0);
					return cn;
}

TimeConstraint* Rules::readActivityPreferredTime(const QDomElement& elem3, FakeString& xmlReadingLog,
	bool& reportUnspecifiedPermanentlyLockedTime, bool& reportUnspecifiedDayOrHourPreferredStartingTime){
					assert(elem3.tagName()=="ConstraintActivityPreferredTime");
				
					ConstraintActivityPreferredStartingTime* cn=new ConstraintActivityPreferredStartingTime();
					cn->day = cn->hour = -1;
					cn->permanentlyLocked=false; //default not locked
					bool foundLocked=false;
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight"){
							//cn->weight=elem4.text().toDouble();
							xmlReadingLog+="    Ignoring old tag - weight - making weight percentage=100\n";
							cn->weightPercentage=100;
						}
						else if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Compulsory"){
							if(elem4.text()=="yes"){
								//cn->compulsory=true;
								xmlReadingLog+="    Ignoring old tag - Current constraint is compulsory\n";
								cn->weightPercentage=100;
							}
							else{
								//cn->compulsory=false;
								xmlReadingLog+="    Old tag - current constraint is not compulsory - making weightPercentage=0%\n";
								cn->weightPercentage=0;
							}
						}
						else if(elem4.tagName()=="Permanently_Locked"){
							if(elem4.text()=="true" || elem4.text()=="1" || elem4.text()=="yes"){
								xmlReadingLog+="    Permanently locked\n";
								cn->permanentlyLocked=true;
							}
							else{
								if(!(elem4.text()=="no" || elem4.text()=="false" || elem4.text()=="0")){
									QMessageBox::warning(NULL, tr("FET warning"),
									 tr("Found constraint activity preferred starting time with tag permanently locked"
									 " which is not 'true', 'false', 'yes', 'no', '1' or '0'."
									 " The tag will be considered false",
									 "Instructions for translators: please leave the 'true', 'false', 'yes' and 'no' fields untranslated, as they are in English"));
								}
								//assert(elem4.text()=="false" || elem4.text()=="0" || elem4.text()=="no");
								xmlReadingLog+="    Not permanently locked\n";
								cn->permanentlyLocked=false;
							}
							foundLocked=true;
						}
						else if(elem4.tagName()=="Activity_Id"){
							cn->activityId=elem4.text().toInt();
							xmlReadingLog+="    Read activity id="+QString::number(cn->activityId)+"\n";
						}
						else if(elem4.tagName()=="Preferred_Day"){
							for(cn->day=0; cn->day<this->nDaysPerWeek; cn->day++)
								if(this->daysOfTheWeek[cn->day]==elem4.text())
									break;
							if(cn->day>=this->nDaysPerWeek){
								QMessageBox::information(NULL, tr("FET information"), 
								 tr("Constraint ActivityPreferredTime day corrupt for activity with id %1, day %2 is inexistent ... ignoring constraint")
								 .arg(cn->activityId)
								 .arg(elem4.text()));
								delete cn;
								cn=NULL;
								//goto corruptConstraintTime;
								return NULL;
							}
							assert(cn->day<this->nDaysPerWeek);
							xmlReadingLog+="    Preferred day="+this->daysOfTheWeek[cn->day]+"\n";
						}
						else if(elem4.tagName()=="Preferred_Hour"){
							for(cn->hour=0; cn->hour < this->nHoursPerDay; cn->hour++)
								if(this->hoursOfTheDay[cn->hour]==elem4.text())
									break;
							if(cn->hour>=this->nHoursPerDay){
								QMessageBox::information(NULL, tr("FET information"), 
								 tr("Constraint ActivityPreferredTime hour corrupt for activity with id %1, hour %2 is inexistent ... ignoring constraint")
								 .arg(cn->activityId)
								 .arg(elem4.text()));
								delete cn;
								cn=NULL;
								//goto corruptConstraintTime;
								return NULL;
							}
							assert(cn->hour>=0 && cn->hour < this->nHoursPerDay);
							xmlReadingLog+="    Preferred hour="+this->hoursOfTheDay[cn->hour]+"\n";
						}
					}
					//crt_constraint=cn;

					if(cn->hour>=0 && cn->day>=0 && !foundLocked && reportUnspecifiedPermanentlyLockedTime){
						int t=QMessageBox::information(NULL, tr("FET information"),
						 tr("Found constraint activity preferred starting time, with unspecified tag"
						 " 'permanently locked' - this tag will be set to 'false' by default. You can always modify it"
						 " by editing the constraint in the 'Data' menu")+"\n\n"
						 +tr("Explanation: starting with version 5.8.0 (January 2009), the constraint"
						 " activity preferred starting time has"
						 " a new tag, 'permanently locked' (true or false)."
						 " It is recommended to make the tag 'permanently locked' true for the constraints you"
						 " need to be not modifiable from the 'Timetable' menu"
						 " and leave this tag false for the constraints you need to be modifiable from the 'Timetable' menu"
						 " (the 'permanently locked' tag can be modified by editing the constraint from the 'Data' menu)."
						 " This way, when viewing the timetable"
						 " and locking/unlocking some activities, you will not unlock the constraints which"
						 " need to be locked all the time."
						 ),
						  tr("Skip rest"), tr("See next"), QString(), 1, 0 );
						if(t==0)
							reportUnspecifiedPermanentlyLockedTime=false;
					}
					
					if(cn->hour==-1 || cn->day==-1){
						if(reportUnspecifiedDayOrHourPreferredStartingTime){
							int t=QMessageBox::information(NULL, tr("FET information"),
							 tr("Found constraint activity preferred starting time, with unspecified day or hour."
							  " This constraint will be transformed into constraint activity preferred starting times (a set of times, not only one)."
							  " This change is done in FET versions 5.8.1 and higher."
							 ),
							  tr("Skip rest"), tr("See next"), QString(), 1, 0 );
							if(t==0)
								reportUnspecifiedDayOrHourPreferredStartingTime=false;
						}
							
						ConstraintActivityPreferredStartingTimes* cgood=new ConstraintActivityPreferredStartingTimes();
						if(cn->day==-1){
							cgood->activityId=cn->activityId;
							cgood->weightPercentage=cn->weightPercentage;
							cgood->nPreferredStartingTimes_L=this->nDaysPerWeek;
							for(int i=0; i<cgood->nPreferredStartingTimes_L; i++){
								/*cgood->days[i]=i;
								cgood->hours[i]=cn->hour;*/
								cgood->days_L.append(i);
								cgood->hours_L.append(cn->hour);
							}
						}
						else{
							assert(cn->hour==-1);
							cgood->activityId=cn->activityId;
							cgood->weightPercentage=cn->weightPercentage;
							cgood->nPreferredStartingTimes_L=this->nHoursPerDay;
							for(int i=0; i<cgood->nPreferredStartingTimes_L; i++){
								/*cgood->days[i]=cn->day;
								cgood->hours[i]=i;*/
								cgood->days_L.append(cn->day);
								cgood->hours_L.append(i);
							}
						}
						
						delete cn;
						return cgood;
					}
					
					return cn;
}

TimeConstraint* Rules::readActivityPreferredStartingTime(const QDomElement& elem3, FakeString& xmlReadingLog,
	bool& reportUnspecifiedPermanentlyLockedTime, bool& reportUnspecifiedDayOrHourPreferredStartingTime){
					assert(elem3.tagName()=="ConstraintActivityPreferredStartingTime");
					ConstraintActivityPreferredStartingTime* cn=new ConstraintActivityPreferredStartingTime();
					cn->day = cn->hour = -1;
					cn->permanentlyLocked=false; //default false
					bool foundLocked=false;
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight"){
							//cn->weight=elem4.text().toDouble();
							xmlReadingLog+="    Ignoring old tag - weight - making weight percentage=100\n";
							cn->weightPercentage=100;
						}
						else if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Compulsory"){
							if(elem4.text()=="yes"){
								//cn->compulsory=true;
								xmlReadingLog+="    Ignoring old tag - Current constraint is compulsory\n";
								cn->weightPercentage=100;
							}
							else{
								//cn->compulsory=false;
								xmlReadingLog+="    Old tag - current constraint is not compulsory - making weightPercentage=0%\n";
								cn->weightPercentage=0;
							}
						}
						else if(elem4.tagName()=="Permanently_Locked"){
							if(elem4.text()=="true" || elem4.text()=="1" || elem4.text()=="yes"){
								xmlReadingLog+="    Permanently locked\n";
								cn->permanentlyLocked=true;
							}
							else{
								if(!(elem4.text()=="no" || elem4.text()=="false" || elem4.text()=="0")){
									QMessageBox::warning(NULL, tr("FET warning"),
									 tr("Found constraint activity preferred starting time with tag permanently locked"
									 " which is not 'true', 'false', 'yes', 'no', '1' or '0'."
									 " The tag will be considered false",
									 "Instructions for translators: please leave the 'true', 'false', 'yes' and 'no' fields untranslated, as they are in English"));
								}
								//assert(elem4.text()=="false" || elem4.text()=="0" || elem4.text()=="no");
								xmlReadingLog+="    Not permanently locked\n";
								cn->permanentlyLocked=false;
							}
							foundLocked=true;
						}
						else if(elem4.tagName()=="Activity_Id"){
							cn->activityId=elem4.text().toInt();
							xmlReadingLog+="    Read activity id="+QString::number(cn->activityId)+"\n";
						}
						else if(elem4.tagName()=="Preferred_Day"){
							for(cn->day=0; cn->day<this->nDaysPerWeek; cn->day++)
								if(this->daysOfTheWeek[cn->day]==elem4.text())
									break;
							if(cn->day>=this->nDaysPerWeek){
								QMessageBox::information(NULL, tr("FET information"), 
								 tr("Constraint ActivityPreferredStartingTime day corrupt for activity with id %1, day %2 is inexistent ... ignoring constraint")
								 .arg(cn->activityId)
								 .arg(elem4.text()));
								delete cn;
								cn=NULL;
								//goto corruptConstraintTime;
								return NULL;
							}
							assert(cn->day<this->nDaysPerWeek);
							xmlReadingLog+="    Preferred day="+this->daysOfTheWeek[cn->day]+"\n";
						}
						else if(elem4.tagName()=="Preferred_Hour"){
							for(cn->hour=0; cn->hour < this->nHoursPerDay; cn->hour++)
								if(this->hoursOfTheDay[cn->hour]==elem4.text())
									break;
							if(cn->hour>=this->nHoursPerDay){
								QMessageBox::information(NULL, tr("FET information"), 
								 tr("Constraint ActivityPreferredStartingTime hour corrupt for activity with id %1, hour %2 is inexistent ... ignoring constraint")
								 .arg(cn->activityId)
								 .arg(elem4.text()));
								delete cn;
								cn=NULL;
								//goto corruptConstraintTime;
								return NULL;
							}
							assert(cn->hour>=0 && cn->hour < this->nHoursPerDay);
							xmlReadingLog+="    Preferred hour="+this->hoursOfTheDay[cn->hour]+"\n";
						}
					}
					//crt_constraint=cn;

					if(cn->hour>=0 && cn->day>=0 && !foundLocked && reportUnspecifiedPermanentlyLockedTime){
						int t=QMessageBox::information(NULL, tr("FET information"),
						 tr("Found constraint activity preferred starting time, with unspecified tag"
						 " 'permanently locked' - this tag will be set to 'false' by default. You can always modify it"
						 " by editing the constraint in the 'Data' menu")+"\n\n"
						 +tr("Explanation: starting with version 5.8.0 (January 2009), the constraint"
						 " activity preferred starting time has"
						 " a new tag, 'permanently locked' (true or false)."
						 " It is recommended to make the tag 'permanently locked' true for the constraints you"
						 " need to be not modifiable from the 'Timetable' menu"
						 " and leave this tag false for the constraints you need to be modifiable from the 'Timetable' menu"
						 " (the 'permanently locked' tag can be modified by editing the constraint from the 'Data' menu)."
						 " This way, when viewing the timetable"
						 " and locking/unlocking some activities, you will not unlock the constraints which"
						 " need to be locked all the time."
						 ),
						  tr("Skip rest"), tr("See next"), QString(), 1, 0 );
						if(t==0)
							reportUnspecifiedPermanentlyLockedTime=false;
					}

					if(cn->hour==-1 || cn->day==-1){
						if(reportUnspecifiedDayOrHourPreferredStartingTime){
							int t=QMessageBox::information(NULL, tr("FET information"),
							 tr("Found constraint activity preferred starting time, with unspecified day or hour."
							  " This constraint will be transformed into constraint activity preferred starting times (a set of times, not only one)."
							  " This change is done in FET versions 5.8.1 and higher."
							 ),
							  tr("Skip rest"), tr("See next"), QString(), 1, 0 );
							if(t==0)
								reportUnspecifiedDayOrHourPreferredStartingTime=false;
						}
							
						ConstraintActivityPreferredStartingTimes* cgood=new ConstraintActivityPreferredStartingTimes();
						if(cn->day==-1){
							cgood->activityId=cn->activityId;
							cgood->weightPercentage=cn->weightPercentage;
							cgood->nPreferredStartingTimes_L=this->nDaysPerWeek;
							for(int i=0; i<cgood->nPreferredStartingTimes_L; i++){
								/*cgood->days[i]=i;
								cgood->hours[i]=cn->hour;*/
								cgood->days_L.append(i);
								cgood->hours_L.append(cn->hour);
							}
						}
						else{
							assert(cn->hour==-1);
							cgood->activityId=cn->activityId;
							cgood->weightPercentage=cn->weightPercentage;
							cgood->nPreferredStartingTimes_L=this->nHoursPerDay;
							for(int i=0; i<cgood->nPreferredStartingTimes_L; i++){
								/*cgood->days[i]=cn->day;
								cgood->hours[i]=i;*/
								cgood->days_L.append(cn->day);
								cgood->hours_L.append(i);
							}
						}
						
						delete cn;
						return cgood;
					}
					
					return cn;
}

TimeConstraint* Rules::readActivityEndsStudentsDay(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintActivityEndsStudentsDay");
					ConstraintActivityEndsStudentsDay* cn=new ConstraintActivityEndsStudentsDay();
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Activity_Id"){
							cn->activityId=elem4.text().toInt();
							xmlReadingLog+="    Read activity id="+QString::number(cn->activityId)+"\n";
						}
					}
					return cn;
}

TimeConstraint* Rules::readActivitiesEndStudentsDay(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintActivitiesEndStudentsDay");
					ConstraintActivitiesEndStudentsDay* cn=new ConstraintActivitiesEndStudentsDay();
					cn->teacherName="";
					cn->studentsName="";
					cn->subjectName="";
					cn->activityTagName="";
					
					//i=0;
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Teacher_Name"){
							cn->teacherName=elem4.text();
							xmlReadingLog+="    Read teacher name="+cn->teacherName+"\n";
						}
						else if(elem4.tagName()=="Students_Name"){
							cn->studentsName=elem4.text();
							xmlReadingLog+="    Read students name="+cn->studentsName+"\n";
						}
						else if(elem4.tagName()=="Subject_Name"){
							cn->subjectName=elem4.text();
							xmlReadingLog+="    Read subject name="+cn->subjectName+"\n";
						}
						else if(elem4.tagName()=="Subject_Tag_Name"){
							cn->activityTagName=elem4.text();
							xmlReadingLog+="    Read activity tag name="+cn->activityTagName+"\n";
						}
						else if(elem4.tagName()=="Activity_Tag_Name"){
							cn->activityTagName=elem4.text();
							xmlReadingLog+="    Read activity tag name="+cn->activityTagName+"\n";
						}
					}
					return cn;
}

TimeConstraint* Rules::read2ActivitiesConsecutive(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="Constraint2ActivitiesConsecutive");
					ConstraintTwoActivitiesConsecutive* cn=new ConstraintTwoActivitiesConsecutive();
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight"){
							//cn->weight=elem4.text().toDouble();
							xmlReadingLog+="    Ignoring old tag - weight - making weight percentage=100\n";
							cn->weightPercentage=100;
						}
						else if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Compulsory"){
							if(elem4.text()=="yes"){
								//cn->compulsory=true;
								xmlReadingLog+="    Ignoring old tag - Current constraint is compulsory\n";
								cn->weightPercentage=100;
							}
							else{
								//cn->compulsory=false;
								xmlReadingLog+="    Old tag - current constraint is not compulsory - making weightPercentage=0%\n";
								cn->weightPercentage=0;
							}
						}
						else if(elem4.tagName()=="First_Activity_Id"){
							cn->firstActivityId=elem4.text().toInt();
							xmlReadingLog+="    Read first activity id="+QString::number(cn->firstActivityId)+"\n";
						}
						else if(elem4.tagName()=="Second_Activity_Id"){
							cn->secondActivityId=elem4.text().toInt();
							xmlReadingLog+="    Read second activity id="+QString::number(cn->secondActivityId)+"\n";
						}
					}
					return cn;
}

TimeConstraint* Rules::read2ActivitiesGrouped(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="Constraint2ActivitiesGrouped");
					ConstraintTwoActivitiesGrouped* cn=new ConstraintTwoActivitiesGrouped();
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight"){
							//cn->weight=elem4.text().toDouble();
							xmlReadingLog+="    Ignoring old tag - weight - making weight percentage=100\n";
							cn->weightPercentage=100;
						}
						else if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Compulsory"){
							if(elem4.text()=="yes"){
								//cn->compulsory=true;
								xmlReadingLog+="    Ignoring old tag - Current constraint is compulsory\n";
								cn->weightPercentage=100;
							}
							else{
								//cn->compulsory=false;
								xmlReadingLog+="    Old tag - current constraint is not compulsory - making weightPercentage=0%\n";
								cn->weightPercentage=0;
							}
						}
						else if(elem4.tagName()=="First_Activity_Id"){
							cn->firstActivityId=elem4.text().toInt();
							xmlReadingLog+="    Read first activity id="+QString::number(cn->firstActivityId)+"\n";
						}
						else if(elem4.tagName()=="Second_Activity_Id"){
							cn->secondActivityId=elem4.text().toInt();
							xmlReadingLog+="    Read second activity id="+QString::number(cn->secondActivityId)+"\n";
						}
					}
					return cn;
}

TimeConstraint* Rules::read3ActivitiesGrouped(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="Constraint3ActivitiesGrouped");
					ConstraintThreeActivitiesGrouped* cn=new ConstraintThreeActivitiesGrouped();
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="First_Activity_Id"){
							cn->firstActivityId=elem4.text().toInt();
							xmlReadingLog+="    Read first activity id="+QString::number(cn->firstActivityId)+"\n";
						}
						else if(elem4.tagName()=="Second_Activity_Id"){
							cn->secondActivityId=elem4.text().toInt();
							xmlReadingLog+="    Read second activity id="+QString::number(cn->secondActivityId)+"\n";
						}
						else if(elem4.tagName()=="Third_Activity_Id"){
							cn->thirdActivityId=elem4.text().toInt();
							xmlReadingLog+="    Read third activity id="+QString::number(cn->thirdActivityId)+"\n";
						}
					}
					return cn;
}

TimeConstraint* Rules::read2ActivitiesOrdered(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="Constraint2ActivitiesOrdered");
					ConstraintTwoActivitiesOrdered* cn=new ConstraintTwoActivitiesOrdered();
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight"){
							//cn->weight=elem4.text().toDouble();
							xmlReadingLog+="    Ignoring old tag - weight - making weight percentage=100\n";
							cn->weightPercentage=100;
						}
						else if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Compulsory"){
							if(elem4.text()=="yes"){
								//cn->compulsory=true;
								xmlReadingLog+="    Ignoring old tag - Current constraint is compulsory\n";
								cn->weightPercentage=100;
							}
							else{
								//cn->compulsory=false;
								xmlReadingLog+="    Old tag - current constraint is not compulsory - making weightPercentage=0%\n";
								cn->weightPercentage=0;
							}
						}
						else if(elem4.tagName()=="First_Activity_Id"){
							cn->firstActivityId=elem4.text().toInt();
							xmlReadingLog+="    Read first activity id="+QString::number(cn->firstActivityId)+"\n";
						}
						else if(elem4.tagName()=="Second_Activity_Id"){
							cn->secondActivityId=elem4.text().toInt();
							xmlReadingLog+="    Read second activity id="+QString::number(cn->secondActivityId)+"\n";
						}
					}
					return cn;
}

TimeConstraint* Rules::readTwoActivitiesConsecutive(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintTwoActivitiesConsecutive");
					ConstraintTwoActivitiesConsecutive* cn=new ConstraintTwoActivitiesConsecutive();
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight"){
							//cn->weight=elem4.text().toDouble();
							xmlReadingLog+="    Ignoring old tag - weight - making weight percentage=100\n";
							cn->weightPercentage=100;
						}
						else if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Compulsory"){
							if(elem4.text()=="yes"){
								//cn->compulsory=true;
								xmlReadingLog+="    Ignoring old tag - Current constraint is compulsory\n";
								cn->weightPercentage=100;
							}
							else{
								//cn->compulsory=false;
								xmlReadingLog+="    Old tag - current constraint is not compulsory - making weightPercentage=0%\n";
								cn->weightPercentage=0;
							}
						}
						else if(elem4.tagName()=="First_Activity_Id"){
							cn->firstActivityId=elem4.text().toInt();
							xmlReadingLog+="    Read first activity id="+QString::number(cn->firstActivityId)+"\n";
						}
						else if(elem4.tagName()=="Second_Activity_Id"){
							cn->secondActivityId=elem4.text().toInt();
							xmlReadingLog+="    Read second activity id="+QString::number(cn->secondActivityId)+"\n";
						}
					}
					return cn;
}

TimeConstraint* Rules::readTwoActivitiesGrouped(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintTwoActivitiesGrouped");
					ConstraintTwoActivitiesGrouped* cn=new ConstraintTwoActivitiesGrouped();
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight"){
							//cn->weight=elem4.text().toDouble();
							xmlReadingLog+="    Ignoring old tag - weight - making weight percentage=100\n";
							cn->weightPercentage=100;
						}
						else if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Compulsory"){
							if(elem4.text()=="yes"){
								//cn->compulsory=true;
								xmlReadingLog+="    Ignoring old tag - Current constraint is compulsory\n";
								cn->weightPercentage=100;
							}
							else{
								//cn->compulsory=false;
								xmlReadingLog+="    Old tag - current constraint is not compulsory - making weightPercentage=0%\n";
								cn->weightPercentage=0;
							}
						}
						else if(elem4.tagName()=="First_Activity_Id"){
							cn->firstActivityId=elem4.text().toInt();
							xmlReadingLog+="    Read first activity id="+QString::number(cn->firstActivityId)+"\n";
						}
						else if(elem4.tagName()=="Second_Activity_Id"){
							cn->secondActivityId=elem4.text().toInt();
							xmlReadingLog+="    Read second activity id="+QString::number(cn->secondActivityId)+"\n";
						}
					}
					return cn;
}

TimeConstraint* Rules::readThreeActivitiesGrouped(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintThreeActivitiesGrouped");
					ConstraintThreeActivitiesGrouped* cn=new ConstraintThreeActivitiesGrouped();
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="First_Activity_Id"){
							cn->firstActivityId=elem4.text().toInt();
							xmlReadingLog+="    Read first activity id="+QString::number(cn->firstActivityId)+"\n";
						}
						else if(elem4.tagName()=="Second_Activity_Id"){
							cn->secondActivityId=elem4.text().toInt();
							xmlReadingLog+="    Read second activity id="+QString::number(cn->secondActivityId)+"\n";
						}
						else if(elem4.tagName()=="Third_Activity_Id"){
							cn->thirdActivityId=elem4.text().toInt();
							xmlReadingLog+="    Read third activity id="+QString::number(cn->thirdActivityId)+"\n";
						}
					}
					return cn;
}

TimeConstraint* Rules::readTwoActivitiesOrdered(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintTwoActivitiesOrdered");
					ConstraintTwoActivitiesOrdered* cn=new ConstraintTwoActivitiesOrdered();
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight"){
							//cn->weight=elem4.text().toDouble();
							xmlReadingLog+="    Ignoring old tag - weight - making weight percentage=100\n";
							cn->weightPercentage=100;
						}
						else if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Compulsory"){
							if(elem4.text()=="yes"){
								//cn->compulsory=true;
								xmlReadingLog+="    Ignoring old tag - Current constraint is compulsory\n";
								cn->weightPercentage=100;
							}
							else{
								//cn->compulsory=false;
								xmlReadingLog+="    Old tag - current constraint is not compulsory - making weightPercentage=0%\n";
								cn->weightPercentage=0;
							}
						}
						else if(elem4.tagName()=="First_Activity_Id"){
							cn->firstActivityId=elem4.text().toInt();
							xmlReadingLog+="    Read first activity id="+QString::number(cn->firstActivityId)+"\n";
						}
						else if(elem4.tagName()=="Second_Activity_Id"){
							cn->secondActivityId=elem4.text().toInt();
							xmlReadingLog+="    Read second activity id="+QString::number(cn->secondActivityId)+"\n";
						}
					}
					return cn;
}

TimeConstraint* Rules::readActivityPreferredTimes(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintActivityPreferredTimes");
				
					ConstraintActivityPreferredStartingTimes* cn=new ConstraintActivityPreferredStartingTimes();
					cn->nPreferredStartingTimes_L=0;
					int i;
					/*for(i=0; i<MAX_N_CONSTRAINT_ACTIVITY_PREFERRED_STARTING_TIMES; i++){
						cn->days[i] = cn->hours[i] = -1;
					}*/
					i=0;
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight"){
							//cn->weight=elem4.text().toDouble();
							xmlReadingLog+="    Ignoring old tag - weight - making weight percentage=100\n";
							cn->weightPercentage=100;
						}
						else if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Compulsory"){
							if(elem4.text()=="yes"){
								//cn->compulsory=true;
								xmlReadingLog+="    Ignoring old tag - Current constraint is compulsory\n";
								cn->weightPercentage=100;
							}
							else{
								//cn->compulsory=false;
								xmlReadingLog+="    Old tag - current constraint is not compulsory - making weightPercentage=0%\n";
								cn->weightPercentage=0;
							}
						}
						else if(elem4.tagName()=="Activity_Id"){
							cn->activityId=elem4.text().toInt();
							xmlReadingLog+="    Read activity id="+QString::number(cn->activityId)+"\n";
						}
						else if(elem4.tagName()=="Number_of_Preferred_Times"){
							cn->nPreferredStartingTimes_L=elem4.text().toInt();
							xmlReadingLog+="    Read number of preferred times="+QString::number(cn->nPreferredStartingTimes_L)+"\n";
						}
						else if(elem4.tagName()=="Preferred_Time"){
							xmlReadingLog+="    Read: preferred time\n";

							for(QDomNode node5=elem4.firstChild(); !node5.isNull(); node5=node5.nextSibling()){
								QDomElement elem5=node5.toElement();
								if(elem5.isNull()){
									xmlReadingLog+="    Null node here\n";
									continue;
								}
								xmlReadingLog+="    Found "+elem5.tagName()+" tag\n";
								if(elem5.tagName()=="Preferred_Day"){
									cn->days_L.append(0);
									assert(cn->days_L.count()-1==i);
									for(cn->days_L[i]=0; cn->days_L[i]<this->nDaysPerWeek; cn->days_L[i]++)
										if(this->daysOfTheWeek[cn->days_L[i]]==elem5.text())
											break;

									if(cn->days_L[i]>=this->nDaysPerWeek){
										QMessageBox::information(NULL, tr("FET information"), 
										 tr("Constraint ActivityPreferredTimes day corrupt for activity with id %1, day %2 is inexistent ... ignoring constraint")
										 .arg(cn->activityId)
										 .arg(elem5.text()));
										delete cn;
										cn=NULL;
										//goto corruptConstraintTime;
										return NULL;
									}
						
									assert(cn->days_L[i]<this->nDaysPerWeek);
									xmlReadingLog+="    Preferred day="+this->daysOfTheWeek[cn->days_L[i]]+"("+QString::number(i)+")"+"\n";
								}
								else if(elem5.tagName()=="Preferred_Hour"){
									cn->hours_L.append(0);
									assert(cn->hours_L.count()-1==i);
									for(cn->hours_L[i]=0; cn->hours_L[i] < this->nHoursPerDay; cn->hours_L[i]++)
										if(this->hoursOfTheDay[cn->hours_L[i]]==elem5.text())
											break;
									
									if(cn->hours_L[i]>=this->nHoursPerDay){
										QMessageBox::information(NULL, tr("FET information"), 
										 tr("Constraint ActivityPreferredTimes hour corrupt for activity with id %1, hour %2 is inexistent ... ignoring constraint")
										 .arg(cn->activityId)
										 .arg(elem5.text()));
										delete cn;
										cn=NULL;
										//goto corruptConstraintTime;
										return NULL;
									}
									
									assert(cn->hours_L[i]>=0 && cn->hours_L[i] < this->nHoursPerDay);
									xmlReadingLog+="    Preferred hour="+this->hoursOfTheDay[cn->hours_L[i]]+"\n";
								}
							}

							i++;
						}
					}
					assert(i==cn->nPreferredStartingTimes_L);
					return cn;
}

TimeConstraint* Rules::readActivityPreferredTimeSlots(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintActivityPreferredTimeSlots");
					ConstraintActivityPreferredTimeSlots* cn=new ConstraintActivityPreferredTimeSlots();
					cn->p_nPreferredTimeSlots_L=0;
					int i;
					/*for(i=0; i<MAX_N_CONSTRAINT_ACTIVITY_PREFERRED_TIME_SLOTS; i++){
						cn->p_days[i] = cn->p_hours[i] = -1;
					}*/
					i=0;
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight"){
							//cn->weight=elem4.text().toDouble();
							xmlReadingLog+="    Ignoring old tag - weight - making weight percentage=100\n";
							cn->weightPercentage=100;
						}
						else if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Compulsory"){
							if(elem4.text()=="yes"){
								//cn->compulsory=true;
								xmlReadingLog+="    Ignoring old tag - Current constraint is compulsory\n";
								cn->weightPercentage=100;
							}
							else{
								//cn->compulsory=false;
								xmlReadingLog+="    Old tag - current constraint is not compulsory - making weightPercentage=0%\n";
								cn->weightPercentage=0;
							}
						}
						else if(elem4.tagName()=="Activity_Id"){
							cn->p_activityId=elem4.text().toInt();
							xmlReadingLog+="    Read activity id="+QString::number(cn->p_activityId)+"\n";
						}
						else if(elem4.tagName()=="Number_of_Preferred_Time_Slots"){
							cn->p_nPreferredTimeSlots_L=elem4.text().toInt();
							xmlReadingLog+="    Read number of preferred times="+QString::number(cn->p_nPreferredTimeSlots_L)+"\n";
						}
						else if(elem4.tagName()=="Preferred_Time_Slot"){
							xmlReadingLog+="    Read: preferred time slot\n";

							for(QDomNode node5=elem4.firstChild(); !node5.isNull(); node5=node5.nextSibling()){
								QDomElement elem5=node5.toElement();
								if(elem5.isNull()){
									xmlReadingLog+="    Null node here\n";
									continue;
								}
								xmlReadingLog+="    Found "+elem5.tagName()+" tag\n";
								if(elem5.tagName()=="Preferred_Day"){
									cn->p_days_L.append(0);
									assert(cn->p_days_L.count()-1==i);
									for(cn->p_days_L[i]=0; cn->p_days_L[i]<this->nDaysPerWeek; cn->p_days_L[i]++)
										if(this->daysOfTheWeek[cn->p_days_L[i]]==elem5.text())
											break;

									if(cn->p_days_L[i]>=this->nDaysPerWeek){
										QMessageBox::information(NULL, tr("FET information"), 
										 tr("Constraint ActivityPreferredTimeSlots day corrupt for activity with id %1, day %2 is inexistent ... ignoring constraint")
										 .arg(cn->p_activityId)
										 .arg(elem5.text()));
										delete cn;
										cn=NULL;
										//goto corruptConstraintTime;
										return NULL;
									}
						
									assert(cn->p_days_L[i]<this->nDaysPerWeek);
									xmlReadingLog+="    Preferred day="+this->daysOfTheWeek[cn->p_days_L[i]]+"("+QString::number(i)+")"+"\n";
								}
								else if(elem5.tagName()=="Preferred_Hour"){
									cn->p_hours_L.append(0);
									assert(cn->p_hours_L.count()-1==i);
									for(cn->p_hours_L[i]=0; cn->p_hours_L[i] < this->nHoursPerDay; cn->p_hours_L[i]++)
										if(this->hoursOfTheDay[cn->p_hours_L[i]]==elem5.text())
											break;
									
									if(cn->p_hours_L[i]>=this->nHoursPerDay){
										QMessageBox::information(NULL, tr("FET information"), 
										 tr("Constraint ActivityPreferredTimeSlots hour corrupt for activity with id %1, hour %2 is inexistent ... ignoring constraint")
										 .arg(cn->p_activityId)
										 .arg(elem5.text()));
										delete cn;
										cn=NULL;
										//goto corruptConstraintTime;
										return NULL;
									}
									
									assert(cn->p_hours_L[i]>=0 && cn->p_hours_L[i] < this->nHoursPerDay);
									xmlReadingLog+="    Preferred hour="+this->hoursOfTheDay[cn->p_hours_L[i]]+"\n";
								}
							}

							i++;
						}
					}
					assert(i==cn->p_nPreferredTimeSlots_L);
					return cn;
}

TimeConstraint* Rules::readActivityPreferredStartingTimes(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintActivityPreferredStartingTimes");
					ConstraintActivityPreferredStartingTimes* cn=new ConstraintActivityPreferredStartingTimes();
					cn->nPreferredStartingTimes_L=0;
					int i;
					/*for(i=0; i<MAX_N_CONSTRAINT_ACTIVITY_PREFERRED_STARTING_TIMES; i++){
						cn->days[i] = cn->hours[i] = -1;
					}*/
					i=0;
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight"){
							//cn->weight=elem4.text().toDouble();
							xmlReadingLog+="    Ignoring old tag - weight - making weight percentage=100\n";
							cn->weightPercentage=100;
						}
						else if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Compulsory"){
							if(elem4.text()=="yes"){
								//cn->compulsory=true;
								xmlReadingLog+="    Ignoring old tag - Current constraint is compulsory\n";
								cn->weightPercentage=100;
							}
							else{
								//cn->compulsory=false;
								xmlReadingLog+="    Old tag - current constraint is not compulsory - making weightPercentage=0%\n";
								cn->weightPercentage=0;
							}
						}
						else if(elem4.tagName()=="Activity_Id"){
							cn->activityId=elem4.text().toInt();
							xmlReadingLog+="    Read activity id="+QString::number(cn->activityId)+"\n";
						}
						else if(elem4.tagName()=="Number_of_Preferred_Starting_Times"){
							cn->nPreferredStartingTimes_L=elem4.text().toInt();
							xmlReadingLog+="    Read number of preferred starting times="+QString::number(cn->nPreferredStartingTimes_L)+"\n";
						}
						else if(elem4.tagName()=="Preferred_Starting_Time"){
							xmlReadingLog+="    Read: preferred starting time\n";

							for(QDomNode node5=elem4.firstChild(); !node5.isNull(); node5=node5.nextSibling()){
								QDomElement elem5=node5.toElement();
								if(elem5.isNull()){
									xmlReadingLog+="    Null node here\n";
									continue;
								}
								xmlReadingLog+="    Found "+elem5.tagName()+" tag\n";
								if(elem5.tagName()=="Preferred_Starting_Day"){
									cn->days_L.append(0);
									assert(cn->days_L.count()-1==i);
									for(cn->days_L[i]=0; cn->days_L[i]<this->nDaysPerWeek; cn->days_L[i]++)
										if(this->daysOfTheWeek[cn->days_L[i]]==elem5.text())
											break;

									if(cn->days_L[i]>=this->nDaysPerWeek){
										QMessageBox::information(NULL, tr("FET information"), 
										 tr("Constraint ActivityPreferredStartingTimes day corrupt for activity with id %1, day %2 is inexistent ... ignoring constraint")
										 .arg(cn->activityId)
										 .arg(elem5.text()));
										delete cn;
										cn=NULL;
										//goto corruptConstraintTime;
										return NULL;
									}
						
									assert(cn->days_L[i]<this->nDaysPerWeek);
									xmlReadingLog+="    Preferred starting day="+this->daysOfTheWeek[cn->days_L[i]]+"("+QString::number(i)+")"+"\n";
								}
								else if(elem5.tagName()=="Preferred_Starting_Hour"){
									cn->hours_L.append(0);
									assert(cn->hours_L.count()-1==i);
									for(cn->hours_L[i]=0; cn->hours_L[i] < this->nHoursPerDay; cn->hours_L[i]++)
										if(this->hoursOfTheDay[cn->hours_L[i]]==elem5.text())
											break;
									
									if(cn->hours_L[i]>=this->nHoursPerDay){
										QMessageBox::information(NULL, tr("FET information"), 
										 tr("Constraint ActivityPreferredStartingTimes hour corrupt for activity with id %1, hour %2 is inexistent ... ignoring constraint")
										 .arg(cn->activityId)
										 .arg(elem5.text()));
										delete cn;
										cn=NULL;
										//goto corruptConstraintTime;
										return NULL;
									}
									
									assert(cn->hours_L[i]>=0 && cn->hours_L[i] < this->nHoursPerDay);
									xmlReadingLog+="    Preferred starting hour="+this->hoursOfTheDay[cn->hours_L[i]]+"\n";
								}
							}

							i++;
						}
					}
					assert(i==cn->nPreferredStartingTimes_L);
					return cn;
}

TimeConstraint* Rules::readBreak(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintBreak");

					QList<int> days;
					QList<int> hours;
					double weightPercentage=100;
					int d=-1, h1=-1, h2=-1;
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight_Percentage"){
							weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Read weight percentage="+QString::number(weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Day"){
							for(d=0; d<this->nDaysPerWeek; d++)
								if(this->daysOfTheWeek[d]==elem4.text())
									break;
							if(d>=this->nDaysPerWeek){
								QMessageBox::information(NULL, tr("FET information"), 
								 tr("Constraint Break day corrupt for day %1 is inexistent ... ignoring constraint")
								 .arg(elem4.text()));
								//cn=NULL;
								//goto corruptConstraintTime;
								return NULL;
							}
							assert(d<this->nDaysPerWeek);
							xmlReadingLog+="    Crt. day="+this->daysOfTheWeek[d]+"\n";
						}
						else if(elem4.tagName()=="Start_Hour"){
							for(h1=0; h1 < this->nHoursPerDay; h1++)
								if(this->hoursOfTheDay[h1]==elem4.text())
									break;
							if(h1>=this->nHoursPerDay){
								QMessageBox::information(NULL, tr("FET information"), 
								 tr("Constraint Break start hour corrupt for hour %1 is inexistent ... ignoring constraint")
								 .arg(elem4.text()));
								//cn=NULL;
								//goto corruptConstraintTime;
								return NULL;
							}
							assert(h1>=0 && h1 < this->nHoursPerDay);
							xmlReadingLog+="    Start hour="+this->hoursOfTheDay[h1]+"\n";
						}
						else if(elem4.tagName()=="End_Hour"){
							for(h2=0; h2 < this->nHoursPerDay; h2++)
								if(this->hoursOfTheDay[h2]==elem4.text())
									break;
							if(h2<=0 || h2>this->nHoursPerDay){
								QMessageBox::information(NULL, tr("FET information"), 
								 tr("Constraint Break end hour corrupt for hour %1 is inexistent ... ignoring constraint")
								 .arg(elem4.text()));
								//goto corruptConstraintTime;
								return NULL;
							}
							assert(h2>0 && h2 <= this->nHoursPerDay);
							xmlReadingLog+="    End hour="+this->hoursOfTheDay[h2]+"\n";
						}
					}
					
					assert(weightPercentage>=0);
					assert(d>=0 && h1>=0 && h2>=0);
					
					ConstraintBreakTimes* cn = NULL;
					
					bool found=false;
					foreach(TimeConstraint* c, this->timeConstraintsList)
						if(c->type==CONSTRAINT_BREAK_TIMES){
							ConstraintBreakTimes* tna=(ConstraintBreakTimes*) c;
							if(true){
								found=true;
								
								for(int hh=h1; hh<h2; hh++){
									int k;
									for(k=0; k<tna->days.count(); k++)
										if(tna->days.at(k)==d && tna->hours.at(k)==hh)
											break;
									if(k==tna->days.count()){
										tna->days.append(d);
										tna->hours.append(hh);
									}
								}
								
								assert(tna->days.count()==tna->hours.count());
							}
						}
					if(!found){
						days.clear();
						hours.clear();
						for(int hh=h1; hh<h2; hh++){
							days.append(d);
							hours.append(hh);
						}
					
						cn=new ConstraintBreakTimes(weightPercentage, days, hours);

						return cn;
					}
					else
						return NULL;
}

TimeConstraint* Rules::readBreakTimes(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintBreakTimes");
					ConstraintBreakTimes* cn=new ConstraintBreakTimes();
					int nNotAvailableSlots=-1;
					int i=0;
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Read weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}

						else if(elem4.tagName()=="Number_of_Break_Times"){
							nNotAvailableSlots=elem4.text().toInt();
							xmlReadingLog+="    Read number of break times="+QString::number(nNotAvailableSlots)+"\n";
						}

						else if(elem4.tagName()=="Break_Time"){
							xmlReadingLog+="    Read: not available time\n";
							
							int d=-1;
							int h=-1;

							for(QDomNode node5=elem4.firstChild(); !node5.isNull(); node5=node5.nextSibling()){
								QDomElement elem5=node5.toElement();
								if(elem5.isNull()){
									xmlReadingLog+="    Null node here\n";
									continue;
								}
								xmlReadingLog+="    Found "+elem5.tagName()+" tag\n";
								if(elem5.tagName()=="Day"){
									for(d=0; d<this->nDaysPerWeek; d++)
										if(this->daysOfTheWeek[d]==elem5.text())
											break;

									if(d>=this->nDaysPerWeek){
										QMessageBox::information(NULL, tr("FET information"), 
										 tr("Constraint BreakTimes day corrupt for day %1 is inexistent ... ignoring constraint")
										 .arg(elem5.text()));
										delete cn;
										cn=NULL;
										//goto corruptConstraintTime;
										return NULL;
									}
						
									assert(d<this->nDaysPerWeek);
									xmlReadingLog+="    Day="+this->daysOfTheWeek[d]+"("+QString::number(i)+")"+"\n";
								}
								else if(elem5.tagName()=="Hour"){
									for(h=0; h < this->nHoursPerDay; h++)
										if(this->hoursOfTheDay[h]==elem5.text())
											break;
									
									if(h>=this->nHoursPerDay){
										QMessageBox::information(NULL, tr("FET information"), 
										 tr("Constraint BreakTimes hour corrupt for hour %1 is inexistent ... ignoring constraint")
										 .arg(elem5.text()));
										delete cn;
										cn=NULL;
										//goto corruptConstraintTime;
										return NULL;
									}
									
									assert(h>=0 && h < this->nHoursPerDay);
									xmlReadingLog+="    Hour="+this->hoursOfTheDay[h]+"\n";
								}
							}
							i++;
							
							cn->days.append(d);
							cn->hours.append(h);
						}
					}
					assert(i==cn->days.count() && i==cn->hours.count());
					assert(i==nNotAvailableSlots);
					return cn;
}

TimeConstraint* Rules::readTeachersNoGaps(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintTeachersNoGaps");
					ConstraintTeachersMaxGapsPerWeek* cn=new ConstraintTeachersMaxGapsPerWeek();
					cn->maxGaps=0;
					//ConstraintTeachersNoGaps* cn=new ConstraintTeachersNoGaps();
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight"){
							//cn->weight=elem4.text().toDouble();
							xmlReadingLog+="    Ignoring old tag - weight - making weight percentage=100\n";
							cn->weightPercentage=100;
						}
						else if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Compulsory"){
							if(elem4.text()=="yes"){
								//cn->compulsory=true;
								xmlReadingLog+="    Ignoring old tag - Current constraint is compulsory\n";
								cn->weightPercentage=100;
							}
							else{
								//cn->compulsory=false;
								xmlReadingLog+="    Old tag - current constraint is not compulsory - making weightPercentage=0%\n";
								cn->weightPercentage=0;
							}
						}
					}
					return cn;
}

TimeConstraint* Rules::readTeachersMaxGapsPerWeek(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintTeachersMaxGapsPerWeek");
					ConstraintTeachersMaxGapsPerWeek* cn=new ConstraintTeachersMaxGapsPerWeek();
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight"){
							//cn->weight=elem4.text().toDouble();
							xmlReadingLog+="    Ignoring old tag - weight - making weight percentage=100\n";
							cn->weightPercentage=100;
						}
						else if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Max_Gaps"){
							cn->maxGaps=elem4.text().toInt();
							xmlReadingLog+="    Adding max gaps="+QString::number(cn->maxGaps)+"\n";
						}
						else if(elem4.tagName()=="Compulsory"){
							if(elem4.text()=="yes"){
								//cn->compulsory=true;
								xmlReadingLog+="    Ignoring old tag - Current constraint is compulsory\n";
								cn->weightPercentage=100;
							}
							else{
								//cn->compulsory=false;
								xmlReadingLog+="    Old tag - current constraint is not compulsory - making weightPercentage=0%\n";
								cn->weightPercentage=0;
							}
						}
					}
					return cn;
}

TimeConstraint* Rules::readTeacherMaxGapsPerWeek(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintTeacherMaxGapsPerWeek");
					ConstraintTeacherMaxGapsPerWeek* cn=new ConstraintTeacherMaxGapsPerWeek();
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight"){
							//cn->weight=elem4.text().toDouble();
							xmlReadingLog+="    Ignoring old tag - weight - making weight percentage=100\n";
							cn->weightPercentage=100;
						}
						else if(elem4.tagName()=="Teacher_Name"){
							cn->teacherName=elem4.text();
							xmlReadingLog+="    Read teacher name="+cn->teacherName+"\n";
						}
						else if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Max_Gaps"){
							cn->maxGaps=elem4.text().toInt();
							xmlReadingLog+="    Adding max gaps="+QString::number(cn->maxGaps)+"\n";
						}
						else if(elem4.tagName()=="Compulsory"){
							if(elem4.text()=="yes"){
								//cn->compulsory=true;
								xmlReadingLog+="    Ignoring old tag - Current constraint is compulsory\n";
								cn->weightPercentage=100;
							}
							else{
								//cn->compulsory=false;
								xmlReadingLog+="    Old tag - current constraint is not compulsory - making weightPercentage=0%\n";
								cn->weightPercentage=0;
							}
						}
					}
					return cn;
}

TimeConstraint* Rules::readTeachersMaxGapsPerDay(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintTeachersMaxGapsPerDay");
					ConstraintTeachersMaxGapsPerDay* cn=new ConstraintTeachersMaxGapsPerDay();
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight"){
							//cn->weight=elem4.text().toDouble();
							xmlReadingLog+="    Ignoring old tag - weight - making weight percentage=100\n";
							cn->weightPercentage=100;
						}
						else if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Max_Gaps"){
							cn->maxGaps=elem4.text().toInt();
							xmlReadingLog+="    Adding max gaps="+QString::number(cn->maxGaps)+"\n";
						}
						else if(elem4.tagName()=="Compulsory"){
							if(elem4.text()=="yes"){
								//cn->compulsory=true;
								xmlReadingLog+="    Ignoring old tag - Current constraint is compulsory\n";
								cn->weightPercentage=100;
							}
							else{
								//cn->compulsory=false;
								xmlReadingLog+="    Old tag - current constraint is not compulsory - making weightPercentage=0%\n";
								cn->weightPercentage=0;
							}
						}
					}
					return cn;
}

TimeConstraint* Rules::readTeacherMaxGapsPerDay(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintTeacherMaxGapsPerDay");
					ConstraintTeacherMaxGapsPerDay* cn=new ConstraintTeacherMaxGapsPerDay();
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight"){
							//cn->weight=elem4.text().toDouble();
							xmlReadingLog+="    Ignoring old tag - weight - making weight percentage=100\n";
							cn->weightPercentage=100;
						}
						else if(elem4.tagName()=="Teacher_Name"){
							cn->teacherName=elem4.text();
							xmlReadingLog+="    Read teacher name="+cn->teacherName+"\n";
						}
						else if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Max_Gaps"){
							cn->maxGaps=elem4.text().toInt();
							xmlReadingLog+="    Adding max gaps="+QString::number(cn->maxGaps)+"\n";
						}
						else if(elem4.tagName()=="Compulsory"){
							if(elem4.text()=="yes"){
								//cn->compulsory=true;
								xmlReadingLog+="    Ignoring old tag - Current constraint is compulsory\n";
								cn->weightPercentage=100;
							}
							else{
								//cn->compulsory=false;
								xmlReadingLog+="    Old tag - current constraint is not compulsory - making weightPercentage=0%\n";
								cn->weightPercentage=0;
							}
						}
					}
					return cn;
}

TimeConstraint* Rules::readStudentsNoGaps(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintStudentsNoGaps");

					ConstraintStudentsMaxGapsPerWeek* cn=new ConstraintStudentsMaxGapsPerWeek();
					
					cn->maxGaps=0;
					
					//bool compulsory_read=false;
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight"){
							//cn->weight=elem4.text().toDouble();
							xmlReadingLog+="    Ignoring old tag - weight - making weight percentage=100\n";
							cn->weightPercentage=100;
						}
						else if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Compulsory"){
							if(elem4.text()=="yes"){
								//cn->compulsory=true;
								xmlReadingLog+="    Ignoring old tag - Current constraint is compulsory\n";
								cn->weightPercentage=100;
							}
							else{
								//cn->compulsory=false;
								xmlReadingLog+="    Old tag - current constraint is not compulsory - making weightPercentage=0%\n";
								cn->weightPercentage=0;
							}
							//compulsory_read=true;
						}
					}
					return cn;
}

TimeConstraint* Rules::readStudentsSetNoGaps(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintStudentsSetNoGaps");

					ConstraintStudentsSetMaxGapsPerWeek* cn=new ConstraintStudentsSetMaxGapsPerWeek();
					
					cn->maxGaps=0;
					
					//bool compulsory_read=false;
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight"){
							//cn->weight=elem4.text().toDouble();
							xmlReadingLog+="    Ignoring old tag - weight - making weight percentage=100\n";
							cn->weightPercentage=100;
						}
						else if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Compulsory"){
							if(elem4.text()=="yes"){
								//cn->compulsory=true;
								xmlReadingLog+="    Ignoring old tag - Current constraint is compulsory\n";
								cn->weightPercentage=100;
							}
							else{
								//cn->compulsory=false;
								xmlReadingLog+="    Old tag - current constraint is not compulsory - making weightPercentage=0%\n";
								cn->weightPercentage=0;
							}
							//compulsory_read=true;
						}
						else if(elem4.tagName()=="Students"){
							cn->students=elem4.text();
							xmlReadingLog+="    Read students name="+cn->students+"\n";
						}
					}
					return cn;
}

TimeConstraint* Rules::readStudentsMaxGapsPerWeek(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintStudentsMaxGapsPerWeek");
					ConstraintStudentsMaxGapsPerWeek* cn=new ConstraintStudentsMaxGapsPerWeek();

					//bool compulsory_read=false;
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight"){
							//cn->weight=elem4.text().toDouble();
							xmlReadingLog+="    Ignoring old tag - weight - making weight percentage=100\n";
							cn->weightPercentage=100;
						}
						else if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Max_Gaps"){
							cn->maxGaps=elem4.text().toInt();
							xmlReadingLog+="    Adding max gaps="+QString::number(cn->maxGaps)+"\n";
						}
						else if(elem4.tagName()=="Compulsory"){
							if(elem4.text()=="yes"){
								//cn->compulsory=true;
								xmlReadingLog+="    Ignoring old tag - Current constraint is compulsory\n";
								cn->weightPercentage=100;
							}
							else{
								//cn->compulsory=false;
								xmlReadingLog+="    Old tag - current constraint is not compulsory - making weightPercentage=0%\n";
								cn->weightPercentage=0;
							}
							//compulsory_read=true;
						}
					}
					return cn;
}

TimeConstraint* Rules::readStudentsSetMaxGapsPerWeek(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintStudentsSetMaxGapsPerWeek");
					ConstraintStudentsSetMaxGapsPerWeek* cn=new ConstraintStudentsSetMaxGapsPerWeek();
					
					//bool compulsory_read=false;
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight"){
							//cn->weight=elem4.text().toDouble();
							xmlReadingLog+="    Ignoring old tag - weight - making weight percentage=100\n";
							cn->weightPercentage=100;
						}
						else if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Max_Gaps"){
							cn->maxGaps=elem4.text().toInt();
							xmlReadingLog+="    Adding max gaps="+QString::number(cn->maxGaps)+"\n";
						}
						else if(elem4.tagName()=="Compulsory"){
							if(elem4.text()=="yes"){
								//cn->compulsory=true;
								xmlReadingLog+="    Ignoring old tag - Current constraint is compulsory\n";
								cn->weightPercentage=100;
							}
							else{
								//cn->compulsory=false;
								xmlReadingLog+="    Old tag - current constraint is not compulsory - making weightPercentage=0%\n";
								cn->weightPercentage=0;
							}
							//compulsory_read=true;
						}
						else if(elem4.tagName()=="Students"){
							cn->students=elem4.text();
							xmlReadingLog+="    Read students name="+cn->students+"\n";
						}
					}
					return cn;
}

TimeConstraint* Rules::readStudentsMaxGapsPerDay(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintStudentsMaxGapsPerDay");
					ConstraintStudentsMaxGapsPerDay* cn=new ConstraintStudentsMaxGapsPerDay();

					//bool compulsory_read=false;
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight"){
							//cn->weight=elem4.text().toDouble();
							xmlReadingLog+="    Ignoring old tag - weight - making weight percentage=100\n";
							cn->weightPercentage=100;
						}
						else if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Max_Gaps"){
							cn->maxGaps=elem4.text().toInt();
							xmlReadingLog+="    Adding max gaps="+QString::number(cn->maxGaps)+"\n";
						}
						else if(elem4.tagName()=="Compulsory"){
							if(elem4.text()=="yes"){
								//cn->compulsory=true;
								xmlReadingLog+="    Ignoring old tag - Current constraint is compulsory\n";
								cn->weightPercentage=100;
							}
							else{
								//cn->compulsory=false;
								xmlReadingLog+="    Old tag - current constraint is not compulsory - making weightPercentage=0%\n";
								cn->weightPercentage=0;
							}
							//compulsory_read=true;
						}
					}
					return cn;
}

TimeConstraint* Rules::readStudentsSetMaxGapsPerDay(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintStudentsSetMaxGapsPerDay");
					ConstraintStudentsSetMaxGapsPerDay* cn=new ConstraintStudentsSetMaxGapsPerDay();
					
					//bool compulsory_read=false;
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight"){
							//cn->weight=elem4.text().toDouble();
							xmlReadingLog+="    Ignoring old tag - weight - making weight percentage=100\n";
							cn->weightPercentage=100;
						}
						else if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Max_Gaps"){
							cn->maxGaps=elem4.text().toInt();
							xmlReadingLog+="    Adding max gaps="+QString::number(cn->maxGaps)+"\n";
						}
						else if(elem4.tagName()=="Compulsory"){
							if(elem4.text()=="yes"){
								//cn->compulsory=true;
								xmlReadingLog+="    Ignoring old tag - Current constraint is compulsory\n";
								cn->weightPercentage=100;
							}
							else{
								//cn->compulsory=false;
								xmlReadingLog+="    Old tag - current constraint is not compulsory - making weightPercentage=0%\n";
								cn->weightPercentage=0;
							}
							//compulsory_read=true;
						}
						else if(elem4.tagName()=="Students"){
							cn->students=elem4.text();
							xmlReadingLog+="    Read students name="+cn->students+"\n";
						}
					}
					return cn;
}


TimeConstraint* Rules::readStudentsEarly(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintStudentsEarly");
					ConstraintStudentsEarlyMaxBeginningsAtSecondHour* cn=new ConstraintStudentsEarlyMaxBeginningsAtSecondHour();
					
					cn->maxBeginningsAtSecondHour=0;
					
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight"){
							//cn->weight=elem4.text().toDouble();
							xmlReadingLog+="    Ignoring old tag - weight - making weight percentage=100\n";
							cn->weightPercentage=100;
						}
						else if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Compulsory"){
							if(elem4.text()=="yes"){
								//cn->compulsory=true;
								xmlReadingLog+="    Ignoring old tag - Current constraint is compulsory\n";
								cn->weightPercentage=100;
							}
							else{
								//cn->compulsory=false;
								xmlReadingLog+="    Old tag - current constraint is not compulsory - making weightPercentage=0%\n";
								cn->weightPercentage=0;
							}
						}
					}
					return cn;
}

TimeConstraint* Rules::readStudentsEarlyMaxBeginningsAtSecondHour(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintStudentsEarlyMaxBeginningsAtSecondHour");
					ConstraintStudentsEarlyMaxBeginningsAtSecondHour* cn=new ConstraintStudentsEarlyMaxBeginningsAtSecondHour();
					cn->maxBeginningsAtSecondHour=-1;
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight"){
							//cn->weight=elem4.text().toDouble();
							xmlReadingLog+="    Ignoring old tag - weight - making weight percentage=100\n";
							cn->weightPercentage=100;
						}
						else if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Max_Beginnings_At_Second_Hour"){
							cn->maxBeginningsAtSecondHour=elem4.text().toInt();
							xmlReadingLog+="    Adding max beginnings at second hour="+QString::number(cn->maxBeginningsAtSecondHour)+"\n";
						}
						else if(elem4.tagName()=="Compulsory"){
							if(elem4.text()=="yes"){
								//cn->compulsory=true;
								xmlReadingLog+="    Ignoring old tag - Current constraint is compulsory\n";
								cn->weightPercentage=100;
							}
							else{
								//cn->compulsory=false;
								xmlReadingLog+="    Old tag - current constraint is not compulsory - making weightPercentage=0%\n";
								cn->weightPercentage=0;
							}
						}
					}
					assert(cn->maxBeginningsAtSecondHour>=0);
					return cn;
}

TimeConstraint* Rules::readStudentsSetEarly(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintStudentsSetEarly");
					ConstraintStudentsSetEarlyMaxBeginningsAtSecondHour* cn=new ConstraintStudentsSetEarlyMaxBeginningsAtSecondHour();
					
					cn->maxBeginningsAtSecondHour=0;
					
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight"){
							//cn->weight=elem4.text().toDouble();
							xmlReadingLog+="    Ignoring old tag - weight - making weight percentage=100\n";
							cn->weightPercentage=100;
						}
						else if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Compulsory"){
							if(elem4.text()=="yes"){
								//cn->compulsory=true;
								xmlReadingLog+="    Ignoring old tag - Current constraint is compulsory\n";
								cn->weightPercentage=100;
							}
							else{
								//cn->compulsory=false;
								xmlReadingLog+="    Old tag - current constraint is not compulsory - making weightPercentage=0%\n";
								cn->weightPercentage=0;
							}
						}
						else if(elem4.tagName()=="Students"){
							cn->students=elem4.text();
							xmlReadingLog+="    Read students name="+cn->students+"\n";
						}
					}
					return cn;
}

TimeConstraint* Rules::readStudentsSetEarlyMaxBeginningsAtSecondHour(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintStudentsSetEarlyMaxBeginningsAtSecondHour");
					ConstraintStudentsSetEarlyMaxBeginningsAtSecondHour* cn=new ConstraintStudentsSetEarlyMaxBeginningsAtSecondHour();
					cn->maxBeginningsAtSecondHour=-1;					
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight"){
							//cn->weight=elem4.text().toDouble();
							xmlReadingLog+="    Ignoring old tag - weight - making weight percentage=100\n";
							cn->weightPercentage=100;
						}
						else if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Max_Beginnings_At_Second_Hour"){
							cn->maxBeginningsAtSecondHour=elem4.text().toInt();
							xmlReadingLog+="    Adding max beginnings at second hour="+QString::number(cn->maxBeginningsAtSecondHour)+"\n";
						}
						else if(elem4.tagName()=="Compulsory"){
							if(elem4.text()=="yes"){
								//cn->compulsory=true;
								xmlReadingLog+="    Ignoring old tag - Current constraint is compulsory\n";
								cn->weightPercentage=100;
							}
							else{
								//cn->compulsory=false;
								xmlReadingLog+="    Old tag - current constraint is not compulsory - making weightPercentage=0%\n";
								cn->weightPercentage=0;
							}
						}
						else if(elem4.tagName()=="Students"){
							cn->students=elem4.text();
							xmlReadingLog+="    Read students name="+cn->students+"\n";
						}
					}
					assert(cn->maxBeginningsAtSecondHour>=0);
					return cn;
}

TimeConstraint* Rules::readActivitiesPreferredTimes(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintActivitiesPreferredTimes");
				
					ConstraintActivitiesPreferredStartingTimes* cn=new ConstraintActivitiesPreferredStartingTimes();
					cn->nPreferredStartingTimes_L=0;
					int i;
					/*for(i=0; i<MAX_N_CONSTRAINT_ACTIVITIES_PREFERRED_STARTING_TIMES; i++){
						cn->days[i] = cn->hours[i] = -1;
					}*/
					cn->teacherName="";
					cn->studentsName="";
					cn->subjectName="";
					cn->activityTagName="";
					
					i=0;
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight"){
							//cn->weight=elem4.text().toDouble();
							xmlReadingLog+="    Ignoring old tag - weight - making weight percentage=100\n";
							cn->weightPercentage=100;
						}
						else if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Compulsory"){
							if(elem4.text()=="yes"){
								//cn->compulsory=true;
								xmlReadingLog+="    Ignoring old tag - Current constraint is compulsory\n";
								cn->weightPercentage=100;
							}
							else{
								//cn->compulsory=false;
								xmlReadingLog+="    Old tag - current constraint is not compulsory - making weightPercentage=0%\n";
								cn->weightPercentage=0;
							}
						}
						else if(elem4.tagName()=="Teacher_Name"){
							cn->teacherName=elem4.text();
							xmlReadingLog+="    Read teacher name="+cn->teacherName+"\n";
						}
						else if(elem4.tagName()=="Students_Name"){
							cn->studentsName=elem4.text();
							xmlReadingLog+="    Read students name="+cn->studentsName+"\n";
						}
						else if(elem4.tagName()=="Subject_Name"){
							cn->subjectName=elem4.text();
							xmlReadingLog+="    Read subject name="+cn->subjectName+"\n";
						}
						else if(elem4.tagName()=="Subject_Tag_Name"){
							cn->activityTagName=elem4.text();
							xmlReadingLog+="    Read activity tag name="+cn->activityTagName+"\n";
						}
						else if(elem4.tagName()=="Activity_Tag_Name"){
							cn->activityTagName=elem4.text();
							xmlReadingLog+="    Read activity tag name="+cn->activityTagName+"\n";
						}
						else if(elem4.tagName()=="Number_of_Preferred_Times"){
							cn->nPreferredStartingTimes_L=elem4.text().toInt();
							xmlReadingLog+="    Read number of preferred times="+QString::number(cn->nPreferredStartingTimes_L)+"\n";
						}
						else if(elem4.tagName()=="Preferred_Time"){
							xmlReadingLog+="    Read: preferred time\n";

							for(QDomNode node5=elem4.firstChild(); !node5.isNull(); node5=node5.nextSibling()){
								QDomElement elem5=node5.toElement();
								if(elem5.isNull()){
									xmlReadingLog+="    Null node here\n";
									continue;
								}
								xmlReadingLog+="    Found "+elem5.tagName()+" tag\n";
								if(elem5.tagName()=="Preferred_Day"){
									cn->days_L.append(0);
									assert(cn->days_L.count()-1==i);
									for(cn->days_L[i]=0; cn->days_L[i]<this->nDaysPerWeek; cn->days_L[i]++)
										if(this->daysOfTheWeek[cn->days_L[i]]==elem5.text())
											break;
											
									if(cn->days_L[i]>=this->nDaysPerWeek){
										QMessageBox::information(NULL, tr("FET information"), 
										 tr("Constraint ActivitiesPreferredTimes day corrupt for teacher name=%1, students names=%2, subject name=%3, activity tag name=%4, day %5 is inexistent ... ignoring constraint")
										 .arg(cn->teacherName)
										 .arg(cn->studentsName)
										 .arg(cn->subjectName)
										 .arg(cn->activityTagName)
										 .arg(elem5.text()));
										delete cn;
										cn=NULL;
										//goto corruptConstraintTime;
										return NULL;
									}
											
									assert(cn->days_L[i]<this->nDaysPerWeek);
									xmlReadingLog+="    Preferred day="+this->daysOfTheWeek[cn->days_L[i]]+"("+QString::number(i)+")"+"\n";
								}
								else if(elem5.tagName()=="Preferred_Hour"){
									cn->hours_L.append(0);
									assert(cn->hours_L.count()-1==i);
									for(cn->hours_L[i]=0; cn->hours_L[i] < this->nHoursPerDay; cn->hours_L[i]++)
										if(this->hoursOfTheDay[cn->hours_L[i]]==elem5.text())
											break;
											
									if(cn->hours_L[i]>=this->nHoursPerDay){
										QMessageBox::information(NULL, tr("FET information"), 
										 tr("Constraint ActivitiesPreferredTimes hour corrupt for teacher name=%1, students names=%2, subject name=%3, activity tag name=%4, hour %5 is inexistent ... ignoring constraint")
										 .arg(cn->teacherName)
										 .arg(cn->studentsName)
										 .arg(cn->subjectName)
										 .arg(cn->activityTagName)
										 .arg(elem5.text()));
										delete cn;
										cn=NULL;
										//goto corruptConstraintTime;
										return NULL;
									}
											
									assert(cn->hours_L[i]>=0 && cn->hours_L[i] < this->nHoursPerDay);
									xmlReadingLog+="    Preferred hour="+this->hoursOfTheDay[cn->hours_L[i]]+"\n";
								}
							}

							i++;
						}
					}
					assert(i==cn->nPreferredStartingTimes_L);
					return cn;
}

TimeConstraint* Rules::readActivitiesPreferredTimeSlots(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintActivitiesPreferredTimeSlots");
					ConstraintActivitiesPreferredTimeSlots* cn=new ConstraintActivitiesPreferredTimeSlots();
					cn->p_nPreferredTimeSlots_L=0;
					int i;
					/*for(i=0; i<MAX_N_CONSTRAINT_ACTIVITIES_PREFERRED_TIME_SLOTS; i++){
						cn->p_days[i] = cn->p_hours[i] = -1;
					}*/
					cn->p_teacherName="";
					cn->p_studentsName="";
					cn->p_subjectName="";
					cn->p_activityTagName="";
					
					i=0;
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight"){
							//cn->weight=elem4.text().toDouble();
							xmlReadingLog+="    Ignoring old tag - weight - making weight percentage=100\n";
							cn->weightPercentage=100;
						}
						else if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Compulsory"){
							if(elem4.text()=="yes"){
								//cn->compulsory=true;
								xmlReadingLog+="    Ignoring old tag - Current constraint is compulsory\n";
								cn->weightPercentage=100;
							}
							else{
								//cn->compulsory=false;
								xmlReadingLog+="    Old tag - current constraint is not compulsory - making weightPercentage=0%\n";
								cn->weightPercentage=0;
							}
						}
						else if(elem4.tagName()=="Teacher_Name"){
							cn->p_teacherName=elem4.text();
							xmlReadingLog+="    Read teacher name="+cn->p_teacherName+"\n";
						}
						else if(elem4.tagName()=="Students_Name"){
							cn->p_studentsName=elem4.text();
							xmlReadingLog+="    Read students name="+cn->p_studentsName+"\n";
						}
						else if(elem4.tagName()=="Subject_Name"){
							cn->p_subjectName=elem4.text();
							xmlReadingLog+="    Read subject name="+cn->p_subjectName+"\n";
						}
						else if(elem4.tagName()=="Subject_Tag_Name"){
							cn->p_activityTagName=elem4.text();
							xmlReadingLog+="    Read activity tag name="+cn->p_activityTagName+"\n";
						}
						else if(elem4.tagName()=="Activity_Tag_Name"){
							cn->p_activityTagName=elem4.text();
							xmlReadingLog+="    Read activity tag name="+cn->p_activityTagName+"\n";
						}
						else if(elem4.tagName()=="Number_of_Preferred_Time_Slots"){
							cn->p_nPreferredTimeSlots_L=elem4.text().toInt();
							xmlReadingLog+="    Read number of preferred times="+QString::number(cn->p_nPreferredTimeSlots_L)+"\n";
						}
						else if(elem4.tagName()=="Preferred_Time_Slot"){
							xmlReadingLog+="    Read: preferred time slot\n";

							for(QDomNode node5=elem4.firstChild(); !node5.isNull(); node5=node5.nextSibling()){
								QDomElement elem5=node5.toElement();
								if(elem5.isNull()){
									xmlReadingLog+="    Null node here\n";
									continue;
								}
								xmlReadingLog+="    Found "+elem5.tagName()+" tag\n";
								if(elem5.tagName()=="Preferred_Day"){
									cn->p_days_L.append(0);
									assert(cn->p_days_L.count()-1==i);
									for(cn->p_days_L[i]=0; cn->p_days_L[i]<this->nDaysPerWeek; cn->p_days_L[i]++)
										if(this->daysOfTheWeek[cn->p_days_L[i]]==elem5.text())
											break;
											
									if(cn->p_days_L[i]>=this->nDaysPerWeek){
										QMessageBox::information(NULL, tr("FET information"), 
										 tr("Constraint ActivitiesPreferredTimeSlots day corrupt for teacher name=%1, students names=%2, subject name=%3, activity tag name=%4, day %5 is inexistent ... ignoring constraint")
										 .arg(cn->p_teacherName)
										 .arg(cn->p_studentsName)
										 .arg(cn->p_subjectName)
										 .arg(cn->p_activityTagName)
										 .arg(elem5.text()));
										delete cn;
										cn=NULL;
										//goto corruptConstraintTime;
										return NULL;
									}
											
									assert(cn->p_days_L[i]<this->nDaysPerWeek);
									xmlReadingLog+="    Preferred day="+this->daysOfTheWeek[cn->p_days_L[i]]+"("+QString::number(i)+")"+"\n";
								}
								else if(elem5.tagName()=="Preferred_Hour"){
									cn->p_hours_L.append(0);
									assert(cn->p_hours_L.count()-1==i);
									for(cn->p_hours_L[i]=0; cn->p_hours_L[i] < this->nHoursPerDay; cn->p_hours_L[i]++)
										if(this->hoursOfTheDay[cn->p_hours_L[i]]==elem5.text())
											break;
											
									if(cn->p_hours_L[i]>=this->nHoursPerDay){
										QMessageBox::information(NULL, tr("FET information"), 
										 tr("Constraint ActivitiesPreferredTimeSlots hour corrupt for teacher name=%1, students names=%2, subject name=%3, activity tag name=%4, hour %5 is inexistent ... ignoring constraint")
										 .arg(cn->p_teacherName)
										 .arg(cn->p_studentsName)
										 .arg(cn->p_subjectName)
										 .arg(cn->p_activityTagName)
										 .arg(elem5.text()));
										delete cn;
										cn=NULL;
										//goto corruptConstraintTime;
										return NULL;
									}
											
									assert(cn->p_hours_L[i]>=0 && cn->p_hours_L[i] < this->nHoursPerDay);
									xmlReadingLog+="    Preferred hour="+this->hoursOfTheDay[cn->p_hours_L[i]]+"\n";
								}
							}

							i++;
						}
					}
					assert(i==cn->p_nPreferredTimeSlots_L);
					return cn;
}

TimeConstraint* Rules::readActivitiesPreferredStartingTimes(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintActivitiesPreferredStartingTimes");
					ConstraintActivitiesPreferredStartingTimes* cn=new ConstraintActivitiesPreferredStartingTimes();
					cn->nPreferredStartingTimes_L=0;
					int i;
					/*for(i=0; i<MAX_N_CONSTRAINT_ACTIVITIES_PREFERRED_STARTING_TIMES; i++){
						cn->days[i] = cn->hours[i] = -1;
					}*/
					cn->teacherName="";
					cn->studentsName="";
					cn->subjectName="";
					cn->activityTagName="";
					
					i=0;
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight"){
							//cn->weight=elem4.text().toDouble();
							xmlReadingLog+="    Ignoring old tag - weight - making weight percentage=100\n";
							cn->weightPercentage=100;
						}
						else if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Compulsory"){
							if(elem4.text()=="yes"){
								//cn->compulsory=true;
								xmlReadingLog+="    Ignoring old tag - Current constraint is compulsory\n";
								cn->weightPercentage=100;
							}
							else{
								//cn->compulsory=false;
								xmlReadingLog+="    Old tag - current constraint is not compulsory - making weightPercentage=0%\n";
								cn->weightPercentage=0;
							}
						}
						else if(elem4.tagName()=="Teacher_Name"){
							cn->teacherName=elem4.text();
							xmlReadingLog+="    Read teacher name="+cn->teacherName+"\n";
						}
						else if(elem4.tagName()=="Students_Name"){
							cn->studentsName=elem4.text();
							xmlReadingLog+="    Read students name="+cn->studentsName+"\n";
						}
						else if(elem4.tagName()=="Subject_Name"){
							cn->subjectName=elem4.text();
							xmlReadingLog+="    Read subject name="+cn->subjectName+"\n";
						}
						else if(elem4.tagName()=="Subject_Tag_Name"){
							cn->activityTagName=elem4.text();
							xmlReadingLog+="    Read activity tag name="+cn->activityTagName+"\n";
						}
						else if(elem4.tagName()=="Activity_Tag_Name"){
							cn->activityTagName=elem4.text();
							xmlReadingLog+="    Read activity tag name="+cn->activityTagName+"\n";
						}
						else if(elem4.tagName()=="Number_of_Preferred_Starting_Times"){
							cn->nPreferredStartingTimes_L=elem4.text().toInt();
							xmlReadingLog+="    Read number of preferred starting times="+QString::number(cn->nPreferredStartingTimes_L)+"\n";
						}
						else if(elem4.tagName()=="Preferred_Starting_Time"){
							xmlReadingLog+="    Read: preferred starting time\n";

							for(QDomNode node5=elem4.firstChild(); !node5.isNull(); node5=node5.nextSibling()){
								QDomElement elem5=node5.toElement();
								if(elem5.isNull()){
									xmlReadingLog+="    Null node here\n";
									continue;
								}
								xmlReadingLog+="    Found "+elem5.tagName()+" tag\n";
								if(elem5.tagName()=="Preferred_Starting_Day"){
									cn->days_L.append(0);
									assert(cn->days_L.count()-1==i);
									for(cn->days_L[i]=0; cn->days_L[i]<this->nDaysPerWeek; cn->days_L[i]++)
										if(this->daysOfTheWeek[cn->days_L[i]]==elem5.text())
											break;
											
									if(cn->days_L[i]>=this->nDaysPerWeek){
										QMessageBox::information(NULL, tr("FET information"), 
										 tr("Constraint ActivitiesPreferredStartingTimes day corrupt for teacher name=%1, students names=%2, subject name=%3, activity tag name=%4, day %5 is inexistent ... ignoring constraint")
										 .arg(cn->teacherName)
										 .arg(cn->studentsName)
										 .arg(cn->subjectName)
										 .arg(cn->activityTagName)
										 .arg(elem5.text()));
										delete cn;
										cn=NULL;
										//goto corruptConstraintTime;
										return NULL;
									}
											
									assert(cn->days_L[i]<this->nDaysPerWeek);
									xmlReadingLog+="    Preferred starting day="+this->daysOfTheWeek[cn->days_L[i]]+"("+QString::number(i)+")"+"\n";
								}
								else if(elem5.tagName()=="Preferred_Starting_Hour"){
									cn->hours_L.append(0);
									assert(cn->hours_L.count()-1==i);
									for(cn->hours_L[i]=0; cn->hours_L[i] < this->nHoursPerDay; cn->hours_L[i]++)
										if(this->hoursOfTheDay[cn->hours_L[i]]==elem5.text())
											break;
											
									if(cn->hours_L[i]>=this->nHoursPerDay){
										QMessageBox::information(NULL, tr("FET information"), 
										 tr("Constraint ActivitiesPreferredStartingTimes hour corrupt for teacher name=%1, students names=%2, subject name=%3, activity tag name=%4, hour %5 is inexistent ... ignoring constraint")
										 .arg(cn->teacherName)
										 .arg(cn->studentsName)
										 .arg(cn->subjectName)
										 .arg(cn->activityTagName)
										 .arg(elem5.text()));
										delete cn;
										cn=NULL;
										//goto corruptConstraintTime;
										return NULL;
									}
											
									assert(cn->hours_L[i]>=0 && cn->hours_L[i] < this->nHoursPerDay);
									xmlReadingLog+="    Preferred starting hour="+this->hoursOfTheDay[cn->hours_L[i]]+"\n";
								}
							}

							i++;
						}
					}
					assert(i==cn->nPreferredStartingTimes_L);
					return cn;
}

////////////////
TimeConstraint* Rules::readSubactivitiesPreferredTimeSlots(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintSubactivitiesPreferredTimeSlots");
					ConstraintSubactivitiesPreferredTimeSlots* cn=new ConstraintSubactivitiesPreferredTimeSlots();
					cn->p_nPreferredTimeSlots_L=0;
					cn->componentNumber=0;
					int i;
					/*for(i=0; i<MAX_N_CONSTRAINT_SUBACTIVITIES_PREFERRED_TIME_SLOTS; i++){
						cn->p_days[i] = cn->p_hours[i] = -1;
					}*/
					cn->p_teacherName="";
					cn->p_studentsName="";
					cn->p_subjectName="";
					cn->p_activityTagName="";
					
					i=0;
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight"){
							//cn->weight=elem4.text().toDouble();
							xmlReadingLog+="    Ignoring old tag - weight - making weight percentage=100\n";
							cn->weightPercentage=100;
						}
						else if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Component_Number"){
							cn->componentNumber=elem4.text().toInt();
							xmlReadingLog+="    Adding component number="+QString::number(cn->componentNumber)+"\n";
						}
						else if(elem4.tagName()=="Compulsory"){
							if(elem4.text()=="yes"){
								//cn->compulsory=true;
								xmlReadingLog+="    Ignoring old tag - Current constraint is compulsory\n";
								cn->weightPercentage=100;
							}
							else{
								//cn->compulsory=false;
								xmlReadingLog+="    Old tag - current constraint is not compulsory - making weightPercentage=0%\n";
								cn->weightPercentage=0;
							}
						}
						else if(elem4.tagName()=="Teacher_Name"){
							cn->p_teacherName=elem4.text();
							xmlReadingLog+="    Read teacher name="+cn->p_teacherName+"\n";
						}
						else if(elem4.tagName()=="Students_Name"){
							cn->p_studentsName=elem4.text();
							xmlReadingLog+="    Read students name="+cn->p_studentsName+"\n";
						}
						else if(elem4.tagName()=="Subject_Name"){
							cn->p_subjectName=elem4.text();
							xmlReadingLog+="    Read subject name="+cn->p_subjectName+"\n";
						}
						else if(elem4.tagName()=="Subject_Tag_Name"){
							cn->p_activityTagName=elem4.text();
							xmlReadingLog+="    Read activity tag name="+cn->p_activityTagName+"\n";
						}
						else if(elem4.tagName()=="Activity_Tag_Name"){
							cn->p_activityTagName=elem4.text();
							xmlReadingLog+="    Read activity tag name="+cn->p_activityTagName+"\n";
						}
						else if(elem4.tagName()=="Number_of_Preferred_Time_Slots"){
							cn->p_nPreferredTimeSlots_L=elem4.text().toInt();
							xmlReadingLog+="    Read number of preferred times="+QString::number(cn->p_nPreferredTimeSlots_L)+"\n";
						}
						else if(elem4.tagName()=="Preferred_Time_Slot"){
							xmlReadingLog+="    Read: preferred time slot\n";

							for(QDomNode node5=elem4.firstChild(); !node5.isNull(); node5=node5.nextSibling()){
								QDomElement elem5=node5.toElement();
								if(elem5.isNull()){
									xmlReadingLog+="    Null node here\n";
									continue;
								}
								xmlReadingLog+="    Found "+elem5.tagName()+" tag\n";
								if(elem5.tagName()=="Preferred_Day"){
									cn->p_days_L.append(0);
									assert(cn->p_days_L.count()-1==i);
									for(cn->p_days_L[i]=0; cn->p_days_L[i]<this->nDaysPerWeek; cn->p_days_L[i]++)
										if(this->daysOfTheWeek[cn->p_days_L[i]]==elem5.text())
											break;
											
									if(cn->p_days_L[i]>=this->nDaysPerWeek){
										QMessageBox::information(NULL, tr("FET information"), 
										 tr("Constraint ActivitiesPreferredTimeSlots day corrupt for teacher name=%1, students names=%2, subject name=%3, activity tag name=%4, day %5 is inexistent ... ignoring constraint")
										 .arg(cn->p_teacherName)
										 .arg(cn->p_studentsName)
										 .arg(cn->p_subjectName)
										 .arg(cn->p_activityTagName)
										 .arg(elem5.text()));
										delete cn;
										cn=NULL;
										//goto corruptConstraintTime;
										return NULL;
									}
											
									assert(cn->p_days_L[i]<this->nDaysPerWeek);
									xmlReadingLog+="    Preferred day="+this->daysOfTheWeek[cn->p_days_L[i]]+"("+QString::number(i)+")"+"\n";
								}
								else if(elem5.tagName()=="Preferred_Hour"){
									cn->p_hours_L.append(0);
									assert(cn->p_hours_L.count()-1==i);
									for(cn->p_hours_L[i]=0; cn->p_hours_L[i] < this->nHoursPerDay; cn->p_hours_L[i]++)
										if(this->hoursOfTheDay[cn->p_hours_L[i]]==elem5.text())
											break;
											
									if(cn->p_hours_L[i]>=this->nHoursPerDay){
										QMessageBox::information(NULL, tr("FET information"), 
										 tr("Constraint ActivitiesPreferredTimeSlots hour corrupt for teacher name=%1, students names=%2, subject name=%3, activity tag name=%4, hour %5 is inexistent ... ignoring constraint")
										 .arg(cn->p_teacherName)
										 .arg(cn->p_studentsName)
										 .arg(cn->p_subjectName)
										 .arg(cn->p_activityTagName)
										 .arg(elem5.text()));
										delete cn;
										cn=NULL;
										//goto corruptConstraintTime;
										return NULL;
									}
											
									assert(cn->p_hours_L[i]>=0 && cn->p_hours_L[i] < this->nHoursPerDay);
									xmlReadingLog+="    Preferred hour="+this->hoursOfTheDay[cn->p_hours_L[i]]+"\n";
								}
							}

							i++;
						}
					}
					assert(i==cn->p_nPreferredTimeSlots_L);
					return cn;
}

TimeConstraint* Rules::readSubactivitiesPreferredStartingTimes(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintSubactivitiesPreferredStartingTimes");
					ConstraintSubactivitiesPreferredStartingTimes* cn=new ConstraintSubactivitiesPreferredStartingTimes();
					cn->nPreferredStartingTimes_L=0;
					cn->componentNumber=0;
					int i;
					/*for(i=0; i<MAX_N_CONSTRAINT_SUBACTIVITIES_PREFERRED_STARTING_TIMES; i++){
						cn->days[i] = cn->hours[i] = -1;
					}*/
					cn->teacherName="";
					cn->studentsName="";
					cn->subjectName="";
					cn->activityTagName="";
					
					i=0;
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight"){
							//cn->weight=elem4.text().toDouble();
							xmlReadingLog+="    Ignoring old tag - weight - making weight percentage=100\n";
							cn->weightPercentage=100;
						}
						else if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Component_Number"){
							cn->componentNumber=elem4.text().toInt();
							xmlReadingLog+="    Adding component number="+QString::number(cn->componentNumber)+"\n";
						}
						else if(elem4.tagName()=="Compulsory"){
							if(elem4.text()=="yes"){
								//cn->compulsory=true;
								xmlReadingLog+="    Ignoring old tag - Current constraint is compulsory\n";
								cn->weightPercentage=100;
							}
							else{
								//cn->compulsory=false;
								xmlReadingLog+="    Old tag - current constraint is not compulsory - making weightPercentage=0%\n";
								cn->weightPercentage=0;
							}
						}
						else if(elem4.tagName()=="Teacher_Name"){
							cn->teacherName=elem4.text();
							xmlReadingLog+="    Read teacher name="+cn->teacherName+"\n";
						}
						else if(elem4.tagName()=="Students_Name"){
							cn->studentsName=elem4.text();
							xmlReadingLog+="    Read students name="+cn->studentsName+"\n";
						}
						else if(elem4.tagName()=="Subject_Name"){
							cn->subjectName=elem4.text();
							xmlReadingLog+="    Read subject name="+cn->subjectName+"\n";
						}
						else if(elem4.tagName()=="Subject_Tag_Name"){
							cn->activityTagName=elem4.text();
							xmlReadingLog+="    Read activity tag name="+cn->activityTagName+"\n";
						}
						else if(elem4.tagName()=="Activity_Tag_Name"){
							cn->activityTagName=elem4.text();
							xmlReadingLog+="    Read activity tag name="+cn->activityTagName+"\n";
						}
						else if(elem4.tagName()=="Number_of_Preferred_Starting_Times"){
							cn->nPreferredStartingTimes_L=elem4.text().toInt();
							xmlReadingLog+="    Read number of preferred starting times="+QString::number(cn->nPreferredStartingTimes_L)+"\n";
						}
						else if(elem4.tagName()=="Preferred_Starting_Time"){
							xmlReadingLog+="    Read: preferred starting time\n";

							for(QDomNode node5=elem4.firstChild(); !node5.isNull(); node5=node5.nextSibling()){
								QDomElement elem5=node5.toElement();
								if(elem5.isNull()){
									xmlReadingLog+="    Null node here\n";
									continue;
								}
								xmlReadingLog+="    Found "+elem5.tagName()+" tag\n";
								if(elem5.tagName()=="Preferred_Starting_Day"){
									cn->days_L.append(0);
									assert(cn->days_L.count()-1==i);
									for(cn->days_L[i]=0; cn->days_L[i]<this->nDaysPerWeek; cn->days_L[i]++)
										if(this->daysOfTheWeek[cn->days_L[i]]==elem5.text())
											break;
											
									if(cn->days_L[i]>=this->nDaysPerWeek){
										QMessageBox::information(NULL, tr("FET information"), 
										 tr("Constraint ActivitiesPreferredStartingTimes day corrupt for teacher name=%1, students names=%2, subject name=%3, activity tag name=%4, day %5 is inexistent ... ignoring constraint")
										 .arg(cn->teacherName)
										 .arg(cn->studentsName)
										 .arg(cn->subjectName)
										 .arg(cn->activityTagName)
										 .arg(elem5.text()));
										delete cn;
										cn=NULL;
										//goto corruptConstraintTime;
										return NULL;
									}
											
									assert(cn->days_L[i]<this->nDaysPerWeek);
									xmlReadingLog+="    Preferred starting day="+this->daysOfTheWeek[cn->days_L[i]]+"("+QString::number(i)+")"+"\n";
								}
								else if(elem5.tagName()=="Preferred_Starting_Hour"){
									cn->hours_L.append(0);
									assert(cn->hours_L.count()-1==i);
									for(cn->hours_L[i]=0; cn->hours_L[i] < this->nHoursPerDay; cn->hours_L[i]++)
										if(this->hoursOfTheDay[cn->hours_L[i]]==elem5.text())
											break;
											
									if(cn->hours_L[i]>=this->nHoursPerDay){
										QMessageBox::information(NULL, tr("FET information"), 
										 tr("Constraint ActivitiesPreferredStartingTimes hour corrupt for teacher name=%1, students names=%2, subject name=%3, activity tag name=%4, hour %5 is inexistent ... ignoring constraint")
										 .arg(cn->teacherName)
										 .arg(cn->studentsName)
										 .arg(cn->subjectName)
										 .arg(cn->activityTagName)
										 .arg(elem5.text()));
										delete cn;
										cn=NULL;
										//goto corruptConstraintTime;
										return NULL;
									}
											
									assert(cn->hours_L[i]>=0 && cn->hours_L[i] < this->nHoursPerDay);
									xmlReadingLog+="    Preferred starting hour="+this->hoursOfTheDay[cn->hours_L[i]]+"\n";
								}
							}

							i++;
						}
					}
					assert(i==cn->nPreferredStartingTimes_L);
					return cn;
}
////////////////


///space constraints reading routines
SpaceConstraint* Rules::readBasicCompulsorySpace(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintBasicCompulsorySpace");
					ConstraintBasicCompulsorySpace* cn=new ConstraintBasicCompulsorySpace();
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}

						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";

						if(elem4.tagName()=="Weight"){
							//cn->weight=elem4.text().toDouble();
							xmlReadingLog+="    Ignoring old tag - weight - making weight percentage=100\n";
							cn->weightPercentage=100;
						}
						else if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Compulsory"){
							if(elem4.text()=="yes"){
								//cn->compulsory=true;
								xmlReadingLog+="    Ignoring old tag - Current constraint is compulsory\n";
								cn->weightPercentage=100;
							}
							else{
								//cn->compulsory=false;
								xmlReadingLog+="    Old tag - current constraint is not compulsory - making weightPercentage=0%\n";
								cn->weightPercentage=0;
							}
						}
						/*if(elem4.tagName()=="Weight"){
							cn->weight=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight="+QString::number(cn->weight)+"\n";
						}
						else if(elem4.tagName()=="Compulsory"){
							if(elem4.text()=="yes"){
								cn->compulsory=true;
								xmlReadingLog+="    Current constraint is compulsory\n";
							}
							else{
								cn->compulsory=false;
								xmlReadingLog+="    Current constraint is not compulsory\n";
							}
						}*/
					}
					return cn;
}

SpaceConstraint* Rules::readRoomNotAvailable(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintRoomNotAvailable");

					QList<int> days;
					QList<int> hours;
					QString room;
					double weightPercentage=100;
					int d=-1, h1=-1, h2=-1;
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight_Percentage"){
							weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Read weight percentage="+QString::number(weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Day"){
							for(d=0; d<this->nDaysPerWeek; d++)
								if(this->daysOfTheWeek[d]==elem4.text())
									break;
							if(d>=this->nDaysPerWeek){
								QMessageBox::information(NULL, tr("FET information"), 
								 tr("Constraint RoomNotAvailable day corrupt for room %1, day %2 is inexistent ... ignoring constraint")
								 .arg(room)
								 .arg(elem4.text()));
								//cn=NULL;
								//goto corruptConstraintSpace;
								return NULL;
							}
							assert(d<this->nDaysPerWeek);
							xmlReadingLog+="    Crt. day="+this->daysOfTheWeek[d]+"\n";
						}
						else if(elem4.tagName()=="Start_Hour"){
							for(h1=0; h1 < this->nHoursPerDay; h1++)
								if(this->hoursOfTheDay[h1]==elem4.text())
									break;
							if(h1>=this->nHoursPerDay){
								QMessageBox::information(NULL, tr("FET information"), 
								 tr("Constraint RoomNotAvailable start hour corrupt for room %1, hour %2 is inexistent ... ignoring constraint")
								 .arg(room)
								 .arg(elem4.text()));
								//cn=NULL;
								//goto corruptConstraintSpace;
								return NULL;
							}
							assert(h1>=0 && h1 < this->nHoursPerDay);
							xmlReadingLog+="    Start hour="+this->hoursOfTheDay[h1]+"\n";
						}
						else if(elem4.tagName()=="End_Hour"){
							for(h2=0; h2 < this->nHoursPerDay; h2++)
								if(this->hoursOfTheDay[h2]==elem4.text())
									break;
							if(h2<=0 || h2>this->nHoursPerDay){
								QMessageBox::information(NULL, tr("FET information"), 
								 tr("Constraint RoomNotAvailable end hour corrupt for room %1, hour %2 is inexistent ... ignoring constraint")
								 .arg(room)
								 .arg(elem4.text()));
								//goto corruptConstraintSpace;
								return NULL;
							}
							assert(h2>0 && h2 <= this->nHoursPerDay);
							xmlReadingLog+="    End hour="+this->hoursOfTheDay[h2]+"\n";
						}
						else if(elem4.tagName()=="Room_Name"){
							room=elem4.text();
							xmlReadingLog+="    Read room name="+room+"\n";
						}
					}
					
					assert(weightPercentage>=0);
					assert(d>=0 && h1>=0 && h2>=0);
					
					ConstraintRoomNotAvailableTimes* cn = NULL;
					
					bool found=false;
					foreach(SpaceConstraint* c, this->spaceConstraintsList)
						if(c->type==CONSTRAINT_ROOM_NOT_AVAILABLE_TIMES){
							ConstraintRoomNotAvailableTimes* tna=(ConstraintRoomNotAvailableTimes*) c;
							if(tna->room==room){
								found=true;
								
								for(int hh=h1; hh<h2; hh++){
									int k;
									for(k=0; k<tna->days.count(); k++)
										if(tna->days.at(k)==d && tna->hours.at(k)==hh)
											break;
									if(k==tna->days.count()){
										tna->days.append(d);
										tna->hours.append(hh);
									}
								}
								
								assert(tna->days.count()==tna->hours.count());
							}
						}
					if(!found){
						days.clear();
						hours.clear();
						for(int hh=h1; hh<h2; hh++){
							days.append(d);
							hours.append(hh);
						}
					
						cn=new ConstraintRoomNotAvailableTimes(weightPercentage, room, days, hours);

						return cn;
					}
					else
						return NULL;
}

SpaceConstraint* Rules::readRoomNotAvailableTimes(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintRoomNotAvailableTimes");
					ConstraintRoomNotAvailableTimes* cn=new ConstraintRoomNotAvailableTimes();
					int nNotAvailableSlots=-1;
					int i=0;
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Read weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}

						else if(elem4.tagName()=="Number_of_Not_Available_Times"){
							nNotAvailableSlots=elem4.text().toInt();
							xmlReadingLog+="    Read number of not available times="+QString::number(nNotAvailableSlots)+"\n";
						}

						else if(elem4.tagName()=="Not_Available_Time"){
							xmlReadingLog+="    Read: not available time\n";
							
							int d=-1;
							int h=-1;

							for(QDomNode node5=elem4.firstChild(); !node5.isNull(); node5=node5.nextSibling()){
								QDomElement elem5=node5.toElement();
								if(elem5.isNull()){
									xmlReadingLog+="    Null node here\n";
									continue;
								}
								xmlReadingLog+="    Found "+elem5.tagName()+" tag\n";
								if(elem5.tagName()=="Day"){
									for(d=0; d<this->nDaysPerWeek; d++)
										if(this->daysOfTheWeek[d]==elem5.text())
											break;

									if(d>=this->nDaysPerWeek){
										QMessageBox::information(NULL, tr("FET information"), 
										 tr("Constraint RoomNotAvailableTimes day corrupt for room %1, day %2 is inexistent ... ignoring constraint")
										 .arg(cn->room)
										 .arg(elem5.text()));
										delete cn;
										cn=NULL;
										return NULL;
										//goto corruptConstraintSpace;
									}
						
									assert(d<this->nDaysPerWeek);
									xmlReadingLog+="    Day="+this->daysOfTheWeek[d]+"("+QString::number(i)+")"+"\n";
								}
								else if(elem5.tagName()=="Hour"){
									for(h=0; h < this->nHoursPerDay; h++)
										if(this->hoursOfTheDay[h]==elem5.text())
											break;
									
									if(h>=this->nHoursPerDay){
										QMessageBox::information(NULL, tr("FET information"), 
										 tr("Constraint RoomNotAvailableTimes hour corrupt for room %1, hour %2 is inexistent ... ignoring constraint")
										 .arg(cn->room)
										 .arg(elem5.text()));
										delete cn;
										cn=NULL;
										//goto corruptConstraintSpace;
										return NULL;
									}
									
									assert(h>=0 && h < this->nHoursPerDay);
									xmlReadingLog+="    Hour="+this->hoursOfTheDay[h]+"\n";
								}
							}
							i++;
							
							cn->days.append(d);
							cn->hours.append(h);
						}
						else if(elem4.tagName()=="Room"){
							cn->room=elem4.text();
							xmlReadingLog+="    Read room name="+cn->room+"\n";
						}
					}
					assert(i==cn->days.count() && i==cn->hours.count());
					assert(i==nNotAvailableSlots);
					return cn;
}

SpaceConstraint* Rules::readActivityPreferredRoom(const QDomElement& elem3, FakeString& xmlReadingLog,
	bool& reportUnspecifiedPermanentlyLockedSpace){
					assert(elem3.tagName()=="ConstraintActivityPreferredRoom");
					ConstraintActivityPreferredRoom* cn=new ConstraintActivityPreferredRoom();
					cn->permanentlyLocked=false; //default
					bool foundLocked=false;
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight"){
							//cn->weight=elem4.text().toDouble();
							xmlReadingLog+="    Ignoring old tag - weight - making weight percentage=100\n";
							cn->weightPercentage=100;
						}
						else if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Compulsory"){
							if(elem4.text()=="yes"){
								//cn->compulsory=true;
								xmlReadingLog+="    Ignoring old tag - Current constraint is compulsory\n";
								cn->weightPercentage=100;
							}
							else{
								//cn->compulsory=false;
								xmlReadingLog+="    Old tag - current constraint is not compulsory - making weightPercentage=0%\n";
								cn->weightPercentage=0;
							}
						}
						else if(elem4.tagName()=="Permanently_Locked"){
							if(elem4.text()=="true" || elem4.text()=="1" || elem4.text()=="yes"){
								xmlReadingLog+="    Permanently locked\n";
								cn->permanentlyLocked=true;
							}
							else{
								if(!(elem4.text()=="no" || elem4.text()=="false" || elem4.text()=="0")){
									QMessageBox::warning(NULL, tr("FET warning"),
									 tr("Found constraint activity preferred room with tag permanently locked"
									 " which is not 'true', 'false', 'yes', 'no', '1' or '0'."
									 " The tag will be considered false",
									 "Instructions for translators: please leave the 'true', 'false', 'yes' and 'no' fields untranslated, as they are in English"));
								}
								//assert(elem4.text()=="false" || elem4.text()=="0" || elem4.text()=="no");
								xmlReadingLog+="    Not permanently locked\n";
								cn->permanentlyLocked=false;
							}
							foundLocked=true;
						}

						/*if(elem4.tagName()=="Weight"){
							cn->weight=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight="+QString::number(cn->weight)+"\n";
						}
						else if(elem4.tagName()=="Compulsory"){
							if(elem4.text()=="yes"){
								cn->compulsory=true;
								xmlReadingLog+="    Current constraint is compulsory\n";
							}
							else{
								cn->compulsory=false;
								xmlReadingLog+="    Current constraint is not compulsory\n";
							}
						}*/
						else if(elem4.tagName()=="Activity_Id"){
							cn->activityId=elem4.text().toInt();
							xmlReadingLog+="    Read activity id="+QString::number(cn->activityId)+"\n";
						}
						else if(elem4.tagName()=="Room"){
							cn->roomName=elem4.text();
							xmlReadingLog+="    Read room="+elem4.text()+"\n";
						}
					}
					if(!foundLocked && reportUnspecifiedPermanentlyLockedSpace){
						int t=QMessageBox::information(NULL, tr("FET information"),
						 tr("Found constraint activity preferred room, with unspecified tag"
						 " 'permanently locked' - this tag will be set to 'false' by default. You can always modify it"
						 " by editing the constraint in the 'Data' menu")+"\n\n"
						 +tr("Explanation: starting with version 5.8.0 (January 2009), the constraint"
						 " activity preferred room has"
						 " a new tag, 'permanently locked' (true or false)."
						 " It is recommended to make the tag 'permanently locked' true for the constraints you"
						 " need to be not modifiable from the 'Timetable' menu"
						 " and leave this tag false for the constraints you need to be modifiable from the 'Timetable' menu"
						 " (the 'permanently locked' tag can be modified by editing the constraint from the 'Data' menu)."
						 " This way, when viewing the timetable"
						 " and locking/unlocking some activities, you will not unlock the constraints which"
						 " need to be locked all the time."
						 ),
						  tr("Skip rest"), tr("See next"), QString(), 1, 0 );
						if(t==0)
							reportUnspecifiedPermanentlyLockedSpace=false;
					}

					return cn;
}

SpaceConstraint* Rules::readActivityPreferredRooms(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintActivityPreferredRooms");
					int _n_preferred_rooms=0;
					ConstraintActivityPreferredRooms* cn=new ConstraintActivityPreferredRooms();
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight"){
							//cn->weight=elem4.text().toDouble();
							xmlReadingLog+="    Ignoring old tag - weight - making weight percentage=100\n";
							cn->weightPercentage=100;
						}
						else if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Compulsory"){
							if(elem4.text()=="yes"){
								//cn->compulsory=true;
								xmlReadingLog+="    Ignoring old tag - Current constraint is compulsory\n";
								cn->weightPercentage=100;
							}
							else{
								//cn->compulsory=false;
								xmlReadingLog+="    Old tag - current constraint is not compulsory - making weightPercentage=0%\n";
								cn->weightPercentage=0;
							}
						}
						/*if(elem4.tagName()=="Weight"){
							cn->weight=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight="+QString::number(cn->weight)+"\n";
						}
						else if(elem4.tagName()=="Compulsory"){
							if(elem4.text()=="yes"){
								cn->compulsory=true;
								xmlReadingLog+="    Current constraint is compulsory\n";
							}
							else{
								cn->compulsory=false;
								xmlReadingLog+="    Current constraint is not compulsory\n";
							}
						}*/
						else if(elem4.tagName()=="Activity_Id"){
							cn->activityId=elem4.text().toInt();
							xmlReadingLog+="    Read activity id="+QString::number(cn->activityId)+"\n";
						}
						else if(elem4.tagName()=="Number_of_Preferred_Rooms"){
							_n_preferred_rooms=elem4.text().toInt();
							xmlReadingLog+="    Read number of preferred rooms: "+QString::number(_n_preferred_rooms)+"\n";
							assert(_n_preferred_rooms>=2);
						}
						else if(elem4.tagName()=="Preferred_Room"){
							cn->roomsNames.append(elem4.text());
							xmlReadingLog+="    Read room="+elem4.text()+"\n";
						}
					}
					assert(_n_preferred_rooms==cn->roomsNames.count());
					return cn;
}

SpaceConstraint* Rules::readSubjectPreferredRoom(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintSubjectPreferredRoom");
					ConstraintSubjectPreferredRoom* cn=new ConstraintSubjectPreferredRoom();
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight"){
							//cn->weight=elem4.text().toDouble();
							xmlReadingLog+="    Ignoring old tag - weight - making weight percentage=100\n";
							cn->weightPercentage=100;
						}
						else if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Compulsory"){
							if(elem4.text()=="yes"){
								//cn->compulsory=true;
								xmlReadingLog+="    Ignoring old tag - Current constraint is compulsory\n";
								cn->weightPercentage=100;
							}
							else{
								//cn->compulsory=false;
								xmlReadingLog+="    Old tag - current constraint is not compulsory - making weightPercentage=0%\n";
								cn->weightPercentage=0;
							}
						}
						else if(elem4.tagName()=="Subject"){
							cn->subjectName=elem4.text();
							xmlReadingLog+="    Read subject="+cn->subjectName+"\n";
						}
						else if(elem4.tagName()=="Room"){
							cn->roomName=elem4.text();
							xmlReadingLog+="    Read room="+cn->roomName+"\n";
						}
					}
					return cn;
}

SpaceConstraint* Rules::readSubjectPreferredRooms(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintSubjectPreferredRooms");
					int _n_preferred_rooms=0;
					ConstraintSubjectPreferredRooms* cn=new ConstraintSubjectPreferredRooms();
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight"){
							//cn->weight=elem4.text().toDouble();
							xmlReadingLog+="    Ignoring old tag - weight - making weight percentage=100\n";
							cn->weightPercentage=100;
						}
						else if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Compulsory"){
							if(elem4.text()=="yes"){
								//cn->compulsory=true;
								xmlReadingLog+="    Ignoring old tag - Current constraint is compulsory\n";
								cn->weightPercentage=100;
							}
							else{
								//cn->compulsory=false;
								xmlReadingLog+="    Old tag - current constraint is not compulsory - making weightPercentage=0%\n";
								cn->weightPercentage=0;
							}
						}
						else if(elem4.tagName()=="Subject"){
							cn->subjectName=elem4.text();
							xmlReadingLog+="    Read subject="+cn->subjectName+"\n";
						}
						else if(elem4.tagName()=="Number_of_Preferred_Rooms"){
							_n_preferred_rooms=elem4.text().toInt();
							xmlReadingLog+="    Read number of preferred rooms: "+QString::number(_n_preferred_rooms)+"\n";
							assert(_n_preferred_rooms>=2);
						}
						else if(elem4.tagName()=="Preferred_Room"){
							cn->roomsNames.append(elem4.text());
							xmlReadingLog+="    Read room="+elem4.text()+"\n";
						}
					}
					assert(_n_preferred_rooms==cn->roomsNames.count());
					return cn;
}

SpaceConstraint* Rules::readSubjectSubjectTagPreferredRoom(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintSubjectSubjectTagPreferredRoom");
					ConstraintSubjectActivityTagPreferredRoom* cn=new ConstraintSubjectActivityTagPreferredRoom();
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight"){
							//cn->weight=elem4.text().toDouble();
							xmlReadingLog+="    Ignoring old tag - weight - making weight percentage=100\n";
							cn->weightPercentage=100;
						}
						else if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Compulsory"){
							if(elem4.text()=="yes"){
								//cn->compulsory=true;
								xmlReadingLog+="    Ignoring old tag - Current constraint is compulsory\n";
								cn->weightPercentage=100;
							}
							else{
								//cn->compulsory=false;
								xmlReadingLog+="    Old tag - current constraint is not compulsory - making weightPercentage=0%\n";
								cn->weightPercentage=0;
							}
						}
						else if(elem4.tagName()=="Subject"){
							cn->subjectName=elem4.text();
							xmlReadingLog+="    Read subject="+cn->subjectName+"\n";
						}
						else if(elem4.tagName()=="Subject_Tag"){
							cn->activityTagName=elem4.text();
							xmlReadingLog+="    Read activity tag="+cn->activityTagName+"\n";
						}
						else if(elem4.tagName()=="Room"){
							cn->roomName=elem4.text();
							xmlReadingLog+="    Read room="+cn->roomName+"\n";
						}
					}
					return cn;
}

SpaceConstraint* Rules::readSubjectSubjectTagPreferredRooms(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintSubjectSubjectTagPreferredRooms");
					int _n_preferred_rooms=0;
					ConstraintSubjectActivityTagPreferredRooms* cn=new ConstraintSubjectActivityTagPreferredRooms();
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight"){
							//cn->weight=elem4.text().toDouble();
							xmlReadingLog+="    Ignoring old tag - weight - making weight percentage=100\n";
							cn->weightPercentage=100;
						}
						else if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Compulsory"){
							if(elem4.text()=="yes"){
								//cn->compulsory=true;
								xmlReadingLog+="    Ignoring old tag - Current constraint is compulsory\n";
								cn->weightPercentage=100;
							}
							else{
								//cn->compulsory=false;
								xmlReadingLog+="    Old tag - current constraint is not compulsory - making weightPercentage=0%\n";
								cn->weightPercentage=0;
							}
						}
						else if(elem4.tagName()=="Subject"){
							cn->subjectName=elem4.text();
							xmlReadingLog+="    Read subject="+cn->subjectName+"\n";
						}
						else if(elem4.tagName()=="Subject_Tag"){
							cn->activityTagName=elem4.text();
							xmlReadingLog+="    Read activity tag="+cn->activityTagName+"\n";
						}
						else if(elem4.tagName()=="Number_of_Preferred_Rooms"){
							_n_preferred_rooms=elem4.text().toInt();
							xmlReadingLog+="    Read number of preferred rooms: "+QString::number(_n_preferred_rooms)+"\n";
							assert(_n_preferred_rooms>=2);
						}
						else if(elem4.tagName()=="Preferred_Room"){
							cn->roomsNames.append(elem4.text());
							xmlReadingLog+="    Read room="+elem4.text()+"\n";
						}
					}
					assert(_n_preferred_rooms==cn->roomsNames.count());
					return cn;
}

SpaceConstraint* Rules::readSubjectActivityTagPreferredRoom(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintSubjectActivityTagPreferredRoom");
					ConstraintSubjectActivityTagPreferredRoom* cn=new ConstraintSubjectActivityTagPreferredRoom();
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight"){
							//cn->weight=elem4.text().toDouble();
							xmlReadingLog+="    Ignoring old tag - weight - making weight percentage=100\n";
							cn->weightPercentage=100;
						}
						else if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Compulsory"){
							if(elem4.text()=="yes"){
								//cn->compulsory=true;
								xmlReadingLog+="    Ignoring old tag - Current constraint is compulsory\n";
								cn->weightPercentage=100;
							}
							else{
								//cn->compulsory=false;
								xmlReadingLog+="    Old tag - current constraint is not compulsory - making weightPercentage=0%\n";
								cn->weightPercentage=0;
							}
						}
						else if(elem4.tagName()=="Subject"){
							cn->subjectName=elem4.text();
							xmlReadingLog+="    Read subject="+cn->subjectName+"\n";
						}
						else if(elem4.tagName()=="Activity_Tag"){
							cn->activityTagName=elem4.text();
							xmlReadingLog+="    Read activity tag="+cn->activityTagName+"\n";
						}
						else if(elem4.tagName()=="Room"){
							cn->roomName=elem4.text();
							xmlReadingLog+="    Read room="+cn->roomName+"\n";
						}
					}
					return cn;
}

SpaceConstraint* Rules::readSubjectActivityTagPreferredRooms(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintSubjectActivityTagPreferredRooms");
					int _n_preferred_rooms=0;
					ConstraintSubjectActivityTagPreferredRooms* cn=new ConstraintSubjectActivityTagPreferredRooms();
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight"){
							//cn->weight=elem4.text().toDouble();
							xmlReadingLog+="    Ignoring old tag - weight - making weight percentage=100\n";
							cn->weightPercentage=100;
						}
						else if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Compulsory"){
							if(elem4.text()=="yes"){
								//cn->compulsory=true;
								xmlReadingLog+="    Ignoring old tag - Current constraint is compulsory\n";
								cn->weightPercentage=100;
							}
							else{
								//cn->compulsory=false;
								xmlReadingLog+="    Old tag - current constraint is not compulsory - making weightPercentage=0%\n";
								cn->weightPercentage=0;
							}
						}
						else if(elem4.tagName()=="Subject"){
							cn->subjectName=elem4.text();
							xmlReadingLog+="    Read subject="+cn->subjectName+"\n";
						}
						else if(elem4.tagName()=="Activity_Tag"){
							cn->activityTagName=elem4.text();
							xmlReadingLog+="    Read activity tag="+cn->activityTagName+"\n";
						}
						else if(elem4.tagName()=="Number_of_Preferred_Rooms"){
							_n_preferred_rooms=elem4.text().toInt();
							xmlReadingLog+="    Read number of preferred rooms: "+QString::number(_n_preferred_rooms)+"\n";
							assert(_n_preferred_rooms>=2);
						}
						else if(elem4.tagName()=="Preferred_Room"){
							cn->roomsNames.append(elem4.text());
							xmlReadingLog+="    Read room="+elem4.text()+"\n";
						}
					}
					assert(_n_preferred_rooms==cn->roomsNames.count());
					return cn;
}

SpaceConstraint* Rules::readActivityTagPreferredRoom(const QDomElement& elem3, FakeString& xmlReadingLog){
					//added 6 apr 2009
					assert(elem3.tagName()=="ConstraintActivityTagPreferredRoom");
					ConstraintActivityTagPreferredRoom* cn=new ConstraintActivityTagPreferredRoom();
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Activity_Tag"){
							cn->activityTagName=elem4.text();
							xmlReadingLog+="    Read activity tag="+cn->activityTagName+"\n";
						}
						else if(elem4.tagName()=="Room"){
							cn->roomName=elem4.text();
							xmlReadingLog+="    Read room="+cn->roomName+"\n";
						}
					}
					return cn;
}

SpaceConstraint* Rules::readActivityTagPreferredRooms(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintActivityTagPreferredRooms");
					int _n_preferred_rooms=0;
					ConstraintActivityTagPreferredRooms* cn=new ConstraintActivityTagPreferredRooms();
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Activity_Tag"){
							cn->activityTagName=elem4.text();
							xmlReadingLog+="    Read activity tag="+cn->activityTagName+"\n";
						}
						else if(elem4.tagName()=="Number_of_Preferred_Rooms"){
							_n_preferred_rooms=elem4.text().toInt();
							xmlReadingLog+="    Read number of preferred rooms: "+QString::number(_n_preferred_rooms)+"\n";
							assert(_n_preferred_rooms>=2);
						}
						else if(elem4.tagName()=="Preferred_Room"){
							cn->roomsNames.append(elem4.text());
							xmlReadingLog+="    Read room="+elem4.text()+"\n";
						}
					}
					assert(_n_preferred_rooms==cn->roomsNames.count());
					return cn;
}

SpaceConstraint* Rules::readStudentsSetHomeRoom(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintStudentsSetHomeRoom");
					ConstraintStudentsSetHomeRoom* cn=new ConstraintStudentsSetHomeRoom();
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Students"){
							cn->studentsName=elem4.text();
							xmlReadingLog+="    Read students="+cn->studentsName+"\n";
						}
						else if(elem4.tagName()=="Room"){
							cn->roomName=elem4.text();
							xmlReadingLog+="    Read room="+cn->roomName+"\n";
						}
					}
					return cn;
}

SpaceConstraint* Rules::readStudentsSetHomeRooms(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintStudentsSetHomeRooms");
					int _n_preferred_rooms=0;
					ConstraintStudentsSetHomeRooms* cn=new ConstraintStudentsSetHomeRooms();
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Students"){
							cn->studentsName=elem4.text();
							xmlReadingLog+="    Read students="+cn->studentsName+"\n";
						}
						else if(elem4.tagName()=="Number_of_Preferred_Rooms"){
							_n_preferred_rooms=elem4.text().toInt();
							xmlReadingLog+="    Read number of preferred rooms: "+QString::number(_n_preferred_rooms)+"\n";
							assert(_n_preferred_rooms>=2);
						}
						else if(elem4.tagName()=="Preferred_Room"){
							cn->roomsNames.append(elem4.text());
							xmlReadingLog+="    Read room="+elem4.text()+"\n";
						}
					}
					assert(_n_preferred_rooms==cn->roomsNames.count());
					return cn;
}

SpaceConstraint* Rules::readTeacherHomeRoom(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintTeacherHomeRoom");
					ConstraintTeacherHomeRoom* cn=new ConstraintTeacherHomeRoom();
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Teacher"){
							cn->teacherName=elem4.text();
							xmlReadingLog+="    Read teacher="+cn->teacherName+"\n";
						}
						else if(elem4.tagName()=="Room"){
							cn->roomName=elem4.text();
							xmlReadingLog+="    Read room="+cn->roomName+"\n";
						}
					}
					return cn;
}

SpaceConstraint* Rules::readTeacherHomeRooms(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintTeacherHomeRooms");
					int _n_preferred_rooms=0;
					ConstraintTeacherHomeRooms* cn=new ConstraintTeacherHomeRooms();
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Teacher"){
							cn->teacherName=elem4.text();
							xmlReadingLog+="    Read teacher="+cn->teacherName+"\n";
						}
						else if(elem4.tagName()=="Number_of_Preferred_Rooms"){
							_n_preferred_rooms=elem4.text().toInt();
							xmlReadingLog+="    Read number of preferred rooms: "+QString::number(_n_preferred_rooms)+"\n";
							assert(_n_preferred_rooms>=2);
						}
						else if(elem4.tagName()=="Preferred_Room"){
							cn->roomsNames.append(elem4.text());
							xmlReadingLog+="    Read room="+elem4.text()+"\n";
						}
					}
					assert(_n_preferred_rooms==cn->roomsNames.count());
					return cn;
}

SpaceConstraint* Rules::readTeacherMaxBuildingChangesPerDay(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintTeacherMaxBuildingChangesPerDay");
					ConstraintTeacherMaxBuildingChangesPerDay* cn=new ConstraintTeacherMaxBuildingChangesPerDay();
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Teacher"){
							cn->teacherName=elem4.text();
							xmlReadingLog+="    Read teacher name="+cn->teacherName+"\n";
						}
						else if(elem4.tagName()=="Max_Building_Changes_Per_Day"){
							cn->maxBuildingChangesPerDay=elem4.text().toInt();
							xmlReadingLog+="    Max. building changes per day="+QString::number(cn->maxBuildingChangesPerDay)+"\n";
						}
					}
					return cn;
}

SpaceConstraint* Rules::readTeachersMaxBuildingChangesPerDay(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintTeachersMaxBuildingChangesPerDay");
					ConstraintTeachersMaxBuildingChangesPerDay* cn=new ConstraintTeachersMaxBuildingChangesPerDay();
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Max_Building_Changes_Per_Day"){
							cn->maxBuildingChangesPerDay=elem4.text().toInt();
							xmlReadingLog+="    Max. building changes per day="+QString::number(cn->maxBuildingChangesPerDay)+"\n";
						}
					}
					return cn;
}

SpaceConstraint* Rules::readTeacherMaxBuildingChangesPerWeek(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintTeacherMaxBuildingChangesPerWeek");
					ConstraintTeacherMaxBuildingChangesPerWeek* cn=new ConstraintTeacherMaxBuildingChangesPerWeek();
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Teacher"){
							cn->teacherName=elem4.text();
							xmlReadingLog+="    Read teacher name="+cn->teacherName+"\n";
						}
						else if(elem4.tagName()=="Max_Building_Changes_Per_Week"){
							cn->maxBuildingChangesPerWeek=elem4.text().toInt();
							xmlReadingLog+="    Max. building changes per week="+QString::number(cn->maxBuildingChangesPerWeek)+"\n";
						}
					}
					return cn;
}

SpaceConstraint* Rules::readTeachersMaxBuildingChangesPerWeek(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintTeachersMaxBuildingChangesPerWeek");
					ConstraintTeachersMaxBuildingChangesPerWeek* cn=new ConstraintTeachersMaxBuildingChangesPerWeek();
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Max_Building_Changes_Per_Week"){
							cn->maxBuildingChangesPerWeek=elem4.text().toInt();
							xmlReadingLog+="    Max. building changes per week="+QString::number(cn->maxBuildingChangesPerWeek)+"\n";
						}
					}
					return cn;
}

SpaceConstraint* Rules::readTeacherMinGapsBetweenBuildingChanges(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintTeacherMinGapsBetweenBuildingChanges");
					ConstraintTeacherMinGapsBetweenBuildingChanges* cn=new ConstraintTeacherMinGapsBetweenBuildingChanges();
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Teacher"){
							cn->teacherName=elem4.text();
							xmlReadingLog+="    Read teacher name="+cn->teacherName+"\n";
						}
						else if(elem4.tagName()=="Min_Gaps_Between_Building_Changes"){
							cn->minGapsBetweenBuildingChanges=elem4.text().toInt();
							xmlReadingLog+="    Min gaps between building changes="+QString::number(cn->minGapsBetweenBuildingChanges)+"\n";
						}
					}
					return cn;
}

SpaceConstraint* Rules::readTeachersMinGapsBetweenBuildingChanges(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintTeachersMinGapsBetweenBuildingChanges");
					ConstraintTeachersMinGapsBetweenBuildingChanges* cn=new ConstraintTeachersMinGapsBetweenBuildingChanges();
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Min_Gaps_Between_Building_Changes"){
							cn->minGapsBetweenBuildingChanges=elem4.text().toInt();
							xmlReadingLog+="    Min gaps between building changes="+QString::number(cn->minGapsBetweenBuildingChanges)+"\n";
						}
					}
					return cn;
}

SpaceConstraint* Rules::readStudentsSetMaxBuildingChangesPerDay(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintStudentsSetMaxBuildingChangesPerDay");
					ConstraintStudentsSetMaxBuildingChangesPerDay* cn=new ConstraintStudentsSetMaxBuildingChangesPerDay();
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Students"){
							cn->studentsName=elem4.text();
							xmlReadingLog+="    Read students name="+cn->studentsName+"\n";
						}
						else if(elem4.tagName()=="Max_Building_Changes_Per_Day"){
							cn->maxBuildingChangesPerDay=elem4.text().toInt();
							xmlReadingLog+="    Max. building changes per day="+QString::number(cn->maxBuildingChangesPerDay)+"\n";
						}
					}
					return cn;
}

SpaceConstraint* Rules::readStudentsMaxBuildingChangesPerDay(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintStudentsMaxBuildingChangesPerDay");
					ConstraintStudentsMaxBuildingChangesPerDay* cn=new ConstraintStudentsMaxBuildingChangesPerDay();
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Max_Building_Changes_Per_Day"){
							cn->maxBuildingChangesPerDay=elem4.text().toInt();
							xmlReadingLog+="    Max. building changes per day="+QString::number(cn->maxBuildingChangesPerDay)+"\n";
						}
					}
					return cn;
}

SpaceConstraint* Rules::readStudentsSetMaxBuildingChangesPerWeek(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintStudentsSetMaxBuildingChangesPerWeek");
					ConstraintStudentsSetMaxBuildingChangesPerWeek* cn=new ConstraintStudentsSetMaxBuildingChangesPerWeek();
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Students"){
							cn->studentsName=elem4.text();
							xmlReadingLog+="    Read students name="+cn->studentsName+"\n";
						}
						else if(elem4.tagName()=="Max_Building_Changes_Per_Week"){
							cn->maxBuildingChangesPerWeek=elem4.text().toInt();
							xmlReadingLog+="    Max. building changes per week="+QString::number(cn->maxBuildingChangesPerWeek)+"\n";
						}
					}
					return cn;
}

SpaceConstraint* Rules::readStudentsMaxBuildingChangesPerWeek(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintStudentsMaxBuildingChangesPerWeek");
					ConstraintStudentsMaxBuildingChangesPerWeek* cn=new ConstraintStudentsMaxBuildingChangesPerWeek();
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Max_Building_Changes_Per_Week"){
							cn->maxBuildingChangesPerWeek=elem4.text().toInt();
							xmlReadingLog+="    Max. building changes per week="+QString::number(cn->maxBuildingChangesPerWeek)+"\n";
						}
					}
					return cn;
}

SpaceConstraint* Rules::readStudentsSetMinGapsBetweenBuildingChanges(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintStudentsSetMinGapsBetweenBuildingChanges");
					ConstraintStudentsSetMinGapsBetweenBuildingChanges* cn=new ConstraintStudentsSetMinGapsBetweenBuildingChanges();
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Students"){
							cn->studentsName=elem4.text();
							xmlReadingLog+="    Read students name="+cn->studentsName+"\n";
						}
						else if(elem4.tagName()=="Min_Gaps_Between_Building_Changes"){
							cn->minGapsBetweenBuildingChanges=elem4.text().toInt();
							xmlReadingLog+="    min gaps between building changes="+QString::number(cn->minGapsBetweenBuildingChanges)+"\n";
						}
					}
					return cn;
}

SpaceConstraint* Rules::readStudentsMinGapsBetweenBuildingChanges(const QDomElement& elem3, FakeString& xmlReadingLog){
					assert(elem3.tagName()=="ConstraintStudentsMinGapsBetweenBuildingChanges");
					ConstraintStudentsMinGapsBetweenBuildingChanges* cn=new ConstraintStudentsMinGapsBetweenBuildingChanges();
					for(QDomNode node4=elem3.firstChild(); !node4.isNull(); node4=node4.nextSibling()){
						QDomElement elem4=node4.toElement();
						if(elem4.isNull()){
							xmlReadingLog+="    Null node here\n";
							continue;
						}
						xmlReadingLog+="    Found "+elem4.tagName()+" tag\n";
						if(elem4.tagName()=="Weight_Percentage"){
							cn->weightPercentage=elem4.text().toDouble();
							xmlReadingLog+="    Adding weight percentage="+QString::number(cn->weightPercentage)+"\n";
						}
						else if(elem4.tagName()=="Min_Gaps_Between_Building_Changes"){
							cn->minGapsBetweenBuildingChanges=elem4.text().toInt();
							xmlReadingLog+="    min gaps between building changes="+QString::number(cn->minGapsBetweenBuildingChanges)+"\n";
						}
					}
					return cn;
}
