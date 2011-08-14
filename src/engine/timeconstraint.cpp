/*
File timeconstraint.cpp
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

/*#include <QFile>
#include <QTextStream>*/

#include "timetable_defs.h"
#include "timeconstraint.h"
#include "rules.h"
#include "solution.h"
#include "activity.h"
#include "teacher.h"
#include "subject.h"
#include "activitytag.h"
#include "studentsset.h"

#include "matrix.h"

#include <QString>

#include <QMessageBox>

#include <iostream>
using namespace std;

//for min max functions
#include <algorithm>

//#define yesNo(x)				((x)==0?"no":"yes")

///#define trueFalse(x)			((x)==0?"false":"true")
static QString trueFalse(bool x){
	if(!x)
		return QString("false");
	else
		return QString("true");
}

///#define yesNoTranslated(x)		((x)==0?tr("no"):tr("yes"))
static QString yesNoTranslated(bool x){
	if(!x)
		return QCoreApplication::translate("TimeConstraint", "no", "no - meaning negation");
	else
		return QCoreApplication::translate("TimeConstraint", "yes", "yes - meaning affirmative");
}

///#define minimu(x,y)	((x)<(y)?(x):(y))
/*static int minimu(int x, int y){
	if(x<y)
		return x;
	else
		return y;
}
*/

///#define maximu(x,y)	((x)>(y)?(x):(y))
/*static int maximu(int x, int y){
	if(x>y)
		return x;
	else
		return y;
}
*/

//static Solution* crt_chrom=NULL;
//static Rules* crt_rules=NULL;

//The following 2 matrices are kept to make the computation faster
//They are calculated only at the beginning of the computation of the fitness
//of the solution.
/*static qint8 subgroupsMatrix[MAX_TOTAL_SUBGROUPS][MAX_DAYS_PER_WEEK][MAX_HOURS_PER_DAY];
static qint8 teachersMatrix[MAX_TEACHERS][MAX_DAYS_PER_WEEK][MAX_HOURS_PER_DAY];*/
static Matrix3D<qint8> subgroupsMatrix;
static Matrix3D<qint8> teachersMatrix;

static int teachers_conflicts=-1;
static int subgroups_conflicts=-1;

//extern bool breakDayHour[MAX_DAYS_PER_WEEK][MAX_HOURS_PER_DAY];
extern Matrix2D<bool> breakDayHour;

/*extern bool teacherNotAvailableDayHour[MAX_TEACHERS][MAX_DAYS_PER_WEEK][MAX_HOURS_PER_DAY];

extern bool subgroupNotAvailableDayHour[MAX_TOTAL_SUBGROUPS][MAX_DAYS_PER_WEEK][MAX_HOURS_PER_DAY];*/
extern Matrix3D<bool> teacherNotAvailableDayHour;

extern Matrix3D<bool> subgroupNotAvailableDayHour;


/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

QString getActivityDetailedDescription(Rules& r, int id){
	QString s;

	int ai;
	for(ai=0; ai<r.activitiesList.size(); ai++)
		if(r.activitiesList[ai]->id==id)
			break;
	
	if(ai==r.activitiesList.size()){
		s+=QCoreApplication::translate("Activity", "Invalid (inexistent) id for activity");
		return s;
	}
	assert(ai<r.activitiesList.size());
	
	Activity* act=r.activitiesList.at(ai);
	
	if(act->activityTagsNames.count()>0){
		s+=QCoreApplication::translate("Activity", "T:%1, S:%2, AT:%3, St:%4", "This is an important translation for an activity's detailed description, please take care (it appears in many places in constraints)."
		 "The abbreviations are: Teachers, Subject, Activity tags, Students. This variant includes activity tags").arg(act->teachersNames.join(",")).arg(act->subjectName).arg(act->activityTagsNames.join(",")).arg(act->studentsNames.join(","));
	}
	else{
		s+=QCoreApplication::translate("Activity", "T:%1, S:%2, St:%3", "This is an important translation for an activity's detailed description, please take care (it appears in many places in constraints)."
		 "The abbreviations are: Teachers, Subject, Students. There are no activity tags here").arg(act->teachersNames.join(",")).arg(act->subjectName).arg(act->studentsNames.join(","));
	}
	return s;
}

TimeConstraint::TimeConstraint()
{
	type=CONSTRAINT_GENERIC_TIME;
}

TimeConstraint::~TimeConstraint()
{
}

TimeConstraint::TimeConstraint(double wp)
{
	weightPercentage=wp;
	//assert(wp<=100.0 && wp>=0.0 && wp==floor(wp) && wp==ceil(wp)); //integer, for now
}

/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

ConstraintBasicCompulsoryTime::ConstraintBasicCompulsoryTime(): TimeConstraint()
{
	this->type=CONSTRAINT_BASIC_COMPULSORY_TIME;
	this->weightPercentage=100;
}

ConstraintBasicCompulsoryTime::ConstraintBasicCompulsoryTime(double wp): TimeConstraint(wp)
{
	this->type=CONSTRAINT_BASIC_COMPULSORY_TIME;
}

bool ConstraintBasicCompulsoryTime::computeInternalStructure(Rules& r)
{
	Q_UNUSED(r);
	//if(&r!=NULL)
	//	;
	/*do nothing*/
	
	return true;
}

bool ConstraintBasicCompulsoryTime::hasInactiveActivities(Rules& r)
{
	Q_UNUSED(r);
	return false;
}

QString ConstraintBasicCompulsoryTime::getXmlDescription(Rules& r)
{
	Q_UNUSED(r);

	QString s = "<ConstraintBasicCompulsoryTime>\n";
	s += "	<Weight_Percentage>"+QString::number(this->weightPercentage)+"</Weight_Percentage>\n";
	assert(this->weightPercentage==100.0);
	/*s+="	<Compulsory>";
	s+=yesNo(this->compulsory);
	s+="</Compulsory>\n";*/
	s += "</ConstraintBasicCompulsoryTime>\n";
	return s;
}

QString ConstraintBasicCompulsoryTime::getDescription(Rules& r)
{
	Q_UNUSED(r);

	return tr("Basic compulsory constraints (time)") + ", " + tr("WP:%1\%", "Weight percentage").arg(this->weightPercentage);
}

QString ConstraintBasicCompulsoryTime::getDetailedDescription(Rules& r)
{
	Q_UNUSED(r);	

	QString s=tr("These are the basic compulsory constraints (referring to time allocation) for any timetable");
	s+="\n";

	s+=tr("Weight (percentage)=%1\%").arg(this->weightPercentage);s+="\n";
	s+=tr("The basic time constraints try to avoid:");s+="\n";
	s+=QString("- ");s+=tr("teachers assigned to more than one activity simultaneously");s+="\n";
	s+=QString("- ");s+=tr("students assigned to more than one activity simultaneously");s+="\n";

	return s;
}

double ConstraintBasicCompulsoryTime::fitness(Solution& c, Rules& r, QList<double>& cl, QList<QString>&dl, QString* conflictsString){
	assert(r.internalStructureComputed);

	int teachersConflicts, subgroupsConflicts;
	
	assert(weightPercentage==100.0);

	//This constraint fitness calculation routine is called firstly,
	//so we can compute the teacher and subgroups conflicts faster this way.
	if(!c.teachersMatrixReady || !c.subgroupsMatrixReady){
		c.teachersMatrixReady=true;
		c.subgroupsMatrixReady=true;
	
		subgroups_conflicts = subgroupsConflicts = c.getSubgroupsMatrix(r, subgroupsMatrix);
		teachers_conflicts = teachersConflicts = c.getTeachersMatrix(r, teachersMatrix);

		//crt_chrom = &c;
		//crt_rules = &r;
		
		c.changedForMatrixCalculation=false;
	}
	else{
		assert(subgroups_conflicts>=0);
		assert(teachers_conflicts>=0);
		subgroupsConflicts = subgroups_conflicts;
		teachersConflicts = teachers_conflicts;
	}

	int i,dd;

	int unallocated; //unallocated activities
	int late; //late activities
	int nte; //number of teacher exhaustions
	int nse; //number of students exhaustions

	//Part without logging..................................................................
	if(conflictsString==NULL){
		//Unallocated or late activities
		unallocated=0;
		late=0;
		for(i=0; i<r.nInternalActivities; i++){
			if(c.times[i]==UNALLOCATED_TIME){
				//Firstly, we consider a big clash each unallocated activity.
				//Needs to be very a large constant, bigger than any other broken constraint.
				unallocated += /*r.internalActivitiesList[i].duration * r.internalActivitiesList[i].nSubgroups * */ 10000;
				//(an unallocated activity for a year is more important than an unallocated activity for a subgroup)
			}
			else{
				//Calculates the number of activities that are scheduled too late (in fact we
				//calculate a function that increases as the activity is getting late)
				int h=c.times[i]/r.nDaysPerWeek;
				dd=r.internalActivitiesList[i].duration;
				if(h+dd>r.nHoursPerDay){
					int tmp;
					/*if(r.internalActivitiesList[i].parity==PARITY_WEEKLY)
						tmp=2;
					else{
						assert(r.internalActivitiesList[i].parity==PARITY_FORTNIGHTLY);*/
						tmp=1;
					//}
					late += (h+dd-r.nHoursPerDay) * tmp * r.internalActivitiesList[i].iSubgroupsList.count();
					//multiplied with 2 for weekly activities and with the number
					//of subgroups implied, for seeing the importance of the
					//activity
				}
			}
		}
		
		assert(late==0);

		//Below, for teachers and students, please remember that 2 means a weekly activity
		//and 1 fortnightly one. So, if the matrix teachersMatrix[teacher][day][hour]==2, it is ok.

		//Calculates the number of teachers exhaustion (when he has to teach more than
		//one activity at the same time)
		/*nte=0;
		for(i=0; i<r.nInternalTeachers; i++)
			for(int j=0; j<r.nDaysPerWeek; j++)
				for(int k=0; k<r.nHoursPerDay; k++){
					int tmp=teachersMatrix[i][j][k]-2;
					if(tmp>0)
						nte+=tmp;
				}*/
		nte = teachersConflicts; //faster
		
		assert(nte==0);

		//Calculates the number of subgroups exhaustion (a subgroup cannot attend two
		//activities at the same time)
		/*nse=0;
		for(i=0; i<r.nInternalSubgroups; i++)
			for(int j=0; j<r.nDaysPerWeek; j++)
				for(int k=0; k<r.nHoursPerDay; k++){
					int tmp=subgroupsMatrix[i][j][k]-2;
					if(tmp>0)
						nse += tmp;
				}*/
		nse = subgroupsConflicts; //faster
		
		assert(nse==0);			
	}
	//part with logging....................................................................
	else{
		//Unallocated or late activities
		unallocated=0;
		late=0;
		for(i=0; i<r.nInternalActivities; i++){
			if(c.times[i]==UNALLOCATED_TIME){
				//Firstly, we consider a big clash each unallocated activity.
				//Needs to be very a large constant, bigger than any other broken constraint.
				unallocated += /*r.internalActivitiesList[i].duration * r.internalActivitiesList[i].nSubgroups * */ 10000;
				//(an unallocated activity for a year is more important than an unallocated activity for a subgroup)
				if(conflictsString!=NULL){
					QString s= tr("Time constraint basic compulsory broken: unallocated activity with id=%1 (%2)",
						"%2 is the detailed description of activity - teachers, subject, students")
						.arg(r.internalActivitiesList[i].id).arg(getActivityDetailedDescription(r, r.internalActivitiesList[i].id));
					s+=" - ";
					s += tr("this increases the conflicts total by %1")
					 .arg(weightPercentage/100 * /*r.internalActivitiesList[i].duration*r.internalActivitiesList[i].nSubgroups * */10000);
					//s += "\n";
					
					dl.append(s);
					cl.append(weightPercentage/100 * 10000);

					(*conflictsString) += s + "\n";
				}
			}
			else{
				//Calculates the number of activities that are scheduled too late (in fact we
				//calculate a function that increases as the activity is getting late)
				int h=c.times[i]/r.nDaysPerWeek;
				dd=r.internalActivitiesList[i].duration;
				if(h+dd>r.nHoursPerDay){
					assert(0);	
				
					int tmp;
					/*if(r.internalActivitiesList[i].parity==PARITY_WEEKLY)
						tmp=2;
					else{
						assert(r.internalActivitiesList[i].parity==PARITY_FORTNIGHTLY);*/
						tmp=1;
					//}
					late += (h+dd-r.nHoursPerDay) * tmp * r.internalActivitiesList[i].iSubgroupsList.count();
					//multiplied with 2 for weekly activities and with the number
					//of subgroups implied, for seeing the importance of the
					//activity

					if(conflictsString!=NULL){
						QString s=tr("Time constraint basic compulsory");
						s+=": ";
						s+=tr("activity with id=%1 is late.")
						 .arg(r.internalActivitiesList[i].id);
						s+=" ";
						s+=tr("This increases the conflicts total by %1")
						 .arg((h+dd-r.nHoursPerDay)*tmp*r.internalActivitiesList[i].iSubgroupsList.count()*weightPercentage/100);
						s+="\n";
						
						dl.append(s);
						cl.append((h+dd-r.nHoursPerDay)*tmp*r.internalActivitiesList[i].iSubgroupsList.count()*weightPercentage/100);

						(*conflictsString) += s+"\n";
					}
				}
			}
		}

		//Below, for teachers and students, please remember that 2 means a weekly activity
		//and 1 fortnightly one. So, if the matrix teachersMatrix[teacher][day][hour]==2,
		//that is ok.

		//Calculates the number of teachers exhaustion (when he has to teach more than
		//one activity at the same time)
		nte=0;
		for(i=0; i<r.nInternalTeachers; i++)
			for(int j=0; j<r.nDaysPerWeek; j++)
				for(int k=0; k<r.nHoursPerDay; k++){
					int tmp=teachersMatrix[i][j][k]-1;
					if(tmp>0){
						if(conflictsString!=NULL){
							QString s=tr("Time constraint basic compulsory");
							s+=": ";
							s+=tr("teacher with name %1 has more than one allocated activity on day %2, hour %3")
							 .arg(r.internalTeachersList[i]->name)
							 .arg(r.daysOfTheWeek[j])
							 .arg(r.hoursOfTheDay[k]);
							s+=". ";
							s+=tr("This increases the conflicts total by %1")
							 .arg(tmp*weightPercentage/100);
						
							(*conflictsString)+= s+"\n";
							
							dl.append(s);
							cl.append(tmp*weightPercentage/100);
						}
						nte+=tmp;
					}
				}

		assert(nte==0);
		
		//Calculates the number of subgroups exhaustion (a subgroup cannot attend two
		//activities at the same time)
		nse=0;
		for(i=0; i<r.nInternalSubgroups; i++)
			for(int j=0; j<r.nDaysPerWeek; j++)
				for(int k=0; k<r.nHoursPerDay; k++){
					int tmp=subgroupsMatrix[i][j][k]-1;
					if(tmp>0){
						if(conflictsString!=NULL){
							QString s=tr("Time constraint basic compulsory");
							s+=": ";
							s+=tr("subgroup %1 has more than one allocated activity on day %2, hour %3")
							 .arg(r.internalSubgroupsList[i]->name)
							 .arg(r.daysOfTheWeek[j])
							 .arg(r.hoursOfTheDay[k]);
							s+=". ";
							s+=tr("This increases the conflicts total by %1")
							 .arg((subgroupsMatrix[i][j][k]-1)*weightPercentage/100);
							 
							dl.append(s);
							cl.append((subgroupsMatrix[i][j][k]-1)*weightPercentage/100);
						
							*conflictsString += s+"\n";
						}
						nse += tmp;
					}
				}
			
		assert(nse==0);
	}

	/*if(nte!=teachersConflicts){
		cout<<"nte=="<<nte<<", teachersConflicts=="<<teachersConflicts<<endl;
		cout<<c.getTeachersMatrix(r, teachersMatrix)<<endl;
	}
	if(nse!=subgroupsConflicts){
		cout<<"nse=="<<nse<<", subgroupsConflicts=="<<subgroupsConflicts<<endl;
		cout<<c.getSubgroupsMatrix(r, subgroupsMatrix)<<endl;
	}*/
	
	/*assert(nte==teachersConflicts); //just a check, works only on logged fitness calculation
	assert(nse==subgroupsConflicts);*/

	//return int (ceil ( weight * (unallocated + late + nte + nse) ) ); //conflicts factor
	return weightPercentage/100 * (unallocated + late + nte + nse); //conflicts factor
}

bool ConstraintBasicCompulsoryTime::isRelatedToActivity(Rules& r, Activity* a)
{
	Q_UNUSED(a);
	Q_UNUSED(r);
	//if(a)
	//	;

	return false;
}

bool ConstraintBasicCompulsoryTime::isRelatedToTeacher(Teacher* t)
{
	Q_UNUSED(t);

	//if(t)
	//	;

	return false;
}

bool ConstraintBasicCompulsoryTime::isRelatedToSubject(Subject* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintBasicCompulsoryTime::isRelatedToActivityTag(ActivityTag* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintBasicCompulsoryTime::isRelatedToStudentsSet(Rules& r, StudentsSet* s)
{
	Q_UNUSED(r);
	Q_UNUSED(s);
	/*if(s)
		;
	if(&r)
		;*/

	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

ConstraintTeacherNotAvailableTimes::ConstraintTeacherNotAvailableTimes()
	: TimeConstraint()
{
	this->type=CONSTRAINT_TEACHER_NOT_AVAILABLE_TIMES;
}

ConstraintTeacherNotAvailableTimes::ConstraintTeacherNotAvailableTimes(double wp, const QString& tn, QList<int> d, QList<int> h)
	: TimeConstraint(wp)
{
	this->teacher=tn;
	assert(d.count()==h.count());
	this->days=d;
	this->hours=h;
	this->type=CONSTRAINT_TEACHER_NOT_AVAILABLE_TIMES;
}

QString ConstraintTeacherNotAvailableTimes::getXmlDescription(Rules& r){
	QString s="<ConstraintTeacherNotAvailableTimes>\n";
	s+="	<Weight_Percentage>"+QString::number(weightPercentage)+"</Weight_Percentage>\n";
	s+="	<Teacher>"+protect(this->teacher)+"</Teacher>\n";

	s+="	<Number_of_Not_Available_Times>"+QString::number(this->days.count())+"</Number_of_Not_Available_Times>\n";
	assert(days.count()==hours.count());
	for(int i=0; i<days.count(); i++){
		s+="	<Not_Available_Time>\n";
		if(this->days.at(i)>=0)
			s+="		<Day>"+protect(r.daysOfTheWeek[this->days.at(i)])+"</Day>\n";
		if(this->hours.at(i)>=0)
			s+="		<Hour>"+protect(r.hoursOfTheDay[this->hours.at(i)])+"</Hour>\n";
		s+="	</Not_Available_Time>\n";
	}

	s+="</ConstraintTeacherNotAvailableTimes>\n";
	return s;
}

QString ConstraintTeacherNotAvailableTimes::getDescription(Rules& r){
	QString s=tr("Teacher not available");s+=", ";
	s+=tr("WP:%1\%", "Weight percentage").arg(this->weightPercentage);s+=", ";
	s+=tr("T:%1", "Teacher").arg(this->teacher);s+=", ";

	s+=tr("NA at:", "Not available at");
	s+=" ";
	assert(days.count()==hours.count());
	for(int i=0; i<days.count(); i++){
		if(this->days.at(i)>=0){
			s+=r.daysOfTheWeek[this->days.at(i)];
			s+=" ";
		}
		if(this->hours.at(i)>=0){
			s+=r.hoursOfTheDay[this->hours.at(i)];
		}
		if(i<days.count()-1)
			s+="; ";
	}

	return s;
}

QString ConstraintTeacherNotAvailableTimes::getDetailedDescription(Rules& r){
	QString s=tr("Time constraint");s+="\n";
	s+=tr("A teacher is not available");s+="\n";
	s+=tr("Weight (percentage)=%1\%").arg(this->weightPercentage);s+="\n";
	s+=tr("Teacher=%1").arg(this->teacher);s+="\n";

	s+=tr("Not available at:");
	s+="\n";
	assert(days.count()==hours.count());
	for(int i=0; i<days.count(); i++){
		if(this->days.at(i)>=0){
			s+=r.daysOfTheWeek[this->days.at(i)];
			s+=" ";
		}
		if(this->hours.at(i)>=0){
			s+=r.hoursOfTheDay[this->hours.at(i)];
		}
		if(i<days.count()-1)
			s+="; ";
	}
	s+="\n";

	return s;
}

bool ConstraintTeacherNotAvailableTimes::computeInternalStructure(Rules& r){
	this->teacher_ID=r.searchTeacher(this->teacher);

	if(this->teacher_ID<0){
		QMessageBox::warning(NULL, tr("FET warning"),
		 tr("Constraint teacher not available times is wrong because it refers to inexistent teacher."
		 " Please correct it (removing it might be a solution). Please report potential bug. Constraint is:\n%1").arg(this->getDetailedDescription(r)));
		 
		return false;
	}	

	assert(days.count()==hours.count());
	for(int k=0; k<days.count(); k++){
		if(this->days.at(k) >= r.nDaysPerWeek){
			QMessageBox::information(NULL, tr("FET information"),
			 tr("Constraint teacher not available times is wrong because it refers to removed day. Please correct"
			 " and try again. Correcting means editing the constraint and updating information. Constraint is:\n%1").arg(this->getDetailedDescription(r)));
		 
			return false;
		}		
		if(this->hours.at(k) >= r.nHoursPerDay){
			QMessageBox::information(NULL, tr("FET information"),
			 tr("Constraint teacher not available times is wrong because an hour is too late (after the last acceptable slot). Please correct"
			 " and try again. Correcting means editing the constraint and updating information. Constraint is:\n%1").arg(this->getDetailedDescription(r)));
		 
			return false;
		}
	}

	assert(this->teacher_ID>=0);
	return true;
}

bool ConstraintTeacherNotAvailableTimes::hasInactiveActivities(Rules& r)
{
	Q_UNUSED(r);
	return false;
}

double ConstraintTeacherNotAvailableTimes::fitness(Solution& c, Rules& r, QList<double>& cl, QList<QString>&dl, QString *conflictsString)
{
	//if the matrices subgroupsMatrix and teachersMatrix are already calculated, do not calculate them again!
	if(!c.teachersMatrixReady || !c.subgroupsMatrixReady){
		c.teachersMatrixReady=true;
		c.subgroupsMatrixReady=true;

		subgroups_conflicts = c.getSubgroupsMatrix(r, subgroupsMatrix);
		teachers_conflicts = c.getTeachersMatrix(r, teachersMatrix);

		//crt_chrom=&c;
		//crt_rules=&r;
		
		c.changedForMatrixCalculation=false;
	}

	//Calculates the number of hours when the teacher is supposed to be teaching, but he is not available
	//This function consideres all the hours, I mean if there are for example 5 weekly courses
	//scheduled on that hour (which is already a broken compulsory restriction - we only
	//are allowed 1 weekly course for a certain teacher at a certain hour) we calculate
	//5 broken restrictions for that function.
	//TODO: decide if it is better to consider only 2 or 10 as a return value in this particular case
	//(currently it is 10)
	int tch=this->teacher_ID;

	int nbroken;

	nbroken=0;

	assert(days.count()==hours.count());
	for(int k=0; k<days.count(); k++){
		int d=days.at(k);
		int h=hours.at(k);
		
		if(teachersMatrix[tch][d][h]>0){
			nbroken+=teachersMatrix[tch][d][h];
	
			if(conflictsString!=NULL){
				QString s= tr("Time constraint teacher not available");
				s += " ";
				s += tr("broken for teacher: %1 on day %2, hour %3")
				 .arg(r.internalTeachersList[tch]->name)
				 .arg(r.daysOfTheWeek[d])
				 .arg(r.hoursOfTheDay[h]);
				s += ". ";
				s += tr("This increases the conflicts total by %1")
				 .arg(teachersMatrix[tch][d][h]*weightPercentage/100);
				 
				dl.append(s);
				cl.append(teachersMatrix[tch][d][h]*weightPercentage/100);
			
				*conflictsString += s+"\n";
			}
		}
	}

	if(weightPercentage==100.0)
		assert(nbroken==0);
	return weightPercentage/100 * nbroken;
}

bool ConstraintTeacherNotAvailableTimes::isRelatedToActivity(Rules& r, Activity* a)
{
	Q_UNUSED(a);
	Q_UNUSED(r);

	return false;
}

bool ConstraintTeacherNotAvailableTimes::isRelatedToTeacher(Teacher* t)
{
	if(this->teacher==t->name)
		return true;
	return false;
}

bool ConstraintTeacherNotAvailableTimes::isRelatedToSubject(Subject* s)
{
	Q_UNUSED(s);

	return false;
}

bool ConstraintTeacherNotAvailableTimes::isRelatedToActivityTag(ActivityTag* s)
{
	Q_UNUSED(s);

	return false;
}

bool ConstraintTeacherNotAvailableTimes::isRelatedToStudentsSet(Rules& r, StudentsSet* s)
{
	Q_UNUSED(r);
	Q_UNUSED(s);

	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

ConstraintStudentsSetNotAvailableTimes::ConstraintStudentsSetNotAvailableTimes()
	: TimeConstraint()
{
	this->type=CONSTRAINT_STUDENTS_SET_NOT_AVAILABLE_TIMES;
}

ConstraintStudentsSetNotAvailableTimes::ConstraintStudentsSetNotAvailableTimes(double wp, const QString& sn, QList<int> d, QList<int> h)
	 : TimeConstraint(wp){
	this->students = sn;
	assert(d.count()==h.count());
	this->days=d;
	this->hours=h;
	this->type=CONSTRAINT_STUDENTS_SET_NOT_AVAILABLE_TIMES;
}

bool ConstraintStudentsSetNotAvailableTimes::computeInternalStructure(Rules& r){
	StudentsSet* ss=r.searchAugmentedStudentsSet(this->students);
	
	if(ss==NULL){
		QMessageBox::warning(NULL, tr("FET warning"),
		 tr("Constraint students set not available is wrong because it refers to inexistent students set."
		 " Please correct it (removing it might be a solution). Please report potential bug. Constraint is:\n%1").arg(this->getDetailedDescription(r)));
		 
		return false;
	}	
	
	assert(days.count()==hours.count());
	for(int k=0; k<days.count(); k++){
		if(this->days.at(k) >= r.nDaysPerWeek){
			QMessageBox::information(NULL, tr("FET information"),
			 tr("Constraint students set not available times is wrong because it refers to removed day. Please correct"
			 " and try again. Correcting means editing the constraint and updating information. Constraint is:\n%1").arg(this->getDetailedDescription(r)));
		 
			return false;
		}		
		if(this->hours.at(k) >= r.nHoursPerDay){
			QMessageBox::information(NULL, tr("FET information"),
			 tr("Constraint students set not available times is wrong because an hour is too late (after the last acceptable slot). Please correct"
			 " and try again. Correcting means editing the constraint and updating information. Constraint is:\n%1").arg(this->getDetailedDescription(r)));
		 
			return false;
		}
	}
	
	assert(ss);

	//this->nSubgroups=0;
	this->iSubgroupsList.clear();
	if(ss->type==STUDENTS_SUBGROUP){
		int tmp;
		tmp=((StudentsSubgroup*)ss)->indexInInternalSubgroupsList;
		/*for(tmp=0; tmp<=r.nInternalSubgroups; tmp++)
			if(r.internalSubgroupsList[tmp]->name == ss->name)
				break;*/
		assert(tmp>=0);
		assert(tmp<r.nInternalSubgroups);
		//assert(this->nSubgroups<MAX_SUBGROUPS_PER_CONSTRAINT);
		//this->subgroups[this->nSubgroups++]=tmp;
		if(!this->iSubgroupsList.contains(tmp))
			this->iSubgroupsList.append(tmp);
	}
	else if(ss->type==STUDENTS_GROUP){
		StudentsGroup* stg=(StudentsGroup*)ss;
		for(int i=0; i<stg->subgroupsList.size(); i++){
			StudentsSubgroup* sts=stg->subgroupsList[i];
			int tmp;
			/*for(tmp=0; tmp<=r.nInternalSubgroups; tmp++)
				if(r.internalSubgroupsList[tmp]->name == sts->name)
					break;*/
			tmp=sts->indexInInternalSubgroupsList;
			assert(tmp>=0);
			assert(tmp<r.nInternalSubgroups);
			//assert(this->nSubgroups<MAX_SUBGROUPS_PER_CONSTRAINT);
			//this->subgroups[this->nSubgroups++]=tmp;
			if(!this->iSubgroupsList.contains(tmp))
				this->iSubgroupsList.append(tmp);
		}
	}
	else if(ss->type==STUDENTS_YEAR){
		StudentsYear* sty=(StudentsYear*)ss;
		for(int i=0; i<sty->groupsList.size(); i++){
			StudentsGroup* stg=sty->groupsList[i];
			for(int j=0; j<stg->subgroupsList.size(); j++){
				StudentsSubgroup* sts=stg->subgroupsList[j];
				int tmp;
				/*for(tmp=0; tmp<=r.nInternalSubgroups; tmp++)
					if(r.internalSubgroupsList[tmp]->name == sts->name)
						break;*/
				tmp=sts->indexInInternalSubgroupsList;
				assert(tmp>=0);
				assert(tmp<r.nInternalSubgroups);
				//assert(this->nSubgroups<MAX_SUBGROUPS_PER_CONSTRAINT);
				//this->subgroups[this->nSubgroups++]=tmp;
				if(!this->iSubgroupsList.contains(tmp))
					this->iSubgroupsList.append(tmp);
			}
		}
	}
	else
		assert(0);
	return true;
}

bool ConstraintStudentsSetNotAvailableTimes::hasInactiveActivities(Rules& r)
{
	Q_UNUSED(r);
	return false;
}

QString ConstraintStudentsSetNotAvailableTimes::getXmlDescription(Rules& r){
	QString s="<ConstraintStudentsSetNotAvailableTimes>\n";
	s+="	<Weight_Percentage>"+QString::number(this->weightPercentage)+"</Weight_Percentage>\n";
	//s+="	<Compulsory>";s+=yesNo(this->compulsory);s+="</Compulsory>\n";
	s+="	<Students>"+protect(this->students)+"</Students>\n";

	s+="	<Number_of_Not_Available_Times>"+QString::number(this->days.count())+"</Number_of_Not_Available_Times>\n";
	assert(days.count()==hours.count());
	for(int i=0; i<days.count(); i++){
		s+="	<Not_Available_Time>\n";
		if(this->days.at(i)>=0)
			s+="		<Day>"+protect(r.daysOfTheWeek[this->days.at(i)])+"</Day>\n";
		if(this->hours.at(i)>=0)
			s+="		<Hour>"+protect(r.hoursOfTheDay[this->hours.at(i)])+"</Hour>\n";
		s+="	</Not_Available_Time>\n";
	}

	s+="</ConstraintStudentsSetNotAvailableTimes>\n";
	return s;
}

QString ConstraintStudentsSetNotAvailableTimes::getDescription(Rules& r){
	QString s;
	s=tr("Students set not available");s+=", ";
	s+=tr("WP:%1\%", "Weight percentage").arg(this->weightPercentage);s+=", ";
	s+=tr("St:%1", "Students").arg(this->students);s+=", ";

	s+=tr("NA at:", "Not available at");
	s+=" ";
	assert(days.count()==hours.count());
	for(int i=0; i<days.count(); i++){
		if(this->days.at(i)>=0){
			s+=r.daysOfTheWeek[this->days.at(i)];
			s+=" ";
		}
		if(this->hours.at(i)>=0){
			s+=r.hoursOfTheDay[this->hours.at(i)];
		}
		if(i<days.count()-1)
			s+="; ";
	}

	return s;
}

QString ConstraintStudentsSetNotAvailableTimes::getDetailedDescription(Rules& r){
	QString s=tr("Time constraint");s+="\n";
	s+=tr("A students set is not available");s+="\n";
	s+=tr("Weight (percentage)=%1\%").arg(this->weightPercentage);s+="\n";

	s+=tr("Students=%1").arg(this->students);s+="\n";

	s+=tr("Not available at:");s+="\n";
	
	assert(days.count()==hours.count());
	for(int i=0; i<days.count(); i++){
		if(this->days.at(i)>=0){
			s+=r.daysOfTheWeek[this->days.at(i)];
			s+=" ";
		}
		if(this->hours.at(i)>=0){
			s+=r.hoursOfTheDay[this->hours.at(i)];
		}
		if(i<days.count()-1)
			s+="; ";
	}
	s+="\n";

	return s;
}

double ConstraintStudentsSetNotAvailableTimes::fitness(Solution& c, Rules& r, QList<double>& cl, QList<QString>&dl, QString *conflictsString)
{
	//if the matrices subgroupsMatrix and teachersMatrix are already calculated, do not calculate them again!
	if(!c.teachersMatrixReady || !c.subgroupsMatrixReady){
		c.teachersMatrixReady=true;
		c.subgroupsMatrixReady=true;
	//if(crt_chrom!=&c || crt_rules!=&r || subgroups_conflicts<0 || teachers_conflicts<0 || c.changedForMatrixCalculation){
		subgroups_conflicts = c.getSubgroupsMatrix(r, subgroupsMatrix);
		teachers_conflicts = c.getTeachersMatrix(r, teachersMatrix);

		//crt_chrom=&c;
		//crt_rules=&r;
		
		c.changedForMatrixCalculation=false;
	}

	int nbroken;

	nbroken=0;
	for(int m=0; m<this->iSubgroupsList.count(); m++){
		int sbg=this->iSubgroupsList.at(m);
		
		assert(days.count()==hours.count());
		for(int k=0; k<days.count(); k++){
			int d=days.at(k);
			int h=hours.at(k);
			
			if(subgroupsMatrix[sbg][d][h]>0){
				nbroken+=subgroupsMatrix[sbg][d][h];

				if(conflictsString!=NULL){
					QString s= tr("Time constraint students set not available");
					s += " ";
					s += tr("broken for subgroup: %1 on day %2, hour %3")
					 .arg(r.internalSubgroupsList[sbg]->name)
					 .arg(r.daysOfTheWeek[d])
					 .arg(r.hoursOfTheDay[h]);
					s += ". ";
					s += tr("This increases the conflicts total by %1")
					 .arg(subgroupsMatrix[sbg][d][h]*weightPercentage/100);
					 
					dl.append(s);
					cl.append(subgroupsMatrix[sbg][d][h]*weightPercentage/100);
				
					*conflictsString += s+"\n";
				}
			}
		}
	}

	if(weightPercentage==100.0)
		assert(nbroken==0);
	return weightPercentage/100 * nbroken;
}

bool ConstraintStudentsSetNotAvailableTimes::isRelatedToActivity(Rules& r, Activity* a)
{
	Q_UNUSED(a);
	Q_UNUSED(r);
	//if(a)
	//	;

	return false;
}

bool ConstraintStudentsSetNotAvailableTimes::isRelatedToTeacher(Teacher* t)
{
	Q_UNUSED(t);
	/*if(t)
		;*/

	return false;
}

bool ConstraintStudentsSetNotAvailableTimes::isRelatedToSubject(Subject* s)
{
	Q_UNUSED(s);
	/*if(s)
		;*/

	return false;
}

bool ConstraintStudentsSetNotAvailableTimes::isRelatedToActivityTag(ActivityTag* s)
{
	Q_UNUSED(s);
	/*if(s)
		;*/

	return false;
}

bool ConstraintStudentsSetNotAvailableTimes::isRelatedToStudentsSet(Rules& r, StudentsSet* s)
{
	return r.setsShareStudents(this->students, s->name);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////

ConstraintActivitiesSameStartingTime::ConstraintActivitiesSameStartingTime()
	: TimeConstraint()
{
	type=CONSTRAINT_ACTIVITIES_SAME_STARTING_TIME;
}

ConstraintActivitiesSameStartingTime::ConstraintActivitiesSameStartingTime(double wp, int nact, const QList<int>& act)
 : TimeConstraint(wp)
 {
	assert(nact>=2);
	assert(act.count()==nact);
	this->n_activities=nact;
	this->activitiesId.clear();
	for(int i=0; i<nact; i++)
		this->activitiesId.append(act.at(i));

	this->type=CONSTRAINT_ACTIVITIES_SAME_STARTING_TIME;
}

bool ConstraintActivitiesSameStartingTime::computeInternalStructure(Rules &r)
{
	//compute the indices of the activities,
	//based on their unique ID

	assert(this->n_activities==this->activitiesId.count());

	this->_activities.clear();
	for(int i=0; i<this->n_activities; i++){
		int j;
		Activity* act;
		for(j=0; j<r.nInternalActivities; j++){
			act=&r.internalActivitiesList[j];
			if(act->id==this->activitiesId[i]){
				this->_activities.append(j);
				break;
			}
		}
	}
	this->_n_activities=this->_activities.count();
	
	if(this->_n_activities<=1){
		QMessageBox::warning(NULL, tr("FET error in data"), 
			tr("Following constraint is wrong (because you need 2 or more activities. Please correct it):\n%1").arg(this->getDetailedDescription(r)));
		//assert(0);
		return false;
	}

	return true;
}

void ConstraintActivitiesSameStartingTime::removeUseless(Rules& r)
{
	//remove the activitiesId which no longer exist (used after the deletion of an activity)
	
	assert(this->n_activities==this->activitiesId.count());

	QList<int> tmpList;

	for(int i=0; i<this->n_activities; i++){
		for(int k=0; k<r.activitiesList.size(); k++){
			Activity* act=r.activitiesList[k];
			if(act->id==this->activitiesId[i]){
				tmpList.append(act->id);
				break;
			}
		}
	}
	
	this->activitiesId=tmpList;
	this->n_activities=this->activitiesId.count();

	r.internalStructureComputed=false;
}

bool ConstraintActivitiesSameStartingTime::hasInactiveActivities(Rules& r)
{
	int count=0;

	for(int i=0; i<this->n_activities; i++)
		if(r.inactiveActivities.contains(this->activitiesId[i]))
			count++;

	if(this->n_activities-count<=1)
		return true;
	else
		return false;
}

QString ConstraintActivitiesSameStartingTime::getXmlDescription(Rules& r){
	Q_UNUSED(r);
	/*if(&r!=NULL)
		;*/

	QString s="<ConstraintActivitiesSameStartingTime>\n";
	s+="	<Weight_Percentage>"+QString::number(this->weightPercentage)+"</Weight_Percentage>\n";
	//s+="	<Compulsory>";s+=yesNo(this->compulsory);s+="</Compulsory>\n";
	s+="	<Number_of_Activities>"+QString::number(this->n_activities)+"</Number_of_Activities>\n";
	for(int i=0; i<this->n_activities; i++)
		s+="	<Activity_Id>"+QString::number(this->activitiesId[i])+"</Activity_Id>\n";
	s+="</ConstraintActivitiesSameStartingTime>\n";
	return s;
}

QString ConstraintActivitiesSameStartingTime::getDescription(Rules& r){
	Q_UNUSED(r);
	/*if(&r!=NULL)
		;*/

	QString s;
	s+=tr("Activities same starting time");s+=", ";
	s+=tr("WP:%1\%", "Weight percentage").arg(this->weightPercentage);s+=", ";
	s+=tr("NA:%1", "Number of activities").arg(this->n_activities);s+=", ";
	for(int i=0; i<this->n_activities; i++){
		s+=tr("Id:%1", "Id of activity").arg(this->activitiesId[i]);
		if(i<this->n_activities-1)
			s+=", ";
	}

	return s;
}

QString ConstraintActivitiesSameStartingTime::getDetailedDescription(Rules& r){
	QString s;
	
	s=tr("Time constraint");s+="\n";
	s+=tr("Activities must have the same starting time");s+="\n";
	s+=tr("Weight (percentage)=%1\%").arg(this->weightPercentage);s+="\n";
	s+=tr("Number of activities=%1").arg(this->n_activities);s+="\n";
	for(int i=0; i<this->n_activities; i++){
		s+=tr("Activity with id=%1 (%2)", "%1 is the id, %2 is the detailed description of the activity").arg(this->activitiesId[i]).arg(getActivityDetailedDescription(r, this->activitiesId[i]));
		s+="\n";
	}

	return s;
}

double ConstraintActivitiesSameStartingTime::fitness(Solution& c, Rules& r, QList<double>& cl, QList<QString>&dl, QString* conflictsString)
{
	assert(r.internalStructureComputed);

	int nbroken;

	//We do not use the matrices 'subgroupsMatrix' nor 'teachersMatrix'.

	//sum the differences in the scheduled time for all pairs of activities.

	//without logging
	if(conflictsString==NULL){
		nbroken=0;
		for(int i=1; i<this->_n_activities; i++){
			int t1=c.times[this->_activities[i]];
			if(t1!=UNALLOCATED_TIME){
				int day1=t1%r.nDaysPerWeek;
				int hour1=t1/r.nDaysPerWeek;
				for(int j=0; j<i; j++){
					int t2=c.times[this->_activities[j]];
					if(t2!=UNALLOCATED_TIME){
						int day2=t2%r.nDaysPerWeek;
						int hour2=t2/r.nDaysPerWeek;
						int tmp=0;

						//activity weekly - counts as double
						/*if(r.internalActivitiesList[this->_activities[i]].parity==PARITY_WEEKLY &&
						 r.internalActivitiesList[this->_activities[j]].parity==PARITY_WEEKLY)
							tmp = 4 * (abs(day1-day2) + abs(hour1-hour2));
						else if(r.internalActivitiesList[this->_activities[i]].parity==PARITY_WEEKLY ||
						 r.internalActivitiesList[this->_activities[j]].parity==PARITY_WEEKLY)
							tmp = 2 * (abs(day1-day2) + abs(hour1-hour2));
						else*/
							tmp = abs(day1-day2) + abs(hour1-hour2);
							
						if(tmp>0)
							tmp=1;

						nbroken+=tmp;
					}
				}
			}
		}
	}
	//with logging
	else{
		nbroken=0;
		for(int i=1; i<this->_n_activities; i++){
			int t1=c.times[this->_activities[i]];
			if(t1!=UNALLOCATED_TIME){
				int day1=t1%r.nDaysPerWeek;
				int hour1=t1/r.nDaysPerWeek;
				for(int j=0; j<i; j++){
					int t2=c.times[this->_activities[j]];
					if(t2!=UNALLOCATED_TIME){
						int day2=t2%r.nDaysPerWeek;
						int hour2=t2/r.nDaysPerWeek;
						int tmp=0;

						//activity weekly - counts as double
						/*if(r.internalActivitiesList[this->_activities[i]].parity==PARITY_WEEKLY &&
						 r.internalActivitiesList[this->_activities[j]].parity==PARITY_WEEKLY)
							tmp = 4 * (abs(day1-day2) + abs(hour1-hour2));
						else if(r.internalActivitiesList[this->_activities[i]].parity==PARITY_WEEKLY ||
						 r.internalActivitiesList[this->_activities[j]].parity==PARITY_WEEKLY)
							tmp = 2 * (abs(day1-day2) + abs(hour1-hour2));
						else*/
							tmp = abs(day1-day2) + abs(hour1-hour2);
							
						if(tmp>0)
							tmp=1;

						nbroken+=tmp;

						if(tmp>0 && conflictsString!=NULL){
							QString s=tr("Time constraint activities same starting time broken, because activity with id=%1 (%2) is not at the same starting time with activity with id=%3 (%4)",
							"%1 is the id, %2 is the detailed description of the activity, %3 id, %4 det. descr.")
							 .arg(this->activitiesId[i])
							 .arg(getActivityDetailedDescription(r, this->activitiesId[i]))
							 .arg(this->activitiesId[j])
							 .arg(getActivityDetailedDescription(r, this->activitiesId[j]));
							s+=". ";
							s+=tr("Conflicts factor increase=%1").arg(tmp*weightPercentage/100);
						
							dl.append(s);
							cl.append(tmp*weightPercentage/100);
							
							*conflictsString+= s+"\n";
						}
					}
				}
			}
		}
	}

	if(weightPercentage==100)
		assert(nbroken==0);
	return weightPercentage/100 * nbroken;
}

bool ConstraintActivitiesSameStartingTime::isRelatedToActivity(Rules& r, Activity* a)
{
	Q_UNUSED(r);

	for(int i=0; i<this->n_activities; i++)
		if(this->activitiesId[i]==a->id)
			return true;
	return false;
}

bool ConstraintActivitiesSameStartingTime::isRelatedToTeacher(Teacher* t)
{
	Q_UNUSED(t);
	//if(t)
	//	;

	return false;
}

bool ConstraintActivitiesSameStartingTime::isRelatedToSubject(Subject* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintActivitiesSameStartingTime::isRelatedToActivityTag(ActivityTag* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintActivitiesSameStartingTime::isRelatedToStudentsSet(Rules& r, StudentsSet* s)
{
	Q_UNUSED(r);
	Q_UNUSED(s);
	/*if(s)
		;
	if(&r)
		;*/

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

ConstraintActivitiesNotOverlapping::ConstraintActivitiesNotOverlapping()
	: TimeConstraint()
{
	type=CONSTRAINT_ACTIVITIES_NOT_OVERLAPPING;
}

ConstraintActivitiesNotOverlapping::ConstraintActivitiesNotOverlapping(double wp, int nact, const QList<int>& act)
 : TimeConstraint(wp)
 {
  	assert(nact>=2);
  	assert(act.count()==nact);
	this->n_activities=nact;
	this->activitiesId.clear();
	for(int i=0; i<nact; i++)
		this->activitiesId.append(act.at(i));

	this->type=CONSTRAINT_ACTIVITIES_NOT_OVERLAPPING;
}

bool ConstraintActivitiesNotOverlapping::computeInternalStructure(Rules &r)
{
	//compute the indices of the activities,
	//based on their unique ID

	assert(this->n_activities==this->activitiesId.count());

	this->_activities.clear();
	for(int i=0; i<this->n_activities; i++){
		int j;
		Activity* act;
		for(j=0; j<r.nInternalActivities; j++){
			act=&r.internalActivitiesList[j];
			if(act->id==this->activitiesId[i]){
				this->_activities.append(j);
				break;
			}
		}
	}
	this->_n_activities=this->_activities.count();
	
	if(this->_n_activities<=1){
		QMessageBox::warning(NULL, tr("FET error in data"), 
			tr("Following constraint is wrong (because you need 2 or more activities. Please correct it):\n%1").arg(this->getDetailedDescription(r)));
		//assert(0);
		return false;
	}

	return true;
}

void ConstraintActivitiesNotOverlapping::removeUseless(Rules& r)
{
	//remove the activitiesId which no longer exist (used after the deletion of an activity)
	
	assert(this->n_activities==this->activitiesId.count());

	QList<int> tmpList;

	for(int i=0; i<this->n_activities; i++){
		for(int k=0; k<r.activitiesList.size(); k++){
			Activity* act=r.activitiesList[k];
			if(act->id==this->activitiesId[i]){
				tmpList.append(act->id);
				break;
			}
		}
	}
	
	this->activitiesId=tmpList;
	this->n_activities=this->activitiesId.count();

	r.internalStructureComputed=false;
}

bool ConstraintActivitiesNotOverlapping::hasInactiveActivities(Rules& r)
{
	int count=0;

	for(int i=0; i<this->n_activities; i++)
		if(r.inactiveActivities.contains(this->activitiesId[i]))
			count++;

	if(this->n_activities-count<=1)
		return true;
	else
		return false;
}

QString ConstraintActivitiesNotOverlapping::getXmlDescription(Rules& r){
	Q_UNUSED(r);

	QString s="<ConstraintActivitiesNotOverlapping>\n";
	s+="	<Weight_Percentage>"+QString::number(this->weightPercentage)+"</Weight_Percentage>\n";
	s+="	<Number_of_Activities>"+QString::number(this->n_activities)+"</Number_of_Activities>\n";
	for(int i=0; i<this->n_activities; i++)
		s+="	<Activity_Id>"+QString::number(this->activitiesId[i])+"</Activity_Id>\n";
	s+="</ConstraintActivitiesNotOverlapping>\n";
	return s;
}

QString ConstraintActivitiesNotOverlapping::getDescription(Rules& r){
	Q_UNUSED(r);

	QString s;
	s+=tr("Activities not overlapping");s+=", ";
	s+=tr("WP:%1\%", "Weight percentage").arg(this->weightPercentage);s+=", ";
	s+=tr("NA:%1", "Number of activities").arg(this->n_activities);s+=", ";
	for(int i=0; i<this->n_activities; i++){
		s+=tr("Id:%1", "Id of activity").arg(this->activitiesId[i]);
		if(i<this->n_activities-1)
			s+=", ";
	}

	return s;
}

QString ConstraintActivitiesNotOverlapping::getDetailedDescription(Rules& r){
	QString s=tr("Time constraint");s+="\n";
	s+=tr("Activities must not overlap");s+="\n";
	s+=tr("Weight (percentage)=%1\%").arg(this->weightPercentage);s+="\n";
	s+=tr("Number of activities=%1").arg(this->n_activities);s+="\n";
	for(int i=0; i<this->n_activities; i++){
		s+=tr("Activity with id=%1 (%2)", "%1 is the id, %2 is the detailed description of the activity")
			.arg(this->activitiesId[i]).arg(getActivityDetailedDescription(r, this->activitiesId[i]));
		s+="\n";
	}

	return s;
}

double ConstraintActivitiesNotOverlapping::fitness(Solution& c, Rules& r, QList<double>& cl, QList<QString>&dl, QString* conflictsString)
{
	assert(r.internalStructureComputed);

	int nbroken;

	//We do not use the matrices 'subgroupsMatrix' nor 'teachersMatrix'.

	//sum the overlapping hours for all pairs of activities.
	//without logging
	if(conflictsString==NULL){
		nbroken=0;
		for(int i=1; i<this->_n_activities; i++){
			int t1=c.times[this->_activities[i]];
			if(t1!=UNALLOCATED_TIME){
				int day1=t1%r.nDaysPerWeek;
				int hour1=t1/r.nDaysPerWeek;
				int duration1=r.internalActivitiesList[this->_activities[i]].duration;

				for(int j=0; j<i; j++){
					int t2=c.times[this->_activities[j]];
					if(t2!=UNALLOCATED_TIME){
						int day2=t2%r.nDaysPerWeek;
						int hour2=t2/r.nDaysPerWeek;
						int duration2=r.internalActivitiesList[this->_activities[j]].duration;

						//the number of overlapping hours
						int tt=0;
						if(day1==day2){
							int start=max(hour1, hour2);
							int stop=min(hour1+duration1, hour2+duration2);
							if(stop>start)
								tt+=stop-start;
						}
						
						nbroken+=tt;
					}
				}
			}
		}
	}
	//with logging
	else{
		nbroken=0;
		for(int i=1; i<this->_n_activities; i++){
			int t1=c.times[this->_activities[i]];
			if(t1!=UNALLOCATED_TIME){
				int day1=t1%r.nDaysPerWeek;
				int hour1=t1/r.nDaysPerWeek;
				int duration1=r.internalActivitiesList[this->_activities[i]].duration;

				for(int j=0; j<i; j++){
					int t2=c.times[this->_activities[j]];
					if(t2!=UNALLOCATED_TIME){
						int day2=t2%r.nDaysPerWeek;
						int hour2=t2/r.nDaysPerWeek;
						int duration2=r.internalActivitiesList[this->_activities[j]].duration;
	
						//the number of overlapping hours
						int tt=0;
						if(day1==day2){
							int start=max(hour1, hour2);
							int stop=min(hour1+duration1, hour2+duration2);
							if(stop>start)
								tt+=stop-start;
						}

						//The overlapping hours, considering weekly activities more important than fortnightly ones
						int tmp=tt;

						nbroken+=tmp;

						if(tt>0 && conflictsString!=NULL){

							QString s=tr("Time constraint activities not overlapping broken: activity with id=%1 (%2) overlaps with activity with id=%3 (%4) on a number of %5 periods",
							 "%1 is the id, %2 is the detailed description of the activity, %3 id, %4 det. descr.")
							 .arg(this->activitiesId[i])
							 .arg(getActivityDetailedDescription(r, this->activitiesId[i]))
							 .arg(this->activitiesId[j])
							 .arg(getActivityDetailedDescription(r, this->activitiesId[j]))
							 .arg(tt);
							s+=", ";
							s+=tr("conflicts factor increase=%1").arg(tmp*weightPercentage/100);
							
							dl.append(s);
							cl.append(tmp*weightPercentage/100);
						
							*conflictsString+= s+"\n";
						}
					}
				}
			}
		}
	}

	if(weightPercentage==100)
		assert(nbroken==0);
	return weightPercentage/100 * nbroken;
}

bool ConstraintActivitiesNotOverlapping::isRelatedToActivity(Rules& r, Activity* a)
{
	Q_UNUSED(r);

	for(int i=0; i<this->n_activities; i++)
		if(this->activitiesId[i]==a->id)
			return true;
	return false;
}

bool ConstraintActivitiesNotOverlapping::isRelatedToTeacher(Teacher* t)
{
	Q_UNUSED(t);
	//if(t)
	//	;

	return false;
}

bool ConstraintActivitiesNotOverlapping::isRelatedToSubject(Subject* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintActivitiesNotOverlapping::isRelatedToActivityTag(ActivityTag* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintActivitiesNotOverlapping::isRelatedToStudentsSet(Rules& r, StudentsSet* s)
{
	Q_UNUSED(r);
	Q_UNUSED(s);
	/*if(s)
		;
	if(&r)
		;*/

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

ConstraintMinDaysBetweenActivities::ConstraintMinDaysBetweenActivities()
	: TimeConstraint()
{
	type=CONSTRAINT_MIN_DAYS_BETWEEN_ACTIVITIES;
}

ConstraintMinDaysBetweenActivities::ConstraintMinDaysBetweenActivities(double wp, bool cisd, int nact, const QList<int>& act, int n)
 : TimeConstraint(wp)
 {
 	this->consecutiveIfSameDay=cisd;
 
  	//assert(nact>=2 && nact<=MAX_CONSTRAINT_MIN_DAYS_BETWEEN_ACTIVITIES);
  	assert(nact>=2);
  	assert(act.count()==nact);
	this->n_activities=nact;
	this->activitiesId.clear();
	for(int i=0; i<nact; i++)
		this->activitiesId.append(act.at(i));

	assert(n>0);
	this->minDays=n;

	this->type=CONSTRAINT_MIN_DAYS_BETWEEN_ACTIVITIES;
}

bool ConstraintMinDaysBetweenActivities::operator==(ConstraintMinDaysBetweenActivities& c){
	assert(this->n_activities==this->activitiesId.count());
	assert(c.n_activities==c.activitiesId.count());

	if(this->n_activities!=c.n_activities)
		return false;
	for(int i=0; i<this->n_activities; i++)
		if(this->activitiesId[i]!=c.activitiesId[i])
			return false;
	if(this->minDays!=c.minDays)
		return false;
	if(this->weightPercentage!=c.weightPercentage)
		return false;
	if(this->consecutiveIfSameDay!=c.consecutiveIfSameDay)
		return false;
	return true;
}

bool ConstraintMinDaysBetweenActivities::computeInternalStructure(Rules &r)
{
	//compute the indices of the activities,
	//based on their unique ID

	assert(this->n_activities==this->activitiesId.count());

	this->_activities.clear();
	for(int i=0; i<this->n_activities; i++){
		int j;
		Activity* act;
		for(j=0; j<r.nInternalActivities; j++){
			act=&r.internalActivitiesList[j];
			if(act->id==this->activitiesId[i]){
				this->_activities.append(j);
				break;
			}
		}
	}
	this->_n_activities=this->_activities.count();
	
	if(this->_n_activities<=1){
		QMessageBox::warning(NULL, tr("FET error in data"), 
			tr("Following constraint is wrong (because you need 2 or more activities. Please correct it):\n%1").arg(this->getDetailedDescription(r)));
		//assert(0);
		return false;
	}

	return true;
}

void ConstraintMinDaysBetweenActivities::removeUseless(Rules& r)
{
	//remove the activitiesId which no longer exist (used after the deletion of an activity)
	
	assert(this->n_activities==this->activitiesId.count());

	QList<int> tmpList;

	for(int i=0; i<this->n_activities; i++){
		for(int k=0; k<r.activitiesList.size(); k++){
			Activity* act=r.activitiesList[k];
			if(act->id==this->activitiesId[i]){
				tmpList.append(act->id);
				break;
			}
		}
	}
	
	this->activitiesId=tmpList;
	this->n_activities=this->activitiesId.count();

	r.internalStructureComputed=false;
}

bool ConstraintMinDaysBetweenActivities::hasInactiveActivities(Rules& r)
{
	int count=0;

	for(int i=0; i<this->n_activities; i++)
		if(r.inactiveActivities.contains(this->activitiesId[i]))
			count++;

	if(this->n_activities-count<=1)
		return true;
	else
		return false;
}

QString ConstraintMinDaysBetweenActivities::getXmlDescription(Rules& r){
	Q_UNUSED(r);

	QString s="<ConstraintMinDaysBetweenActivities>\n";
	s+="	<Weight_Percentage>"+QString::number(this->weightPercentage)+"</Weight_Percentage>\n";
	//s+="	<Compulsory>";s+=yesNo(this->compulsory);s+="</Compulsory>\n";
//	s+="	<Consecutive_If_Same_Day>";s+=yesNo(this->consecutiveIfSameDay);s+="</Consecutive_If_Same_Day>\n";
	s+="	<Consecutive_If_Same_Day>";s+=trueFalse(this->consecutiveIfSameDay);s+="</Consecutive_If_Same_Day>\n";
	s+="	<Number_of_Activities>"+QString::number(this->n_activities)+"</Number_of_Activities>\n";
	for(int i=0; i<this->n_activities; i++)
		s+="	<Activity_Id>"+QString::number(this->activitiesId[i])+"</Activity_Id>\n";
	s+="	<MinDays>"+QString::number(this->minDays)+"</MinDays>\n";
	s+="</ConstraintMinDaysBetweenActivities>\n";
	return s;
}

QString ConstraintMinDaysBetweenActivities::getDescription(Rules& r){
	Q_UNUSED(r);

	QString s;
	s+=tr("Min days between activities");s+=", ";
	s+=tr("WP:%1\%", "Weight percentage").arg(this->weightPercentage);s+=", ";
	s+=tr("NA:%1", "Number of activities").arg(this->n_activities);s+=", ";
	for(int i=0; i<this->n_activities; i++){
		s+=tr("Id:%1", "Id of activity").arg(this->activitiesId[i]);s+=", ";
	}
	s+=tr("mD:%1", "Min days").arg(this->minDays);s+=", ";
	s+=tr("CSD:%1", "Consecutive if same day").arg(yesNoTranslated(this->consecutiveIfSameDay));

	return s;
}

QString ConstraintMinDaysBetweenActivities::getDetailedDescription(Rules& r){
	QString s=tr("Time constraint");s+="\n";
	s+=tr("Minimum number of days between activities");s+="\n";
	s+=tr("Weight (percentage)=%1\%").arg(this->weightPercentage);s+="\n";
	s+=tr("Number of activities=%1").arg(this->n_activities);s+="\n";
	for(int i=0; i<this->n_activities; i++){
		s+=tr("Activity with id=%1 (%2)", "%1 is the id, %2 is the detailed description of the activity")
			.arg(this->activitiesId[i])
			.arg(getActivityDetailedDescription(r, this->activitiesId[i]));
		s+="\n";
	}
	s+=tr("Minimum number of days=%1").arg(this->minDays);s+="\n";
	s+=tr("Consecutive if same day=%1").arg(yesNoTranslated(this->consecutiveIfSameDay));s+="\n";

	return s;
}

double ConstraintMinDaysBetweenActivities::fitness(Solution& c, Rules& r, QList<double>& cl, QList<QString>&dl, QString* conflictsString)
{
	assert(r.internalStructureComputed);

	int nbroken;

	//We do not use the matrices 'subgroupsMatrix' nor 'teachersMatrix'.

	//sum the overlapping hours for all pairs of activities.
	//without logging
	if(conflictsString==NULL){
		nbroken=0;
		for(int i=1; i<this->_n_activities; i++){
			int t1=c.times[this->_activities[i]];
			if(t1!=UNALLOCATED_TIME){
				int day1=t1%r.nDaysPerWeek;
				int hour1=t1/r.nDaysPerWeek;
				int duration1=r.internalActivitiesList[this->_activities[i]].duration;

				for(int j=0; j<i; j++){
					int t2=c.times[this->_activities[j]];
					if(t2!=UNALLOCATED_TIME){
						int day2=t2%r.nDaysPerWeek;
						int hour2=t2/r.nDaysPerWeek;
						int duration2=r.internalActivitiesList[this->_activities[j]].duration;
					
						int tmp;
						int tt=0;
						int dist=abs(day1-day2);
						if(dist<minDays){
							tt=minDays-dist;
							
							if(this->consecutiveIfSameDay && day1==day2)
								assert( day1==day2 && (hour1+duration1==hour2 || hour2+duration2==hour1) );
						}
						
						tmp=tt;
	
						nbroken+=tmp;
					}
				}
			}
		}
	}
	//with logging
	else{
		nbroken=0;
		for(int i=1; i<this->_n_activities; i++){
			int t1=c.times[this->_activities[i]];
			if(t1!=UNALLOCATED_TIME){
				int day1=t1%r.nDaysPerWeek;
				int hour1=t1/r.nDaysPerWeek;
				int duration1=r.internalActivitiesList[this->_activities[i]].duration;

				for(int j=0; j<i; j++){
					int t2=c.times[this->_activities[j]];
					if(t2!=UNALLOCATED_TIME){
						int day2=t2%r.nDaysPerWeek;
						int hour2=t2/r.nDaysPerWeek;
						int duration2=r.internalActivitiesList[this->_activities[j]].duration;
					
						int tmp;
						int tt=0;
						int dist=abs(day1-day2);

						if(dist<minDays){
							tt=minDays-dist;
							
							if(this->consecutiveIfSameDay && day1==day2)
								assert( day1==day2 && (hour1+duration1==hour2 || hour2+duration2==hour1) );
						}

						tmp=tt;
	
						nbroken+=tmp;

						if(tt>0 && conflictsString!=NULL){
							QString s=tr("Time constraint min days between activities broken: activity with id=%1 (%2) conflicts with activity with id=%3 (%4), being %5 days too close, on days %6 and %7",
							 "%1 is the id, %2 is the detailed description of the activity, %3 id, %4 det. descr. Close here means near")
							 .arg(this->activitiesId[i])
							 .arg(getActivityDetailedDescription(r, this->activitiesId[i]))
							 .arg(this->activitiesId[j])
							 .arg(getActivityDetailedDescription(r, this->activitiesId[j]))
							 .arg(tt)
							 .arg(r.daysOfTheWeek[day1])
							 .arg(r.daysOfTheWeek[day2]);
							 ;

							s+=", ";
							s+=tr("conflicts factor increase=%1").arg(tmp*weightPercentage/100);
							s+=".";
							
							if(this->consecutiveIfSameDay && day1==day2){
								s+=" ";
								s+=tr("The activities are placed consecutively in the timetable, because you selected this option"
								 " in case the activities are in the same day");
							}
							
							dl.append(s);
							cl.append(tmp*weightPercentage/100);
							
							*conflictsString+= s+"\n";
						}
					}
				}
			}
		}
	}

	if(weightPercentage==100)
		assert(nbroken==0);
	return weightPercentage/100 * nbroken;
}

bool ConstraintMinDaysBetweenActivities::isRelatedToActivity(Rules& r, Activity* a)
{
	Q_UNUSED(r);

	for(int i=0; i<this->n_activities; i++)
		if(this->activitiesId[i]==a->id)
			return true;
	return false;
}

bool ConstraintMinDaysBetweenActivities::isRelatedToTeacher(Teacher* t)
{
	Q_UNUSED(t);

	return false;
}

bool ConstraintMinDaysBetweenActivities::isRelatedToSubject(Subject* s)
{
	Q_UNUSED(s);

	return false;
}

bool ConstraintMinDaysBetweenActivities::isRelatedToActivityTag(ActivityTag* s)
{
	Q_UNUSED(s);

	return false;
}

bool ConstraintMinDaysBetweenActivities::isRelatedToStudentsSet(Rules& r, StudentsSet* s)
{
	Q_UNUSED(r);
	Q_UNUSED(s);

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

ConstraintMaxDaysBetweenActivities::ConstraintMaxDaysBetweenActivities()
	: TimeConstraint()
{
	type=CONSTRAINT_MAX_DAYS_BETWEEN_ACTIVITIES;
}

ConstraintMaxDaysBetweenActivities::ConstraintMaxDaysBetweenActivities(double wp, int nact, const QList<int>& act, int n)
 : TimeConstraint(wp)
 {
  	assert(nact>=2);
  	assert(act.count()==nact);
	this->n_activities=nact;
	this->activitiesId.clear();
	for(int i=0; i<nact; i++)
		this->activitiesId.append(act.at(i));

	assert(n>=0);
	this->maxDays=n;

	this->type=CONSTRAINT_MAX_DAYS_BETWEEN_ACTIVITIES;
}

bool ConstraintMaxDaysBetweenActivities::computeInternalStructure(Rules &r)
{
	//compute the indices of the activities,
	//based on their unique ID

	assert(this->n_activities==this->activitiesId.count());

	this->_activities.clear();
	for(int i=0; i<this->n_activities; i++){
		int j;
		Activity* act;
		for(j=0; j<r.nInternalActivities; j++){
			act=&r.internalActivitiesList[j];
			if(act->id==this->activitiesId[i]){
				this->_activities.append(j);
				break;
			}
		}
	}
	this->_n_activities=this->_activities.count();
	
	if(this->_n_activities<=1){
		QMessageBox::warning(NULL, tr("FET error in data"), 
			tr("Following constraint is wrong (because you need 2 or more activities. Please correct it):\n%1").arg(this->getDetailedDescription(r)));
		//assert(0);
		return false;
	}

	return true;
}

void ConstraintMaxDaysBetweenActivities::removeUseless(Rules& r)
{
	//remove the activitiesId which no longer exist (used after the deletion of an activity)
	
	assert(this->n_activities==this->activitiesId.count());

	QList<int> tmpList;

	for(int i=0; i<this->n_activities; i++){
		for(int k=0; k<r.activitiesList.size(); k++){
			Activity* act=r.activitiesList[k];
			if(act->id==this->activitiesId[i]){
				tmpList.append(act->id);
				break;
			}
		}
	}
	
	this->activitiesId=tmpList;
	this->n_activities=this->activitiesId.count();

	r.internalStructureComputed=false;
}

bool ConstraintMaxDaysBetweenActivities::hasInactiveActivities(Rules& r)
{
	int count=0;

	for(int i=0; i<this->n_activities; i++)
		if(r.inactiveActivities.contains(this->activitiesId[i]))
			count++;

	if(this->n_activities-count<=1)
		return true;
	else
		return false;
}

QString ConstraintMaxDaysBetweenActivities::getXmlDescription(Rules& r){
	Q_UNUSED(r);

	QString s="<ConstraintMaxDaysBetweenActivities>\n";
	s+="	<Weight_Percentage>"+QString::number(this->weightPercentage)+"</Weight_Percentage>\n";
	s+="	<Number_of_Activities>"+QString::number(this->n_activities)+"</Number_of_Activities>\n";
	for(int i=0; i<this->n_activities; i++)
		s+="	<Activity_Id>"+QString::number(this->activitiesId[i])+"</Activity_Id>\n";
	s+="	<MaxDays>"+QString::number(this->maxDays)+"</MaxDays>\n";
	s+="</ConstraintMaxDaysBetweenActivities>\n";
	return s;
}

QString ConstraintMaxDaysBetweenActivities::getDescription(Rules& r){
	Q_UNUSED(r);

	QString s;
	s+=tr("Max days between activities");s+=", ";
	s+=tr("WP:%1\%", "Weight percentage").arg(this->weightPercentage);s+=", ";
	s+=tr("NA:%1", "Number of activities").arg(this->n_activities);s+=", ";
	for(int i=0; i<this->n_activities; i++){
		s+=tr("Id:%1", "Id of activity").arg(this->activitiesId[i]);s+=", ";
	}
	s+=tr("MD:%1", "Abbreviation for maximum days").arg(this->maxDays);

	return s;
}

QString ConstraintMaxDaysBetweenActivities::getDetailedDescription(Rules& r){
	QString s=tr("Time constraint");s+="\n";
	s+=tr("Maximum number of days between activities");s+="\n";
	s+=tr("Weight (percentage)=%1\%").arg(this->weightPercentage);s+="\n";
	s+=tr("Number of activities=%1").arg(this->n_activities);s+="\n";
	for(int i=0; i<this->n_activities; i++){
		s+=tr("Activity with id=%1 (%2)", "%1 is the id, %2 is the detailed description of the activity")
			.arg(this->activitiesId[i])
			.arg(getActivityDetailedDescription(r, this->activitiesId[i]));
		s+="\n";
	}
	s+=tr("Maximum number of days=%1").arg(this->maxDays);s+="\n";

	return s;
}

double ConstraintMaxDaysBetweenActivities::fitness(Solution& c, Rules& r, QList<double>& cl, QList<QString>&dl, QString* conflictsString)
{
	assert(r.internalStructureComputed);

	int nbroken;

	//We do not use the matrices 'subgroupsMatrix' nor 'teachersMatrix'.

	//sum the overlapping hours for all pairs of activities.
	//without logging
	if(conflictsString==NULL){
		nbroken=0;
		for(int i=1; i<this->_n_activities; i++){
			int t1=c.times[this->_activities[i]];
			if(t1!=UNALLOCATED_TIME){
				int day1=t1%r.nDaysPerWeek;
				//int hour1=t1/r.nDaysPerWeek;
				//int duration1=r.internalActivitiesList[this->_activities[i]].duration;

				for(int j=0; j<i; j++){
					int t2=c.times[this->_activities[j]];
					if(t2!=UNALLOCATED_TIME){
						int day2=t2%r.nDaysPerWeek;
						//int hour2=t2/r.nDaysPerWeek;
						//int duration2=r.internalActivitiesList[this->_activities[j]].duration;
					
						int tmp;
						int tt=0;
						int dist=abs(day1-day2);
						if(dist>maxDays){
							tt=dist-maxDays;
							
							//if(this->consecutiveIfSameDay && day1==day2)
							//	assert( day1==day2 && (hour1+duration1==hour2 || hour2+duration2==hour1) );
						}
						
						tmp=tt;
	
						nbroken+=tmp;
					}
				}
			}
		}
	}
	//with logging
	else{
		nbroken=0;
		for(int i=1; i<this->_n_activities; i++){
			int t1=c.times[this->_activities[i]];
			if(t1!=UNALLOCATED_TIME){
				int day1=t1%r.nDaysPerWeek;
				//int hour1=t1/r.nDaysPerWeek;
				//int duration1=r.internalActivitiesList[this->_activities[i]].duration;

				for(int j=0; j<i; j++){
					int t2=c.times[this->_activities[j]];
					if(t2!=UNALLOCATED_TIME){
						int day2=t2%r.nDaysPerWeek;
						//int hour2=t2/r.nDaysPerWeek;
						//int duration2=r.internalActivitiesList[this->_activities[j]].duration;
					
						int tmp;
						int tt=0;
						int dist=abs(day1-day2);

						if(dist>maxDays){
							tt=dist-maxDays;
							
							//if(this->consecutiveIfSameDay && day1==day2)
							//	assert( day1==day2 && (hour1+duration1==hour2 || hour2+duration2==hour1) );
						}

						tmp=tt;
	
						nbroken+=tmp;

						if(tt>0 && conflictsString!=NULL){
							QString s=tr("Time constraint max days between activities broken: activity with id=%1 (%2) conflicts with activity with id=%3 (%4), being %5 days too far away"
							 ", on days %6 and %7", "%1 is the id, %2 is the detailed description of the activity, %3 id, %4 det. descr.")
							 .arg(this->activitiesId[i])
							 .arg(getActivityDetailedDescription(r, this->activitiesId[i]))
							 .arg(this->activitiesId[j])
							 .arg(getActivityDetailedDescription(r, this->activitiesId[j]))
							 .arg(tt)
							 .arg(r.daysOfTheWeek[day1])
							 .arg(r.daysOfTheWeek[day2]);
							 
							s+=", ";
							s+=tr("conflicts factor increase=%1").arg(tmp*weightPercentage/100);
							s+=".";
							
							/*if(this->consecutiveIfSameDay && day1==day2){
								s+=" ";
								s+=tr("The activities are placed consecutively in the timetable, because you selected this option"
								 " in case the activities are in the same day");
							}*/
							
							dl.append(s);
							cl.append(tmp*weightPercentage/100);
							
							*conflictsString+= s+"\n";
						}
					}
				}
			}
		}
	}

	if(weightPercentage==100)
		assert(nbroken==0);
	return weightPercentage/100 * nbroken;
}

bool ConstraintMaxDaysBetweenActivities::isRelatedToActivity(Rules& r, Activity* a)
{
	Q_UNUSED(r);

	for(int i=0; i<this->n_activities; i++)
		if(this->activitiesId[i]==a->id)
			return true;
	return false;
}

bool ConstraintMaxDaysBetweenActivities::isRelatedToTeacher(Teacher* t)
{
	Q_UNUSED(t);

	return false;
}

bool ConstraintMaxDaysBetweenActivities::isRelatedToSubject(Subject* s)
{
	Q_UNUSED(s);

	return false;
}

bool ConstraintMaxDaysBetweenActivities::isRelatedToActivityTag(ActivityTag* s)
{
	Q_UNUSED(s);

	return false;
}

bool ConstraintMaxDaysBetweenActivities::isRelatedToStudentsSet(Rules& r, StudentsSet* s)
{
	Q_UNUSED(r);
	Q_UNUSED(s);

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

ConstraintMinGapsBetweenActivities::ConstraintMinGapsBetweenActivities()
	: TimeConstraint()
{
	type=CONSTRAINT_MIN_GAPS_BETWEEN_ACTIVITIES;
}

//ConstraintMinGapsBetweenActivities::ConstraintMinGapsBetweenActivities(double wp, int nact, const int act[], int ngaps)
ConstraintMinGapsBetweenActivities::ConstraintMinGapsBetweenActivities(double wp, int nact, const QList<int>& actList, int ngaps)
 : TimeConstraint(wp)
 {
 	//assert(nact>=2 && nact<=MAX_CONSTRAINT_MIN_GAPS_BETWEEN_ACTIVITIES);
	this->n_activities=nact;
	assert(nact==actList.count());
	this->activitiesId.clear();
	for(int i=0; i<nact; i++)
		this->activitiesId.append(actList.at(i));

	assert(ngaps>0);
	this->minGaps=ngaps;

	this->type=CONSTRAINT_MIN_GAPS_BETWEEN_ACTIVITIES;
}

bool ConstraintMinGapsBetweenActivities::computeInternalStructure(Rules &r)
{
	//compute the indices of the activities,
	//based on their unique ID

	assert(this->n_activities==this->activitiesId.count());

	this->_activities.clear();
	for(int i=0; i<this->n_activities; i++){
		int j;
		Activity* act;
		for(j=0; j<r.nInternalActivities; j++){
			act=&r.internalActivitiesList[j];
			if(act->id==this->activitiesId[i]){
				this->_activities.append(j);
				break;
			}
		}
	}
	this->_n_activities=this->_activities.count();
	
	if(this->_n_activities<=1){
		QMessageBox::warning(NULL, tr("FET error in data"), 
			tr("Following constraint is wrong (because you need 2 or more activities. Please correct it):\n%1").arg(this->getDetailedDescription(r)));
		//assert(0);
		return false;
	}

	return true;
}

void ConstraintMinGapsBetweenActivities::removeUseless(Rules& r)
{
	//remove the activitiesId which no longer exist (used after the deletion of an activity)
	
	assert(this->n_activities==this->activitiesId.count());

	QList<int> tmpList;

	for(int i=0; i<this->n_activities; i++){
		for(int k=0; k<r.activitiesList.size(); k++){
			Activity* act=r.activitiesList[k];
			if(act->id==this->activitiesId[i]){
				tmpList.append(act->id);
				break;
			}
		}
	}
	
	this->activitiesId=tmpList;
	this->n_activities=this->activitiesId.count();

	r.internalStructureComputed=false;
}

bool ConstraintMinGapsBetweenActivities::hasInactiveActivities(Rules& r)
{
	int count=0;

	for(int i=0; i<this->n_activities; i++)
		if(r.inactiveActivities.contains(this->activitiesId[i]))
			count++;

	if(this->n_activities-count<=1)
		return true;
	else
		return false;
}

QString ConstraintMinGapsBetweenActivities::getXmlDescription(Rules& r){
	Q_UNUSED(r);

	QString s="<ConstraintMinGapsBetweenActivities>\n";
	s+="	<Weight_Percentage>"+QString::number(this->weightPercentage)+"</Weight_Percentage>\n";
	//s+="	<Compulsory>";s+=yesNo(this->compulsory);s+="</Compulsory>\n";
	s+="	<Number_of_Activities>"+QString::number(this->n_activities)+"</Number_of_Activities>\n";
	for(int i=0; i<this->n_activities; i++)
		s+="	<Activity_Id>"+QString::number(this->activitiesId[i])+"</Activity_Id>\n";
	s+="	<MinGaps>"+QString::number(this->minGaps)+"</MinGaps>\n";
	s+="</ConstraintMinGapsBetweenActivities>\n";
	return s;
}

QString ConstraintMinGapsBetweenActivities::getDescription(Rules& r){
	Q_UNUSED(r);

	QString s;
	s+=tr("Min gaps between activities");s+=", ";
	s+=tr("WP:%1\%", "Weight percentage").arg(this->weightPercentage);s+=", ";
	s+=tr("NA:%1", "Number of activities").arg(this->n_activities);s+=", ";
	for(int i=0; i<this->n_activities; i++){
		s+=tr("Id:%1", "Id of activity").arg(this->activitiesId[i]);s+=", ";
	}
	s+=tr("mG:%1", "Minimum number of gaps").arg(this->minGaps);

	return s;
}

QString ConstraintMinGapsBetweenActivities::getDetailedDescription(Rules& r){
	QString s=tr("Time constraint");s+="\n";
	s+=tr("Minimum gaps between activities (if activities on the same day)");s+="\n";
	s+=tr("Weight (percentage)=%1\%").arg(this->weightPercentage);s+="\n";
	s+=tr("Number of activities=%1").arg(this->n_activities);s+="\n";
	for(int i=0; i<this->n_activities; i++){
		s+=tr("Activity with id=%1 (%2)", "%1 is the id, %2 is the detailed description of the activity")
			.arg(this->activitiesId[i])
			.arg(getActivityDetailedDescription(r, this->activitiesId[i]));
		s+="\n";
	}
	s+=tr("Minimum number of gaps=%1").arg(this->minGaps);s+="\n";

	return s;
}

double ConstraintMinGapsBetweenActivities::fitness(Solution& c, Rules& r, QList<double>& cl, QList<QString>&dl, QString* conflictsString)
{
	assert(r.internalStructureComputed);

	int nbroken;

	//We do not use the matrices 'subgroupsMatrix' nor 'teachersMatrix'.

	nbroken=0;
	for(int i=1; i<this->_n_activities; i++){
		int t1=c.times[this->_activities[i]];
		if(t1!=UNALLOCATED_TIME){
			int day1=t1%r.nDaysPerWeek;
			int hour1=t1/r.nDaysPerWeek;
			int duration1=r.internalActivitiesList[this->_activities[i]].duration;

			for(int j=0; j<i; j++){
				int t2=c.times[this->_activities[j]];
				if(t2!=UNALLOCATED_TIME){
					int day2=t2%r.nDaysPerWeek;
					int hour2=t2/r.nDaysPerWeek;
					int duration2=r.internalActivitiesList[this->_activities[j]].duration;
				
					int tmp;
					int tt=0;
					int dist=abs(day1-day2);
					
					if(dist==0){ //same day
						assert(day1==day2);
						if(hour2>=hour1){
							//assert(hour1+duration1<=hour2); not true for activities which are not incompatible
							if(hour1+duration1+minGaps > hour2)
								tt = (hour1+duration1+minGaps) - hour2;
						}
						else{
							//assert(hour2+duration2<=hour1); not true for activities which are not incompatible
							if(hour2+duration2+minGaps > hour1)
								tt = (hour2+duration2+minGaps) - hour1;
						}
					}

					tmp=tt;
	
					nbroken+=tmp;

					if(tt>0 && conflictsString!=NULL){
						QString s=tr("Time constraint min gaps between activities broken: activity with id=%1 (%2) conflicts with activity with id=%3 (%4), they are on the same day %5 and there are %6 extra hours between them",
							"%1 is the id, %2 is the detailed description of the activity, %3 id, %4 det. descr.")
						 .arg(this->activitiesId[i])
						 .arg(getActivityDetailedDescription(r, this->activitiesId[i]))
						 .arg(this->activitiesId[j])
						 .arg(getActivityDetailedDescription(r, this->activitiesId[j]))
						 .arg(r.daysOfTheWeek[day1])
						 .arg(tt);

						s+=", ";
						s+=tr("conflicts factor increase=%1").arg(tmp*weightPercentage/100);
						s+=".";
							
						dl.append(s);
						cl.append(tmp*weightPercentage/100);
							
						*conflictsString+= s+"\n";
					}
				}
			}
		}
	}

	if(weightPercentage==100)
		assert(nbroken==0);
	return weightPercentage/100 * nbroken;
}

bool ConstraintMinGapsBetweenActivities::isRelatedToActivity(
Rules& r, Activity* a)
{
	Q_UNUSED(r);

	for(int i=0; i<this->n_activities; i++)
		if(this->activitiesId[i]==a->id)
			return true;
	return false;
}

bool ConstraintMinGapsBetweenActivities::isRelatedToTeacher(Teacher* t)
{
	Q_UNUSED(t);
	//if(t)
	//	;

	return false;
}

bool ConstraintMinGapsBetweenActivities::isRelatedToSubject(Subject* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintMinGapsBetweenActivities::isRelatedToActivityTag(ActivityTag* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintMinGapsBetweenActivities::isRelatedToStudentsSet(Rules& r, StudentsSet* s)
{
	Q_UNUSED(r);
	Q_UNUSED(s);
	/*if(s)
		;
	if(&r)
		;*/

	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

ConstraintTeachersMaxHoursDaily::ConstraintTeachersMaxHoursDaily()
	: TimeConstraint()
{
	this->type=CONSTRAINT_TEACHERS_MAX_HOURS_DAILY;
}

ConstraintTeachersMaxHoursDaily::ConstraintTeachersMaxHoursDaily(double wp, int maxhours)
 : TimeConstraint(wp)
 {
	assert(maxhours>0);
	this->maxHoursDaily=maxhours;

	this->type=CONSTRAINT_TEACHERS_MAX_HOURS_DAILY;
}

bool ConstraintTeachersMaxHoursDaily::computeInternalStructure(Rules& r)
{
	Q_UNUSED(r);
	/*if(&r!=NULL)
		;*/

	return true;
}

bool ConstraintTeachersMaxHoursDaily::hasInactiveActivities(Rules& r)
{
	Q_UNUSED(r);
	return false;
}

QString ConstraintTeachersMaxHoursDaily::getXmlDescription(Rules& r){
	Q_UNUSED(r);

	QString s="<ConstraintTeachersMaxHoursDaily>\n";
	s+="	<Weight_Percentage>"+QString::number(this->weightPercentage)+"</Weight_Percentage>\n";
	//s+="	<Compulsory>";s+=yesNo(this->compulsory);s+="</Compulsory>\n";
	s+="	<Maximum_Hours_Daily>"+QString::number(this->maxHoursDaily)+"</Maximum_Hours_Daily>\n";
	s+="</ConstraintTeachersMaxHoursDaily>\n";
	return s;
}

QString ConstraintTeachersMaxHoursDaily::getDescription(Rules& r){
	Q_UNUSED(r);

	QString s;
	s+=tr("Teachers max hours daily"), s+=", ";
	s+=tr("WP:%1\%", "Weight percentage").arg(this->weightPercentage);s+=", ";
	s+=tr("MH:%1", "Maximum hours (daily)").arg(this->maxHoursDaily);//s+=", ";

	return s;
}

QString ConstraintTeachersMaxHoursDaily::getDetailedDescription(Rules& r){
	Q_UNUSED(r);

	QString s=tr("Time constraint");s+="\n";
	s+=tr("All teachers must respect the maximum number of hours daily");s+="\n";
	s+=tr("Weight (percentage)=%1\%").arg(this->weightPercentage);s+="\n";
	s+=tr("Maximum hours daily=%1").arg(this->maxHoursDaily);s+="\n";

	return s;
}

double ConstraintTeachersMaxHoursDaily::fitness(Solution& c, Rules& r, QList<double>& cl, QList<QString>&dl, QString* conflictsString)
{
	//if the matrices subgroupsMatrix and teachersMatrix are already calculated, do not calculate them again!
	if(!c.teachersMatrixReady || !c.subgroupsMatrixReady){
		c.teachersMatrixReady=true;
		c.subgroupsMatrixReady=true;
	//if(crt_chrom!=&c || crt_rules!=&r || subgroups_conflicts<0 || teachers_conflicts<0 || c.changedForMatrixCalculation){
		subgroups_conflicts = c.getSubgroupsMatrix(r, subgroupsMatrix);
		teachers_conflicts = c.getTeachersMatrix(r, teachersMatrix);

		//crt_chrom=&c;
		//crt_rules=&r;
		
		c.changedForMatrixCalculation=false;
	}

	int nbroken;

	//without logging
	if(conflictsString==NULL){
		nbroken=0;
		for(int i=0; i<r.nInternalTeachers; i++){
			for(int d=0; d<r.nDaysPerWeek; d++){
				int n_hours_daily=0;
				for(int h=0; h<r.nHoursPerDay; h++)
					if(teachersMatrix[i][d][h]>0)
						n_hours_daily++;

				if(n_hours_daily>this->maxHoursDaily)
					nbroken++;
			}
		}
	}
	//with logging
	else{
		nbroken=0;
		for(int i=0; i<r.nInternalTeachers; i++){
			for(int d=0; d<r.nDaysPerWeek; d++){
				int n_hours_daily=0;
				for(int h=0; h<r.nHoursPerDay; h++)
					if(teachersMatrix[i][d][h]>0)
						n_hours_daily++;

				if(n_hours_daily>this->maxHoursDaily){
					nbroken++;

					if(conflictsString!=NULL){
						QString s=(tr(
						 "Time constraint teachers max %1 hours daily broken for teacher %2, on day %3, length=%4.")
						 .arg(QString::number(this->maxHoursDaily))
						 .arg(r.internalTeachersList[i]->name)
						 .arg(r.daysOfTheWeek[d])
						 .arg(n_hours_daily)
						 )
						 +
						 " "
						 +
						 (tr("This increases the conflicts total by %1").arg(QString::number(weightPercentage/100)));
						
						dl.append(s);
						cl.append(weightPercentage/100);
					
						*conflictsString+= s+"\n";
					}
				}
			}
		}
	}

	if(weightPercentage==100)	
		assert(nbroken==0);
	return weightPercentage/100 * nbroken;
}

bool ConstraintTeachersMaxHoursDaily::isRelatedToActivity(Rules& r, Activity* a)
{
	Q_UNUSED(r);
	Q_UNUSED(a);
	//if(a)
	//	;

	return false;
}

bool ConstraintTeachersMaxHoursDaily::isRelatedToTeacher(Teacher* t)
{
	Q_UNUSED(t);
	//if(t)
	//	;

	return true;
}

bool ConstraintTeachersMaxHoursDaily::isRelatedToSubject(Subject* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintTeachersMaxHoursDaily::isRelatedToActivityTag(ActivityTag* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintTeachersMaxHoursDaily::isRelatedToStudentsSet(Rules& r, StudentsSet* s)
{
	Q_UNUSED(r);
	Q_UNUSED(s);
	/*if(s)
		;
	if(&r)
		;*/

	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

ConstraintTeacherMaxHoursDaily::ConstraintTeacherMaxHoursDaily()
	: TimeConstraint()
{
	this->type=CONSTRAINT_TEACHER_MAX_HOURS_DAILY;
}

ConstraintTeacherMaxHoursDaily::ConstraintTeacherMaxHoursDaily(double wp, int maxhours, const QString& teacher)
 : TimeConstraint(wp)
 {
	assert(maxhours>0);
	this->maxHoursDaily=maxhours;
	this->teacherName=teacher;

	this->type=CONSTRAINT_TEACHER_MAX_HOURS_DAILY;
}

bool ConstraintTeacherMaxHoursDaily::computeInternalStructure(Rules& r)
{
	this->teacher_ID=r.searchTeacher(this->teacherName);
	assert(this->teacher_ID>=0);
	return true;
}

bool ConstraintTeacherMaxHoursDaily::hasInactiveActivities(Rules& r)
{
	Q_UNUSED(r);
	return false;
}

QString ConstraintTeacherMaxHoursDaily::getXmlDescription(Rules& r){
	Q_UNUSED(r);

	QString s="<ConstraintTeacherMaxHoursDaily>\n";
	s+="	<Weight_Percentage>"+QString::number(this->weightPercentage)+"</Weight_Percentage>\n";
	//s+="	<Compulsory>";s+=yesNo(this->compulsory);s+="</Compulsory>\n";
	s+="	<Teacher_Name>"+protect(this->teacherName)+"</Teacher_Name>\n";
	s+="	<Maximum_Hours_Daily>"+QString::number(this->maxHoursDaily)+"</Maximum_Hours_Daily>\n";
	s+="</ConstraintTeacherMaxHoursDaily>\n";
	return s;
}

QString ConstraintTeacherMaxHoursDaily::getDescription(Rules& r){
	Q_UNUSED(r);

	QString s;
	s+=tr("Teacher max hours daily");s+=", ";
	s+=tr("WP:%1\%", "Weight percentage").arg(this->weightPercentage);s+=", ";
	s+=tr("T:%1", "Teacher").arg(this->teacherName);s+=", ";
	s+=tr("MH:%1", "Maximum hours (daily)").arg(this->maxHoursDaily); //s+=", ";

	return s;
}

QString ConstraintTeacherMaxHoursDaily::getDetailedDescription(Rules& r){
	Q_UNUSED(r);

	QString s=tr("Time constraint");s+="\n";
	s+=tr("A teacher must respect the maximum number of hours daily");s+="\n";
	s+=tr("Weight (percentage)=%1\%").arg(this->weightPercentage);s+="\n";
	s+=tr("Teacher=%1").arg(this->teacherName);s+="\n";
	s+=tr("Maximum hours daily=%1").arg(this->maxHoursDaily);s+="\n";

	return s;
}

double ConstraintTeacherMaxHoursDaily::fitness(Solution& c, Rules& r, QList<double>& cl, QList<QString>&dl, QString* conflictsString)
{
	//if the matrices subgroupsMatrix and teachersMatrix are already calculated, do not calculate them again!
	if(!c.teachersMatrixReady || !c.subgroupsMatrixReady){
		c.teachersMatrixReady=true;
		c.subgroupsMatrixReady=true;
	//if(crt_chrom!=&c || crt_rules!=&r || subgroups_conflicts<0 || teachers_conflicts<0 || c.changedForMatrixCalculation){
		subgroups_conflicts = c.getSubgroupsMatrix(r, subgroupsMatrix);
		teachers_conflicts = c.getTeachersMatrix(r, teachersMatrix);

		//crt_chrom=&c;
		//crt_rules=&r;
		
		c.changedForMatrixCalculation=false;
	}

	int nbroken;

	//without logging
	if(conflictsString==NULL){
		nbroken=0;
		int i=this->teacher_ID;
		//for(int i=0; i<r.nInternalTeachers; i++){
		for(int d=0; d<r.nDaysPerWeek; d++){
			int n_hours_daily=0;
			for(int h=0; h<r.nHoursPerDay; h++)
				if(teachersMatrix[i][d][h]>0)
					n_hours_daily++;

			if(n_hours_daily>this->maxHoursDaily){
				nbroken++;
			}
		}
		//}
	}
	//with logging
	else{
		nbroken=0;
		int i=this->teacher_ID;
		//for(int i=0; i<r.nInternalTeachers; i++){
		for(int d=0; d<r.nDaysPerWeek; d++){
			int n_hours_daily=0;
			for(int h=0; h<r.nHoursPerDay; h++)
				if(teachersMatrix[i][d][h]>0)
					n_hours_daily++;

			if(n_hours_daily>this->maxHoursDaily){
				nbroken++;

				if(conflictsString!=NULL){
					QString s=(tr(
					 "Time constraint teacher max %1 hours daily broken for teacher %2, on day %3, length=%4.")
					 .arg(QString::number(this->maxHoursDaily))
					 .arg(r.internalTeachersList[i]->name)
					 .arg(r.daysOfTheWeek[d])
					 .arg(n_hours_daily)
					 )
					 +" "
					 +
					 (tr("This increases the conflicts total by %1").arg(QString::number(weightPercentage/100)));
						
					dl.append(s);
					cl.append(weightPercentage/100);
				
					*conflictsString+= s+"\n";
				}
			}
		}
		//}
	}

	if(weightPercentage==100)	
		assert(nbroken==0);
	return weightPercentage/100 * nbroken;
}

bool ConstraintTeacherMaxHoursDaily::isRelatedToActivity(Rules& r, Activity* a)
{
	Q_UNUSED(r);
	Q_UNUSED(a);
	//if(a)
	//	;

	return false;
}

bool ConstraintTeacherMaxHoursDaily::isRelatedToTeacher(Teacher* t)
{
	if(this->teacherName==t->name)
		return true;
	return false;
}

bool ConstraintTeacherMaxHoursDaily::isRelatedToSubject(Subject* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintTeacherMaxHoursDaily::isRelatedToActivityTag(ActivityTag* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintTeacherMaxHoursDaily::isRelatedToStudentsSet(Rules& r, StudentsSet* s)
{
	Q_UNUSED(r);
	Q_UNUSED(s);
	/*if(s)
		;
	if(&r)
		;*/

	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

ConstraintTeachersMaxHoursContinuously::ConstraintTeachersMaxHoursContinuously()
	: TimeConstraint()
{
	this->type=CONSTRAINT_TEACHERS_MAX_HOURS_CONTINUOUSLY;
}

ConstraintTeachersMaxHoursContinuously::ConstraintTeachersMaxHoursContinuously(double wp, int maxhours)
 : TimeConstraint(wp)
 {
	assert(maxhours>0);
	this->maxHoursContinuously=maxhours;

	this->type=CONSTRAINT_TEACHERS_MAX_HOURS_CONTINUOUSLY;
}

bool ConstraintTeachersMaxHoursContinuously::computeInternalStructure(Rules& r)
{
	Q_UNUSED(r);

	return true;
}

bool ConstraintTeachersMaxHoursContinuously::hasInactiveActivities(Rules& r)
{
	Q_UNUSED(r);
	return false;
}

QString ConstraintTeachersMaxHoursContinuously::getXmlDescription(Rules& r){
	Q_UNUSED(r);

	QString s="<ConstraintTeachersMaxHoursContinuously>\n";
	s+="	<Weight_Percentage>"+QString::number(this->weightPercentage)+"</Weight_Percentage>\n";
	//s+="	<Compulsory>";s+=yesNo(this->compulsory);s+="</Compulsory>\n";
	s+="	<Maximum_Hours_Continuously>"+QString::number(this->maxHoursContinuously)+"</Maximum_Hours_Continuously>\n";
	s+="</ConstraintTeachersMaxHoursContinuously>\n";
	return s;
}

QString ConstraintTeachersMaxHoursContinuously::getDescription(Rules& r){
	Q_UNUSED(r);

	QString s;
	s+=tr("Teachers max hours continuously");s+=", ";
	s+=tr("WP:%1\%", "Weight percentage").arg(this->weightPercentage);s+=", ";
	s+=tr("MH:%1", "Maximum hours (continuously)").arg(this->maxHoursContinuously);//s+=", ";

	return s;
}

QString ConstraintTeachersMaxHoursContinuously::getDetailedDescription(Rules& r){
	Q_UNUSED(r);

	QString s=tr("Time constraint");s+="\n";
	s+=tr("All teachers must respect the maximum number of hours continuously");s+="\n";
	s+=tr("Weight (percentage)=%1\%").arg(this->weightPercentage);s+="\n";
	s+=tr("Maximum hours continuously=%1").arg(this->maxHoursContinuously);s+="\n";

	return s;
}

double ConstraintTeachersMaxHoursContinuously::fitness(Solution& c, Rules& r, QList<double>& cl, QList<QString>&dl, QString* conflictsString)
{
	//if the matrices subgroupsMatrix and teachersMatrix are already calculated, do not calculate them again!
	if(!c.teachersMatrixReady || !c.subgroupsMatrixReady){
		c.teachersMatrixReady=true;
		c.subgroupsMatrixReady=true;
	//if(crt_chrom!=&c || crt_rules!=&r || subgroups_conflicts<0 || teachers_conflicts<0 || c.changedForMatrixCalculation){
		subgroups_conflicts = c.getSubgroupsMatrix(r, subgroupsMatrix);
		teachers_conflicts = c.getTeachersMatrix(r, teachersMatrix);

		//crt_chrom=&c;
		//crt_rules=&r;
		
		c.changedForMatrixCalculation=false;
	}

	int nbroken;

	nbroken=0;
	for(int i=0; i<r.nInternalTeachers; i++){
		for(int d=0; d<r.nDaysPerWeek; d++){
			int nc=0;
			for(int h=0; h<r.nHoursPerDay; h++){
				if(teachersMatrix[i][d][h]>0)
					nc++;
				else{
					if(nc>this->maxHoursContinuously){
						nbroken++;

						if(conflictsString!=NULL){
							QString s=(tr(
							 "Time constraint teachers max %1 hours continuously broken for teacher %2, on day %3, length=%4.")
							 .arg(QString::number(this->maxHoursContinuously))
							 .arg(r.internalTeachersList[i]->name)
							 .arg(r.daysOfTheWeek[d])
							 .arg(nc)
							 )
							 +
							 " "
							 +
							 (tr("This increases the conflicts total by %1").arg(QString::number(weightPercentage/100)));
							
							dl.append(s);
							cl.append(weightPercentage/100);
				
							*conflictsString+= s+"\n";
						}
					}
				
					nc=0;
				}
			}

			if(nc>this->maxHoursContinuously){
				nbroken++;

				if(conflictsString!=NULL){
					QString s=(tr(
					 "Time constraint teachers max %1 hours continuously broken for teacher %2, on day %3, length=%4.")
					 .arg(QString::number(this->maxHoursContinuously))
					 .arg(r.internalTeachersList[i]->name)
					 .arg(r.daysOfTheWeek[d])
					 .arg(nc)
					 )
					 +
					 " "
					 +
					 (tr("This increases the conflicts total by %1").arg(QString::number(weightPercentage/100)));
							
					dl.append(s);
					cl.append(weightPercentage/100);
				
					*conflictsString+= s+"\n";
				}
			}
		}
	}

	if(weightPercentage==100)	
		assert(nbroken==0);
	return weightPercentage/100 * nbroken;
}

bool ConstraintTeachersMaxHoursContinuously::isRelatedToActivity(Rules& r, Activity* a)
{
	Q_UNUSED(r);
	Q_UNUSED(a);
	//if(a)
	//	;

	return false;
}

bool ConstraintTeachersMaxHoursContinuously::isRelatedToTeacher(Teacher* t)
{
	Q_UNUSED(t);
	//if(t)
	//	;

	return true;
}

bool ConstraintTeachersMaxHoursContinuously::isRelatedToSubject(Subject* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintTeachersMaxHoursContinuously::isRelatedToActivityTag(ActivityTag* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintTeachersMaxHoursContinuously::isRelatedToStudentsSet(Rules& r, StudentsSet* s)
{
	Q_UNUSED(r);
	Q_UNUSED(s);
	/*if(s)
		;
	if(&r)
		;*/

	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

ConstraintTeacherMaxHoursContinuously::ConstraintTeacherMaxHoursContinuously()
	: TimeConstraint()
{
	this->type=CONSTRAINT_TEACHER_MAX_HOURS_CONTINUOUSLY;
}

ConstraintTeacherMaxHoursContinuously::ConstraintTeacherMaxHoursContinuously(double wp, int maxhours, const QString& teacher)
 : TimeConstraint(wp)
 {
	assert(maxhours>0);
	this->maxHoursContinuously=maxhours;
	this->teacherName=teacher;

	this->type=CONSTRAINT_TEACHER_MAX_HOURS_CONTINUOUSLY;
}

bool ConstraintTeacherMaxHoursContinuously::computeInternalStructure(Rules& r)
{
	this->teacher_ID=r.searchTeacher(this->teacherName);
	assert(this->teacher_ID>=0);
	return true;
}

bool ConstraintTeacherMaxHoursContinuously::hasInactiveActivities(Rules& r)
{
	Q_UNUSED(r);
	return false;
}

QString ConstraintTeacherMaxHoursContinuously::getXmlDescription(Rules& r){
	Q_UNUSED(r);

	QString s="<ConstraintTeacherMaxHoursContinuously>\n";
	s+="	<Weight_Percentage>"+QString::number(this->weightPercentage)+"</Weight_Percentage>\n";
	//s+="	<Compulsory>";s+=yesNo(this->compulsory);s+="</Compulsory>\n";
	s+="	<Teacher_Name>"+protect(this->teacherName)+"</Teacher_Name>\n";
	s+="	<Maximum_Hours_Continuously>"+QString::number(this->maxHoursContinuously)+"</Maximum_Hours_Continuously>\n";
	s+="</ConstraintTeacherMaxHoursContinuously>\n";
	return s;
}

QString ConstraintTeacherMaxHoursContinuously::getDescription(Rules& r){
	Q_UNUSED(r);

	QString s;
	s+=tr("Teacher max hours continuously");s+=", ";
	s+=tr("WP:%1\%", "Weight percentage").arg(this->weightPercentage);s+=", ";
	s+=tr("T:%1", "Teacher").arg(this->teacherName);s+=", ";
	s+=tr("MH:%1", "Maximum hours continuously").arg(this->maxHoursContinuously); //s+=", ";

	return s;
}

QString ConstraintTeacherMaxHoursContinuously::getDetailedDescription(Rules& r){
	Q_UNUSED(r);

	QString s=tr("Time constraint");s+="\n";
	s+=tr("A teacher must respect the maximum number of hours continuously");s+="\n";
	s+=tr("Weight (percentage)=%1\%").arg(this->weightPercentage);s+="\n";
	s+=tr("Teacher=%1").arg(this->teacherName);s+="\n";
	s+=tr("Maximum hours continuously=%1").arg(this->maxHoursContinuously);s+="\n";

	return s;
}

double ConstraintTeacherMaxHoursContinuously::fitness(Solution& c, Rules& r, QList<double>& cl, QList<QString>&dl, QString* conflictsString)
{
	//if the matrices subgroupsMatrix and teachersMatrix are already calculated, do not calculate them again!
	if(!c.teachersMatrixReady || !c.subgroupsMatrixReady){
		c.teachersMatrixReady=true;
		c.subgroupsMatrixReady=true;
	//if(crt_chrom!=&c || crt_rules!=&r || subgroups_conflicts<0 || teachers_conflicts<0 || c.changedForMatrixCalculation){
		subgroups_conflicts = c.getSubgroupsMatrix(r, subgroupsMatrix);
		teachers_conflicts = c.getTeachersMatrix(r, teachersMatrix);

		//crt_chrom=&c;
		//crt_rules=&r;
		
		c.changedForMatrixCalculation=false;
	}

	int nbroken;

	nbroken=0;
	int i=this->teacher_ID;
	//for(int i=0; i<r.nInternalTeachers; i++){
		for(int d=0; d<r.nDaysPerWeek; d++){
			int nc=0;
			for(int h=0; h<r.nHoursPerDay; h++){
				if(teachersMatrix[i][d][h]>0)
					nc++;
				else{
					if(nc>this->maxHoursContinuously){
						nbroken++;

						if(conflictsString!=NULL){
							QString s=(tr(
							 "Time constraint teacher max %1 hours continuously broken for teacher %2, on day %3, length=%4.")
							 .arg(QString::number(this->maxHoursContinuously))
							 .arg(r.internalTeachersList[i]->name)
							 .arg(r.daysOfTheWeek[d])
							 .arg(nc)
							 )
							 +
							 " "
							 +
							 (tr("This increases the conflicts total by %1").arg(QString::number(weightPercentage/100)));
							
							dl.append(s);
							cl.append(weightPercentage/100);
				
							*conflictsString+= s+"\n";
						}
					}
				
					nc=0;
				}
			}

			if(nc>this->maxHoursContinuously){
				nbroken++;

				if(conflictsString!=NULL){
					QString s=(tr(
					 "Time constraint teacher max %1 hours continuously broken for teacher %2, on day %3, length=%4.")
					 .arg(QString::number(this->maxHoursContinuously))
					 .arg(r.internalTeachersList[i]->name)
					 .arg(r.daysOfTheWeek[d])
					 .arg(nc)
					 )
					 +
					 " "
					 +
					 (tr("This increases the conflicts total by %1").arg(QString::number(weightPercentage/100)));
							
					dl.append(s);
					cl.append(weightPercentage/100);
				
					*conflictsString+= s+"\n";
				}
			}
		}
	//}

	if(weightPercentage==100)	
		assert(nbroken==0);
	return weightPercentage/100 * nbroken;
}

bool ConstraintTeacherMaxHoursContinuously::isRelatedToActivity(Rules& r, Activity* a)
{
	Q_UNUSED(r);
	Q_UNUSED(a);
	//if(a)
	//	;

	return false;
}

bool ConstraintTeacherMaxHoursContinuously::isRelatedToTeacher(Teacher* t)
{
	if(this->teacherName==t->name)
		return true;
	return false;
}

bool ConstraintTeacherMaxHoursContinuously::isRelatedToSubject(Subject* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintTeacherMaxHoursContinuously::isRelatedToActivityTag(ActivityTag* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintTeacherMaxHoursContinuously::isRelatedToStudentsSet(Rules& r, StudentsSet* s)
{
	Q_UNUSED(r);
	Q_UNUSED(s);
	/*if(s)
		;
	if(&r)
		;*/

	return false;
}



///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

ConstraintTeachersActivityTagMaxHoursContinuously::ConstraintTeachersActivityTagMaxHoursContinuously()
	: TimeConstraint()
{
	this->type=CONSTRAINT_TEACHERS_ACTIVITY_TAG_MAX_HOURS_CONTINUOUSLY;
}

ConstraintTeachersActivityTagMaxHoursContinuously::ConstraintTeachersActivityTagMaxHoursContinuously(double wp, int maxhours, const QString& activityTag)
 : TimeConstraint(wp)
 {
	assert(maxhours>0);
	this->maxHoursContinuously=maxhours;
	this->activityTagName=activityTag;

	this->type=CONSTRAINT_TEACHERS_ACTIVITY_TAG_MAX_HOURS_CONTINUOUSLY;
}

bool ConstraintTeachersActivityTagMaxHoursContinuously::computeInternalStructure(Rules& r)
{
	//Q_UNUSED(r);

	this->activityTagIndex=r.searchActivityTag(this->activityTagName);
	assert(this->activityTagIndex>=0);
	
	this->canonicalTeachersList.clear();
	for(int i=0; i<r.nInternalTeachers; i++){
		bool found=false;
	
/*		Teacher* tch=r.internalTeachersList[i];
		foreach(int actIndex, tch->activitiesForTeacher){
			int actTagIndex=r.internalActivitiesList[actIndex].activityTagIndex;
			if(actTagIndex==this->activityTagIndex){
				found=true;
				break;
			}
		}*/
		
		Teacher* tch=r.internalTeachersList[i];
		foreach(int actIndex, tch->activitiesForTeacher){
			if(r.internalActivitiesList[actIndex].iActivityTagsSet.contains(this->activityTagIndex)){
				found=true;
				break;
			}
		}
		
		if(found)
			this->canonicalTeachersList.append(i);
	}

	return true;
}

bool ConstraintTeachersActivityTagMaxHoursContinuously::hasInactiveActivities(Rules& r)
{
	Q_UNUSED(r);
	return false;
}

QString ConstraintTeachersActivityTagMaxHoursContinuously::getXmlDescription(Rules& r){
	Q_UNUSED(r);

	QString s="<ConstraintTeachersActivityTagMaxHoursContinuously>\n";
	s+="	<Weight_Percentage>"+QString::number(this->weightPercentage)+"</Weight_Percentage>\n";
	//s+="	<Compulsory>";s+=yesNo(this->compulsory);s+="</Compulsory>\n";
	s+="	<Activity_Tag_Name>"+protect(this->activityTagName)+"</Activity_Tag_Name>\n";
	s+="	<Maximum_Hours_Continuously>"+QString::number(this->maxHoursContinuously)+"</Maximum_Hours_Continuously>\n";
	s+="</ConstraintTeachersActivityTagMaxHoursContinuously>\n";
	return s;
}

QString ConstraintTeachersActivityTagMaxHoursContinuously::getDescription(Rules& r){
	Q_UNUSED(r);

	QString s;
	s+=tr("Teachers for activity tag %1 have max %2 hours continuously").arg(this->activityTagName).arg(this->maxHoursContinuously);s+=", ";
	s+=tr("WP:%1\%", "Weight percentage").arg(this->weightPercentage);//s+=", ";

	return s;
}

QString ConstraintTeachersActivityTagMaxHoursContinuously::getDetailedDescription(Rules& r){
	Q_UNUSED(r);

	QString s=tr("Time constraint");s+="\n";
	s+=tr("All teachers, for an activity tag, must respect the maximum number of hours continuously");s+="\n";
	s+=tr("Weight (percentage)=%1\%").arg(this->weightPercentage);s+="\n";
	s+=tr("Activity tag=%1").arg(this->activityTagName); s+="\n";
	s+=tr("Maximum hours continuously=%1").arg(this->maxHoursContinuously); s+="\n";

	return s;
}

double ConstraintTeachersActivityTagMaxHoursContinuously::fitness(Solution& c, Rules& r, QList<double>& cl, QList<QString>&dl, QString* conflictsString)
{
	//if the matrices subgroupsMatrix and teachersMatrix are already calculated, do not calculate them again!
	if(!c.teachersMatrixReady || !c.subgroupsMatrixReady){
		c.teachersMatrixReady=true;
		c.subgroupsMatrixReady=true;
	//if(crt_chrom!=&c || crt_rules!=&r || subgroups_conflicts<0 || teachers_conflicts<0 || c.changedForMatrixCalculation){
		subgroups_conflicts = c.getSubgroupsMatrix(r, subgroupsMatrix);
		teachers_conflicts = c.getTeachersMatrix(r, teachersMatrix);

		//crt_chrom=&c;
		//crt_rules=&r;
		
		c.changedForMatrixCalculation=false;
	}

	int nbroken;

	nbroken=0;
	foreach(int i, this->canonicalTeachersList){
	//for(int i=0; i<r.nInternalTeachers; i++){
		Teacher* tch=r.internalTeachersList[i];
		int crtTeacherTimetableActivityTag[MAX_DAYS_PER_WEEK][MAX_HOURS_PER_DAY];
		for(int d=0; d<r.nDaysPerWeek; d++)
			for(int h=0; h<r.nHoursPerDay; h++)
				crtTeacherTimetableActivityTag[d][h]=-1;
		foreach(int ai, tch->activitiesForTeacher)if(c.times[ai]!=UNALLOCATED_TIME){
			int d=c.times[ai]%r.nDaysPerWeek;
			int h=c.times[ai]/r.nDaysPerWeek;
			for(int dur=0; dur<r.internalActivitiesList[ai].duration; dur++){
				assert(h+dur<r.nHoursPerDay);
				assert(crtTeacherTimetableActivityTag[d][h+dur]==-1);
				if(r.internalActivitiesList[ai].iActivityTagsSet.contains(this->activityTagIndex))
					crtTeacherTimetableActivityTag[d][h+dur]=this->activityTagIndex;
				//crtTeacherTimetableActivityTag[d][h+dur]=r.internalActivitiesList[ai].activityTagIndex;
			}
		}
	
		for(int d=0; d<r.nDaysPerWeek; d++){
			int nc=0;
			for(int h=0; h<r.nHoursPerDay; h++){
				bool inc=false;
				if(crtTeacherTimetableActivityTag[d][h]==this->activityTagIndex)
					inc=true;
				
				if(inc){
					nc++;
				}
				else{
					if(nc>this->maxHoursContinuously){
						nbroken++;

						if(conflictsString!=NULL){
							QString s=(tr(
							 "Time constraint teachers activity tag %1 max %2 hours continuously broken for teacher %3, on day %4, length=%5.")
							 .arg(this->activityTagName)
							 .arg(QString::number(this->maxHoursContinuously))
							 .arg(r.internalTeachersList[i]->name)
							 .arg(r.daysOfTheWeek[d])
							 .arg(nc)
							 )
							 +
							 " "
							 +
							 (tr("This increases the conflicts total by %1").arg(QString::number(weightPercentage/100)));
							
							dl.append(s);
							cl.append(weightPercentage/100);
				
							*conflictsString+= s+"\n";
						}
					}
				
					nc=0;
				}
			}

			if(nc>this->maxHoursContinuously){
				nbroken++;

				if(conflictsString!=NULL){
					QString s=(tr(
					 "Time constraint teachers activity tag %1 max %2 hours continuously broken for teacher %3, on day %4, length=%5.")
					 .arg(this->activityTagName)
					 .arg(QString::number(this->maxHoursContinuously))
					 .arg(r.internalTeachersList[i]->name)
					 .arg(r.daysOfTheWeek[d])
					 .arg(nc)
					 )
					 +
					 " "
					 +
					 (tr("This increases the conflicts total by %1").arg(QString::number(weightPercentage/100)));
							
					dl.append(s);
					cl.append(weightPercentage/100);
				
					*conflictsString+= s+"\n";
				}
			}
		}
	}

	if(weightPercentage==100)	
		assert(nbroken==0);
	return weightPercentage/100 * nbroken;
}

bool ConstraintTeachersActivityTagMaxHoursContinuously::isRelatedToActivity(Rules& r, Activity* a)
{
	Q_UNUSED(r);
	Q_UNUSED(a);
	//if(a)
	//	;

	return false;
}

bool ConstraintTeachersActivityTagMaxHoursContinuously::isRelatedToTeacher(Teacher* t)
{
	Q_UNUSED(t);
	//if(t)
	//	;

	return true;
}

bool ConstraintTeachersActivityTagMaxHoursContinuously::isRelatedToSubject(Subject* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintTeachersActivityTagMaxHoursContinuously::isRelatedToActivityTag(ActivityTag* s)
{
	return s->name==this->activityTagName;
	
	//return false;
}

bool ConstraintTeachersActivityTagMaxHoursContinuously::isRelatedToStudentsSet(Rules& r, StudentsSet* s)
{
	Q_UNUSED(r);
	Q_UNUSED(s);
	/*if(s)
		;
	if(&r)
		;*/

	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
ConstraintTeacherActivityTagMaxHoursContinuously::ConstraintTeacherActivityTagMaxHoursContinuously()
	: TimeConstraint()
{
	this->type=CONSTRAINT_TEACHER_ACTIVITY_TAG_MAX_HOURS_CONTINUOUSLY;
}

ConstraintTeacherActivityTagMaxHoursContinuously::ConstraintTeacherActivityTagMaxHoursContinuously(double wp, int maxhours, const QString& teacher, const QString& activityTag)
 : TimeConstraint(wp)
 {
	assert(maxhours>0);
	this->maxHoursContinuously=maxhours;
	this->teacherName=teacher;
	this->activityTagName=activityTag;

	this->type=CONSTRAINT_TEACHER_ACTIVITY_TAG_MAX_HOURS_CONTINUOUSLY;
}

bool ConstraintTeacherActivityTagMaxHoursContinuously::computeInternalStructure(Rules& r)
{
	this->teacher_ID=r.searchTeacher(this->teacherName);
	assert(this->teacher_ID>=0);

	this->activityTagIndex=r.searchActivityTag(this->activityTagName);
	assert(this->activityTagIndex>=0);

	this->canonicalTeachersList.clear();
	int i=this->teacher_ID;
	bool found=false;
	
/*	Teacher* tch=r.internalTeachersList[i];
	foreach(int actIndex, tch->activitiesForTeacher){
		int actTagIndex=r.internalActivitiesList[actIndex].activityTagIndex;
		if(actTagIndex==this->activityTagIndex){
			found=true;
			break;
		}
	}*/
		
	Teacher* tch=r.internalTeachersList[i];
	foreach(int actIndex, tch->activitiesForTeacher){
		if(r.internalActivitiesList[actIndex].iActivityTagsSet.contains(this->activityTagIndex)){
			found=true;
			break;
		}
	}
		
	if(found)
		this->canonicalTeachersList.append(i);

	return true;
}

bool ConstraintTeacherActivityTagMaxHoursContinuously::hasInactiveActivities(Rules& r)
{
	Q_UNUSED(r);
	return false;
}

QString ConstraintTeacherActivityTagMaxHoursContinuously::getXmlDescription(Rules& r){
	Q_UNUSED(r);

	QString s="<ConstraintTeacherActivityTagMaxHoursContinuously>\n";
	s+="	<Weight_Percentage>"+QString::number(this->weightPercentage)+"</Weight_Percentage>\n";
	//s+="	<Compulsory>";s+=yesNo(this->compulsory);s+="</Compulsory>\n";
	s+="	<Teacher_Name>"+protect(this->teacherName)+"</Teacher_Name>\n";
	s+="	<Activity_Tag_Name>"+protect(this->activityTagName)+"</Activity_Tag_Name>\n";
	s+="	<Maximum_Hours_Continuously>"+QString::number(this->maxHoursContinuously)+"</Maximum_Hours_Continuously>\n";
	s+="</ConstraintTeacherActivityTagMaxHoursContinuously>\n";
	return s;
}

QString ConstraintTeacherActivityTagMaxHoursContinuously::getDescription(Rules& r){
	Q_UNUSED(r);

	QString s;
	s+=tr("Teacher %1 for activity tag %2 has max %3 hours continuously").arg(this->teacherName).arg(this->activityTagName).arg(this->maxHoursContinuously);s+=", ";
	//s+=tr("TN:%1", "Teacher name").arg(this->teacherName);s+=", ";
	//s+=tr("ATN:%1", "Activity tag name").arg(this->activityTagName);s+=", ";
	s+=tr("WP:%1\%", "Weight percentage").arg(this->weightPercentage);//s+=", ";

	return s;
}

QString ConstraintTeacherActivityTagMaxHoursContinuously::getDetailedDescription(Rules& r){
	Q_UNUSED(r);

	QString s=tr("Time constraint");s+="\n";
	s+=tr("A teacher for an activity tag must respect the maximum number of hours continuously");s+="\n";
	s+=tr("Weight (percentage)=%1\%").arg(this->weightPercentage);s+="\n";
	s+=tr("Teacher=%1").arg(this->teacherName);s+="\n";
	s+=tr("Activity tag=%1").arg(this->activityTagName);s+="\n";
	s+=tr("Maximum hours continuously=%1").arg(this->maxHoursContinuously); s+="\n";

	return s;
}

double ConstraintTeacherActivityTagMaxHoursContinuously::fitness(Solution& c, Rules& r, QList<double>& cl, QList<QString>&dl, QString* conflictsString)
{
	//if the matrices subgroupsMatrix and teachersMatrix are already calculated, do not calculate them again!
	if(!c.teachersMatrixReady || !c.subgroupsMatrixReady){
		c.teachersMatrixReady=true;
		c.subgroupsMatrixReady=true;
	//if(crt_chrom!=&c || crt_rules!=&r || subgroups_conflicts<0 || teachers_conflicts<0 || c.changedForMatrixCalculation){
		subgroups_conflicts = c.getSubgroupsMatrix(r, subgroupsMatrix);
		teachers_conflicts = c.getTeachersMatrix(r, teachersMatrix);

		//crt_chrom=&c;
		//crt_rules=&r;
		
		c.changedForMatrixCalculation=false;
	}

	int nbroken;

	nbroken=0;
	//int i=this->teacher_ID;
	//for(int i=0; i<r.nInternalTeachers; i++){
	foreach(int i, this->canonicalTeachersList){
		Teacher* tch=r.internalTeachersList[i];
		int crtTeacherTimetableActivityTag[MAX_DAYS_PER_WEEK][MAX_HOURS_PER_DAY];
		for(int d=0; d<r.nDaysPerWeek; d++)
			for(int h=0; h<r.nHoursPerDay; h++)
				crtTeacherTimetableActivityTag[d][h]=-1;
		foreach(int ai, tch->activitiesForTeacher)if(c.times[ai]!=UNALLOCATED_TIME){
			int d=c.times[ai]%r.nDaysPerWeek;
			int h=c.times[ai]/r.nDaysPerWeek;
			for(int dur=0; dur<r.internalActivitiesList[ai].duration; dur++){
				assert(h+dur<r.nHoursPerDay);
				assert(crtTeacherTimetableActivityTag[d][h+dur]==-1);
				if(r.internalActivitiesList[ai].iActivityTagsSet.contains(this->activityTagIndex))
				//crtTeacherTimetableActivityTag[d][h+dur]=r.internalActivitiesList[ai].activityTagIndex;
					crtTeacherTimetableActivityTag[d][h+dur]=this->activityTagIndex;
			}
		}

		for(int d=0; d<r.nDaysPerWeek; d++){
			int nc=0;
			for(int h=0; h<r.nHoursPerDay; h++){
				bool inc=false;

				if(crtTeacherTimetableActivityTag[d][h]==this->activityTagIndex)
					inc=true;
				
				if(inc)
					nc++;
				else{
					if(nc>this->maxHoursContinuously){
						nbroken++;

						if(conflictsString!=NULL){
							QString s=(tr(
							 "Time constraint teacher activity tag max %1 hours continuously broken for teacher %2, activity tag %3, on day %4, length=%5.")
							 .arg(QString::number(this->maxHoursContinuously))
							 .arg(r.internalTeachersList[i]->name)
							 .arg(this->activityTagName)
							 .arg(r.daysOfTheWeek[d])
							 .arg(nc)
							 )
							 +
							 " "
							 +
							 (tr("This increases the conflicts total by %1").arg(QString::number(weightPercentage/100)));
							
							dl.append(s);
							cl.append(weightPercentage/100);
				
							*conflictsString+= s+"\n";
						}
					}
				
					nc=0;
				}
			}

			if(nc>this->maxHoursContinuously){
				nbroken++;

				if(conflictsString!=NULL){
					QString s=(tr(
					 "Time constraint teacher activity tag max %1 hours continuously broken for teacher %2, activity tag %3, on day %4, length=%5.")
					 .arg(QString::number(this->maxHoursContinuously))
					 .arg(r.internalTeachersList[i]->name)
					 .arg(this->activityTagName)
					 .arg(r.daysOfTheWeek[d])
					 .arg(nc)
					 )
					 +
					 " "
					 +
					 (tr("This increases the conflicts total by %1").arg(QString::number(weightPercentage/100)));
							
					dl.append(s);
					cl.append(weightPercentage/100);
				
					*conflictsString+= s+"\n";
				}
			}
		}
	}

	if(weightPercentage==100)	
		assert(nbroken==0);
	return weightPercentage/100 * nbroken;
}

bool ConstraintTeacherActivityTagMaxHoursContinuously::isRelatedToActivity(Rules& r, Activity* a)
{
	Q_UNUSED(r);
	Q_UNUSED(a);

	return false;
}

bool ConstraintTeacherActivityTagMaxHoursContinuously::isRelatedToTeacher(Teacher* t)
{
	if(this->teacherName==t->name)
		return true;
	return false;
}

bool ConstraintTeacherActivityTagMaxHoursContinuously::isRelatedToSubject(Subject* s)
{
	Q_UNUSED(s);

	return false;
}

bool ConstraintTeacherActivityTagMaxHoursContinuously::isRelatedToActivityTag(ActivityTag* s)
{
	return this->activityTagName==s->name;
}

bool ConstraintTeacherActivityTagMaxHoursContinuously::isRelatedToStudentsSet(Rules& r, StudentsSet* s)
{
	Q_UNUSED(r);
	Q_UNUSED(s);

	return false;
}




///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

ConstraintTeacherMaxDaysPerWeek::ConstraintTeacherMaxDaysPerWeek()
	: TimeConstraint()
{
	this->type=CONSTRAINT_TEACHER_MAX_DAYS_PER_WEEK;
}

ConstraintTeacherMaxDaysPerWeek::ConstraintTeacherMaxDaysPerWeek(double wp, int maxnd, QString tn)
	 : TimeConstraint(wp)
{
	this->teacherName = tn;
	this->maxDaysPerWeek=maxnd;
	this->type=CONSTRAINT_TEACHER_MAX_DAYS_PER_WEEK;
}

bool ConstraintTeacherMaxDaysPerWeek::computeInternalStructure(Rules& r)
{
	this->teacher_ID=r.searchTeacher(this->teacherName);
	assert(this->teacher_ID>=0);
	return true;
}

bool ConstraintTeacherMaxDaysPerWeek::hasInactiveActivities(Rules& r)
{
	Q_UNUSED(r);
	return false;
}

QString ConstraintTeacherMaxDaysPerWeek::getXmlDescription(Rules& r)
{
	Q_UNUSED(r);

	QString s="<ConstraintTeacherMaxDaysPerWeek>\n";
	s+="	<Weight_Percentage>"+QString::number(this->weightPercentage)+"</Weight_Percentage>\n";
	//s+="	<Compulsory>";s+=yesNo(this->compulsory);s+="</Compulsory>\n";
	s+="	<Teacher_Name>"+protect(this->teacherName)+"</Teacher_Name>\n";
	s+="	<Max_Days_Per_Week>"+QString::number(this->maxDaysPerWeek)+"</Max_Days_Per_Week>\n";
	s+="</ConstraintTeacherMaxDaysPerWeek>\n";
	return s;
}

QString ConstraintTeacherMaxDaysPerWeek::getDescription(Rules& r){
	Q_UNUSED(r);

	QString s=tr("Teacher max days per week");s+=", ";
	s+=tr("WP:%1\%", "Weight percentage").arg(this->weightPercentage);s+=", ";
	s+=tr("T:%1", "Teacher").arg(this->teacherName);s+=", ";
	s+=tr("MD:%1", "Max days (per week)").arg(this->maxDaysPerWeek);

	return s;
}

QString ConstraintTeacherMaxDaysPerWeek::getDetailedDescription(Rules& r){
	Q_UNUSED(r);

	QString s=tr("Time constraint");s+="\n";
	s+=tr("A teacher must respect the maximum number of days per week");s+="\n";
	s+=tr("Weight (percentage)=%1\%").arg(this->weightPercentage);s+="\n";
	s+=tr("Teacher=%1").arg(this->teacherName);s+="\n";
	s+=tr("Maximum days per week=%1").arg(this->maxDaysPerWeek);s+="\n";

	return s;
}

double ConstraintTeacherMaxDaysPerWeek::fitness(Solution& c, Rules& r, QList<double>& cl, QList<QString>&dl, QString *conflictsString)
{
	//if the matrices subgroupsMatrix and teachersMatrix are already calculated, do not calculate them again!
	if(!c.teachersMatrixReady || !c.subgroupsMatrixReady){
		c.teachersMatrixReady=true;
		c.subgroupsMatrixReady=true;
	//if(crt_chrom!=&c || crt_rules!=&r || subgroups_conflicts<0 || teachers_conflicts<0 || c.changedForMatrixCalculation){
		subgroups_conflicts = c.getSubgroupsMatrix(r, subgroupsMatrix);
		teachers_conflicts = c.getTeachersMatrix(r, teachersMatrix);

		//crt_chrom=&c;
		//crt_rules=&r;
		
		c.changedForMatrixCalculation=false;
	}

	int nbroken;

	//without logging
	if(conflictsString==NULL){
		nbroken=0;
		//count sort
		int t=this->teacher_ID;
		int nd[MAX_HOURS_PER_DAY + 1];
		for(int h=0; h<=r.nHoursPerDay; h++)
			nd[h]=0;
		for(int d=0; d<r.nDaysPerWeek; d++){
			int nh=0;
			for(int h=0; h<r.nHoursPerDay; h++)
				nh += teachersMatrix[t][d][h]>=1 ? 1 : 0;
			nd[nh]++;
		}
		//return the minimum occupied days which do not respect this constraint
		int i = r.nDaysPerWeek - this->maxDaysPerWeek;
		for(int k=0; k<=r.nHoursPerDay; k++){
			if(nd[k]>0){
				if(i>nd[k]){
					i-=nd[k];
					nbroken+=nd[k]*k;
				}
				else{
					nbroken+=i*k;
					break;
				}
			}
		}
	}
	//with logging
	else{
		nbroken=0;
		//count sort
		int t=this->teacher_ID;
		int nd[MAX_HOURS_PER_DAY + 1];
		for(int h=0; h<=r.nHoursPerDay; h++)
			nd[h]=0;
		for(int d=0; d<r.nDaysPerWeek; d++){
			int nh=0;
			for(int h=0; h<r.nHoursPerDay; h++)
				nh += teachersMatrix[t][d][h]>=1 ? 1 : 0;
			nd[nh]++;
		}
		//return the minimum occupied days which do not respect this constraint
		int i = r.nDaysPerWeek - this->maxDaysPerWeek;
		for(int k=0; k<=r.nHoursPerDay; k++){
			if(nd[k]>0){
				if(i>nd[k]){
					i-=nd[k];
					nbroken+=nd[k]*k;
				}
				else{
					nbroken+=i*k;
					break;
				}
			}
		}

		if(nbroken>0){
			QString s= tr("Time constraint teacher max days per week broken for teacher: %1.")
			 .arg(r.internalTeachersList[t]->name);
			s += tr("This increases the conflicts total by %1")
			 .arg(nbroken*weightPercentage/100);
			 
			dl.append(s);
			cl.append(nbroken*weightPercentage/100);
		
			*conflictsString += s+"\n";
		}
	}

	if(weightPercentage==100)
		assert(nbroken==0);
	return weightPercentage/100 * nbroken;
}

bool ConstraintTeacherMaxDaysPerWeek::isRelatedToActivity(Rules& r, Activity* a)
{
	Q_UNUSED(r);
	Q_UNUSED(a);
	//if(a)
	//	;

	return false;
}

bool ConstraintTeacherMaxDaysPerWeek::isRelatedToTeacher(Teacher* t)
{
	if(this->teacherName==t->name)
		return true;
	return false;
}

bool ConstraintTeacherMaxDaysPerWeek::isRelatedToSubject(Subject* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintTeacherMaxDaysPerWeek::isRelatedToActivityTag(ActivityTag* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintTeacherMaxDaysPerWeek::isRelatedToStudentsSet(Rules& r, StudentsSet* s)
{
	Q_UNUSED(r);
	Q_UNUSED(s);
	/*if(s)
		;
	if(&r)
		;*/

	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

ConstraintTeachersMaxDaysPerWeek::ConstraintTeachersMaxDaysPerWeek()
	: TimeConstraint()
{
	this->type=CONSTRAINT_TEACHERS_MAX_DAYS_PER_WEEK;
}

ConstraintTeachersMaxDaysPerWeek::ConstraintTeachersMaxDaysPerWeek(double wp, int maxnd)
	 : TimeConstraint(wp)
{
	this->maxDaysPerWeek=maxnd;
	this->type=CONSTRAINT_TEACHERS_MAX_DAYS_PER_WEEK;
}

bool ConstraintTeachersMaxDaysPerWeek::computeInternalStructure(Rules& r)
{
	Q_UNUSED(r);
	return true;
}

bool ConstraintTeachersMaxDaysPerWeek::hasInactiveActivities(Rules& r)
{
	Q_UNUSED(r);
	return false;
}

QString ConstraintTeachersMaxDaysPerWeek::getXmlDescription(Rules& r)
{
	Q_UNUSED(r);

	QString s="<ConstraintTeachersMaxDaysPerWeek>\n";
	s+="	<Weight_Percentage>"+QString::number(this->weightPercentage)+"</Weight_Percentage>\n";
	//s+="	<Compulsory>";s+=yesNo(this->compulsory);s+="</Compulsory>\n";
	//s+="	<Teacher_Name>"+protect(this->teacherName)+"</Teacher_Name>\n";
	s+="	<Max_Days_Per_Week>"+QString::number(this->maxDaysPerWeek)+"</Max_Days_Per_Week>\n";
	s+="</ConstraintTeachersMaxDaysPerWeek>\n";
	return s;
}

QString ConstraintTeachersMaxDaysPerWeek::getDescription(Rules& r){
	Q_UNUSED(r);

	QString s=tr("Teachers max days per week");s+=", ";
	s+=tr("WP:%1\%", "Weight percentage").arg(this->weightPercentage);s+=", ";
	s+=tr("MD:%1", "Max days (per week)").arg(this->maxDaysPerWeek);

	return s;
}

QString ConstraintTeachersMaxDaysPerWeek::getDetailedDescription(Rules& r){
	Q_UNUSED(r);

	QString s=tr("Time constraint");s+="\n";
	s+=tr("All teachers must respect the maximum number of days per week");s+="\n";
	s+=tr("Weight (percentage)=%1\%").arg(this->weightPercentage);s+="\n";
	s+=tr("Maximum days per week=%1").arg(this->maxDaysPerWeek);s+="\n";

	return s;
}

double ConstraintTeachersMaxDaysPerWeek::fitness(Solution& c, Rules& r, QList<double>& cl, QList<QString>&dl, QString *conflictsString)
{
	//if the matrices subgroupsMatrix and teachersMatrix are already calculated, do not calculate them again!
	if(!c.teachersMatrixReady || !c.subgroupsMatrixReady){
		c.teachersMatrixReady=true;
		c.subgroupsMatrixReady=true;
	//if(crt_chrom!=&c || crt_rules!=&r || subgroups_conflicts<0 || teachers_conflicts<0 || c.changedForMatrixCalculation){
		subgroups_conflicts = c.getSubgroupsMatrix(r, subgroupsMatrix);
		teachers_conflicts = c.getTeachersMatrix(r, teachersMatrix);

		//crt_chrom=&c;
		//crt_rules=&r;
		
		c.changedForMatrixCalculation=false;
	}

	int nbroken;

	//without logging
	if(conflictsString==NULL){
		nbroken=0;
		//count sort
		
for(int t=0; t<r.nInternalTeachers; t++){
		
		//int t=this->teacher_ID;
		int nd[MAX_HOURS_PER_DAY + 1];
		for(int h=0; h<=r.nHoursPerDay; h++)
			nd[h]=0;
		for(int d=0; d<r.nDaysPerWeek; d++){
			int nh=0;
			for(int h=0; h<r.nHoursPerDay; h++)
				nh += teachersMatrix[t][d][h]>=1 ? 1 : 0;
			nd[nh]++;
		}
		//return the minimum occupied days which do not respect this constraint
		int i = r.nDaysPerWeek - this->maxDaysPerWeek;
		for(int k=0; k<=r.nHoursPerDay; k++){
			if(nd[k]>0){
				if(i>nd[k]){
					i-=nd[k];
					nbroken+=nd[k]*k;
				}
				else{
					nbroken+=i*k;
					break;
				}
			}
		}
		
}
	}
	//with logging
	else{
		nbroken=0;

for(int t=0; t<r.nInternalTeachers; t++){

		int nbr=0;

		//count sort
		//int t=this->teacher_ID;
		int nd[MAX_HOURS_PER_DAY + 1];
		for(int h=0; h<=r.nHoursPerDay; h++)
			nd[h]=0;
		for(int d=0; d<r.nDaysPerWeek; d++){
			int nh=0;
			for(int h=0; h<r.nHoursPerDay; h++)
				nh += teachersMatrix[t][d][h]>=1 ? 1 : 0;
			nd[nh]++;
		}
		//return the minimum occupied days which do not respect this constraint
		int i = r.nDaysPerWeek - this->maxDaysPerWeek;
		for(int k=0; k<=r.nHoursPerDay; k++){
			if(nd[k]>0){
				if(i>nd[k]){
					i-=nd[k];
					nbroken+=nd[k]*k;
					nbr+=nd[k]*k;
				}
				else{
					nbroken+=i*k;
					nbr+=i*k;
					break;
				}
			}
		}

		if(nbr>0){
			QString s= tr("Time constraint teachers max days per week broken for teacher: %1.")
			 .arg(r.internalTeachersList[t]->name);
			s += tr("This increases the conflicts total by %1")
			 .arg(nbr*weightPercentage/100);
			 
			dl.append(s);
			cl.append(nbr*weightPercentage/100);
		
			*conflictsString += s+"\n";
		}
		
}
		
	}

	if(weightPercentage==100)
		assert(nbroken==0);
	return weightPercentage/100 * nbroken;
}

bool ConstraintTeachersMaxDaysPerWeek::isRelatedToActivity(Rules& r, Activity* a)
{
	Q_UNUSED(r);
	Q_UNUSED(a);
	//if(a)
	//	;

	return false;
}

bool ConstraintTeachersMaxDaysPerWeek::isRelatedToTeacher(Teacher* t)
{
	Q_UNUSED(t);
	//if(this->teacherName==t->name)
	return true;
	//return false;
}

bool ConstraintTeachersMaxDaysPerWeek::isRelatedToSubject(Subject* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintTeachersMaxDaysPerWeek::isRelatedToActivityTag(ActivityTag* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintTeachersMaxDaysPerWeek::isRelatedToStudentsSet(Rules& r, StudentsSet* s)
{
	Q_UNUSED(r);
	Q_UNUSED(s);
	/*if(s)
		;
	if(&r)
		;*/

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

ConstraintTeachersMaxGapsPerWeek::ConstraintTeachersMaxGapsPerWeek()
	: TimeConstraint()
{
	this->type = CONSTRAINT_TEACHERS_MAX_GAPS_PER_WEEK;
}

ConstraintTeachersMaxGapsPerWeek::ConstraintTeachersMaxGapsPerWeek(double wp, int mg)
	: TimeConstraint(wp)
{
	this->type = CONSTRAINT_TEACHERS_MAX_GAPS_PER_WEEK;
	this->maxGaps=mg;
}

bool ConstraintTeachersMaxGapsPerWeek::computeInternalStructure(Rules& r)
{
	Q_UNUSED(r);
	/*if(&r!=NULL)
		;*/

	/*do nothing*/
	return true;
}

bool ConstraintTeachersMaxGapsPerWeek::hasInactiveActivities(Rules& r)
{
	Q_UNUSED(r);
	return false;
}

QString ConstraintTeachersMaxGapsPerWeek::getXmlDescription(Rules& r){
	//to avoid non-used parameter warning
	Q_UNUSED(r);

	QString s="<ConstraintTeachersMaxGapsPerWeek>\n";
	s+="	<Weight_Percentage>"+QString::number(this->weightPercentage)+"</Weight_Percentage>\n";
	s+="	<Max_Gaps>"+QString::number(this->maxGaps)+"</Max_Gaps>\n";
	//s+="	<Compulsory>";s+=yesNo(this->compulsory);s+="</Compulsory>\n";
	s+="</ConstraintTeachersMaxGapsPerWeek>\n";
	return s;
}

QString ConstraintTeachersMaxGapsPerWeek::getDescription(Rules& r){
	//to avoid non-used parameter warning
	Q_UNUSED(r);

	QString s;
	s+=tr("Teachers max gaps per week");s+=", ";
	s+=tr("WP:%1\%", "Weight percentage").arg(this->weightPercentage);s+=", ";
	s+=tr("MG:%1", "Max gaps (per week)").arg(this->maxGaps);

	return s;
}

QString ConstraintTeachersMaxGapsPerWeek::getDetailedDescription(Rules& r){
	//to avoid non-used parameter warning
	Q_UNUSED(r);

	QString s=tr("Time constraint");s+="\n";
	s+=tr("All teachers must respect the maximum number of gaps per week");s+="\n";
	s+=tr("(breaks and teacher not available not counted)");s+="\n";
	s+=tr("Maximum gaps per week=%1").arg(this->maxGaps); s+="\n";
	s+=tr("Weight (percentage)=%1\%").arg(this->weightPercentage);s+="\n";

	return s;
}

double ConstraintTeachersMaxGapsPerWeek::fitness(Solution& c, Rules& r, QList<double>& cl, QList<QString>&dl, QString* conflictsString)
{ 
	//if the matrices subgroupsMatrix and teachersMatrix are already calculated, do not calculate them again!
	if(!c.teachersMatrixReady || !c.subgroupsMatrixReady){
		c.teachersMatrixReady=true;
		c.subgroupsMatrixReady=true;
	//if(crt_chrom!=&c || crt_rules!=&r || subgroups_conflicts<0 || teachers_conflicts<0 || c.changedForMatrixCalculation){
		subgroups_conflicts = c.getSubgroupsMatrix(r, subgroupsMatrix);
		teachers_conflicts = c.getTeachersMatrix(r, teachersMatrix);

		//crt_chrom=&c;
		//crt_rules=&r;
		
		c.changedForMatrixCalculation=false;
	}
	
	int tg;
	int i, j, k;
	int totalGaps;

	totalGaps=0;
	for(i=0; i<r.nInternalTeachers; i++){
		tg=0;
		for(j=0; j<r.nDaysPerWeek; j++){
			for(k=0; k<r.nHoursPerDay; k++)
				if(teachersMatrix[i][j][k]>0){
					assert(!breakDayHour[j][k] && !teacherNotAvailableDayHour[i][j][k]);
					break;
				}

			int cnt=0;
			for(; k<r.nHoursPerDay; k++) if(!breakDayHour[j][k] && !teacherNotAvailableDayHour[i][j][k]){
				if(teachersMatrix[i][j][k]>0){
					tg+=cnt;
					cnt=0;
				}
				else
					cnt++;
			}
		}
		if(tg>this->maxGaps){
			totalGaps+=tg-maxGaps;
			//assert(this->weightPercentage<100); partial solutions might break this rule
			if(conflictsString!=NULL){
				QString s=tr("Time constraint teachers max gaps per week broken for teacher: %1, conflicts factor increase=%2")
					.arg(r.internalTeachersList[i]->name)
					.arg((tg-maxGaps)*weightPercentage/100);
					
				*conflictsString+= s+"\n";
						
				dl.append(s);
				cl.append((tg-maxGaps)*weightPercentage/100);
			}
		}
	}
	
	/*int na=0;
	for(int i=0; i<r.nInternalActivities; i++)
		if(c.times[i]!=UNALLOCATED_TIME)
			na++;*/

	if(c.nPlacedActivities==r.nInternalActivities)
	//if(na==r.nInternalActivities)
		if(weightPercentage==100){
		
			/*if(totalGaps>0){
				cout<<"c.nPlacedActivities=="<<c.nPlacedActivities<<endl;
				
				int k=0;
				for(int j=0; j<r.nInternalActivities; j++){
					cout<<"j=="<<j<<", c.times[j]=="<<c.times[j]<<endl;
					if(c.times[j]!=UNALLOCATED_TIME)
						k++;
				}
				
				cout<<"sum(allocated activities)=="<<k<<endl;
			}*/
				
			assert(totalGaps==0); //for partial solutions this rule might be broken
		}
	return weightPercentage/100 * totalGaps;
}

bool ConstraintTeachersMaxGapsPerWeek::isRelatedToActivity(Rules& r, Activity* a)
{
	Q_UNUSED(r);
	Q_UNUSED(a);
	//if(a)
	//	;

	return false;
}

bool ConstraintTeachersMaxGapsPerWeek::isRelatedToTeacher(Teacher* t)
{
	Q_UNUSED(t);
	//if(t)
	//	;

	return true;
}

bool ConstraintTeachersMaxGapsPerWeek::isRelatedToSubject(Subject* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintTeachersMaxGapsPerWeek::isRelatedToActivityTag(ActivityTag* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintTeachersMaxGapsPerWeek::isRelatedToStudentsSet(Rules& r, StudentsSet* s)
{
	Q_UNUSED(r);
	Q_UNUSED(s);
	/*if(s)
		;
	if(&r)
		;*/

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

ConstraintTeacherMaxGapsPerWeek::ConstraintTeacherMaxGapsPerWeek()
	: TimeConstraint()
{
	this->type = CONSTRAINT_TEACHER_MAX_GAPS_PER_WEEK;
}

ConstraintTeacherMaxGapsPerWeek::ConstraintTeacherMaxGapsPerWeek(double wp, QString tn, int mg)
	: TimeConstraint(wp)
{
	this->type = CONSTRAINT_TEACHER_MAX_GAPS_PER_WEEK;
	this->teacherName=tn;
	this->maxGaps=mg;
}

bool ConstraintTeacherMaxGapsPerWeek::computeInternalStructure(Rules& r)
{
	this->teacherIndex=r.searchTeacher(this->teacherName);
	assert(this->teacherIndex>=0);
	return true;
}

bool ConstraintTeacherMaxGapsPerWeek::hasInactiveActivities(Rules& r)
{
	Q_UNUSED(r);
	return false;
}

QString ConstraintTeacherMaxGapsPerWeek::getXmlDescription(Rules& r){
	//to avoid non-used parameter warning
	Q_UNUSED(r);

	QString s="<ConstraintTeacherMaxGapsPerWeek>\n";
	s+="	<Weight_Percentage>"+QString::number(this->weightPercentage)+"</Weight_Percentage>\n";
	s+="	<Teacher_Name>"+protect(this->teacherName)+"</Teacher_Name>\n";
	s+="	<Max_Gaps>"+QString::number(this->maxGaps)+"</Max_Gaps>\n";
	//s+="	<Compulsory>";s+=yesNo(this->compulsory);s+="</Compulsory>\n";
	s+="</ConstraintTeacherMaxGapsPerWeek>\n";
	return s;
}

QString ConstraintTeacherMaxGapsPerWeek::getDescription(Rules& r){
	//to avoid non-used parameter warning
	Q_UNUSED(r);

	QString s;
	s+=tr("Teacher max gaps per week");s+=", ";
	s+=tr("WP:%1\%", "Weight percentage").arg(this->weightPercentage);s+=", ";
	s+=tr("T:%1", "Teacher").arg(this->teacherName); s+=", ";
	s+=tr("MG:%1", "Max gaps (per week").arg(this->maxGaps);

	return s;
}

QString ConstraintTeacherMaxGapsPerWeek::getDetailedDescription(Rules& r){
	//to avoid non-used parameter warning
	Q_UNUSED(r);

	QString s=tr("Time constraint"); s+="\n";
	s+=tr("A teacher must respect the maximum number of gaps per week"); s+="\n";
	s+=tr("(breaks and teacher not available not counted)");s+="\n";
	s+=tr("Weight (percentage)=%1\%").arg(this->weightPercentage); s+="\n";
	s+=tr("Teacher=%1").arg(this->teacherName); s+="\n";
	s+=tr("Maximum gaps per week=%1").arg(this->maxGaps); s+="\n";

	return s;
}

double ConstraintTeacherMaxGapsPerWeek::fitness(Solution& c, Rules& r, QList<double>& cl, QList<QString>&dl, QString* conflictsString)
{ 
	//if the matrices subgroupsMatrix and teachersMatrix are already calculated, do not calculate them again!
	if(!c.teachersMatrixReady || !c.subgroupsMatrixReady){
		c.teachersMatrixReady=true;
		c.subgroupsMatrixReady=true;
	//if(crt_chrom!=&c || crt_rules!=&r || subgroups_conflicts<0 || teachers_conflicts<0 || c.changedForMatrixCalculation){
		subgroups_conflicts = c.getSubgroupsMatrix(r, subgroupsMatrix);
		teachers_conflicts = c.getTeachersMatrix(r, teachersMatrix);

		//crt_chrom=&c;
		//crt_rules=&r;
		
		c.changedForMatrixCalculation=false;
	}
	
	int tg;
	int i, j, k;
	int totalGaps;

	totalGaps=0;
		
	i=this->teacherIndex;
	
	tg=0;
	for(j=0; j<r.nDaysPerWeek; j++){
		for(k=0; k<r.nHoursPerDay; k++)
			if(teachersMatrix[i][j][k]>0){
				assert(!breakDayHour[j][k] && !teacherNotAvailableDayHour[i][j][k]);
				break;
			}

		int cnt=0;
		for(; k<r.nHoursPerDay; k++) if(!breakDayHour[j][k] && !teacherNotAvailableDayHour[i][j][k]){
			if(teachersMatrix[i][j][k]>0){
				tg+=cnt;
				cnt=0;
			}
			else
				cnt++;
		}
	}
	if(tg>this->maxGaps){
		totalGaps+=tg-maxGaps;
		//assert(this->weightPercentage<100); partial solutions might break this rule
		if(conflictsString!=NULL){
			QString s=tr("Time constraint teacher max gaps per week broken for teacher: %1, conflicts factor increase=%2")
				.arg(r.internalTeachersList[i]->name)
				.arg((tg-maxGaps)*weightPercentage/100);
					
			*conflictsString+= s+"\n";
						
			dl.append(s);
			cl.append((tg-maxGaps)*weightPercentage/100);
		}
	}

	/*int na=0;
	for(int i=0; i<r.nInternalActivities; i++)
		if(c.times[i]!=UNALLOCATED_TIME)
			na++;*/
	if(c.nPlacedActivities==r.nInternalActivities)
	//if(na==r.nInternalActivities)
		if(weightPercentage==100)
			assert(totalGaps==0); //for partial solutions this rule might be broken
	return weightPercentage/100 * totalGaps;
}

bool ConstraintTeacherMaxGapsPerWeek::isRelatedToActivity(Rules& r, Activity* a)
{
	Q_UNUSED(r);
	Q_UNUSED(a);
	//if(a)
	//	;

	return false;
}

bool ConstraintTeacherMaxGapsPerWeek::isRelatedToTeacher(Teacher* t)
{
	if(this->teacherName==t->name)
		return true;
	return false;
}

bool ConstraintTeacherMaxGapsPerWeek::isRelatedToSubject(Subject* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintTeacherMaxGapsPerWeek::isRelatedToActivityTag(ActivityTag* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintTeacherMaxGapsPerWeek::isRelatedToStudentsSet(Rules& r, StudentsSet* s)
{
	Q_UNUSED(r);
	Q_UNUSED(s);
	/*if(s)
		;
	if(&r)
		;*/

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

ConstraintTeachersMaxGapsPerDay::ConstraintTeachersMaxGapsPerDay()
	: TimeConstraint()
{
	this->type = CONSTRAINT_TEACHERS_MAX_GAPS_PER_DAY;
}

ConstraintTeachersMaxGapsPerDay::ConstraintTeachersMaxGapsPerDay(double wp, int mg)
	: TimeConstraint(wp)
{
	this->type = CONSTRAINT_TEACHERS_MAX_GAPS_PER_DAY;
	this->maxGaps=mg;
}

bool ConstraintTeachersMaxGapsPerDay::computeInternalStructure(Rules& r)
{
	Q_UNUSED(r);
	/*if(&r!=NULL)
		;*/

	/*do nothing*/
	return true;
}

bool ConstraintTeachersMaxGapsPerDay::hasInactiveActivities(Rules& r)
{
	Q_UNUSED(r);
	return false;
}

QString ConstraintTeachersMaxGapsPerDay::getXmlDescription(Rules& r){
	//to avoid non-used parameter warning
	Q_UNUSED(r);

	QString s="<ConstraintTeachersMaxGapsPerDay>\n";
	s+="	<Weight_Percentage>"+QString::number(this->weightPercentage)+"</Weight_Percentage>\n";
	s+="	<Max_Gaps>"+QString::number(this->maxGaps)+"</Max_Gaps>\n";
	//s+="	<Compulsory>";s+=yesNo(this->compulsory);s+="</Compulsory>\n";
	s+="</ConstraintTeachersMaxGapsPerDay>\n";
	return s;
}

QString ConstraintTeachersMaxGapsPerDay::getDescription(Rules& r){
	//to avoid non-used parameter warning
	Q_UNUSED(r);

	QString s;
	s+=tr("Teachers max gaps per day");s+=", ";
	s+=tr("WP:%1\%", "Weight percentage").arg(this->weightPercentage);s+=", ";
	s+=tr("MG:%1", "Max gaps (per day)").arg(this->maxGaps);

	return s;
}

QString ConstraintTeachersMaxGapsPerDay::getDetailedDescription(Rules& r){
	//to avoid non-used parameter warning
	Q_UNUSED(r);

	QString s=tr("Time constraint");s+="\n";
	s+=tr("All teachers must respect the maximum gaps per day");s+="\n";
	s+=tr("(breaks and teacher not available not counted)");s+="\n";
	s+=tr("Weight (percentage)=%1\%").arg(this->weightPercentage);s+="\n";
	s+=tr("Maximum gaps per day=%1").arg(this->maxGaps); s+="\n";

	return s;
}

double ConstraintTeachersMaxGapsPerDay::fitness(Solution& c, Rules& r, QList<double>& cl, QList<QString>&dl, QString* conflictsString)
{ 
	//if the matrices subgroupsMatrix and teachersMatrix are already calculated, do not calculate them again!
	if(!c.teachersMatrixReady || !c.subgroupsMatrixReady){
		c.teachersMatrixReady=true;
		c.subgroupsMatrixReady=true;
	//if(crt_chrom!=&c || crt_rules!=&r || subgroups_conflicts<0 || teachers_conflicts<0 || c.changedForMatrixCalculation){
		subgroups_conflicts = c.getSubgroupsMatrix(r, subgroupsMatrix);
		teachers_conflicts = c.getTeachersMatrix(r, teachersMatrix);

		//crt_chrom=&c;
		//crt_rules=&r;
		
		c.changedForMatrixCalculation=false;
	}
	
	int tg;
	int i, j, k;
	int totalGaps;

	totalGaps=0;
	for(i=0; i<r.nInternalTeachers; i++){
		for(j=0; j<r.nDaysPerWeek; j++){
			tg=0;
			for(k=0; k<r.nHoursPerDay; k++)
				if(teachersMatrix[i][j][k]>0){
					assert(!breakDayHour[j][k] && !teacherNotAvailableDayHour[i][j][k]);
					break;
				}

			int cnt=0;
			for(; k<r.nHoursPerDay; k++) if(!breakDayHour[j][k] && !teacherNotAvailableDayHour[i][j][k]){
				if(teachersMatrix[i][j][k]>0){
					tg+=cnt;
					cnt=0;
				}
				else
					cnt++;
			}
			if(tg>this->maxGaps){
				totalGaps+=tg-maxGaps;
				//assert(this->weightPercentage<100); partial solutions might break this rule
				if(conflictsString!=NULL){
					QString s=tr("Time constraint teachers max gaps per day broken for teacher: %1, day: %2, conflicts factor increase=%3")
						.arg(r.internalTeachersList[i]->name)
						.arg(r.daysOfTheWeek[j])
						.arg((tg-maxGaps)*weightPercentage/100);
					
					*conflictsString+= s+"\n";
								
					dl.append(s);
					cl.append((tg-maxGaps)*weightPercentage/100);
				}
			}
		}
	}
	
	if(c.nPlacedActivities==r.nInternalActivities)
		if(weightPercentage==100)
			assert(totalGaps==0); //for partial solutions this rule might be broken
	return weightPercentage/100 * totalGaps;
}

bool ConstraintTeachersMaxGapsPerDay::isRelatedToActivity(Rules& r, Activity* a)
{
	Q_UNUSED(r);
	Q_UNUSED(a);
	//if(a)
	//	;

	return false;
}

bool ConstraintTeachersMaxGapsPerDay::isRelatedToTeacher(Teacher* t)
{	
	Q_UNUSED(t);
	//if(t)
	//	;

	return true;
}

bool ConstraintTeachersMaxGapsPerDay::isRelatedToSubject(Subject* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintTeachersMaxGapsPerDay::isRelatedToActivityTag(ActivityTag* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintTeachersMaxGapsPerDay::isRelatedToStudentsSet(Rules& r, StudentsSet* s)
{
	Q_UNUSED(r);
	Q_UNUSED(s);
	/*if(s)
		;
	if(&r)
		;*/

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

ConstraintTeacherMaxGapsPerDay::ConstraintTeacherMaxGapsPerDay()
	: TimeConstraint()
{
	this->type = CONSTRAINT_TEACHER_MAX_GAPS_PER_DAY;
}

ConstraintTeacherMaxGapsPerDay::ConstraintTeacherMaxGapsPerDay(double wp, QString tn, int mg)
	: TimeConstraint(wp)
{
	this->type = CONSTRAINT_TEACHER_MAX_GAPS_PER_DAY;
	this->teacherName=tn;
	this->maxGaps=mg;
}

bool ConstraintTeacherMaxGapsPerDay::computeInternalStructure(Rules& r)
{
	this->teacherIndex=r.searchTeacher(this->teacherName);
	assert(this->teacherIndex>=0);
	return true;
}

bool ConstraintTeacherMaxGapsPerDay::hasInactiveActivities(Rules& r)
{
	Q_UNUSED(r);
	return false;
}

QString ConstraintTeacherMaxGapsPerDay::getXmlDescription(Rules& r){
	//to avoid non-used parameter warning
	Q_UNUSED(r);

	QString s="<ConstraintTeacherMaxGapsPerDay>\n";
	s+="	<Weight_Percentage>"+QString::number(this->weightPercentage)+"</Weight_Percentage>\n";
	s+="	<Teacher_Name>"+protect(this->teacherName)+"</Teacher_Name>\n";
	s+="	<Max_Gaps>"+QString::number(this->maxGaps)+"</Max_Gaps>\n";
	//s+="	<Compulsory>";s+=yesNo(this->compulsory);s+="</Compulsory>\n";
	s+="</ConstraintTeacherMaxGapsPerDay>\n";
	return s;
}

QString ConstraintTeacherMaxGapsPerDay::getDescription(Rules& r){
	//to avoid non-used parameter warning
	Q_UNUSED(r);

	QString s;
	s+=tr("Teacher max gaps per day");s+=", ";
	s+=tr("WP:%1\%", "Weight percentage").arg(this->weightPercentage);s+=", ";
	s+=tr("T:%1", "Teacher").arg(this->teacherName); s+=", ";
	s+=tr("MG:%1", "Max gaps (per day)").arg(this->maxGaps);

	return s;
}

QString ConstraintTeacherMaxGapsPerDay::getDetailedDescription(Rules& r){
	//to avoid non-used parameter warning
	Q_UNUSED(r);

	QString s=tr("Time constraint"); s+="\n";
	s+=tr("A teacher must respect the maximum number of gaps per day"); s+="\n";
	s+=tr("(breaks and teacher not available not counted)");s+="\n";
	s+=tr("Weight (percentage)=%1\%").arg(this->weightPercentage); s+="\n";
	s+=tr("Teacher=%1").arg(this->teacherName); s+="\n";
	s+=tr("Maximum gaps per day=%1").arg(this->maxGaps); s+="\n";

	return s;
}

double ConstraintTeacherMaxGapsPerDay::fitness(Solution& c, Rules& r, QList<double>& cl, QList<QString>&dl, QString* conflictsString)
{ 
	//if the matrices subgroupsMatrix and teachersMatrix are already calculated, do not calculate them again!
	if(!c.teachersMatrixReady || !c.subgroupsMatrixReady){
		c.teachersMatrixReady=true;
		c.subgroupsMatrixReady=true;
	//if(crt_chrom!=&c || crt_rules!=&r || subgroups_conflicts<0 || teachers_conflicts<0 || c.changedForMatrixCalculation){
		subgroups_conflicts = c.getSubgroupsMatrix(r, subgroupsMatrix);
		teachers_conflicts = c.getTeachersMatrix(r, teachersMatrix);

		//crt_chrom=&c;
		//crt_rules=&r;
		
		c.changedForMatrixCalculation=false;
	}
	
	int tg;
	int i, j, k;
	int totalGaps;

	totalGaps=0;
		
	i=this->teacherIndex;
	
	for(j=0; j<r.nDaysPerWeek; j++){
		tg=0;
		for(k=0; k<r.nHoursPerDay; k++)
			if(teachersMatrix[i][j][k]>0){
				assert(!breakDayHour[j][k] && !teacherNotAvailableDayHour[i][j][k]);
				break;
			}

		int cnt=0;
		for(; k<r.nHoursPerDay; k++) if(!breakDayHour[j][k] && !teacherNotAvailableDayHour[i][j][k]){
			if(teachersMatrix[i][j][k]>0){
				tg+=cnt;
				cnt=0;
			}
			else
				cnt++;
		}
		if(tg>this->maxGaps){
			totalGaps+=tg-maxGaps;
			//assert(this->weightPercentage<100); partial solutions might break this rule
			if(conflictsString!=NULL){
				QString s=tr("Time constraint teacher max gaps per day broken for teacher: %1, day: %2, conflicts factor increase=%3")
					.arg(r.internalTeachersList[i]->name)
					.arg(r.daysOfTheWeek[j])
					.arg((tg-maxGaps)*weightPercentage/100);
						
				*conflictsString+= s+"\n";
							
				dl.append(s);
				cl.append((tg-maxGaps)*weightPercentage/100);
			}
		}
	}

	if(c.nPlacedActivities==r.nInternalActivities)
		if(weightPercentage==100)
			assert(totalGaps==0); //for partial solutions this rule might be broken
	return weightPercentage/100 * totalGaps;
}

bool ConstraintTeacherMaxGapsPerDay::isRelatedToActivity(Rules& r, Activity* a)
{
	Q_UNUSED(r);
	Q_UNUSED(a);
	//if(a)
	//	;

	return false;
}

bool ConstraintTeacherMaxGapsPerDay::isRelatedToTeacher(Teacher* t)
{
	if(this->teacherName==t->name)
		return true;
	return false;
}

bool ConstraintTeacherMaxGapsPerDay::isRelatedToSubject(Subject* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintTeacherMaxGapsPerDay::isRelatedToActivityTag(ActivityTag* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintTeacherMaxGapsPerDay::isRelatedToStudentsSet(Rules& r, StudentsSet* s)
{
	Q_UNUSED(r);
	Q_UNUSED(s);
	/*if(s)
		;
	if(&r)
		;*/

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

ConstraintBreakTimes::ConstraintBreakTimes()
	: TimeConstraint()
{
	this->type = CONSTRAINT_BREAK_TIMES;
}

ConstraintBreakTimes::ConstraintBreakTimes(double wp, QList<int> d, QList<int> h)
	: TimeConstraint(wp)
{
	this->days = d;
	this->hours = h;
	this->type = CONSTRAINT_BREAK_TIMES;
}

bool ConstraintBreakTimes::hasInactiveActivities(Rules& r)
{
	Q_UNUSED(r);
	return false;
}

QString ConstraintBreakTimes::getXmlDescription(Rules& r){
	QString s="<ConstraintBreakTimes>\n";
	s+="	<Weight_Percentage>"+QString::number(this->weightPercentage)+"</Weight_Percentage>\n";

	s+="	<Number_of_Break_Times>"+QString::number(this->days.count())+"</Number_of_Break_Times>\n";
	assert(days.count()==hours.count());
	for(int i=0; i<days.count(); i++){
		s+="	<Break_Time>\n";
		if(this->days.at(i)>=0)
			s+="		<Day>"+protect(r.daysOfTheWeek[this->days.at(i)])+"</Day>\n";
		if(this->hours.at(i)>=0)
			s+="		<Hour>"+protect(r.hoursOfTheDay[this->hours.at(i)])+"</Hour>\n";
		s+="	</Break_Time>\n";
	}

	s+="</ConstraintBreakTimes>\n";
	return s;
}

QString ConstraintBreakTimes::getDescription(Rules& r){
	QString s;
	s+=tr("Break times");s+=", ";
	s+=tr("WP:%1\%", "Weight percentage").arg(this->weightPercentage);s+=", ";

	s+=tr("B at:", "Break at");
	s+=" ";
	assert(days.count()==hours.count());
	for(int i=0; i<days.count(); i++){
		if(this->days.at(i)>=0){
			s+=r.daysOfTheWeek[this->days.at(i)];
			s+=" ";
		}
		if(this->hours.at(i)>=0){
			s+=r.hoursOfTheDay[this->hours.at(i)];
		}
		if(i<days.count()-1)
			s+="; ";
	}
	
	return s;
}

QString ConstraintBreakTimes::getDetailedDescription(Rules& r){
	QString s=tr("Time constraint");s+="\n";
	s+=tr("Break times");s+="\n";
	s+=tr("Weight (percentage)=%1\%").arg(this->weightPercentage);s+="\n";

	s+=tr("Break at:"); s+="\n";
	assert(days.count()==hours.count());
	for(int i=0; i<days.count(); i++){
		if(this->days.at(i)>=0){
			s+=r.daysOfTheWeek[this->days.at(i)];
			s+=" ";
		}
		if(this->hours.at(i)>=0){
			s+=r.hoursOfTheDay[this->hours.at(i)];
		}
		if(i<days.count()-1)
			s+="; ";
	}
	
	return s;
}

bool ConstraintBreakTimes::computeInternalStructure(Rules& r)
{
	Q_UNUSED(r);
	
	assert(days.count()==hours.count());
	for(int k=0; k<days.count(); k++){
		if(this->days.at(k) >= r.nDaysPerWeek){
			QMessageBox::information(NULL, tr("FET information"),
			 tr("Constraint break times is wrong because it refers to removed day. Please correct"
			 " and try again. Correcting means editing the constraint and updating information. Constraint is:\n%1").arg(this->getDetailedDescription(r)));
		 
			return false;
		}		
		if(this->hours.at(k) >= r.nHoursPerDay){
			QMessageBox::information(NULL, tr("FET information"),
			 tr("Constraint break times is wrong because an hour is too late (after the last acceptable slot). Please correct"
			 " and try again. Correcting means editing the constraint and updating information. Constraint is:\n%1").arg(this->getDetailedDescription(r)));
		 
			return false;
		}
	}

	return true;
}

double ConstraintBreakTimes::fitness(Solution& c, Rules& r, QList<double>& cl, QList<QString>&dl, QString* conflictsString)
{
	//if the matrices subgroupsMatrix and teachersMatrix are already calculated, do not calculate them again!
	if(!c.teachersMatrixReady || !c.subgroupsMatrixReady){
		c.teachersMatrixReady=true;
		c.subgroupsMatrixReady=true;
	//if(crt_chrom!=&c || crt_rules!=&r || subgroups_conflicts<0 || teachers_conflicts<0 || c.changedForMatrixCalculation){
		subgroups_conflicts = c.getSubgroupsMatrix(r, subgroupsMatrix);
		teachers_conflicts = c.getTeachersMatrix(r, teachersMatrix);

		//crt_chrom=&c;
		//crt_rules=&r;
		
		c.changedForMatrixCalculation=false;
	}

	//DEPRECATED COMMENT
	//For the moment, this function sums the number of hours each teacher
	//is teaching in this break period.
	//This function consideres all the hours, I mean if there are for example 5 weekly courses
	//scheduled on that hour (which is already a broken hard restriction - we only
	//are allowed 1 weekly course for a certain teacher at a certain hour) we calculate
	//5 broken restrictions for this break period.
	//TODO: decide if it is better to consider only 2 or 10 as a return value in this particular case
	//(currently it is 10)
	
	//int j=this->d;
	int nbroken;
	
	nbroken=0;
		
	for(int i=0; i<r.nInternalActivities; i++){
		int dayact=c.times[i]%r.nDaysPerWeek;
		int houract=c.times[i]/r.nDaysPerWeek;
		
		assert(days.count()==hours.count());
		for(int kk=0; kk<days.count(); kk++){
			int d=days.at(kk);
			int h=hours.at(kk);
			
			int dur=r.internalActivitiesList[i].duration;
			if(d==dayact && !(houract+dur<=h || houract>h))
			{			
				nbroken++;

				if(conflictsString!=NULL){
					QString s=tr("Time constraint break not respected for activity with id %1, on day %2, hours %3")
						.arg(r.internalActivitiesList[i].id)
						.arg(r.daysOfTheWeek[dayact])
						.arg(r.daysOfTheWeek[houract]);
					s+=". ";
					s+=tr("This increases the conflicts total by %1").arg(weightPercentage/100);
					
					dl.append(s);
					cl.append(weightPercentage/100);
				
					*conflictsString+= s+"\n";
				}
			}
		}
	}

	if(weightPercentage==100)
		assert(nbroken==0);
	return weightPercentage/100 * nbroken;
}

bool ConstraintBreakTimes::isRelatedToActivity(Rules& r, Activity* a)
{
	Q_UNUSED(r);
	Q_UNUSED(a);

	return false;
}

bool ConstraintBreakTimes::isRelatedToTeacher(Teacher* t)
{
	Q_UNUSED(t);

	return false;
}

bool ConstraintBreakTimes::isRelatedToSubject(Subject* s)
{
	Q_UNUSED(s);

	return false;
}

bool ConstraintBreakTimes::isRelatedToActivityTag(ActivityTag* s)
{
	Q_UNUSED(s);

	return false;
}

bool ConstraintBreakTimes::isRelatedToStudentsSet(Rules& r, StudentsSet* s)
{
	Q_UNUSED(r);
	Q_UNUSED(s);

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

ConstraintStudentsMaxGapsPerWeek::ConstraintStudentsMaxGapsPerWeek()
	: TimeConstraint()
{
	this->type = CONSTRAINT_STUDENTS_MAX_GAPS_PER_WEEK;
}

ConstraintStudentsMaxGapsPerWeek::ConstraintStudentsMaxGapsPerWeek(double wp, int mg)
	: TimeConstraint(wp)
{
	this->type = CONSTRAINT_STUDENTS_MAX_GAPS_PER_WEEK;
	this->maxGaps=mg;
}

bool ConstraintStudentsMaxGapsPerWeek::computeInternalStructure(Rules& r)
{
	Q_UNUSED(r);
	//if(&r!=NULL)
	//	;

	/*do nothing*/
	return true;
}

bool ConstraintStudentsMaxGapsPerWeek::hasInactiveActivities(Rules& r)
{
	Q_UNUSED(r);
	return false;
}

QString ConstraintStudentsMaxGapsPerWeek::getXmlDescription(Rules& r)
{
	//to avoid non-used parameter warning
	Q_UNUSED(r);
	//if(&r==NULL)
	//	;

	QString s="<ConstraintStudentsMaxGapsPerWeek>\n";
	s+="	<Weight_Percentage>"+QString::number(this->weightPercentage)+"</Weight_Percentage>\n";
	s+="	<Max_Gaps>"+QString::number(this->maxGaps)+"</Max_Gaps>\n";
	s+="</ConstraintStudentsMaxGapsPerWeek>\n";
	return s;
}

QString ConstraintStudentsMaxGapsPerWeek::getDescription(Rules& r)
{
	//to avoid non-used parameter warning
	Q_UNUSED(r);

	QString s;
	s+=tr("Students max gaps per week");s+=", ";
	s+=tr("WP:%1\%", "Weight percentage").arg(this->weightPercentage);s+=", ";
	s+=tr("MG:%1", "Max gaps (per week)").arg(this->maxGaps);

	return s;
}

QString ConstraintStudentsMaxGapsPerWeek::getDetailedDescription(Rules& r)
{
	//to avoid non-used parameter warning
	Q_UNUSED(r);

	QString s=tr("Time constraint");s+="\n";
	s+=tr("All students must respect the maximum number of gaps per week");s+="\n";
	s+=tr("(breaks and students set not available not counted)");s+="\n";
	s+=tr("Weight (percentage)=%1\%").arg(this->weightPercentage);s+="\n";
	s+=tr("Maximum gaps per week=%1").arg(this->maxGaps);s+="\n";
	s+="\n";

	return s;
}

double ConstraintStudentsMaxGapsPerWeek::fitness(Solution& c, Rules& r, QList<double>& cl, QList<QString>&dl, QString* conflictsString)
{
	//returns a number equal to the number of windows of the subgroups (in hours)

	//if the matrices subgroupsMatrix and teachersMatrix are already calculated, do not calculate them again!
	if(!c.teachersMatrixReady || !c.subgroupsMatrixReady){
		c.teachersMatrixReady=true;
		c.subgroupsMatrixReady=true;
	//if(crt_chrom!=&c || crt_rules!=&r || subgroups_conflicts<0 || teachers_conflicts<0 || c.changedForMatrixCalculation){
		subgroups_conflicts = c.getSubgroupsMatrix(r, subgroupsMatrix);
		teachers_conflicts = c.getTeachersMatrix(r, teachersMatrix);

		//crt_chrom=&c;
		//crt_rules=&r;
		
		c.changedForMatrixCalculation=false;
	}

	int windows;
	int tmp;
	int i;
	
	int tIllegalWindows=0;

	for(i=0; i<r.nInternalSubgroups; i++){
		windows=0;
		for(int j=0; j<r.nDaysPerWeek; j++){
			int k;
			tmp=0;
			for(k=0; k<r.nHoursPerDay; k++)
				if(subgroupsMatrix[i][j][k]>0){
					assert(!breakDayHour[j][k] && !subgroupNotAvailableDayHour[i][j][k]);
					break;
				}
			for(; k<r.nHoursPerDay; k++) if(!breakDayHour[j][k] && !subgroupNotAvailableDayHour[i][j][k]){
				if(subgroupsMatrix[i][j][k]>0){
					windows+=tmp;
					tmp=0;
				}
				else
					tmp++;
			}
		}
		
		int illegalWindows=windows-this->maxGaps;
		if(illegalWindows<0)
			illegalWindows=0;

		if(illegalWindows>0 && conflictsString!=NULL){
			QString s=tr("Time constraint students max gaps per week broken for subgroup: %1, it has %2 extra gaps, conflicts increase=%3")
			 .arg(r.internalSubgroupsList[i]->name)
			 .arg(illegalWindows)
			 .arg(illegalWindows*weightPercentage/100);
						 
			dl.append(s);
			cl.append(illegalWindows*weightPercentage/100);
					
			*conflictsString+= s+"\n";
		}
		
		tIllegalWindows+=illegalWindows;
	}
		
	if(c.nPlacedActivities==r.nInternalActivities)
		if(weightPercentage==100)    //for partial solutions it might be broken
			assert(tIllegalWindows==0);
	return weightPercentage/100 * tIllegalWindows;
}

bool ConstraintStudentsMaxGapsPerWeek::isRelatedToActivity(Rules& r, Activity* a)
{
	Q_UNUSED(r);
	Q_UNUSED(a);

	return false;
}

bool ConstraintStudentsMaxGapsPerWeek::isRelatedToTeacher(Teacher* t)
{
	Q_UNUSED(t);

	return false;
}

bool ConstraintStudentsMaxGapsPerWeek::isRelatedToSubject(Subject* s)
{
	Q_UNUSED(s);

	return false;
}

bool ConstraintStudentsMaxGapsPerWeek::isRelatedToActivityTag(ActivityTag* s)
{
	Q_UNUSED(s);

	return false;
}

bool ConstraintStudentsMaxGapsPerWeek::isRelatedToStudentsSet(Rules& r, StudentsSet* s)
{
	Q_UNUSED(r);
	Q_UNUSED(s);

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

ConstraintStudentsSetMaxGapsPerWeek::ConstraintStudentsSetMaxGapsPerWeek()
	: TimeConstraint()
{
	this->type = CONSTRAINT_STUDENTS_SET_MAX_GAPS_PER_WEEK;
}

ConstraintStudentsSetMaxGapsPerWeek::ConstraintStudentsSetMaxGapsPerWeek(double wp, int mg, const QString& st )
	: TimeConstraint(wp)
{
	this->type = CONSTRAINT_STUDENTS_SET_MAX_GAPS_PER_WEEK;
	this->maxGaps=mg;
	this->students = st;
}

bool ConstraintStudentsSetMaxGapsPerWeek::computeInternalStructure(Rules& r){
	StudentsSet* ss=r.searchAugmentedStudentsSet(this->students);

	if(ss==NULL){
		QMessageBox::warning(NULL, tr("FET warning"),
		 tr("Constraint students set max gaps per week is wrong because it refers to inexistent students set."
		 " Please correct it (removing it might be a solution). Please report potential bug. Constraint is:\n%1").arg(this->getDetailedDescription(r)));
		 
		return false;
	}	

	assert(ss);

	//this->nSubgroups=0;
	this->iSubgroupsList.clear();
	if(ss->type==STUDENTS_SUBGROUP){
		int tmp;
		/*for(tmp=0; tmp<=r.nInternalSubgroups; tmp++)
			if(r.internalSubgroupsList[tmp]->name == ss->name)
				break;*/
		tmp=((StudentsSubgroup*)ss)->indexInInternalSubgroupsList;
		assert(tmp>=0);
		assert(tmp<r.nInternalSubgroups);
		//assert(this->nSubgroups<MAX_SUBGROUPS_PER_CONSTRAINT);
		//this->subgroups[this->nSubgroups++]=tmp;
		if(!this->iSubgroupsList.contains(tmp))
			this->iSubgroupsList.append(tmp);
	}
	else if(ss->type==STUDENTS_GROUP){
		StudentsGroup* stg=(StudentsGroup*)ss;
		for(int i=0; i<stg->subgroupsList.size(); i++){
			StudentsSubgroup* sts=stg->subgroupsList[i];
			int tmp;
			/*for(tmp=0; tmp<=r.nInternalSubgroups; tmp++)
				if(r.internalSubgroupsList[tmp]->name == sts->name)
					break;*/
			tmp=sts->indexInInternalSubgroupsList;
			assert(tmp>=0);
			assert(tmp<r.nInternalSubgroups);
			//assert(this->nSubgroups<MAX_SUBGROUPS_PER_CONSTRAINT);
			//this->subgroups[this->nSubgroups++]=tmp;
			if(!this->iSubgroupsList.contains(tmp))
				this->iSubgroupsList.append(tmp);
		}
	}
	else if(ss->type==STUDENTS_YEAR){
		StudentsYear* sty=(StudentsYear*)ss;
		for(int i=0; i<sty->groupsList.size(); i++){
			StudentsGroup* stg=sty->groupsList[i];
			for(int j=0; j<stg->subgroupsList.size(); j++){
				StudentsSubgroup* sts=stg->subgroupsList[j];
				int tmp;
				/*for(tmp=0; tmp<=r.nInternalSubgroups; tmp++)
					if(r.internalSubgroupsList[tmp]->name == sts->name)
						break;*/
				tmp=sts->indexInInternalSubgroupsList;
				assert(tmp>=0);
				assert(tmp<r.nInternalSubgroups);
				//assert(this->nSubgroups<MAX_SUBGROUPS_PER_CONSTRAINT);
				//this->subgroups[this->nSubgroups++]=tmp;
				if(!this->iSubgroupsList.contains(tmp))
					this->iSubgroupsList.append(tmp);
			}
		}
	}
	else
		assert(0);
		
	return true;
}

bool ConstraintStudentsSetMaxGapsPerWeek::hasInactiveActivities(Rules& r)
{
	Q_UNUSED(r);
	return false;
}

QString ConstraintStudentsSetMaxGapsPerWeek::getXmlDescription(Rules& r){
	//to avoid non-used parameter warning
	Q_UNUSED(r);

	QString s="<ConstraintStudentsSetMaxGapsPerWeek>\n";
	s+="	<Weight_Percentage>"+QString::number(this->weightPercentage)+"</Weight_Percentage>\n";
	s+="	<Max_Gaps>"+QString::number(this->maxGaps)+"</Max_Gaps>\n";
	s+="	<Students>"; s+=protect(this->students); s+="</Students>\n";
	s+="</ConstraintStudentsSetMaxGapsPerWeek>\n";
	return s;
}

QString ConstraintStudentsSetMaxGapsPerWeek::getDescription(Rules& r){
	//to avoid non-used parameter warning
	Q_UNUSED(r);

	QString s;
	s+=tr("Students set max gaps per week"); s+=", ";
	s+=tr("WP:%1\%", "Weight percentage").arg(this->weightPercentage); s+=", ";
	s+=tr("MG:%1", "Max gaps (per week)").arg(this->maxGaps);s+=", ";
	s+=tr("St:%1", "Students").arg(this->students);

	return s;
}

QString ConstraintStudentsSetMaxGapsPerWeek::getDetailedDescription(Rules& r){
	//to avoid non-used parameter warning
	Q_UNUSED(r);

	QString s=tr("Time constraint");s+="\n";
	s+=tr("A students set must respect the maximum number of gaps per week");s+="\n";
	s+=tr("(breaks and students set not available not counted)");s+="\n";
	s+=tr("Weight (percentage)=%1").arg(this->weightPercentage);s+="\n";
	s+=tr("Maximum gaps per week=%1").arg(this->maxGaps);s+="\n";
	s+=tr("Students=%1").arg(this->students); s+="\n";
	
	return s;
}

double ConstraintStudentsSetMaxGapsPerWeek::fitness(Solution& c, Rules& r, QList<double>& cl, QList<QString>&dl, QString* conflictsString)
{
	//OLD COMMENT
	//returns a number equal to the number of windows of the subgroups (in hours)

	//if the matrices subgroupsMatrix and teachersMatrix are already calculated, do not calculate them again!
	if(!c.teachersMatrixReady || !c.subgroupsMatrixReady){
		c.teachersMatrixReady=true;
		c.subgroupsMatrixReady=true;
	//if(crt_chrom!=&c || crt_rules!=&r || subgroups_conflicts<0 || teachers_conflicts<0 || c.changedForMatrixCalculation){
		subgroups_conflicts = c.getSubgroupsMatrix(r, subgroupsMatrix);
		teachers_conflicts = c.getTeachersMatrix(r, teachersMatrix);

		//crt_chrom=&c;
		//crt_rules=&r;
		
		c.changedForMatrixCalculation=false;
	}
	
	int windows;
	int tmp;
	
	int tIllegalWindows=0;
	
	for(int sg=0; sg<this->iSubgroupsList.count(); sg++){
		windows=0;
		int i=this->iSubgroupsList.at(sg);
		for(int j=0; j<r.nDaysPerWeek; j++){
			int k;
			tmp=0;
			for(k=0; k<r.nHoursPerDay; k++)
				if(subgroupsMatrix[i][j][k]>0){
					assert(!breakDayHour[j][k] && !subgroupNotAvailableDayHour[i][j][k]);
					break;
				}
			for(; k<r.nHoursPerDay; k++) if(!breakDayHour[j][k] && !subgroupNotAvailableDayHour[i][j][k]){
				if(subgroupsMatrix[i][j][k]>0){
					windows+=tmp;
					tmp=0;
				}
				else
					tmp++;
			}
		}
		
		int illegalWindows=windows-this->maxGaps;
		if(illegalWindows<0)
			illegalWindows=0;

		if(illegalWindows>0 && conflictsString!=NULL){
			QString s=tr("Time constraint students set max gaps per week broken for subgroup: %1, extra gaps=%2, conflicts increase=%3")
			 .arg(r.internalSubgroupsList[i]->name)
			 .arg(illegalWindows)
			 .arg(weightPercentage/100*illegalWindows);
						 
			dl.append(s);
			cl.append(weightPercentage/100*illegalWindows);
				
			*conflictsString+= s+"\n";
		}
		
		tIllegalWindows+=illegalWindows;
	}

	if(c.nPlacedActivities==r.nInternalActivities)
		if(weightPercentage==100)     //for partial solutions it might be broken
			assert(tIllegalWindows==0);
	return weightPercentage/100 * tIllegalWindows;
}

bool ConstraintStudentsSetMaxGapsPerWeek::isRelatedToActivity(Rules& r, Activity* a)
{
	Q_UNUSED(r);
	Q_UNUSED(a);

	return false;
}

bool ConstraintStudentsSetMaxGapsPerWeek::isRelatedToTeacher(Teacher* t)
{
	Q_UNUSED(t);

	return false;
}

bool ConstraintStudentsSetMaxGapsPerWeek::isRelatedToSubject(Subject* s)
{
	Q_UNUSED(s);

	return false;
}

bool ConstraintStudentsSetMaxGapsPerWeek::isRelatedToActivityTag(ActivityTag* s)
{
	Q_UNUSED(s);

	return false;
}

bool ConstraintStudentsSetMaxGapsPerWeek::isRelatedToStudentsSet(Rules& r, StudentsSet* s)
{
	return r.setsShareStudents(this->students, s->name);
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

ConstraintStudentsEarlyMaxBeginningsAtSecondHour::ConstraintStudentsEarlyMaxBeginningsAtSecondHour()
	: TimeConstraint()
{
	this->type = CONSTRAINT_STUDENTS_EARLY_MAX_BEGINNINGS_AT_SECOND_HOUR;
}

ConstraintStudentsEarlyMaxBeginningsAtSecondHour::ConstraintStudentsEarlyMaxBeginningsAtSecondHour(double wp, int mBSH)
	: TimeConstraint(wp)
{
	this->type = CONSTRAINT_STUDENTS_EARLY_MAX_BEGINNINGS_AT_SECOND_HOUR;
	this->maxBeginningsAtSecondHour=mBSH;
}

bool ConstraintStudentsEarlyMaxBeginningsAtSecondHour::computeInternalStructure(Rules& r)
{
	Q_UNUSED(r);
	
	return true;
}

bool ConstraintStudentsEarlyMaxBeginningsAtSecondHour::hasInactiveActivities(Rules& r)
{
	Q_UNUSED(r);
	return false;
}

QString ConstraintStudentsEarlyMaxBeginningsAtSecondHour::getXmlDescription(Rules& r)
{
	//to avoid non-used parameter warning
	Q_UNUSED(r);

	QString s="<ConstraintStudentsEarlyMaxBeginningsAtSecondHour>\n";
	s+="	<Weight_Percentage>"+QString::number(this->weightPercentage)+"</Weight_Percentage>\n";
	s+="	<Max_Beginnings_At_Second_Hour>"+QString::number(this->maxBeginningsAtSecondHour)+"</Max_Beginnings_At_Second_Hour>\n";
	s+="</ConstraintStudentsEarlyMaxBeginningsAtSecondHour>\n";
	return s;
}

QString ConstraintStudentsEarlyMaxBeginningsAtSecondHour::getDescription(Rules& r)
{
	//to avoid non-used parameter warning
	Q_UNUSED(r);

	QString s;
	s+=tr("Students must arrive early, respecting maximum %1 arrivals at second hour")
	 .arg(this->maxBeginningsAtSecondHour);
	s+=", ";
	s+=tr("WP:%1\%", "Weight percentage").arg(this->weightPercentage);//s+=", ";

	return s;
}

QString ConstraintStudentsEarlyMaxBeginningsAtSecondHour::getDetailedDescription(Rules& r)
{
	//to avoid non-used parameter warning
	Q_UNUSED(r);

	QString s=tr("Time constraint");s+="\n";
	s+=tr("All students must begin their courses early, respecting maximum %1 later arrivals, at second hour")
	 .arg(this->maxBeginningsAtSecondHour);s+="\n";
	s+=tr("(breaks and students set not available not counted)");s+="\n";
	s+=tr("Weight (percentage)=%1\%").arg(this->weightPercentage);s+="\n";

	return s;
}

double ConstraintStudentsEarlyMaxBeginningsAtSecondHour::fitness(Solution& c, Rules& r, QList<double>& cl, QList<QString>&dl, QString* conflictsString)
{
	//considers the condition that the hours of subgroups begin as early as possible

	//if the matrices subgroupsMatrix and teachersMatrix are already calculated, do not calculate them again!
	if(!c.teachersMatrixReady || !c.subgroupsMatrixReady){
		c.teachersMatrixReady=true;
		c.subgroupsMatrixReady=true;
	//if(crt_chrom!=&c || crt_rules!=&r || subgroups_conflicts<0 || teachers_conflicts<0 || c.changedForMatrixCalculation){
		subgroups_conflicts = c.getSubgroupsMatrix(r, subgroupsMatrix);
		teachers_conflicts = c.getTeachersMatrix(r, teachersMatrix);

		//crt_chrom=&c;
		//crt_rules=&r;
		
		c.changedForMatrixCalculation=false;
	}
	
	//int free;
	//int i;
	
	int conflTotal=0;
	
	//free=0; //number of free hours before starting the courses
	for(int i=0; i<r.nInternalSubgroups; i++){
		int nGapsFirstHour=0;
		for(int j=0; j<r.nDaysPerWeek; j++){
			int k;
			for(k=0; k<r.nHoursPerDay; k++)
				if(!breakDayHour[j][k] && !subgroupNotAvailableDayHour[i][j][k])
					break;
				
			bool firstHourOccupied=false;
			if(k<r.nHoursPerDay && subgroupsMatrix[i][j][k]>0)
				firstHourOccupied=true;
					
			bool dayOccupied=firstHourOccupied;
			
			bool illegalGap=false;
				
			for(k++; k<r.nHoursPerDay && !dayOccupied; k++)
				if(!breakDayHour[j][k] && !subgroupNotAvailableDayHour[i][j][k]){
					if(subgroupsMatrix[i][j][k]>0)
						dayOccupied=true;
					else
						illegalGap=true;
				}
				
			if(dayOccupied && illegalGap){
				if(conflictsString!=NULL){
					QString s=tr("Constraint students early max %1 beginnings at second hour broken for subgroup %2, on day %3,"
					 " because students have an illegal gap, increases conflicts total by %4")
					 .arg(this->maxBeginningsAtSecondHour)
					 .arg(r.internalSubgroupsList[i]->name)
					 .arg(r.daysOfTheWeek[j])
					 .arg(1*weightPercentage/100);
					 
					dl.append(s);
					cl.append(1*weightPercentage/100);
						
					*conflictsString+= s+"\n";
					
					conflTotal+=1;
				}
				
				if(c.nPlacedActivities==r.nInternalActivities){
					cout<<"day=="<<j<<endl;
					cout<<"subgroup=="<<qPrintable(r.internalSubgroupsList[i]->name)<<endl;
					for(int h2=0; h2<r.nHoursPerDay; h2++){
						for(int d2=0; d2<r.nDaysPerWeek; d2++)
							cout<<int(subgroupsMatrix[i][d2][h2]);
						cout<<endl;
					}
				
					assert(0);
				}
			}
			
			if(dayOccupied && !firstHourOccupied)
				nGapsFirstHour++;
		}
		
		if(nGapsFirstHour>this->maxBeginningsAtSecondHour){
			if(conflictsString!=NULL){
				QString s=tr("Constraint students early max %1 beginnings at second hour broken for subgroup %2,"
				 " because students have too many arrivals at second hour, increases conflicts total by %3")
				 .arg(this->maxBeginningsAtSecondHour)
				 .arg(r.internalSubgroupsList[i]->name)
				 .arg((nGapsFirstHour-this->maxBeginningsAtSecondHour)*weightPercentage/100);
				 
				dl.append(s);
				cl.append((nGapsFirstHour-this->maxBeginningsAtSecondHour)*weightPercentage/100);
					
				*conflictsString+= s+"\n";
				
				conflTotal+=(nGapsFirstHour-this->maxBeginningsAtSecondHour);
			}
			
			if(c.nPlacedActivities==r.nInternalActivities){
				for(int h=0; h<r.nHoursPerDay; h++){
					for(int d=0; d<r.nDaysPerWeek; d++)
						cout<<int(subgroupsMatrix[i][d][h]);
					cout<<endl;
				}
				cout<<endl;
			
				assert(0);
			}
		}
	}
					
	if(c.nPlacedActivities==r.nInternalActivities)
		if(weightPercentage==100)    //might be broken for partial solutions
			assert(conflTotal==0);
	return weightPercentage/100 * conflTotal;
}

bool ConstraintStudentsEarlyMaxBeginningsAtSecondHour::isRelatedToActivity(Rules& r, Activity* a)
{
	Q_UNUSED(r);
	Q_UNUSED(a);
	//if(a)
	//	;

	return false;
}

bool ConstraintStudentsEarlyMaxBeginningsAtSecondHour::isRelatedToTeacher(Teacher* t)
{
	Q_UNUSED(t);
	//if(t)
	//	;

	return false;
}

bool ConstraintStudentsEarlyMaxBeginningsAtSecondHour::isRelatedToSubject(Subject* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintStudentsEarlyMaxBeginningsAtSecondHour::isRelatedToActivityTag(ActivityTag* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintStudentsEarlyMaxBeginningsAtSecondHour::isRelatedToStudentsSet(Rules& r, StudentsSet* s)
{
	Q_UNUSED(r);
	Q_UNUSED(s);
	/*if(s)
		;
	if(&r)
		;*/

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

ConstraintStudentsSetEarlyMaxBeginningsAtSecondHour::ConstraintStudentsSetEarlyMaxBeginningsAtSecondHour()
	: TimeConstraint()
{
	this->type = CONSTRAINT_STUDENTS_SET_EARLY_MAX_BEGINNINGS_AT_SECOND_HOUR;
}

ConstraintStudentsSetEarlyMaxBeginningsAtSecondHour::ConstraintStudentsSetEarlyMaxBeginningsAtSecondHour(double wp, int mBSH, const QString& students)
	: TimeConstraint(wp)
{
	this->type = CONSTRAINT_STUDENTS_SET_EARLY_MAX_BEGINNINGS_AT_SECOND_HOUR;
	this->students=students;
	this->maxBeginningsAtSecondHour=mBSH;
}

bool ConstraintStudentsSetEarlyMaxBeginningsAtSecondHour::computeInternalStructure(Rules& r)
{
	StudentsSet* ss=r.searchAugmentedStudentsSet(this->students);
	
	if(ss==NULL){
		QMessageBox::warning(NULL, tr("FET warning"),
		 tr("Constraint students set early is wrong because it refers to inexistent students set."
		 " Please correct it (removing it might be a solution). Please report potential bug. Constraint is:\n%1").arg(this->getDetailedDescription(r)));
		 
		return false;
	}	

	assert(ss);

	//this->nSubgroups=0;
	this->iSubgroupsList.clear();
	if(ss->type==STUDENTS_SUBGROUP){
		int tmp;
		/*for(tmp=0; tmp<=r.nInternalSubgroups; tmp++)
			if(r.internalSubgroupsList[tmp]->name == ss->name)
				break;*/
		tmp=((StudentsSubgroup*)ss)->indexInInternalSubgroupsList;
		assert(tmp>=0);
		assert(tmp<r.nInternalSubgroups);
		//assert(this->nSubgroups<MAX_SUBGROUPS_PER_CONSTRAINT);
		//this->subgroups[this->nSubgroups++]=tmp;
		if(!this->iSubgroupsList.contains(tmp))
			this->iSubgroupsList.append(tmp);
	}
	else if(ss->type==STUDENTS_GROUP){
		StudentsGroup* stg=(StudentsGroup*)ss;
		for(int i=0; i<stg->subgroupsList.size(); i++){
			StudentsSubgroup* sts=stg->subgroupsList[i];
			int tmp;
			/*for(tmp=0; tmp<=r.nInternalSubgroups; tmp++)
				if(r.internalSubgroupsList[tmp]->name == sts->name)
					break;*/
			tmp=sts->indexInInternalSubgroupsList;
			assert(tmp>=0);
			assert(tmp<r.nInternalSubgroups);
			//assert(this->nSubgroups<MAX_SUBGROUPS_PER_CONSTRAINT);
			//this->subgroups[this->nSubgroups++]=tmp;
			if(!this->iSubgroupsList.contains(tmp))
				this->iSubgroupsList.append(tmp);
		}
	}
	else if(ss->type==STUDENTS_YEAR){
		StudentsYear* sty=(StudentsYear*)ss;
		for(int i=0; i<sty->groupsList.size(); i++){
			StudentsGroup* stg=sty->groupsList[i];
			for(int j=0; j<stg->subgroupsList.size(); j++){
				StudentsSubgroup* sts=stg->subgroupsList[j];
				int tmp;
				/*for(tmp=0; tmp<=r.nInternalSubgroups; tmp++)
					if(r.internalSubgroupsList[tmp]->name == sts->name)
						break;*/
				tmp=sts->indexInInternalSubgroupsList;
				assert(tmp>=0);
				assert(tmp<r.nInternalSubgroups);
				//assert(this->nSubgroups<MAX_SUBGROUPS_PER_CONSTRAINT);
				//this->subgroups[this->nSubgroups++]=tmp;
				if(!this->iSubgroupsList.contains(tmp))
					this->iSubgroupsList.append(tmp);
			}
		}
	}
	else
		assert(0);
	return true;
}

bool ConstraintStudentsSetEarlyMaxBeginningsAtSecondHour::hasInactiveActivities(Rules& r)
{
	Q_UNUSED(r);
	return false;
}

QString ConstraintStudentsSetEarlyMaxBeginningsAtSecondHour::getXmlDescription(Rules& r)
{
	//to avoid non-used parameter warning
	Q_UNUSED(r);

	QString s="<ConstraintStudentsSetEarlyMaxBeginningsAtSecondHour>\n";
	s+="	<Weight_Percentage>"+QString::number(this->weightPercentage)+"</Weight_Percentage>\n";
	s+="	<Max_Beginnings_At_Second_Hour>"+QString::number(this->maxBeginningsAtSecondHour)+"</Max_Beginnings_At_Second_Hour>\n";
	s+="	<Students>"+protect(this->students)+"</Students>\n";
	s+="</ConstraintStudentsSetEarlyMaxBeginningsAtSecondHour>\n";
	return s;
}

QString ConstraintStudentsSetEarlyMaxBeginningsAtSecondHour::getDescription(Rules& r)
{
	//to avoid non-used parameter warning
	Q_UNUSED(r);

	QString s;

	s+=tr("Students set must arrive early, respecting maximum %1 arrivals at second hour")
	 .arg(this->maxBeginningsAtSecondHour); s+=", ";
	s+=tr("WP:%1\%", "Weight percentage").arg(this->weightPercentage);s+=", ";
	s+=tr("St:%1", "Students set").arg(this->students); //s+=", ";

	return s;
}

QString ConstraintStudentsSetEarlyMaxBeginningsAtSecondHour::getDetailedDescription(Rules& r)
{
	//to avoid non-used parameter warning
	Q_UNUSED(r);

	QString s=tr("Time constraint");s+="\n";

	s+=tr("A students set must begin its courses early, respecting a maximum number of later arrivals, at second hour"); s+="\n";
	s+=tr("(breaks and students set not available not counted)");s+="\n";
	s+=tr("Weight (percentage)=%1\%").arg(this->weightPercentage);s+="\n";
	s+=tr("Students set=%1").arg(this->students); s+="\n";
	s+=tr("Maximum number of arrivals at the second hour=%1").arg(this->maxBeginningsAtSecondHour);s+="\n";

	return s;
}

double ConstraintStudentsSetEarlyMaxBeginningsAtSecondHour::fitness(Solution& c, Rules& r, QList<double>& cl, QList<QString>&dl, QString* conflictsString)
{
	//considers the condition that the hours of subgroups begin as early as possible

	//if the matrices subgroupsMatrix and teachersMatrix are already calculated, do not calculate them again!
	if(!c.teachersMatrixReady || !c.subgroupsMatrixReady){
		c.teachersMatrixReady=true;
		c.subgroupsMatrixReady=true;
	//if(crt_chrom!=&c || crt_rules!=&r || subgroups_conflicts<0 || teachers_conflicts<0 || c.changedForMatrixCalculation){
		subgroups_conflicts = c.getSubgroupsMatrix(r, subgroupsMatrix);
		teachers_conflicts = c.getTeachersMatrix(r, teachersMatrix);

		//crt_chrom=&c;
		//crt_rules=&r;
		
		c.changedForMatrixCalculation=false;
	}
	
	//int free;
	//int i;
	
	int conflTotal=0;

	foreach(int i, this->iSubgroupsList){
	//for(i=0; i<r.nInternalSubgroups; i++){
		int nGapsFirstHour=0;
		for(int j=0; j<r.nDaysPerWeek; j++){
			int k;
			for(k=0; k<r.nHoursPerDay; k++)
				if(!breakDayHour[j][k] && !subgroupNotAvailableDayHour[i][j][k])
					break;
				
			bool firstHourOccupied=false;
			if(k<r.nHoursPerDay && subgroupsMatrix[i][j][k]>0)
				firstHourOccupied=true;
					
			bool dayOccupied=firstHourOccupied;
			
			bool illegalGap=false;
				
			for(k++; k<r.nHoursPerDay && !dayOccupied; k++)
				if(!breakDayHour[j][k] && !subgroupNotAvailableDayHour[i][j][k]){
					if(subgroupsMatrix[i][j][k]>0)
						dayOccupied=true;
					else
						illegalGap=true;
				}
				
			if(dayOccupied && illegalGap){
				if(conflictsString!=NULL){
					QString s=tr("Constraint students set early max %1 beginnings at second hour broken for subgroup %2, on day %3,"
					 " because students have an illegal gap, increases conflicts total by %4")
					 .arg(this->maxBeginningsAtSecondHour)
					 .arg(r.internalSubgroupsList[i]->name)
					 .arg(r.daysOfTheWeek[j])
					 .arg(1*weightPercentage/100);
					 
					dl.append(s);
					cl.append(1*weightPercentage/100);
						
					*conflictsString+= s+"\n";
					
					conflTotal+=1;
				}
				
				if(c.nPlacedActivities==r.nInternalActivities)
					assert(0);
			}
			
			if(dayOccupied && !firstHourOccupied)
				nGapsFirstHour++;
		}
		
		if(nGapsFirstHour>this->maxBeginningsAtSecondHour){
			if(conflictsString!=NULL){
				QString s=tr("Constraint students set early max %1 beginnings at second hour broken for subgroup %2,"
				 " because students have too many arrivals at second hour, increases conflicts total by %3")
				 .arg(this->maxBeginningsAtSecondHour)
				 .arg(r.internalSubgroupsList[i]->name)
				 .arg((nGapsFirstHour-this->maxBeginningsAtSecondHour)*weightPercentage/100);
				 
				dl.append(s);
				cl.append((nGapsFirstHour-this->maxBeginningsAtSecondHour)*weightPercentage/100);
					
				*conflictsString+= s+"\n";
				
				conflTotal+=(nGapsFirstHour-this->maxBeginningsAtSecondHour);
			}
			
			if(c.nPlacedActivities==r.nInternalActivities)
				assert(0);
		}
	}
					
	if(c.nPlacedActivities==r.nInternalActivities)
		if(weightPercentage==100)    //might be broken for partial solutions
			assert(conflTotal==0);
	return weightPercentage/100 * conflTotal;
}

bool ConstraintStudentsSetEarlyMaxBeginningsAtSecondHour::isRelatedToActivity(Rules& r, Activity* a)
{
	Q_UNUSED(r);
	Q_UNUSED(a);
	//if(a)
	//	;

	return false;
}

bool ConstraintStudentsSetEarlyMaxBeginningsAtSecondHour::isRelatedToTeacher(Teacher* t)
{
	Q_UNUSED(t);
	//if(t)
	//	;

	return false;
}

bool ConstraintStudentsSetEarlyMaxBeginningsAtSecondHour::isRelatedToSubject(Subject* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintStudentsSetEarlyMaxBeginningsAtSecondHour::isRelatedToActivityTag(ActivityTag* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintStudentsSetEarlyMaxBeginningsAtSecondHour::isRelatedToStudentsSet(Rules& r, StudentsSet* s)
{
	return r.setsShareStudents(this->students, s->name);
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

ConstraintStudentsMaxHoursDaily::ConstraintStudentsMaxHoursDaily()
	: TimeConstraint()
{
	this->type = CONSTRAINT_STUDENTS_MAX_HOURS_DAILY;
	this->maxHoursDaily = -1;
}

ConstraintStudentsMaxHoursDaily::ConstraintStudentsMaxHoursDaily(double wp, int maxnh)
	: TimeConstraint(wp)
{
	this->maxHoursDaily = maxnh;
	this->type = CONSTRAINT_STUDENTS_MAX_HOURS_DAILY;
}

bool ConstraintStudentsMaxHoursDaily::computeInternalStructure(Rules& r)
{
	Q_UNUSED(r);
	//if(&r!=NULL)
	//	;

	/*do nothing*/
	
	return true;
}

bool ConstraintStudentsMaxHoursDaily::hasInactiveActivities(Rules& r)
{
	Q_UNUSED(r);
	return false;
}

QString ConstraintStudentsMaxHoursDaily::getXmlDescription(Rules& r)
{
	//to avoid non-used parameter warning
	Q_UNUSED(r);

	QString s="<ConstraintStudentsMaxHoursDaily>\n";
	s+="	<Weight_Percentage>"+QString::number(this->weightPercentage)+"</Weight_Percentage>\n";
	//s+="	<Compulsory>";s+=yesNo(this->compulsory);s+="</Compulsory>\n";
	if(this->maxHoursDaily>=0)
		s+="	<Maximum_Hours_Daily>"+QString::number(this->maxHoursDaily)+"</Maximum_Hours_Daily>\n";
	else
		assert(0);
	s+="</ConstraintStudentsMaxHoursDaily>\n";
	return s;
}

QString ConstraintStudentsMaxHoursDaily::getDescription(Rules& r)
{
	//to avoid non-used parameter warning
	Q_UNUSED(r);

	QString s;
	s+=tr("Students max hours daily");s+=", ";
	s+=tr("WP:%1\%", "Weight percentage").arg(this->weightPercentage);s+=", ";
	s+=tr("MH:%1", "Max hours (daily)").arg(this->maxHoursDaily);

	return s;
}

QString ConstraintStudentsMaxHoursDaily::getDetailedDescription(Rules& r)
{
	//to avoid non-used parameter warning
	Q_UNUSED(r);

	QString s=tr("Time constraint");s+="\n";
	s+=tr("All students must respect the maximum number of hours daily");s+="\n";
	s+=tr("Weight (percentage)=%1\%").arg(this->weightPercentage);s+="\n";
	s+=tr("Maximum hours daily=%1").arg(this->maxHoursDaily);s+="\n";

	return s;
}

double ConstraintStudentsMaxHoursDaily::fitness(Solution& c, Rules& r, QList<double>& cl, QList<QString>&dl, QString* conflictsString)
{
	//if the matrices subgroupsMatrix and teachersMatrix are already calculated, do not calculate them again!
	if(!c.teachersMatrixReady || !c.subgroupsMatrixReady){
		c.teachersMatrixReady=true;
		c.subgroupsMatrixReady=true;
	//if(crt_chrom!=&c || crt_rules!=&r || subgroups_conflicts<0 || teachers_conflicts<0 || c.changedForMatrixCalculation){
		subgroups_conflicts = c.getSubgroupsMatrix(r, subgroupsMatrix);
		teachers_conflicts = c.getTeachersMatrix(r, teachersMatrix);

		//crt_chrom=&c;
		//crt_rules=&r;
		
		c.changedForMatrixCalculation=false;
	}

	int tmp;
	int too_much;
	//int too_little;
	
	assert(this->maxHoursDaily>=0);

	if(1){
		too_much=0;
		//too_little=0;
		for(int i=0; i<r.nInternalSubgroups; i++)
			for(int j=0; j<r.nDaysPerWeek; j++){
				tmp=0;
				for(int k=0; k<r.nHoursPerDay; k++){
					//OLD COMMENT
					//Here we want to see if we have a weekly activity or a 2 weeks activity
					//We don't do tmp+=subgroupsMatrix[i][j][k] because we already counted this as a hard hitness
					if(subgroupsMatrix[i][j][k]>=1)
						tmp++;
				}
				if(this->maxHoursDaily>=0 && tmp > this->maxHoursDaily){ //we would like no more than maxHoursDaily hours per day.
					too_much += 1; //tmp - this->maxHoursDaily;

					if(conflictsString!=NULL){
						QString s=tr("Time constraint students max hours daily broken for subgroup: %1, day: %2, lenght=%3, conflict increase=%4")
						 .arg(r.internalSubgroupsList[i]->name)
						 .arg(r.daysOfTheWeek[j])
						 .arg(QString::number(tmp))
						 .arg(weightPercentage/100*(1)); //tmp-this->maxHoursDaily));
						 
						dl.append(s);
						cl.append(weightPercentage/100*(1)); //tmp-this->maxHoursDaily));
					
						*conflictsString+= s+"\n";
					}
				}
			}
	}

	assert(too_much>=0);
	if(weightPercentage==100)
		assert(too_much==0);
	return too_much * weightPercentage/100;
}

bool ConstraintStudentsMaxHoursDaily::isRelatedToActivity(Rules& r, Activity* a)
{
	Q_UNUSED(r);
	Q_UNUSED(a);
	//if(a)
	//	;

	return false;
}

bool ConstraintStudentsMaxHoursDaily::isRelatedToTeacher(Teacher* t)
{
	Q_UNUSED(t);
	//if(t)
	//	;

	return false;
}

bool ConstraintStudentsMaxHoursDaily::isRelatedToSubject(Subject* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintStudentsMaxHoursDaily::isRelatedToActivityTag(ActivityTag* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintStudentsMaxHoursDaily::isRelatedToStudentsSet(Rules& r, StudentsSet* s)
{
	Q_UNUSED(r);
	Q_UNUSED(s);
	/*if(s)
		;
	if(&r)
		;*/

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

ConstraintStudentsSetMaxHoursDaily::ConstraintStudentsSetMaxHoursDaily()
	: TimeConstraint()
{
	this->type = CONSTRAINT_STUDENTS_SET_MAX_HOURS_DAILY;
	this->maxHoursDaily = -1;
}

ConstraintStudentsSetMaxHoursDaily::ConstraintStudentsSetMaxHoursDaily(double wp, int maxnh, QString s)
	: TimeConstraint(wp)
{
	this->maxHoursDaily = maxnh;
	this->students = s;
	this->type = CONSTRAINT_STUDENTS_SET_MAX_HOURS_DAILY;
}

bool ConstraintStudentsSetMaxHoursDaily::hasInactiveActivities(Rules& r)
{
	Q_UNUSED(r);
	return false;
}

QString ConstraintStudentsSetMaxHoursDaily::getXmlDescription(Rules& r)
{
	//to avoid non-used parameter warning
	Q_UNUSED(r);

	QString s="<ConstraintStudentsSetMaxHoursDaily>\n";
	s+="	<Weight_Percentage>"+QString::number(this->weightPercentage)+"</Weight_Percentage>\n";
	//s+="	<Compulsory>";s+=yesNo(this->compulsory);s+="</Compulsory>\n";
	s+="	<Maximum_Hours_Daily>"+QString::number(this->maxHoursDaily)+"</Maximum_Hours_Daily>\n";
	//s+="	<MinHoursDaily>"+QString::number(this->minHoursDaily)+"</MinHoursDaily>\n";
	s+="	<Students>"+protect(this->students)+"</Students>\n";
	s+="</ConstraintStudentsSetMaxHoursDaily>\n";
	return s;
}

QString ConstraintStudentsSetMaxHoursDaily::getDescription(Rules& r)
{
	//to avoid non-used parameter warning
	Q_UNUSED(r);

	QString s;
	s+=tr("Students set max hours daily");s+=", ";
	s+=tr("WP:%1\%", "Weight percentage").arg(this->weightPercentage);s+=", ";
	s+=tr("St:%1", "Students (set)").arg(this->students); s+=", ";
	s+=tr("MH:%1", "Max hours (daily)").arg(this->maxHoursDaily); //s+=", ";

	return s;
}

QString ConstraintStudentsSetMaxHoursDaily::getDetailedDescription(Rules& r)
{
	//to avoid non-used parameter warning
	Q_UNUSED(r);

	QString s=tr("Time constraint");s+="\n";
	s+=tr("A students set must respect the maximum number of hours daily");s+="\n";
	s+=tr("Weight (percentage)=%1\%").arg(this->weightPercentage);s+="\n";
	s+=tr("Students set=%1").arg(this->students);s+="\n";
	s+=tr("Maximum hours daily=%1").arg(this->maxHoursDaily);s+="\n";

	return s;
}

bool ConstraintStudentsSetMaxHoursDaily::computeInternalStructure(Rules &r)
{
	StudentsSet* ss=r.searchAugmentedStudentsSet(this->students);
	
	if(ss==NULL){
		QMessageBox::warning(NULL, tr("FET warning"),
		 tr("Constraint students set max hours daily is wrong because it refers to inexistent students set."
		 " Please correct it (removing it might be a solution). Please report potential bug. Constraint is:\n%1").arg(this->getDetailedDescription(r)));
		 
		return false;
	}	

	assert(ss);

	//this->nSubgroups=0;
	this->iSubgroupsList.clear();
	if(ss->type==STUDENTS_SUBGROUP){
		int tmp;
		/*for(tmp=0; tmp<=r.nInternalSubgroups; tmp++)
			if(r.internalSubgroupsList[tmp]->name == ss->name)
				break;*/
		tmp=((StudentsSubgroup*)ss)->indexInInternalSubgroupsList;
		assert(tmp>=0);
		assert(tmp<r.nInternalSubgroups);
		//assert(this->nSubgroups<MAX_SUBGROUPS_PER_CONSTRAINT);
		//this->subgroups[this->nSubgroups++]=tmp;
		if(!this->iSubgroupsList.contains(tmp))
			this->iSubgroupsList.append(tmp);
	}
	else if(ss->type==STUDENTS_GROUP){
		StudentsGroup* stg=(StudentsGroup*)ss;
		for(int i=0; i<stg->subgroupsList.size(); i++){
			StudentsSubgroup* sts=stg->subgroupsList[i];
			int tmp;
			/*for(tmp=0; tmp<=r.nInternalSubgroups; tmp++)
				if(r.internalSubgroupsList[tmp]->name == sts->name)
					break;*/
			tmp=sts->indexInInternalSubgroupsList;
			assert(tmp>=0);
			assert(tmp<r.nInternalSubgroups);
			//assert(this->nSubgroups<MAX_SUBGROUPS_PER_CONSTRAINT);
			//this->subgroups[this->nSubgroups++]=tmp;
			if(!this->iSubgroupsList.contains(tmp))
				this->iSubgroupsList.append(tmp);
		}
	}
	else if(ss->type==STUDENTS_YEAR){
		StudentsYear* sty=(StudentsYear*)ss;
		for(int i=0; i<sty->groupsList.size(); i++){
			StudentsGroup* stg=sty->groupsList[i];
			for(int j=0; j<stg->subgroupsList.size(); j++){
				StudentsSubgroup* sts=stg->subgroupsList[j];
				int tmp;
				/*for(tmp=0; tmp<=r.nInternalSubgroups; tmp++)
					if(r.internalSubgroupsList[tmp]->name == sts->name)
						break;*/
				tmp=sts->indexInInternalSubgroupsList;
				assert(tmp>=0);
				assert(tmp<r.nInternalSubgroups);
				//assert(this->nSubgroups<MAX_SUBGROUPS_PER_CONSTRAINT);
				//this->subgroups[this->nSubgroups++]=tmp;
				if(!this->iSubgroupsList.contains(tmp))
					this->iSubgroupsList.append(tmp);
			}
		}
	}
	else
		assert(0);
		
	return true;
}

double ConstraintStudentsSetMaxHoursDaily::fitness(Solution& c, Rules& r, QList<double>& cl, QList<QString>&dl, QString* conflictsString)
{
	//if the matrices subgroupsMatrix and teachersMatrix are already calculated, do not calculate them again!
	if(!c.teachersMatrixReady || !c.subgroupsMatrixReady){
		c.teachersMatrixReady=true;
		c.subgroupsMatrixReady=true;
	//if(crt_chrom!=&c || crt_rules!=&r || subgroups_conflicts<0 || teachers_conflicts<0 || c.changedForMatrixCalculation){
		subgroups_conflicts = c.getSubgroupsMatrix(r, subgroupsMatrix);
		teachers_conflicts = c.getTeachersMatrix(r, teachersMatrix);

		//crt_chrom=&c;
		//crt_rules=&r;
		
		c.changedForMatrixCalculation=false;
	}

	int tmp;
	int too_much;
	//int too_little;
	
	assert(this->maxHoursDaily>=0);

	if(1){
		too_much=0;
		//too_little=0;
		for(int sg=0; sg<this->iSubgroupsList.count(); sg++){
			int i=iSubgroupsList.at(sg);
			for(int j=0; j<r.nDaysPerWeek; j++){
				tmp=0;
				for(int k=0; k<r.nHoursPerDay; k++){
					//Here we want to see if we have a weekly activity or a 2 weeks activity
					//We don't do tmp+=subgroupsMatrix[i][j][k] because we already counted this as a hard hitness
					if(subgroupsMatrix[i][j][k]>=1)
						tmp++;
				}
				if(this->maxHoursDaily>=0 && tmp > this->maxHoursDaily){ //we would like no more than max_hours_daily hours per day.
					too_much += 1; //tmp - this->maxHoursDaily;

					if(conflictsString!=NULL){
						QString s=tr("Time constraint students set max hours daily broken for subgroup: %1, day: %2, lenght=%3, conflicts increase=%4")
						 .arg(r.internalSubgroupsList[i]->name)
						 .arg(r.daysOfTheWeek[j])
						 .arg(QString::number(tmp))
						 .arg( 1 /*(tmp-this->maxHoursDaily)*/ *weightPercentage/100);
						 
						dl.append(s);
						cl.append( 1 /*(tmp-this->maxHoursDaily)*/ *weightPercentage/100);
					
						*conflictsString+= s+"\n";
					}
				}
			}
		}
	}
	
	assert(too_much>=0);
	//assert(too_little>=0);
	if(weightPercentage==100)
		assert(too_much==0);
	return too_much * weightPercentage;
}

bool ConstraintStudentsSetMaxHoursDaily::isRelatedToActivity(Rules& r, Activity* a)
{
	Q_UNUSED(r);
	Q_UNUSED(a);
	//if(a)
	//	;

	return false;
}

bool ConstraintStudentsSetMaxHoursDaily::isRelatedToTeacher(Teacher* t)
{
	Q_UNUSED(t);
	//if(t)
	//	;

	return false;
}

bool ConstraintStudentsSetMaxHoursDaily::isRelatedToSubject(Subject* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintStudentsSetMaxHoursDaily::isRelatedToActivityTag(ActivityTag* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintStudentsSetMaxHoursDaily::isRelatedToStudentsSet(Rules& r, StudentsSet* s)
{
	return r.setsShareStudents(this->students, s->name);
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

ConstraintStudentsMaxHoursContinuously::ConstraintStudentsMaxHoursContinuously()
	: TimeConstraint()
{
	this->type = CONSTRAINT_STUDENTS_MAX_HOURS_CONTINUOUSLY;
	this->maxHoursContinuously = -1;
}

ConstraintStudentsMaxHoursContinuously::ConstraintStudentsMaxHoursContinuously(double wp, int maxnh)
	: TimeConstraint(wp)
{
	this->maxHoursContinuously = maxnh;
	this->type = CONSTRAINT_STUDENTS_MAX_HOURS_CONTINUOUSLY;
}

bool ConstraintStudentsMaxHoursContinuously::computeInternalStructure(Rules& r)
{
	Q_UNUSED(r);
	//if(&r!=NULL)
	//	;

	/*do nothing*/
	
	return true;
}

bool ConstraintStudentsMaxHoursContinuously::hasInactiveActivities(Rules& r)
{
	Q_UNUSED(r);
	return false;
}

QString ConstraintStudentsMaxHoursContinuously::getXmlDescription(Rules& r)
{
	//to avoid non-used parameter warning
	Q_UNUSED(r);

	QString s="<ConstraintStudentsMaxHoursContinuously>\n";
	s+="	<Weight_Percentage>"+QString::number(this->weightPercentage)+"</Weight_Percentage>\n";
	//s+="	<Compulsory>";s+=yesNo(this->compulsory);s+="</Compulsory>\n";
	if(this->maxHoursContinuously>=0)
		s+="	<Maximum_Hours_Continuously>"+QString::number(this->maxHoursContinuously)+"</Maximum_Hours_Continuously>\n";
	else
		assert(0);
	s+="</ConstraintStudentsMaxHoursContinuously>\n";
	return s;
}

QString ConstraintStudentsMaxHoursContinuously::getDescription(Rules& r)
{
	//to avoid non-used parameter warning
	Q_UNUSED(r);

	QString s;
	s+=tr("Students max hours continuously");s+=", ";
	s+=tr("WP:%1\%", "Weight percentage").arg(this->weightPercentage);s+=", ";
	s+=tr("MH:%1", "Max hours (continuously)").arg(this->maxHoursContinuously);

	return s;
}

QString ConstraintStudentsMaxHoursContinuously::getDetailedDescription(Rules& r)
{
	//to avoid non-used parameter warning
	Q_UNUSED(r);

	QString s=tr("Time constraint");s+="\n";
	s+=tr("All students must respect the maximum number of hours continuously");s+="\n";
	s+=tr("Weight (percentage)=%1\%").arg(this->weightPercentage);s+="\n";
	s+=tr("Maximum hours continuously=%1").arg(this->maxHoursContinuously);s+="\n";

	return s;
}

double ConstraintStudentsMaxHoursContinuously::fitness(Solution& c, Rules& r, QList<double>& cl, QList<QString>&dl, QString* conflictsString)
{
	//if the matrices subgroupsMatrix and teachersMatrix are already calculated, do not calculate them again!
	if(!c.teachersMatrixReady || !c.subgroupsMatrixReady){
		c.teachersMatrixReady=true;
		c.subgroupsMatrixReady=true;
	//if(crt_chrom!=&c || crt_rules!=&r || subgroups_conflicts<0 || teachers_conflicts<0 || c.changedForMatrixCalculation){
		subgroups_conflicts = c.getSubgroupsMatrix(r, subgroupsMatrix);
		teachers_conflicts = c.getTeachersMatrix(r, teachersMatrix);

		//crt_chrom=&c;
		//crt_rules=&r;
		
		c.changedForMatrixCalculation=false;
	}
	
	int nbroken;

	nbroken=0;
	for(int i=0; i<r.nInternalSubgroups; i++){
		for(int d=0; d<r.nDaysPerWeek; d++){
			int nc=0;
			for(int h=0; h<r.nHoursPerDay; h++){
				if(subgroupsMatrix[i][d][h]>0)
					nc++;
				else{
					if(nc>this->maxHoursContinuously){
						nbroken++;

						if(conflictsString!=NULL){
							QString s=(tr(
							 "Time constraint students max %1 hours continuously broken for subgroup %2, on day %3, length=%4.")
							 .arg(QString::number(this->maxHoursContinuously))
							 .arg(r.internalSubgroupsList[i]->name)
							 .arg(r.daysOfTheWeek[d])
							 .arg(nc)
							 )
							 +
							 " "
							 +
							 (tr("This increases the conflicts total by %1").arg(QString::number(weightPercentage/100)));
							
							dl.append(s);
							cl.append(weightPercentage/100);
				
							*conflictsString+= s+"\n";
						}
					}
				
					nc=0;
				}
			}

			if(nc>this->maxHoursContinuously){
				nbroken++;

				if(conflictsString!=NULL){
					QString s=(tr(
					 "Time constraint students max %1 hours continuously broken for subgroup %2, on day %3, length=%4.")
					 .arg(QString::number(this->maxHoursContinuously))
					 .arg(r.internalSubgroupsList[i]->name)
					 .arg(r.daysOfTheWeek[d])
					 .arg(nc)
					 )
					 +
					 " "
					 +
					 (tr("This increases the conflicts total by %1").arg(QString::number(weightPercentage/100)));
							
					dl.append(s);
					cl.append(weightPercentage/100);
				
					*conflictsString+= s+"\n";
				}
			}
		}
	}

	if(weightPercentage==100)	
		assert(nbroken==0);
	return weightPercentage/100 * nbroken;
}

bool ConstraintStudentsMaxHoursContinuously::isRelatedToActivity(Rules& r, Activity* a)
{
	Q_UNUSED(r);
	Q_UNUSED(a);
	//if(a)
	//	;

	return false;
}

bool ConstraintStudentsMaxHoursContinuously::isRelatedToTeacher(Teacher* t)
{
	Q_UNUSED(t);
	//if(t)
	//	;

	return false;
}

bool ConstraintStudentsMaxHoursContinuously::isRelatedToSubject(Subject* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintStudentsMaxHoursContinuously::isRelatedToActivityTag(ActivityTag* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintStudentsMaxHoursContinuously::isRelatedToStudentsSet(Rules& r, StudentsSet* s)
{
	Q_UNUSED(r);
	Q_UNUSED(s);
	/*if(s)
		;
	if(&r)
		;*/

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

ConstraintStudentsSetMaxHoursContinuously::ConstraintStudentsSetMaxHoursContinuously()
	: TimeConstraint()
{
	this->type = CONSTRAINT_STUDENTS_SET_MAX_HOURS_CONTINUOUSLY;
	this->maxHoursContinuously = -1;
}

ConstraintStudentsSetMaxHoursContinuously::ConstraintStudentsSetMaxHoursContinuously(double wp, int maxnh, QString s)
	: TimeConstraint(wp)
{
	this->maxHoursContinuously = maxnh;
	this->students = s;
	this->type = CONSTRAINT_STUDENTS_SET_MAX_HOURS_CONTINUOUSLY;
}

bool ConstraintStudentsSetMaxHoursContinuously::hasInactiveActivities(Rules& r)
{
	Q_UNUSED(r);
	return false;
}

QString ConstraintStudentsSetMaxHoursContinuously::getXmlDescription(Rules& r)
{
	//to avoid non-used parameter warning
	Q_UNUSED(r);

	QString s="<ConstraintStudentsSetMaxHoursContinuously>\n";
	s+="	<Weight_Percentage>"+QString::number(this->weightPercentage)+"</Weight_Percentage>\n";
	//s+="	<Compulsory>";s+=yesNo(this->compulsory);s+="</Compulsory>\n";
	s+="	<Maximum_Hours_Continuously>"+QString::number(this->maxHoursContinuously)+"</Maximum_Hours_Continuously>\n";
	//s+="	<MinHoursDaily>"+QString::number(this->minHoursDaily)+"</MinHoursDaily>\n";
	s+="	<Students>"+protect(this->students)+"</Students>\n";
	s+="</ConstraintStudentsSetMaxHoursContinuously>\n";
	return s;
}

QString ConstraintStudentsSetMaxHoursContinuously::getDescription(Rules& r)
{
	//to avoid non-used parameter warning
	Q_UNUSED(r);

	QString s;
	s+=tr("Students set max hours continuously");s+=", ";
	s+=tr("WP:%1\%", "Weight percentage").arg(this->weightPercentage);s+=", ";
	s+=tr("St:%1", "Students (set)").arg(this->students);s+=", ";
	s+=tr("MH:%1", "Max hours (continuously)").arg(this->maxHoursContinuously);

	return s;
}

QString ConstraintStudentsSetMaxHoursContinuously::getDetailedDescription(Rules& r)
{
	//to avoid non-used parameter warning
	Q_UNUSED(r);

	QString s=tr("Time constraint");s+="\n";
	s+=tr("A students set must respect the maximum number of hours continuously");s+="\n";
	s+=tr("Weight (percentage)=%1\%").arg(this->weightPercentage);s+="\n";
	s+=tr("Students set=%1").arg(this->students);s+="\n";
	s+=tr("Maximum hours continuously=%1").arg(this->maxHoursContinuously);s+="\n";

	return s;
}

bool ConstraintStudentsSetMaxHoursContinuously::computeInternalStructure(Rules &r)
{
	StudentsSet* ss=r.searchAugmentedStudentsSet(this->students);
	
	if(ss==NULL){
		QMessageBox::warning(NULL, tr("FET warning"),
		 tr("Constraint students set max hours continuously is wrong because it refers to inexistent students set."
		 " Please correct it (removing it might be a solution). Please report potential bug. Constraint is:\n%1").arg(this->getDetailedDescription(r)));
		 
		return false;
	}	

	assert(ss);

	//this->nSubgroups=0;
	this->iSubgroupsList.clear();
	if(ss->type==STUDENTS_SUBGROUP){
		int tmp;
		/*for(tmp=0; tmp<=r.nInternalSubgroups; tmp++)
			if(r.internalSubgroupsList[tmp]->name == ss->name)
				break;*/
		tmp=((StudentsSubgroup*)ss)->indexInInternalSubgroupsList;
		assert(tmp>=0);
		assert(tmp<r.nInternalSubgroups);
		//assert(this->nSubgroups<MAX_SUBGROUPS_PER_CONSTRAINT);
		//this->subgroups[this->nSubgroups++]=tmp;
		if(!this->iSubgroupsList.contains(tmp))
			this->iSubgroupsList.append(tmp);
	}
	else if(ss->type==STUDENTS_GROUP){
		StudentsGroup* stg=(StudentsGroup*)ss;
		for(int i=0; i<stg->subgroupsList.size(); i++){
			StudentsSubgroup* sts=stg->subgroupsList[i];
			int tmp;
			/*for(tmp=0; tmp<=r.nInternalSubgroups; tmp++)
				if(r.internalSubgroupsList[tmp]->name == sts->name)
					break;*/
			tmp=sts->indexInInternalSubgroupsList;
			assert(tmp>=0);
			assert(tmp<r.nInternalSubgroups);
			//assert(this->nSubgroups<MAX_SUBGROUPS_PER_CONSTRAINT);
			//this->subgroups[this->nSubgroups++]=tmp;
			if(!this->iSubgroupsList.contains(tmp))
				this->iSubgroupsList.append(tmp);
		}
	}
	else if(ss->type==STUDENTS_YEAR){
		StudentsYear* sty=(StudentsYear*)ss;
		for(int i=0; i<sty->groupsList.size(); i++){
			StudentsGroup* stg=sty->groupsList[i];
			for(int j=0; j<stg->subgroupsList.size(); j++){
				StudentsSubgroup* sts=stg->subgroupsList[j];
				int tmp;
				/*for(tmp=0; tmp<=r.nInternalSubgroups; tmp++)
					if(r.internalSubgroupsList[tmp]->name == sts->name)
						break;*/
				tmp=sts->indexInInternalSubgroupsList;
				assert(tmp>=0);
				assert(tmp<r.nInternalSubgroups);
				//assert(this->nSubgroups<MAX_SUBGROUPS_PER_CONSTRAINT);
				//this->subgroups[this->nSubgroups++]=tmp;
				if(!this->iSubgroupsList.contains(tmp))
					this->iSubgroupsList.append(tmp);
			}
		}
	}
	else
		assert(0);
		
	return true;
}

double ConstraintStudentsSetMaxHoursContinuously::fitness(Solution& c, Rules& r, QList<double>& cl, QList<QString>&dl, QString* conflictsString)
{
	//if the matrices subgroupsMatrix and teachersMatrix are already calculated, do not calculate them again!
	if(!c.teachersMatrixReady || !c.subgroupsMatrixReady){
		c.teachersMatrixReady=true;
		c.subgroupsMatrixReady=true;
	//if(crt_chrom!=&c || crt_rules!=&r || subgroups_conflicts<0 || teachers_conflicts<0 || c.changedForMatrixCalculation){
		subgroups_conflicts = c.getSubgroupsMatrix(r, subgroupsMatrix);
		teachers_conflicts = c.getTeachersMatrix(r, teachersMatrix);

		//crt_chrom=&c;
		//crt_rules=&r;
		
		c.changedForMatrixCalculation=false;
	}

	int nbroken;

	nbroken=0;
	foreach(int i, this->iSubgroupsList){
		for(int d=0; d<r.nDaysPerWeek; d++){
			int nc=0;
			for(int h=0; h<r.nHoursPerDay; h++){
				if(subgroupsMatrix[i][d][h]>0)
					nc++;
				else{
					if(nc>this->maxHoursContinuously){
						nbroken++;

						if(conflictsString!=NULL){
							QString s=(tr(
							 "Time constraint students set max %1 hours continuously broken for subgroup %2, on day %3, length=%4.")
							 .arg(QString::number(this->maxHoursContinuously))
							 .arg(r.internalSubgroupsList[i]->name)
							 .arg(r.daysOfTheWeek[d])
							 .arg(nc)
							 )
							 +
							 " "
							 +
							 (tr("This increases the conflicts total by %1").arg(QString::number(weightPercentage/100)));
							
							dl.append(s);
							cl.append(weightPercentage/100);
				
							*conflictsString+= s+"\n";
						}
					}
				
					nc=0;
				}
			}

			if(nc>this->maxHoursContinuously){
				nbroken++;

				if(conflictsString!=NULL){
					QString s=(tr(
					 "Time constraint students set max %1 hours continuously broken for subgroup %2, on day %3, length=%4.")
					 .arg(QString::number(this->maxHoursContinuously))
					 .arg(r.internalSubgroupsList[i]->name)
					 .arg(r.daysOfTheWeek[d])
					 .arg(nc)
					 )
					 +
					 " "
					 +
					 (tr("This increases the conflicts total by %1").arg(QString::number(weightPercentage/100)));
							
					dl.append(s);
					cl.append(weightPercentage/100);
				
					*conflictsString+= s+"\n";
				}
			}
		}
	}

	if(weightPercentage==100)	
		assert(nbroken==0);
	return weightPercentage/100 * nbroken;
}

bool ConstraintStudentsSetMaxHoursContinuously::isRelatedToActivity(Rules& r, Activity* a)
{
	Q_UNUSED(r);
	Q_UNUSED(a);
	//if(a)
	//	;

	return false;
}

bool ConstraintStudentsSetMaxHoursContinuously::isRelatedToTeacher(Teacher* t)
{
	Q_UNUSED(t);
	//if(t)
	//	;

	return false;
}

bool ConstraintStudentsSetMaxHoursContinuously::isRelatedToSubject(Subject* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintStudentsSetMaxHoursContinuously::isRelatedToActivityTag(ActivityTag* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintStudentsSetMaxHoursContinuously::isRelatedToStudentsSet(Rules& r, StudentsSet* s)
{
	return r.setsShareStudents(this->students, s->name);
}




////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

ConstraintStudentsActivityTagMaxHoursContinuously::ConstraintStudentsActivityTagMaxHoursContinuously()
	: TimeConstraint()
{
	this->type = CONSTRAINT_STUDENTS_ACTIVITY_TAG_MAX_HOURS_CONTINUOUSLY;
	this->maxHoursContinuously = -1;
}

ConstraintStudentsActivityTagMaxHoursContinuously::ConstraintStudentsActivityTagMaxHoursContinuously(double wp, int maxnh, const QString& activityTag)
	: TimeConstraint(wp)
{
	this->maxHoursContinuously = maxnh;
	this->activityTagName=activityTag;
	this->type = CONSTRAINT_STUDENTS_ACTIVITY_TAG_MAX_HOURS_CONTINUOUSLY;
}

bool ConstraintStudentsActivityTagMaxHoursContinuously::computeInternalStructure(Rules& r)
{
	this->activityTagIndex=r.searchActivityTag(this->activityTagName);
	assert(this->activityTagIndex>=0);
	
	this->canonicalSubgroupsList.clear();
	for(int i=0; i<r.nInternalSubgroups; i++){
		bool found=false;
	
/*		StudentsSubgroup* sbg=r.internalSubgroupsList[i];
		foreach(int actIndex, sbg->activitiesForSubgroup){
			int actTagIndex=r.internalActivitiesList[actIndex].activityTagIndex;
			if(actTagIndex==this->activityTagIndex){
				found=true;
				break;
			}
		}*/
		
		StudentsSubgroup* sbg=r.internalSubgroupsList[i];
		foreach(int actIndex, sbg->activitiesForSubgroup){
			if(r.internalActivitiesList[actIndex].iActivityTagsSet.contains(this->activityTagIndex)){
				found=true;
				break;
			}
		}
		
		if(found)
			this->canonicalSubgroupsList.append(i);
	}

	return true;
}

bool ConstraintStudentsActivityTagMaxHoursContinuously::hasInactiveActivities(Rules& r)
{
	Q_UNUSED(r);
	return false;
}

QString ConstraintStudentsActivityTagMaxHoursContinuously::getXmlDescription(Rules& r)
{
	//to avoid non-used parameter warning
	Q_UNUSED(r);

	QString s="<ConstraintStudentsActivityTagMaxHoursContinuously>\n";
	s+="	<Weight_Percentage>"+QString::number(this->weightPercentage)+"</Weight_Percentage>\n";
	
	s+="	<Activity_Tag>"+protect(this->activityTagName)+"</Activity_Tag>\n";
	
	//s+="	<Compulsory>";s+=yesNo(this->compulsory);s+="</Compulsory>\n";
	if(this->maxHoursContinuously>=0)
		s+="	<Maximum_Hours_Continuously>"+QString::number(this->maxHoursContinuously)+"</Maximum_Hours_Continuously>\n";
	else
		assert(0);
	s+="</ConstraintStudentsActivityTagMaxHoursContinuously>\n";
	return s;
}

QString ConstraintStudentsActivityTagMaxHoursContinuously::getDescription(Rules& r)
{
	//to avoid non-used parameter warning
	Q_UNUSED(r);

	QString s;
	s+=tr("Students for activity tag %1 have max %2 hours continuously")
		.arg(this->activityTagName).arg(this->maxHoursContinuously); s+=", ";
	s+=tr("WP:%1\%", "Weight percentage").arg(this->weightPercentage);//s+=", ";

	return s;
}

QString ConstraintStudentsActivityTagMaxHoursContinuously::getDetailedDescription(Rules& r)
{
	//to avoid non-used parameter warning
	Q_UNUSED(r);

	QString s=tr("Time constraint");s+="\n";
	s+=tr("All students, for an activity tag, must respect the maximum number of hours continuously"); s+="\n";
	s+=tr("Weight (percentage)=%1\%").arg(this->weightPercentage);s+="\n";
	s+=tr("Activity tag=%1").arg(this->activityTagName);s+="\n";
	s+=tr("Maximum hours continuously=%1").arg(this->maxHoursContinuously);s+="\n";

	return s;
}

double ConstraintStudentsActivityTagMaxHoursContinuously::fitness(Solution& c, Rules& r, QList<double>& cl, QList<QString>&dl, QString* conflictsString)
{
	//if the matrices subgroupsMatrix and teachersMatrix are already calculated, do not calculate them again!
	if(!c.teachersMatrixReady || !c.subgroupsMatrixReady){
		c.teachersMatrixReady=true;
		c.subgroupsMatrixReady=true;
	//if(crt_chrom!=&c || crt_rules!=&r || subgroups_conflicts<0 || teachers_conflicts<0 || c.changedForMatrixCalculation){
		subgroups_conflicts = c.getSubgroupsMatrix(r, subgroupsMatrix);
		teachers_conflicts = c.getTeachersMatrix(r, teachersMatrix);

		//crt_chrom=&c;
		//crt_rules=&r;
		
		c.changedForMatrixCalculation=false;
	}
	
	int nbroken;

	nbroken=0;
	
	foreach(int i, this->canonicalSubgroupsList){
		StudentsSubgroup* sbg=r.internalSubgroupsList[i];
		int crtSubgroupTimetableActivityTag[MAX_DAYS_PER_WEEK][MAX_HOURS_PER_DAY];
		for(int d=0; d<r.nDaysPerWeek; d++)
			for(int h=0; h<r.nHoursPerDay; h++)
				crtSubgroupTimetableActivityTag[d][h]=-1;
		foreach(int ai, sbg->activitiesForSubgroup)if(c.times[ai]!=UNALLOCATED_TIME){
			int d=c.times[ai]%r.nDaysPerWeek;
			int h=c.times[ai]/r.nDaysPerWeek;
			for(int dur=0; dur<r.internalActivitiesList[ai].duration; dur++){
				assert(h+dur<r.nHoursPerDay);
				assert(crtSubgroupTimetableActivityTag[d][h+dur]==-1);
				if(r.internalActivitiesList[ai].iActivityTagsSet.contains(this->activityTagIndex))
					crtSubgroupTimetableActivityTag[d][h+dur]=this->activityTagIndex;
				//crtSubgroupTimetableActivityTag[d][h+dur]=r.internalActivitiesList[ai].activityTagIndex;
			}
		}

		for(int d=0; d<r.nDaysPerWeek; d++){
			int nc=0;
			for(int h=0; h<r.nHoursPerDay; h++){
				bool inc=false;
				
				if(crtSubgroupTimetableActivityTag[d][h]==this->activityTagIndex)
					inc=true;
				
				if(inc)
					nc++;
				else{
					if(nc>this->maxHoursContinuously){
						nbroken++;

						if(conflictsString!=NULL){
							QString s=(tr(
							 "Time constraint students, activity tag %1, max %2 hours continuously, broken for subgroup %3, on day %4, length=%5.")
							 .arg(this->activityTagName)
							 .arg(QString::number(this->maxHoursContinuously))
							 .arg(r.internalSubgroupsList[i]->name)
							 .arg(r.daysOfTheWeek[d])
							 .arg(nc)
							 )
							 +
							 " "
							 +
							 (tr("This increases the conflicts total by %1").arg(QString::number(weightPercentage/100)));
							
							dl.append(s);
							cl.append(weightPercentage/100);
				
							*conflictsString+= s+"\n";
						}
					}
				
					nc=0;
				}
			}

			if(nc>this->maxHoursContinuously){
				nbroken++;

				if(conflictsString!=NULL){
					QString s=(tr(
					 "Time constraint students, activity tag %1, max %2 hours continuously, broken for subgroup %3, on day %4, length=%5.")
					 .arg(this->activityTagName)
					 .arg(QString::number(this->maxHoursContinuously))
					 .arg(r.internalSubgroupsList[i]->name)
					 .arg(r.daysOfTheWeek[d])
					 .arg(nc)
					 )
					 +
					 " "
					 +
					 (tr("This increases the conflicts total by %1").arg(QString::number(weightPercentage/100)));
							
					dl.append(s);
					cl.append(weightPercentage/100);
				
					*conflictsString+= s+"\n";
				}
			}
		}
	}

	if(weightPercentage==100)	
		assert(nbroken==0);
	return weightPercentage/100 * nbroken;
}

bool ConstraintStudentsActivityTagMaxHoursContinuously::isRelatedToActivity(Rules& r, Activity* a)
{
	Q_UNUSED(r);
	Q_UNUSED(a);
	//if(a)
	//	;

	return false;
}

bool ConstraintStudentsActivityTagMaxHoursContinuously::isRelatedToTeacher(Teacher* t)
{
	Q_UNUSED(t);
	//if(t)
	//	;

	return false;
}

bool ConstraintStudentsActivityTagMaxHoursContinuously::isRelatedToSubject(Subject* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintStudentsActivityTagMaxHoursContinuously::isRelatedToActivityTag(ActivityTag* s)
{
	//Q_UNUSED(s);
	//if(s)
	//	;
	
	return s->name==this->activityTagName;

	//return false;
}

bool ConstraintStudentsActivityTagMaxHoursContinuously::isRelatedToStudentsSet(Rules& r, StudentsSet* s)
{
	Q_UNUSED(r);
	Q_UNUSED(s);
	/*if(s)
		;
	if(&r)
		;*/

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

ConstraintStudentsSetActivityTagMaxHoursContinuously::ConstraintStudentsSetActivityTagMaxHoursContinuously()
	: TimeConstraint()
{
	this->type = CONSTRAINT_STUDENTS_SET_ACTIVITY_TAG_MAX_HOURS_CONTINUOUSLY;
	this->maxHoursContinuously = -1;
}

ConstraintStudentsSetActivityTagMaxHoursContinuously::ConstraintStudentsSetActivityTagMaxHoursContinuously(double wp, int maxnh, const QString& s, const QString& activityTag)
	: TimeConstraint(wp)
{
	this->maxHoursContinuously = maxnh;
	this->students = s;
	this->activityTagName=activityTag;
	this->type = CONSTRAINT_STUDENTS_SET_ACTIVITY_TAG_MAX_HOURS_CONTINUOUSLY;
}

bool ConstraintStudentsSetActivityTagMaxHoursContinuously::hasInactiveActivities(Rules& r)
{
	Q_UNUSED(r);
	return false;
}

QString ConstraintStudentsSetActivityTagMaxHoursContinuously::getXmlDescription(Rules& r)
{
	//to avoid non-used parameter warning
	Q_UNUSED(r);

	QString s="<ConstraintStudentsSetActivityTagMaxHoursContinuously>\n";
	s+="	<Weight_Percentage>"+QString::number(this->weightPercentage)+"</Weight_Percentage>\n";
	//s+="	<Compulsory>";s+=yesNo(this->compulsory);s+="</Compulsory>\n";
	s+="	<Maximum_Hours_Continuously>"+QString::number(this->maxHoursContinuously)+"</Maximum_Hours_Continuously>\n";
	//s+="	<MinHoursDaily>"+QString::number(this->minHoursDaily)+"</MinHoursDaily>\n";
	s+="	<Students>"+protect(this->students)+"</Students>\n";
	s+="	<Activity_Tag>"+protect(this->activityTagName)+"</Activity_Tag>\n";
	s+="</ConstraintStudentsSetActivityTagMaxHoursContinuously>\n";
	return s;
}

QString ConstraintStudentsSetActivityTagMaxHoursContinuously::getDescription(Rules& r)
{
	//to avoid non-used parameter warning
	Q_UNUSED(r);

	QString s;
	s+=tr("Students set %1 for activity tag %2 has max %3 hours continuously").arg(this->students).arg(this->activityTagName).arg(this->maxHoursContinuously);
	s+=", ";
	s+=tr("WP:%1\%", "Weight percentage").arg(this->weightPercentage);//s+=", ";

	return s;
}

QString ConstraintStudentsSetActivityTagMaxHoursContinuously::getDetailedDescription(Rules& r)
{
	//to avoid non-used parameter warning
	Q_UNUSED(r);

	QString s=tr("Time constraint");s+="\n";
	s+=tr("A students set, for an activity tag, must respect the maximum number of hours continuously"); s+="\n";
	s+=tr("Weight (percentage)=%1\%").arg(this->weightPercentage);s+="\n";
	s+=tr("Students set=%1").arg(this->students);s+="\n";
	s+=tr("Activity tag=%1").arg(this->activityTagName);s+="\n";
	s+=tr("Maximum hours continuously=%1").arg(this->maxHoursContinuously);s+="\n";

	return s;
}

bool ConstraintStudentsSetActivityTagMaxHoursContinuously::computeInternalStructure(Rules &r)
{
	this->activityTagIndex=r.searchActivityTag(this->activityTagName);
	assert(this->activityTagIndex>=0);

	StudentsSet* ss=r.searchAugmentedStudentsSet(this->students);
	
	if(ss==NULL){
		QMessageBox::warning(NULL, tr("FET warning"),
		 tr("Constraint students set max hours continuously is wrong because it refers to inexistent students set."
		 " Please correct it (removing it might be a solution). Please report potential bug. Constraint is:\n%1").arg(this->getDetailedDescription(r)));
		 
		return false;
	}	

	assert(ss);

	//this->nSubgroups=0;
	this->iSubgroupsList.clear();
	if(ss->type==STUDENTS_SUBGROUP){
		int tmp;
		/*for(tmp=0; tmp<=r.nInternalSubgroups; tmp++)
			if(r.internalSubgroupsList[tmp]->name == ss->name)
				break;*/
		tmp=((StudentsSubgroup*)ss)->indexInInternalSubgroupsList;
		assert(tmp>=0);
		assert(tmp<r.nInternalSubgroups);
		//assert(this->nSubgroups<MAX_SUBGROUPS_PER_CONSTRAINT);
		//this->subgroups[this->nSubgroups++]=tmp;
		if(!this->iSubgroupsList.contains(tmp))
			this->iSubgroupsList.append(tmp);
	}
	else if(ss->type==STUDENTS_GROUP){
		StudentsGroup* stg=(StudentsGroup*)ss;
		for(int i=0; i<stg->subgroupsList.size(); i++){
			StudentsSubgroup* sts=stg->subgroupsList[i];
			int tmp;
			/*for(tmp=0; tmp<=r.nInternalSubgroups; tmp++)
				if(r.internalSubgroupsList[tmp]->name == sts->name)
					break;*/
			tmp=sts->indexInInternalSubgroupsList;
			assert(tmp>=0);
			assert(tmp<r.nInternalSubgroups);
			//assert(this->nSubgroups<MAX_SUBGROUPS_PER_CONSTRAINT);
			//this->subgroups[this->nSubgroups++]=tmp;
			if(!this->iSubgroupsList.contains(tmp))
				this->iSubgroupsList.append(tmp);
		}
	}
	else if(ss->type==STUDENTS_YEAR){
		StudentsYear* sty=(StudentsYear*)ss;
		for(int i=0; i<sty->groupsList.size(); i++){
			StudentsGroup* stg=sty->groupsList[i];
			for(int j=0; j<stg->subgroupsList.size(); j++){
				StudentsSubgroup* sts=stg->subgroupsList[j];
				int tmp;
				/*for(tmp=0; tmp<=r.nInternalSubgroups; tmp++)
					if(r.internalSubgroupsList[tmp]->name == sts->name)
						break;*/
				tmp=sts->indexInInternalSubgroupsList;
				assert(tmp>=0);
				assert(tmp<r.nInternalSubgroups);
				//assert(this->nSubgroups<MAX_SUBGROUPS_PER_CONSTRAINT);
				//this->subgroups[this->nSubgroups++]=tmp;
				if(!this->iSubgroupsList.contains(tmp))
					this->iSubgroupsList.append(tmp);
			}
		}
	}
	else
		assert(0);
		
	/////////////
	this->canonicalSubgroupsList.clear();
	foreach(int i, this->iSubgroupsList){
		bool found=false;
	
/*		StudentsSubgroup* sbg=r.internalSubgroupsList[i];
		foreach(int actIndex, sbg->activitiesForSubgroup){
			int actTagIndex=r.internalActivitiesList[actIndex].activityTagIndex;
			if(actTagIndex==this->activityTagIndex){
				found=true;
				break;
			}
		}*/
		
		StudentsSubgroup* sbg=r.internalSubgroupsList[i];
		foreach(int actIndex, sbg->activitiesForSubgroup){
			if(r.internalActivitiesList[actIndex].iActivityTagsSet.contains(this->activityTagIndex)){
				found=true;
				break;
			}
		}
		
		if(found)
			this->canonicalSubgroupsList.append(i);
	}

		
	return true;
}

double ConstraintStudentsSetActivityTagMaxHoursContinuously::fitness(Solution& c, Rules& r, QList<double>& cl, QList<QString>&dl, QString* conflictsString)
{
	//if the matrices subgroupsMatrix and teachersMatrix are already calculated, do not calculate them again!
	if(!c.teachersMatrixReady || !c.subgroupsMatrixReady){
		c.teachersMatrixReady=true;
		c.subgroupsMatrixReady=true;
	//if(crt_chrom!=&c || crt_rules!=&r || subgroups_conflicts<0 || teachers_conflicts<0 || c.changedForMatrixCalculation){
		subgroups_conflicts = c.getSubgroupsMatrix(r, subgroupsMatrix);
		teachers_conflicts = c.getTeachersMatrix(r, teachersMatrix);

		//crt_chrom=&c;
		//crt_rules=&r;
		
		c.changedForMatrixCalculation=false;
	}

	int nbroken;

	nbroken=0;

	foreach(int i, this->canonicalSubgroupsList){
		StudentsSubgroup* sbg=r.internalSubgroupsList[i];
		int crtSubgroupTimetableActivityTag[MAX_DAYS_PER_WEEK][MAX_HOURS_PER_DAY];
		for(int d=0; d<r.nDaysPerWeek; d++)
			for(int h=0; h<r.nHoursPerDay; h++)
				crtSubgroupTimetableActivityTag[d][h]=-1;
		foreach(int ai, sbg->activitiesForSubgroup)if(c.times[ai]!=UNALLOCATED_TIME){
			int d=c.times[ai]%r.nDaysPerWeek;
			int h=c.times[ai]/r.nDaysPerWeek;
			for(int dur=0; dur<r.internalActivitiesList[ai].duration; dur++){
				assert(h+dur<r.nHoursPerDay);
				assert(crtSubgroupTimetableActivityTag[d][h+dur]==-1);
				if(r.internalActivitiesList[ai].iActivityTagsSet.contains(this->activityTagIndex))
					crtSubgroupTimetableActivityTag[d][h+dur]=this->activityTagIndex;
				//crtSubgroupTimetableActivityTag[d][h+dur]=r.internalActivitiesList[ai].activityTagIndex;
			}
		}

		for(int d=0; d<r.nDaysPerWeek; d++){
			int nc=0;
			for(int h=0; h<r.nHoursPerDay; h++){
				bool inc=false;
				
				if(crtSubgroupTimetableActivityTag[d][h]==this->activityTagIndex)
					inc=true;
			
				if(inc)
					nc++;
				else{
					if(nc>this->maxHoursContinuously){
						nbroken++;

						if(conflictsString!=NULL){
							QString s=(tr(
							 "Time constraint students set max %1 hours continuously broken for subgroup %2, on day %3, length=%4.")
							 .arg(QString::number(this->maxHoursContinuously))
							 .arg(r.internalSubgroupsList[i]->name)
							 .arg(r.daysOfTheWeek[d])
							 .arg(nc)
							 )
							 +
							 " "
							 +
							 (tr("This increases the conflicts total by %1").arg(QString::number(weightPercentage/100)));
							
							dl.append(s);
							cl.append(weightPercentage/100);
				
							*conflictsString+= s+"\n";
						}
					}
				
					nc=0;
				}
			}

			if(nc>this->maxHoursContinuously){
				nbroken++;

				if(conflictsString!=NULL){
					QString s=(tr(
					 "Time constraint students set max %1 hours continuously broken for subgroup %2, on day %3, length=%4.")
					 .arg(QString::number(this->maxHoursContinuously))
					 .arg(r.internalSubgroupsList[i]->name)
					 .arg(r.daysOfTheWeek[d])
					 .arg(nc)
					 )
					 +
					 " "
					 +
					 (tr("This increases the conflicts total by %1").arg(QString::number(weightPercentage/100)));
							
					dl.append(s);
					cl.append(weightPercentage/100);
				
					*conflictsString+= s+"\n";
				}
			}
		}
	}

	if(weightPercentage==100)	
		assert(nbroken==0);
	return weightPercentage/100 * nbroken;
}

bool ConstraintStudentsSetActivityTagMaxHoursContinuously::isRelatedToActivity(Rules& r, Activity* a)
{
	Q_UNUSED(r);
	Q_UNUSED(a);
	//if(a)
	//	;

	return false;
}

bool ConstraintStudentsSetActivityTagMaxHoursContinuously::isRelatedToTeacher(Teacher* t)
{
	Q_UNUSED(t);
	//if(t)
	//	;

	return false;
}

bool ConstraintStudentsSetActivityTagMaxHoursContinuously::isRelatedToSubject(Subject* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintStudentsSetActivityTagMaxHoursContinuously::isRelatedToActivityTag(ActivityTag* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintStudentsSetActivityTagMaxHoursContinuously::isRelatedToStudentsSet(Rules& r, StudentsSet* s)
{
	return r.setsShareStudents(this->students, s->name);
}






////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

ConstraintStudentsMinHoursDaily::ConstraintStudentsMinHoursDaily()
	: TimeConstraint()
{
	this->type = CONSTRAINT_STUDENTS_MIN_HOURS_DAILY;
	this->minHoursDaily = -1;
	
	this->allowEmptyDays=false;
}

ConstraintStudentsMinHoursDaily::ConstraintStudentsMinHoursDaily(double wp, int minnh, bool _allowEmptyDays)
	: TimeConstraint(wp)
{
	this->minHoursDaily = minnh;
	this->type = CONSTRAINT_STUDENTS_MIN_HOURS_DAILY;
	
	this->allowEmptyDays=_allowEmptyDays;
}

bool ConstraintStudentsMinHoursDaily::computeInternalStructure(Rules& r)
{
	Q_UNUSED(r);
	
	return true;
}

bool ConstraintStudentsMinHoursDaily::hasInactiveActivities(Rules& r)
{
	Q_UNUSED(r);
	return false;
}

QString ConstraintStudentsMinHoursDaily::getXmlDescription(Rules& r)
{
	//to avoid non-used parameter warning
	Q_UNUSED(r);

	QString s="<ConstraintStudentsMinHoursDaily>\n";
	s+="	<Weight_Percentage>"+QString::number(this->weightPercentage)+"</Weight_Percentage>\n";
	if(this->minHoursDaily>=0)
		s+="	<Minimum_Hours_Daily>"+QString::number(this->minHoursDaily)+"</Minimum_Hours_Daily>\n";
	else
		assert(0);
	if(this->allowEmptyDays)
		s+="	<Allow_Empty_Days>true</Allow_Empty_Days>\n";
	else
		s+="	<Allow_Empty_Days>false</Allow_Empty_Days>\n";
	s+="</ConstraintStudentsMinHoursDaily>\n";
	return s;
}

QString ConstraintStudentsMinHoursDaily::getDescription(Rules& r)
{
	//to avoid non-used parameter warning
	Q_UNUSED(r);

	QString s;

	if(this->allowEmptyDays)
		s+="! ";
	s+=tr("Students min hours daily");s+=", ";
	s+=tr("WP:%1\%", "Weight percentage").arg(this->weightPercentage);s+=", ";
	s+=tr("mH:%1", "Min hours (daily)").arg(this->minHoursDaily);s+=", ";
	s+=tr("AED:%1", "Allow empty days").arg(yesNoTranslated(this->allowEmptyDays));

	return s;
}

QString ConstraintStudentsMinHoursDaily::getDetailedDescription(Rules& r)
{
	//to avoid non-used parameter warning
	Q_UNUSED(r);

	QString s=tr("Time constraint");s+="\n";
	if(this->allowEmptyDays==true){
		s+=tr("(non-standard, students may have empty days)");
		s+="\n";
	}
	s+=tr("All students must respect the minimum number of hours daily");s+="\n";
	s+=tr("Weight (percentage)=%1\%").arg(this->weightPercentage);s+="\n";
	s+=tr("Minimum hours daily=%1").arg(this->minHoursDaily);s+="\n";
	s+=tr("Allow empty days=%1").arg(yesNoTranslated(this->allowEmptyDays));s+="\n";
	//s+=tr("Note: FET considers that each day of the week must have the minimum number of working hours, so you cannot have empty days for affected students (constraint is not flexible)");s+="\n";

	return s;
}

double ConstraintStudentsMinHoursDaily::fitness(Solution& c, Rules& r, QList<double>& cl, QList<QString>&dl, QString* conflictsString)
{
	//if the matrices subgroupsMatrix and teachersMatrix are already calculated, do not calculate them again!
	if(!c.teachersMatrixReady || !c.subgroupsMatrixReady){
		c.teachersMatrixReady=true;
		c.subgroupsMatrixReady=true;
	//if(crt_chrom!=&c || crt_rules!=&r || subgroups_conflicts<0 || teachers_conflicts<0 || c.changedForMatrixCalculation){
		subgroups_conflicts = c.getSubgroupsMatrix(r, subgroupsMatrix);
		teachers_conflicts = c.getTeachersMatrix(r, teachersMatrix);

		//crt_chrom=&c;
		//crt_rules=&r;
		
		c.changedForMatrixCalculation=false;
	}

	int tmp;
	int too_little;
	
	assert(this->minHoursDaily>=0);

	too_little=0;
	for(int i=0; i<r.nInternalSubgroups; i++)
		for(int j=0; j<r.nDaysPerWeek; j++){
			tmp=0;
			for(int k=0; k<r.nHoursPerDay; k++){
				if(subgroupsMatrix[i][j][k]>=1)
					tmp++;
			}
			
			//if(tmp>0) - smart, for empty days does not consider
			
			bool searchDay;
			if(this->allowEmptyDays==true)
				searchDay=(tmp>0);
			else
				searchDay=true;
			
			if(/*tmp>0*/ searchDay && this->minHoursDaily>=0 && tmp < this->minHoursDaily){ //we would like no less than minHoursDaily hours per day.
				too_little += - tmp + this->minHoursDaily;

				if(conflictsString!=NULL){
					QString s=tr("Time constraint students min hours daily broken for subgroup: %1, day: %2, lenght=%3, conflict increase=%4")
					 .arg(r.internalSubgroupsList[i]->name)
					 .arg(r.daysOfTheWeek[j])
					 .arg(QString::number(tmp))
					 .arg(weightPercentage/100*(-tmp+this->minHoursDaily));
						 
					dl.append(s);
					cl.append(weightPercentage/100*(-tmp+this->minHoursDaily));
					
					*conflictsString+= s+"\n";
				}
			}
		}

	//for empty days should not consider
			
	assert(too_little>=0);

	/*int na=0;
	for(int i=0; i<r.nInternalActivities; i++)
		if(c.times[i]!=UNALLOCATED_TIME)
			na++;*/
	if(c.nPlacedActivities==r.nInternalActivities)
	//if(na==r.nInternalActivities)
		if(weightPercentage==100) //does not work for partial solutions
			assert(too_little==0);

	return too_little * weightPercentage/100;
}

bool ConstraintStudentsMinHoursDaily::isRelatedToActivity(Rules& r, Activity* a)
{
	Q_UNUSED(r);
	Q_UNUSED(a);
	//if(a)
	//	;

	return false;
}

bool ConstraintStudentsMinHoursDaily::isRelatedToTeacher(Teacher* t)
{
	Q_UNUSED(t);
	//if(t)
	//	;

	return false;
}

bool ConstraintStudentsMinHoursDaily::isRelatedToSubject(Subject* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintStudentsMinHoursDaily::isRelatedToActivityTag(ActivityTag* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintStudentsMinHoursDaily::isRelatedToStudentsSet(Rules& r, StudentsSet* s)
{
	Q_UNUSED(r);
	Q_UNUSED(s);
	/*if(s)
		;
	if(&r)
		;*/

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

ConstraintStudentsSetMinHoursDaily::ConstraintStudentsSetMinHoursDaily()
	: TimeConstraint()
{
	this->type = CONSTRAINT_STUDENTS_SET_MIN_HOURS_DAILY;
	this->minHoursDaily = -1;
	
	this->allowEmptyDays=false;
}

ConstraintStudentsSetMinHoursDaily::ConstraintStudentsSetMinHoursDaily(double wp, int minnh, QString s, bool _allowEmptyDays)
	: TimeConstraint(wp)
{
	this->minHoursDaily = minnh;
	this->students = s;
	this->type = CONSTRAINT_STUDENTS_SET_MIN_HOURS_DAILY;
	
	this->allowEmptyDays=_allowEmptyDays;
}

bool ConstraintStudentsSetMinHoursDaily::hasInactiveActivities(Rules& r)
{
	Q_UNUSED(r);
	return false;
}

QString ConstraintStudentsSetMinHoursDaily::getXmlDescription(Rules& r)
{
	//to avoid non-used parameter warning
	Q_UNUSED(r);

	QString s="<ConstraintStudentsSetMinHoursDaily>\n";
	s+="	<Weight_Percentage>"+QString::number(this->weightPercentage)+"</Weight_Percentage>\n";
	s+="	<Minimum_Hours_Daily>"+QString::number(this->minHoursDaily)+"</Minimum_Hours_Daily>\n";
	s+="	<Students>"+protect(this->students)+"</Students>\n";
	if(this->allowEmptyDays)
		s+="	<Allow_Empty_Days>true</Allow_Empty_Days>\n";
	else
		s+="	<Allow_Empty_Days>false</Allow_Empty_Days>\n";
	s+="</ConstraintStudentsSetMinHoursDaily>\n";
	return s;
}

QString ConstraintStudentsSetMinHoursDaily::getDescription(Rules& r)
{
	//to avoid non-used parameter warning
	Q_UNUSED(r);

	QString s;
	
	if(this->allowEmptyDays)
		s+="! ";
	s+=tr("Students set min hours daily");s+=", ";
	s+=tr("WP:%1\%", "Weight percentage").arg(this->weightPercentage);s+=", ";
	s+=tr("St:%1", "Students (set)").arg(this->students);s+=", ";
	s+=tr("mH:%1", "Min hours (daily)").arg(this->minHoursDaily);s+=", ";
	s+=tr("AED:%1", "Allow empty days").arg(yesNoTranslated(this->allowEmptyDays));

	return s;
}

QString ConstraintStudentsSetMinHoursDaily::getDetailedDescription(Rules& r)
{
	//to avoid non-used parameter warning
	Q_UNUSED(r);

	QString s=tr("Time constraint");s+="\n";
	if(this->allowEmptyDays==true){
		s+=tr("(non standard, students may have empty days)");
		s+="\n";
	}
	s+=tr("A students set must respect the minimum number of hours daily");s+="\n";
	s+=tr("Weight (percentage)=%1\%").arg(this->weightPercentage);s+="\n";
	s+=tr("Students set=%1").arg(this->students);s+="\n";
	s+=tr("Minimum hours daily=%1").arg(this->minHoursDaily);s+="\n";
	s+=tr("Allow empty days=%1").arg(yesNoTranslated(this->allowEmptyDays));s+="\n";
	//s+=tr("Note: FET considers that each day of the week must have the minimum number of working hours, so you cannot have empty days for affected students (constraint is not flexible)");s+="\n";

	return s;
}

bool ConstraintStudentsSetMinHoursDaily::computeInternalStructure(Rules &r)
{
	StudentsSet* ss=r.searchAugmentedStudentsSet(this->students);
	
	if(ss==NULL){
		QMessageBox::warning(NULL, tr("FET warning"),
		 tr("Constraint students set min hours daily is wrong because it refers to inexistent students set."
		 " Please correct it (removing it might be a solution). Please report potential bug. Constraint is:\n%1").arg(this->getDetailedDescription(r)));
		 
		return false;
	}	

	assert(ss);

	this->iSubgroupsList.clear();
	if(ss->type==STUDENTS_SUBGROUP){
		int tmp;
		/*for(tmp=0; tmp<=r.nInternalSubgroups; tmp++)
			if(r.internalSubgroupsList[tmp]->name == ss->name)
				break;*/
		tmp=((StudentsSubgroup*)ss)->indexInInternalSubgroupsList;
		assert(tmp>=0);
		assert(tmp<r.nInternalSubgroups);
		//assert(this->nSubgroups<MAX_SUBGROUPS_PER_CONSTRAINT);
		//this->subgroups[this->nSubgroups++]=tmp;
		if(!this->iSubgroupsList.contains(tmp))
			this->iSubgroupsList.append(tmp);
	}
	else if(ss->type==STUDENTS_GROUP){
		StudentsGroup* stg=(StudentsGroup*)ss;
		for(int i=0; i<stg->subgroupsList.size(); i++){
			StudentsSubgroup* sts=stg->subgroupsList[i];
			int tmp;
			/*for(tmp=0; tmp<=r.nInternalSubgroups; tmp++)
				if(r.internalSubgroupsList[tmp]->name == sts->name)
					break;*/
			tmp=sts->indexInInternalSubgroupsList;
			assert(tmp>=0);
			assert(tmp<r.nInternalSubgroups);
			//assert(this->nSubgroups<MAX_SUBGROUPS_PER_CONSTRAINT);
			//this->subgroups[this->nSubgroups++]=tmp;
			if(!this->iSubgroupsList.contains(tmp))
				this->iSubgroupsList.append(tmp);
		}
	}
	else if(ss->type==STUDENTS_YEAR){
		StudentsYear* sty=(StudentsYear*)ss;
		for(int i=0; i<sty->groupsList.size(); i++){
			StudentsGroup* stg=sty->groupsList[i];
			for(int j=0; j<stg->subgroupsList.size(); j++){
				StudentsSubgroup* sts=stg->subgroupsList[j];
				int tmp;
				/*for(tmp=0; tmp<=r.nInternalSubgroups; tmp++)
					if(r.internalSubgroupsList[tmp]->name == sts->name)
						break;*/
				tmp=sts->indexInInternalSubgroupsList;
				assert(tmp>=0);
				assert(tmp<r.nInternalSubgroups);
				//assert(this->nSubgroups<MAX_SUBGROUPS_PER_CONSTRAINT);
				//this->subgroups[this->nSubgroups++]=tmp;
				if(!this->iSubgroupsList.contains(tmp))
					this->iSubgroupsList.append(tmp);
			}
		}
	}
	else
		assert(0);
		
	return true;
}

double ConstraintStudentsSetMinHoursDaily::fitness(Solution& c, Rules& r, QList<double>& cl, QList<QString>&dl, QString* conflictsString)
{
	//if the matrices subgroupsMatrix and teachersMatrix are already calculated, do not calculate them again!
	if(!c.teachersMatrixReady || !c.subgroupsMatrixReady){
		c.teachersMatrixReady=true;
		c.subgroupsMatrixReady=true;
	//if(crt_chrom!=&c || crt_rules!=&r || subgroups_conflicts<0 || teachers_conflicts<0 || c.changedForMatrixCalculation){
		subgroups_conflicts = c.getSubgroupsMatrix(r, subgroupsMatrix);
		teachers_conflicts = c.getTeachersMatrix(r, teachersMatrix);

		//crt_chrom=&c;
		//crt_rules=&r;
		
		c.changedForMatrixCalculation=false;
	}

	int tmp;
	int too_little;
	
	assert(this->minHoursDaily>=0);

	too_little=0;
	for(int sg=0; sg<this->iSubgroupsList.count(); sg++){
		int i=iSubgroupsList.at(sg);
		for(int j=0; j<r.nDaysPerWeek; j++){
			tmp=0;
			for(int k=0; k<r.nHoursPerDay; k++){
				if(subgroupsMatrix[i][j][k]>=1)
					tmp++;
			}
			
			bool searchDay;
			if(this->allowEmptyDays==true)
				searchDay=(tmp>0);
			else
				searchDay=true;
			
			if(/*tmp>0*/ searchDay && this->minHoursDaily>=0 && tmp < this->minHoursDaily){
				too_little += - tmp + this->minHoursDaily;

				if(conflictsString!=NULL){
					QString s=tr("Time constraint students set min hours daily broken for subgroup: %1, day: %2, lenght=%3, conflicts increase=%4")
					 .arg(r.internalSubgroupsList[i]->name)
					 .arg(r.daysOfTheWeek[j])
					 .arg(QString::number(tmp))
					 .arg((-tmp+this->minHoursDaily)*weightPercentage/100);
						 
					dl.append(s);
					cl.append((-tmp+this->minHoursDaily)*weightPercentage/100);
					
					*conflictsString+= s+"\n";
				}
			}
		}
	}
	
	assert(too_little>=0);

	/*int na=0;
	for(int i=0; i<r.nInternalActivities; i++)
		if(c.times[i]!=UNALLOCATED_TIME)
			na++;*/
	if(c.nPlacedActivities==r.nInternalActivities)
	//if(na==r.nInternalActivities)
		if(weightPercentage==100) //does not work for partial solutions
			assert(too_little==0);

	return too_little * weightPercentage;
}

bool ConstraintStudentsSetMinHoursDaily::isRelatedToActivity(Rules& r, Activity* a)
{
	Q_UNUSED(r);
	Q_UNUSED(a);
	//if(a)
	//	;

	return false;
}

bool ConstraintStudentsSetMinHoursDaily::isRelatedToTeacher(Teacher* t)
{
	Q_UNUSED(t);
	//if(t)
	//	;

	return false;
}

bool ConstraintStudentsSetMinHoursDaily::isRelatedToSubject(Subject* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintStudentsSetMinHoursDaily::isRelatedToActivityTag(ActivityTag* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintStudentsSetMinHoursDaily::isRelatedToStudentsSet(Rules& r, StudentsSet* s)
{
	return r.setsShareStudents(this->students, s->name);
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

ConstraintActivityPreferredStartingTime::ConstraintActivityPreferredStartingTime()
	: TimeConstraint()
{
	this->type = CONSTRAINT_ACTIVITY_PREFERRED_STARTING_TIME;
}

ConstraintActivityPreferredStartingTime::ConstraintActivityPreferredStartingTime(double wp, int actId, int d, int h, bool perm)
	: TimeConstraint(wp)
{
	this->activityId = actId;
	this->day = d;
	this->hour = h;
	this->type = CONSTRAINT_ACTIVITY_PREFERRED_STARTING_TIME;
	this->permanentlyLocked=perm;
}

bool ConstraintActivityPreferredStartingTime::operator==(ConstraintActivityPreferredStartingTime& c){
	if(this->day!=c.day)
		return false;
	if(this->hour!=c.hour)
		return false;
	if(this->activityId!=c.activityId)
		return false;
	//if(this->compulsory!=c.compulsory)
	//	return false;
	if(this->weightPercentage!=c.weightPercentage)
		return false;
	//no need to care about permanently locked
	return true;
}

bool ConstraintActivityPreferredStartingTime::computeInternalStructure(Rules& r)
{
	Activity* act;
	int i;
	for(i=0; i<r.nInternalActivities; i++){
		act=&r.internalActivitiesList[i];
		if(act->id==this->activityId)
			break;
	}
	
	if(i==r.nInternalActivities){	
		//assert(0);
		QMessageBox::warning(NULL, tr("FET error in data"), 
			tr("Following constraint is wrong (because it refers to invalid activity id. Please correct (maybe removing it is a solution)):\n%1").arg(this->getDetailedDescription(r)));
		return false;
	}

	if(this->day >= r.nDaysPerWeek){
		QMessageBox::information(NULL, tr("FET information"),
		 tr("Constraint activity preferred time is wrong because it refers to removed day. Please correct"
		 " and try again. Correcting means editing the constraint and updating information. Constraint is:\n%1").arg(this->getDetailedDescription(r)));
		 
		return false;
	}
	if(this->hour == r.nHoursPerDay){
		QMessageBox::information(NULL, tr("FET information"),
		 tr("Constraint activity preferred time is wrong because preferred hour is too late (after the last acceptable slot). Please correct"
		 " and try again. Correcting means editing the constraint and updating information. Constraint is:\n%1").arg(this->getDetailedDescription(r)));
		 
		return false;
	}
	if(this->hour > r.nHoursPerDay){
		QMessageBox::information(NULL, tr("FET information"),
		 tr("Constraint activity preferred time is wrong because it refers to removed hour. Please correct"
		 " and try again. Correcting means editing the constraint and updating information. Constraint is:\n%1").arg(this->getDetailedDescription(r)));
		 
		return false;
	}

	this->activityIndex=i;	
	return true;
}

bool ConstraintActivityPreferredStartingTime::hasInactiveActivities(Rules& r)
{
	if(r.inactiveActivities.contains(this->activityId))
		return true;
	return false;
}

QString ConstraintActivityPreferredStartingTime::getXmlDescription(Rules& r)
{
	//to avoid non-used parameter warning
	Q_UNUSED(r);

	QString s="<ConstraintActivityPreferredStartingTime>\n";
	s+="	<Weight_Percentage>"+QString::number(this->weightPercentage)+"</Weight_Percentage>\n";
	//s+="	<Compulsory>";s+=yesNo(this->compulsory);s+="</Compulsory>\n";
	s+="	<Activity_Id>"+QString::number(this->activityId)+"</Activity_Id>\n";
	if(this->day>=0)
		s+="	<Preferred_Day>"+protect(r.daysOfTheWeek[this->day])+"</Preferred_Day>\n";
	if(this->hour>=0)
		s+="	<Preferred_Hour>"+protect(r.hoursOfTheDay[this->hour])+"</Preferred_Hour>\n";
	s+="	<Permanently_Locked>";s+=trueFalse(this->permanentlyLocked);s+="</Permanently_Locked>\n";
	s+="</ConstraintActivityPreferredStartingTime>\n";
	return s;
}

QString ConstraintActivityPreferredStartingTime::getDescription(Rules& r)
{
	//to avoid non-used parameter warning
	Q_UNUSED(r);

	QString s;
	s+=tr("Act. id: %1 (%2) has a preferred starting time: %3", "%1 is the id, %2 is the detailed description of the activity. %3 is time (day and hour)")
	 .arg(this->activityId)
	 .arg(getActivityDetailedDescription(r, this->activityId))
	 .arg(r.daysOfTheWeek[this->day]+" "+r.hoursOfTheDay[this->hour]);

	s+=", ";

	s+=tr("WP:%1\%", "Weight percentage").arg(this->weightPercentage);
	s+=", ";
	s+=tr("PL:%1", "Abbreviation for permanently locked").arg(yesNoTranslated(this->permanentlyLocked));

	return s;
}

QString ConstraintActivityPreferredStartingTime::getDetailedDescription(Rules& r)
{
	QString s=tr("Time constraint");s+="\n";
	s+=tr("Activity with id=%1 (%2)", "%1 is the id, %2 is the detailed description of the activity")
		.arg(this->activityId)
		.arg(getActivityDetailedDescription(r, this->activityId));
	s+="\n";

	s+=tr("has a preferred starting time:");
	s+="\n";
	s+=tr("Day=%1").arg(r.daysOfTheWeek[this->day]);
	s+="\n";
	s+=tr("Hour=%1").arg(r.hoursOfTheDay[this->hour]);
	s+="\n";

	s+=tr("Weight (percentage)=%1\%").arg(this->weightPercentage);s+="\n";
	if(this->permanentlyLocked){
		s+=tr("This activity is permanently locked, which means you cannot unlock it from the 'Timetable' menu"
		" (you can unlock this activity by removing the constraint from the constraints dialog or by setting the 'permanently"
		" locked' attribute false when editing this constraint)");
	}
	else{
		s+=tr("This activity is not permanently locked, which means you can unlock it from the 'Timetable' menu");
	}
	s+="\n";

	return s;
}

double ConstraintActivityPreferredStartingTime::fitness(Solution& c, Rules& r, QList<double>& cl, QList<QString>&dl, QString* conflictsString)
{
	//if the matrices subgroupsMatrix and teachersMatrix are already calculated, do not calculate them again!
	if(!c.teachersMatrixReady || !c.subgroupsMatrixReady){
		c.teachersMatrixReady=true;
		c.subgroupsMatrixReady=true;
	//if(crt_chrom!=&c || crt_rules!=&r || subgroups_conflicts<0 || teachers_conflicts<0 || c.changedForMatrixCalculation){
		subgroups_conflicts = c.getSubgroupsMatrix(r, subgroupsMatrix);
		teachers_conflicts = c.getTeachersMatrix(r, teachersMatrix);

		//crt_chrom=&c;
		//crt_rules=&r;
		
		c.changedForMatrixCalculation=false;
	}

	int nbroken;

	assert(r.internalStructureComputed);

	nbroken=0;
	if(c.times[this->activityIndex]!=UNALLOCATED_TIME){
		int d=c.times[this->activityIndex]%r.nDaysPerWeek; //the day when this activity was scheduled
		int h=c.times[this->activityIndex]/r.nDaysPerWeek; //the hour
		if(this->day>=0)
			nbroken+=abs(this->day-d);
		if(this->hour>=0)
			nbroken+=abs(this->hour-h);
		/*if(r.internalActivitiesList[this->activityIndex].parity==PARITY_WEEKLY) //for weekly activities, double the conflicts
			nbroken*=2;*/
	}
	if(nbroken>0)
		nbroken=1;

	if(conflictsString!=NULL && nbroken>0){
		QString s=tr("Time constraint activity preferred starting time broken for activity with id=%1 (%2), increases conflicts total by %3",
			"%1 is the id, %2 is the detailed description of the activity")
			.arg(this->activityId)
			.arg(getActivityDetailedDescription(r, this->activityId))
			.arg(weightPercentage/100*nbroken);

		dl.append(s);
		cl.append(weightPercentage/100*nbroken);
	
		*conflictsString+= s+"\n";
	}

	if(weightPercentage==100)
		assert(nbroken==0);
	return nbroken * weightPercentage/100;
}

bool ConstraintActivityPreferredStartingTime::isRelatedToActivity(Rules& r, Activity* a)
{
	Q_UNUSED(r);

	if(this->activityId==a->id)
		return true;
	return false;
}

bool ConstraintActivityPreferredStartingTime::isRelatedToTeacher(Teacher* t)
{
	Q_UNUSED(t);
	//if(t)
	//	;

	return false;
}

bool ConstraintActivityPreferredStartingTime::isRelatedToSubject(Subject* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintActivityPreferredStartingTime::isRelatedToActivityTag(ActivityTag* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintActivityPreferredStartingTime::isRelatedToStudentsSet(Rules& r, StudentsSet* s)
{
	Q_UNUSED(r);
	Q_UNUSED(s);
	/*if(s)
		;
	if(&r)
		;*/
		
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

ConstraintActivityPreferredTimeSlots::ConstraintActivityPreferredTimeSlots()
	: TimeConstraint()
{
	this->type = CONSTRAINT_ACTIVITY_PREFERRED_TIME_SLOTS;
}

ConstraintActivityPreferredTimeSlots::ConstraintActivityPreferredTimeSlots(double wp, int actId, int nPT_L, QList<int> d_L, QList<int> h_L)
	: TimeConstraint(wp)
{
	assert(d_L.count()==nPT_L);
	assert(h_L.count()==nPT_L);

	this->p_activityId=actId;
	this->p_nPreferredTimeSlots_L=nPT_L;
	//assert(nPT<=MAX_N_CONSTRAINT_ACTIVITY_PREFERRED_TIME_SLOTS);
	/*for(int i=0; i<nPT; i++){
		this->p_days[i]=d[i];
		this->p_hours[i]=h[i];
	}*/
	this->p_days_L=d_L;
	this->p_hours_L=h_L;
	this->type=CONSTRAINT_ACTIVITY_PREFERRED_TIME_SLOTS;
}

bool ConstraintActivityPreferredTimeSlots::computeInternalStructure(Rules& r)
{
	Activity* act;
	int i;
	for(i=0; i<r.nInternalActivities; i++){
		act=&r.internalActivitiesList[i];
		if(act->id==this->p_activityId)
			break;
	}

	if(i==r.nInternalActivities){
		QMessageBox::warning(NULL, tr("FET error in data"), 
			tr("Following constraint is wrong (because it refers to invalid activity id. Please correct it (maybe removing it is a solution)):\n%1").arg(this->getDetailedDescription(r)));
		//assert(0);
		return false;
	}

	for(int k=0; k<p_nPreferredTimeSlots_L; k++){
		if(this->p_days_L[k] >= r.nDaysPerWeek){
			QMessageBox::information(NULL, tr("FET information"),
			 tr("Constraint activity preferred time slots is wrong because it refers to removed day. Please correct"
			 " and try again. Correcting means editing the constraint and updating information. Constraint is:\n%1").arg(this->getDetailedDescription(r)));
		 
			return false;
		}		
		if(this->p_hours_L[k] == r.nHoursPerDay){
			QMessageBox::information(NULL, tr("FET information"),
			 tr("Constraint activity preferred time slots is wrong because a preferred hour is too late (after the last acceptable slot). Please correct"
			 " and try again. Correcting means editing the constraint and updating information. Constraint is:\n%1").arg(this->getDetailedDescription(r)));
		 
			return false;
		}
		if(this->p_hours_L[k] > r.nHoursPerDay){
			QMessageBox::information(NULL, tr("FET information"),
			 tr("Constraint activity preferred time slots is wrong because it refers to removed hour. Please correct"
			 " and try again. Correcting means editing the constraint and updating information. Constraint is:\n%1").arg(this->getDetailedDescription(r)));
		 
			return false;
		}

		if(this->p_hours_L[k]<0 || this->p_days_L[k]<0){
			QMessageBox::information(NULL, tr("FET information"),
			 tr("Constraint activity preferred time slots is wrong because it has hour or day not specified for a slot (-1). Please correct"
			 " and try again. Correcting means editing the constraint and updating information. Constraint is:\n%1").arg(this->getDetailedDescription(r)));
		 
			return false;
		}
	}

	this->p_activityIndex=i;	
	return true;
}

bool ConstraintActivityPreferredTimeSlots::hasInactiveActivities(Rules& r)
{
	if(r.inactiveActivities.contains(this->p_activityId))
		return true;
	return false;
}

QString ConstraintActivityPreferredTimeSlots::getXmlDescription(Rules& r)
{
	//to avoid non-used parameter warning

	QString s="<ConstraintActivityPreferredTimeSlots>\n";
	s+="	<Weight_Percentage>"+QString::number(this->weightPercentage)+"</Weight_Percentage>\n";
	//s+="	<Compulsory>";s+=yesNo(this->compulsory);s+="</Compulsory>\n";
	s+="	<Activity_Id>"+QString::number(this->p_activityId)+"</Activity_Id>\n";
	s+="	<Number_of_Preferred_Time_Slots>"+QString::number(this->p_nPreferredTimeSlots_L)+"</Number_of_Preferred_Time_Slots>\n";
	for(int i=0; i<p_nPreferredTimeSlots_L; i++){
		s+="	<Preferred_Time_Slot>\n";
		if(this->p_days_L[i]>=0)
			s+="		<Preferred_Day>"+protect(r.daysOfTheWeek[this->p_days_L[i]])+"</Preferred_Day>\n";
		if(this->p_hours_L[i]>=0)
			s+="		<Preferred_Hour>"+protect(r.hoursOfTheDay[this->p_hours_L[i]])+"</Preferred_Hour>\n";
		s+="	</Preferred_Time_Slot>\n";
	}
	s+="</ConstraintActivityPreferredTimeSlots>\n";
	return s;
}

QString ConstraintActivityPreferredTimeSlots::getDescription(Rules& r)
{
	//to avoid non-used parameter warning

	QString s;
	s+=tr("Act. id: %1 (%2)", "%1 is the id, %2 is the detailed description of the activity")
		.arg(this->p_activityId)
		.arg(getActivityDetailedDescription(r, this->p_activityId));
	s+=" ";

	s+=tr("has a set of preferred time slots:");
	//s+=tr("must be scheduled in the allowed slots:");
	s+=" ";
	for(int i=0; i<this->p_nPreferredTimeSlots_L; i++){
		//s+=QString::number(i+1);
		//s+=":";
		if(this->p_days_L[i]>=0){
			s+=r.daysOfTheWeek[this->p_days_L[i]];
			s+=" ";
		}
		if(this->p_hours_L[i]>=0){
			s+=r.hoursOfTheDay[this->p_hours_L[i]];
		}
		if(i<this->p_nPreferredTimeSlots_L-1)
			s+="; ";
	}
	s+=", ";

	s+=tr("WP:%1", "Weight percentage").arg(this->weightPercentage);//s+=", ";

	return s;
}

QString ConstraintActivityPreferredTimeSlots::getDetailedDescription(Rules& r)
{
	QString s=tr("Time constraint");s+="\n";
	s+=tr("Activity with id=%1 (%2)", "%1 is the id, %2 is the detailed description of the activity")
		.arg(this->p_activityId)
		.arg(getActivityDetailedDescription(r, this->p_activityId));
	s+="\n";

	s+=tr("has a set of preferred time slots (all hours of the activity must be in the allowed slots):");
	s+="\n";
	for(int i=0; i<this->p_nPreferredTimeSlots_L; i++){
		//s+=QString::number(i+1);
		//s+=". ";
		if(this->p_days_L[i]>=0){
			s+=r.daysOfTheWeek[this->p_days_L[i]];
			s+=" ";
		}
		if(this->p_hours_L[i]>=0){
			s+=r.hoursOfTheDay[this->p_hours_L[i]];
		}
		if(i<this->p_nPreferredTimeSlots_L-1)
			s+=";  ";
	}
	s+="\n";

	s+=tr("Weight (percentage)=%1\%").arg(this->weightPercentage);s+="\n";

	return s;
}

double ConstraintActivityPreferredTimeSlots::fitness(Solution& c, Rules& r, QList<double>& cl, QList<QString>&dl, QString* conflictsString)
{
	//if the matrices subgroupsMatrix and teachersMatrix are already calculated, do not calculate them again!
	if(!c.teachersMatrixReady || !c.subgroupsMatrixReady){
		c.teachersMatrixReady=true;
		c.subgroupsMatrixReady=true;
	//if(crt_chrom!=&c || crt_rules!=&r || subgroups_conflicts<0 || teachers_conflicts<0 || c.changedForMatrixCalculation){
		subgroups_conflicts = c.getSubgroupsMatrix(r, subgroupsMatrix);
		teachers_conflicts = c.getTeachersMatrix(r, teachersMatrix);

		//crt_chrom=&c;
		//crt_rules=&r;
		
		c.changedForMatrixCalculation=false;
	}

	int nbroken;

	assert(r.internalStructureComputed);
	
	Matrix2D<bool> allowed;
	allowed.resize(r.nDaysPerWeek, r.nHoursPerDay);
	//bool allowed[MAX_DAYS_PER_WEEK][MAX_HOURS_PER_DAY];
	for(int d=0; d<r.nDaysPerWeek; d++)
		for(int h=0; h<r.nHoursPerDay; h++)
			allowed[d][h]=false;
	for(int i=0; i<this->p_nPreferredTimeSlots_L; i++){
		if(this->p_days_L[i]>=0 && this->p_hours_L[i]>=0)
			allowed[this->p_days_L[i]][this->p_hours_L[i]]=true;
		else
			assert(0);
	}

	nbroken=0;
	if(c.times[this->p_activityIndex]!=UNALLOCATED_TIME){
		int d=c.times[this->p_activityIndex]%r.nDaysPerWeek; //the day when this activity was scheduled
		int h=c.times[this->p_activityIndex]/r.nDaysPerWeek; //the hour
		for(int dur=0; dur<r.internalActivitiesList[this->p_activityIndex].duration; dur++)
			if(!allowed[d][h+dur])
				nbroken++;
	}

	if(conflictsString!=NULL && nbroken>0){
		QString s=tr("Time constraint activity preferred time slots broken for activity with id=%1 (%2) on %3 hours, increases conflicts total by %4",
		 "%1 is the id, %2 is the detailed description of the activity.")
		 .arg(this->p_activityId)
		 .arg(getActivityDetailedDescription(r, this->p_activityId))
		 .arg(nbroken)
		 .arg(weightPercentage/100*nbroken);
		 
		dl.append(s);
		cl.append(weightPercentage/100*nbroken);
	
		*conflictsString+= s+"\n";
	}

	if(weightPercentage==100)
		assert(nbroken==0);
	return nbroken * weightPercentage/100;
}

bool ConstraintActivityPreferredTimeSlots::isRelatedToActivity(Rules& r, Activity* a)
{
	Q_UNUSED(r);

	if(this->p_activityId==a->id)
		return true;
	return false;
}

bool ConstraintActivityPreferredTimeSlots::isRelatedToTeacher(Teacher* t)
{
	Q_UNUSED(t);
	//if(t)
	//	;

	return false;
}

bool ConstraintActivityPreferredTimeSlots::isRelatedToSubject(Subject* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintActivityPreferredTimeSlots::isRelatedToActivityTag(ActivityTag* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintActivityPreferredTimeSlots::isRelatedToStudentsSet(Rules& r, StudentsSet* s)
{
	Q_UNUSED(r);
	Q_UNUSED(s);
	/*if(s)
		;
	if(&r)
		;*/
		
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

ConstraintActivitiesPreferredTimeSlots::ConstraintActivitiesPreferredTimeSlots()
	: TimeConstraint()
{
	this->type = CONSTRAINT_ACTIVITIES_PREFERRED_TIME_SLOTS;
}

ConstraintActivitiesPreferredTimeSlots::ConstraintActivitiesPreferredTimeSlots(double wp, QString te,
	QString st, QString su, QString sut, int nPT_L, QList<int> d_L, QList<int> h_L)
	: TimeConstraint(wp)
{
	assert(d_L.count()==nPT_L);
	assert(h_L.count()==nPT_L);

	this->p_teacherName=te;
	this->p_subjectName=su;
	this->p_activityTagName=sut;
	this->p_studentsName=st;
	this->p_nPreferredTimeSlots_L=nPT_L;
	/*assert(nPT<=MAX_N_CONSTRAINT_ACTIVITIES_PREFERRED_TIME_SLOTS);
	for(int i=0; i<nPT; i++){
		this->p_days[i]=d[i];
		this->p_hours[i]=h[i];
	}*/
	this->p_days_L=d_L;
	this->p_hours_L=h_L;
	this->type=CONSTRAINT_ACTIVITIES_PREFERRED_TIME_SLOTS;
}

bool ConstraintActivitiesPreferredTimeSlots::computeInternalStructure(Rules& r)
{
	//assert(this->teacherName!="" || this->studentsName!="" || this->subjectName!="" || this->subjectTagName!="");

	this->p_nActivities=0;
	this->p_activitiesIndices.clear();

	QStringList::iterator it;
	Activity* act;
	int i;
	for(i=0; i<r.nInternalActivities; i++){
		act=&r.internalActivitiesList[i];

		//check if this activity has the corresponding teacher
		if(this->p_teacherName!=""){
			it = act->teachersNames.find(this->p_teacherName);
			if(it==act->teachersNames.end())
				continue;
		}
		//check if this activity has the corresponding students
		if(this->p_studentsName!=""){
			bool commonStudents=false;
			foreach(QString st, act->studentsNames)
				if(r.setsShareStudents(st, p_studentsName)){
					commonStudents=true;
					break;
				}
		
			/*it = act->studentsNames.find(this->studentsName);
			if(it==act->studentsNames.end())
				continue;*/
			if(!commonStudents)
				continue;
		}
		//check if this activity has the corresponding subject
		if(this->p_subjectName!="" && act->subjectName!=this->p_subjectName){
				continue;
		}
		//check if this activity has the corresponding activity tag
		//if(this->p_activityTagName!="" && act->activityTagName!=this->p_activityTagName){
		if(this->p_activityTagName!="" && !act->activityTagsNames.contains(this->p_activityTagName)){
				continue;
		}
	
		assert(this->p_nActivities < MAX_ACTIVITIES);
		this->p_nActivities++;
		//this->p_activitiesIndices[this->p_nActivities++]=i;
		this->p_activitiesIndices.append(i);
	}
	
	assert(this->p_nActivities==this->p_activitiesIndices.count());

	//////////////////////	
	for(int k=0; k<p_nPreferredTimeSlots_L; k++){
		if(this->p_days_L[k] >= r.nDaysPerWeek){
			QMessageBox::information(NULL, tr("FET information"),
			 tr("Constraint activities preferred time slots is wrong because it refers to removed day. Please correct"
			 " and try again. Correcting means editing the constraint and updating information. Constraint is:\n%1").arg(this->getDetailedDescription(r)));
		 
			return false;
		}
		if(this->p_hours_L[k] == r.nHoursPerDay){
			QMessageBox::information(NULL, tr("FET information"),
			 tr("Constraint activities preferred time slots is wrong because a preferred hour is too late (after the last acceptable slot). Please correct"
			 " and try again. Correcting means editing the constraint and updating information. Constraint is:\n%1").arg(this->getDetailedDescription(r)));
		 
			return false;
		}
		if(this->p_hours_L[k] > r.nHoursPerDay){
			QMessageBox::information(NULL, tr("FET information"),
			 tr("Constraint activities preferred time slots is wrong because it refers to removed hour. Please correct"
			 " and try again. Correcting means editing the constraint and updating information. Constraint is:\n%1").arg(this->getDetailedDescription(r)));
		 
			return false;
		}
		if(this->p_hours_L[k]<0 || this->p_days_L[k]<0){
			QMessageBox::information(NULL, tr("FET information"),
			 tr("Constraint activities preferred time slots is wrong because hour or day is not specified for a slot (-1). Please correct"
			 " and try again. Correcting means editing the constraint and updating information. Constraint is:\n%1").arg(this->getDetailedDescription(r)));
		 
			return false;
		}
	}
	///////////////////////
	
	if(this->p_nActivities>0)
		return true;
	else{
		QMessageBox::warning(NULL, tr("FET error in data"), 
			tr("Following constraint is wrong (refers to no activities). Please correct it:\n%1").arg(this->getDetailedDescription(r)));
		return false;
	}
}

bool ConstraintActivitiesPreferredTimeSlots::hasInactiveActivities(Rules& r)
{
	QList<int> localActiveActs;
	QList<int> localAllActs;

	//returns true if all activities are inactive
	QStringList::iterator it;
	Activity* act;
	int i;
	for(i=0; i<r.activitiesList.count(); i++){
		act=r.activitiesList.at(i);

		//check if this activity has the corresponding teacher
		if(this->p_teacherName!=""){
			it = act->teachersNames.find(this->p_teacherName);
			if(it==act->teachersNames.end())
				continue;
		}
		//check if this activity has the corresponding students
		if(this->p_studentsName!=""){
			bool commonStudents=false;
			foreach(QString st, act->studentsNames)
				if(r.setsShareStudents(st, p_studentsName)){
					commonStudents=true;
					break;
				}
		
			/*it = act->studentsNames.find(this->studentsName);
			if(it==act->studentsNames.end())
				continue;*/
			if(!commonStudents)
				continue;
		}
		//check if this activity has the corresponding subject
		if(this->p_subjectName!="" && act->subjectName!=this->p_subjectName){
				continue;
		}
		//check if this activity has the corresponding activity tag
		//if(this->p_activityTagName!="" && act->activityTagName!=this->p_activityTagName){
		if(this->p_activityTagName!="" && !act->activityTagsNames.contains(this->p_activityTagName)){
				continue;
		}
	
		//assert(this->p_nActivities < MAX_ACTIVITIES);	
		//this->p_activitiesIndices[this->p_nActivities++]=i;
		if(!r.inactiveActivities.contains(act->id))
			localActiveActs.append(act->id);
			
		localAllActs.append(act->id);
	}

	if(localActiveActs.count()==0 && localAllActs.count()>0)
	//because if this constraint does not refer to any activity,
	//it should be reported as incorrect
		return true;
	else
		return false;
}

QString ConstraintActivitiesPreferredTimeSlots::getXmlDescription(Rules& r)
{
	QString s="<ConstraintActivitiesPreferredTimeSlots>\n";
	s+="	<Weight_Percentage>"+QString::number(this->weightPercentage)+"</Weight_Percentage>\n";
	//s+="	<Compulsory>";s+=yesNo(this->compulsory);s+="</Compulsory>\n";
	//if(this->teacherName!="")
		s+="	<Teacher_Name>"+protect(this->p_teacherName)+"</Teacher_Name>\n";
	//if(this->studentsName!="")
		s+="	<Students_Name>"+protect(this->p_studentsName)+"</Students_Name>\n";
	//if(this->subjectName!="")
		s+="	<Subject_Name>"+protect(this->p_subjectName)+"</Subject_Name>\n";
	//if(this->subjectTagName!="")
		s+="	<Activity_Tag_Name>"+protect(this->p_activityTagName)+"</Activity_Tag_Name>\n";
	s+="	<Number_of_Preferred_Time_Slots>"+QString::number(this->p_nPreferredTimeSlots_L)+"</Number_of_Preferred_Time_Slots>\n";
	for(int i=0; i<p_nPreferredTimeSlots_L; i++){
		s+="	<Preferred_Time_Slot>\n";
		if(this->p_days_L[i]>=0)
			s+="		<Preferred_Day>"+protect(r.daysOfTheWeek[this->p_days_L[i]])+"</Preferred_Day>\n";
		if(this->p_hours_L[i]>=0)
			s+="		<Preferred_Hour>"+protect(r.hoursOfTheDay[this->p_hours_L[i]])+"</Preferred_Hour>\n";
		s+="	</Preferred_Time_Slot>\n";
	}
	s+="</ConstraintActivitiesPreferredTimeSlots>\n";
	return s;
}

QString ConstraintActivitiesPreferredTimeSlots::getDescription(Rules& r)
{
	QString s;

	QString tc, st, su, at;
	
	if(this->p_teacherName!="")
		tc=tr("teacher=%1").arg(this->p_teacherName);
	else
		tc=tr("all teachers");
		
	if(this->p_studentsName!="")
		st=tr("students=%1").arg(this->p_studentsName);
	else
		st=tr("all students");
		
	if(this->p_subjectName!="")
		su=tr("subject=%1").arg(this->p_subjectName);
	else
		su=tr("all subjects");
		
	if(this->p_activityTagName!="")
		at+=tr("activity tag=%1").arg(this->p_activityTagName);
	else
		at+=tr("all activity tags");
	
	s+=tr("Activities with %1, %2, %3, %4, have a set of preferred time slots:", "%1...%4 are conditions for the activities").arg(tc).arg(st).arg(su).arg(at);
	s+=" ";
	for(int i=0; i<this->p_nPreferredTimeSlots_L; i++){
		//s+=QString::number(i+1);
		//s+=":";
		if(this->p_days_L[i]>=0){
			s+=r.daysOfTheWeek[this->p_days_L[i]];
			s+=" ";
		}
		if(this->p_hours_L[i]>=0){
			s+=r.hoursOfTheDay[this->p_hours_L[i]];
		}
		if(i<this->p_nPreferredTimeSlots_L-1)
			s+="; ";
	}
	s+=", ";

	s+=tr("WP:%1\%", "Weight percentage").arg(this->weightPercentage);//s+=", ";

	return s;
}

QString ConstraintActivitiesPreferredTimeSlots::getDetailedDescription(Rules& r)
{
	QString s=tr("Time constraint");s+="\n";
	s+=tr("Activities with:");s+="\n";

	if(this->p_teacherName!="")
		s+=tr("Teacher=%1").arg(this->p_teacherName);
	else
		s+=tr("All teachers");
	s+="\n";
	if(this->p_studentsName!="")
		s+=tr("Students=%1").arg(this->p_studentsName);
	else
		s+=tr("All students");
	s+="\n";
	if(this->p_subjectName!="")
		s+=tr("Subject=%1").arg(this->p_subjectName);
	else
		s+=tr("All subjects");
	s+="\n";
	if(this->p_activityTagName!="")
		s+=tr("Activity tag=%1").arg(this->p_activityTagName);
	else
		s+=tr("All activity tags");
	s+="\n";

	s+=tr("have a set of preferred time slots (all hours of each affected activity must be in the allowed slots):");
	s+="\n";
	for(int i=0; i<this->p_nPreferredTimeSlots_L; i++){
		//s+=QString::number(i+1);
		//s+=". ";
		if(this->p_days_L[i]>=0){
			s+=r.daysOfTheWeek[this->p_days_L[i]];
			s+=" ";
		}
		if(this->p_hours_L[i]>=0){
			s+=r.hoursOfTheDay[this->p_hours_L[i]];
		}
		if(i<this->p_nPreferredTimeSlots_L-1)
			s+=";  ";
	}
	s+="\n";

	s+=tr("Weight (percentage)=%1").arg(this->weightPercentage);s+="\n";

	return s;
}

double ConstraintActivitiesPreferredTimeSlots::fitness(Solution& c, Rules& r, QList<double>& cl, QList<QString>&dl, QString* conflictsString)
{
	//if the matrices subgroupsMatrix and teachersMatrix are already calculated, do not calculate them again!
	if(!c.teachersMatrixReady || !c.subgroupsMatrixReady){
		c.teachersMatrixReady=true;
		c.subgroupsMatrixReady=true;
	//if(crt_chrom!=&c || crt_rules!=&r || subgroups_conflicts<0 || teachers_conflicts<0 || c.changedForMatrixCalculation){
		subgroups_conflicts = c.getSubgroupsMatrix(r, subgroupsMatrix);
		teachers_conflicts = c.getTeachersMatrix(r, teachersMatrix);

		//crt_chrom=&c;
		//crt_rules=&r;
		
		c.changedForMatrixCalculation=false;
	}

	int nbroken;

	assert(r.internalStructureComputed);

///////////////////
	Matrix2D<bool> allowed;
	allowed.resize(r.nDaysPerWeek, r.nHoursPerDay);
	//bool allowed[MAX_DAYS_PER_WEEK][MAX_HOURS_PER_DAY];
	for(int d=0; d<r.nDaysPerWeek; d++)
		for(int h=0; h<r.nHoursPerDay; h++)
			allowed[d][h]=false;
	for(int i=0; i<this->p_nPreferredTimeSlots_L; i++){
		if(this->p_days_L[i]>=0 && this->p_hours_L[i]>=0)
			allowed[this->p_days_L[i]][this->p_hours_L[i]]=true;
		else
			assert(0);
	}
////////////////////

	nbroken=0;
	int tmp;
	
	for(int i=0; i<this->p_nActivities; i++){
		tmp=0;
		int ai=this->p_activitiesIndices[i];
		if(c.times[ai]!=UNALLOCATED_TIME){
			int d=c.times[ai]%r.nDaysPerWeek; //the day when this activity was scheduled
			int h=c.times[ai]/r.nDaysPerWeek; //the hour
			
			for(int dur=0; dur<r.internalActivitiesList[ai].duration; dur++)
				if(!allowed[d][h+dur])
					tmp++;
		}
		nbroken+=tmp;
		if(conflictsString!=NULL && tmp>0){
			QString s=tr("Time constraint activities preferred time slots broken"
			 " for activity with id=%1 (%2) on %3 hours,"
			 " increases conflicts total by %4", "%1 is the id, %2 is the detailed description of the activity.")
			 .arg(r.internalActivitiesList[ai].id)
			 .arg(getActivityDetailedDescription(r, r.internalActivitiesList[ai].id))
			 .arg(tmp)
			 .arg(weightPercentage/100*tmp);
				 
			dl.append(s);
			cl.append(weightPercentage/100*tmp);
		
			*conflictsString+= s+"\n";
		}
	}

	if(weightPercentage==100)
		assert(nbroken==0);
	return nbroken * weightPercentage;
}

bool ConstraintActivitiesPreferredTimeSlots::isRelatedToActivity(Rules& r, Activity* a)
{
	QStringList::iterator it;

	//check if this activity has the corresponding teacher
	if(this->p_teacherName!=""){
		it = a->teachersNames.find(this->p_teacherName);
		if(it==a->teachersNames.end())
			return false;
	}
	//check if this activity has the corresponding students
	if(this->p_studentsName!=""){
		bool commonStudents=false;
		foreach(QString st, a->studentsNames){
			if(r.setsShareStudents(st, this->p_studentsName)){
				commonStudents=true;
				break;
			}
		}
		if(!commonStudents)
			return false;

		//it = a->studentsNames.find(this->p_studentsName);
		//if(it==a->studentsNames.end())
		//	return false;
	}
	//check if this activity has the corresponding subject
	if(this->p_subjectName!="" && a->subjectName!=this->p_subjectName)
		return false;
	//check if this activity has the corresponding activity tag
	if(this->p_activityTagName!="" && !a->activityTagsNames.contains(this->p_activityTagName))
		return false;

	return true;
}

bool ConstraintActivitiesPreferredTimeSlots::isRelatedToTeacher(Teacher* t)
{
	Q_UNUSED(t);
	//if(t)
	//	;

	return false;
}

bool ConstraintActivitiesPreferredTimeSlots::isRelatedToSubject(Subject* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintActivitiesPreferredTimeSlots::isRelatedToActivityTag(ActivityTag* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintActivitiesPreferredTimeSlots::isRelatedToStudentsSet(Rules& r, StudentsSet* s)
{
	Q_UNUSED(r);
	Q_UNUSED(s);
	/*if(s)
		;
	if(&r)
		;*/
		
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

ConstraintSubactivitiesPreferredTimeSlots::ConstraintSubactivitiesPreferredTimeSlots()
	: TimeConstraint()
{
	this->type = CONSTRAINT_SUBACTIVITIES_PREFERRED_TIME_SLOTS;
}

ConstraintSubactivitiesPreferredTimeSlots::ConstraintSubactivitiesPreferredTimeSlots(double wp, int compNo, QString te,
	QString st, QString su, QString sut, int nPT_L, QList<int> d_L, QList<int> h_L)
	: TimeConstraint(wp)
{
	assert(d_L.count()==nPT_L);
	assert(h_L.count()==nPT_L);

	this->componentNumber=compNo;
	this->p_teacherName=te;
	this->p_subjectName=su;
	this->p_activityTagName=sut;
	this->p_studentsName=st;
	this->p_nPreferredTimeSlots_L=nPT_L;
	/*assert(nPT<=MAX_N_CONSTRAINT_SUBACTIVITIES_PREFERRED_TIME_SLOTS);
	for(int i=0; i<nPT; i++){
		this->p_days[i]=d[i];
		this->p_hours[i]=h[i];
	}*/
	this->p_days_L=d_L;
	this->p_hours_L=h_L;
	this->type=CONSTRAINT_SUBACTIVITIES_PREFERRED_TIME_SLOTS;
}

bool ConstraintSubactivitiesPreferredTimeSlots::computeInternalStructure(Rules& r)
{
	//assert(this->teacherName!="" || this->studentsName!="" || this->subjectName!="" || this->subjectTagName!="");

	this->p_nActivities=0;
	this->p_activitiesIndices.clear();

	QStringList::iterator it;
	Activity* act;
	int i;
	for(i=0; i<r.nInternalActivities; i++){
		act=&r.internalActivitiesList[i];
		
		if(!act->representsComponentNumber(this->componentNumber))
			continue;

		//check if this activity has the corresponding teacher
		if(this->p_teacherName!=""){
			it = act->teachersNames.find(this->p_teacherName);
			if(it==act->teachersNames.end())
				continue;
		}
		//check if this activity has the corresponding students
		if(this->p_studentsName!=""){
			bool commonStudents=false;
			foreach(QString st, act->studentsNames)
				if(r.setsShareStudents(st, p_studentsName)){
					commonStudents=true;
					break;
				}
		
			/*it = act->studentsNames.find(this->studentsName);
			if(it==act->studentsNames.end())
				continue;*/
			if(!commonStudents)
				continue;
		}
		//check if this activity has the corresponding subject
		if(this->p_subjectName!="" && act->subjectName!=this->p_subjectName){
			continue;
		}
		//check if this activity has the corresponding activity tag
		if(this->p_activityTagName!="" && !act->activityTagsNames.contains(this->p_activityTagName)){
			continue;
		}
	
		assert(this->p_nActivities < MAX_ACTIVITIES);	
		//this->p_activitiesIndices[this->p_nActivities++]=i;
		this->p_nActivities++;
		this->p_activitiesIndices.append(i);
		
		//cout<<endl;
		//cout<<"Activity with id == "<<act->id<<" corresponds to constraint subactivities preferred time slots:"<<endl;
		//cout<<qPrintable(this->getDescription(r))<<endl;
	}

	assert(this->p_nActivities==this->p_activitiesIndices.count());

	//////////////////////	
	for(int k=0; k<p_nPreferredTimeSlots_L; k++){
		if(this->p_days_L[k] >= r.nDaysPerWeek){
			QMessageBox::information(NULL, tr("FET information"),
			 tr("Constraint subactivities preferred time slots is wrong because it refers to removed day. Please correct"
			 " and try again. Correcting means editing the constraint and updating information. Constraint is:\n%1").arg(this->getDetailedDescription(r)));
		 
			return false;
		}
		if(this->p_hours_L[k] == r.nHoursPerDay){
			QMessageBox::information(NULL, tr("FET information"),
			 tr("Constraint subactivities preferred time slots is wrong because a preferred hour is too late (after the last acceptable slot). Please correct"
			 " and try again. Correcting means editing the constraint and updating information. Constraint is:\n%1").arg(this->getDetailedDescription(r)));
		 
			return false;
		}
		if(this->p_hours_L[k] > r.nHoursPerDay){
			QMessageBox::information(NULL, tr("FET information"),
			 tr("Constraint subactivities preferred time slots is wrong because it refers to removed hour. Please correct"
			 " and try again. Correcting means editing the constraint and updating information. Constraint is:\n%1").arg(this->getDetailedDescription(r)));
		 
			return false;
		}
		if(this->p_hours_L[k]<0 || this->p_days_L[k]<0){
			QMessageBox::information(NULL, tr("FET information"),
			 tr("Constraint subactivities preferred time slots is wrong because hour or day is not specified for a slot (-1). Please correct"
			 " and try again. Correcting means editing the constraint and updating information. Constraint is:\n%1").arg(this->getDetailedDescription(r)));
		 
			return false;
		}
	}
	///////////////////////
	
	if(this->p_nActivities>0)
		return true;
	else{
		QMessageBox::warning(NULL, tr("FET error in data"), 
			tr("Following constraint is wrong (refers to no activities). Please correct it:\n%1").arg(this->getDetailedDescription(r)));
		return false;
	}
}

bool ConstraintSubactivitiesPreferredTimeSlots::hasInactiveActivities(Rules& r)
{
	QList<int> localActiveActs;
	QList<int> localAllActs;

	//returns true if all activities are inactive
	QStringList::iterator it;
	Activity* act;
	int i;
	for(i=0; i<r.activitiesList.count(); i++){
		act=r.activitiesList.at(i);

		if(!act->representsComponentNumber(this->componentNumber))
			continue;

		//check if this activity has the corresponding teacher
		if(this->p_teacherName!=""){
			it = act->teachersNames.find(this->p_teacherName);
			if(it==act->teachersNames.end())
				continue;
		}
		//check if this activity has the corresponding students
		if(this->p_studentsName!=""){
			bool commonStudents=false;
			foreach(QString st, act->studentsNames)
				if(r.setsShareStudents(st, p_studentsName)){
					commonStudents=true;
					break;
				}
		
			/*it = act->studentsNames.find(this->studentsName);
			if(it==act->studentsNames.end())
				continue;*/
			if(!commonStudents)
				continue;
		}
		//check if this activity has the corresponding subject
		if(this->p_subjectName!="" && act->subjectName!=this->p_subjectName){
				continue;
		}
		//check if this activity has the corresponding activity tag
		//if(this->p_activityTagName!="" && act->activityTagName!=this->p_activityTagName){
		if(this->p_activityTagName!="" && !act->activityTagsNames.contains(this->p_activityTagName)){
				continue;
		}
	
		//assert(this->p_nActivities < MAX_ACTIVITIES);	
		//this->p_activitiesIndices[this->p_nActivities++]=i;
		if(!r.inactiveActivities.contains(act->id))
			localActiveActs.append(act->id);
			
		localAllActs.append(act->id);
	}

	if(localActiveActs.count()==0 && localAllActs.count()>0)
		return true;
	else
		return false;
}

QString ConstraintSubactivitiesPreferredTimeSlots::getXmlDescription(Rules& r)
{
	//to avoid non-used parameter warning
	//Q_UNUSED(r);
	//if(&r==NULL)
	//	;

	QString s="<ConstraintSubactivitiesPreferredTimeSlots>\n";
	s+="	<Weight_Percentage>"+QString::number(this->weightPercentage)+"</Weight_Percentage>\n";
	//s+="	<Compulsory>";s+=yesNo(this->compulsory);s+="</Compulsory>\n";
	//if(this->teacherName!="")
	s+="	<Component_Number>"+QString::number(this->componentNumber)+"</Component_Number>\n";
		s+="	<Teacher_Name>"+protect(this->p_teacherName)+"</Teacher_Name>\n";
	//if(this->studentsName!="")
		s+="	<Students_Name>"+protect(this->p_studentsName)+"</Students_Name>\n";
	//if(this->subjectName!="")
		s+="	<Subject_Name>"+protect(this->p_subjectName)+"</Subject_Name>\n";
	//if(this->subjectTagName!="")
		s+="	<Activity_Tag_Name>"+protect(this->p_activityTagName)+"</Activity_Tag_Name>\n";
	s+="	<Number_of_Preferred_Time_Slots>"+QString::number(this->p_nPreferredTimeSlots_L)+"</Number_of_Preferred_Time_Slots>\n";
	for(int i=0; i<p_nPreferredTimeSlots_L; i++){
		s+="	<Preferred_Time_Slot>\n";
		if(this->p_days_L[i]>=0)
			s+="		<Preferred_Day>"+protect(r.daysOfTheWeek[this->p_days_L[i]])+"</Preferred_Day>\n";
		if(this->p_hours_L[i]>=0)
			s+="		<Preferred_Hour>"+protect(r.hoursOfTheDay[this->p_hours_L[i]])+"</Preferred_Hour>\n";
		s+="	</Preferred_Time_Slot>\n";
	}
	s+="</ConstraintSubactivitiesPreferredTimeSlots>\n";
	return s;
}

QString ConstraintSubactivitiesPreferredTimeSlots::getDescription(Rules& r)
{
	//to avoid non-used parameter warning
	//Q_UNUSED(r);
	//if(&r==NULL)
	//	;

	QString s;
	
	QString tc, st, su, at;
	
	if(this->p_teacherName!="")
		tc=tr("teacher=%1").arg(this->p_teacherName);
	else
		tc=tr("all teachers");
		
	if(this->p_studentsName!="")
		st=tr("students=%1").arg(this->p_studentsName);
	else
		st=tr("all students");
		
	if(this->p_subjectName!="")
		su=tr("subject=%1").arg(this->p_subjectName);
	else
		su=tr("all subjects");
		
	if(this->p_activityTagName!="")
		at+=tr("activity tag=%1").arg(this->p_activityTagName);
	else
		at+=tr("all activity tags");
	
	s+=tr("Subactivities with %1, %2, %3, %4, %5, have a set of preferred time slots:", "%1...%5 are conditions for the subactivities")
		.arg(tr("component number=%1").arg(this->componentNumber)).arg(tc).arg(st).arg(su).arg(at);
		
	s+=" ";
	
	for(int i=0; i<this->p_nPreferredTimeSlots_L; i++){
		//s+=QString::number(i+1);
		//s+=":";
		if(this->p_days_L[i]>=0){
			s+=r.daysOfTheWeek[this->p_days_L[i]];
			s+=" ";
		}
		if(this->p_hours_L[i]>=0){
			s+=r.hoursOfTheDay[this->p_hours_L[i]];
		}
		if(i<this->p_nPreferredTimeSlots_L-1)
			s+="; ";
	}
	s+=", ";

	s+=tr("WP:%1\%", "Weight percentage").arg(this->weightPercentage);//s+=", ";

	return s;
}

QString ConstraintSubactivitiesPreferredTimeSlots::getDetailedDescription(Rules& r)
{
	QString s=tr("Time constraint");s+="\n";
	s+=tr("Subactivities with:");s+="\n";
	
	s+=tr("Component number=%1").arg(this->componentNumber);
	s+="\n";

	if(this->p_teacherName!="")
		s+=tr("Teacher=%1").arg(this->p_teacherName);
	else
		s+=tr("All teachers");
	s+="\n";
		
	if(this->p_studentsName!="")
		s+=tr("Students=%1").arg(this->p_studentsName);
	else
		s+=tr("All students");
	s+="\n";
	
	if(this->p_subjectName!="")
		s+=tr("Subject=%1").arg(this->p_subjectName);
	else
		s+=tr("All subjects");
	s+="\n";
	
	if(this->p_activityTagName!="")
		s+=tr("Activity tag=%1").arg(this->p_activityTagName);
	else
		s+=tr("All activity tags");
	s+="\n";

	s+=tr("have a set of preferred time slots (all hours of each affected subactivity must be in the allowed slots):");
	s+="\n";
	for(int i=0; i<this->p_nPreferredTimeSlots_L; i++){
		//s+=QString::number(i+1);
		//s+=". ";
		if(this->p_days_L[i]>=0){
			s+=r.daysOfTheWeek[this->p_days_L[i]];
			s+=" ";
		}
		if(this->p_hours_L[i]>=0){
			s+=r.hoursOfTheDay[this->p_hours_L[i]];
		}
		if(i<this->p_nPreferredTimeSlots_L-1)
			s+=";  ";
	}
	s+="\n";

	s+=tr("Weight (percentage)=%1").arg(this->weightPercentage);s+="\n";

	return s;
}

double ConstraintSubactivitiesPreferredTimeSlots::fitness(Solution& c, Rules& r, QList<double>& cl, QList<QString>&dl, QString* conflictsString)
{
	//if the matrices subgroupsMatrix and teachersMatrix are already calculated, do not calculate them again!
	if(!c.teachersMatrixReady || !c.subgroupsMatrixReady){
		c.teachersMatrixReady=true;
		c.subgroupsMatrixReady=true;
	//if(crt_chrom!=&c || crt_rules!=&r || subgroups_conflicts<0 || teachers_conflicts<0 || c.changedForMatrixCalculation){
		subgroups_conflicts = c.getSubgroupsMatrix(r, subgroupsMatrix);
		teachers_conflicts = c.getTeachersMatrix(r, teachersMatrix);

		//crt_chrom=&c;
		//crt_rules=&r;
		
		c.changedForMatrixCalculation=false;
	}

	int nbroken;

	assert(r.internalStructureComputed);

///////////////////
	Matrix2D<bool> allowed;
	allowed.resize(r.nDaysPerWeek, r.nHoursPerDay);
	//bool allowed[MAX_DAYS_PER_WEEK][MAX_HOURS_PER_DAY];
	for(int d=0; d<r.nDaysPerWeek; d++)
		for(int h=0; h<r.nHoursPerDay; h++)
			allowed[d][h]=false;
	for(int i=0; i<this->p_nPreferredTimeSlots_L; i++){
		if(this->p_days_L[i]>=0 && this->p_hours_L[i]>=0)
			allowed[this->p_days_L[i]][this->p_hours_L[i]]=true;
		else
			assert(0);
	}
////////////////////

	nbroken=0;
	int tmp;
	
	for(int i=0; i<this->p_nActivities; i++){
		tmp=0;
		int ai=this->p_activitiesIndices[i];
		if(c.times[ai]!=UNALLOCATED_TIME){
			int d=c.times[ai]%r.nDaysPerWeek; //the day when this activity was scheduled
			int h=c.times[ai]/r.nDaysPerWeek; //the hour
			
			for(int dur=0; dur<r.internalActivitiesList[ai].duration; dur++)
				if(!allowed[d][h+dur])
					tmp++;
		}
		nbroken+=tmp;
		if(conflictsString!=NULL && tmp>0){
			QString s=tr("Time constraint subactivities preferred time slots broken"
			 " for activity with id=%1 (%2), component number %3, on %4 hours,"
			 " increases conflicts total by %5", "%1 is the id, %2 is the detailed description of the activity.")
			 .arg(r.internalActivitiesList[ai].id)
			 .arg(getActivityDetailedDescription(r, r.internalActivitiesList[ai].id))
			 .arg(componentNumber)
			 .arg(tmp)
			 .arg(weightPercentage/100*tmp);

			dl.append(s);
			cl.append(weightPercentage/100*tmp);
		
			*conflictsString+= s+"\n";
		}
	}

	if(weightPercentage==100)
		assert(nbroken==0);
	return nbroken * weightPercentage;
}

bool ConstraintSubactivitiesPreferredTimeSlots::isRelatedToActivity(Rules& r, Activity* a)
{
	if(!a->representsComponentNumber(this->componentNumber))
		return false;

	QStringList::iterator it;

	//check if this activity has the corresponding teacher
	if(this->p_teacherName!=""){
		it = a->teachersNames.find(this->p_teacherName);
		if(it==a->teachersNames.end())
			return false;
	}
	//check if this activity has the corresponding students
	if(this->p_studentsName!=""){
		bool commonStudents=false;
		foreach(QString st, a->studentsNames){
			if(r.setsShareStudents(st, this->p_studentsName)){
				commonStudents=true;
				break;
			}
		}
		if(!commonStudents)
			return false;

		//it = a->studentsNames.find(this->p_studentsName);
		//if(it==a->studentsNames.end())
		//	return false;
	}
	//check if this activity has the corresponding subject
	if(this->p_subjectName!="" && a->subjectName!=this->p_subjectName)
		return false;
	//check if this activity has the corresponding activity tag
	if(this->p_activityTagName!="" && !a->activityTagsNames.contains(this->p_activityTagName))
		return false;

	return true;
}

bool ConstraintSubactivitiesPreferredTimeSlots::isRelatedToTeacher(Teacher* t)
{
	Q_UNUSED(t);
	//if(t)
	//	;

	return false;
}

bool ConstraintSubactivitiesPreferredTimeSlots::isRelatedToSubject(Subject* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintSubactivitiesPreferredTimeSlots::isRelatedToActivityTag(ActivityTag* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintSubactivitiesPreferredTimeSlots::isRelatedToStudentsSet(Rules& r, StudentsSet* s)
{
	Q_UNUSED(r);
	Q_UNUSED(s);
	/*if(s)
		;
	if(&r)
		;*/
		
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

ConstraintActivityPreferredStartingTimes::ConstraintActivityPreferredStartingTimes()
	: TimeConstraint()
{
	this->type = CONSTRAINT_ACTIVITY_PREFERRED_STARTING_TIMES;
}

ConstraintActivityPreferredStartingTimes::ConstraintActivityPreferredStartingTimes(double wp, int actId, int nPT_L, QList<int> d_L, QList<int> h_L)
	: TimeConstraint(wp)
{
	assert(d_L.count()==nPT_L);
	assert(h_L.count()==nPT_L);

	this->activityId=actId;
	this->nPreferredStartingTimes_L=nPT_L;
	/*assert(nPT<=MAX_N_CONSTRAINT_ACTIVITY_PREFERRED_STARTING_TIMES);
	for(int i=0; i<nPT; i++){
		this->days[i]=d[i];
		this->hours[i]=h[i];
	}*/
	this->days_L=d_L;
	this->hours_L=h_L;
	this->type=CONSTRAINT_ACTIVITY_PREFERRED_STARTING_TIMES;
}

bool ConstraintActivityPreferredStartingTimes::computeInternalStructure(Rules& r)
{
	Activity* act;
	int i;
	for(i=0; i<r.nInternalActivities; i++){
		act=&r.internalActivitiesList[i];
		if(act->id==this->activityId)
			break;
	}

	if(i==r.nInternalActivities){
		QMessageBox::warning(NULL, tr("FET error in data"), 
			tr("Following constraint is wrong (because it refers to invalid activity id. Please correct it (maybe removing it is a solution)):\n%1").arg(this->getDetailedDescription(r)));
		//assert(0);
		return false;
	}

	for(int k=0; k<nPreferredStartingTimes_L; k++){
		if(this->days_L[k] >= r.nDaysPerWeek){
			QMessageBox::information(NULL, tr("FET information"),
			 tr("Constraint activity preferred starting times is wrong because it refers to removed day. Please correct"
			 " and try again. Correcting means editing the constraint and updating information. Constraint is:\n%1").arg(this->getDetailedDescription(r)));
		 
			return false;
		}		
		if(this->hours_L[k] == r.nHoursPerDay){
			QMessageBox::information(NULL, tr("FET information"),
			 tr("Constraint activity preferred starting times is wrong because a preferred hour is too late (after the last acceptable slot). Please correct"
			 " and try again. Correcting means editing the constraint and updating information. Constraint is:\n%1").arg(this->getDetailedDescription(r)));
		 
			return false;
		}
		if(this->hours_L[k] > r.nHoursPerDay){
			QMessageBox::information(NULL, tr("FET information"),
			 tr("Constraint activity preferred starting times is wrong because it refers to removed hour. Please correct"
			 " and try again. Correcting means editing the constraint and updating information. Constraint is:\n%1").arg(this->getDetailedDescription(r)));
		 
			return false;
		}
	}

	this->activityIndex=i;	
	return true;
}

bool ConstraintActivityPreferredStartingTimes::hasInactiveActivities(Rules& r)
{
	if(r.inactiveActivities.contains(this->activityId))
		return true;
	return false;
}

QString ConstraintActivityPreferredStartingTimes::getXmlDescription(Rules& r)
{
	QString s="<ConstraintActivityPreferredStartingTimes>\n";
	s+="	<Weight_Percentage>"+QString::number(this->weightPercentage)+"</Weight_Percentage>\n";
	//s+="	<Compulsory>";s+=yesNo(this->compulsory);s+="</Compulsory>\n";
	s+="	<Activity_Id>"+QString::number(this->activityId)+"</Activity_Id>\n";
	s+="	<Number_of_Preferred_Starting_Times>"+QString::number(this->nPreferredStartingTimes_L)+"</Number_of_Preferred_Starting_Times>\n";
	for(int i=0; i<nPreferredStartingTimes_L; i++){
		s+="	<Preferred_Starting_Time>\n";
		if(this->days_L[i]>=0)
			s+="		<Preferred_Starting_Day>"+protect(r.daysOfTheWeek[this->days_L[i]])+"</Preferred_Starting_Day>\n";
		if(this->hours_L[i]>=0)
			s+="		<Preferred_Starting_Hour>"+protect(r.hoursOfTheDay[this->hours_L[i]])+"</Preferred_Starting_Hour>\n";
		s+="	</Preferred_Starting_Time>\n";
	}
	s+="</ConstraintActivityPreferredStartingTimes>\n";
	return s;
}

QString ConstraintActivityPreferredStartingTimes::getDescription(Rules& r)
{
	QString s;
	s+=tr("Act. id: %1 (%2)", "%1 is the id, %2 is the detailed description of the activity.")
		.arg(this->activityId)
		.arg(getActivityDetailedDescription(r, this->activityId));
	
	s+=" ";
	s+=tr("has a set of preferred starting times:");
	s+=" ";
	for(int i=0; i<this->nPreferredStartingTimes_L; i++){
		//s+=QString::number(i+1);
		//s+=":";
		if(this->days_L[i]>=0){
			s+=r.daysOfTheWeek[this->days_L[i]];
			s+=" ";
		}
		if(this->hours_L[i]>=0){
			s+=r.hoursOfTheDay[this->hours_L[i]];
		}
		if(i<nPreferredStartingTimes_L-1)
			s+="; ";
	}
	s+=", ";

	s+=tr("WP:%1", "Weight Percentage").arg(this->weightPercentage);//s+=", ";

	return s;
}

QString ConstraintActivityPreferredStartingTimes::getDetailedDescription(Rules& r)
{
	QString s=tr("Time constraint");s+="\n";
	s+=tr("Activity with id=%1 (%2)", "%1 is the id, %2 is the detailed description of the activity")
		.arg(this->activityId)
		.arg(getActivityDetailedDescription(r, this->activityId));
	
	s+="\n";
	s+=tr("has a set of preferred starting times:");
	s+="\n";
	for(int i=0; i<this->nPreferredStartingTimes_L; i++){
		//s+=QString::number(i+1);
		//s+=". ";
		if(this->days_L[i]>=0){
			s+=r.daysOfTheWeek[this->days_L[i]];
			s+=" ";
		}
		if(this->hours_L[i]>=0){
			s+=r.hoursOfTheDay[this->hours_L[i]];
		}
		//s+="\n";
		if(i<this->nPreferredStartingTimes_L-1)
			s+=";  ";
	}
	s+="\n";

	s+=tr("Weight (percentage)=%1\%").arg(this->weightPercentage);s+="\n";

	return s;
}

double ConstraintActivityPreferredStartingTimes::fitness(Solution& c, Rules& r, QList<double>& cl, QList<QString>&dl, QString* conflictsString)
{
	//if the matrices subgroupsMatrix and teachersMatrix are already calculated, do not calculate them again!
	if(!c.teachersMatrixReady || !c.subgroupsMatrixReady){
		c.teachersMatrixReady=true;
		c.subgroupsMatrixReady=true;
	//if(crt_chrom!=&c || crt_rules!=&r || subgroups_conflicts<0 || teachers_conflicts<0 || c.changedForMatrixCalculation){
		subgroups_conflicts = c.getSubgroupsMatrix(r, subgroupsMatrix);
		teachers_conflicts = c.getTeachersMatrix(r, teachersMatrix);

		//crt_chrom=&c;
		//crt_rules=&r;
		
		c.changedForMatrixCalculation=false;
	}

	int nbroken;

	assert(r.internalStructureComputed);

	nbroken=0;
	if(c.times[this->activityIndex]!=UNALLOCATED_TIME){
		int d=c.times[this->activityIndex]%r.nDaysPerWeek; //the day when this activity was scheduled
		int h=c.times[this->activityIndex]/r.nDaysPerWeek; //the hour
		int i;
		for(i=0; i<this->nPreferredStartingTimes_L; i++){
			if(this->days_L[i]>=0 && this->days_L[i]!=d)
				continue;
			if(this->hours_L[i]>=0 && this->hours_L[i]!=h)
				continue;
			break;
		}
		if(i==this->nPreferredStartingTimes_L){
			nbroken=1;
			/*if(r.internalActivitiesList[this->activityIndex].parity==PARITY_WEEKLY) //for weekly activities, double the conflicts
				nbroken*=2;*/
		}
	}

	if(conflictsString!=NULL && nbroken>0){
		QString s=tr("Time constraint activity preferred starting times broken for activity with id=%1 (%2), increases conflicts total by %3",
		 "%1 is the id, %2 is the detailed description of the activity")
		 .arg(this->activityId)
		 .arg(getActivityDetailedDescription(r, this->activityId))
		 .arg(weightPercentage/100*nbroken);

		dl.append(s);
		cl.append(weightPercentage/100*nbroken);
	
		*conflictsString+= s+"\n";
	}

	if(weightPercentage==100)
		assert(nbroken==0);
	return nbroken * weightPercentage/100;
}

bool ConstraintActivityPreferredStartingTimes::isRelatedToActivity(Rules& r, Activity* a)
{
	Q_UNUSED(r);

	if(this->activityId==a->id)
		return true;
	return false;
}

bool ConstraintActivityPreferredStartingTimes::isRelatedToTeacher(Teacher* t)
{
	Q_UNUSED(t);
	//if(t)
	//	;

	return false;
}

bool ConstraintActivityPreferredStartingTimes::isRelatedToSubject(Subject* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintActivityPreferredStartingTimes::isRelatedToActivityTag(ActivityTag* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintActivityPreferredStartingTimes::isRelatedToStudentsSet(Rules& r, StudentsSet* s)
{
	Q_UNUSED(r);
	Q_UNUSED(s);
	/*if(s)
		;
	if(&r)
		;*/
		
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

ConstraintActivitiesPreferredStartingTimes::ConstraintActivitiesPreferredStartingTimes()
	: TimeConstraint()
{
	this->type = CONSTRAINT_ACTIVITIES_PREFERRED_STARTING_TIMES;
}

ConstraintActivitiesPreferredStartingTimes::ConstraintActivitiesPreferredStartingTimes(double wp, QString te,
	QString st, QString su, QString sut, int nPT_L, QList<int> d_L, QList<int> h_L)
	: TimeConstraint(wp)
{
	assert(d_L.count()==nPT_L);
	assert(h_L.count()==nPT_L);

	this->teacherName=te;
	this->subjectName=su;
	this->activityTagName=sut;
	this->studentsName=st;
	this->nPreferredStartingTimes_L=nPT_L;
	/*assert(nPT<=MAX_N_CONSTRAINT_ACTIVITIES_PREFERRED_STARTING_TIMES);
	for(int i=0; i<nPT; i++){
		this->days[i]=d[i];
		this->hours[i]=h[i];
	}*/
	this->days_L=d_L;
	this->hours_L=h_L;
	this->type=CONSTRAINT_ACTIVITIES_PREFERRED_STARTING_TIMES;
}

bool ConstraintActivitiesPreferredStartingTimes::computeInternalStructure(Rules& r)
{
	//assert(this->teacherName!="" || this->studentsName!="" || this->subjectName!="" || this->subjectTagName!="");

	this->nActivities=0;
	this->activitiesIndices.clear();

	QStringList::iterator it;
	Activity* act;
	int i;
	for(i=0; i<r.nInternalActivities; i++){
		act=&r.internalActivitiesList[i];

		//check if this activity has the corresponding teacher
		if(this->teacherName!=""){
			it = act->teachersNames.find(this->teacherName);
			if(it==act->teachersNames.end())
				continue;
		}
		//check if this activity has the corresponding students
		if(this->studentsName!=""){
			bool commonStudents=false;
			foreach(QString st, act->studentsNames)
				if(r.setsShareStudents(st, studentsName)){
					commonStudents=true;
					break;
				}
		
			/*it = act->studentsNames.find(this->studentsName);
			if(it==act->studentsNames.end())
				continue;*/
			if(!commonStudents)
				continue;
		}
		//check if this activity has the corresponding subject
		if(this->subjectName!="" && act->subjectName!=this->subjectName){
				continue;
		}
		//check if this activity has the corresponding activity tag
		if(this->activityTagName!="" && !act->activityTagsNames.contains(this->activityTagName)){
				continue;
		}
	
		assert(this->nActivities < MAX_ACTIVITIES);	
		//this->activitiesIndices[this->nActivities++]=i;
		this->activitiesIndices.append(i);
		this->nActivities++;
	}
	
	assert(this->activitiesIndices.count()==this->nActivities);

	//////////////////////	
	for(int k=0; k<nPreferredStartingTimes_L; k++){
		if(this->days_L[k] >= r.nDaysPerWeek){
			QMessageBox::information(NULL, tr("FET information"),
			 tr("Constraint activities preferred starting times is wrong because it refers to removed day. Please correct"
			 " and try again. Correcting means editing the constraint and updating information. Constraint is:\n%1").arg(this->getDetailedDescription(r)));
		 
			return false;
		}
		if(this->hours_L[k] == r.nHoursPerDay){
			QMessageBox::information(NULL, tr("FET information"),
			 tr("Constraint activities preferred starting times is wrong because a preferred hour is too late (after the last acceptable slot). Please correct"
			 " and try again. Correcting means editing the constraint and updating information. Constraint is:\n%1").arg(this->getDetailedDescription(r)));
		 
			return false;
		}
		if(this->hours_L[k] > r.nHoursPerDay){
			QMessageBox::information(NULL, tr("FET information"),
			 tr("Constraint activities preferred starting times is wrong because it refers to removed hour. Please correct"
			 " and try again. Correcting means editing the constraint and updating information. Constraint is:\n%1").arg(this->getDetailedDescription(r)));
		 
			return false;
		}
	}
	///////////////////////
	
	if(this->nActivities>0)
		return true;
	else{
		QMessageBox::warning(NULL, tr("FET error in data"), 
			tr("Following constraint is wrong (refers to no activities). Please correct it:\n%1").arg(this->getDetailedDescription(r)));
		return false;
	}
}

bool ConstraintActivitiesPreferredStartingTimes::hasInactiveActivities(Rules& r)
{
	QList<int> localActiveActs;
	QList<int> localAllActs;

	//returns true if all activities are inactive
	QStringList::iterator it;
	Activity* act;
	int i;
	for(i=0; i<r.activitiesList.count(); i++){
		act=r.activitiesList.at(i);

		//check if this activity has the corresponding teacher
		if(this->teacherName!=""){
			it = act->teachersNames.find(this->teacherName);
			if(it==act->teachersNames.end())
				continue;
		}
		//check if this activity has the corresponding students
		if(this->studentsName!=""){
			bool commonStudents=false;
			foreach(QString st, act->studentsNames)
				if(r.setsShareStudents(st, studentsName)){
					commonStudents=true;
					break;
				}
		
			/*it = act->studentsNames.find(this->studentsName);
			if(it==act->studentsNames.end())
				continue;*/
			if(!commonStudents)
				continue;
		}
		//check if this activity has the corresponding subject
		if(this->subjectName!="" && act->subjectName!=this->subjectName){
				continue;
		}
		//check if this activity has the corresponding activity tag
		//if(this->p_activityTagName!="" && act->activityTagName!=this->p_activityTagName){
		if(this->activityTagName!="" && !act->activityTagsNames.contains(this->activityTagName)){
				continue;
		}
	
		//assert(this->p_nActivities < MAX_ACTIVITIES);	
		//this->p_activitiesIndices[this->p_nActivities++]=i;
		if(!r.inactiveActivities.contains(act->id))
			localActiveActs.append(act->id);
			
		localAllActs.append(act->id);
	}

	if(localActiveActs.count()==0 && localAllActs.count()>0)
		return true;
	else
		return false;
}

QString ConstraintActivitiesPreferredStartingTimes::getXmlDescription(Rules& r)
{
	//to avoid non-used parameter warning
	//Q_UNUSED(r);
	//if(&r==NULL)
	//	;

	QString s="<ConstraintActivitiesPreferredStartingTimes>\n";
	s+="	<Weight_Percentage>"+QString::number(this->weightPercentage)+"</Weight_Percentage>\n";
	//s+="	<Compulsory>";s+=yesNo(this->compulsory);s+="</Compulsory>\n";
	//if(this->teacherName!="")
		s+="	<Teacher_Name>"+protect(this->teacherName)+"</Teacher_Name>\n";
	//if(this->studentsName!="")
		s+="	<Students_Name>"+protect(this->studentsName)+"</Students_Name>\n";
	//if(this->subjectName!="")
		s+="	<Subject_Name>"+protect(this->subjectName)+"</Subject_Name>\n";
	//if(this->subjectTagName!="")
		s+="	<Activity_Tag_Name>"+protect(this->activityTagName)+"</Activity_Tag_Name>\n";
	s+="	<Number_of_Preferred_Starting_Times>"+QString::number(this->nPreferredStartingTimes_L)+"</Number_of_Preferred_Starting_Times>\n";
	for(int i=0; i<nPreferredStartingTimes_L; i++){
		s+="	<Preferred_Starting_Time>\n";
		if(this->days_L[i]>=0)
			s+="		<Preferred_Starting_Day>"+protect(r.daysOfTheWeek[this->days_L[i]])+"</Preferred_Starting_Day>\n";
		if(this->hours_L[i]>=0)
			s+="		<Preferred_Starting_Hour>"+protect(r.hoursOfTheDay[this->hours_L[i]])+"</Preferred_Starting_Hour>\n";
		s+="	</Preferred_Starting_Time>\n";
	}
	s+="</ConstraintActivitiesPreferredStartingTimes>\n";
	return s;
}

QString ConstraintActivitiesPreferredStartingTimes::getDescription(Rules& r)
{
	//to avoid non-used parameter warning
	
	QString s;

	QString tc, st, su, at;
	
	if(this->teacherName!="")
		tc=tr("teacher=%1").arg(this->teacherName);
	else
		tc=tr("all teachers");
		
	if(this->studentsName!="")
		st=tr("students=%1").arg(this->studentsName);
	else
		st=tr("all students");
		
	if(this->subjectName!="")
		su=tr("subject=%1").arg(this->subjectName);
	else
		su=tr("all subjects");
		
	if(this->activityTagName!="")
		at+=tr("activity tag=%1").arg(this->activityTagName);
	else
		at+=tr("all activity tags");
	
	s+=tr("Activities with %1, %2, %3, %4, have a set of preferred starting times:", "%1...%4 are conditions for the activities").arg(tc).arg(st).arg(su).arg(at);
	s+=" ";

	for(int i=0; i<this->nPreferredStartingTimes_L; i++){
		//s+=QString::number(i+1);
		//s+=":";
		if(this->days_L[i]>=0){
			s+=r.daysOfTheWeek[this->days_L[i]];
			s+=" ";
		}
		if(this->hours_L[i]>=0){
			s+=r.hoursOfTheDay[this->hours_L[i]];
		}
		if(i<this->nPreferredStartingTimes_L-1)
			s+="; ";
	}
	s+=", ";
	
	s+=tr("WP:%1\%", "Weight percentage").arg(this->weightPercentage);//s+=", ";

	return s;
}

QString ConstraintActivitiesPreferredStartingTimes::getDetailedDescription(Rules& r)
{
	QString s=tr("Time constraint");s+="\n";
	s+=tr("Activities with:");s+="\n";

	if(this->teacherName!="")
		s+=tr("Teacher=%1").arg(this->teacherName);
	else
		s+=tr("All teachers");
	s+="\n";
	
	if(this->studentsName!="")
		s+=tr("Students=%1").arg(this->studentsName);
	else
		s+=tr("All students");
	s+="\n";
		
	if(this->subjectName!="")
		s+=tr("Subject=%1").arg(this->subjectName);
	else
		s+=tr("All subjects");
	s+="\n";
	
	if(this->activityTagName!="")
		s+=tr("Activity tag=%1").arg(this->activityTagName);
	else
		s+=tr("All activity tags");
	s+="\n";

	s+=tr("have a set of preferred starting times:");
	s+="\n";
	for(int i=0; i<this->nPreferredStartingTimes_L; i++){
		//s+=QString::number(i+1);
		//s+=". ";
		if(this->days_L[i]>=0){
			s+=r.daysOfTheWeek[this->days_L[i]];
			s+=" ";
		}
		if(this->hours_L[i]>=0){
			s+=r.hoursOfTheDay[this->hours_L[i]];
		}
		if(i<this->nPreferredStartingTimes_L-1)
			s+=";  ";
	}
	s+="\n";

	s+=tr("Weight (percentage)=%1").arg(this->weightPercentage);s+="\n";

	return s;
}

double ConstraintActivitiesPreferredStartingTimes::fitness(Solution& c, Rules& r, QList<double>& cl, QList<QString>&dl, QString* conflictsString)
{
	//if the matrices subgroupsMatrix and teachersMatrix are already calculated, do not calculate them again!
	if(!c.teachersMatrixReady || !c.subgroupsMatrixReady){
		c.teachersMatrixReady=true;
		c.subgroupsMatrixReady=true;
	//if(crt_chrom!=&c || crt_rules!=&r || subgroups_conflicts<0 || teachers_conflicts<0 || c.changedForMatrixCalculation){
		subgroups_conflicts = c.getSubgroupsMatrix(r, subgroupsMatrix);
		teachers_conflicts = c.getTeachersMatrix(r, teachersMatrix);

		//crt_chrom=&c;
		//crt_rules=&r;
		
		c.changedForMatrixCalculation=false;
	}

	int nbroken;

	assert(r.internalStructureComputed);

	nbroken=0;
	int tmp;
	
	for(int i=0; i<this->nActivities; i++){
		tmp=0;
		int ai=this->activitiesIndices[i];
		if(c.times[ai]!=UNALLOCATED_TIME){
			int d=c.times[ai]%r.nDaysPerWeek; //the day when this activity was scheduled
			int h=c.times[ai]/r.nDaysPerWeek; //the hour
			int i;
			for(i=0; i<this->nPreferredStartingTimes_L; i++){
				if(this->days_L[i]>=0 && this->days_L[i]!=d)
					continue;
				if(this->hours_L[i]>=0 && this->hours_L[i]!=h)
					continue;
				break;
			}
			if(i==this->nPreferredStartingTimes_L){
				tmp=1;
				//if(r.internalActivitiesList[ai].parity==PARITY_WEEKLY) //for weekly activities, double the conflicts
				//	tmp=2;
			}
		}
		nbroken+=tmp;
		if(conflictsString!=NULL && tmp>0){
			QString s=tr("Time constraint activities preferred starting times broken"
			 " for activity with id=%1 (%2),"
			 " increases conflicts total by %3", "%1 is the id, %2 is the detailed description of the activity")
			 .arg(r.internalActivitiesList[ai].id)
			 .arg(getActivityDetailedDescription(r, r.internalActivitiesList[ai].id))
			 .arg(weightPercentage/100*tmp);
			 
			dl.append(s);
			cl.append(weightPercentage/100*tmp);
		
			*conflictsString+= s+"\n";
		}
	}

	if(weightPercentage==100)
		assert(nbroken==0);
	return nbroken * weightPercentage;
}

bool ConstraintActivitiesPreferredStartingTimes::isRelatedToActivity(Rules& r, Activity* a)
{
	QStringList::iterator it;

	//check if this activity has the corresponding teacher
	if(this->teacherName!=""){
		it = a->teachersNames.find(this->teacherName);
		if(it==a->teachersNames.end())
			return false;
	}
	//check if this activity has the corresponding students
	if(this->studentsName!=""){
		bool commonStudents=false;
		foreach(QString st, a->studentsNames){
			if(r.setsShareStudents(st, this->studentsName)){
				commonStudents=true;
				break;
			}
		}
		if(!commonStudents)
			return false;

		//it = a->studentsNames.find(this->studentsName);
		//if(it==a->studentsNames.end())
		//	return false;
	}
	//check if this activity has the corresponding subject
	if(this->subjectName!="" && a->subjectName!=this->subjectName)
		return false;
	//check if this activity has the corresponding activity tag
	if(this->activityTagName!="" && !a->activityTagsNames.contains(this->activityTagName))
		return false;

	return true;
}

bool ConstraintActivitiesPreferredStartingTimes::isRelatedToTeacher(Teacher* t)
{
	Q_UNUSED(t);
	//if(t)
	//	;

	return false;
}

bool ConstraintActivitiesPreferredStartingTimes::isRelatedToSubject(Subject* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintActivitiesPreferredStartingTimes::isRelatedToActivityTag(ActivityTag* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintActivitiesPreferredStartingTimes::isRelatedToStudentsSet(Rules& r, StudentsSet* s)
{
	Q_UNUSED(r);
	Q_UNUSED(s);
	/*if(s)
		;
	if(&r)
		;*/
		
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

ConstraintSubactivitiesPreferredStartingTimes::ConstraintSubactivitiesPreferredStartingTimes()
	: TimeConstraint()
{
	this->type = CONSTRAINT_SUBACTIVITIES_PREFERRED_STARTING_TIMES;
}

ConstraintSubactivitiesPreferredStartingTimes::ConstraintSubactivitiesPreferredStartingTimes(double wp, int compNo, QString te,
	QString st, QString su, QString sut, int nPT_L, QList<int> d_L, QList<int> h_L)
	: TimeConstraint(wp)
{
	assert(d_L.count()==nPT_L);
	assert(h_L.count()==nPT_L);

	this->componentNumber=compNo;
	this->teacherName=te;
	this->subjectName=su;
	this->activityTagName=sut;
	this->studentsName=st;
	this->nPreferredStartingTimes_L=nPT_L;
	/*assert(nPT<=MAX_N_CONSTRAINT_SUBACTIVITIES_PREFERRED_STARTING_TIMES);
	for(int i=0; i<nPT; i++){
		this->days[i]=d[i];
		this->hours[i]=h[i];
	}*/
	this->days_L=d_L;
	this->hours_L=h_L;
	this->type=CONSTRAINT_SUBACTIVITIES_PREFERRED_STARTING_TIMES;
}

bool ConstraintSubactivitiesPreferredStartingTimes::computeInternalStructure(Rules& r)
{
	//assert(this->teacherName!="" || this->studentsName!="" || this->subjectName!="" || this->subjectTagName!="");

	this->nActivities=0;
	this->activitiesIndices.clear();

	QStringList::iterator it;
	Activity* act;
	int i;
	for(i=0; i<r.nInternalActivities; i++){
		act=&r.internalActivitiesList[i];
		
		if(!act->representsComponentNumber(this->componentNumber))
			continue;

		//check if this activity has the corresponding teacher
		if(this->teacherName!=""){
			it = act->teachersNames.find(this->teacherName);
			if(it==act->teachersNames.end())
				continue;
		}
		//check if this activity has the corresponding students
		if(this->studentsName!=""){
			bool commonStudents=false;
			foreach(QString st, act->studentsNames)
				if(r.setsShareStudents(st, studentsName)){
					commonStudents=true;
					break;
				}
		
			/*it = act->studentsNames.find(this->studentsName);
			if(it==act->studentsNames.end())
				continue;*/
			if(!commonStudents)
				continue;
		}
		//check if this activity has the corresponding subject
		if(this->subjectName!="" && act->subjectName!=this->subjectName){
				continue;
		}
		//check if this activity has the corresponding activity tag
		if(this->activityTagName!="" && !act->activityTagsNames.contains(this->activityTagName)){
				continue;
		}
	
		assert(this->nActivities < MAX_ACTIVITIES);	
		//this->activitiesIndices[this->nActivities++]=i;
		this->nActivities++;
		this->activitiesIndices.append(i);
		
		//cout<<endl;
		//cout<<"activity with id == "<<act->id<<" corresponds to constraint subactivities preferred starting times"<<endl;
		//cout<<qPrintable(this->getDescription(r))<<endl;
	}
	
	assert(this->activitiesIndices.count()==this->nActivities);

	//////////////////////	
	for(int k=0; k<nPreferredStartingTimes_L; k++){
		if(this->days_L[k] >= r.nDaysPerWeek){
			QMessageBox::information(NULL, tr("FET information"),
			 tr("Constraint subactivities preferred starting times is wrong because it refers to removed day. Please correct"
			 " and try again. Correcting means editing the constraint and updating information. Constraint is:\n%1").arg(this->getDetailedDescription(r)));
		 
			return false;
		}
		if(this->hours_L[k] == r.nHoursPerDay){
			QMessageBox::information(NULL, tr("FET information"),
			 tr("Constraint subactivities preferred starting times is wrong because a preferred hour is too late (after the last acceptable slot). Please correct"
			 " and try again. Correcting means editing the constraint and updating information. Constraint is:\n%1").arg(this->getDetailedDescription(r)));
		 
			return false;
		}
		if(this->hours_L[k] > r.nHoursPerDay){
			QMessageBox::information(NULL, tr("FET information"),
			 tr("Constraint subactivities preferred starting times is wrong because it refers to removed hour. Please correct"
			 " and try again. Correcting means editing the constraint and updating information. Constraint is:\n%1").arg(this->getDetailedDescription(r)));
		 
			return false;
		}
	}
	///////////////////////
	
	if(this->nActivities>0)
		return true;
	else{
		QMessageBox::warning(NULL, tr("FET error in data"), 
			tr("Following constraint is wrong (refers to no activities). Please correct it:\n%1").arg(this->getDetailedDescription(r)));
		return false;
	}
}

bool ConstraintSubactivitiesPreferredStartingTimes::hasInactiveActivities(Rules& r)
{
	QList<int> localActiveActs;
	QList<int> localAllActs;

	//returns true if all activities are inactive
	QStringList::iterator it;
	Activity* act;
	int i;
	for(i=0; i<r.activitiesList.count(); i++){
		act=r.activitiesList.at(i);

		if(!act->representsComponentNumber(this->componentNumber))
			continue;

		//check if this activity has the corresponding teacher
		if(this->teacherName!=""){
			it = act->teachersNames.find(this->teacherName);
			if(it==act->teachersNames.end())
				continue;
		}
		//check if this activity has the corresponding students
		if(this->studentsName!=""){
			bool commonStudents=false;
			foreach(QString st, act->studentsNames)
				if(r.setsShareStudents(st, studentsName)){
					commonStudents=true;
					break;
				}
		
			/*it = act->studentsNames.find(this->studentsName);
			if(it==act->studentsNames.end())
				continue;*/
			if(!commonStudents)
				continue;
		}
		//check if this activity has the corresponding subject
		if(this->subjectName!="" && act->subjectName!=this->subjectName){
				continue;
		}
		//check if this activity has the corresponding activity tag
		//if(this->p_activityTagName!="" && act->activityTagName!=this->p_activityTagName){
		if(this->activityTagName!="" && !act->activityTagsNames.contains(this->activityTagName)){
				continue;
		}
	
		//assert(this->p_nActivities < MAX_ACTIVITIES);	
		//this->p_activitiesIndices[this->p_nActivities++]=i;
		if(!r.inactiveActivities.contains(act->id))
			localActiveActs.append(act->id);
			
		localAllActs.append(act->id);
	}

	if(localActiveActs.count()==0 && localAllActs.count()>0)
		return true;
	else
		return false;
}

QString ConstraintSubactivitiesPreferredStartingTimes::getXmlDescription(Rules& r)
{
	//to avoid non-used parameter warning
	//Q_UNUSED(r);
	//if(&r==NULL)
	//	;
	QString s="<ConstraintSubactivitiesPreferredStartingTimes>\n";
	s+="	<Weight_Percentage>"+QString::number(this->weightPercentage)+"</Weight_Percentage>\n";
	//s+="	<Compulsory>";s+=yesNo(this->compulsory);s+="</Compulsory>\n";
	//if(this->teacherName!="")
	s+="	<Component_Number>"+QString::number(this->componentNumber)+"</Component_Number>\n";
		s+="	<Teacher_Name>"+protect(this->teacherName)+"</Teacher_Name>\n";
	//if(this->studentsName!="")
		s+="	<Students_Name>"+protect(this->studentsName)+"</Students_Name>\n";
	//if(this->subjectName!="")
		s+="	<Subject_Name>"+protect(this->subjectName)+"</Subject_Name>\n";
	//if(this->subjectTagName!="")
		s+="	<Activity_Tag_Name>"+protect(this->activityTagName)+"</Activity_Tag_Name>\n";
	s+="	<Number_of_Preferred_Starting_Times>"+QString::number(this->nPreferredStartingTimes_L)+"</Number_of_Preferred_Starting_Times>\n";
	for(int i=0; i<nPreferredStartingTimes_L; i++){
		s+="	<Preferred_Starting_Time>\n";
		if(this->days_L[i]>=0)
			s+="		<Preferred_Starting_Day>"+protect(r.daysOfTheWeek[this->days_L[i]])+"</Preferred_Starting_Day>\n";
		if(this->hours_L[i]>=0)
			s+="		<Preferred_Starting_Hour>"+protect(r.hoursOfTheDay[this->hours_L[i]])+"</Preferred_Starting_Hour>\n";
		s+="	</Preferred_Starting_Time>\n";
	}
	s+="</ConstraintSubactivitiesPreferredStartingTimes>\n";
	return s;
}

QString ConstraintSubactivitiesPreferredStartingTimes::getDescription(Rules& r)
{
	QString tc, st, su, at;
	
	if(this->teacherName!="")
		tc=tr("teacher=%1").arg(this->teacherName);
	else
		tc=tr("all teachers");
		
	if(this->studentsName!="")
		st=tr("students=%1").arg(this->studentsName);
	else
		st=tr("all students");
		
	if(this->subjectName!="")
		su=tr("subject=%1").arg(this->subjectName);
	else
		su=tr("all subjects");
		
	if(this->activityTagName!="")
		at+=tr("activity tag=%1").arg(this->activityTagName);
	else
		at+=tr("all activity tags");
		
	QString s;
	
	s+=tr("Subactivities with %1, %2, %3, %4, %5, have a set of preferred starting times:", "%1...%5 are conditions for the subactivities")
		.arg(tr("component number=%1").arg(this->componentNumber)).arg(tc).arg(st).arg(su).arg(at);
	s+=" ";

	for(int i=0; i<this->nPreferredStartingTimes_L; i++){
		//s+=QString::number(i+1);
		//s+=":";
		if(this->days_L[i]>=0){
			s+=r.daysOfTheWeek[this->days_L[i]];
			s+=" ";
		}
		if(this->hours_L[i]>=0){
			s+=r.hoursOfTheDay[this->hours_L[i]];
		}
		if(i<this->nPreferredStartingTimes_L-1)
			s+="; ";
	}
	s+=", ";

	s+=tr("WP:%1\%", "Weight percentage").arg(this->weightPercentage);//s+=", ";

	return s;
}

QString ConstraintSubactivitiesPreferredStartingTimes::getDetailedDescription(Rules& r)
{
	QString s=tr("Time constraint");s+="\n";
	s+=tr("Subactivities with:");s+="\n";

	s+=tr("Component number=%1").arg(this->componentNumber);s+="\n";

	if(this->teacherName!="")
		s+=tr("Teacher=%1").arg(this->teacherName);
	else
		s+=tr("All teachers");
	s+="\n";
		
	if(this->studentsName!="")
		s+=tr("Students=%1").arg(this->studentsName);
	else
		s+=tr("All students");
	s+="\n";
		
	if(this->subjectName!="")
		s+=tr("Subject=%1").arg(this->subjectName);
	else
		s+=tr("All subjects");
	s+="\n";
	
	if(this->activityTagName!="")
		s+=tr("Activity tag=%1").arg(this->activityTagName);
	else
		s+=tr("All activity tags");
	s+="\n";

	s+=tr("have a set of preferred starting times:");
	s+="\n";
	for(int i=0; i<this->nPreferredStartingTimes_L; i++){
		//s+=QString::number(i+1);
		//s+=". ";
		if(this->days_L[i]>=0){
			s+=r.daysOfTheWeek[this->days_L[i]];
			s+=" ";
		}
		if(this->hours_L[i]>=0){
			s+=r.hoursOfTheDay[this->hours_L[i]];
		}
		if(i<this->nPreferredStartingTimes_L-1)
			s+=";  ";
	}
	s+="\n";

	s+=tr("Weight (percentage)=%1").arg(this->weightPercentage);s+="\n";

	return s;
}

double ConstraintSubactivitiesPreferredStartingTimes::fitness(Solution& c, Rules& r, QList<double>& cl, QList<QString>&dl, QString* conflictsString)
{
	//if the matrices subgroupsMatrix and teachersMatrix are already calculated, do not calculate them again!
	if(!c.teachersMatrixReady || !c.subgroupsMatrixReady){
		c.teachersMatrixReady=true;
		c.subgroupsMatrixReady=true;
	//if(crt_chrom!=&c || crt_rules!=&r || subgroups_conflicts<0 || teachers_conflicts<0 || c.changedForMatrixCalculation){
		subgroups_conflicts = c.getSubgroupsMatrix(r, subgroupsMatrix);
		teachers_conflicts = c.getTeachersMatrix(r, teachersMatrix);

		//crt_chrom=&c;
		//crt_rules=&r;
		
		c.changedForMatrixCalculation=false;
	}

	int nbroken;

	assert(r.internalStructureComputed);

	nbroken=0;
	int tmp;
	
	for(int i=0; i<this->nActivities; i++){
		tmp=0;
		int ai=this->activitiesIndices[i];
		if(c.times[ai]!=UNALLOCATED_TIME){
			int d=c.times[ai]%r.nDaysPerWeek; //the day when this activity was scheduled
			int h=c.times[ai]/r.nDaysPerWeek; //the hour
			int i;
			for(i=0; i<this->nPreferredStartingTimes_L; i++){
				if(this->days_L[i]>=0 && this->days_L[i]!=d)
					continue;
				if(this->hours_L[i]>=0 && this->hours_L[i]!=h)
					continue;
				break;
			}
			if(i==this->nPreferredStartingTimes_L){
				tmp=1;
				//if(r.internalActivitiesList[ai].parity==PARITY_WEEKLY) //for weekly activities, double the conflicts
				//	tmp=2;
			}
		}
		nbroken+=tmp;
		if(conflictsString!=NULL && tmp>0){
			QString s=tr("Time constraint subactivities preferred starting times broken"
			 " for activity with id=%1 (%2), component number %3,"
			 " increases conflicts total by %4", "%1 is the id, %2 is the detailed description of the activity")
			 .arg(r.internalActivitiesList[ai].id)
			 .arg(getActivityDetailedDescription(r, r.internalActivitiesList[ai].id))
			 .arg(this->componentNumber)
			 .arg(weightPercentage/100*tmp);

			dl.append(s);
			cl.append(weightPercentage/100*tmp);
		
			*conflictsString+= s+"\n";
		}
	}

	if(weightPercentage==100)
		assert(nbroken==0);
	return nbroken * weightPercentage;
}

bool ConstraintSubactivitiesPreferredStartingTimes::isRelatedToActivity(Rules& r, Activity* a)
{
	if(!a->representsComponentNumber(this->componentNumber))
		return false;

	QStringList::iterator it;
	
	//check if this activity has the corresponding teacher
	if(this->teacherName!=""){
		it = a->teachersNames.find(this->teacherName);
		if(it==a->teachersNames.end())
			return false;
	}
	//check if this activity has the corresponding students
	if(this->studentsName!=""){
		bool commonStudents=false;
		foreach(QString st, a->studentsNames){
			if(r.setsShareStudents(st, this->studentsName)){
				commonStudents=true;
				break;
			}
		}
		if(!commonStudents)
			return false;

		//it = a->studentsNames.find(this->studentsName);
		//if(it==a->studentsNames.end())
		//	return false;
	}
	//check if this activity has the corresponding subject
	if(this->subjectName!="" && a->subjectName!=this->subjectName)
		return false;
	//check if this activity has the corresponding activity tag
	if(this->activityTagName!="" && !a->activityTagsNames.contains(this->activityTagName))
		return false;

	return true;
}

bool ConstraintSubactivitiesPreferredStartingTimes::isRelatedToTeacher(Teacher* t)
{
	Q_UNUSED(t);
	//if(t)
	//	;

	return false;
}

bool ConstraintSubactivitiesPreferredStartingTimes::isRelatedToSubject(Subject* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintSubactivitiesPreferredStartingTimes::isRelatedToActivityTag(ActivityTag* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintSubactivitiesPreferredStartingTimes::isRelatedToStudentsSet(Rules& r, StudentsSet* s)
{
	Q_UNUSED(r);
	Q_UNUSED(s);
	/*if(s)
		;
	if(&r)
		;*/
		
	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////

ConstraintActivitiesSameStartingHour::ConstraintActivitiesSameStartingHour()
	: TimeConstraint()
{
	type=CONSTRAINT_ACTIVITIES_SAME_STARTING_HOUR;
}

ConstraintActivitiesSameStartingHour::ConstraintActivitiesSameStartingHour(double wp, int nact, const QList<int>& act)
 : TimeConstraint(wp)
 {
	assert(nact>=2);
	assert(act.count()==nact);
	this->n_activities=nact;
	this->activitiesId.clear();
	for(int i=0; i<nact; i++)
		this->activitiesId.append(act.at(i));

	this->type=CONSTRAINT_ACTIVITIES_SAME_STARTING_HOUR;
}

bool ConstraintActivitiesSameStartingHour::computeInternalStructure(Rules &r)
{
	//compute the indices of the activities,
	//based on their unique ID

	assert(this->n_activities==this->activitiesId.count());

	this->_activities.clear();
	for(int i=0; i<this->n_activities; i++){
		int j;
		Activity* act;
		for(j=0; j<r.nInternalActivities; j++){
			act=&r.internalActivitiesList[j];
			if(act->id==this->activitiesId[i]){
				this->_activities.append(j);
				break;
			}
		}
	}
	this->_n_activities=this->_activities.count();
	
	if(this->_n_activities<=1){
		QMessageBox::warning(NULL, tr("FET error in data"), 
			tr("Following constraint is wrong (because you need 2 or more activities. Please correct it):\n%1").arg(this->getDetailedDescription(r)));
		//assert(0);
		return false;
	}

	return true;
}

void ConstraintActivitiesSameStartingHour::removeUseless(Rules& r)
{
	//remove the activitiesId which no longer exist (used after the deletion of an activity)
	
	assert(this->n_activities==this->activitiesId.count());

	QList<int> tmpList;

	for(int i=0; i<this->n_activities; i++){
		for(int k=0; k<r.activitiesList.size(); k++){
			Activity* act=r.activitiesList[k];
			if(act->id==this->activitiesId[i]){
				tmpList.append(act->id);
				break;
			}
		}
	}
	
	this->activitiesId=tmpList;
	this->n_activities=this->activitiesId.count();

	r.internalStructureComputed=false;
}

bool ConstraintActivitiesSameStartingHour::hasInactiveActivities(Rules& r)
{
	int count=0;

	for(int i=0; i<this->n_activities; i++)
		if(r.inactiveActivities.contains(this->activitiesId[i]))
			count++;

	if(this->n_activities-count<=1)
		return true;
	else
		return false;
}

QString ConstraintActivitiesSameStartingHour::getXmlDescription(Rules& r){
	Q_UNUSED(r);

	QString s="<ConstraintActivitiesSameStartingHour>\n";
	s+="	<Weight_Percentage>"+QString::number(this->weightPercentage)+"</Weight_Percentage>\n";
	//s+="	<Compulsory>";s+=yesNo(this->compulsory);s+="</Compulsory>\n";
	s+="	<Number_of_Activities>"+QString::number(this->n_activities)+"</Number_of_Activities>\n";
	for(int i=0; i<this->n_activities; i++)
		s+="	<Activity_Id>"+QString::number(this->activitiesId[i])+"</Activity_Id>\n";
	s+="</ConstraintActivitiesSameStartingHour>\n";
	return s;
}

QString ConstraintActivitiesSameStartingHour::getDescription(Rules& r){
	Q_UNUSED(r);

	QString s;
	s+=tr("Activities same starting hour");s+=", ";
	s+=tr("WP:%1\%", "Weight percentage").arg(this->weightPercentage);s+=", ";
	s+=tr("NA:%1", "Number of activities").arg(this->n_activities);s+=", ";
	for(int i=0; i<this->n_activities; i++){
		s+=tr("Id:%1", "Id of activity").arg(this->activitiesId[i]);
		if(i<this->n_activities-1)
			s+=", ";
	}

	return s;
}

QString ConstraintActivitiesSameStartingHour::getDetailedDescription(Rules& r){
	QString s;
	
	s=tr("Time constraint");s+="\n";
	s+=tr("Activities must have the same starting hour");s+="\n";
	s+=tr("Weight (percentage)=%1\%").arg(this->weightPercentage);s+="\n";
	s+=tr("Number of activities=%1").arg(this->n_activities);s+="\n";
	for(int i=0; i<this->n_activities; i++){
		s+=tr("Activity with id=%1 (%2)", "%1 is the id, %2 is the detailed description of the activity.")
			.arg(this->activitiesId[i])
			.arg(getActivityDetailedDescription(r, this->activitiesId[i]));
		s+="\n";
	}

	return s;
}

double ConstraintActivitiesSameStartingHour::fitness(Solution& c, Rules& r, QList<double>& cl, QList<QString>&dl, QString* conflictsString)
{
	assert(r.internalStructureComputed);

	int nbroken;

	//We do not use the matrices 'subgroupsMatrix' nor 'teachersMatrix'.

	//sum the differences in the scheduled hour for all pairs of activities.

	//without logging
	if(conflictsString==NULL){
		nbroken=0;
		for(int i=1; i<this->_n_activities; i++){
			int t1=c.times[this->_activities[i]];
			if(t1!=UNALLOCATED_TIME){
				//int day1=t1%r.nDaysPerWeek;
				int hour1=t1/r.nDaysPerWeek;
				for(int j=0; j<i; j++){
					int t2=c.times[this->_activities[j]];
					if(t2!=UNALLOCATED_TIME){
						//int day2=t2%r.nDaysPerWeek;
						int hour2=t2/r.nDaysPerWeek;
						int tmp=0;

						//activity weekly - counts as double
						/*if(r.internalActivitiesList[this->_activities[i]].parity==PARITY_WEEKLY &&
						 r.internalActivitiesList[this->_activities[j]].parity==PARITY_WEEKLY)
							tmp = 4 * abs(hour1-hour2);
						else if(r.internalActivitiesList[this->_activities[i]].parity==PARITY_WEEKLY ||
						 r.internalActivitiesList[this->_activities[j]].parity==PARITY_WEEKLY)
							tmp = 2 * abs(hour1-hour2);
						else*/
						
						//	tmp = abs(hour1-hour2);
						if(hour1!=hour2)
							tmp=1;

						nbroken+=tmp;
					}
				}
			}
		}
	}
	//with logging
	else{
		nbroken=0;
		for(int i=1; i<this->_n_activities; i++){
			int t1=c.times[this->_activities[i]];
			if(t1!=UNALLOCATED_TIME){
				//int day1=t1%r.nDaysPerWeek;
				int hour1=t1/r.nDaysPerWeek;
				for(int j=0; j<i; j++){
					int t2=c.times[this->_activities[j]];
					if(t2!=UNALLOCATED_TIME){
						//int day2=t2%r.nDaysPerWeek;
						int hour2=t2/r.nDaysPerWeek;
						int tmp=0;

						//activity weekly - counts as double
						/*if(r.internalActivitiesList[this->_activities[i]].parity==PARITY_WEEKLY &&
						 r.internalActivitiesList[this->_activities[j]].parity==PARITY_WEEKLY)
							tmp = 4 * abs(hour1-hour2);
						else if(r.internalActivitiesList[this->_activities[i]].parity==PARITY_WEEKLY ||
						 r.internalActivitiesList[this->_activities[j]].parity==PARITY_WEEKLY)
							tmp = 2 * abs(hour1-hour2);
						else*/
						
						//	tmp = abs(hour1-hour2);						
						if(hour1!=hour2)
							tmp=1;

						nbroken+=tmp;

						if(tmp>0 && conflictsString!=NULL){
							QString s=tr("Time constraint activities same starting hour broken, because activity with id=%1 (%2) is not at the same hour with activity with id=%3 (%4)"
							 , "%1 is the id, %2 is the detailed description of the activity, %3 id, %4 det. descr.")
							 .arg(this->activitiesId[i])
							 .arg(getActivityDetailedDescription(r, this->activitiesId[i]))
							 .arg(this->activitiesId[j])
							 .arg(getActivityDetailedDescription(r, this->activitiesId[j]));
							s+=". ";
							s+=tr("Conflicts factor increase=%1").arg(tmp*weightPercentage/100);
							
							dl.append(s);
							cl.append(tmp*weightPercentage/100);
						
							*conflictsString+= s+"\n";
						}
					}
				}
			}
		}
	}

	if(weightPercentage==100)
		assert(nbroken==0);
	return weightPercentage/100 * nbroken;
}

bool ConstraintActivitiesSameStartingHour::isRelatedToActivity(Rules& r, Activity* a)
{
	Q_UNUSED(r);

	for(int i=0; i<this->n_activities; i++)
		if(this->activitiesId[i]==a->id)
			return true;
	return false;
}

bool ConstraintActivitiesSameStartingHour::isRelatedToTeacher(Teacher* t)
{
	Q_UNUSED(t);
	//if(t)
	//	;

	return false;
}

bool ConstraintActivitiesSameStartingHour::isRelatedToSubject(Subject* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintActivitiesSameStartingHour::isRelatedToActivityTag(ActivityTag* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintActivitiesSameStartingHour::isRelatedToStudentsSet(Rules& r, StudentsSet* s)
{
	Q_UNUSED(r);
	Q_UNUSED(s);
	/*if(s)
		;
	if(&r)
		;*/
		
	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////

ConstraintActivitiesSameStartingDay::ConstraintActivitiesSameStartingDay()
	: TimeConstraint()
{
	type=CONSTRAINT_ACTIVITIES_SAME_STARTING_DAY;
}

ConstraintActivitiesSameStartingDay::ConstraintActivitiesSameStartingDay(double wp, int nact, const QList<int>& act)
 : TimeConstraint(wp)
 {
	assert(nact>=2);
	assert(act.count()==nact);
	this->n_activities=nact;
	this->activitiesId.clear();
	for(int i=0; i<nact; i++)
		this->activitiesId.append(act.at(i));

	this->type=CONSTRAINT_ACTIVITIES_SAME_STARTING_DAY;
}

bool ConstraintActivitiesSameStartingDay::computeInternalStructure(Rules &r)
{
	//compute the indices of the activities,
	//based on their unique ID

	assert(this->n_activities==this->activitiesId.count());

	this->_activities.clear();
	for(int i=0; i<this->n_activities; i++){
		int j;
		Activity* act;
		for(j=0; j<r.nInternalActivities; j++){
			act=&r.internalActivitiesList[j];
			if(act->id==this->activitiesId[i]){
				this->_activities.append(j);
				break;
			}
		}
	}
	this->_n_activities=this->_activities.count();
	
	if(this->_n_activities<=1){
		QMessageBox::warning(NULL, tr("FET error in data"), 
			tr("Following constraint is wrong (because you need 2 or more activities. Please correct it):\n%1").arg(this->getDetailedDescription(r)));
		//assert(0);
		return false;
	}

	return true;
}

void ConstraintActivitiesSameStartingDay::removeUseless(Rules& r)
{
	//remove the activitiesId which no longer exist (used after the deletion of an activity)
	
	assert(this->n_activities==this->activitiesId.count());

	QList<int> tmpList;

	for(int i=0; i<this->n_activities; i++){
		for(int k=0; k<r.activitiesList.size(); k++){
			Activity* act=r.activitiesList[k];
			if(act->id==this->activitiesId[i]){
				tmpList.append(act->id);
				break;
			}
		}
	}
	
	this->activitiesId=tmpList;
	this->n_activities=this->activitiesId.count();

	r.internalStructureComputed=false;
}

bool ConstraintActivitiesSameStartingDay::hasInactiveActivities(Rules& r)
{
	int count=0;

	for(int i=0; i<this->n_activities; i++)
		if(r.inactiveActivities.contains(this->activitiesId[i]))
			count++;

	if(this->n_activities-count<=1)
		return true;
	else
		return false;
}

QString ConstraintActivitiesSameStartingDay::getXmlDescription(Rules& r){
	Q_UNUSED(r);

	QString s="<ConstraintActivitiesSameStartingDay>\n";
	s+="	<Weight_Percentage>"+QString::number(this->weightPercentage)+"</Weight_Percentage>\n";
	//s+="	<Compulsory>";s+=yesNo(this->compulsory);s+="</Compulsory>\n";
	s+="	<Number_of_Activities>"+QString::number(this->n_activities)+"</Number_of_Activities>\n";
	for(int i=0; i<this->n_activities; i++)
		s+="	<Activity_Id>"+QString::number(this->activitiesId[i])+"</Activity_Id>\n";
	s+="</ConstraintActivitiesSameStartingDay>\n";
	return s;
}

QString ConstraintActivitiesSameStartingDay::getDescription(Rules& r){
	Q_UNUSED(r);

	QString s;
	s+=tr("Activities same starting day");s+=", ";
	s+=tr("WP:%1\%", "Weight percentage").arg(this->weightPercentage);s+=", ";
	s+=tr("NA:%1", "Number of activities").arg(this->n_activities);s+=", ";
	for(int i=0; i<this->n_activities; i++){
		s+=tr("Id:%1", "Id of activity").arg(this->activitiesId[i]);
		if(i<this->n_activities-1)
			s+=", ";
	}

	return s;
}

QString ConstraintActivitiesSameStartingDay::getDetailedDescription(Rules& r){
	QString s;
	
	s=tr("Time constraint");s+="\n";
	s+=tr("Activities must have the same starting day");s+="\n";
	s+=tr("Weight (percentage)=%1\%").arg(this->weightPercentage);s+="\n";
	s+=tr("Number of activities=%1").arg(this->n_activities);s+="\n";
	for(int i=0; i<this->n_activities; i++){
		s+=tr("Activity with id=%1 (%2)", "%1 is the id, %2 is the detailed description of the activity.")
			.arg(this->activitiesId[i])
			.arg(getActivityDetailedDescription(r, this->activitiesId[i]));
		s+="\n";
	}

	return s;
}

double ConstraintActivitiesSameStartingDay::fitness(Solution& c, Rules& r, QList<double>& cl, QList<QString>&dl, QString* conflictsString)
{
	assert(r.internalStructureComputed);

	int nbroken;

	//We do not use the matrices 'subgroupsMatrix' nor 'teachersMatrix'.

	//sum the differences in the scheduled hour for all pairs of activities.

	//without logging
	if(conflictsString==NULL){
		nbroken=0;
		for(int i=1; i<this->_n_activities; i++){
			int t1=c.times[this->_activities[i]];
			if(t1!=UNALLOCATED_TIME){
				int day1=t1%r.nDaysPerWeek;
				//int hour1=t1/r.nDaysPerWeek;
				for(int j=0; j<i; j++){
					int t2=c.times[this->_activities[j]];
					if(t2!=UNALLOCATED_TIME){
						int day2=t2%r.nDaysPerWeek;
						//int hour2=t2/r.nDaysPerWeek;
						int tmp=0;

						if(day1!=day2)
							tmp=1;

						nbroken+=tmp;
					}
				}
			}
		}
	}
	//with logging
	else{
		nbroken=0;
		for(int i=1; i<this->_n_activities; i++){
			int t1=c.times[this->_activities[i]];
			if(t1!=UNALLOCATED_TIME){
				int day1=t1%r.nDaysPerWeek;
				//int hour1=t1/r.nDaysPerWeek;
				for(int j=0; j<i; j++){
					int t2=c.times[this->_activities[j]];
					if(t2!=UNALLOCATED_TIME){
						int day2=t2%r.nDaysPerWeek;
						//int hour2=t2/r.nDaysPerWeek;
						int tmp=0;

						if(day1!=day2)
							tmp=1;

						nbroken+=tmp;

						if(tmp>0 && conflictsString!=NULL){
							QString s=tr("Time constraint activities same starting day broken, because activity with id=%1 (%2) is not in the same day with activity with id=%3 (%4)"
							 , "%1 is the id, %2 is the detailed description of the activity, %3 id, %4 det. descr.")
							 .arg(this->activitiesId[i])
							 .arg(getActivityDetailedDescription(r, this->activitiesId[i]))
							 .arg(this->activitiesId[j])
							 .arg(getActivityDetailedDescription(r, this->activitiesId[j]));
							s+=". ";
							s+=tr("Conflicts factor increase=%1").arg(tmp*weightPercentage/100);
							
							dl.append(s);
							cl.append(tmp*weightPercentage/100);
						
							*conflictsString+= s+"\n";
						}
					}
				}
			}
		}
	}

	if(weightPercentage==100)
		assert(nbroken==0);
	return weightPercentage/100 * nbroken;
}

bool ConstraintActivitiesSameStartingDay::isRelatedToActivity(Rules& r, Activity* a)
{
	Q_UNUSED(r);

	for(int i=0; i<this->n_activities; i++)
		if(this->activitiesId[i]==a->id)
			return true;
	return false;
}

bool ConstraintActivitiesSameStartingDay::isRelatedToTeacher(Teacher* t)
{
	Q_UNUSED(t);
	//if(t)
	//	;

	return false;
}

bool ConstraintActivitiesSameStartingDay::isRelatedToSubject(Subject* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintActivitiesSameStartingDay::isRelatedToActivityTag(ActivityTag* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintActivitiesSameStartingDay::isRelatedToStudentsSet(Rules& r, StudentsSet* s)
{
	Q_UNUSED(r);
	Q_UNUSED(s);
	/*if(s)
		;
	if(&r)
		;*/
		
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

ConstraintTwoActivitiesConsecutive::ConstraintTwoActivitiesConsecutive()
	: TimeConstraint()
{
	this->type = CONSTRAINT_TWO_ACTIVITIES_CONSECUTIVE;
}

ConstraintTwoActivitiesConsecutive::ConstraintTwoActivitiesConsecutive(double wp, int firstActId, int secondActId)
	: TimeConstraint(wp)
{
	this->firstActivityId = firstActId;
	this->secondActivityId=secondActId;
	this->type = CONSTRAINT_TWO_ACTIVITIES_CONSECUTIVE;
}

bool ConstraintTwoActivitiesConsecutive::computeInternalStructure(Rules& r)
{
	Activity* act;
	int i;
	for(i=0; i<r.nInternalActivities; i++){
		act=&r.internalActivitiesList[i];
		if(act->id==this->firstActivityId)
			break;
	}
	
	if(i==r.nInternalActivities){	
		//assert(0);
		QMessageBox::warning(NULL, tr("FET error in data"), 
			tr("Following constraint is wrong (refers to inexistent activity ids):\n%1").arg(this->getDetailedDescription(r)));
		return false;
	}

	this->firstActivityIndex=i;	

	////////
	
	for(i=0; i<r.nInternalActivities; i++){
		act=&r.internalActivitiesList[i];
		if(act->id==this->secondActivityId)
			break;
	}
	
	if(i==r.nInternalActivities){	
		//assert(0);
		QMessageBox::warning(NULL, tr("FET error in data"), 
			tr("Following constraint is wrong (refers to inexistent activity ids):\n%1").arg(this->getDetailedDescription(r)));
		return false;
	}

	this->secondActivityIndex=i;
	
	if(firstActivityIndex==secondActivityIndex){	
		//assert(0);
		QMessageBox::warning(NULL, tr("FET error in data"), 
			tr("Following constraint is wrong (refers to same activities):\n%1").arg(this->getDetailedDescription(r)));
		return false;
	}
	assert(firstActivityIndex!=secondActivityIndex);
	
	return true;
}

bool ConstraintTwoActivitiesConsecutive::hasInactiveActivities(Rules& r)
{
	if(r.inactiveActivities.contains(this->firstActivityId))
		return true;
	if(r.inactiveActivities.contains(this->secondActivityId))
		return true;
	return false;
}

QString ConstraintTwoActivitiesConsecutive::getXmlDescription(Rules& r)
{
	//to avoid non-used parameter warning
	Q_UNUSED(r);

	QString s="<ConstraintTwoActivitiesConsecutive>\n";
	s+="	<Weight_Percentage>"+QString::number(this->weightPercentage)+"</Weight_Percentage>\n";
	//s+="	<Compulsory>";s+=yesNo(this->compulsory);s+="</Compulsory>\n";
	s+="	<First_Activity_Id>"+QString::number(this->firstActivityId)+"</First_Activity_Id>\n";
	s+="	<Second_Activity_Id>"+QString::number(this->secondActivityId)+"</Second_Activity_Id>\n";
	s+="</ConstraintTwoActivitiesConsecutive>\n";
	return s;
}

QString ConstraintTwoActivitiesConsecutive::getDescription(Rules& r)
{
	//to avoid non-used parameter warning
	Q_UNUSED(r);

	QString s;
	
	s=tr("Constraint two activities consecutive:");
	s+=" ";
	
	s+=tr("first act. id: %1", "act.=activity").arg(this->firstActivityId);
	s+=", ";
	s+=tr("second act. id: %1", "act.=activity").arg(this->secondActivityId);
	s+=", ";
	s+=tr("WP:%1\%", "Weight percentage").arg(this->weightPercentage);

	return s;
}

QString ConstraintTwoActivitiesConsecutive::getDetailedDescription(Rules& r)
{
	QString s=tr("Time constraint");s+="\n";
	s+=tr("Constraint two activities consecutive (second activity must be placed immediately after the first"
	 " activity, in the same day, possibly separated by breaks)"); s+="\n";
	
	s+=tr("Weight (percentage)=%1\%").arg(this->weightPercentage);s+="\n";

	s+=tr("First activity id=%1 (%2)", "%1 is the id, %2 is the detailed description of the activity.")
		.arg(this->firstActivityId)
		.arg(getActivityDetailedDescription(r, this->firstActivityId));
	s+="\n";

	s+=tr("Second activity id=%1 (%2)", "%1 is the id, %2 is the detailed description of the activity.")
		.arg(this->secondActivityId)
		.arg(getActivityDetailedDescription(r, this->secondActivityId));
	s+="\n";
	
	return s;
}

double ConstraintTwoActivitiesConsecutive::fitness(Solution& c, Rules& r, QList<double>& cl, QList<QString>&dl, QString* conflictsString)
{
	//if the matrices subgroupsMatrix and teachersMatrix are already calculated, do not calculate them again!
	if(!c.teachersMatrixReady || !c.subgroupsMatrixReady){
		c.teachersMatrixReady=true;
		c.subgroupsMatrixReady=true;
	//if(crt_chrom!=&c || crt_rules!=&r || subgroups_conflicts<0 || teachers_conflicts<0 || c.changedForMatrixCalculation){
		subgroups_conflicts = c.getSubgroupsMatrix(r, subgroupsMatrix);
		teachers_conflicts = c.getTeachersMatrix(r, teachersMatrix);

		//crt_chrom=&c;
		//crt_rules=&r;
		
		c.changedForMatrixCalculation=false;
	}

	int nbroken;

	assert(r.internalStructureComputed);

	nbroken=0;
	if(c.times[this->firstActivityIndex]!=UNALLOCATED_TIME && c.times[this->secondActivityIndex]!=UNALLOCATED_TIME){
		int fd=c.times[this->firstActivityIndex]%r.nDaysPerWeek; //the day when first activity was scheduled
		int fh=c.times[this->firstActivityIndex]/r.nDaysPerWeek; //the hour
		int sd=c.times[this->secondActivityIndex]%r.nDaysPerWeek; //the day when second activity was scheduled
		int sh=c.times[this->secondActivityIndex]/r.nDaysPerWeek; //the hour
		
		//cout<<"fd=="<<fd<<", fh=="<<fh<<", sd=="<<sd<<", sh=="<<sh<<endl;
		
		if(fd!=sd)
			nbroken=1;
		else if(fh+r.internalActivitiesList[this->firstActivityIndex].duration>sh)
			nbroken=1;
		else if(fd==sd){
			int h;
			int d=fd;
			assert(d==sd);
			for(h=fh+r.internalActivitiesList[this->firstActivityIndex].duration; h<r.nHoursPerDay; h++)
				if(!breakDayHour[d][h])
					break;
					
			assert(h<=sh);	
				
			if(h!=sh)
				nbroken=1;
		}
	}
	
	assert(nbroken==0 || nbroken==1);

	if(conflictsString!=NULL && nbroken>0){
		QString s=tr("Time constraint two activities consecutive broken for first activity with id=%1 (%2) and "
		 "second activity with id=%3 (%4), increases conflicts total by %5", "%1 is the id, %2 is the detailed description of the activity, %3 id, %4 det. descr.")
		 .arg(this->firstActivityId)
		 .arg(getActivityDetailedDescription(r, this->firstActivityId))
		 .arg(this->secondActivityId)
		 .arg(getActivityDetailedDescription(r, this->secondActivityId))
		 .arg(weightPercentage/100*nbroken);

		dl.append(s);
		cl.append(weightPercentage/100*nbroken);
	
		*conflictsString+= s+"\n";
	}
	
	if(weightPercentage==100)
		assert(nbroken==0);
	return nbroken * weightPercentage/100;
}

bool ConstraintTwoActivitiesConsecutive::isRelatedToActivity(Rules& r, Activity* a)
{
	Q_UNUSED(r);

	if(this->firstActivityId==a->id)
		return true;
	if(this->secondActivityId==a->id)
		return true;
	return false;
}

bool ConstraintTwoActivitiesConsecutive::isRelatedToTeacher(Teacher* t)
{
	Q_UNUSED(t);
	//if(t)
	//	;

	return false;
}

bool ConstraintTwoActivitiesConsecutive::isRelatedToSubject(Subject* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintTwoActivitiesConsecutive::isRelatedToActivityTag(ActivityTag* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintTwoActivitiesConsecutive::isRelatedToStudentsSet(Rules& r, StudentsSet* s)
{
	Q_UNUSED(r);
	Q_UNUSED(s);
	/*if(s)
		;
	if(&r)
		;*/
		
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

ConstraintTwoActivitiesGrouped::ConstraintTwoActivitiesGrouped()
	: TimeConstraint()
{
	this->type = CONSTRAINT_TWO_ACTIVITIES_GROUPED;
}

ConstraintTwoActivitiesGrouped::ConstraintTwoActivitiesGrouped(double wp, int firstActId, int secondActId)
	: TimeConstraint(wp)
{
	this->firstActivityId = firstActId;
	this->secondActivityId=secondActId;
	this->type = CONSTRAINT_TWO_ACTIVITIES_GROUPED;
}

bool ConstraintTwoActivitiesGrouped::computeInternalStructure(Rules& r)
{
	Activity* act;
	int i;
	for(i=0; i<r.nInternalActivities; i++){
		act=&r.internalActivitiesList[i];
		if(act->id==this->firstActivityId)
			break;
	}
	
	if(i==r.nInternalActivities){	
		//assert(0);
		QMessageBox::warning(NULL, tr("FET error in data"), 
			tr("Following constraint is wrong (refers to inexistent activity ids):\n%1").arg(this->getDetailedDescription(r)));
		return false;
	}

	this->firstActivityIndex=i;	

	////////
	
	for(i=0; i<r.nInternalActivities; i++){
		act=&r.internalActivitiesList[i];
		if(act->id==this->secondActivityId)
			break;
	}
	
	if(i==r.nInternalActivities){	
		//assert(0);
		QMessageBox::warning(NULL, tr("FET error in data"), 
			tr("Following constraint is wrong (refers to inexistent activity ids):\n%1").arg(this->getDetailedDescription(r)));
		return false;
	}

	this->secondActivityIndex=i;
	
	if(firstActivityIndex==secondActivityIndex){	
		//assert(0);
		QMessageBox::warning(NULL, tr("FET error in data"), 
			tr("Following constraint is wrong (refers to same activities):\n%1").arg(this->getDetailedDescription(r)));
		return false;
	}
	assert(firstActivityIndex!=secondActivityIndex);
	
	return true;
}

bool ConstraintTwoActivitiesGrouped::hasInactiveActivities(Rules& r)
{
	if(r.inactiveActivities.contains(this->firstActivityId))
		return true;
	if(r.inactiveActivities.contains(this->secondActivityId))
		return true;
	return false;
}

QString ConstraintTwoActivitiesGrouped::getXmlDescription(Rules& r)
{
	//to avoid non-used parameter warning
	Q_UNUSED(r);

	QString s="<ConstraintTwoActivitiesGrouped>\n";
	s+="	<Weight_Percentage>"+QString::number(this->weightPercentage)+"</Weight_Percentage>\n";
	//s+="	<Compulsory>";s+=yesNo(this->compulsory);s+="</Compulsory>\n";
	s+="	<First_Activity_Id>"+QString::number(this->firstActivityId)+"</First_Activity_Id>\n";
	s+="	<Second_Activity_Id>"+QString::number(this->secondActivityId)+"</Second_Activity_Id>\n";
	s+="</ConstraintTwoActivitiesGrouped>\n";
	return s;
}

QString ConstraintTwoActivitiesGrouped::getDescription(Rules& r)
{
	//to avoid non-used parameter warning
	Q_UNUSED(r);

	QString s;
	
	s=tr("Constraint two activities grouped:");
	s+=" ";
	
	s+=tr("first act. id: %1", "act.=activity").arg(this->firstActivityId);
	s+=", ";
	s+=tr("second act. id: %1", "act.=activity").arg(this->secondActivityId);
	s+=", ";
	s+=tr("WP:%1\%", "Weight percentage").arg(this->weightPercentage);

	return s;
}

QString ConstraintTwoActivitiesGrouped::getDetailedDescription(Rules& r)
{
	QString s=tr("Time constraint");s+="\n";
	s+=tr("Constraint two activities grouped (the activities must be placed in the same day, "
	 "one immediately following the other, in any order, possibly separated by breaks)"); s+="\n";
	
	s+=tr("Weight (percentage)=%1\%").arg(this->weightPercentage);s+="\n";

	s+=tr("First activity id=%1 (%2)", "%1 is the id, %2 is the detailed description of the activity.")
		.arg(this->firstActivityId)
		.arg(getActivityDetailedDescription(r, this->firstActivityId));
	s+="\n";

	s+=tr("Second activity id=%1 (%2)", "%1 is the id, %2 is the detailed description of the activity.")
		.arg(this->secondActivityId)
		.arg(getActivityDetailedDescription(r, this->secondActivityId));
	s+="\n";
	
	return s;
}

double ConstraintTwoActivitiesGrouped::fitness(Solution& c, Rules& r, QList<double>& cl, QList<QString>&dl, QString* conflictsString)
{
	//if the matrices subgroupsMatrix and teachersMatrix are already calculated, do not calculate them again!
	if(!c.teachersMatrixReady || !c.subgroupsMatrixReady){
		c.teachersMatrixReady=true;
		c.subgroupsMatrixReady=true;
	//if(crt_chrom!=&c || crt_rules!=&r || subgroups_conflicts<0 || teachers_conflicts<0 || c.changedForMatrixCalculation){
		subgroups_conflicts = c.getSubgroupsMatrix(r, subgroupsMatrix);
		teachers_conflicts = c.getTeachersMatrix(r, teachersMatrix);

		//crt_chrom=&c;
		//crt_rules=&r;
		
		c.changedForMatrixCalculation=false;
	}

	int nbroken;

	assert(r.internalStructureComputed);

	nbroken=0;
	if(c.times[this->firstActivityIndex]!=UNALLOCATED_TIME && c.times[this->secondActivityIndex]!=UNALLOCATED_TIME){
		int fd=c.times[this->firstActivityIndex]%r.nDaysPerWeek; //the day when first activity was scheduled
		int fh=c.times[this->firstActivityIndex]/r.nDaysPerWeek; //the hour
		int sd=c.times[this->secondActivityIndex]%r.nDaysPerWeek; //the day when second activity was scheduled
		int sh=c.times[this->secondActivityIndex]/r.nDaysPerWeek; //the hour
		
		//cout<<"fd=="<<fd<<", fh=="<<fh<<", sd=="<<sd<<", sh=="<<sh<<endl;
		
		if(fd!=sd)
			nbroken=1;
		else if(fd==sd && fh+r.internalActivitiesList[this->firstActivityIndex].duration <= sh){
			int h;
			int d=fd;
			assert(d==sd);
			for(h=fh+r.internalActivitiesList[this->firstActivityIndex].duration; h<r.nHoursPerDay; h++)
				if(!breakDayHour[d][h])
					break;
					
			assert(h<=sh);	
				
			if(h!=sh)
				nbroken=1;
		}
		else if(fd==sd && sh+r.internalActivitiesList[this->secondActivityIndex].duration <= fh){
			int h;
			int d=sd;
			assert(d==fd);
			for(h=sh+r.internalActivitiesList[this->secondActivityIndex].duration; h<r.nHoursPerDay; h++)
				if(!breakDayHour[d][h])
					break;
					
			assert(h<=fh);	
				
			if(h!=fh)
				nbroken=1;
		}
		else
			nbroken=1;
	}
	
	assert(nbroken==0 || nbroken==1);

	if(conflictsString!=NULL && nbroken>0){
		QString s=tr("Time constraint two activities grouped broken for first activity with id=%1 (%2) and "
		 "second activity with id=%3 (%4), increases conflicts total by %5", "%1 is the id, %2 is the detailed description of the activity, %3 id, %4 det. descr.")
		 .arg(this->firstActivityId)
		 .arg(getActivityDetailedDescription(r, this->firstActivityId))
		 .arg(this->secondActivityId)
		 .arg(getActivityDetailedDescription(r, this->secondActivityId))
		 .arg(weightPercentage/100*nbroken);

		dl.append(s);
		cl.append(weightPercentage/100*nbroken);
	
		*conflictsString+= s+"\n";
	}
	
	if(weightPercentage==100)
		assert(nbroken==0);
	return nbroken * weightPercentage/100;
}

bool ConstraintTwoActivitiesGrouped::isRelatedToActivity(Rules& r, Activity* a)
{
	Q_UNUSED(r);

	if(this->firstActivityId==a->id)
		return true;
	if(this->secondActivityId==a->id)
		return true;
	return false;
}

bool ConstraintTwoActivitiesGrouped::isRelatedToTeacher(Teacher* t)
{
	Q_UNUSED(t);
	//if(t)
	//	;

	return false;
}

bool ConstraintTwoActivitiesGrouped::isRelatedToSubject(Subject* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintTwoActivitiesGrouped::isRelatedToActivityTag(ActivityTag* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintTwoActivitiesGrouped::isRelatedToStudentsSet(Rules& r, StudentsSet* s)
{
	Q_UNUSED(r);
	Q_UNUSED(s);
	/*if(s)
		;
	if(&r)
		;*/
		
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

ConstraintThreeActivitiesGrouped::ConstraintThreeActivitiesGrouped()
	: TimeConstraint()
{
	this->type = CONSTRAINT_THREE_ACTIVITIES_GROUPED;
}

ConstraintThreeActivitiesGrouped::ConstraintThreeActivitiesGrouped(double wp, int firstActId, int secondActId, int thirdActId)
	: TimeConstraint(wp)
{
	this->firstActivityId = firstActId;
	this->secondActivityId=secondActId;
	this->thirdActivityId=thirdActId;
	this->type = CONSTRAINT_THREE_ACTIVITIES_GROUPED;
}

bool ConstraintThreeActivitiesGrouped::computeInternalStructure(Rules& r)
{
	Activity* act;
	int i;
	for(i=0; i<r.nInternalActivities; i++){
		act=&r.internalActivitiesList[i];
		if(act->id==this->firstActivityId)
			break;
	}
	
	if(i==r.nInternalActivities){	
		//assert(0);
		QMessageBox::warning(NULL, tr("FET error in data"), 
			tr("Following constraint is wrong (refers to inexistent activity ids):\n%1").arg(this->getDetailedDescription(r)));
		return false;
	}

	this->firstActivityIndex=i;	

	////////
	
	for(i=0; i<r.nInternalActivities; i++){
		act=&r.internalActivitiesList[i];
		if(act->id==this->secondActivityId)
			break;
	}
	
	if(i==r.nInternalActivities){	
		//assert(0);
		QMessageBox::warning(NULL, tr("FET error in data"), 
			tr("Following constraint is wrong (refers to inexistent activity ids):\n%1").arg(this->getDetailedDescription(r)));
		return false;
	}

	this->secondActivityIndex=i;
	
	////////
	
	for(i=0; i<r.nInternalActivities; i++){
		act=&r.internalActivitiesList[i];
		if(act->id==this->thirdActivityId)
			break;
	}
	
	if(i==r.nInternalActivities){	
		//assert(0);
		QMessageBox::warning(NULL, tr("FET error in data"), 
			tr("Following constraint is wrong (refers to inexistent activity ids):\n%1").arg(this->getDetailedDescription(r)));
		return false;
	}

	this->thirdActivityIndex=i;
	
	if(firstActivityIndex==secondActivityIndex || firstActivityIndex==thirdActivityIndex || secondActivityIndex==thirdActivityIndex){
		//assert(0);
		QMessageBox::warning(NULL, tr("FET error in data"), 
			tr("Following constraint is wrong (refers to same activities):\n%1").arg(this->getDetailedDescription(r)));
		return false;
	}
	assert(firstActivityIndex!=secondActivityIndex && firstActivityIndex!=thirdActivityIndex && secondActivityIndex!=thirdActivityIndex);
	
	return true;
}

bool ConstraintThreeActivitiesGrouped::hasInactiveActivities(Rules& r)
{
	if(r.inactiveActivities.contains(this->firstActivityId))
		return true;
	if(r.inactiveActivities.contains(this->secondActivityId))
		return true;
	if(r.inactiveActivities.contains(this->thirdActivityId))
		return true;
	return false;
}

QString ConstraintThreeActivitiesGrouped::getXmlDescription(Rules& r)
{
	//to avoid non-used parameter warning
	Q_UNUSED(r);

	QString s="<ConstraintThreeActivitiesGrouped>\n";
	s+="	<Weight_Percentage>"+QString::number(this->weightPercentage)+"</Weight_Percentage>\n";
	//s+="	<Compulsory>";s+=yesNo(this->compulsory);s+="</Compulsory>\n";
	s+="	<First_Activity_Id>"+QString::number(this->firstActivityId)+"</First_Activity_Id>\n";
	s+="	<Second_Activity_Id>"+QString::number(this->secondActivityId)+"</Second_Activity_Id>\n";
	s+="	<Third_Activity_Id>"+QString::number(this->thirdActivityId)+"</Third_Activity_Id>\n";
	s+="</ConstraintThreeActivitiesGrouped>\n";
	return s;
}

QString ConstraintThreeActivitiesGrouped::getDescription(Rules& r)
{
	//to avoid non-used parameter warning
	Q_UNUSED(r);

	QString s;
	
	s=tr("Constraint three activities grouped:");
	s+=" ";
	
	s+=tr("first act. id: %1", "act.=activity").arg(this->firstActivityId);
	s+=", ";
	s+=tr("second act. id: %1", "act.=activity").arg(this->secondActivityId);
	s+=", ";
	s+=tr("third act. id: %1", "act.=activity").arg(this->thirdActivityId);
	s+=", ";
	s+=tr("WP:%1\%", "Weight percentage").arg(this->weightPercentage);

	return s;
}

QString ConstraintThreeActivitiesGrouped::getDetailedDescription(Rules& r)
{
	QString s=tr("Time constraint");s+="\n";
	s+=tr("Constraint three activities grouped (the activities must be placed in the same day, "
	 "one immediately following the other, as a block of three activities, in any order, possibly separated by breaks)"); s+="\n";
	
	s+=tr("Weight (percentage)=%1\%").arg(this->weightPercentage);s+="\n";

	s+=tr("First activity id=%1 (%2)", "%1 is the id, %2 is the detailed description of the activity.")
		.arg(this->firstActivityId)
		.arg(getActivityDetailedDescription(r, this->firstActivityId));
	s+="\n";

	s+=tr("Second activity id=%1 (%2)", "%1 is the id, %2 is the detailed description of the activity.")
		.arg(this->secondActivityId)
		.arg(getActivityDetailedDescription(r, this->secondActivityId));
	s+="\n";
	
	s+=tr("Third activity id=%1 (%2)", "%1 is the id, %2 is the detailed description of the activity.")
		.arg(this->thirdActivityId)
		.arg(getActivityDetailedDescription(r, this->thirdActivityId));
	s+="\n";

	return s;
}

double ConstraintThreeActivitiesGrouped::fitness(Solution& c, Rules& r, QList<double>& cl, QList<QString>&dl, QString* conflictsString)
{
	//if the matrices subgroupsMatrix and teachersMatrix are already calculated, do not calculate them again!
	if(!c.teachersMatrixReady || !c.subgroupsMatrixReady){
		c.teachersMatrixReady=true;
		c.subgroupsMatrixReady=true;
	//if(crt_chrom!=&c || crt_rules!=&r || subgroups_conflicts<0 || teachers_conflicts<0 || c.changedForMatrixCalculation){
		subgroups_conflicts = c.getSubgroupsMatrix(r, subgroupsMatrix);
		teachers_conflicts = c.getTeachersMatrix(r, teachersMatrix);

		//crt_chrom=&c;
		//crt_rules=&r;
		
		c.changedForMatrixCalculation=false;
	}

	int nbroken;

	assert(r.internalStructureComputed);

	nbroken=0;
	if(c.times[this->firstActivityIndex]!=UNALLOCATED_TIME && c.times[this->secondActivityIndex]!=UNALLOCATED_TIME && c.times[this->thirdActivityIndex]!=UNALLOCATED_TIME){
		int fd=c.times[this->firstActivityIndex]%r.nDaysPerWeek; //the day when first activity was scheduled
		int fh=c.times[this->firstActivityIndex]/r.nDaysPerWeek; //the hour
		int sd=c.times[this->secondActivityIndex]%r.nDaysPerWeek; //the day when second activity was scheduled
		int sh=c.times[this->secondActivityIndex]/r.nDaysPerWeek; //the hour
		int td=c.times[this->thirdActivityIndex]%r.nDaysPerWeek; //the day when third activity was scheduled
		int th=c.times[this->thirdActivityIndex]/r.nDaysPerWeek; //the hour
		
		//cout<<"fd=="<<fd<<", fh=="<<fh<<", sd=="<<sd<<", sh=="<<sh<<endl;
		
/*		QFile file("output.txt");
		file.open(QIODevice::Append);
		QTextStream out(&file);
		*/
		
		if(!(fd==sd && fd==td))
			nbroken=1;
		else{
			assert(fd==sd && fd==td && sd==td);
			int a1=-1,a2=-1,a3=-1;
			if(fh>=sh && fh>=th && sh>=th){
				a1=thirdActivityIndex;
				a2=secondActivityIndex;
				a3=firstActivityIndex;
				//out<<"321"<<endl;
			}
			else if(fh>=sh && fh>=th && th>=sh){
				a1=secondActivityIndex;
				a2=thirdActivityIndex;
				a3=firstActivityIndex;
				//out<<"231"<<endl;
			}
			else if(sh>=fh && sh>=th && fh>=th){
				a1=thirdActivityIndex;
				a2=firstActivityIndex;
				a3=secondActivityIndex;
				//out<<"312"<<endl;
			}
			else if(sh>=fh && sh>=th && th>=fh){
				a1=firstActivityIndex;
				a2=thirdActivityIndex;
				a3=secondActivityIndex;
				//out<<"132"<<endl;
			}
			else if(th>=fh && th>=sh && fh>=sh){
				a1=secondActivityIndex;
				a2=firstActivityIndex;
				a3=thirdActivityIndex;
				//out<<"213"<<endl;
			}
			else if(th>=fh && th>=sh && sh>=fh){
				a1=firstActivityIndex;
				a2=secondActivityIndex;
				a3=thirdActivityIndex;
				//out<<"123"<<endl;
			}
			else
				assert(0);
			
			int a1d=c.times[a1]%r.nDaysPerWeek; //the day for a1
			int a1h=c.times[a1]/r.nDaysPerWeek; //the day for a1
			int a1dur=r.internalActivitiesList[a1].duration;

			int a2d=c.times[a2]%r.nDaysPerWeek; //the day for a1
			int a2h=c.times[a2]/r.nDaysPerWeek; //the day for a1
			int a2dur=r.internalActivitiesList[a2].duration;

			int a3d=c.times[a3]%r.nDaysPerWeek; //the day for a1
			int a3h=c.times[a3]/r.nDaysPerWeek; //the day for a1
			//int a3dur=r.internalActivitiesList[a3].duration;
			
			int hoursBetweenThem=-1;
			
			assert(a1d==a2d && a1d==a3d);
			
			if(a1h+a1dur<=a2h && a2h+a2dur<=a3h){
				hoursBetweenThem=0;
				for(int hh=a1h+a1dur; hh<a2h; hh++)
					if(!breakDayHour[a1d][hh])
						hoursBetweenThem++;

				for(int hh=a2h+a2dur; hh<a3h; hh++)
					if(!breakDayHour[a2d][hh])
						hoursBetweenThem++;
			}
			
			if(hoursBetweenThem==0)
				nbroken=0;
			else
				nbroken=1;
		}
	}
	
	assert(nbroken==0 || nbroken==1);

	if(conflictsString!=NULL && nbroken>0){
		QString s=tr("Time constraint three activities grouped broken for first activity with id=%1 (%2), "
		 "second activity with id=%3 (%4) and third activity with id=%5 (%6), increases conflicts total by %7",
		 "%1 is the id, %2 is the detailed description of the activity, %3 id, %4 det. descr., %5 id, %6 det. descr.")
		 .arg(this->firstActivityId)
		 .arg(getActivityDetailedDescription(r, this->firstActivityId))
		 .arg(this->secondActivityId)
		 .arg(getActivityDetailedDescription(r, this->secondActivityId))
		 .arg(this->thirdActivityId)
		 .arg(getActivityDetailedDescription(r, this->thirdActivityId))
		 .arg(weightPercentage/100*nbroken);

		dl.append(s);
		cl.append(weightPercentage/100*nbroken);
	
		*conflictsString+= s+"\n";
	}
	
	if(weightPercentage==100)
		assert(nbroken==0);
	return nbroken * weightPercentage/100;
}

bool ConstraintThreeActivitiesGrouped::isRelatedToActivity(Rules& r, Activity* a)
{
	Q_UNUSED(r);

	if(this->firstActivityId==a->id)
		return true;
	if(this->secondActivityId==a->id)
		return true;
	if(this->thirdActivityId==a->id)
		return true;
	return false;
}

bool ConstraintThreeActivitiesGrouped::isRelatedToTeacher(Teacher* t)
{
	Q_UNUSED(t);
	//if(t)
	//	;

	return false;
}

bool ConstraintThreeActivitiesGrouped::isRelatedToSubject(Subject* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintThreeActivitiesGrouped::isRelatedToActivityTag(ActivityTag* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintThreeActivitiesGrouped::isRelatedToStudentsSet(Rules& r, StudentsSet* s)
{
	Q_UNUSED(r);
	Q_UNUSED(s);
	/*if(s)
		;
	if(&r)
		;*/
		
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

ConstraintTwoActivitiesOrdered::ConstraintTwoActivitiesOrdered()
	: TimeConstraint()
{
	this->type = CONSTRAINT_TWO_ACTIVITIES_ORDERED;
}

ConstraintTwoActivitiesOrdered::ConstraintTwoActivitiesOrdered(double wp, int firstActId, int secondActId)
	: TimeConstraint(wp)
{
	this->firstActivityId = firstActId;
	this->secondActivityId=secondActId;
	this->type = CONSTRAINT_TWO_ACTIVITIES_ORDERED;
}

bool ConstraintTwoActivitiesOrdered::computeInternalStructure(Rules& r)
{
	Activity* act;
	int i;
	for(i=0; i<r.nInternalActivities; i++){
		act=&r.internalActivitiesList[i];
		if(act->id==this->firstActivityId)
			break;
	}
	
	if(i==r.nInternalActivities){	
		//assert(0);
		QMessageBox::warning(NULL, tr("FET error in data"), 
			tr("Following constraint is wrong (refers to inexistent activity ids):\n%1").arg(this->getDetailedDescription(r)));
		return false;
	}

	this->firstActivityIndex=i;	

	////////
	
	for(i=0; i<r.nInternalActivities; i++){
		act=&r.internalActivitiesList[i];
		if(act->id==this->secondActivityId)
			break;
	}
	
	if(i==r.nInternalActivities){	
		//assert(0);
		QMessageBox::warning(NULL, tr("FET error in data"), 
			tr("Following constraint is wrong (refers to inexistent activity ids):\n%1").arg(this->getDetailedDescription(r)));
		return false;
	}

	this->secondActivityIndex=i;
	
	if(firstActivityIndex==secondActivityIndex){	
		//assert(0);
		QMessageBox::warning(NULL, tr("FET error in data"), 
			tr("Following constraint is wrong (refers to same activities):\n%1").arg(this->getDetailedDescription(r)));
		return false;
	}
	assert(firstActivityIndex!=secondActivityIndex);
	
	return true;
}

bool ConstraintTwoActivitiesOrdered::hasInactiveActivities(Rules& r)
{
	if(r.inactiveActivities.contains(this->firstActivityId))
		return true;
	if(r.inactiveActivities.contains(this->secondActivityId))
		return true;
	return false;
}

QString ConstraintTwoActivitiesOrdered::getXmlDescription(Rules& r)
{
	//to avoid non-used parameter warning
	Q_UNUSED(r);

	QString s="<ConstraintTwoActivitiesOrdered>\n";
	s+="	<Weight_Percentage>"+QString::number(this->weightPercentage)+"</Weight_Percentage>\n";
	//s+="	<Compulsory>";s+=yesNo(this->compulsory);s+="</Compulsory>\n";
	s+="	<First_Activity_Id>"+QString::number(this->firstActivityId)+"</First_Activity_Id>\n";
	s+="	<Second_Activity_Id>"+QString::number(this->secondActivityId)+"</Second_Activity_Id>\n";
	s+="</ConstraintTwoActivitiesOrdered>\n";
	return s;
}

QString ConstraintTwoActivitiesOrdered::getDescription(Rules& r)
{
	//to avoid non-used parameter warning
	Q_UNUSED(r);

	QString s;
	
	s=tr("Constraint two activities ordered:");
	s+=" ";
	
	s+=tr("first act. id: %1", "act.=activity").arg(this->firstActivityId);
	s+=", ";
	s+=tr("second act. id: %1", "act.=activity").arg(this->secondActivityId);
	s+=", ";
	s+=tr("WP:%1\%", "Weight percentage").arg(this->weightPercentage);

	return s;
}

QString ConstraintTwoActivitiesOrdered::getDetailedDescription(Rules& r)
{
	QString s=tr("Time constraint");s+="\n";
	s+=tr("Constraint two activities ordered (second activity must be placed at any time in the week after the first"
	 " activity)"); s+="\n";

	s+=tr("Weight (percentage)=%1\%").arg(this->weightPercentage);s+="\n";

	s+=tr("First activity id=%1 (%2)", "%1 is the id, %2 is the detailed description of the activity.")
		.arg(this->firstActivityId)
		.arg(getActivityDetailedDescription(r, this->firstActivityId));
	s+="\n";

	s+=tr("Second activity id=%1 (%2)", "%1 is the id, %2 is the detailed description of the activity.")
		.arg(this->secondActivityId)
		.arg(getActivityDetailedDescription(r, this->secondActivityId));
	s+="\n";

	return s;
}

double ConstraintTwoActivitiesOrdered::fitness(Solution& c, Rules& r, QList<double>& cl, QList<QString>&dl, QString* conflictsString)
{
	//if the matrices subgroupsMatrix and teachersMatrix are already calculated, do not calculate them again!
	if(!c.teachersMatrixReady || !c.subgroupsMatrixReady){
		c.teachersMatrixReady=true;
		c.subgroupsMatrixReady=true;
	//if(crt_chrom!=&c || crt_rules!=&r || subgroups_conflicts<0 || teachers_conflicts<0 || c.changedForMatrixCalculation){
		subgroups_conflicts = c.getSubgroupsMatrix(r, subgroupsMatrix);
		teachers_conflicts = c.getTeachersMatrix(r, teachersMatrix);

		//crt_chrom=&c;
		//crt_rules=&r;
		
		c.changedForMatrixCalculation=false;
	}

	int nbroken;

	assert(r.internalStructureComputed);

	nbroken=0;
	if(c.times[this->firstActivityIndex]!=UNALLOCATED_TIME && c.times[this->secondActivityIndex]!=UNALLOCATED_TIME){
		int fd=c.times[this->firstActivityIndex]%r.nDaysPerWeek; //the day when first activity was scheduled
		int fh=c.times[this->firstActivityIndex]/r.nDaysPerWeek
		  + r.internalActivitiesList[this->firstActivityIndex].duration-1; //the end hour of first activity
		int sd=c.times[this->secondActivityIndex]%r.nDaysPerWeek; //the day when second activity was scheduled
		int sh=c.times[this->secondActivityIndex]/r.nDaysPerWeek; //the start hour of second activity
		
		if(!(fd<sd || (fd==sd && fh<sh)))
			nbroken=1;
	}
	
	assert(nbroken==0 || nbroken==1);

	if(conflictsString!=NULL && nbroken>0){
		QString s=tr("Time constraint two activities ordered broken for first activity with id=%1 (%2) and "
		 "second activity with id=%3 (%4), increases conflicts total by %5", "%1 is the id, %2 is the detailed description of the activity, %3 id, %4 det. descr.")
		 .arg(this->firstActivityId)
		 .arg(getActivityDetailedDescription(r, this->firstActivityId))
		 .arg(this->secondActivityId)
		 .arg(getActivityDetailedDescription(r, this->secondActivityId))
		 .arg(weightPercentage/100*nbroken);

		dl.append(s);
		cl.append(weightPercentage/100*nbroken);
	
		*conflictsString+= s+"\n";
	}
	
	if(weightPercentage==100)
		assert(nbroken==0);
	return nbroken * weightPercentage/100;
}

bool ConstraintTwoActivitiesOrdered::isRelatedToActivity(Rules& r, Activity* a)
{
	Q_UNUSED(r);

	if(this->firstActivityId==a->id)
		return true;
	if(this->secondActivityId==a->id)
		return true;
	return false;
}

bool ConstraintTwoActivitiesOrdered::isRelatedToTeacher(Teacher* t)
{
	Q_UNUSED(t);
	//if(t)
	//	;

	return false;
}

bool ConstraintTwoActivitiesOrdered::isRelatedToSubject(Subject* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintTwoActivitiesOrdered::isRelatedToActivityTag(ActivityTag* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintTwoActivitiesOrdered::isRelatedToStudentsSet(Rules& r, StudentsSet* s)
{
	Q_UNUSED(r);
	Q_UNUSED(s);
	/*if(s)
		;
	if(&r)
		;*/
		
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

ConstraintActivityEndsStudentsDay::ConstraintActivityEndsStudentsDay()
	: TimeConstraint()
{
	this->type = CONSTRAINT_ACTIVITY_ENDS_STUDENTS_DAY;
}

ConstraintActivityEndsStudentsDay::ConstraintActivityEndsStudentsDay(double wp, int actId)
	: TimeConstraint(wp)
{
	this->activityId = actId;
	this->type = CONSTRAINT_ACTIVITY_ENDS_STUDENTS_DAY;
}

bool ConstraintActivityEndsStudentsDay::computeInternalStructure(Rules& r)
{
	Activity* act;
	int i;
	for(i=0; i<r.nInternalActivities; i++){
		act=&r.internalActivitiesList[i];
		if(act->id==this->activityId)
			break;
	}
	
	if(i==r.nInternalActivities){	
		//assert(0);
		QMessageBox::warning(NULL, tr("FET error in data"), 
			tr("Following constraint is wrong (because it refers to invalid activity id. Please correct (maybe removing it is a solution)):\n%1").arg(this->getDetailedDescription(r)));
		return false;
	}

	this->activityIndex=i;	
	return true;
}

bool ConstraintActivityEndsStudentsDay::hasInactiveActivities(Rules& r)
{
	if(r.inactiveActivities.contains(this->activityId))
		return true;
	return false;
}

QString ConstraintActivityEndsStudentsDay::getXmlDescription(Rules& r)
{
	//to avoid non-used parameter warning
	Q_UNUSED(r);

	QString s="<ConstraintActivityEndsStudentsDay>\n";
	s+="	<Weight_Percentage>"+QString::number(this->weightPercentage)+"</Weight_Percentage>\n";
	//s+="	<Compulsory>";s+=yesNo(this->compulsory);s+="</Compulsory>\n";
	s+="	<Activity_Id>"+QString::number(this->activityId)+"</Activity_Id>\n";
	s+="</ConstraintActivityEndsStudentsDay>\n";
	return s;
}

QString ConstraintActivityEndsStudentsDay::getDescription(Rules& r)
{
	QString s;
	s+=tr("Act. id: %1 (%2) must end students' day", 
		"%1 is the id, %2 is the detailed description of the activity.")
		.arg(this->activityId)
		.arg(getActivityDetailedDescription(r, this->activityId));
	s+=", ";

	s+=tr("WP:%1\%", "Weight percentage").arg(this->weightPercentage);

	return s;
}

QString ConstraintActivityEndsStudentsDay::getDetailedDescription(Rules& r)
{
	QString s=tr("Time constraint");s+="\n";
	s+=tr("Activity must end students' day");s+="\n";
	s+=tr("Weight (percentage)=%1\%").arg(this->weightPercentage);s+="\n";
	s+=tr("Activity id=%1 (%2)", "%1 is the id, %2 is the detailed description of the activity.")
		.arg(this->activityId)
		.arg(getActivityDetailedDescription(r, this->activityId));s+="\n";

	return s;
}

double ConstraintActivityEndsStudentsDay::fitness(Solution& c, Rules& r, QList<double>& cl, QList<QString>&dl, QString* conflictsString)
{
	//if the matrices subgroupsMatrix and teachersMatrix are already calculated, do not calculate them again!
	if(!c.teachersMatrixReady || !c.subgroupsMatrixReady){
		c.teachersMatrixReady=true;
		c.subgroupsMatrixReady=true;
	//if(crt_chrom!=&c || crt_rules!=&r || subgroups_conflicts<0 || teachers_conflicts<0 || c.changedForMatrixCalculation){
		subgroups_conflicts = c.getSubgroupsMatrix(r, subgroupsMatrix);
		teachers_conflicts = c.getTeachersMatrix(r, teachersMatrix);

		//crt_chrom=&c;
		//crt_rules=&r;
		
		c.changedForMatrixCalculation=false;
	}

	int nbroken;

	assert(r.internalStructureComputed);

	nbroken=0;
	if(c.times[this->activityIndex]!=UNALLOCATED_TIME){
		int d=c.times[this->activityIndex]%r.nDaysPerWeek; //the day when this activity was scheduled
		int h=c.times[this->activityIndex]/r.nDaysPerWeek; //the hour
		
		int i=this->activityIndex;
		for(int j=0; j<r.internalActivitiesList[i].iSubgroupsList.count(); j++){
			int sb=r.internalActivitiesList[i].iSubgroupsList.at(j);
			for(int hh=h+r.internalActivitiesList[i].duration; hh<r.nHoursPerDay; hh++)
				if(subgroupsMatrix[sb][d][hh]>0){
					nbroken=1;
					break;
				}
			if(nbroken)
				break;
		}
	}

	if(conflictsString!=NULL && nbroken>0){
		QString s=tr("Time constraint activity ends students' day broken for activity with id=%1 (%2), increases conflicts total by %3",
		 "%1 is the id, %2 is the detailed description of the activity")
		 .arg(this->activityId)
		 .arg(getActivityDetailedDescription(r, this->activityId))
		 .arg(weightPercentage/100*nbroken);

		dl.append(s);
		cl.append(weightPercentage/100*nbroken);
	
		*conflictsString+= s+"\n";
	}

	if(weightPercentage==100)
		assert(nbroken==0);
	return nbroken * weightPercentage/100;
}

bool ConstraintActivityEndsStudentsDay::isRelatedToActivity(Rules& r, Activity* a)
{
	Q_UNUSED(r);

	if(this->activityId==a->id)
		return true;
	return false;
}

bool ConstraintActivityEndsStudentsDay::isRelatedToTeacher(Teacher* t)
{
	Q_UNUSED(t);
	//if(t)
	//	;

	return false;
}

bool ConstraintActivityEndsStudentsDay::isRelatedToSubject(Subject* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintActivityEndsStudentsDay::isRelatedToActivityTag(ActivityTag* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintActivityEndsStudentsDay::isRelatedToStudentsSet(Rules& r, StudentsSet* s)
{
	Q_UNUSED(r);
	Q_UNUSED(s);
	/*if(s)
		;
	if(&r)
		;*/
		
	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

ConstraintTeachersMinHoursDaily::ConstraintTeachersMinHoursDaily()
	: TimeConstraint()
{
	this->type=CONSTRAINT_TEACHERS_MIN_HOURS_DAILY;
	
	this->allowEmptyDays=true;
}

ConstraintTeachersMinHoursDaily::ConstraintTeachersMinHoursDaily(double wp, int minhours, bool _allowEmptyDays)
 : TimeConstraint(wp)
 {
	assert(minhours>0);
	this->minHoursDaily=minhours;
	
	this->allowEmptyDays=_allowEmptyDays;

	this->type=CONSTRAINT_TEACHERS_MIN_HOURS_DAILY;
}

bool ConstraintTeachersMinHoursDaily::computeInternalStructure(Rules& r)
{
	Q_UNUSED(r);
	
	if(allowEmptyDays==false){
		QString s=tr("Cannot generate a timetable with a constraint teachers min hours daily with allow empty days=false. Please modify it,"
			" so that it allows empty days. If you need a facility like that, please use constraint teachers min days per week");
		s+="\n\n";
		s+=tr("Constraint is:")+"\n"+this->getDetailedDescription(r);
		QMessageBox::warning(NULL, tr("FET warning"), s);
		
		return false;
	}
	
	return true;
}

bool ConstraintTeachersMinHoursDaily::hasInactiveActivities(Rules& r)
{
	Q_UNUSED(r);
	return false;
}

QString ConstraintTeachersMinHoursDaily::getXmlDescription(Rules& r){
	Q_UNUSED(r);

	QString s="<ConstraintTeachersMinHoursDaily>\n";
	s+="	<Weight_Percentage>"+QString::number(this->weightPercentage)+"</Weight_Percentage>\n";
	s+="	<Minimum_Hours_Daily>"+QString::number(this->minHoursDaily)+"</Minimum_Hours_Daily>\n";
	if(this->allowEmptyDays)
		s+="	<Allow_Empty_Days>true</Allow_Empty_Days>\n";
	else
		s+="	<Allow_Empty_Days>false</Allow_Empty_Days>\n";
	s+="</ConstraintTeachersMinHoursDaily>\n";
	return s;
}

QString ConstraintTeachersMinHoursDaily::getDescription(Rules& r){
	Q_UNUSED(r);

	QString s;
	s+=tr("Teachers min hours daily");s+=", ";
	s+=tr("WP:%1\%", "Weight percentage").arg(this->weightPercentage);s+=", ";
	s+=tr("mH:%1", "Min hours (daily)").arg(this->minHoursDaily);s+=", ";
	s+=tr("AED:%1", "Allow empty days").arg(yesNoTranslated(this->allowEmptyDays));

	return s;
}

QString ConstraintTeachersMinHoursDaily::getDetailedDescription(Rules& r){
	Q_UNUSED(r);

	QString s=tr("Time constraint");s+="\n";
	s+=tr("All teachers must respect the minimum number of hours daily"); s+="\n";
	s+=tr("Weight (percentage)=%1\%").arg(this->weightPercentage);s+="\n";
	s+=tr("Minimum hours daily=%1").arg(this->minHoursDaily);s+="\n";
	s+=tr("Allow empty days=%1").arg(yesNoTranslated(this->allowEmptyDays));s+="\n";
	//s+=tr("Note: FET is smart enough to use this constraint only on working days of the teachers");s+="\n";

	return s;
}

double ConstraintTeachersMinHoursDaily::fitness(Solution& c, Rules& r, QList<double>& cl, QList<QString>&dl, QString* conflictsString)
{
	//if the matrices subgroupsMatrix and teachersMatrix are already calculated, do not calculate them again!
	if(!c.teachersMatrixReady || !c.subgroupsMatrixReady){
		c.teachersMatrixReady=true;
		c.subgroupsMatrixReady=true;
	//if(crt_chrom!=&c || crt_rules!=&r || subgroups_conflicts<0 || teachers_conflicts<0 || c.changedForMatrixCalculation){
		subgroups_conflicts = c.getSubgroupsMatrix(r, subgroupsMatrix);
		teachers_conflicts = c.getTeachersMatrix(r, teachersMatrix);

		//crt_chrom=&c;
		//crt_rules=&r;
		
		c.changedForMatrixCalculation=false;
	}
	
	assert(this->allowEmptyDays==true);

	int nbroken;

	//without logging
	if(conflictsString==NULL){
		nbroken=0;
		for(int i=0; i<r.nInternalTeachers; i++){
			for(int d=0; d<r.nDaysPerWeek; d++){
				int n_hours_daily=0;
				for(int h=0; h<r.nHoursPerDay; h++)
					if(teachersMatrix[i][d][h]>0)
						n_hours_daily++;

				if(n_hours_daily>0 && n_hours_daily<this->minHoursDaily){
					nbroken++;
				}
			}
		}
	}
	//with logging
	else{
		nbroken=0;
		for(int i=0; i<r.nInternalTeachers; i++){
			for(int d=0; d<r.nDaysPerWeek; d++){
				int n_hours_daily=0;
				for(int h=0; h<r.nHoursPerDay; h++)
					if(teachersMatrix[i][d][h]>0)
						n_hours_daily++;

				if(n_hours_daily>0 && n_hours_daily<this->minHoursDaily){
					nbroken++;

					if(conflictsString!=NULL){
						QString s=(tr("Time constraint teachers min %1 hours daily broken for teacher %2, on day %3, length=%4.")
						 .arg(QString::number(this->minHoursDaily))
						 .arg(r.internalTeachersList[i]->name)
						 .arg(r.daysOfTheWeek[d])
						 .arg(n_hours_daily)
						 )
						 +
						 " "
						 +
						 (tr("This increases the conflicts total by %1").arg(QString::number(weightPercentage/100)));
						
						dl.append(s);
						cl.append(weightPercentage/100);
					
						*conflictsString+= s+"\n";
					}
				}
			}
		}
	}

	/*int na=0;
	for(int i=0; i<r.nInternalActivities; i++)
		if(c.times[i]!=UNALLOCATED_TIME)
			na++;*/
	if(c.nPlacedActivities==r.nInternalActivities)
	//if(na==r.nInternalActivities)
		//does not work for partial solutions
		if(weightPercentage==100)	
			assert(nbroken==0);
	return weightPercentage/100 * nbroken;
}

bool ConstraintTeachersMinHoursDaily::isRelatedToActivity(Rules& r, Activity* a)
{
	Q_UNUSED(a);
	Q_UNUSED(r);
	//if(a)
	//	;

	return false;
}

bool ConstraintTeachersMinHoursDaily::isRelatedToTeacher(Teacher* t)
{
	Q_UNUSED(t);
	//if(t)
	//	;

	return true;
}

bool ConstraintTeachersMinHoursDaily::isRelatedToSubject(Subject* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintTeachersMinHoursDaily::isRelatedToActivityTag(ActivityTag* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintTeachersMinHoursDaily::isRelatedToStudentsSet(Rules& r, StudentsSet* s)
{
	Q_UNUSED(r);
	Q_UNUSED(s);
	/*if(s)
		;
	if(&r)
		;*/

	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

ConstraintTeacherMinHoursDaily::ConstraintTeacherMinHoursDaily()
	: TimeConstraint()
{
	this->type=CONSTRAINT_TEACHER_MIN_HOURS_DAILY;
	
	this->allowEmptyDays=true;
}

ConstraintTeacherMinHoursDaily::ConstraintTeacherMinHoursDaily(double wp, int minhours, const QString& teacher, bool _allowEmptyDays)
 : TimeConstraint(wp)
 {
	assert(minhours>0);
	this->minHoursDaily=minhours;
	this->teacherName=teacher;
	
	this->allowEmptyDays=_allowEmptyDays;

	this->type=CONSTRAINT_TEACHER_MIN_HOURS_DAILY;
}

bool ConstraintTeacherMinHoursDaily::computeInternalStructure(Rules& r)
{
	this->teacher_ID=r.searchTeacher(this->teacherName);
	assert(this->teacher_ID>=0);
	
	if(allowEmptyDays==false){
		QString s=tr("Cannot generate a timetable with a constraint teacher min hours daily with allow empty days=false. Please modify it,"
			" so that it allows empty days. If you need a facility like that, please use constraint teacher min days per week");
		s+="\n\n";
		s+=tr("Constraint is:")+"\n"+this->getDetailedDescription(r);
		QMessageBox::warning(NULL, tr("FET warning"), s);
		
		return false;
	}
	
	return true;
}

bool ConstraintTeacherMinHoursDaily::hasInactiveActivities(Rules& r)
{
	Q_UNUSED(r);
	return false;
}

QString ConstraintTeacherMinHoursDaily::getXmlDescription(Rules& r){
	Q_UNUSED(r);

	QString s="<ConstraintTeacherMinHoursDaily>\n";
	s+="	<Weight_Percentage>"+QString::number(this->weightPercentage)+"</Weight_Percentage>\n";
	s+="	<Teacher_Name>"+protect(this->teacherName)+"</Teacher_Name>\n";
	s+="	<Minimum_Hours_Daily>"+QString::number(this->minHoursDaily)+"</Minimum_Hours_Daily>\n";
	if(this->allowEmptyDays)
		s+="	<Allow_Empty_Days>true</Allow_Empty_Days>\n";
	else
		s+="	<Allow_Empty_Days>false</Allow_Empty_Days>\n";
	s+="</ConstraintTeacherMinHoursDaily>\n";
	return s;
}

QString ConstraintTeacherMinHoursDaily::getDescription(Rules& r){
	Q_UNUSED(r);

	QString s;
	s+=tr("Teacher min hours daily");s+=", ";
	s+=tr("WP:%1\%", "Weight percentage").arg(this->weightPercentage);s+=", ";
	s+=tr("T:%1", "Teacher").arg(this->teacherName);s+=", ";
	s+=tr("mH:%1", "Minimum hours (daily)").arg(this->minHoursDaily);s+=", ";
	s+=tr("AED:%1", "Allow empty days").arg(yesNoTranslated(this->allowEmptyDays));

	return s;
}

QString ConstraintTeacherMinHoursDaily::getDetailedDescription(Rules& r){
	Q_UNUSED(r);

	QString s=tr("Time constraint");s+="\n";
	s+=tr("A teacher must respect the minimum number of hours daily");s+="\n";
	s+=tr("Weight (percentage)=%1\%").arg(this->weightPercentage);s+="\n";
	s+=tr("Teacher=%1").arg(this->teacherName);s+="\n";
	s+=tr("Minimum hours daily=%1").arg(this->minHoursDaily);s+="\n";
	s+=tr("Allow empty days=%1").arg(yesNoTranslated(this->allowEmptyDays));s+="\n";
	//s+=tr("Note: FET is smart enough to use this constraint only on working days of the teacher");s+="\n";

	return s;
}

double ConstraintTeacherMinHoursDaily::fitness(Solution& c, Rules& r, QList<double>& cl, QList<QString>&dl, QString* conflictsString)
{
	//if the matrices subgroupsMatrix and teachersMatrix are already calculated, do not calculate them again!
	if(!c.teachersMatrixReady || !c.subgroupsMatrixReady){
		c.teachersMatrixReady=true;
		c.subgroupsMatrixReady=true;
	//if(crt_chrom!=&c || crt_rules!=&r || subgroups_conflicts<0 || teachers_conflicts<0 || c.changedForMatrixCalculation){
		subgroups_conflicts = c.getSubgroupsMatrix(r, subgroupsMatrix);
		teachers_conflicts = c.getTeachersMatrix(r, teachersMatrix);

		//crt_chrom=&c;
		//crt_rules=&r;
		
		c.changedForMatrixCalculation=false;
	}
	
	assert(this->allowEmptyDays==true);

	int nbroken;

	//without logging
	if(conflictsString==NULL){
		nbroken=0;
		int i=this->teacher_ID;
		for(int d=0; d<r.nDaysPerWeek; d++){
			int n_hours_daily=0;
			for(int h=0; h<r.nHoursPerDay; h++)
				if(teachersMatrix[i][d][h]>0)
					n_hours_daily++;

			if(n_hours_daily>0 && n_hours_daily<this->minHoursDaily){
				nbroken++;
			}
		}
	}
	//with logging
	else{
		nbroken=0;
		int i=this->teacher_ID;
		for(int d=0; d<r.nDaysPerWeek; d++){
			int n_hours_daily=0;
			for(int h=0; h<r.nHoursPerDay; h++)
				if(teachersMatrix[i][d][h]>0)
					n_hours_daily++;

			if(n_hours_daily>0 && n_hours_daily<this->minHoursDaily){
				nbroken++;

				if(conflictsString!=NULL){
					QString s=(tr(
					 "Time constraint teacher min %1 hours daily broken for teacher %2, on day %3, length=%4.")
					 .arg(QString::number(this->minHoursDaily))
					 .arg(r.internalTeachersList[i]->name)
					 .arg(r.daysOfTheWeek[d])
					 .arg(n_hours_daily)
					 )
					 +" "
					 +
					 tr("This increases the conflicts total by %1").arg(QString::number(weightPercentage/100));
						
					dl.append(s);
					cl.append(weightPercentage/100);
				
					*conflictsString+= s+"\n";
				}
			}
		}
	}

	/*int na=0;
	for(int i=0; i<r.nInternalActivities; i++)
		if(c.times[i]!=UNALLOCATED_TIME)
			na++;*/
	if(c.nPlacedActivities==r.nInternalActivities)
	//if(na==r.nInternalActivities)
		//does not work for partial solutions
		if(weightPercentage==100)
			assert(nbroken==0);
			
	return weightPercentage/100 * nbroken;
}

bool ConstraintTeacherMinHoursDaily::isRelatedToActivity(Rules& r, Activity* a)
{
	Q_UNUSED(r);
	Q_UNUSED(a);
	//if(a)
	//	;

	return false;
}

bool ConstraintTeacherMinHoursDaily::isRelatedToTeacher(Teacher* t)
{
	if(this->teacherName==t->name)
		return true;
	return false;
}

bool ConstraintTeacherMinHoursDaily::isRelatedToSubject(Subject* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintTeacherMinHoursDaily::isRelatedToActivityTag(ActivityTag* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintTeacherMinHoursDaily::isRelatedToStudentsSet(Rules& r, StudentsSet* s)
{
	Q_UNUSED(r);
	Q_UNUSED(s);
	/*if(s)
		;
	if(&r)
		;*/

	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

ConstraintTeacherMinDaysPerWeek::ConstraintTeacherMinDaysPerWeek()
	: TimeConstraint()
{
	this->type=CONSTRAINT_TEACHER_MIN_DAYS_PER_WEEK;
}

ConstraintTeacherMinDaysPerWeek::ConstraintTeacherMinDaysPerWeek(double wp, int mindays, const QString& teacher)
 : TimeConstraint(wp)
 {
	assert(mindays>0);
	this->minDaysPerWeek=mindays;
	this->teacherName=teacher;

	this->type=CONSTRAINT_TEACHER_MIN_DAYS_PER_WEEK;
}

bool ConstraintTeacherMinDaysPerWeek::computeInternalStructure(Rules& r)
{
	this->teacher_ID=r.searchTeacher(this->teacherName);
	assert(this->teacher_ID>=0);
	return true;
}

bool ConstraintTeacherMinDaysPerWeek::hasInactiveActivities(Rules& r)
{
	Q_UNUSED(r);
	return false;
}

QString ConstraintTeacherMinDaysPerWeek::getXmlDescription(Rules& r){
	Q_UNUSED(r);

	QString s="<ConstraintTeacherMinDaysPerWeek>\n";
	s+="	<Weight_Percentage>"+QString::number(this->weightPercentage)+"</Weight_Percentage>\n";
	s+="	<Teacher_Name>"+protect(this->teacherName)+"</Teacher_Name>\n";
	s+="	<Minimum_Days_Per_Week>"+QString::number(this->minDaysPerWeek)+"</Minimum_Days_Per_Week>\n";
	s+="</ConstraintTeacherMinDaysPerWeek>\n";
	return s;
}

QString ConstraintTeacherMinDaysPerWeek::getDescription(Rules& r){
	Q_UNUSED(r);

	QString s;
	s+=tr("Teacher min days per week");s+=", ";
	s+=tr("WP:%1\%", "Weight percentage").arg(this->weightPercentage);s+=", ";
	s+=tr("T:%1", "Teacher").arg(this->teacherName);s+=", ";
	s+=tr("mD:%1", "Minimum days per week").arg(this->minDaysPerWeek);//s+=", ";

	return s;
}

QString ConstraintTeacherMinDaysPerWeek::getDetailedDescription(Rules& r){
	Q_UNUSED(r);

	QString s=tr("Time constraint");s+="\n";
	s+=tr("A teacher must respect the minimum number of days per week");s+="\n";
	s+=tr("Weight (percentage)=%1\%").arg(this->weightPercentage);s+="\n";
	s+=tr("Teacher=%1").arg(this->teacherName);s+="\n";
	s+=tr("Minimum days per week=%1").arg(this->minDaysPerWeek);s+="\n";

	return s;
}

double ConstraintTeacherMinDaysPerWeek::fitness(Solution& c, Rules& r, QList<double>& cl, QList<QString>&dl, QString* conflictsString)
{
	//if the matrices subgroupsMatrix and teachersMatrix are already calculated, do not calculate them again!
	if(!c.teachersMatrixReady || !c.subgroupsMatrixReady){
		c.teachersMatrixReady=true;
		c.subgroupsMatrixReady=true;
	//if(crt_chrom!=&c || crt_rules!=&r || subgroups_conflicts<0 || teachers_conflicts<0 || c.changedForMatrixCalculation){
		subgroups_conflicts = c.getSubgroupsMatrix(r, subgroupsMatrix);
		teachers_conflicts = c.getTeachersMatrix(r, teachersMatrix);

		//crt_chrom=&c;
		//crt_rules=&r;
		
		c.changedForMatrixCalculation=false;
	}

	int nbroken;

	nbroken=0;
	int i=this->teacher_ID;
	int nd=0;
	for(int d=0; d<r.nDaysPerWeek; d++){
		for(int h=0; h<r.nHoursPerDay; h++){
			if(teachersMatrix[i][d][h]>0){
				nd++;
				break;
			}
		}
	}

	if(nd<this->minDaysPerWeek){
		nbroken+=this->minDaysPerWeek-nd;

		if(conflictsString!=NULL){
			QString s=(tr(
			 "Time constraint teacher min %1 days per week broken for teacher %2.")
			 .arg(QString::number(this->minDaysPerWeek))
			 .arg(r.internalTeachersList[i]->name)
			 )
			 +" "
			 +
			 tr("This increases the conflicts total by %1").arg(QString::number(double(nbroken)*weightPercentage/100));
				
			dl.append(s);
			cl.append(double(nbroken)*weightPercentage/100);
		
			*conflictsString+= s+"\n";
		}
	}

	if(c.nPlacedActivities==r.nInternalActivities)
		if(weightPercentage==100)
			assert(nbroken==0);
			
	return weightPercentage/100 * nbroken;
}

bool ConstraintTeacherMinDaysPerWeek::isRelatedToActivity(Rules& r, Activity* a)
{
	Q_UNUSED(r);
	Q_UNUSED(a);

	return false;
}

bool ConstraintTeacherMinDaysPerWeek::isRelatedToTeacher(Teacher* t)
{
	if(this->teacherName==t->name)
		return true;
	return false;
}

bool ConstraintTeacherMinDaysPerWeek::isRelatedToSubject(Subject* s)
{
	Q_UNUSED(s);

	return false;
}

bool ConstraintTeacherMinDaysPerWeek::isRelatedToActivityTag(ActivityTag* s)
{
	Q_UNUSED(s);

	return false;
}

bool ConstraintTeacherMinDaysPerWeek::isRelatedToStudentsSet(Rules& r, StudentsSet* s)
{
	Q_UNUSED(r);
	Q_UNUSED(s);

	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

ConstraintTeachersMinDaysPerWeek::ConstraintTeachersMinDaysPerWeek()
	: TimeConstraint()
{
	this->type=CONSTRAINT_TEACHERS_MIN_DAYS_PER_WEEK;
}

ConstraintTeachersMinDaysPerWeek::ConstraintTeachersMinDaysPerWeek(double wp, int mindays)
 : TimeConstraint(wp)
 {
	assert(mindays>0);
	this->minDaysPerWeek=mindays;

	this->type=CONSTRAINT_TEACHERS_MIN_DAYS_PER_WEEK;
}

bool ConstraintTeachersMinDaysPerWeek::computeInternalStructure(Rules& r)
{
	Q_UNUSED(r);
	return true;
}

bool ConstraintTeachersMinDaysPerWeek::hasInactiveActivities(Rules& r)
{
	Q_UNUSED(r);
	return false;
}

QString ConstraintTeachersMinDaysPerWeek::getXmlDescription(Rules& r){
	Q_UNUSED(r);

	QString s="<ConstraintTeachersMinDaysPerWeek>\n";
	s+="	<Weight_Percentage>"+QString::number(this->weightPercentage)+"</Weight_Percentage>\n";
	s+="	<Minimum_Days_Per_Week>"+QString::number(this->minDaysPerWeek)+"</Minimum_Days_Per_Week>\n";
	s+="</ConstraintTeachersMinDaysPerWeek>\n";
	return s;
}

QString ConstraintTeachersMinDaysPerWeek::getDescription(Rules& r){
	Q_UNUSED(r);

	QString s;
	s+=tr("Teachers min days per week");s+=", ";
	s+=tr("WP:%1\%", "Weight percentage").arg(this->weightPercentage);s+=", ";
	s+=tr("mD:%1", "Minimum days per week").arg(this->minDaysPerWeek);//s+=", ";

	return s;
}

QString ConstraintTeachersMinDaysPerWeek::getDetailedDescription(Rules& r){
	Q_UNUSED(r);

	QString s=tr("Time constraint");s+="\n";
	s+=tr("All teachers must respect the minimum number of days per week");s+="\n";
	s+=tr("Weight (percentage)=%1\%").arg(this->weightPercentage);s+="\n";
	s+=tr("Minimum days per week=%1").arg(this->minDaysPerWeek);s+="\n";

	return s;
}

double ConstraintTeachersMinDaysPerWeek::fitness(Solution& c, Rules& r, QList<double>& cl, QList<QString>&dl, QString* conflictsString)
{
	//if the matrices subgroupsMatrix and teachersMatrix are already calculated, do not calculate them again!
	if(!c.teachersMatrixReady || !c.subgroupsMatrixReady){
		c.teachersMatrixReady=true;
		c.subgroupsMatrixReady=true;
	//if(crt_chrom!=&c || crt_rules!=&r || subgroups_conflicts<0 || teachers_conflicts<0 || c.changedForMatrixCalculation){
		subgroups_conflicts = c.getSubgroupsMatrix(r, subgroupsMatrix);
		teachers_conflicts = c.getTeachersMatrix(r, teachersMatrix);

		//crt_chrom=&c;
		//crt_rules=&r;
		
		c.changedForMatrixCalculation=false;
	}

	int nbrokentotal=0;
	for(int i=0; i<r.nInternalTeachers; i++){
		int nbroken;

		nbroken=0;
		//int i=this->teacher_ID;
		int nd=0;
		for(int d=0; d<r.nDaysPerWeek; d++){
			for(int h=0; h<r.nHoursPerDay; h++){
				if(teachersMatrix[i][d][h]>0){
					nd++;
					break;
				}
			}
		}

		if(nd<this->minDaysPerWeek){
			nbroken+=this->minDaysPerWeek-nd;
			nbrokentotal+=nbroken;

			if(conflictsString!=NULL){
				QString s=(tr(
				 "Time constraint teachers min %1 days per week broken for teacher %2.")
				 .arg(QString::number(this->minDaysPerWeek))
				 .arg(r.internalTeachersList[i]->name)
				 )
				 +" "
				 +
				 tr("This increases the conflicts total by %1").arg(QString::number(double(nbroken)*weightPercentage/100));
					
				dl.append(s);
				cl.append(double(nbroken)*weightPercentage/100);
			
				*conflictsString+= s+"\n";
			}
		}
	}

	if(c.nPlacedActivities==r.nInternalActivities)
		if(weightPercentage==100)
			assert(nbrokentotal==0);
			
	return weightPercentage/100 * nbrokentotal;
}

bool ConstraintTeachersMinDaysPerWeek::isRelatedToActivity(Rules& r, Activity* a)
{
	Q_UNUSED(r);
	Q_UNUSED(a);

	return false;
}

bool ConstraintTeachersMinDaysPerWeek::isRelatedToTeacher(Teacher* t)
{
	Q_UNUSED(t);
	return true;
}

bool ConstraintTeachersMinDaysPerWeek::isRelatedToSubject(Subject* s)
{
	Q_UNUSED(s);

	return false;
}

bool ConstraintTeachersMinDaysPerWeek::isRelatedToActivityTag(ActivityTag* s)
{
	Q_UNUSED(s);

	return false;
}

bool ConstraintTeachersMinDaysPerWeek::isRelatedToStudentsSet(Rules& r, StudentsSet* s)
{
	Q_UNUSED(r);
	Q_UNUSED(s);

	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

ConstraintTeacherIntervalMaxDaysPerWeek::ConstraintTeacherIntervalMaxDaysPerWeek()
	: TimeConstraint()
{
	this->type=CONSTRAINT_TEACHER_INTERVAL_MAX_DAYS_PER_WEEK;
}

ConstraintTeacherIntervalMaxDaysPerWeek::ConstraintTeacherIntervalMaxDaysPerWeek(double wp, int maxnd, QString tn, int sh, int eh)
	 : TimeConstraint(wp)
{
	this->teacherName = tn;
	this->maxDaysPerWeek=maxnd;
	this->type=CONSTRAINT_TEACHER_INTERVAL_MAX_DAYS_PER_WEEK;
	this->startHour=sh;
	this->endHour=eh;
	assert(sh<eh);
	assert(sh>=0);
}

bool ConstraintTeacherIntervalMaxDaysPerWeek::computeInternalStructure(Rules& r)
{
	this->teacher_ID=r.searchTeacher(this->teacherName);
	assert(this->teacher_ID>=0);
	if(this->startHour>=this->endHour){
		QMessageBox::warning(NULL, tr("FET warning"),
		 tr("Constraint teacher interval max days per week is wrong because start hour >= end hour."
		 " Please correct it. Constraint is:\n%1").arg(this->getDetailedDescription(r)));

		return false;
	}
	if(this->startHour<0){
		QMessageBox::warning(NULL, tr("FET warning"),
		 tr("Constraint teacher interval max days per week is wrong because start hour < first hour or the day."
		 " Please correct it. Constraint is:\n%1").arg(this->getDetailedDescription(r)));

		return false;
	}
	if(this->endHour>r.nHoursPerDay){
		QMessageBox::warning(NULL, tr("FET warning"),
		 tr("Constraint teacher interval max days per week is wrong because end hour > number of hours per day."
		 " Please correct it. Constraint is:\n%1").arg(this->getDetailedDescription(r)));

		return false;
	}
	return true;
}

bool ConstraintTeacherIntervalMaxDaysPerWeek::hasInactiveActivities(Rules& r)
{
	Q_UNUSED(r);
	return false;
}

QString ConstraintTeacherIntervalMaxDaysPerWeek::getXmlDescription(Rules& r)
{
	Q_UNUSED(r);

	QString s="<ConstraintTeacherIntervalMaxDaysPerWeek>\n";
	s+="	<Weight_Percentage>"+QString::number(this->weightPercentage)+"</Weight_Percentage>\n";
	//s+="	<Compulsory>";s+=yesNo(this->compulsory);s+="</Compulsory>\n";
	s+="	<Teacher_Name>"+protect(this->teacherName)+"</Teacher_Name>\n";
	s+="	<Interval_Start_Hour>"+protect(r.hoursOfTheDay[this->startHour])+"</Interval_Start_Hour>\n";
	if(this->endHour < r.nHoursPerDay){
		s+="	<Interval_End_Hour>"+protect(r.hoursOfTheDay[this->endHour])+"</Interval_End_Hour>\n";
	}
	else{
		s+="	<Interval_End_Hour></Interval_End_Hour>\n";
		s+="	<!-- Interval_End_Hour void means the end of the day (which has no name) -->\n";
	}
	s+="	<Max_Days_Per_Week>"+QString::number(this->maxDaysPerWeek)+"</Max_Days_Per_Week>\n";
	s+="</ConstraintTeacherIntervalMaxDaysPerWeek>\n";
	return s;
}

QString ConstraintTeacherIntervalMaxDaysPerWeek::getDescription(Rules& r){
	Q_UNUSED(r);

	QString s=tr("Teacher interval max days per week");s+=", ";
	s+=tr("WP:%1\%", "Abbreviation for weight percentage").arg(this->weightPercentage);s+=", ";
	s+=tr("T:%1", "Abbreviation for teacher").arg(this->teacherName);s+=", ";
	s+=tr("ISH:%1", "Abbreviation for interval start hour").arg(r.hoursOfTheDay[this->startHour]);s+=", ";
	if(this->endHour<r.nHoursPerDay)
		s+=tr("IEH:%1", "Abbreviation for interval end hour").arg(r.hoursOfTheDay[this->endHour]);
	else
		s+=tr("IEH:%1", "Abbreviation for interval end hour").arg(tr("End of the day"));
	s+=", ";
	s+=tr("MD:%1", "Abbreviation for max days").arg(this->maxDaysPerWeek);

	return s;
}

QString ConstraintTeacherIntervalMaxDaysPerWeek::getDetailedDescription(Rules& r){
	Q_UNUSED(r);

	QString s=tr("Time constraint");s+="\n";
	s+=tr("A teacher respects working in an hourly interval a maximum number of days per week");s+="\n";
	s+=tr("Weight (percentage)=%1\%").arg(this->weightPercentage);s+="\n";
	s+=tr("Teacher=%1").arg(this->teacherName);s+="\n";
	s+=tr("Interval start hour=%1").arg(r.hoursOfTheDay[this->startHour]);s+="\n";

	if(this->endHour<r.nHoursPerDay)
		s+=tr("Interval end hour=%1").arg(r.hoursOfTheDay[this->endHour]);
	else
		s+=tr("Interval end hour=%1").arg(tr("End of the day"));
	s+="\n";

	s+=tr("Maximum days per week=%1").arg(this->maxDaysPerWeek);s+="\n";

	return s;
}

double ConstraintTeacherIntervalMaxDaysPerWeek::fitness(Solution& c, Rules& r, QList<double>& cl, QList<QString>&dl, QString *conflictsString)
{
	//if the matrices subgroupsMatrix and teachersMatrix are already calculated, do not calculate them again!
	if(!c.teachersMatrixReady || !c.subgroupsMatrixReady){
		c.teachersMatrixReady=true;
		c.subgroupsMatrixReady=true;
	//if(crt_chrom!=&c || crt_rules!=&r || subgroups_conflicts<0 || teachers_conflicts<0 || c.changedForMatrixCalculation){
		subgroups_conflicts = c.getSubgroupsMatrix(r, subgroupsMatrix);
		teachers_conflicts = c.getTeachersMatrix(r, teachersMatrix);

		//crt_chrom=&c;
		//crt_rules=&r;
		
		c.changedForMatrixCalculation=false;
	}

	int nbroken;
	
	int t=this->teacher_ID;

	nbroken=0;
	bool ocDay[MAX_DAYS_PER_WEEK];
	for(int d=0; d<r.nDaysPerWeek; d++){
		ocDay[d]=false;
		for(int h=startHour; h<endHour; h++){
			if(teachersMatrix[t][d][h]>0){
				ocDay[d]=true;
			}
		}
	}
	int nOcDays=0;
	for(int d=0; d<r.nDaysPerWeek; d++)
		if(ocDay[d])
			nOcDays++;
	if(nOcDays > this->maxDaysPerWeek){
		nbroken+=nOcDays-this->maxDaysPerWeek;

		if(nbroken>0){
			QString s= tr("Time constraint teacher interval max days per week broken for teacher: %1, allowed %2 days, required %3 days.")
			 .arg(r.internalTeachersList[t]->name)
			 .arg(this->maxDaysPerWeek)
			 .arg(nOcDays);
			s+=" ";
			s += tr("This increases the conflicts total by %1")
			 .arg(nbroken*weightPercentage/100);
			 
			dl.append(s);
			cl.append(nbroken*weightPercentage/100);
		
			*conflictsString += s+"\n";
		}
	}
	//cout<<"teacher = "<<qPrintable(this->teacherName)<<", nOcDays = "<<nOcDays<<", max days per week = "<<this->maxDaysPerWeek<<endl;

	if(weightPercentage==100)
		assert(nbroken==0);
	return weightPercentage/100 * nbroken;
}

bool ConstraintTeacherIntervalMaxDaysPerWeek::isRelatedToActivity(Rules& r, Activity* a)
{
	Q_UNUSED(r);
	Q_UNUSED(a);
	//if(a)
	//	;

	return false;
}

bool ConstraintTeacherIntervalMaxDaysPerWeek::isRelatedToTeacher(Teacher* t)
{
	if(this->teacherName==t->name)
		return true;
	return false;
}

bool ConstraintTeacherIntervalMaxDaysPerWeek::isRelatedToSubject(Subject* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintTeacherIntervalMaxDaysPerWeek::isRelatedToActivityTag(ActivityTag* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintTeacherIntervalMaxDaysPerWeek::isRelatedToStudentsSet(Rules& r, StudentsSet* s)
{
	Q_UNUSED(r);
	Q_UNUSED(s);
	/*if(s)
		;
	if(&r)
		;*/

	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

ConstraintTeachersIntervalMaxDaysPerWeek::ConstraintTeachersIntervalMaxDaysPerWeek()
	: TimeConstraint()
{
	this->type=CONSTRAINT_TEACHERS_INTERVAL_MAX_DAYS_PER_WEEK;
}

ConstraintTeachersIntervalMaxDaysPerWeek::ConstraintTeachersIntervalMaxDaysPerWeek(double wp, int maxnd, int sh, int eh)
	 : TimeConstraint(wp)
{
	this->maxDaysPerWeek=maxnd;
	this->type=CONSTRAINT_TEACHERS_INTERVAL_MAX_DAYS_PER_WEEK;
	this->startHour=sh;
	this->endHour=eh;
	assert(sh<eh);
	assert(sh>=0);
}

bool ConstraintTeachersIntervalMaxDaysPerWeek::computeInternalStructure(Rules& r)
{
	//this->teacher_ID=r.searchTeacher(this->teacherName);
	//assert(this->teacher_ID>=0);
	if(this->startHour>=this->endHour){
		QMessageBox::warning(NULL, tr("FET warning"),
		 tr("Constraint teacher interval max days per week is wrong because start hour >= end hour."
		 " Please correct it. Constraint is:\n%1").arg(this->getDetailedDescription(r)));

		return false;
	}
	if(this->startHour<0){
		QMessageBox::warning(NULL, tr("FET warning"),
		 tr("Constraint teacher interval max days per week is wrong because start hour < first hour or the day."
		 " Please correct it. Constraint is:\n%1").arg(this->getDetailedDescription(r)));

		return false;
	}
	if(this->endHour>r.nHoursPerDay){
		QMessageBox::warning(NULL, tr("FET warning"),
		 tr("Constraint teacher interval max days per week is wrong because end hour > number of hours per day."
		 " Please correct it. Constraint is:\n%1").arg(this->getDetailedDescription(r)));

		return false;
	}
	return true;
}

bool ConstraintTeachersIntervalMaxDaysPerWeek::hasInactiveActivities(Rules& r)
{
	Q_UNUSED(r);
	return false;
}

QString ConstraintTeachersIntervalMaxDaysPerWeek::getXmlDescription(Rules& r)
{
	Q_UNUSED(r);

	QString s="<ConstraintTeachersIntervalMaxDaysPerWeek>\n";
	s+="	<Weight_Percentage>"+QString::number(this->weightPercentage)+"</Weight_Percentage>\n";
	//s+="	<Teacher_Name>"+protect(this->teacherName)+"</Teacher_Name>\n";
	s+="	<Interval_Start_Hour>"+protect(r.hoursOfTheDay[this->startHour])+"</Interval_Start_Hour>\n";
	if(this->endHour < r.nHoursPerDay){
		s+="	<Interval_End_Hour>"+protect(r.hoursOfTheDay[this->endHour])+"</Interval_End_Hour>\n";
	}
	else{
		s+="	<Interval_End_Hour></Interval_End_Hour>\n";
		s+="	<!-- Interval_End_Hour void means the end of the day (which has no name) -->\n";
	}
	s+="	<Max_Days_Per_Week>"+QString::number(this->maxDaysPerWeek)+"</Max_Days_Per_Week>\n";
	s+="</ConstraintTeachersIntervalMaxDaysPerWeek>\n";
	return s;
}

QString ConstraintTeachersIntervalMaxDaysPerWeek::getDescription(Rules& r){
	Q_UNUSED(r);

	QString s=tr("Teachers interval max days per week");s+=", ";
	s+=tr("WP:%1\%", "Abbreviation for weight percentage").arg(this->weightPercentage);s+=", ";
	s+=tr("ISH:%1", "Abbreviation for interval start hour").arg(r.hoursOfTheDay[this->startHour]);
	s+=", ";
	if(this->endHour<r.nHoursPerDay)
		s+=tr("IEH:%1", "Abbreviation for interval end hour").arg(r.hoursOfTheDay[this->endHour]);
	else
		s+=tr("IEH:%1", "Abbreviation for interval end hour").arg(tr("End of the day"));
	s+=", ";
	s+=tr("MD:%1", "Abbreviation for max days").arg(this->maxDaysPerWeek);

	return s;
}

QString ConstraintTeachersIntervalMaxDaysPerWeek::getDetailedDescription(Rules& r){
	Q_UNUSED(r);

	QString s=tr("Time constraint");s+="\n";
	s+=tr("All teachers respect working in an hourly interval a maximum number of days per week");s+="\n";
	s+=tr("Weight (percentage)=%1\%").arg(this->weightPercentage);s+="\n";
	s+=tr("Interval start hour=%1").arg(r.hoursOfTheDay[this->startHour]);s+="\n";

	if(this->endHour<r.nHoursPerDay)
		s+=tr("Interval end hour=%1").arg(r.hoursOfTheDay[this->endHour]);
	else
		s+=tr("Interval end hour=%1").arg(tr("End of the day"));
	s+="\n";

	s+=tr("Maximum days per week=%1").arg(this->maxDaysPerWeek);s+="\n";

	return s;
}

double ConstraintTeachersIntervalMaxDaysPerWeek::fitness(Solution& c, Rules& r, QList<double>& cl, QList<QString>&dl, QString *conflictsString)
{
	//if the matrices subgroupsMatrix and teachersMatrix are already calculated, do not calculate them again!
	if(!c.teachersMatrixReady || !c.subgroupsMatrixReady){
		c.teachersMatrixReady=true;
		c.subgroupsMatrixReady=true;
	//if(crt_chrom!=&c || crt_rules!=&r || subgroups_conflicts<0 || teachers_conflicts<0 || c.changedForMatrixCalculation){
		subgroups_conflicts = c.getSubgroupsMatrix(r, subgroupsMatrix);
		teachers_conflicts = c.getTeachersMatrix(r, teachersMatrix);

		//crt_chrom=&c;
		//crt_rules=&r;
		
		c.changedForMatrixCalculation=false;
	}

	int nbroken=0;
	
	for(int t=0; t<r.nInternalTeachers; t++){
		bool ocDay[MAX_DAYS_PER_WEEK];
		for(int d=0; d<r.nDaysPerWeek; d++){
			ocDay[d]=false;
			for(int h=startHour; h<endHour; h++){
				if(teachersMatrix[t][d][h]>0){
					ocDay[d]=true;
				}
			}
		}
		int nOcDays=0;
		for(int d=0; d<r.nDaysPerWeek; d++)
			if(ocDay[d])
				nOcDays++;
		if(nOcDays > this->maxDaysPerWeek){
			nbroken+=nOcDays-this->maxDaysPerWeek;

			if(nOcDays-this->maxDaysPerWeek>0){
				QString s= tr("Time constraint teachers interval max days per week broken for teacher: %1, allowed %2 days, required %3 days.")
				 .arg(r.internalTeachersList[t]->name)
				 .arg(this->maxDaysPerWeek)
				 .arg(nOcDays);
				s+=" ";
				s += tr("This increases the conflicts total by %1")
				 .arg((nOcDays-this->maxDaysPerWeek)*weightPercentage/100);
				 
				dl.append(s);
				cl.append((nOcDays-this->maxDaysPerWeek)*weightPercentage/100);
			
				*conflictsString += s+"\n";
			}
		}
		//cout<<"teacher = "<<qPrintable(r.internalTeachersList[t]->name)<<", nOcDays = "<<nOcDays<<", max days per week = "<<this->maxDaysPerWeek<<endl;
	}

	if(weightPercentage==100)
		assert(nbroken==0);
	return weightPercentage/100 * nbroken;
}

bool ConstraintTeachersIntervalMaxDaysPerWeek::isRelatedToActivity(Rules& r, Activity* a)
{
	Q_UNUSED(r);
	Q_UNUSED(a);
	//if(a)
	//	;

	return false;
}

bool ConstraintTeachersIntervalMaxDaysPerWeek::isRelatedToTeacher(Teacher* t)
{
	Q_UNUSED(t);
	
	return true;
}

bool ConstraintTeachersIntervalMaxDaysPerWeek::isRelatedToSubject(Subject* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintTeachersIntervalMaxDaysPerWeek::isRelatedToActivityTag(ActivityTag* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintTeachersIntervalMaxDaysPerWeek::isRelatedToStudentsSet(Rules& r, StudentsSet* s)
{
	Q_UNUSED(r);
	Q_UNUSED(s);
	/*if(s)
		;
	if(&r)
		;*/

	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

ConstraintStudentsSetIntervalMaxDaysPerWeek::ConstraintStudentsSetIntervalMaxDaysPerWeek()
	: TimeConstraint()
{
	this->type=CONSTRAINT_STUDENTS_SET_INTERVAL_MAX_DAYS_PER_WEEK;
}

ConstraintStudentsSetIntervalMaxDaysPerWeek::ConstraintStudentsSetIntervalMaxDaysPerWeek(double wp, int maxnd, QString sn, int sh, int eh)
	 : TimeConstraint(wp)
{
	this->students = sn;
	this->maxDaysPerWeek=maxnd;
	this->type=CONSTRAINT_STUDENTS_SET_INTERVAL_MAX_DAYS_PER_WEEK;
	this->startHour=sh;
	this->endHour=eh;
	assert(sh<eh);
	assert(sh>=0);
}

bool ConstraintStudentsSetIntervalMaxDaysPerWeek::computeInternalStructure(Rules& r)
{
	if(this->startHour>=this->endHour){
		QMessageBox::warning(NULL, tr("FET warning"),
		 tr("Constraint teacher interval max days per week is wrong because start hour >= end hour."
		 " Please correct it. Constraint is:\n%1").arg(this->getDetailedDescription(r)));

		return false;
	}
	if(this->startHour<0){
		QMessageBox::warning(NULL, tr("FET warning"),
		 tr("Constraint teacher interval max days per week is wrong because start hour < first hour or the day."
		 " Please correct it. Constraint is:\n%1").arg(this->getDetailedDescription(r)));

		return false;
	}
	if(this->endHour>r.nHoursPerDay){
		QMessageBox::warning(NULL, tr("FET warning"),
		 tr("Constraint teacher interval max days per week is wrong because end hour > number of hours per day."
		 " Please correct it. Constraint is:\n%1").arg(this->getDetailedDescription(r)));

		return false;
	}

	/////////
	StudentsSet* ss=r.searchAugmentedStudentsSet(this->students);

	if(ss==NULL){
		QMessageBox::warning(NULL, tr("FET warning"),
		 tr("Constraint students set interval max days per week is wrong because it refers to inexistent students set."
		 " Please correct it (removing it might be a solution). Please report potential bug. Constraint is:\n%1").arg(this->getDetailedDescription(r)));
		 
		return false;
	}	

	assert(ss);

	//this->nSubgroups=0;
	this->iSubgroupsList.clear();
	if(ss->type==STUDENTS_SUBGROUP){
		int tmp;
		/*for(tmp=0; tmp<=r.nInternalSubgroups; tmp++)
			if(r.internalSubgroupsList[tmp]->name == ss->name)
				break;*/
		tmp=((StudentsSubgroup*)ss)->indexInInternalSubgroupsList;
		assert(tmp>=0);
		assert(tmp<r.nInternalSubgroups);
		//assert(this->nSubgroups<MAX_SUBGROUPS_PER_CONSTRAINT);
		//this->subgroups[this->nSubgroups++]=tmp;
		if(!this->iSubgroupsList.contains(tmp))
			this->iSubgroupsList.append(tmp);
	}
	else if(ss->type==STUDENTS_GROUP){
		StudentsGroup* stg=(StudentsGroup*)ss;
		for(int i=0; i<stg->subgroupsList.size(); i++){
			StudentsSubgroup* sts=stg->subgroupsList[i];
			int tmp;
			/*for(tmp=0; tmp<=r.nInternalSubgroups; tmp++)
				if(r.internalSubgroupsList[tmp]->name == sts->name)
					break;*/
			tmp=sts->indexInInternalSubgroupsList;
			assert(tmp>=0);
			assert(tmp<r.nInternalSubgroups);
			//assert(this->nSubgroups<MAX_SUBGROUPS_PER_CONSTRAINT);
			//this->subgroups[this->nSubgroups++]=tmp;
			if(!this->iSubgroupsList.contains(tmp))
				this->iSubgroupsList.append(tmp);
		}
	}
	else if(ss->type==STUDENTS_YEAR){
		StudentsYear* sty=(StudentsYear*)ss;
		for(int i=0; i<sty->groupsList.size(); i++){
			StudentsGroup* stg=sty->groupsList[i];
			for(int j=0; j<stg->subgroupsList.size(); j++){
				StudentsSubgroup* sts=stg->subgroupsList[j];
				int tmp;
				/*for(tmp=0; tmp<=r.nInternalSubgroups; tmp++)
					if(r.internalSubgroupsList[tmp]->name == sts->name)
						break;*/
				tmp=sts->indexInInternalSubgroupsList;
				assert(tmp>=0);
				assert(tmp<r.nInternalSubgroups);
				//assert(this->nSubgroups<MAX_SUBGROUPS_PER_CONSTRAINT);
				//this->subgroups[this->nSubgroups++]=tmp;
				if(!this->iSubgroupsList.contains(tmp))
					this->iSubgroupsList.append(tmp);
			}
		}
	}
	else
		assert(0);
		
	return true;
}

bool ConstraintStudentsSetIntervalMaxDaysPerWeek::hasInactiveActivities(Rules& r)
{
	Q_UNUSED(r);
	return false;
}

QString ConstraintStudentsSetIntervalMaxDaysPerWeek::getXmlDescription(Rules& r)
{
	Q_UNUSED(r);

	QString s="<ConstraintStudentsSetIntervalMaxDaysPerWeek>\n";
	s+="	<Weight_Percentage>"+QString::number(this->weightPercentage)+"</Weight_Percentage>\n";
	//s+="	<Compulsory>";s+=yesNo(this->compulsory);s+="</Compulsory>\n";
	s+="	<Students>"+protect(this->students)+"</Students>\n";
	s+="	<Interval_Start_Hour>"+protect(r.hoursOfTheDay[this->startHour])+"</Interval_Start_Hour>\n";
	if(this->endHour < r.nHoursPerDay){
		s+="	<Interval_End_Hour>"+protect(r.hoursOfTheDay[this->endHour])+"</Interval_End_Hour>\n";
	}
	else{
		s+="	<Interval_End_Hour></Interval_End_Hour>\n";
		s+="	<!-- Interval_End_Hour void means the end of the day (which has no name) -->\n";
	}
	s+="	<Max_Days_Per_Week>"+QString::number(this->maxDaysPerWeek)+"</Max_Days_Per_Week>\n";
	s+="</ConstraintStudentsSetIntervalMaxDaysPerWeek>\n";
	return s;
}

QString ConstraintStudentsSetIntervalMaxDaysPerWeek::getDescription(Rules& r){
	Q_UNUSED(r);

	QString s=tr("Students set interval max days per week");s+=", ";
	s+=tr("WP:%1\%", "Abbreviation for weight percentage").arg(this->weightPercentage);s+=", ";
	s+=tr("St:%1", "Abbreviation for students (sets)").arg(this->students);s+=", ";
	s+=tr("ISH:%1", "Abbreviation for interval start hour").arg(r.hoursOfTheDay[this->startHour]);
	s+=", ";
	if(this->endHour<r.nHoursPerDay)
		s+=tr("IEH:%1", "Abbreviation for interval end hour").arg(r.hoursOfTheDay[this->endHour]);
	else
		s+=tr("IEH:%1", "Abbreviation for interval end hour").arg(tr("End of the day"));
	s+=", ";
	s+=tr("MD:%1", "Abbreviation for max days").arg(this->maxDaysPerWeek);

	return s;
}

QString ConstraintStudentsSetIntervalMaxDaysPerWeek::getDetailedDescription(Rules& r){
	Q_UNUSED(r);

	QString s=tr("Time constraint");s+="\n";
	s+=tr("A students set respects working in an hourly interval a maximum number of days per week");s+="\n";
	s+=tr("Weight (percentage)=%1\%").arg(this->weightPercentage);s+="\n";
	s+=tr("Students set=%1").arg(this->students);s+="\n";
	s+=tr("Interval start hour=%1").arg(r.hoursOfTheDay[this->startHour]);s+="\n";

	if(this->endHour<r.nHoursPerDay)
		s+=tr("Interval end hour=%1").arg(r.hoursOfTheDay[this->endHour]);
	else
		s+=tr("Interval end hour=%1").arg(tr("End of the day"));
	s+="\n";

	s+=tr("Maximum days per week=%1").arg(this->maxDaysPerWeek);s+="\n";

	return s;
}

double ConstraintStudentsSetIntervalMaxDaysPerWeek::fitness(Solution& c, Rules& r, QList<double>& cl, QList<QString>&dl, QString *conflictsString)
{
	//if the matrices subgroupsMatrix and teachersMatrix are already calculated, do not calculate them again!
	if(!c.teachersMatrixReady || !c.subgroupsMatrixReady){
		c.teachersMatrixReady=true;
		c.subgroupsMatrixReady=true;
	//if(crt_chrom!=&c || crt_rules!=&r || subgroups_conflicts<0 || teachers_conflicts<0 || c.changedForMatrixCalculation){
		subgroups_conflicts = c.getSubgroupsMatrix(r, subgroupsMatrix);
		teachers_conflicts = c.getTeachersMatrix(r, teachersMatrix);

		//crt_chrom=&c;
		//crt_rules=&r;
		
		c.changedForMatrixCalculation=false;
	}

	int nbroken;
	
	nbroken=0;
	
	foreach(int sbg, this->iSubgroupsList){
		bool ocDay[MAX_DAYS_PER_WEEK];
		for(int d=0; d<r.nDaysPerWeek; d++){
			ocDay[d]=false;
			for(int h=startHour; h<endHour; h++){
				if(subgroupsMatrix[sbg][d][h]>0){
					ocDay[d]=true;
				}
			}
		}
		int nOcDays=0;
		for(int d=0; d<r.nDaysPerWeek; d++)
			if(ocDay[d])
				nOcDays++;
		if(nOcDays > this->maxDaysPerWeek){
			nbroken+=nOcDays-this->maxDaysPerWeek;

			if((nOcDays-this->maxDaysPerWeek)>0){
				QString s= tr("Time constraint students set interval max days per week broken for subgroup: %1, allowed %2 days, required %3 days.")
				 .arg(r.internalSubgroupsList[sbg]->name)
				 .arg(this->maxDaysPerWeek)
				 .arg(nOcDays);
				s+=" ";
				s += tr("This increases the conflicts total by %1")
				 .arg((nOcDays-this->maxDaysPerWeek)*weightPercentage/100);
			 
				dl.append(s);
				cl.append((nOcDays-this->maxDaysPerWeek)*weightPercentage/100);
		
				*conflictsString += s+"\n";
			}
		}
		//cout<<"subgroup = "<<qPrintable(r.internalSubgroupsList[sbg]->name)<<", nOcDays = "<<nOcDays<<", max days per week = "<<this->maxDaysPerWeek<<endl;
	}

	if(weightPercentage==100)
		assert(nbroken==0);
	return weightPercentage/100 * nbroken;
}

bool ConstraintStudentsSetIntervalMaxDaysPerWeek::isRelatedToActivity(Rules& r, Activity* a)
{
	Q_UNUSED(r);
	Q_UNUSED(a);
	//if(a)
	//	;

	return false;
}

bool ConstraintStudentsSetIntervalMaxDaysPerWeek::isRelatedToTeacher(Teacher* t)
{
	Q_UNUSED(t);
	return false;
}

bool ConstraintStudentsSetIntervalMaxDaysPerWeek::isRelatedToSubject(Subject* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintStudentsSetIntervalMaxDaysPerWeek::isRelatedToActivityTag(ActivityTag* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintStudentsSetIntervalMaxDaysPerWeek::isRelatedToStudentsSet(Rules& r, StudentsSet* s)
{
	return r.setsShareStudents(this->students, s->name);
}

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

ConstraintStudentsIntervalMaxDaysPerWeek::ConstraintStudentsIntervalMaxDaysPerWeek()
	: TimeConstraint()
{
	this->type=CONSTRAINT_STUDENTS_INTERVAL_MAX_DAYS_PER_WEEK;
}

ConstraintStudentsIntervalMaxDaysPerWeek::ConstraintStudentsIntervalMaxDaysPerWeek(double wp, int maxnd, int sh, int eh)
	 : TimeConstraint(wp)
{
	//this->students = sn;
	this->maxDaysPerWeek=maxnd;
	this->type=CONSTRAINT_STUDENTS_INTERVAL_MAX_DAYS_PER_WEEK;
	this->startHour=sh;
	this->endHour=eh;
	assert(sh<eh);
	assert(sh>=0);
}

bool ConstraintStudentsIntervalMaxDaysPerWeek::computeInternalStructure(Rules& r)
{
	if(this->startHour>=this->endHour){
		QMessageBox::warning(NULL, tr("FET warning"),
		 tr("Constraint teacher interval max days per week is wrong because start hour >= end hour."
		 " Please correct it. Constraint is:\n%1").arg(this->getDetailedDescription(r)));

		return false;
	}
	if(this->startHour<0){
		QMessageBox::warning(NULL, tr("FET warning"),
		 tr("Constraint teacher interval max days per week is wrong because start hour < first hour or the day."
		 " Please correct it. Constraint is:\n%1").arg(this->getDetailedDescription(r)));

		return false;
	}
	if(this->endHour>r.nHoursPerDay){
		QMessageBox::warning(NULL, tr("FET warning"),
		 tr("Constraint teacher interval max days per week is wrong because end hour > number of hours per day."
		 " Please correct it. Constraint is:\n%1").arg(this->getDetailedDescription(r)));

		return false;
	}

	return true;
}

bool ConstraintStudentsIntervalMaxDaysPerWeek::hasInactiveActivities(Rules& r)
{
	Q_UNUSED(r);
	return false;
}

QString ConstraintStudentsIntervalMaxDaysPerWeek::getXmlDescription(Rules& r)
{
	Q_UNUSED(r);

	QString s="<ConstraintStudentsIntervalMaxDaysPerWeek>\n";
	s+="	<Weight_Percentage>"+QString::number(this->weightPercentage)+"</Weight_Percentage>\n";
//	s+="	<Students>"+protect(this->students)+"</Students>\n";
	s+="	<Interval_Start_Hour>"+protect(r.hoursOfTheDay[this->startHour])+"</Interval_Start_Hour>\n";
	if(this->endHour < r.nHoursPerDay){
		s+="	<Interval_End_Hour>"+protect(r.hoursOfTheDay[this->endHour])+"</Interval_End_Hour>\n";
	}
	else{
		s+="	<Interval_End_Hour></Interval_End_Hour>\n";
		s+="	<!-- Interval_End_Hour void means the end of the day (which has no name) -->\n";
	}
	s+="	<Max_Days_Per_Week>"+QString::number(this->maxDaysPerWeek)+"</Max_Days_Per_Week>\n";
	s+="</ConstraintStudentsIntervalMaxDaysPerWeek>\n";
	return s;
}

QString ConstraintStudentsIntervalMaxDaysPerWeek::getDescription(Rules& r){
	Q_UNUSED(r);

	QString s=tr("Students interval max days per week");s+=", ";
	s+=tr("WP:%1\%", "Abbreviation for weight percentage").arg(this->weightPercentage);s+=", ";
	s+=tr("ISH:%1", "Abbreviation for interval start hour").arg(r.hoursOfTheDay[this->startHour]);
	s+=", ";
	if(this->endHour<r.nHoursPerDay)
		s+=tr("IEH:%1", "Abbreviation for interval end hour").arg(r.hoursOfTheDay[this->endHour]);
	else
		s+=tr("IEH:%1", "Abbreviation for interval end hour").arg(tr("End of the day"));
	s+=", ";
	s+=tr("MD:%1", "Abbreviation for max days").arg(this->maxDaysPerWeek);

	return s;
}

QString ConstraintStudentsIntervalMaxDaysPerWeek::getDetailedDescription(Rules& r){
	Q_UNUSED(r);

	QString s=tr("Time constraint");s+="\n";
	s+=tr("All students respect working in an hourly interval a maximum number of days per week");s+="\n";
	s+=tr("Weight (percentage)=%1\%").arg(this->weightPercentage);s+="\n";
	s+=tr("Interval start hour=%1").arg(r.hoursOfTheDay[this->startHour]);s+="\n";

	if(this->endHour<r.nHoursPerDay)
		s+=tr("Interval end hour=%1").arg(r.hoursOfTheDay[this->endHour]);
	else
		s+=tr("Interval end hour=%1").arg(tr("End of the day"));
	s+="\n";

	s+=tr("Maximum days per week=%1").arg(this->maxDaysPerWeek);s+="\n";

	return s;
}

double ConstraintStudentsIntervalMaxDaysPerWeek::fitness(Solution& c, Rules& r, QList<double>& cl, QList<QString>&dl, QString *conflictsString)
{
	//if the matrices subgroupsMatrix and teachersMatrix are already calculated, do not calculate them again!
	if(!c.teachersMatrixReady || !c.subgroupsMatrixReady){
		c.teachersMatrixReady=true;
		c.subgroupsMatrixReady=true;
	//if(crt_chrom!=&c || crt_rules!=&r || subgroups_conflicts<0 || teachers_conflicts<0 || c.changedForMatrixCalculation){
		subgroups_conflicts = c.getSubgroupsMatrix(r, subgroupsMatrix);
		teachers_conflicts = c.getTeachersMatrix(r, teachersMatrix);

		//crt_chrom=&c;
		//crt_rules=&r;
		
		c.changedForMatrixCalculation=false;
	}

	int nbroken;
	
	nbroken=0;
	
	for(int sbg=0; sbg<r.nInternalSubgroups; sbg++){
	//foreach(int sbg, this->iSubgroupsList){
		bool ocDay[MAX_DAYS_PER_WEEK];
		for(int d=0; d<r.nDaysPerWeek; d++){
			ocDay[d]=false;
			for(int h=startHour; h<endHour; h++){
				if(subgroupsMatrix[sbg][d][h]>0){
					ocDay[d]=true;
				}
			}
		}
		int nOcDays=0;
		for(int d=0; d<r.nDaysPerWeek; d++)
			if(ocDay[d])
				nOcDays++;
		if(nOcDays > this->maxDaysPerWeek){
			nbroken+=nOcDays-this->maxDaysPerWeek;

			if((nOcDays-this->maxDaysPerWeek)>0){
				QString s= tr("Time constraint students interval max days per week broken for subgroup: %1, allowed %2 days, required %3 days.")
				 .arg(r.internalSubgroupsList[sbg]->name)
				 .arg(this->maxDaysPerWeek)
				 .arg(nOcDays);
				s+=" ";
				s += tr("This increases the conflicts total by %1")
				 .arg((nOcDays-this->maxDaysPerWeek)*weightPercentage/100);
			 
				dl.append(s);
				cl.append((nOcDays-this->maxDaysPerWeek)*weightPercentage/100);
		
				*conflictsString += s+"\n";
			}
		}
		//cout<<"subgroup = "<<qPrintable(r.internalSubgroupsList[sbg]->name)<<", nOcDays = "<<nOcDays<<", max days per week = "<<this->maxDaysPerWeek<<endl;
	}

	if(weightPercentage==100)
		assert(nbroken==0);
	return weightPercentage/100 * nbroken;
}

bool ConstraintStudentsIntervalMaxDaysPerWeek::isRelatedToActivity(Rules& r, Activity* a)
{
	Q_UNUSED(r);
	Q_UNUSED(a);
	//if(a)
	//	;

	return false;
}

bool ConstraintStudentsIntervalMaxDaysPerWeek::isRelatedToTeacher(Teacher* t)
{
	Q_UNUSED(t);
	return false;
}

bool ConstraintStudentsIntervalMaxDaysPerWeek::isRelatedToSubject(Subject* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintStudentsIntervalMaxDaysPerWeek::isRelatedToActivityTag(ActivityTag* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintStudentsIntervalMaxDaysPerWeek::isRelatedToStudentsSet(Rules& r, StudentsSet* s)
{
	Q_UNUSED(r);
	Q_UNUSED(s);
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

ConstraintActivitiesEndStudentsDay::ConstraintActivitiesEndStudentsDay()
	: TimeConstraint()
{
	this->type = CONSTRAINT_ACTIVITIES_END_STUDENTS_DAY;
}

ConstraintActivitiesEndStudentsDay::ConstraintActivitiesEndStudentsDay(double wp, QString te,
	QString st, QString su, QString sut)
	: TimeConstraint(wp)
{
	this->teacherName=te;
	this->subjectName=su;
	this->activityTagName=sut;
	this->studentsName=st;
	this->type=CONSTRAINT_ACTIVITIES_END_STUDENTS_DAY;
}

bool ConstraintActivitiesEndStudentsDay::computeInternalStructure(Rules& r)
{
	//assert(this->teacherName!="" || this->studentsName!="" || this->subjectName!="" || this->subjectTagName!="");

	this->nActivities=0;
	this->activitiesIndices.clear();

	QStringList::iterator it;
	Activity* act;
	int i;
	for(i=0; i<r.nInternalActivities; i++){
		act=&r.internalActivitiesList[i];

		//check if this activity has the corresponding teacher
		if(this->teacherName!=""){
			it = act->teachersNames.find(this->teacherName);
			if(it==act->teachersNames.end())
				continue;
		}
		//check if this activity has the corresponding students
		if(this->studentsName!=""){
			bool commonStudents=false;
			foreach(QString st, act->studentsNames)
				if(r.setsShareStudents(st, studentsName)){
					commonStudents=true;
					break;
				}
		
			/*it = act->studentsNames.find(this->studentsName);
			if(it==act->studentsNames.end())
				continue;*/
			if(!commonStudents)
				continue;
		}
		//check if this activity has the corresponding subject
		if(this->subjectName!="" && act->subjectName!=this->subjectName){
				continue;
		}
		//check if this activity has the corresponding activity tag
		if(this->activityTagName!="" && !act->activityTagsNames.contains(this->activityTagName)){
				continue;
		}
	
		assert(this->nActivities < MAX_ACTIVITIES);	
		//this->activitiesIndices[this->nActivities++]=i;
		this->nActivities++;
		this->activitiesIndices.append(i);
	}
	
	assert(this->activitiesIndices.count()==this->nActivities);

	if(this->nActivities>0)
		return true;
	else{
		QMessageBox::warning(NULL, tr("FET error in data"), 
			tr("Following constraint is wrong (refers to no activities). Please correct it:\n%1").arg(this->getDetailedDescription(r)));
		return false;
	}
}

bool ConstraintActivitiesEndStudentsDay::hasInactiveActivities(Rules& r)
{
	Q_UNUSED(r);
	return false;
}

QString ConstraintActivitiesEndStudentsDay::getXmlDescription(Rules& r)
{
	//to avoid non-used parameter warning
	Q_UNUSED(r);

	QString s="<ConstraintActivitiesEndStudentsDay>\n";
	s+="	<Weight_Percentage>"+QString::number(this->weightPercentage)+"</Weight_Percentage>\n";
	//s+="	<Compulsory>";s+=yesNo(this->compulsory);s+="</Compulsory>\n";
	//if(this->teacherName!="")
		s+="	<Teacher_Name>"+protect(this->teacherName)+"</Teacher_Name>\n";
	//if(this->studentsName!="")
		s+="	<Students_Name>"+protect(this->studentsName)+"</Students_Name>\n";
	//if(this->subjectName!="")
		s+="	<Subject_Name>"+protect(this->subjectName)+"</Subject_Name>\n";
	//if(this->subjectTagName!="")
		s+="	<Activity_Tag_Name>"+protect(this->activityTagName)+"</Activity_Tag_Name>\n";
	s+="</ConstraintActivitiesEndStudentsDay>\n";
	return s;
}

QString ConstraintActivitiesEndStudentsDay::getDescription(Rules& r)
{
	//to avoid non-used parameter warning
	Q_UNUSED(r);
	
	QString tc, st, su, at;
	
	if(this->teacherName!="")
		tc=tr("teacher=%1").arg(this->teacherName);
	else
		tc=tr("all teachers");
		
	if(this->studentsName!="")
		st=tr("students=%1").arg(this->studentsName);
	else
		st=tr("all students");
		
	if(this->subjectName!="")
		su=tr("subject=%1").arg(this->subjectName);
	else
		su=tr("all subjects");
		
	if(this->activityTagName!="")
		at+=tr("activity tag=%1").arg(this->activityTagName);
	else
		at+=tr("all activity tags");
	
	QString s;
	s+=tr("Activities with %1, %2, %3, %4, must end students' day", "%1...%4 are conditions for the activities").arg(tc).arg(st).arg(su).arg(at);

	s+=", ";

	s+=tr("WP:%1\%", "Abbreviation for Weight Percentage").arg(this->weightPercentage);

	return s;
}

QString ConstraintActivitiesEndStudentsDay::getDetailedDescription(Rules& r)
{
	Q_UNUSED(r);

	QString s=tr("Time constraint");s+="\n";
	s+=tr("Activities with:");s+="\n";

	if(this->teacherName!="")
		s+=tr("Teacher=%1").arg(this->teacherName);
	else
		s+=tr("All teachers");
	s+="\n";
		
	if(this->studentsName!="")
		s+=tr("Students=%1").arg(this->studentsName);
	else
		s+=tr("All students");
	s+="\n";
		
	if(this->subjectName!="")
		s+=tr("Subject=%1").arg(this->subjectName);
	else
		s+=tr("All subjects");
	s+="\n";
		
	if(this->activityTagName!="")
		s+=tr("Activity tag=%1").arg(this->activityTagName);
	else
		s+=tr("All activity tags");
	s+="\n";

	s+=tr("must end students' day");
	s+="\n";

	s+=tr("Weight (percentage)=%1").arg(this->weightPercentage);s+="\n";

	return s;
}

double ConstraintActivitiesEndStudentsDay::fitness(Solution& c, Rules& r, QList<double>& cl, QList<QString>&dl, QString* conflictsString)
{
	//if the matrices subgroupsMatrix and teachersMatrix are already calculated, do not calculate them again!
	if(!c.teachersMatrixReady || !c.subgroupsMatrixReady){
		c.teachersMatrixReady=true;
		c.subgroupsMatrixReady=true;
	//if(crt_chrom!=&c || crt_rules!=&r || subgroups_conflicts<0 || teachers_conflicts<0 || c.changedForMatrixCalculation){
		subgroups_conflicts = c.getSubgroupsMatrix(r, subgroupsMatrix);
		teachers_conflicts = c.getTeachersMatrix(r, teachersMatrix);

		//crt_chrom=&c;
		//crt_rules=&r;
		
		c.changedForMatrixCalculation=false;
	}

	int nbroken=0;

	assert(r.internalStructureComputed);

	for(int kk=0; kk<this->nActivities; kk++){
		int tmp=0;
		int ai=this->activitiesIndices[kk];
	
		if(c.times[ai]!=UNALLOCATED_TIME){
			int d=c.times[ai]%r.nDaysPerWeek; //the day when this activity was scheduled
			int h=c.times[ai]/r.nDaysPerWeek; //the hour
		
			for(int j=0; j<r.internalActivitiesList[ai].iSubgroupsList.count(); j++){
				int sb=r.internalActivitiesList[ai].iSubgroupsList.at(j);
				for(int hh=h+r.internalActivitiesList[ai].duration; hh<r.nHoursPerDay; hh++)
					if(subgroupsMatrix[sb][d][hh]>0){
						nbroken++;
						tmp=1;
						break;
					}
				if(tmp>0)
					break;
			}

			if(conflictsString!=NULL && tmp>0){
				QString s=tr("Time constraint activities end students' day broken for activity with id=%1 (%2), increases conflicts total by %3",
				 "%1 is the id, %2 is the detailed description of the activity")
				 .arg(r.internalActivitiesList[ai].id)
				 .arg(getActivityDetailedDescription(r, r.internalActivitiesList[ai].id))
				 .arg(weightPercentage/100*tmp);

				dl.append(s);
				cl.append(weightPercentage/100*tmp);
	
				*conflictsString+= s+"\n";
			}
		}
	}

	if(weightPercentage==100)
		assert(nbroken==0);
	return nbroken * weightPercentage/100;
}

bool ConstraintActivitiesEndStudentsDay::isRelatedToActivity(Rules& r, Activity* a)
{
	QStringList::iterator it;

	//check if this activity has the corresponding teacher
	if(this->teacherName!=""){
		it = a->teachersNames.find(this->teacherName);
		if(it==a->teachersNames.end())
			return false;
	}
	//check if this activity has the corresponding students
	if(this->studentsName!=""){
		bool commonStudents=false;
		foreach(QString st, a->studentsNames){
			if(r.setsShareStudents(st, this->studentsName)){
				commonStudents=true;
				break;
			}
		}
		if(!commonStudents)
			return false;

		//it = a->studentsNames.find(this->studentsName);
		//if(it==a->studentsNames.end())
		//	return false;
	}
	//check if this activity has the corresponding subject
	if(this->subjectName!="" && a->subjectName!=this->subjectName)
		return false;
	//check if this activity has the corresponding activity tag
	if(this->activityTagName!="" && !a->activityTagsNames.contains(this->activityTagName))
		return false;

	return true;
}

bool ConstraintActivitiesEndStudentsDay::isRelatedToTeacher(Teacher* t)
{
	Q_UNUSED(t);
	//if(t)
	//	;

	return false;
}

bool ConstraintActivitiesEndStudentsDay::isRelatedToSubject(Subject* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintActivitiesEndStudentsDay::isRelatedToActivityTag(ActivityTag* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintActivitiesEndStudentsDay::isRelatedToStudentsSet(Rules& r, StudentsSet* s)
{
	Q_UNUSED(r);
	Q_UNUSED(s);
	/*if(s)
		;
	if(&r)
		;*/
		
	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
//new, added on 25 September 2009
ConstraintTeachersActivityTagMaxHoursDaily::ConstraintTeachersActivityTagMaxHoursDaily()
	: TimeConstraint()
{
	this->type=CONSTRAINT_TEACHERS_ACTIVITY_TAG_MAX_HOURS_DAILY;
}

ConstraintTeachersActivityTagMaxHoursDaily::ConstraintTeachersActivityTagMaxHoursDaily(double wp, int maxhours, const QString& activityTag)
 : TimeConstraint(wp)
 {
	assert(maxhours>0);
	this->maxHoursDaily=maxhours;
	this->activityTagName=activityTag;

	this->type=CONSTRAINT_TEACHERS_ACTIVITY_TAG_MAX_HOURS_DAILY;
}

bool ConstraintTeachersActivityTagMaxHoursDaily::computeInternalStructure(Rules& r)
{
	this->activityTagIndex=r.searchActivityTag(this->activityTagName);
	assert(this->activityTagIndex>=0);
	
	this->canonicalTeachersList.clear();
	for(int i=0; i<r.nInternalTeachers; i++){
		bool found=false;
	
		Teacher* tch=r.internalTeachersList[i];
		foreach(int actIndex, tch->activitiesForTeacher){
			if(r.internalActivitiesList[actIndex].iActivityTagsSet.contains(this->activityTagIndex)){
				found=true;
				break;
			}
		}
		
		if(found)
			this->canonicalTeachersList.append(i);
	}

	return true;
}

bool ConstraintTeachersActivityTagMaxHoursDaily::hasInactiveActivities(Rules& r)
{
	Q_UNUSED(r);
	return false;
}

QString ConstraintTeachersActivityTagMaxHoursDaily::getXmlDescription(Rules& r){
	Q_UNUSED(r);

	QString s="<ConstraintTeachersActivityTagMaxHoursDaily>\n";
	s+="	<Weight_Percentage>"+QString::number(this->weightPercentage)+"</Weight_Percentage>\n";
	//s+="	<Compulsory>";s+=yesNo(this->compulsory);s+="</Compulsory>\n";
	s+="	<Activity_Tag_Name>"+protect(this->activityTagName)+"</Activity_Tag_Name>\n";
	s+="	<Maximum_Hours_Daily>"+QString::number(this->maxHoursDaily)+"</Maximum_Hours_Daily>\n";
	s+="</ConstraintTeachersActivityTagMaxHoursDaily>\n";
	return s;
}

QString ConstraintTeachersActivityTagMaxHoursDaily::getDescription(Rules& r){
	Q_UNUSED(r);

	QString s;
	s+="! ";
	s+=tr("Teachers for activity tag %1 have max %2 hours daily").arg(this->activityTagName).arg(this->maxHoursDaily);s+=", ";
	s+=tr("WP:%1\%", "Weight percentage").arg(this->weightPercentage);//s+=", ";

	return s;
}

QString ConstraintTeachersActivityTagMaxHoursDaily::getDetailedDescription(Rules& r){
	Q_UNUSED(r);

	QString s=tr("Time constraint");s+="\n";
	s+=tr("(not perfect)", "It refers to a not perfect constraint"); s+="\n";
	s+=tr("All teachers, for an activity tag, must respect the maximum number of hours daily");s+="\n";
	s+=tr("Weight (percentage)=%1\%").arg(this->weightPercentage);s+="\n";
	s+=tr("Activity tag=%1").arg(this->activityTagName); s+="\n";
	s+=tr("Maximum hours daily=%1").arg(this->maxHoursDaily); s+="\n";

	return s;
}

double ConstraintTeachersActivityTagMaxHoursDaily::fitness(Solution& c, Rules& r, QList<double>& cl, QList<QString>&dl, QString* conflictsString)
{
	//if the matrices subgroupsMatrix and teachersMatrix are already calculated, do not calculate them again!
	if(!c.teachersMatrixReady || !c.subgroupsMatrixReady){
		c.teachersMatrixReady=true;
		c.subgroupsMatrixReady=true;
	//if(crt_chrom!=&c || crt_rules!=&r || subgroups_conflicts<0 || teachers_conflicts<0 || c.changedForMatrixCalculation){
		subgroups_conflicts = c.getSubgroupsMatrix(r, subgroupsMatrix);
		teachers_conflicts = c.getTeachersMatrix(r, teachersMatrix);

		//crt_chrom=&c;
		//crt_rules=&r;
		
		c.changedForMatrixCalculation=false;
	}

	int nbroken;

	nbroken=0;
	foreach(int i, this->canonicalTeachersList){
		Teacher* tch=r.internalTeachersList[i];
		int crtTeacherTimetableActivityTag[MAX_DAYS_PER_WEEK][MAX_HOURS_PER_DAY];
		for(int d=0; d<r.nDaysPerWeek; d++)
			for(int h=0; h<r.nHoursPerDay; h++)
				crtTeacherTimetableActivityTag[d][h]=-1;
				
		foreach(int ai, tch->activitiesForTeacher)if(c.times[ai]!=UNALLOCATED_TIME){
			int d=c.times[ai]%r.nDaysPerWeek;
			int h=c.times[ai]/r.nDaysPerWeek;
			for(int dur=0; dur<r.internalActivitiesList[ai].duration; dur++){
				assert(h+dur<r.nHoursPerDay);
				assert(crtTeacherTimetableActivityTag[d][h+dur]==-1);
				if(r.internalActivitiesList[ai].iActivityTagsSet.contains(this->activityTagIndex))
					crtTeacherTimetableActivityTag[d][h+dur]=this->activityTagIndex;
			}
		}
	
		for(int d=0; d<r.nDaysPerWeek; d++){
			int nd=0;
			for(int h=0; h<r.nHoursPerDay; h++)
				if(crtTeacherTimetableActivityTag[d][h]==this->activityTagIndex)
					nd++;

			if(nd>this->maxHoursDaily){
				nbroken++;

				if(conflictsString!=NULL){
					QString s=(tr("Time constraint teachers activity tag %1 max %2 hours daily broken for teacher %3, on day %4, length=%5.")
					 .arg(this->activityTagName)
					 .arg(QString::number(this->maxHoursDaily))
					 .arg(r.internalTeachersList[i]->name)
					 .arg(r.daysOfTheWeek[d])
					 .arg(nd)
					 )
					 +
					 " "
					 +
					 (tr("This increases the conflicts total by %1").arg(QString::number(weightPercentage/100.0)));
					
					dl.append(s);
					cl.append(weightPercentage/100.0);
				
					*conflictsString+= s+"\n";
				}
			}
		}
	}

	if(weightPercentage==100.0)
		assert(nbroken==0);
	return weightPercentage/100.0 * nbroken;
}

bool ConstraintTeachersActivityTagMaxHoursDaily::isRelatedToActivity(Rules& r, Activity* a)
{
	Q_UNUSED(r);
	Q_UNUSED(a);

	return false;
}

bool ConstraintTeachersActivityTagMaxHoursDaily::isRelatedToTeacher(Teacher* t)
{
	Q_UNUSED(t);

	return true;
}

bool ConstraintTeachersActivityTagMaxHoursDaily::isRelatedToSubject(Subject* s)
{
	Q_UNUSED(s);

	return false;
}

bool ConstraintTeachersActivityTagMaxHoursDaily::isRelatedToActivityTag(ActivityTag* s)
{
	return s->name==this->activityTagName;
}

bool ConstraintTeachersActivityTagMaxHoursDaily::isRelatedToStudentsSet(Rules& r, StudentsSet* s)
{
	Q_UNUSED(r);
	Q_UNUSED(s);

	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
ConstraintTeacherActivityTagMaxHoursDaily::ConstraintTeacherActivityTagMaxHoursDaily()
	: TimeConstraint()
{
	this->type=CONSTRAINT_TEACHER_ACTIVITY_TAG_MAX_HOURS_DAILY;
}

ConstraintTeacherActivityTagMaxHoursDaily::ConstraintTeacherActivityTagMaxHoursDaily(double wp, int maxhours, const QString& teacher, const QString& activityTag)
 : TimeConstraint(wp)
 {
	assert(maxhours>0);
	this->maxHoursDaily=maxhours;
	this->teacherName=teacher;
	this->activityTagName=activityTag;

	this->type=CONSTRAINT_TEACHER_ACTIVITY_TAG_MAX_HOURS_DAILY;
}

bool ConstraintTeacherActivityTagMaxHoursDaily::computeInternalStructure(Rules& r)
{
	this->teacher_ID=r.searchTeacher(this->teacherName);
	assert(this->teacher_ID>=0);

	this->activityTagIndex=r.searchActivityTag(this->activityTagName);
	assert(this->activityTagIndex>=0);

	this->canonicalTeachersList.clear();
	int i=this->teacher_ID;
	bool found=false;
	
	Teacher* tch=r.internalTeachersList[i];
	foreach(int actIndex, tch->activitiesForTeacher){
		if(r.internalActivitiesList[actIndex].iActivityTagsSet.contains(this->activityTagIndex)){
			found=true;
			break;
		}
	}
		
	if(found)
		this->canonicalTeachersList.append(i);

	return true;
}

bool ConstraintTeacherActivityTagMaxHoursDaily::hasInactiveActivities(Rules& r)
{
	Q_UNUSED(r);
	return false;
}

QString ConstraintTeacherActivityTagMaxHoursDaily::getXmlDescription(Rules& r){
	Q_UNUSED(r);

	QString s="<ConstraintTeacherActivityTagMaxHoursDaily>\n";
	s+="	<Weight_Percentage>"+QString::number(this->weightPercentage)+"</Weight_Percentage>\n";
	//s+="	<Compulsory>";s+=yesNo(this->compulsory);s+="</Compulsory>\n";
	s+="	<Teacher_Name>"+protect(this->teacherName)+"</Teacher_Name>\n";
	s+="	<Activity_Tag_Name>"+protect(this->activityTagName)+"</Activity_Tag_Name>\n";
	s+="	<Maximum_Hours_Daily>"+QString::number(this->maxHoursDaily)+"</Maximum_Hours_Daily>\n";
	s+="</ConstraintTeacherActivityTagMaxHoursDaily>\n";
	return s;
}

QString ConstraintTeacherActivityTagMaxHoursDaily::getDescription(Rules& r){
	Q_UNUSED(r);

	QString s;
	s+="! ";
	s+=tr("Teacher %1 for activity tag %2 has max %3 hours daily").arg(this->teacherName).arg(this->activityTagName).arg(this->maxHoursDaily);s+=", ";
	s+=tr("WP:%1\%", "Weight percentage").arg(this->weightPercentage);//s+=", ";

	return s;
}

QString ConstraintTeacherActivityTagMaxHoursDaily::getDetailedDescription(Rules& r){
	Q_UNUSED(r);

	QString s=tr("Time constraint");s+="\n";
	s+=tr("(not perfect)", "It refers to a not perfect constraint"); s+="\n";
	s+=tr("A teacher for an activity tag must respect the maximum number of hours daily");s+="\n";
	s+=tr("Weight (percentage)=%1\%").arg(this->weightPercentage);s+="\n";
	s+=tr("Teacher=%1").arg(this->teacherName);s+="\n";
	s+=tr("Activity tag=%1").arg(this->activityTagName);s+="\n";
	s+=tr("Maximum hours daily=%1").arg(this->maxHoursDaily); s+="\n";

	return s;
}

double ConstraintTeacherActivityTagMaxHoursDaily::fitness(Solution& c, Rules& r, QList<double>& cl, QList<QString>&dl, QString* conflictsString)
{
	//if the matrices subgroupsMatrix and teachersMatrix are already calculated, do not calculate them again!
	if(!c.teachersMatrixReady || !c.subgroupsMatrixReady){
		c.teachersMatrixReady=true;
		c.subgroupsMatrixReady=true;
	//if(crt_chrom!=&c || crt_rules!=&r || subgroups_conflicts<0 || teachers_conflicts<0 || c.changedForMatrixCalculation){
		subgroups_conflicts = c.getSubgroupsMatrix(r, subgroupsMatrix);
		teachers_conflicts = c.getTeachersMatrix(r, teachersMatrix);

		//crt_chrom=&c;
		//crt_rules=&r;
		
		c.changedForMatrixCalculation=false;
	}

	int nbroken;

	nbroken=0;
	foreach(int i, this->canonicalTeachersList){
		Teacher* tch=r.internalTeachersList[i];
		int crtTeacherTimetableActivityTag[MAX_DAYS_PER_WEEK][MAX_HOURS_PER_DAY];
		for(int d=0; d<r.nDaysPerWeek; d++)
			for(int h=0; h<r.nHoursPerDay; h++)
				crtTeacherTimetableActivityTag[d][h]=-1;
				
		foreach(int ai, tch->activitiesForTeacher)if(c.times[ai]!=UNALLOCATED_TIME){
			int d=c.times[ai]%r.nDaysPerWeek;
			int h=c.times[ai]/r.nDaysPerWeek;
			for(int dur=0; dur<r.internalActivitiesList[ai].duration; dur++){
				assert(h+dur<r.nHoursPerDay);
				assert(crtTeacherTimetableActivityTag[d][h+dur]==-1);
				if(r.internalActivitiesList[ai].iActivityTagsSet.contains(this->activityTagIndex))
					crtTeacherTimetableActivityTag[d][h+dur]=this->activityTagIndex;
			}
		}
	
		for(int d=0; d<r.nDaysPerWeek; d++){
			int nd=0;
			for(int h=0; h<r.nHoursPerDay; h++)
				if(crtTeacherTimetableActivityTag[d][h]==this->activityTagIndex)
					nd++;

			if(nd>this->maxHoursDaily){
				nbroken++;

				if(conflictsString!=NULL){
					QString s=(tr("Time constraint teacher activity tag %1 max %2 hours daily broken for teacher %3, on day %4, length=%5.")
					 .arg(this->activityTagName)
					 .arg(QString::number(this->maxHoursDaily))
					 .arg(r.internalTeachersList[i]->name)
					 .arg(r.daysOfTheWeek[d])
					 .arg(nd)
					 )
					 +
					 " "
					 +
					 (tr("This increases the conflicts total by %1").arg(QString::number(weightPercentage/100.0)));
					
					dl.append(s);
					cl.append(weightPercentage/100.0);
				
					*conflictsString+= s+"\n";
				}
			}
		}
	}

	if(weightPercentage==100.0)
		assert(nbroken==0);
	return weightPercentage/100.0 * nbroken;
}

bool ConstraintTeacherActivityTagMaxHoursDaily::isRelatedToActivity(Rules& r, Activity* a)
{
	Q_UNUSED(r);
	Q_UNUSED(a);

	return false;
}

bool ConstraintTeacherActivityTagMaxHoursDaily::isRelatedToTeacher(Teacher* t)
{
	if(this->teacherName==t->name)
		return true;
	return false;
}

bool ConstraintTeacherActivityTagMaxHoursDaily::isRelatedToSubject(Subject* s)
{
	Q_UNUSED(s);

	return false;
}

bool ConstraintTeacherActivityTagMaxHoursDaily::isRelatedToActivityTag(ActivityTag* s)
{
	return this->activityTagName==s->name;
}

bool ConstraintTeacherActivityTagMaxHoursDaily::isRelatedToStudentsSet(Rules& r, StudentsSet* s)
{
	Q_UNUSED(r);
	Q_UNUSED(s);

	return false;
}


////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

ConstraintStudentsActivityTagMaxHoursDaily::ConstraintStudentsActivityTagMaxHoursDaily()
	: TimeConstraint()
{
	this->type = CONSTRAINT_STUDENTS_ACTIVITY_TAG_MAX_HOURS_DAILY;
	this->maxHoursDaily = -1;
}

ConstraintStudentsActivityTagMaxHoursDaily::ConstraintStudentsActivityTagMaxHoursDaily(double wp, int maxnh, const QString& activityTag)
	: TimeConstraint(wp)
{
	this->maxHoursDaily = maxnh;
	this->activityTagName=activityTag;
	this->type = CONSTRAINT_STUDENTS_ACTIVITY_TAG_MAX_HOURS_DAILY;
}

bool ConstraintStudentsActivityTagMaxHoursDaily::computeInternalStructure(Rules& r)
{
	this->activityTagIndex=r.searchActivityTag(this->activityTagName);
	assert(this->activityTagIndex>=0);
	
	this->canonicalSubgroupsList.clear();
	for(int i=0; i<r.nInternalSubgroups; i++){
		bool found=false;
	
		StudentsSubgroup* sbg=r.internalSubgroupsList[i];
		foreach(int actIndex, sbg->activitiesForSubgroup){
			if(r.internalActivitiesList[actIndex].iActivityTagsSet.contains(this->activityTagIndex)){
				found=true;
				break;
			}
		}
		
		if(found)
			this->canonicalSubgroupsList.append(i);
	}

	return true;
}

bool ConstraintStudentsActivityTagMaxHoursDaily::hasInactiveActivities(Rules& r)
{
	Q_UNUSED(r);
	return false;
}

QString ConstraintStudentsActivityTagMaxHoursDaily::getXmlDescription(Rules& r)
{
	//to avoid non-used parameter warning
	Q_UNUSED(r);

	QString s="<ConstraintStudentsActivityTagMaxHoursDaily>\n";
	s+="	<Weight_Percentage>"+QString::number(this->weightPercentage)+"</Weight_Percentage>\n";
	
	s+="	<Activity_Tag>"+protect(this->activityTagName)+"</Activity_Tag>\n";
	
	//s+="	<Compulsory>";s+=yesNo(this->compulsory);s+="</Compulsory>\n";
	if(this->maxHoursDaily>=0)
		s+="	<Maximum_Hours_Daily>"+QString::number(this->maxHoursDaily)+"</Maximum_Hours_Daily>\n";
	else
		assert(0);
	s+="</ConstraintStudentsActivityTagMaxHoursDaily>\n";
	return s;
}

QString ConstraintStudentsActivityTagMaxHoursDaily::getDescription(Rules& r)
{
	//to avoid non-used parameter warning
	Q_UNUSED(r);

	QString s;
	s+="! ";
	s+=tr("Students for activity tag %1 have max %2 hours daily")
		.arg(this->activityTagName).arg(this->maxHoursDaily); s+=", ";
	s+=tr("WP:%1\%", "Weight percentage").arg(this->weightPercentage);//s+=", ";

	return s;
}

QString ConstraintStudentsActivityTagMaxHoursDaily::getDetailedDescription(Rules& r)
{
	//to avoid non-used parameter warning
	Q_UNUSED(r);

	QString s=tr("Time constraint");s+="\n";
	s+=tr("(not perfect)", "It refers to a not perfect constraint"); s+="\n";
	s+=tr("All students, for an activity tag, must respect the maximum number of hours daily"); s+="\n";
	s+=tr("Weight (percentage)=%1\%").arg(this->weightPercentage);s+="\n";
	s+=tr("Activity tag=%1").arg(this->activityTagName);s+="\n";
	s+=tr("Maximum hours daily=%1").arg(this->maxHoursDaily);s+="\n";

	return s;
}

double ConstraintStudentsActivityTagMaxHoursDaily::fitness(Solution& c, Rules& r, QList<double>& cl, QList<QString>&dl, QString* conflictsString)
{
	//if the matrices subgroupsMatrix and teachersMatrix are already calculated, do not calculate them again!
	if(!c.teachersMatrixReady || !c.subgroupsMatrixReady){
		c.teachersMatrixReady=true;
		c.subgroupsMatrixReady=true;
	//if(crt_chrom!=&c || crt_rules!=&r || subgroups_conflicts<0 || teachers_conflicts<0 || c.changedForMatrixCalculation){
		subgroups_conflicts = c.getSubgroupsMatrix(r, subgroupsMatrix);
		teachers_conflicts = c.getTeachersMatrix(r, teachersMatrix);

		//crt_chrom=&c;
		//crt_rules=&r;
		
		c.changedForMatrixCalculation=false;
	}
	
	int nbroken;

	nbroken=0;
	
	foreach(int i, this->canonicalSubgroupsList){
		StudentsSubgroup* sbg=r.internalSubgroupsList[i];
		int crtSubgroupTimetableActivityTag[MAX_DAYS_PER_WEEK][MAX_HOURS_PER_DAY];
		for(int d=0; d<r.nDaysPerWeek; d++)
			for(int h=0; h<r.nHoursPerDay; h++)
				crtSubgroupTimetableActivityTag[d][h]=-1;
		foreach(int ai, sbg->activitiesForSubgroup)if(c.times[ai]!=UNALLOCATED_TIME){
			int d=c.times[ai]%r.nDaysPerWeek;
			int h=c.times[ai]/r.nDaysPerWeek;
			for(int dur=0; dur<r.internalActivitiesList[ai].duration; dur++){
				assert(h+dur<r.nHoursPerDay);
				assert(crtSubgroupTimetableActivityTag[d][h+dur]==-1);
				if(r.internalActivitiesList[ai].iActivityTagsSet.contains(this->activityTagIndex))
					crtSubgroupTimetableActivityTag[d][h+dur]=this->activityTagIndex;
			}
		}

		for(int d=0; d<r.nDaysPerWeek; d++){
			int nd=0;
			for(int h=0; h<r.nHoursPerDay; h++)
				if(crtSubgroupTimetableActivityTag[d][h]==this->activityTagIndex)
					nd++;
				
			if(nd>this->maxHoursDaily){
				nbroken++;

				if(conflictsString!=NULL){
					QString s=(tr(
					 "Time constraint students, activity tag %1, max %2 hours daily, broken for subgroup %3, on day %4, length=%5.")
					 .arg(this->activityTagName)
					 .arg(QString::number(this->maxHoursDaily))
					 .arg(r.internalSubgroupsList[i]->name)
					 .arg(r.daysOfTheWeek[d])
					 .arg(nd)
					 )
					 +
					 " "
					 +
					 (tr("This increases the conflicts total by %1").arg(QString::number(weightPercentage/100.0)));
					
					dl.append(s);
					cl.append(weightPercentage/100);
				
					*conflictsString+= s+"\n";
				}
			}
		}
	}
	
	if(weightPercentage==100.0)
		assert(nbroken==0);
	return weightPercentage/100.0 * nbroken;
}

bool ConstraintStudentsActivityTagMaxHoursDaily::isRelatedToActivity(Rules& r, Activity* a)
{
	Q_UNUSED(r);
	Q_UNUSED(a);
	//if(a)
	//	;

	return false;
}

bool ConstraintStudentsActivityTagMaxHoursDaily::isRelatedToTeacher(Teacher* t)
{
	Q_UNUSED(t);
	//if(t)
	//	;

	return false;
}

bool ConstraintStudentsActivityTagMaxHoursDaily::isRelatedToSubject(Subject* s)
{
	Q_UNUSED(s);
	//if(s)
	//	;

	return false;
}

bool ConstraintStudentsActivityTagMaxHoursDaily::isRelatedToActivityTag(ActivityTag* s)
{
	//Q_UNUSED(s);
	//if(s)
	//	;
	
	return s->name==this->activityTagName;

	//return false;
}

bool ConstraintStudentsActivityTagMaxHoursDaily::isRelatedToStudentsSet(Rules& r, StudentsSet* s)
{
	Q_UNUSED(r);
	Q_UNUSED(s);
	/*if(s)
		;
	if(&r)
		;*/

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

ConstraintStudentsSetActivityTagMaxHoursDaily::ConstraintStudentsSetActivityTagMaxHoursDaily()
	: TimeConstraint()
{
	this->type = CONSTRAINT_STUDENTS_SET_ACTIVITY_TAG_MAX_HOURS_DAILY;
	this->maxHoursDaily = -1;
}

ConstraintStudentsSetActivityTagMaxHoursDaily::ConstraintStudentsSetActivityTagMaxHoursDaily(double wp, int maxnh, const QString& s, const QString& activityTag)
	: TimeConstraint(wp)
{
	this->maxHoursDaily = maxnh;
	this->students = s;
	this->activityTagName=activityTag;
	this->type = CONSTRAINT_STUDENTS_SET_ACTIVITY_TAG_MAX_HOURS_DAILY;
}

bool ConstraintStudentsSetActivityTagMaxHoursDaily::hasInactiveActivities(Rules& r)
{
	Q_UNUSED(r);
	return false;
}

QString ConstraintStudentsSetActivityTagMaxHoursDaily::getXmlDescription(Rules& r)
{
	//to avoid non-used parameter warning
	Q_UNUSED(r);

	QString s="<ConstraintStudentsSetActivityTagMaxHoursDaily>\n";
	s+="	<Weight_Percentage>"+QString::number(this->weightPercentage)+"</Weight_Percentage>\n";
	//s+="	<Compulsory>";s+=yesNo(this->compulsory);s+="</Compulsory>\n";
	s+="	<Maximum_Hours_Daily>"+QString::number(this->maxHoursDaily)+"</Maximum_Hours_Daily>\n";
	//s+="	<MinHoursDaily>"+QString::number(this->minHoursDaily)+"</MinHoursDaily>\n";
	s+="	<Students>"+protect(this->students)+"</Students>\n";
	s+="	<Activity_Tag>"+protect(this->activityTagName)+"</Activity_Tag>\n";
	s+="</ConstraintStudentsSetActivityTagMaxHoursDaily>\n";
	return s;
}

QString ConstraintStudentsSetActivityTagMaxHoursDaily::getDescription(Rules& r)
{
	//to avoid non-used parameter warning
	Q_UNUSED(r);

	QString s;
	s+="! ";
	s+=tr("Students set %1 for activity tag %2 has max %3 hours daily").arg(this->students).arg(this->activityTagName).arg(this->maxHoursDaily);
	s+=", ";
	s+=tr("WP:%1\%", "Weight percentage").arg(this->weightPercentage);//s+=", ";

	return s;
}

QString ConstraintStudentsSetActivityTagMaxHoursDaily::getDetailedDescription(Rules& r)
{
	//to avoid non-used parameter warning
	Q_UNUSED(r);

	QString s=tr("Time constraint");s+="\n";
	s+=tr("(not perfect)", "It refers to a not perfect constraint"); s+="\n";
	s+=tr("A students set, for an activity tag, must respect the maximum number of hours daily"); s+="\n";
	s+=tr("Weight (percentage)=%1\%").arg(this->weightPercentage);s+="\n";
	s+=tr("Students set=%1").arg(this->students);s+="\n";
	s+=tr("Activity tag=%1").arg(this->activityTagName);s+="\n";
	s+=tr("Maximum hours daily=%1").arg(this->maxHoursDaily);s+="\n";

	return s;
}

bool ConstraintStudentsSetActivityTagMaxHoursDaily::computeInternalStructure(Rules &r)
{
	this->activityTagIndex=r.searchActivityTag(this->activityTagName);
	assert(this->activityTagIndex>=0);

	StudentsSet* ss=r.searchAugmentedStudentsSet(this->students);
	
	if(ss==NULL){
		QMessageBox::warning(NULL, tr("FET warning"),
		 tr("Constraint students set max hours daily is wrong because it refers to inexistent students set."
		 " Please correct it (removing it might be a solution). Please report potential bug. Constraint is:\n%1").arg(this->getDetailedDescription(r)));
		 
		return false;
	}	

	assert(ss);

	//this->nSubgroups=0;
	this->iSubgroupsList.clear();
	if(ss->type==STUDENTS_SUBGROUP){
		int tmp;
		/*for(tmp=0; tmp<=r.nInternalSubgroups; tmp++)
			if(r.internalSubgroupsList[tmp]->name == ss->name)
				break;*/
		tmp=((StudentsSubgroup*)ss)->indexInInternalSubgroupsList;
		assert(tmp>=0);
		assert(tmp<r.nInternalSubgroups);
		//assert(this->nSubgroups<MAX_SUBGROUPS_PER_CONSTRAINT);
		//this->subgroups[this->nSubgroups++]=tmp;
		if(!this->iSubgroupsList.contains(tmp))
			this->iSubgroupsList.append(tmp);
	}
	else if(ss->type==STUDENTS_GROUP){
		StudentsGroup* stg=(StudentsGroup*)ss;
		for(int i=0; i<stg->subgroupsList.size(); i++){
			StudentsSubgroup* sts=stg->subgroupsList[i];
			int tmp;
			/*for(tmp=0; tmp<=r.nInternalSubgroups; tmp++)
				if(r.internalSubgroupsList[tmp]->name == sts->name)
					break;*/
			tmp=sts->indexInInternalSubgroupsList;
			assert(tmp>=0);
			assert(tmp<r.nInternalSubgroups);
			//assert(this->nSubgroups<MAX_SUBGROUPS_PER_CONSTRAINT);
			//this->subgroups[this->nSubgroups++]=tmp;
			if(!this->iSubgroupsList.contains(tmp))
				this->iSubgroupsList.append(tmp);
		}
	}
	else if(ss->type==STUDENTS_YEAR){
		StudentsYear* sty=(StudentsYear*)ss;
		for(int i=0; i<sty->groupsList.size(); i++){
			StudentsGroup* stg=sty->groupsList[i];
			for(int j=0; j<stg->subgroupsList.size(); j++){
				StudentsSubgroup* sts=stg->subgroupsList[j];
				int tmp;
				/*for(tmp=0; tmp<=r.nInternalSubgroups; tmp++)
					if(r.internalSubgroupsList[tmp]->name == sts->name)
						break;*/
				tmp=sts->indexInInternalSubgroupsList;
				assert(tmp>=0);
				assert(tmp<r.nInternalSubgroups);
				//assert(this->nSubgroups<MAX_SUBGROUPS_PER_CONSTRAINT);
				//this->subgroups[this->nSubgroups++]=tmp;
				if(!this->iSubgroupsList.contains(tmp))
					this->iSubgroupsList.append(tmp);
			}
		}
	}
	else
		assert(0);
		
	/////////////
	this->canonicalSubgroupsList.clear();
	foreach(int i, this->iSubgroupsList){
		bool found=false;
	
/*		StudentsSubgroup* sbg=r.internalSubgroupsList[i];
		foreach(int actIndex, sbg->activitiesForSubgroup){
			int actTagIndex=r.internalActivitiesList[actIndex].activityTagIndex;
			if(actTagIndex==this->activityTagIndex){
				found=true;
				break;
			}
		}*/
		
		StudentsSubgroup* sbg=r.internalSubgroupsList[i];
		foreach(int actIndex, sbg->activitiesForSubgroup){
			if(r.internalActivitiesList[actIndex].iActivityTagsSet.contains(this->activityTagIndex)){
				found=true;
				break;
			}
		}
		
		if(found)
			this->canonicalSubgroupsList.append(i);
	}

		
	return true;
}

double ConstraintStudentsSetActivityTagMaxHoursDaily::fitness(Solution& c, Rules& r, QList<double>& cl, QList<QString>&dl, QString* conflictsString)
{
	//if the matrices subgroupsMatrix and teachersMatrix are already calculated, do not calculate them again!
	if(!c.teachersMatrixReady || !c.subgroupsMatrixReady){
		c.teachersMatrixReady=true;
		c.subgroupsMatrixReady=true;
	//if(crt_chrom!=&c || crt_rules!=&r || subgroups_conflicts<0 || teachers_conflicts<0 || c.changedForMatrixCalculation){
		subgroups_conflicts = c.getSubgroupsMatrix(r, subgroupsMatrix);
		teachers_conflicts = c.getTeachersMatrix(r, teachersMatrix);

		//crt_chrom=&c;
		//crt_rules=&r;
		
		c.changedForMatrixCalculation=false;
	}

	int nbroken;

	nbroken=0;
	
	foreach(int i, this->canonicalSubgroupsList){
		StudentsSubgroup* sbg=r.internalSubgroupsList[i];
		int crtSubgroupTimetableActivityTag[MAX_DAYS_PER_WEEK][MAX_HOURS_PER_DAY];
		for(int d=0; d<r.nDaysPerWeek; d++)
			for(int h=0; h<r.nHoursPerDay; h++)
				crtSubgroupTimetableActivityTag[d][h]=-1;
		foreach(int ai, sbg->activitiesForSubgroup)if(c.times[ai]!=UNALLOCATED_TIME){
			int d=c.times[ai]%r.nDaysPerWeek;
			int h=c.times[ai]/r.nDaysPerWeek;
			for(int dur=0; dur<r.internalActivitiesList[ai].duration; dur++){
				assert(h+dur<r.nHoursPerDay);
				assert(crtSubgroupTimetableActivityTag[d][h+dur]==-1);
				if(r.internalActivitiesList[ai].iActivityTagsSet.contains(this->activityTagIndex))
					crtSubgroupTimetableActivityTag[d][h+dur]=this->activityTagIndex;
			}
		}

		for(int d=0; d<r.nDaysPerWeek; d++){
			int nd=0;
			for(int h=0; h<r.nHoursPerDay; h++)
				if(crtSubgroupTimetableActivityTag[d][h]==this->activityTagIndex)
					nd++;
				
			if(nd>this->maxHoursDaily){
				nbroken++;

				if(conflictsString!=NULL){
					QString s=(tr(
					 "Time constraint students set, activity tag %1, max %2 hours daily, broken for subgroup %3, on day %4, length=%5.")
					 .arg(this->activityTagName)
					 .arg(QString::number(this->maxHoursDaily))
					 .arg(r.internalSubgroupsList[i]->name)
					 .arg(r.daysOfTheWeek[d])
					 .arg(nd)
					 )
					 +
					 " "
					 +
					 (tr("This increases the conflicts total by %1").arg(QString::number(weightPercentage/100.0)));
					
					dl.append(s);
					cl.append(weightPercentage/100);
				
					*conflictsString+= s+"\n";
				}
			}
		}
	}
	
	if(weightPercentage==100.0)
		assert(nbroken==0);
	return weightPercentage/100.0 * nbroken;
}

bool ConstraintStudentsSetActivityTagMaxHoursDaily::isRelatedToActivity(Rules& r, Activity* a)
{
	Q_UNUSED(r);
	Q_UNUSED(a);

	return false;
}

bool ConstraintStudentsSetActivityTagMaxHoursDaily::isRelatedToTeacher(Teacher* t)
{
	Q_UNUSED(t);

	return false;
}

bool ConstraintStudentsSetActivityTagMaxHoursDaily::isRelatedToSubject(Subject* s)
{
	Q_UNUSED(s);

	return false;
}

bool ConstraintStudentsSetActivityTagMaxHoursDaily::isRelatedToActivityTag(ActivityTag* s)
{
	Q_UNUSED(s);

	return false;
}

bool ConstraintStudentsSetActivityTagMaxHoursDaily::isRelatedToStudentsSet(Rules& r, StudentsSet* s)
{
	return r.setsShareStudents(this->students, s->name);
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

ConstraintStudentsMaxGapsPerDay::ConstraintStudentsMaxGapsPerDay()
	: TimeConstraint()
{
	this->type = CONSTRAINT_STUDENTS_MAX_GAPS_PER_DAY;
}

ConstraintStudentsMaxGapsPerDay::ConstraintStudentsMaxGapsPerDay(double wp, int mg)
	: TimeConstraint(wp)
{
	this->type = CONSTRAINT_STUDENTS_MAX_GAPS_PER_DAY;
	this->maxGaps=mg;
}

bool ConstraintStudentsMaxGapsPerDay::computeInternalStructure(Rules& r)
{
	Q_UNUSED(r);
	//if(&r!=NULL)
	//	;

	/*do nothing*/
	return true;
}

bool ConstraintStudentsMaxGapsPerDay::hasInactiveActivities(Rules& r)
{
	Q_UNUSED(r);
	return false;
}

QString ConstraintStudentsMaxGapsPerDay::getXmlDescription(Rules& r)
{
	//to avoid non-used parameter warning
	Q_UNUSED(r);
	//if(&r==NULL)
	//	;

	QString s="<ConstraintStudentsMaxGapsPerDay>\n";
	s+="	<Weight_Percentage>"+QString::number(this->weightPercentage)+"</Weight_Percentage>\n";
	s+="	<Max_Gaps>"+QString::number(this->maxGaps)+"</Max_Gaps>\n";
	s+="</ConstraintStudentsMaxGapsPerDay>\n";
	return s;
}

QString ConstraintStudentsMaxGapsPerDay::getDescription(Rules& r)
{
	//to avoid non-used parameter warning
	Q_UNUSED(r);

	QString s;
	s+="! ";
	s+=tr("Students max gaps per day");s+=", ";
	s+=tr("WP:%1\%", "Weight percentage").arg(this->weightPercentage);s+=", ";
	s+=tr("MG:%1", "Max gaps (per day)").arg(this->maxGaps);

	return s;
}

QString ConstraintStudentsMaxGapsPerDay::getDetailedDescription(Rules& r)
{
	//to avoid non-used parameter warning
	Q_UNUSED(r);

	QString s=tr("Time constraint");s+="\n";
	s+=tr("(not perfect)", "It refers to a not perfect constraint"); s+="\n";
	s+=tr("All students must respect the maximum number of gaps per day");s+="\n";
	s+=tr("(breaks and students set not available not counted)");s+="\n";
	s+=tr("Weight (percentage)=%1\%").arg(this->weightPercentage);s+="\n";
	s+=tr("Maximum gaps per day=%1").arg(this->maxGaps);s+="\n";
	s+="\n";

	return s;
}

double ConstraintStudentsMaxGapsPerDay::fitness(Solution& c, Rules& r, QList<double>& cl, QList<QString>&dl, QString* conflictsString)
{
	//returns a number equal to the number of windows of the subgroups (in hours)

	//if the matrices subgroupsMatrix and teachersMatrix are already calculated, do not calculate them again!
	if(!c.teachersMatrixReady || !c.subgroupsMatrixReady){
		c.teachersMatrixReady=true;
		c.subgroupsMatrixReady=true;
	//if(crt_chrom!=&c || crt_rules!=&r || subgroups_conflicts<0 || teachers_conflicts<0 || c.changedForMatrixCalculation){
		subgroups_conflicts = c.getSubgroupsMatrix(r, subgroupsMatrix);
		teachers_conflicts = c.getTeachersMatrix(r, teachersMatrix);

		//crt_chrom=&c;
		//crt_rules=&r;
		
		c.changedForMatrixCalculation=false;
	}
	
	int windows;
	int tmp;
	int i;
	
	int tIllegalWindows=0;

	for(i=0; i<r.nInternalSubgroups; i++){
		for(int j=0; j<r.nDaysPerWeek; j++){
			windows=0;
	
			int k;
			tmp=0;
			for(k=0; k<r.nHoursPerDay; k++)
				if(subgroupsMatrix[i][j][k]>0){
					assert(!breakDayHour[j][k] && !subgroupNotAvailableDayHour[i][j][k]);
					break;
				}
			for(; k<r.nHoursPerDay; k++) if(!breakDayHour[j][k] && !subgroupNotAvailableDayHour[i][j][k]){
				if(subgroupsMatrix[i][j][k]>0){
					windows+=tmp;
					tmp=0;
				}
				else
					tmp++;
			}
		
			int illegalWindows=windows-this->maxGaps;
			if(illegalWindows<0)
				illegalWindows=0;

			if(illegalWindows>0 && conflictsString!=NULL){
				QString s=tr("Time constraint students max gaps per day broken for subgroup: %1, it has %2 extra gaps, on day %3, conflicts increase=%4")
				 .arg(r.internalSubgroupsList[i]->name)
				 .arg(illegalWindows)
				 .arg(r.daysOfTheWeek[j])
				 .arg(illegalWindows*weightPercentage/100);
							 
				dl.append(s);
				cl.append(illegalWindows*weightPercentage/100);
					
				*conflictsString+= s+"\n";
			}
		
			tIllegalWindows+=illegalWindows;
		}
	}
		
	if(c.nPlacedActivities==r.nInternalActivities)
		if(weightPercentage==100)    //for partial solutions it might be broken
			assert(tIllegalWindows==0);
	return weightPercentage/100 * tIllegalWindows;
}

bool ConstraintStudentsMaxGapsPerDay::isRelatedToActivity(Rules& r, Activity* a)
{
	Q_UNUSED(r);
	Q_UNUSED(a);

	return false;
}

bool ConstraintStudentsMaxGapsPerDay::isRelatedToTeacher(Teacher* t)
{
	Q_UNUSED(t);

	return false;
}

bool ConstraintStudentsMaxGapsPerDay::isRelatedToSubject(Subject* s)
{
	Q_UNUSED(s);

	return false;
}

bool ConstraintStudentsMaxGapsPerDay::isRelatedToActivityTag(ActivityTag* s)
{
	Q_UNUSED(s);

	return false;
}

bool ConstraintStudentsMaxGapsPerDay::isRelatedToStudentsSet(Rules& r, StudentsSet* s)
{
	Q_UNUSED(r);
	Q_UNUSED(s);

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

ConstraintStudentsSetMaxGapsPerDay::ConstraintStudentsSetMaxGapsPerDay()
	: TimeConstraint()
{
	this->type = CONSTRAINT_STUDENTS_SET_MAX_GAPS_PER_DAY;
}

ConstraintStudentsSetMaxGapsPerDay::ConstraintStudentsSetMaxGapsPerDay(double wp, int mg, const QString& st )
	: TimeConstraint(wp)
{
	this->type = CONSTRAINT_STUDENTS_SET_MAX_GAPS_PER_DAY;
	this->maxGaps=mg;
	this->students = st;
}

bool ConstraintStudentsSetMaxGapsPerDay::computeInternalStructure(Rules& r){
	StudentsSet* ss=r.searchAugmentedStudentsSet(this->students);

	if(ss==NULL){
		QMessageBox::warning(NULL, tr("FET warning"),
		 tr("Constraint students set max gaps per day is wrong because it refers to inexistent students set."
		 " Please correct it (removing it might be a solution). Please report potential bug. Constraint is:\n%1").arg(this->getDetailedDescription(r)));
		 
		return false;
	}	

	assert(ss);

	//this->nSubgroups=0;
	this->iSubgroupsList.clear();
	if(ss->type==STUDENTS_SUBGROUP){
		int tmp;
		/*for(tmp=0; tmp<=r.nInternalSubgroups; tmp++)
			if(r.internalSubgroupsList[tmp]->name == ss->name)
				break;*/
		tmp=((StudentsSubgroup*)ss)->indexInInternalSubgroupsList;
		assert(tmp>=0);
		assert(tmp<r.nInternalSubgroups);
		//assert(this->nSubgroups<MAX_SUBGROUPS_PER_CONSTRAINT);
		//this->subgroups[this->nSubgroups++]=tmp;
		if(!this->iSubgroupsList.contains(tmp))
			this->iSubgroupsList.append(tmp);
	}
	else if(ss->type==STUDENTS_GROUP){
		StudentsGroup* stg=(StudentsGroup*)ss;
		for(int i=0; i<stg->subgroupsList.size(); i++){
			StudentsSubgroup* sts=stg->subgroupsList[i];
			int tmp;
			/*for(tmp=0; tmp<=r.nInternalSubgroups; tmp++)
				if(r.internalSubgroupsList[tmp]->name == sts->name)
					break;*/
			tmp=sts->indexInInternalSubgroupsList;
			assert(tmp>=0);
			assert(tmp<r.nInternalSubgroups);
			//assert(this->nSubgroups<MAX_SUBGROUPS_PER_CONSTRAINT);
			//this->subgroups[this->nSubgroups++]=tmp;
			if(!this->iSubgroupsList.contains(tmp))
				this->iSubgroupsList.append(tmp);
		}
	}
	else if(ss->type==STUDENTS_YEAR){
		StudentsYear* sty=(StudentsYear*)ss;
		for(int i=0; i<sty->groupsList.size(); i++){
			StudentsGroup* stg=sty->groupsList[i];
			for(int j=0; j<stg->subgroupsList.size(); j++){
				StudentsSubgroup* sts=stg->subgroupsList[j];
				int tmp;
				/*for(tmp=0; tmp<=r.nInternalSubgroups; tmp++)
					if(r.internalSubgroupsList[tmp]->name == sts->name)
						break;*/
				tmp=sts->indexInInternalSubgroupsList;
				assert(tmp>=0);
				assert(tmp<r.nInternalSubgroups);
				//assert(this->nSubgroups<MAX_SUBGROUPS_PER_CONSTRAINT);
				//this->subgroups[this->nSubgroups++]=tmp;
				if(!this->iSubgroupsList.contains(tmp))
					this->iSubgroupsList.append(tmp);
			}
		}
	}
	else
		assert(0);
		
	return true;
}

bool ConstraintStudentsSetMaxGapsPerDay::hasInactiveActivities(Rules& r)
{
	Q_UNUSED(r);
	return false;
}

QString ConstraintStudentsSetMaxGapsPerDay::getXmlDescription(Rules& r){
	//to avoid non-used parameter warning
	Q_UNUSED(r);

	QString s="<ConstraintStudentsSetMaxGapsPerDay>\n";
	s+="	<Weight_Percentage>"+QString::number(this->weightPercentage)+"</Weight_Percentage>\n";
	s+="	<Max_Gaps>"+QString::number(this->maxGaps)+"</Max_Gaps>\n";
	s+="	<Students>"; s+=protect(this->students); s+="</Students>\n";
	s+="</ConstraintStudentsSetMaxGapsPerDay>\n";
	return s;
}

QString ConstraintStudentsSetMaxGapsPerDay::getDescription(Rules& r){
	//to avoid non-used parameter warning
	Q_UNUSED(r);

	QString s;
	s+="! ";
	s+=tr("Students set max gaps per day"); s+=", ";
	s+=tr("WP:%1\%", "Weight percentage").arg(this->weightPercentage); s+=", ";
	s+=tr("MG:%1", "Max gaps (per day)").arg(this->maxGaps);s+=", ";
	s+=tr("St:%1", "Students").arg(this->students);

	return s;
}

QString ConstraintStudentsSetMaxGapsPerDay::getDetailedDescription(Rules& r){
	//to avoid non-used parameter warning
	Q_UNUSED(r);

	QString s=tr("Time constraint");s+="\n";
	s+=tr("(not perfect)", "It refers to a not perfect constraint"); s+="\n";
	s+=tr("A students set must respect the maximum number of gaps per day");s+="\n";
	s+=tr("(breaks and students set not available not counted)");s+="\n";
	s+=tr("Weight (percentage)=%1").arg(this->weightPercentage);s+="\n";
	s+=tr("Maximum gaps per day=%1").arg(this->maxGaps);s+="\n";
	s+=tr("Students=%1").arg(this->students); s+="\n";
	
	return s;
}

double ConstraintStudentsSetMaxGapsPerDay::fitness(Solution& c, Rules& r, QList<double>& cl, QList<QString>&dl, QString* conflictsString)
{
	//OLD COMMENT
	//returns a number equal to the number of windows of the subgroups (in hours)

	//if the matrices subgroupsMatrix and teachersMatrix are already calculated, do not calculate them again!
	if(!c.teachersMatrixReady || !c.subgroupsMatrixReady){
		c.teachersMatrixReady=true;
		c.subgroupsMatrixReady=true;
	//if(crt_chrom!=&c || crt_rules!=&r || subgroups_conflicts<0 || teachers_conflicts<0 || c.changedForMatrixCalculation){
		subgroups_conflicts = c.getSubgroupsMatrix(r, subgroupsMatrix);
		teachers_conflicts = c.getTeachersMatrix(r, teachersMatrix);

		//crt_chrom=&c;
		//crt_rules=&r;
		
		c.changedForMatrixCalculation=false;
	}
	
	int windows;
	int tmp;
	
	int tIllegalWindows=0;
	
	for(int sg=0; sg<this->iSubgroupsList.count(); sg++){
		int i=this->iSubgroupsList.at(sg);
		for(int j=0; j<r.nDaysPerWeek; j++){
			windows=0;
	
			int k;
			tmp=0;
			for(k=0; k<r.nHoursPerDay; k++)
				if(subgroupsMatrix[i][j][k]>0){
					assert(!breakDayHour[j][k] && !subgroupNotAvailableDayHour[i][j][k]);
					break;
				}
			for(; k<r.nHoursPerDay; k++) if(!breakDayHour[j][k] && !subgroupNotAvailableDayHour[i][j][k]){
				if(subgroupsMatrix[i][j][k]>0){
					windows+=tmp;
					tmp=0;
				}
				else
					tmp++;
			}
		
			int illegalWindows=windows-this->maxGaps;
			if(illegalWindows<0)
				illegalWindows=0;

			if(illegalWindows>0 && conflictsString!=NULL){
				QString s=tr("Time constraint students set max gaps per day broken for subgroup: %1, extra gaps=%2, on day %3, conflicts increase=%4")
				 .arg(r.internalSubgroupsList[i]->name)
				 .arg(illegalWindows)
				 .arg(r.daysOfTheWeek[j])
				 .arg(weightPercentage/100*illegalWindows);
							 
				dl.append(s);
				cl.append(weightPercentage/100*illegalWindows);
					
				*conflictsString+= s+"\n";
			}
		
			tIllegalWindows+=illegalWindows;
		}
	}

	if(c.nPlacedActivities==r.nInternalActivities)
		if(weightPercentage==100)     //for partial solutions it might be broken
			assert(tIllegalWindows==0);
	return weightPercentage/100 * tIllegalWindows;
}

bool ConstraintStudentsSetMaxGapsPerDay::isRelatedToActivity(Rules& r, Activity* a)
{
	Q_UNUSED(r);
	Q_UNUSED(a);

	return false;
}

bool ConstraintStudentsSetMaxGapsPerDay::isRelatedToTeacher(Teacher* t)
{
	Q_UNUSED(t);

	return false;
}

bool ConstraintStudentsSetMaxGapsPerDay::isRelatedToSubject(Subject* s)
{
	Q_UNUSED(s);

	return false;
}

bool ConstraintStudentsSetMaxGapsPerDay::isRelatedToActivityTag(ActivityTag* s)
{
	Q_UNUSED(s);

	return false;
}

bool ConstraintStudentsSetMaxGapsPerDay::isRelatedToStudentsSet(Rules& r, StudentsSet* s)
{
	return r.setsShareStudents(this->students, s->name);
}


///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
