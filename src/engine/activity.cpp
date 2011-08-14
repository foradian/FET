/*
File activity.cpp 
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
#include "activity.h"
#include "rules.h"

#include <QMessageBox>

#include <iostream>

using namespace std;

Activity::Activity()
{
}

Activity::Activity(
	Rules& r,
	int _id,
	int _activityGroupId,
	const QStringList& _teachersNames,
	const QString& _subjectName,
	const QStringList& _activityTagsNames,
	const QStringList& _studentsNames,
	int _duration,
	int _totalDuration,
	bool _active,
	bool _computeNTotalStudents,
	int _nTotalStudents)
{
	this->id=_id;
	this->activityGroupId=_activityGroupId;
	this->teachersNames = _teachersNames;
	this->subjectName = _subjectName;
	this->activityTagsNames = _activityTagsNames;
	this->studentsNames = _studentsNames;
	this->duration=_duration;
	this->totalDuration=_totalDuration;
	//this->parity=_parity;
	this->active=_active;
	this->computeNTotalStudents=_computeNTotalStudents;
	
	if(_computeNTotalStudents==true){	
		this->nTotalStudents=0;
		for(QStringList::Iterator it=this->studentsNames.begin(); it!=this->studentsNames.end(); it++){
			StudentsSet* ss=r.searchStudentsSet(*it);
			this->nTotalStudents += ss->numberOfStudents;
		}
	}
	else{
		assert(_nTotalStudents>=0);
		this->nTotalStudents=_nTotalStudents;
	}
}

Activity::Activity(
	Rules& r,
	int _id,
	int _activityGroupId,
	const QStringList& _teachersNames,
	const QString& _subjectName,
	const QStringList& _activityTagsNames,
	const QStringList& _studentsNames,
	int _duration,
	int _totalDuration,
	bool _active,
	bool _computeNTotalStudents,
	int _nTotalStudents,
	int _computedNumberOfStudents)
{
	Q_UNUSED(r);
	Q_UNUSED(_nTotalStudents);

	this->id=_id;
	this->activityGroupId=_activityGroupId;
	this->teachersNames = _teachersNames;
	this->subjectName = _subjectName;
	this->activityTagsNames = _activityTagsNames;
	this->studentsNames = _studentsNames;
	this->duration=_duration;
	this->totalDuration=_totalDuration;
	//this->parity=_parity;
	this->active=_active;
	this->computeNTotalStudents=_computeNTotalStudents;
	
	assert(_computeNTotalStudents);
	this->nTotalStudents=_computedNumberOfStudents;
}

bool Activity::operator==(Activity& a)
{
	if(this->teachersNames != a.teachersNames)
		return false;
	if(this->subjectName != a.subjectName)
		return false;
	if(this->activityTagsNames != a.activityTagsNames)
		return false;
	if(this->studentsNames != a.studentsNames)
		return false;
	//if(this->duration != a.duration)
	  //  return false;
	if((this->activityGroupId==0 && a.activityGroupId!=0) || (this->activityGroupId!=0 && a.activityGroupId==0))
		return false;
	return true;
}

bool Activity::searchTeacher(const QString& teacherName)
{
	return this->teachersNames.find(teacherName)!=this->teachersNames.end();
}

bool Activity::removeTeacher(const QString& teacherName)
{
	int t=this->teachersNames.removeAll(teacherName);
	
	return t>0;
}

void Activity::renameTeacher(const QString& initialTeacherName, const QString& finalTeacherName)
{
	int t=0;
	for(QStringList::iterator it=this->teachersNames.begin(); it!=this->teachersNames.end(); it++)
		if((*it)==initialTeacherName){
			*it=finalTeacherName;
			t++;
		}
	assert(t<=1);
}

bool Activity::searchStudents(const QString& studentsName)
{
	return this->studentsNames.find(studentsName)!=this->studentsNames.end();
}

bool Activity::removeStudents(Rules& r, const QString& studentsName, int nStudents)
{
	Q_UNUSED(r);

	int t=this->studentsNames.removeAll(studentsName);

	if(t>0 && this->computeNTotalStudents==true){
		/*StudentsSet* s=r.searchStudentsSet(studentsName);
		assert(s!=NULL);
		this->nTotalStudents-=s->numberOfStudents;*/
		this->nTotalStudents-=nStudents;
		assert(this->nTotalStudents>=0);
	}
	
	return t>0;
}

void Activity::renameStudents(Rules& r, const QString& initialStudentsName, const QString& finalStudentsName, int initialNumberOfStudents, int finalNumberOfStudents)
{
	Q_UNUSED(r);

	int t=0;
	for(QStringList::iterator it=this->studentsNames.begin(); it!=this->studentsNames.end(); it++)
		if((*it)==initialStudentsName){
			/*if(this->computeNTotalStudents==true){
				StudentsSet* s=r.searchStudentsSet(initialStudentsName);
				assert(s!=NULL);
				this->nTotalStudents-=s->numberOfStudents;
				
				StudentsSet* s2=r.searchStudentsSet(finalStudentsName);
				assert(s2!=NULL);
				this->nTotalStudents+=s2->numberOfStudents;
				
				assert(this->nTotalStudents>=0);
			}*/
		
			*it=finalStudentsName;
			t++;
			
			if(this->computeNTotalStudents){
				assert(initialNumberOfStudents>=0);
				assert(finalNumberOfStudents>=0);
			
				nTotalStudents-=initialNumberOfStudents;
				assert(nTotalStudents>=0);
				nTotalStudents+=finalNumberOfStudents;
			}
		}
	assert(t<=1);
}

void Activity::computeInternalStructure(Rules& r)
{
	//the internal subgroups list must be computed before entering here.

	//teachers
	//this->nTeachers=0;
	this->iTeachersList.clear();
	for(QStringList::Iterator it=this->teachersNames.begin(); it!=this->teachersNames.end(); it++){
		int tmp;
		for(tmp=0; tmp<r.nInternalTeachers; tmp++){
			if(r.internalTeachersList[tmp]->name == (*it))
				break;
		}
		assert(tmp < r.nInternalTeachers);
		//assert(this->nTeachers<MAX_TEACHERS_PER_ACTIVITY);
		//this->teachers[this->nTeachers++]=tmp;
		this->iTeachersList.append(tmp);
	}

	//subjects
	this->subjectIndex = r.searchSubject(this->subjectName);
	assert(this->subjectIndex>=0);

	//activity tags
	this->iActivityTagsSet.clear();
	foreach(QString tag, this->activityTagsNames){
		assert(tag!="");
		int index=r.searchActivityTag(tag);
		assert(index>=0);
		this->iActivityTagsSet.insert(index);
	}
	//this->activityTagIndex = r.searchActivityTag(this->activityTagName);

	//students	
	//this->nSubgroups=0;
	this->iSubgroupsList.clear();
	for(QStringList::Iterator it=this->studentsNames.begin(); it!=this->studentsNames.end(); it++){
		StudentsSet* ss=r.searchAugmentedStudentsSet(*it);
		assert(ss);
		if(ss->type==STUDENTS_SUBGROUP){
			int tmp;
			/*for(tmp=0; tmp<r.nInternalSubgroups; tmp++)
				if(r.internalSubgroupsList[tmp]->name == ss->name)
					break;*/
			tmp=((StudentsSubgroup*)ss)->indexInInternalSubgroupsList;
			assert(tmp>=0);
			assert(tmp<r.nInternalSubgroups);
			//assert(this->nSubgroups<MAX_SUBGROUPS_PER_ACTIVITY);
			
			bool duplicate=false;
			if(this->iSubgroupsList.contains(tmp))
			//for(int j=0; j<this->nSubgroups; j++)
			//	if(this->subgroups[j]==tmp)
					duplicate=true;
			if(duplicate){
				/*QString s;
				s=QString("Warning: activity with id=%1 contains duplicated subgroups. Automatically correcting...")
					.arg(this->id);
				cout<<qPrintable(s)<<endl;*/
			}
			else
				this->iSubgroupsList.append(tmp);
				//this->subgroups[this->nSubgroups++]=tmp;
		}
		else if(ss->type==STUDENTS_GROUP){
			StudentsGroup* stg=(StudentsGroup*)ss;
			for(int k=0; k<stg->subgroupsList.size(); k++){
				StudentsSubgroup* sts=stg->subgroupsList[k];
				int tmp;
				/*for(tmp=0; tmp<r.nInternalSubgroups; tmp++)
					if(r.internalSubgroupsList[tmp]->name == sts->name)
						break;*/
				tmp=sts->indexInInternalSubgroupsList;
				assert(tmp>=0);
				assert(tmp<r.nInternalSubgroups);
				//assert(this->nSubgroups<MAX_SUBGROUPS_PER_ACTIVITY);

				bool duplicate=false;
				if(this->iSubgroupsList.contains(tmp))
				//for(int j=0; j<this->nSubgroups; j++)
				//	if(this->subgroups[j]==tmp)
						duplicate=true;
				if(duplicate){
					/*QString s;
					s=QString("Warning: activity with id=%1 contains duplicated subgroups. Automatically correcting...")
						.arg(this->id);
					cout<<qPrintable(s)<<endl;*/
				}
				else
					//this->subgroups[this->nSubgroups++]=tmp;
					this->iSubgroupsList.append(tmp);
			}
		}
		else if(ss->type==STUDENTS_YEAR){
			StudentsYear* sty=(StudentsYear*)ss;
			for(int k=0; k<sty->groupsList.size(); k++){
				StudentsGroup* stg=sty->groupsList[k];
				for(int l=0; l<stg->subgroupsList.size(); l++){
					StudentsSubgroup* sts=stg->subgroupsList[l];
					int tmp;
					/*for(tmp=0; tmp<r.nInternalSubgroups; tmp++)
						if(r.internalSubgroupsList[tmp]->name == sts->name)
							break;*/
					tmp=sts->indexInInternalSubgroupsList;
					assert(tmp>=0);
					assert(tmp<r.nInternalSubgroups);
					//assert(this->nSubgroups<MAX_SUBGROUPS_PER_ACTIVITY);

					bool duplicate=false;
					if(this->iSubgroupsList.contains(tmp))
					//for(int j=0; j<this->nSubgroups; j++)
					//	if(this->subgroups[j]==tmp)
							duplicate=true;
					if(duplicate){
						/*QString s;
						s=QString("Warning: activity with id=%1 contains duplicated subgroups. Automatically correcting...")
							.arg(this->id);
						cout<<qPrintable(s)<<endl;*/
					}
					else{
						//this->subgroups[this->nSubgroups++]=tmp;
						this->iSubgroupsList.append(tmp);
					}
				}
			}
		}
		else
			assert(0);
	}
}

QString Activity::getXmlDescription(Rules& r)
{
	Q_UNUSED(r);

	QString s="<Activity>\n";
	for(QStringList::Iterator it=this->teachersNames.begin(); it!=this->teachersNames.end(); it++)
		s+="	<Teacher>" + protect(*it) + "</Teacher>\n";
	s+="	<Subject>"+protect(this->subjectName)+"</Subject>\n";

	foreach(QString tag, this->activityTagsNames)
		s+="	<Activity_Tag>"+protect(tag)+"</Activity_Tag>\n";

	s+="	<Duration>"+QString::number(this->duration)+"</Duration>\n";
	s+="	<Total_Duration>"+QString::number(this->totalDuration)+"</Total_Duration>\n";
	s+="	<Id>"+QString::number(this->id)+"</Id>\n";
	s+="	<Activity_Group_Id>"+QString::number(this->activityGroupId)+"</Activity_Group_Id>\n";
	/*if(this->parity==PARITY_WEEKLY)
		s+="	<Weekly></Weekly>\n";
	else{
		assert(this->parity==PARITY_FORTNIGHTLY);
		s+="	<Fortnightly></Fortnightly>\n";
	}*/
	if(this->active==true){
		//s+="	<Active>yes</Active>\n";
		s+="	<Active>true</Active>\n";
	}
	else{
		//s+="	<Active>no</Active>\n";
		s+="	<Active>false</Active>\n";
	}
	for(QStringList::Iterator it=this->studentsNames.begin(); it!=this->studentsNames.end(); it++)
		s+="	<Students>" + protect(*it) + "</Students>\n";

	if(this->computeNTotalStudents==false)
		s+="	<Number_Of_Students>"+QString::number(this->nTotalStudents)+"</Number_Of_Students>\n";

	s+="</Activity>";

	return s;
}

QString Activity::getDescription(Rules& r)
{
	const int INDENT=4;

	Q_UNUSED(r);
	
	bool _indent;
	if(this->isSplit() && this->id!=this->activityGroupId)
		_indent=true;
	else
		_indent=false;
		
	bool indentRepr;
	if(this->isSplit() && this->id==this->activityGroupId)
		indentRepr=true;
	else
		indentRepr=false;


		
	QString _teachers="";
	if(teachersNames.count()==0)
		_teachers=tr("no teachers");
	else
		_teachers=this->teachersNames.join(",");

	QString _subject=this->subjectName;
	
	QString _activityTags=this->activityTagsNames.join(",");

	QString _students="";
	if(studentsNames.count()==0)
		_students=tr("no students");
	else
		_students=this->studentsNames.join(",");

	QString _id;
	_id = QString::number(id);

	QString _agid="";
	if(this->isSplit())
		_agid = QString::number(this->activityGroupId);

	QString _duration=QString::number(this->duration);
	
	QString _totalDuration="";
	if(this->isSplit())
		_totalDuration = QString::number(this->totalDuration);

	QString _active;
	if(this->active==true)
		_active="";
	else
		_active="X";

	QString _nstudents="";
	if(this->computeNTotalStudents==false)
		_nstudents=QString::number(this->nTotalStudents);

	/////////
	QString s="";
	if(_indent)
		s+=QString(INDENT, ' ');
		
	s+=_id;
	s+=" - ";

	if(indentRepr)
		s+=QString(INDENT, ' ');
		
	if(_active!=""){
		s+=_active;
		s+=" - ";
	}
	
	s+=_duration;
	if(this->isSplit()){
		s+="/";
		s+=_totalDuration;
	}
	s+=" - ";
	
	s+=_teachers;
	s+=" - ";
	s+=_subject;
	s+=" - ";
	if(_activityTags!=""){
		s+=_activityTags;
		s+=" - ";
	}
	s+=_students;

	if(_nstudents!=""){
		s+=" - ";
		s+=_nstudents;
	}



	//if there is left alignment, I do a double padding with spaces in RTL languages, just to make sure
	if(_indent && LANGUAGE_STYLE_RIGHT_TO_LEFT==true) //this is because old Q3ListBox is left aligned even in RTL layout
		s+=QString(INDENT, ' ');

	return s;
}

QString Activity::getDetailedDescription(Rules& r)
{
	Q_UNUSED(r);

	QString s;

	s=tr("Activity:");
	s+="\n";

	//Id, AGId
	s += tr("Id=%1").arg(QString::number(id));
	s+="\n";
	if(this->isSplit()){
		s += tr("Activity group id=%1").arg(QString::number(this->activityGroupId));
		s+="\n";
	}

	//Active?
	QString activeYesNo;
	if(this->active==true)
		activeYesNo=tr("yes");
	else
		activeYesNo=tr("no");
	s+=tr("Active=%1", "Represents a boolean value, if activity is active or not, %1 is yes or no").arg(activeYesNo);
	s+="\n";

	//Dur, TD
	s+=tr("Duration=%1").arg(QString::number(this->duration));
	s+="\n";
	if(this->isSplit()){
		s += tr("Total duration=%1").arg(QString::number(this->totalDuration));
		s+="\n";
	}
	
	if(teachersNames.count()==0){
		s+=tr("No teachers for this activity");
		s+="\n";
	}
	else
		for(QStringList::Iterator it=this->teachersNames.begin(); it!=this->teachersNames.end(); it++){
			s+=tr("Teacher=%1").arg(*it);
			s+="\n";
		}

	s+=tr("Subject=%1").arg(this->subjectName);
	s+="\n";
	foreach(QString tag, this->activityTagsNames){
		assert(tag!="");
		s+=tr("Activity tag=%1").arg(tag);
		s+="\n";
	}

	if(studentsNames.count()==0){
		s+=tr("No students sets for this activity");
		s+="\n";
	}
	else
		for(QStringList::Iterator it=this->studentsNames.begin(); it!=this->studentsNames.end(); it++){
			s += tr("Students=%1").arg(*it);
			s+="\n";
		}
		
	if(this->computeNTotalStudents==true){
		/*int nStud=0;
		for(QStringList::Iterator it=this->studentsNames.begin(); it!=this->studentsNames.end(); it++){
			StudentsSet* ss=r.searchStudentsSet(*it);
			nStud += ss->numberOfStudents;
		}*/
		int nStud=this->nTotalStudents;
		s+=tr("Total number of students=%1").arg(nStud);
		s+="\n";
	}
	else{
		s+=tr("Total number of students=%1").arg(this->nTotalStudents);
		s+=" ("+tr("specified", "Specified means that the total number of students was specified separately for the activity")+")";
		s+="\n";
	}

	return s;
}

QString Activity::getDetailedDescriptionWithConstraints(Rules& r)
{
	QString s=this->getDetailedDescription(r);

	s+="--------------------------------------------------\n";
	s+=tr("Time constraints directly related to this activity:");
	s+="\n";
	for(int i=0; i<r.timeConstraintsList.size(); i++){
		TimeConstraint* c=r.timeConstraintsList[i];
		if(c->isRelatedToActivity(r, this)){
			s+="\n";
			s+=c->getDetailedDescription(r);
		}
	}

	s+="--------------------------------------------------\n";
	s+=tr("Space constraints directly related to this activity:");
	s+="\n";
	for(int i=0; i<r.spaceConstraintsList.size(); i++){
		SpaceConstraint* c=r.spaceConstraintsList[i];
		if(c->isRelatedToActivity(this)){
			s+="\n";
			s+=c->getDetailedDescription(r);
		}
	}
	s+="--------------------------------------------------\n";

	return s;
}

bool Activity::isSplit()
{
	return this->totalDuration != this->duration;
}

bool Activity::representsComponentNumber(int index)
{
	if(this->activityGroupId==0)
		return index==1;
		//return false;
		
	//assert(this->activityGroupId>0);
	
	return index == (this->id - this->activityGroupId + 1);
}
