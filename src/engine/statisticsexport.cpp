/*
File statisticsexport.cpp
*/

/***************************************************************************
                                FET
                          -------------------
   copyright            : (C) by Lalescu Liviu
    email                : Please see http://lalescu.ro/liviu/ for details about contacting Liviu Lalescu (in particular, you can find here the e-mail address)
 ***************************************************************************
                      statisticsexport.cpp  -  description
                             -------------------
    begin                : Sep 2008
    copyright            : (C) by Volker Dirr
                         : http://www.timetabling.de/
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

// Code contributed by Volker Dirr ( http://www.timetabling.de/ )
// Many thanks to Liviu Lalescu. He told me some speed optimizations.

#include "timetable_defs.h"		//needed, because of QString s2=INPUT_FILENAME_XML.right(INPUT_FILENAME_XML.length()-INPUT_FILENAME_XML.findRev(FILE_SEP)-1);
#include "statisticsexport.h"

// BE CAREFUL: DON'T USE INTERNAL VARIABLES HERE, because maybe computeInternalStructure() is not done!

#include <QString>
#include <QStringList>
#include <QHash>
#include <QMultiHash>
#include <QMap>

#include <QMessageBox>

#include <QLocale>
#include <QTime>
#include <QDate>

#include <QFile>
#include <QTextStream>

//#include <QApplication>
#include <QProgressDialog>
//extern QApplication* pqapplication;

extern Timetable gt;


QHash<QString, QString> hashSubjectIDsStatistics;
QHash<QString, QString> hashActivityTagIDsStatistics;
QHash<QString, QString> hashStudentIDsStatistics;
QHash<QString, QString> hashTeacherIDsStatistics;
QHash<QString, QString> hashRoomIDsStatistics;
QHash<QString, QString> hashDayIDsStatistics;


//extern bool simulation_running;	//needed?

QHash <QString, int> teachersTotalNumberOfHours;
QHash <QString, int> teachersTotalNumberOfHours2;
QHash <QString, int> studentsTotalNumberOfHours;
QHash <QString, int> studentsTotalNumberOfHours2;
QHash <QString, int> subjectsTotalNumberOfHours;
QHash <QString, int> subjectsTotalNumberOfHours4;
QStringList allStudentsNames;				//NOT QSet <QString>!!! Because that do an incorrect order of the lists!
QStringList allTeachersNames;				//NOT QSet <QString>!!! Because that do an incorrect order of the lists!
QStringList allSubjectsNames;				//NOT QSet <QString>!!! Because that do an incorrect order of the lists!
QMultiHash <QString, int> studentsActivities;
QMultiHash <QString, int> teachersActivities;
QMultiHash <QString, int> subjectsActivities;

//TODO: use the external string!!!
//extern const QString STRING_EMPTY_SLOT;
const QString STRING_EMPTY_SLOT_STATISTICS="---";

const char TEACHERS_STUDENTS_STATISTICS[]="teachers_students.html";
const char TEACHERS_SUBJECTS_STATISTICS[]="teachers_subjects.html";
const char STUDENTS_TEACHERS_STATISTICS[]="students_teachers.html";
const char STUDENTS_SUBJECTS_STATISTICS[]="students_subjects.html";
const char SUBJECTS_TEACHERS_STATISTICS[]="subjects_teachers.html";
const char SUBJECTS_STUDENTS_STATISTICS[]="subjects_students.html";
const char STYLESHEET_STATISTICS[]="stylesheet.css";
const char INDEX_STATISTICS[]="index.html";

QString DIRECTORY_STATISTICS;
QString PREFIX_STATISTICS;

#include <QDir>

#include <iostream>
using namespace std;

StatisticsExport::StatisticsExport()
{
}

StatisticsExport::~StatisticsExport()
{
}

void StatisticsExport::exportStatistics(){
	assert(gt.rules.initialized);
	assert(TIMETABLE_HTML_LEVEL>=0);
	assert(TIMETABLE_HTML_LEVEL<=6);

	computeHashForIDsStatistics();

	DIRECTORY_STATISTICS=OUTPUT_DIR+FILE_SEP+"statistics";
	
	if(INPUT_FILENAME_XML=="")
		DIRECTORY_STATISTICS.append(FILE_SEP+"unnamed");
	else{
		DIRECTORY_STATISTICS.append(FILE_SEP+INPUT_FILENAME_XML.right(INPUT_FILENAME_XML.length()-INPUT_FILENAME_XML.findRev(FILE_SEP)-1));

		if(DIRECTORY_STATISTICS.right(4)==".fet")
			DIRECTORY_STATISTICS=DIRECTORY_STATISTICS.left(DIRECTORY_STATISTICS.length()-4);
		else if(INPUT_FILENAME_XML!="")
			cout<<"Minor problem - input file does not end in .fet extension - might be a problem when saving the timetables"<<" (file:"<<__FILE__<<", line:"<<__LINE__<<")"<<endl;
	}
	
	PREFIX_STATISTICS=DIRECTORY_STATISTICS+FILE_SEP;
	
	int ok=QMessageBox::question(NULL, tr("FET Question"),
		 StatisticsExport::tr("Do you want to export detailed statistic files into directory %1 as html files?").arg(QDir::toNativeSeparators(DIRECTORY_STATISTICS)), QMessageBox::Yes | QMessageBox::No);
	if(ok==QMessageBox::No)
		return;

	/* need if i use iTeachersList. Currently unneeded. please remove commented asserts in other funktions if this is needed again!
	bool tmp=gt.rules.computeInternalStructure();
	if(!tmp){
		QMessageBox::critical(NULL, tr("FET critical"),
		StatisticsExport::tr("Incorrect data")+"\n");
		return;
		assert(0==1);
	}*/

	QDir dir;
	if(!dir.exists(OUTPUT_DIR))
		dir.mkpath(OUTPUT_DIR);
	if(!dir.exists(DIRECTORY_STATISTICS))
		dir.mkpath(DIRECTORY_STATISTICS);

	QSet<QString> allStudentsNamesSet;
	allStudentsNames.clear();
	//allStudentsNamesSet.clear();
	foreach(StudentsYear* sty, gt.rules.yearsList){
		if(!allStudentsNamesSet.contains(sty->name)){
			allStudentsNames<<sty->name;
			allStudentsNamesSet.insert(sty->name);
		}
		foreach(StudentsGroup* stg, sty->groupsList){
			if(!allStudentsNamesSet.contains(stg->name)){
				allStudentsNames<<stg->name;
				allStudentsNamesSet.insert(stg->name);
			}
			foreach(StudentsSubgroup* sts, stg->subgroupsList){
				if(!allStudentsNamesSet.contains(sts->name)){
					allStudentsNames<<sts->name;
					allStudentsNamesSet.insert(sts->name);
				}
			}
		}
	}

	allTeachersNames.clear();				// just needed, because i don't need to care about correct iTeacherList if i do it this way.
	foreach(Teacher* t, gt.rules.teachersList){		//  So i don't need gt.rules.computeInternalStructure();
		allTeachersNames<<t->name;
	}

	allSubjectsNames.clear();				// just done, because i always want to do it the same way + it is faster
	foreach(Subject* s, gt.rules.subjectsList){		// Also don't display empty subjects is easier
		allSubjectsNames<<s->name;
	}

	studentsTotalNumberOfHours.clear();
	studentsTotalNumberOfHours2.clear();

	subjectsTotalNumberOfHours.clear();
	subjectsTotalNumberOfHours4.clear();

	teachersTotalNumberOfHours.clear();
	teachersTotalNumberOfHours2.clear();

	studentsActivities.clear();
	subjectsActivities.clear();
	teachersActivities.clear();

	Activity* act;
	
	//QProgressDialog progress(NULL);
	//progress.setLabelText(tr("Processing activities...please wait"));
	//progress.setRange(0,gt.rules.activitiesList.count());
	//progress.setModal(true);
	
	for(int ai=0; ai<gt.rules.activitiesList.size(); ai++){
	
		//progress.setValue(ai);
		//pqapplication->processEvents();
		
		act=gt.rules.activitiesList[ai];
		if(act->active){
				subjectsActivities.insert(act->subjectName, ai);
				int tmp=subjectsTotalNumberOfHours.value(act->subjectName)+act->duration;
				subjectsTotalNumberOfHours.insert(act->subjectName, tmp);						// (1) so teamlearning-teaching is not counted twice!
				foreach(QString t, act->teachersNames){
					teachersActivities.insert(t, ai);
					tmp=teachersTotalNumberOfHours.value(t)+act->duration;
					teachersTotalNumberOfHours.insert(t, tmp);							// (3)
					//subjectstTotalNumberOfHours2[act->subjectIndex]+=duration;				// (1) so teamteaching is counted twice!
				}
				foreach(QString st, act->studentsNames){
					studentsActivities.insert(st, ai);
					tmp=studentsTotalNumberOfHours.value(st)+act->duration;
					studentsTotalNumberOfHours.insert(st, tmp);							// (2)
					//subjectstTotalNumberOfHours3[act->subjectIndex]+=duration;				// (1) so teamlearning is counted twice!
				}
				foreach(QString t, act->teachersNames){
					tmp=teachersTotalNumberOfHours2.value(t);
					tmp += act->duration * act->studentsNames.count();
					teachersTotalNumberOfHours2.insert(t, tmp);						// (3)
				}
				foreach(QString st, act->studentsNames){
					tmp=studentsTotalNumberOfHours2.value(st);
					tmp += act->duration * act->teachersNames.count();
					studentsTotalNumberOfHours2.insert(st, tmp);					// (2)
				}
				tmp=subjectsTotalNumberOfHours4.value(act->subjectName);
				tmp += act->duration * act->studentsNames.count() * act->teachersNames.count();
				subjectsTotalNumberOfHours4.insert(act->subjectName, tmp);			// (1) so teamlearning-teaching is counted twice!
			}
	}
	//progress.setValue(gt.rules.activitiesList.count());
	QStringList tmp;
	tmp.clear();
	foreach(QString students, allStudentsNames){
		if(studentsTotalNumberOfHours.value(students)==0 && studentsTotalNumberOfHours2.value(students)==0){
			studentsTotalNumberOfHours.remove(students);
			studentsTotalNumberOfHours2.remove(students);
		} else
			tmp<<students;
        }
        allStudentsNames=tmp;
	tmp.clear();
	foreach(QString teachers, allTeachersNames){
		if(teachersTotalNumberOfHours.value(teachers)==0 && teachersTotalNumberOfHours2.value(teachers)==0){
				teachersTotalNumberOfHours.remove(teachers);
				teachersTotalNumberOfHours2.remove(teachers);
		} else
			tmp<<teachers;
        }
        allTeachersNames=tmp;
	tmp.clear();
	foreach(QString subjects, allSubjectsNames){
		if(subjectsTotalNumberOfHours.value(subjects)==0 && subjectsTotalNumberOfHours4.value(subjects)==0){
			subjectsTotalNumberOfHours.remove(subjects);
			subjectsTotalNumberOfHours4.remove(subjects);
		} else
			tmp<<subjects;
        }
        allSubjectsNames=tmp;
	tmp.clear();

	QDate dat=QDate::currentDate();
	QTime tim=QTime::currentTime();
	QLocale loc(FET_LANGUAGE);
	QString sTime=loc.toString(dat, QLocale::ShortFormat)+" "+loc.toString(tim, QLocale::ShortFormat);

	ok=true;
	ok=exportStatisticsStylesheetCss(sTime);
	if(ok)
		ok=exportStatisticsIndex(sTime);
	if(ok)
		ok=exportStatisticsTeachersSubjects(sTime);
	if(ok)
		ok=exportStatisticsSubjectsTeachers(sTime);
	if(ok)
		ok=exportStatisticsTeachersStudents(sTime);
	if(ok)
		ok=exportStatisticsStudentsTeachers(sTime);
	if(ok)
		ok=exportStatisticsSubjectsStudents(sTime);
	if(ok)
		ok=exportStatisticsStudentsSubjects(sTime);

	if(ok){
		QMessageBox::information(NULL, tr("FET Information"),
		 StatisticsExport::tr("Statistic files were exported to directory %1 as html files.").arg(QDir::toNativeSeparators(DIRECTORY_STATISTICS)));
	} else {
		QMessageBox::warning(NULL, tr("FET warning"),
		 StatisticsExport::tr("Statistic export incomplete")+"\n");
	}
	teachersTotalNumberOfHours.clear();
	teachersTotalNumberOfHours2.clear();
	studentsTotalNumberOfHours.clear();
	studentsTotalNumberOfHours2.clear();
	subjectsTotalNumberOfHours.clear();
	subjectsTotalNumberOfHours4.clear();
	allStudentsNames.clear();
	allTeachersNames.clear();
	allSubjectsNames.clear();
	studentsActivities.clear();
	teachersActivities.clear();
	subjectsActivities.clear();

	//allStudentsNamesSet.clear();
	hashSubjectIDsStatistics.clear();
	hashActivityTagIDsStatistics.clear();
	hashStudentIDsStatistics.clear();
	hashTeacherIDsStatistics.clear();
	hashRoomIDsStatistics.clear();
	hashDayIDsStatistics.clear();
}



void StatisticsExport::computeHashForIDsStatistics(){		// by Volker Dirr

//TODO if an use a relational data base this is unneded, because we can use the primary key id of the database 
//This is very similar to timetable compute hash. so always check it if you change something here!

/*	QSet<QString> usedStudents;
	for(int i=0; i<gt.rules.nInternalActivities; i++){
		foreach(QString st, gt.rules.activitiesList[i].studentsNames){ //check if active
			if(!usedStudents.contains(st))
				usedStudents<<st;
		}
	}*/
	hashStudentIDsStatistics.clear();
	int cnt=1;
	for(int i=0; i<gt.rules.yearsList.size(); i++){
		StudentsYear* sty=gt.rules.yearsList[i];
		//if(usedStudents.contains(sty->name)){
		if(!hashStudentIDsStatistics.contains(sty->name)){
			hashStudentIDsStatistics.insert(sty->name, QString::number(cnt));
			cnt++;
		}
		//}
		for(int j=0; j<sty->groupsList.size(); j++){
			StudentsGroup* stg=sty->groupsList[j];
		//	if(usedStudents.contains(stg->name)){
			if(!hashStudentIDsStatistics.contains(stg->name)){
				hashStudentIDsStatistics.insert(stg->name, QString::number(cnt));
				cnt++;
			}
		//	}
			for(int k=0; k<stg->subgroupsList.size(); k++){
				StudentsSubgroup* sts=stg->subgroupsList[k];
		//		if(usedStudents.contains(sts->name)){
				if(!hashStudentIDsStatistics.contains(sts->name)){
					hashStudentIDsStatistics.insert(sts->name, QString::number(cnt));
					cnt++;
				}
		//		}
			}
		}
	}

	hashSubjectIDsStatistics.clear();
	for(int i=0; i<gt.rules.subjectsList.size(); i++){
		hashSubjectIDsStatistics.insert(gt.rules.subjectsList[i]->name, QString::number(i+1));
	}
	hashActivityTagIDsStatistics.clear();
	for(int i=0; i<gt.rules.activityTagsList.size(); i++){
		hashActivityTagIDsStatistics.insert(gt.rules.activityTagsList[i]->name, QString::number(i+1));
	}
	hashTeacherIDsStatistics.clear();
	for(int i=0; i<gt.rules.teachersList.size(); i++){
		hashTeacherIDsStatistics.insert(gt.rules.teachersList[i]->name, QString::number(i+1));
	}
	hashRoomIDsStatistics.clear();
	for(int room=0; room<gt.rules.roomsList.size(); room++){
		hashRoomIDsStatistics.insert(gt.rules.roomsList[room]->name, QString::number(room+1));
	}
	hashDayIDsStatistics.clear();
	for(int k=0; k<gt.rules.nDaysPerWeek; k++){
		hashDayIDsStatistics.insert(gt.rules.daysOfTheWeek[k], QString::number(k+1));
	}
}




bool StatisticsExport::exportStatisticsStylesheetCss(QString saveTime){
	assert(gt.rules.initialized);// && gt.rules.internalStructureComputed);
	QString s2=INPUT_FILENAME_XML.right(INPUT_FILENAME_XML.length()-INPUT_FILENAME_XML.findRev(FILE_SEP)-1);	//TODO: remove s2, because to long filenames!

	if(s2.right(4)==".fet")
		s2=s2.left(s2.length()-4);
	//else if(INPUT_FILENAME_XML!="")
	//	cout<<"Minor problem - input file does not end in .fet extension - might be a problem when saving the timetables"<<" (file:"<<__FILE__<<", line:"<<__LINE__<<")"<<endl;

	QString bar;
	if(INPUT_FILENAME_XML=="")
		bar="";
	else
		bar="_";
	
	QString htmlfilename=PREFIX_STATISTICS+s2+bar+STYLESHEET_STATISTICS;

	QFile file(htmlfilename);
	if(!file.open(QIODevice::WriteOnly)){
		QMessageBox::critical(NULL, tr("FET critical"),
		 StatisticsExport::tr("Cannot open file %1 for writing. Please check your disk's free space. Saving of %1 aborted.").arg(htmlfilename));
		return false;
		assert(0);
	}
	QTextStream tos(&file);
	tos.setCodec("UTF-8");
	tos.setGenerateByteOrderMark(true);

	//get used students	//similar in timetableexport.cpp, so maybe use a function?
	QSet<QString> usedStudents;
	for(int i=0; i<gt.rules.activitiesList.size(); i++){
		foreach(QString st, gt.rules.activitiesList[i]->studentsNames){
			if(gt.rules.activitiesList[i]->active){
				if(!usedStudents.contains(st))
					usedStudents<<st;
			}
		}
	}

	tos<<"/* "<<StatisticsExport::tr("CSS Stylesheet of %1", "%1 is the name of the file").arg(INPUT_FILENAME_XML.right(INPUT_FILENAME_XML.length()-INPUT_FILENAME_XML.findRev(FILE_SEP)-1))<<"\n";
	tos<<"   "<<StatisticsExport::tr("Stylesheet generated with FET %1 on %2", "%1 is FET version, %2 is date and time").arg(FET_VERSION).arg(saveTime)<<" */\n\n";
	tos<<"/* "<<StatisticsExport::tr("To hide an element just write the following phrase into the element")<<": display:none; */\n\n";
	tos<<"table {\n  text-align: center;\n}\n\n";
	tos<<"table.modulo2 {\n\n}\n\n";
	tos<<"table.detailed {\n  margin-left:auto; margin-right:auto;\n  text-align: center;\n  border: 0px;\n  border-spacing: 0;\n  border-collapse: collapse;\n}\n\n";
	tos<<"caption {\n\n}\n\n";
	tos<<"thead {\n\n}\n\n";
	//workaround begin. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
	tos<<"/* OpenOffice import the tfoot incorrect. So I used tr.foot instead of tfoot.\n";
	tos<<"   compare http://www.openoffice.org/issues/show_bug.cgi?id=82600\n";
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
		for(int i=0; i<gt.rules.subjectsList.size(); i++){
			tos << "span.s_"<<hashSubjectIDsStatistics.value(gt.rules.subjectsList[i]->name)<<" { /* subject "<<gt.rules.subjectsList[i]->name<<" */\n\n}\n\n";
		}
		for(int i=0; i<gt.rules.activityTagsList.size(); i++){
			tos << "span.at_"<<hashActivityTagIDsStatistics.value(gt.rules.activityTagsList[i]->name)<<" { /* activity tag "<<gt.rules.activityTagsList[i]->name<<" */\n\n}\n\n";
		}
		for(int i=0; i<gt.rules.yearsList.size(); i++){
			StudentsYear* sty=gt.rules.yearsList[i];
			if(usedStudents.contains(sty->name))
				tos << "span.ss_"<<hashStudentIDsStatistics.value(sty->name)<<" { /* students set "<<sty->name<<" */\n\n}\n\n";
			for(int j=0; j<sty->groupsList.size(); j++){
				StudentsGroup* stg=sty->groupsList[j];
				if(usedStudents.contains(stg->name))
					tos << "span.ss_"<<hashStudentIDsStatistics.value(stg->name)<<" { /* students set "<<stg->name<<" */\n\n}\n\n";
				for(int k=0; k<stg->subgroupsList.size(); k++){
					StudentsSubgroup* sts=stg->subgroupsList[k];
					if(usedStudents.contains(sts->name))
						tos << "span.ss_"<<hashStudentIDsStatistics.value(sts->name)<<" { /* students set "<<sts->name<<" */\n\n}\n\n";
				}
			}
		}
		for(int i=0; i<gt.rules.teachersList.size(); i++){
			tos << "span.t_"<<hashTeacherIDsStatistics.value(gt.rules.teachersList[i]->name)<<" { /* teacher "<<gt.rules.teachersList[i]->name<<" */\n\n}\n\n";
		}
		for(int room=0; room<gt.rules.roomsList.size(); room++){
			tos << "span.r_"<<hashRoomIDsStatistics.value(gt.rules.roomsList[room]->name)<<" { /* room "<<gt.rules.roomsList[room]->name<<" */\n\n}\n\n";
		}
	}
	if(TIMETABLE_HTML_LEVEL>=3){
		tos<<"span.subject {\n\n}\n\n";
		tos<<"span.activitytag {\n\n}\n\n";
		tos<<"span.empty {\n  color: gray;\n}\n\n";
		tos<<"td.empty {\n  border-color:silver;\n  border-right-style:none;\n  border-bottom-style:none;\n  border-left-style:dotted;\n  border-top-style:dotted;\n}\n\n";
		tos<<"span.notAvailable {\n  color: gray;\n}\n\n";
		tos<<"td.notAvailable {\n  border-color:silver;\n  border-right-style:none;\n  border-bottom-style:none;\n  border-left-style:dotted;\n  border-top-style:dotted;\n}\n\n";
		tos<<"td.student, div.student {\n\n}\n\n";
		tos<<"td.teacher, div.teacher {\n\n}\n\n";
		tos<<"td.room, div.room {\n\n}\n\n";
		tos<<"tr.line0 {\n  font-size: smaller;\n}\n\n";
		tos<<"tr.line1, div.line1 {\n\n}\n\n";
		tos<<"tr.line2, div.line2 {\n  font-size: smaller;\n  color: gray;\n}\n\n";
		tos<<"tr.line3, div.line3 {\n  font-size: smaller;\n  color: silver;\n}\n\n";
	}
	
	tos<<endl<<"/* "<<StatisticsExport::tr("End of file.")<<" */\n\n";

	if(file.error()>0){
		QMessageBox::critical(NULL, tr("FET critical"),
		 StatisticsExport::tr("Writing %1 gave error code %2, which means saving is compromised. Please check your disk's free space.").arg(htmlfilename).arg(file.error()));
		return false;
	}
	file.close();
	return true;
}



bool StatisticsExport::exportStatisticsIndex(QString saveTime){
	assert(gt.rules.initialized);// && gt.rules.internalStructureComputed);
	QString s2=INPUT_FILENAME_XML.right(INPUT_FILENAME_XML.length()-INPUT_FILENAME_XML.findRev(FILE_SEP)-1);	//TODO: remove s2, because to long filenames!
	
	if(s2.right(4)==".fet")
		s2=s2.left(s2.length()-4);
	//else if(INPUT_FILENAME_XML!="")
	//	cout<<"Minor problem - input file does not end in .fet extension - might be a problem when saving the timetables"<<" (file:"<<__FILE__<<", line:"<<__LINE__<<")"<<endl;

	QString bar;
	if(INPUT_FILENAME_XML=="")
		bar="";
	else
		bar="_";
	
	QString htmlfilename=PREFIX_STATISTICS+s2+bar+INDEX_STATISTICS;
	QFile file(htmlfilename);
	if(!file.open(QIODevice::WriteOnly)){
		QMessageBox::critical(NULL, tr("FET critical"),
		 StatisticsExport::tr("Cannot open file %1 for writing. Please check your disk's free space. Saving of %1 aborted.").arg(htmlfilename));
		return false;
		assert(0);
	}
	QTextStream tos(&file);
	tos.setCodec("UTF-8");
	tos.setGenerateByteOrderMark(true);

	tos<<"<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\"\n";
	tos<<"  \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n\n";

	if(LANGUAGE_STYLE_RIGHT_TO_LEFT==false)
		tos<<"<html xmlns=\"http://www.w3.org/1999/xhtml\" lang=\""<<LANGUAGE_FOR_HTML<<"\" xml:lang=\""<<LANGUAGE_FOR_HTML<<"\">\n";
	else
		tos<<"<html xmlns=\"http://www.w3.org/1999/xhtml\" lang=\""<<LANGUAGE_FOR_HTML<<"\" xml:lang=\""<<LANGUAGE_FOR_HTML<<"\" dir=\"rtl\">\n";
	tos<<"  <head>\n";
	tos<<"    <title>"<<protect2(gt.rules.institutionName)<<"</title>\n";
	tos<<"    <meta http-equiv=\"Content-Type\" content=\"text/html;charset=utf-8\" />\n";

	if(TIMETABLE_HTML_LEVEL>=1){
		QString bar;
		if(INPUT_FILENAME_XML=="")
			bar="";
		else
			bar="_";

		QString cssfilename=s2+bar+STYLESHEET_STATISTICS;
		tos<<"    <link rel=\"stylesheet\" media=\"all\" href=\""<<cssfilename<<"\" type=\"text/css\" />\n";
	}
	if(TIMETABLE_HTML_LEVEL>=5){  // the following JavaScript code is pretty similar to an example of Les Richardson ( http://richtech.ca/openadmin/index.html )
		tos<<"    <meta http-equiv=\"Content-Script-Type\" content=\"text/javascript\" />\n";
		tos<<"    <script type=\"text/javascript\">\n";
		tos<<"      function highlight(classval) {\n";
		tos<<"        var spans = document.getElementsByTagName('span');\n";
		tos<<"        for(var i=0;spans.length>i;i++) {\n";
		tos<<"          if (spans[i].className == classval) {\n";
		tos<<"            spans[i].style.backgroundColor = 'lime';\n";
		tos<<"          } else {\n";
		tos<<"            spans[i].style.backgroundColor = 'white';\n";
		tos<<"          }\n";
		tos<<"        }\n";
		tos<<"      }\n";
		tos<<"    </script>\n";
	}
	tos<<"  </head>\n\n";

	tos<<"  <body>\n";

	tos<<"    <p>\n      <strong>"<<StatisticsExport::tr("Institution name")<<":</strong> "<<protect2(gt.rules.institutionName)<<"<br />\n";
	tos<<"      <strong>"<<StatisticsExport::tr("Comments")<<":</strong> "<<protect2(gt.rules.comments)<<"\n    </p>\n"; 

	tos<<"    <table border=\"1\">\n";
	tos<<"      <caption>"<<protect2(gt.rules.institutionName)<<"</caption>\n";
	tos<<"      <thead>\n        <tr><td rowspan=\"2\"></td><th colspan=\"3\">"+tr("Statistics")+"</th></tr>\n";
	tos<<"        <tr>\n          <!-- span -->\n";
	tos<<"          <th>"+tr("Teachers")+"</th><th>"+tr("Students")+"</th><th>"+tr("Subjects")+"</th>\n";
	tos<<"        </tr>\n";
	tos<<"      </thead>\n";
	tos<<"      <tbody>\n";
	tos<<"        <tr>\n";
	tos<<"          <th>"+tr("Teachers")+"</th>\n";
	tos<<"          <td>"<<protect2(STRING_EMPTY_SLOT_STATISTICS)<<"</td>\n";
	tos<<"          <td><a href=\""<<s2+bar+TEACHERS_STUDENTS_STATISTICS<<"\">"+tr("view")+"</a></td>\n";
	tos<<"          <td><a href=\""<<s2+bar+TEACHERS_SUBJECTS_STATISTICS<<"\">"+tr("view")+"</a></td>\n";
	tos<<"        </tr>\n";
	tos<<"        <tr>\n";
	tos<<"          <th>"+tr("Students")+"</th>\n";
	tos<<"          <td><a href=\""<<s2+bar+STUDENTS_TEACHERS_STATISTICS<<"\">"+tr("view")+"</a></td>\n";
	tos<<"          <td>"<<protect2(STRING_EMPTY_SLOT_STATISTICS)<<"</td>\n";
	tos<<"          <td><a href=\""<<s2+bar+STUDENTS_SUBJECTS_STATISTICS<<"\">"+tr("view")+"</a></td>\n";
	tos<<"        </tr>\n";
	tos<<"        <tr>\n";
	tos<<"          <th>"+tr("Subjects")+"</th>\n";
	tos<<"          <td><a href=\""<<s2+bar+SUBJECTS_TEACHERS_STATISTICS<<"\">"+tr("view")+"</a></td>\n";
	tos<<"          <td><a href=\""<<s2+bar+SUBJECTS_STUDENTS_STATISTICS<<"\">"+tr("view")+"</a></td>\n";
	tos<<"          <td>"<<protect2(STRING_EMPTY_SLOT_STATISTICS)<<"</td>\n";
	tos<<"        </tr>\n";
	//workaround begin. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
	tos<<"      <tr class=\"foot\"><td></td><td colspan=\"3\">"<<StatisticsExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr>\n";
	//workaround end.
	tos<<"      </tbody>\n";
	tos<<"    </table>\n";
	tos<<"  </body>\n</html>\n\n";

	if(file.error()>0){
		QMessageBox::critical(NULL, tr("FET critical"),
		 StatisticsExport::tr("Writing %1 gave error code %2, which means saving is compromised. Please check your disk's free space.").arg(htmlfilename).arg(file.error()));
		return false;
	}
	file.close();
	return true;
}



bool StatisticsExport::exportStatisticsTeachersSubjects(QString saveTime){
	assert(gt.rules.initialized);// && gt.rules.internalStructureComputed);
	QString s2=INPUT_FILENAME_XML.right(INPUT_FILENAME_XML.length()-INPUT_FILENAME_XML.findRev(FILE_SEP)-1);	//TODO: remove s2, because to long filenames!

	if(s2.right(4)==".fet")
		s2=s2.left(s2.length()-4);
	//else if(INPUT_FILENAME_XML!="")
	//	cout<<"Minor problem - input file does not end in .fet extension - might be a problem when saving the timetables"<<" (file:"<<__FILE__<<", line:"<<__LINE__<<")"<<endl;

	QString bar;
	if(INPUT_FILENAME_XML=="")
		bar="";
	else
		bar="_";

	QString htmlfilename=PREFIX_STATISTICS+s2+bar+TEACHERS_SUBJECTS_STATISTICS;
	QFile file(htmlfilename);
	if(!file.open(QIODevice::WriteOnly)){
		QMessageBox::critical(NULL, tr("FET critical"),
		 StatisticsExport::tr("Cannot open file %1 for writing. Please check your disk's free space. Saving of %1 aborted.").arg(htmlfilename));
		return false;
		assert(0);
	}
	QTextStream tos(&file);
	tos.setCodec("UTF-8");
	tos.setGenerateByteOrderMark(true);

	tos<<"<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\"\n";
	tos<<"  \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n\n";

	if(LANGUAGE_STYLE_RIGHT_TO_LEFT==false)
		tos<<"<html xmlns=\"http://www.w3.org/1999/xhtml\" lang=\""<<LANGUAGE_FOR_HTML<<"\" xml:lang=\""<<LANGUAGE_FOR_HTML<<"\">\n";
	else
		tos<<"<html xmlns=\"http://www.w3.org/1999/xhtml\" lang=\""<<LANGUAGE_FOR_HTML<<"\" xml:lang=\""<<LANGUAGE_FOR_HTML<<"\" dir=\"rtl\">\n";
	tos<<"  <head>\n";
	tos<<"    <title>"<<protect2(gt.rules.institutionName)<<"</title>\n";
	tos<<"    <meta http-equiv=\"Content-Type\" content=\"text/html;charset=utf-8\" />\n";
	if(TIMETABLE_HTML_LEVEL>=1){
		QString bar;
		if(INPUT_FILENAME_XML=="")
			bar="";
		else
			bar="_";
	
		QString cssfilename=s2+bar+STYLESHEET_STATISTICS;
		tos<<"    <link rel=\"stylesheet\" media=\"all\" href=\""<<cssfilename<<"\" type=\"text/css\" />\n";
	}
	if(TIMETABLE_HTML_LEVEL>=5){  // the following JavaScript code is pretty similar to an example of Les Richardson ( http://richtech.ca/openadmin/index.html )
		tos<<"    <meta http-equiv=\"Content-Script-Type\" content=\"text/javascript\" />\n";
		tos<<"    <script type=\"text/javascript\">\n";
		tos<<"      function highlight(classval) {\n";
		tos<<"        var spans = document.getElementsByTagName('span');\n";
		tos<<"        for(var i=0;spans.length>i;i++) {\n";
		tos<<"          if (spans[i].className == classval) {\n";
		tos<<"            spans[i].style.backgroundColor = 'lime';\n";
		tos<<"          } else {\n";
		tos<<"            spans[i].style.backgroundColor = 'white';\n";
		tos<<"          }\n";
		tos<<"        }\n";
		tos<<"      }\n";
		tos<<"    </script>\n";
	}
	tos<<"  </head>\n\n";

	tos<<"  <body>\n";
	tos<<"    <table border=\"1\">\n";	
	tos<<"      <caption>"<<protect2(gt.rules.institutionName)<<"</caption>\n";
	tos<<"      <thead>\n        <tr><td rowspan=\"2\"></td><th colspan=\""<<allTeachersNames.size()+1<<"\">"<<tr("Teachers - Subjects Matrix")<<"</th></tr>\n";
	tos<<"        <tr>\n          <!-- span -->\n";
	foreach(QString teachers, allTeachersNames){
		if(TIMETABLE_HTML_LEVEL>=2)
			tos<<"          <th class=\"xAxis\">";
		else
			tos<<"          <th>";
		tos<<protect2(teachers)<<"</th>\n";
	}
	if(TIMETABLE_HTML_LEVEL>=2)
		tos<<"          <th class=\"xAxis\">";
	else
		tos<<"          <th>";
	tos<<protect2(tr("Sum", "This means the sum of more values, the total"))<<"</th>\n";
	tos<<"        </tr>\n";
	tos<<"      </thead>\n";
	//workaround. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
	//tos<<"      <tfoot><tr><td></td><td colspan=\""<<allTeachersNames.size()+1<<"\">"<<StatisticsExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr></tfoot>\n";
	tos<<"      <tbody>\n";
	
	QProgressDialog progress(NULL);
	progress.setWindowTitle(tr("Exporting statistics", "Title of a progress dialog"));
	progress.setLabelText(tr("Processing teachers with subjects...please wait"));
	progress.setRange(0, allSubjectsNames.count());
	progress.setModal(true);
	
	int ttt=0;
	
	foreach(QString subjects, allSubjectsNames){
		progress.setValue(ttt);
		//pqapplication->processEvents();
		if(progress.wasCanceled()){
			QMessageBox::warning(NULL, tr("FET warning"), tr("Canceled"));
			return false;
		}
		ttt++;
	
		QList<int> tmpSubjects;
		QMultiHash<QString, int> tmpTeachers;
		tmpSubjects.clear();
		tmpTeachers.clear();
		tmpSubjects=subjectsActivities.values(subjects);
		foreach(int aidx, tmpSubjects){
			Activity* act=gt.rules.activitiesList.at(aidx);
			foreach(QString teacher, act->teachersNames){
				tmpTeachers.insert(teacher, aidx);
			}
		}
		tos<<"        <tr>\n";
		if(TIMETABLE_HTML_LEVEL>=2)
			tos<<"          <th class=\"yAxis\">";
		else
			tos<<"          <th>";
		tos<<protect2(subjects)<<"</th>\n";
		foreach(QString teachers, allTeachersNames){
			QList<int> tmpActivities;
			tmpActivities.clear();
			tmpActivities=tmpTeachers.values(teachers);
			if(tmpActivities.isEmpty()){
				switch(TIMETABLE_HTML_LEVEL){
					case 3 : ;
					case 4 : tos<<"          <td class=\"empty\"><span class=\"empty\">"<<protect2(STRING_EMPTY_SLOT_STATISTICS)<<"</span></td>\n"; break;
					case 5 : ;
					case 6 : tos<<"          <td class=\"empty\"><span class=\"empty\" onmouseover=\"highlight('empty')\">"<<protect2(STRING_EMPTY_SLOT_STATISTICS)<<"</span></td>\n"; break;
					default: tos<<"          <td>"<<protect2(STRING_EMPTY_SLOT_STATISTICS)<<"</td>\n";
				}
			} else {
				QMap<QString, int> tmpStudentDuration;	//not QHash, because i need the correct order of the activities
				foreach(int tmpAct, tmpActivities){
					Activity* act=gt.rules.activitiesList[tmpAct];
					int tmpD=act->duration;
					QString tmpSt;
					if(act->studentsNames.size()>0){
						for(QStringList::Iterator st=act->studentsNames.begin(); st!=act->studentsNames.end(); st++){
							switch(TIMETABLE_HTML_LEVEL){
								case 4 : tmpSt+="<span class=\"ss_"+hashStudentIDsStatistics.value(*st)+"\">"+protect2(*st)+"</span>"; break;
								case 5 : ;
								case 6 : tmpSt+="<span class=\"ss_"+hashStudentIDsStatistics.value(*st)+"\" onmouseover=\"highlight('ss_"+hashStudentIDsStatistics.value(*st)+"')\">"+protect2(*st)+"</span>"; break;
								default: tmpSt+=protect2(*st); break;
								}
							if(st!=act->studentsNames.end()-1)
								tmpSt+=", ";
						}
					} else
						tmpSt=" ";
					tmpD+=tmpStudentDuration.value(tmpSt);
					tmpStudentDuration.insert(tmpSt, tmpD);
				}
				if(TIMETABLE_HTML_LEVEL>=1)
					tos<<"          <td><table class=\"detailed\">";
				else
					tos<<"          <td><table>";
				if(TIMETABLE_HTML_LEVEL>=3)
					tos<<"<tr class=\"duration line1\">";
				else	tos<<"<tr>";
				QMapIterator<QString, int> it(tmpStudentDuration);
				while(it.hasNext()){
					it.next();
					if(TIMETABLE_HTML_LEVEL>=1)
						tos<<"<td class=\"detailed\">";
					else
						tos<<"<td>";
					tos<<it.value()<<"</td>";	
				}
				tos<<"</tr>";
				if(TIMETABLE_HTML_LEVEL>=3)
					tos<<"<tr class=\"student line2\">";
				else	tos<<"<tr>";
				QMapIterator<QString, int> it2(tmpStudentDuration);	//do it with the same iterator
				while(it2.hasNext()){
					it2.next();
					if(TIMETABLE_HTML_LEVEL>=1)
						tos<<"<td class=\"detailed\">";
					else
						tos<<"<td>";
					tos<<it2.key()<<"</td>";	
				}
				tos<<"</tr>";
				tos<<"</table></td>\n";
			}
		}
		tos<<"          <th>";
		tos<<QString::number(subjectsTotalNumberOfHours.value(subjects));
		if(subjectsTotalNumberOfHours.value(subjects)!=subjectsTotalNumberOfHours4.value(subjects))
			tos<<"<br />("<<QString::number(subjectsTotalNumberOfHours4.value(subjects))<<")";
		tos<<"</th>\n";
		tos<<"        </tr>\n";
	}
	
	progress.setValue(allSubjectsNames.count());
	
	tos<<"        <tr>\n";
	if(TIMETABLE_HTML_LEVEL>=2)
		tos<<"          <th class=\"xAxis\">";
	else
		tos<<"          <th>";
	tos<<protect2(tr("Sum", "This means the sum of more values, the total"))<<"</th>\n";
	foreach(QString teachers, allTeachersNames){
		tos<<"          <th>"<<QString::number(teachersTotalNumberOfHours.value(teachers));
		if(teachersTotalNumberOfHours.value(teachers)!=teachersTotalNumberOfHours2.value(teachers))
			tos<<"<br />("<<QString::number(teachersTotalNumberOfHours2.value(teachers))<<")";
		tos<<"</th>\n";
	}
	tos<<"          <th></th>\n        </tr>\n";
	//workaround begin. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
	tos<<"      <tr class=\"foot\"><td></td><td colspan=\""<<allTeachersNames.size()+1<<"\">"<<StatisticsExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr>\n";
	//workaround end.
	tos<<"      </tbody>\n";
	tos<<"    </table>\n";
	tos<<"  </body>\n</html>\n\n";

	if(file.error()>0){
		QMessageBox::critical(NULL, tr("FET critical"),
		 StatisticsExport::tr("Writing %1 gave error code %2, which means saving is compromised. Please check your disk's free space.").arg(htmlfilename).arg(file.error()));
		return false;
	}
	file.close();
	return true;
}



bool StatisticsExport::exportStatisticsSubjectsTeachers(QString saveTime){
	assert(gt.rules.initialized);// && gt.rules.internalStructureComputed);
	QString s2=INPUT_FILENAME_XML.right(INPUT_FILENAME_XML.length()-INPUT_FILENAME_XML.findRev(FILE_SEP)-1);	//TODO: remove s2, because to long filenames!

	if(s2.right(4)==".fet")
		s2=s2.left(s2.length()-4);
	//else if(INPUT_FILENAME_XML!="")
	//	cout<<"Minor problem - input file does not end in .fet extension - might be a problem when saving the timetables"<<" (file:"<<__FILE__<<", line:"<<__LINE__<<")"<<endl;

	QString bar;
	if(INPUT_FILENAME_XML=="")
		bar="";
	else
		bar="_";

	QString htmlfilename=PREFIX_STATISTICS+s2+bar+SUBJECTS_TEACHERS_STATISTICS;

	QFile file(htmlfilename);
	if(!file.open(QIODevice::WriteOnly)){
		QMessageBox::critical(NULL, tr("FET critical"),
		 StatisticsExport::tr("Cannot open file %1 for writing. Please check your disk's free space. Saving of %1 aborted.").arg(htmlfilename));
		return false;
		assert(0);
	}
	QTextStream tos(&file);
	tos.setCodec("UTF-8");
	tos.setGenerateByteOrderMark(true);

	tos<<"<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\"\n";
	tos<<"  \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n\n";

	if(LANGUAGE_STYLE_RIGHT_TO_LEFT==false)
		tos<<"<html xmlns=\"http://www.w3.org/1999/xhtml\" lang=\""<<LANGUAGE_FOR_HTML<<"\" xml:lang=\""<<LANGUAGE_FOR_HTML<<"\">\n";
	else
		tos<<"<html xmlns=\"http://www.w3.org/1999/xhtml\" lang=\""<<LANGUAGE_FOR_HTML<<"\" xml:lang=\""<<LANGUAGE_FOR_HTML<<"\" dir=\"rtl\">\n";
	tos<<"  <head>\n";
	tos<<"    <title>"<<protect2(gt.rules.institutionName)<<"</title>\n";
	tos<<"    <meta http-equiv=\"Content-Type\" content=\"text/html;charset=utf-8\" />\n";
	if(TIMETABLE_HTML_LEVEL>=1){
		QString bar;
		if(INPUT_FILENAME_XML=="")
			bar="";
		else
			bar="_";
	
		QString cssfilename=s2+bar+STYLESHEET_STATISTICS;
		tos<<"    <link rel=\"stylesheet\" media=\"all\" href=\""<<cssfilename<<"\" type=\"text/css\" />\n";
	}
	if(TIMETABLE_HTML_LEVEL>=5){  // the following JavaScript code is pretty similar to an example of Les Richardson ( http://richtech.ca/openadmin/index.html )
		tos<<"    <meta http-equiv=\"Content-Script-Type\" content=\"text/javascript\" />\n";
		tos<<"    <script type=\"text/javascript\">\n";
		tos<<"      function highlight(classval) {\n";
		tos<<"        var spans = document.getElementsByTagName('span');\n";
		tos<<"        for(var i=0;spans.length>i;i++) {\n";
		tos<<"          if (spans[i].className == classval) {\n";
		tos<<"            spans[i].style.backgroundColor = 'lime';\n";
		tos<<"          } else {\n";
		tos<<"            spans[i].style.backgroundColor = 'white';\n";
		tos<<"          }\n";
		tos<<"        }\n";
		tos<<"      }\n";
		tos<<"    </script>\n";
	}
	tos<<"  </head>\n\n";

	tos<<"  <body>\n";
	tos<<"    <table border=\"1\">\n";	
	tos<<"      <caption>"<<protect2(gt.rules.institutionName)<<"</caption>\n";
	tos<<"      <thead>\n        <tr><td rowspan=\"2\"></td><th colspan=\""<<allSubjectsNames.size()+1<<"\">"<<tr("Subjects - Teachers Matrix")<<"</th></tr>\n";
	tos<<"        <tr>\n          <!-- span -->\n";
	foreach(QString subjects, allSubjectsNames){
		if(TIMETABLE_HTML_LEVEL>=2)
			tos<<"          <th class=\"xAxis\">";
		else
			tos<<"          <th>";
		tos<<protect2(subjects)<<"</th>\n";
	}
	if(TIMETABLE_HTML_LEVEL>=2)
		tos<<"          <th class=\"xAxis\">";
	else
		tos<<"          <th>";
	tos<<protect2(tr("Sum", "This means the sum of more values, the total"))<<"</th>\n";
	tos<<"        </tr>\n";
	tos<<"      </thead>\n";
	//workaround. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
	//tos<<"      <tfoot><tr><td></td><td colspan=\""<<allTeachersNames.size()+1<<"\">"<<StatisticsExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr></tfoot>\n";
	tos<<"      <tbody>\n";
	
	QProgressDialog progress(NULL);
	progress.setWindowTitle(tr("Exporting statistics", "Title of a progress dialog"));
	progress.setLabelText(tr("Processing subject with teachers...please wait"));
	progress.setRange(0, allTeachersNames.count());
	progress.setModal(true);
	
	int ttt=0;
	
	foreach(QString teachers, allTeachersNames){
		progress.setValue(ttt);
		//pqapplication->processEvents();
		if(progress.wasCanceled()){
			QMessageBox::warning(NULL, tr("FET warning"), tr("Canceled"));
			return false;
		}
		ttt++;
	
		QList<int> tmpTeachers;
		QMultiHash<QString, int> tmpSubjects;
		tmpTeachers.clear();
		tmpSubjects.clear();
		tmpTeachers=teachersActivities.values(teachers);
		foreach(int aidx, tmpTeachers){
			Activity* act=gt.rules.activitiesList.at(aidx);
			tmpSubjects.insert(act->subjectName, aidx);
		}
		tos<<"        <tr>\n";
		if(TIMETABLE_HTML_LEVEL>=2)
			tos<<"          <th class=\"yAxis\">";
		else
			tos<<"          <th>";
		tos<<protect2(teachers)<<"</th>\n";
		foreach(QString subjects, allSubjectsNames){
			QList<int> tmpActivities;
			tmpActivities.clear();
			tmpActivities=tmpSubjects.values(subjects);
			if(tmpActivities.isEmpty()){
				switch(TIMETABLE_HTML_LEVEL){
					case 3 : ;
					case 4 : tos<<"          <td class=\"empty\"><span class=\"empty\">"<<protect2(STRING_EMPTY_SLOT_STATISTICS)<<"</span></td>\n"; break;
					case 5 : ;
					case 6 : tos<<"          <td class=\"empty\"><span class=\"empty\" onmouseover=\"highlight('empty')\">"<<protect2(STRING_EMPTY_SLOT_STATISTICS)<<"</span></td>\n"; break;
					default: tos<<"          <td>"<<protect2(STRING_EMPTY_SLOT_STATISTICS)<<"</td>\n";
				}
			} else {
				QMap<QString, int> tmpStudentDuration;	//not QHash, because i need the correct order of the activities
				foreach(int tmpAct, tmpActivities){
					Activity* act=gt.rules.activitiesList[tmpAct];
					int tmpD=act->duration;
					QString tmpSt;
					if(act->studentsNames.size()>0){
						for(QStringList::Iterator st=act->studentsNames.begin(); st!=act->studentsNames.end(); st++){
							switch(TIMETABLE_HTML_LEVEL){
								case 4 : tmpSt+="<span class=\"ss_"+hashStudentIDsStatistics.value(*st)+"\">"+protect2(*st)+"</span>"; break;
								case 5 : ;
								case 6 : tmpSt+="<span class=\"ss_"+hashStudentIDsStatistics.value(*st)+"\" onmouseover=\"highlight('ss_"+hashStudentIDsStatistics.value(*st)+"')\">"+protect2(*st)+"</span>"; break;
								default: tmpSt+=protect2(*st); break;
								}
							if(st!=act->studentsNames.end()-1)
								tmpSt+=", ";
						}
					} else
						tmpSt=" ";
					tmpD+=tmpStudentDuration.value(tmpSt);
					tmpStudentDuration.insert(tmpSt, tmpD);
				}
				if(TIMETABLE_HTML_LEVEL>=1)
					tos<<"          <td><table class=\"detailed\">";
				else
					tos<<"          <td><table>";
				if(TIMETABLE_HTML_LEVEL>=3)
					tos<<"<tr class=\"duration line1\">";
				else	tos<<"<tr>";
				QMapIterator<QString, int> it(tmpStudentDuration);
				while(it.hasNext()){
					it.next();
					if(TIMETABLE_HTML_LEVEL>=1)
						tos<<"<td class=\"detailed\">";
					else
						tos<<"<td>";
					tos<<it.value()<<"</td>";	
				}
				tos<<"</tr>";
				if(TIMETABLE_HTML_LEVEL>=3)
					tos<<"<tr class=\"student line2\">";
				else	tos<<"<tr>";
				QMapIterator<QString, int> it2(tmpStudentDuration);	//do it with the same iterator
				while(it2.hasNext()){
					it2.next();
					if(TIMETABLE_HTML_LEVEL>=1)
						tos<<"<td class=\"detailed\">";
					else
						tos<<"<td>";
					tos<<it2.key()<<"</td>";	
				}
				tos<<"</tr>";
				tos<<"</table></td>\n";
			}
		}
		tos<<"          <th>";
		tos<<QString::number(teachersTotalNumberOfHours.value(teachers));
		if(teachersTotalNumberOfHours.value(teachers)!=teachersTotalNumberOfHours2.value(teachers))
			tos<<"<br />("<<QString::number(teachersTotalNumberOfHours2.value(teachers))<<")";
		tos<<"</th>\n";
		tos<<"        </tr>\n";
	}
	
	progress.setValue(allTeachersNames.count());
	
	tos<<"        <tr>\n";
	if(TIMETABLE_HTML_LEVEL>=2)
		tos<<"          <th class=\"xAxis\">";
	else
		tos<<"          <th>";
	tos<<protect2(tr("Sum", "This means the sum of more values, the total"))<<"</th>\n";
	foreach(QString subjects, allSubjectsNames){
		tos<<"          <th>"<<QString::number(subjectsTotalNumberOfHours.value(subjects));
		if(subjectsTotalNumberOfHours.value(subjects)!=subjectsTotalNumberOfHours4.value(subjects))
			tos<<"<br />("<<QString::number(subjectsTotalNumberOfHours4.value(subjects))<<")";
		tos<<"</th>\n";
	}
	tos<<"          <th></th>\n        </tr>\n";
	//workaround begin. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
	tos<<"      <tr class=\"foot\"><td></td><td colspan=\""<<allSubjectsNames.size()+1<<"\">"<<StatisticsExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr>\n";
	//workaround end.
	tos<<"      </tbody>\n";
	tos<<"    </table>\n";
	tos<<"  </body>\n</html>\n\n";

	if(file.error()>0){
		QMessageBox::critical(NULL, tr("FET critical"),
		 StatisticsExport::tr("Writing %1 gave error code %2, which means saving is compromised. Please check your disk's free space.").arg(htmlfilename).arg(file.error()));
		return false;
	}
	file.close();
	return true;
}



bool StatisticsExport::exportStatisticsTeachersStudents(QString saveTime){
	assert(gt.rules.initialized);// && gt.rules.internalStructureComputed);
	QString s2=INPUT_FILENAME_XML.right(INPUT_FILENAME_XML.length()-INPUT_FILENAME_XML.findRev(FILE_SEP)-1);	//TODO: remove s2, because to long filenames!

	if(s2.right(4)==".fet")
		s2=s2.left(s2.length()-4);
	//else if(INPUT_FILENAME_XML!="")
	//	cout<<"Minor problem - input file does not end in .fet extension - might be a problem when saving the timetables"<<" (file:"<<__FILE__<<", line:"<<__LINE__<<")"<<endl;

	QString bar;
	if(INPUT_FILENAME_XML=="")
		bar="";
	else
		bar="_";

	QString htmlfilename=PREFIX_STATISTICS+s2+bar+TEACHERS_STUDENTS_STATISTICS;

	QFile file(htmlfilename);
	if(!file.open(QIODevice::WriteOnly)){
		QMessageBox::critical(NULL, tr("FET critical"),
		 StatisticsExport::tr("Cannot open file %1 for writing. Please check your disk's free space. Saving of %1 aborted.").arg(htmlfilename));
		return false;
		assert(0);
	}
	QTextStream tos(&file);
	tos.setCodec("UTF-8");
	tos.setGenerateByteOrderMark(true);

	tos<<"<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\"\n";
	tos<<"  \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n\n";

	if(LANGUAGE_STYLE_RIGHT_TO_LEFT==false)
		tos<<"<html xmlns=\"http://www.w3.org/1999/xhtml\" lang=\""<<LANGUAGE_FOR_HTML<<"\" xml:lang=\""<<LANGUAGE_FOR_HTML<<"\">\n";
	else
		tos<<"<html xmlns=\"http://www.w3.org/1999/xhtml\" lang=\""<<LANGUAGE_FOR_HTML<<"\" xml:lang=\""<<LANGUAGE_FOR_HTML<<"\" dir=\"rtl\">\n";
	tos<<"  <head>\n";
	tos<<"    <title>"<<protect2(gt.rules.institutionName)<<"</title>\n";
	tos<<"    <meta http-equiv=\"Content-Type\" content=\"text/html;charset=utf-8\" />\n";
	if(TIMETABLE_HTML_LEVEL>=1){
		QString bar;
		if(INPUT_FILENAME_XML=="")
			bar="";
		else
			bar="_";
	
		QString cssfilename=s2+bar+STYLESHEET_STATISTICS;
		tos<<"    <link rel=\"stylesheet\" media=\"all\" href=\""<<cssfilename<<"\" type=\"text/css\" />\n";
	}
	if(TIMETABLE_HTML_LEVEL>=5){  // the following JavaScript code is pretty similar to an example of Les Richardson ( http://richtech.ca/openadmin/index.html )
		tos<<"    <meta http-equiv=\"Content-Script-Type\" content=\"text/javascript\" />\n";
		tos<<"    <script type=\"text/javascript\">\n";
		tos<<"      function highlight(classval) {\n";
		tos<<"        var spans = document.getElementsByTagName('span');\n";
		tos<<"        for(var i=0;spans.length>i;i++) {\n";
		tos<<"          if (spans[i].className == classval) {\n";
		tos<<"            spans[i].style.backgroundColor = 'lime';\n";
		tos<<"          } else {\n";
		tos<<"            spans[i].style.backgroundColor = 'white';\n";
		tos<<"          }\n";
		tos<<"        }\n";
		tos<<"      }\n";
		tos<<"    </script>\n";
	}
	tos<<"  </head>\n\n";

	tos<<"  <body>\n";
	tos<<"    <table border=\"1\">\n";	
	tos<<"      <caption>"<<protect2(gt.rules.institutionName)<<"</caption>\n";
	tos<<"      <thead>\n        <tr><td rowspan=\"2\"></td><th colspan=\""<<allTeachersNames.size()+1<<"\">"<<tr("Teachers - Students Matrix")<<"</th></tr>\n";
	tos<<"        <tr>\n          <!-- span -->\n";
	foreach(QString teachers, allTeachersNames){
		if(TIMETABLE_HTML_LEVEL>=2)
			tos<<"          <th class=\"xAxis\">";
		else
			tos<<"          <th>";
		tos<<protect2(teachers)<<"</th>\n";
	}
	if(TIMETABLE_HTML_LEVEL>=2)
		tos<<"          <th class=\"xAxis\">";
	else
		tos<<"          <th>";
	tos<<protect2(tr("Sum", "This means the sum of more values, the total"))<<"</th>\n";
	tos<<"        </tr>\n";
	tos<<"      </thead>\n";
	//*workaround. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
	//tos<<"      <tfoot><tr><td></td><td colspan=\""<<allTeachersNames.size()+1<<"\">"<<StatisticsExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr></tfoot>\n";
	tos<<"      <tbody>\n";
	
	QProgressDialog progress(NULL);
	progress.setWindowTitle(tr("Exporting statistics", "Title of a progress dialog"));
	progress.setLabelText(tr("Processing teachers with students...please wait"));
	progress.setRange(0, allStudentsNames.count());
	progress.setModal(true);
	
	int ttt=0;
	
	foreach(QString students, allStudentsNames){
		progress.setValue(ttt);
		//pqapplication->processEvents();
		if(progress.wasCanceled()){
			QMessageBox::warning(NULL, tr("FET warning"), tr("Canceled"));
			return false;
		}
		ttt++;
	
		QList<int> tmpStudents;
		QMultiHash<QString, int> tmpTeachers;
		tmpStudents.clear();
		tmpTeachers.clear();
		tmpStudents=studentsActivities.values(students);
		foreach(int aidx, tmpStudents){
			Activity* act=gt.rules.activitiesList.at(aidx);
			foreach(QString teacher, act->teachersNames){
				tmpTeachers.insert(teacher, aidx);
			}
		}
		tos<<"        <tr>\n";
		if(TIMETABLE_HTML_LEVEL>=2)
			tos<<"          <th class=\"yAxis\">";
		else
			tos<<"          <th>";
		tos<<protect2(students)<<"</th>\n";
		foreach(QString teachers, allTeachersNames){
			QList<int> tmpActivities;
			tmpActivities.clear();
			tmpActivities=tmpTeachers.values(teachers);
			if(tmpActivities.isEmpty()){
				switch(TIMETABLE_HTML_LEVEL){
					case 3 : ;
					case 4 : tos<<"          <td class=\"empty\"><span class=\"empty\">"<<protect2(STRING_EMPTY_SLOT_STATISTICS)<<"</span></td>\n"; break;
					case 5 : ;
					case 6 : tos<<"          <td class=\"empty\"><span class=\"empty\" onmouseover=\"highlight('empty')\">"<<protect2(STRING_EMPTY_SLOT_STATISTICS)<<"</span></td>\n"; break;
					default: tos<<"          <td>"<<protect2(STRING_EMPTY_SLOT_STATISTICS)<<"</td>\n";
				}
			} else {
				QMap<QString, int> tmpSubjectDuration;	//not QHash, because i need the correct order of the activities
				foreach(int tmpAct, tmpActivities){
					Activity* act=gt.rules.activitiesList[tmpAct];
					int tmpD=act->duration;
					QString tmpS;
					if(act->subjectName.size()>0||act->activityTagsNames.size()>0){
						if(act->subjectName.size()>0)
							switch(TIMETABLE_HTML_LEVEL){
								case 3 : tmpS+="<span class=\"subject\">"+protect2(act->subjectName)+"</span>"; break;
								case 4 : tmpS+="<span class=\"subject\"><span class=\"s_"+hashSubjectIDsStatistics.value(act->subjectName)+"\">"+protect2(act->subjectName)+"</span></span>"; break;
								case 5 : ;
								case 6 : tmpS+="<span class=\"subject\"><span class=\"s_"+hashSubjectIDsStatistics.value(act->subjectName)+"\" onmouseover=\"highlight('s_"+hashSubjectIDsStatistics.value(act->subjectName)+"')\">"+protect2(act->subjectName)+"</span></span>"; break;
								default: tmpS+=protect2(act->subjectName); break;
							}
						for(QStringList::Iterator atn=act->activityTagsNames.begin(); atn!=act->activityTagsNames.end(); atn++){
							switch(TIMETABLE_HTML_LEVEL){
								case 3 : tmpS+=" <span class=\"activitytag\">"+protect2(*atn)+"</span>"; break;
								case 4 : tmpS+=" <span class=\"activitytag\"><span class=\"at_"+hashActivityTagIDsStatistics.value(*atn)+"\">"+protect2(*atn)+"</span></span>"; break;
								case 5 : ;
								case 6 : tmpS+=" <span class=\"activitytag\"><span class=\"at_"+hashActivityTagIDsStatistics.value(*atn)+"\" onmouseover=\"highlight('at_"+hashActivityTagIDsStatistics.value(*atn)+"')\">"+protect2(*atn)+"</span></span>"; break;
								default: tmpS+=" "+protect2(*atn); break;
							}
							if(atn!=act->activityTagsNames.end()-1)
								tmpS+=", ";
						}
					} else
						tmpS=" ";
					tmpD+=tmpSubjectDuration.value(tmpS);
					tmpSubjectDuration.insert(tmpS, tmpD);
				}
				if(TIMETABLE_HTML_LEVEL>=1)
					tos<<"          <td><table class=\"detailed\">";
				else
					tos<<"          <td><table>";
				if(TIMETABLE_HTML_LEVEL>=3)
					tos<<"<tr class=\"duration line1\">";
				else	tos<<"<tr>";
				QMapIterator<QString, int> it(tmpSubjectDuration);
				while(it.hasNext()){
					it.next();
					if(TIMETABLE_HTML_LEVEL>=1)
						tos<<"<td class=\"detailed\">";
					else
						tos<<"<td>";
					tos<<it.value()<<"</td>";	
				}
				tos<<"</tr>";
				if(TIMETABLE_HTML_LEVEL>=3)
					tos<<"<tr class=\"subject line2\">";
				else	tos<<"<tr>";
				QMapIterator<QString, int> it2(tmpSubjectDuration);	//do it with the same iterator
				while(it2.hasNext()){
					it2.next();
					if(TIMETABLE_HTML_LEVEL>=1)
						tos<<"<td class=\"detailed\">";
					else
						tos<<"<td>";
					tos<<it2.key()<<"</td>";	
				}
				tos<<"</tr>";
				tos<<"</table></td>\n";
			}
		}
		tos<<"          <th>";
		tos<<QString::number(studentsTotalNumberOfHours.value(students));
		if(studentsTotalNumberOfHours.value(students)!=studentsTotalNumberOfHours2.value(students))
			tos<<"<br />("<<QString::number(studentsTotalNumberOfHours2.value(students))<<")";
		tos<<"</th>\n";
		tos<<"        </tr>\n";
	}
	
	progress.setValue(allStudentsNames.count());
	
	tos<<"        <tr>\n";
	if(TIMETABLE_HTML_LEVEL>=2)
		tos<<"          <th class=\"xAxis\">";
	else
		tos<<"          <th>";
	tos<<protect2(tr("Sum", "This means the sum of more values, the total"))<<"</th>\n";
	foreach(QString teachers, allTeachersNames){
		tos<<"          <th>"<<QString::number(teachersTotalNumberOfHours.value(teachers));
		if(teachersTotalNumberOfHours.value(teachers)!=teachersTotalNumberOfHours2.value(teachers))
			tos<<"<br />("<<QString::number(teachersTotalNumberOfHours2.value(teachers))<<")";
		tos<<"</th>\n";
	}
	tos<<"          <th></th>\n        </tr>\n";
	//workaround begin. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
	tos<<"      <tr class=\"foot\"><td></td><td colspan=\""<<allTeachersNames.size()+1<<"\">"<<StatisticsExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr>\n";
	//workaround end.
	tos<<"      </tbody>\n";
	tos<<"    </table>\n";
	tos<<"  </body>\n</html>\n\n";

	if(file.error()>0){
		QMessageBox::critical(NULL, tr("FET critical"),
		 StatisticsExport::tr("Writing %1 gave error code %2, which means saving is compromised. Please check your disk's free space.").arg(htmlfilename).arg(file.error()));
		return false;
	}
	file.close();
	return true;
}



bool StatisticsExport::exportStatisticsStudentsTeachers(QString saveTime){
	assert(gt.rules.initialized);// && gt.rules.internalStructureComputed);
	QString s2=INPUT_FILENAME_XML.right(INPUT_FILENAME_XML.length()-INPUT_FILENAME_XML.findRev(FILE_SEP)-1);	//TODO: remove s2, because to long filenames!

	if(s2.right(4)==".fet")
		s2=s2.left(s2.length()-4);
	//else if(INPUT_FILENAME_XML!="")
	//	cout<<"Minor problem - input file does not end in .fet extension - might be a problem when saving the timetables"<<" (file:"<<__FILE__<<", line:"<<__LINE__<<")"<<endl;

	QString bar;
	if(INPUT_FILENAME_XML=="")
		bar="";
	else
		bar="_";

	QString htmlfilename=PREFIX_STATISTICS+s2+bar+STUDENTS_TEACHERS_STATISTICS;

	QFile file(htmlfilename);
	if(!file.open(QIODevice::WriteOnly)){
		QMessageBox::critical(NULL, tr("FET critical"),
		 StatisticsExport::tr("Cannot open file %1 for writing. Please check your disk's free space. Saving of %1 aborted.").arg(htmlfilename));
		return false;
		assert(0);
	}
	QTextStream tos(&file);
	tos.setCodec("UTF-8");
	tos.setGenerateByteOrderMark(true);

	tos<<"<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\"\n";
	tos<<"  \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n\n";

	if(LANGUAGE_STYLE_RIGHT_TO_LEFT==false)
		tos<<"<html xmlns=\"http://www.w3.org/1999/xhtml\" lang=\""<<LANGUAGE_FOR_HTML<<"\" xml:lang=\""<<LANGUAGE_FOR_HTML<<"\">\n";
	else
		tos<<"<html xmlns=\"http://www.w3.org/1999/xhtml\" lang=\""<<LANGUAGE_FOR_HTML<<"\" xml:lang=\""<<LANGUAGE_FOR_HTML<<"\" dir=\"rtl\">\n";
	tos<<"  <head>\n";
	tos<<"    <title>"<<protect2(gt.rules.institutionName)<<"</title>\n";
	tos<<"    <meta http-equiv=\"Content-Type\" content=\"text/html;charset=utf-8\" />\n";
	if(TIMETABLE_HTML_LEVEL>=1){
		QString bar;
		if(INPUT_FILENAME_XML=="")
			bar="";
		else
			bar="_";
	
		QString cssfilename=s2+bar+STYLESHEET_STATISTICS;
		tos<<"    <link rel=\"stylesheet\" media=\"all\" href=\""<<cssfilename<<"\" type=\"text/css\" />\n";
	}
	if(TIMETABLE_HTML_LEVEL>=5){  // the following JavaScript code is pretty similar to an example of Les Richardson ( http://richtech.ca/openadmin/index.html )
		tos<<"    <meta http-equiv=\"Content-Script-Type\" content=\"text/javascript\" />\n";
		tos<<"    <script type=\"text/javascript\">\n";
		tos<<"      function highlight(classval) {\n";
		tos<<"        var spans = document.getElementsByTagName('span');\n";
		tos<<"        for(var i=0;spans.length>i;i++) {\n";
		tos<<"          if (spans[i].className == classval) {\n";
		tos<<"            spans[i].style.backgroundColor = 'lime';\n";
		tos<<"          } else {\n";
		tos<<"            spans[i].style.backgroundColor = 'white';\n";
		tos<<"          }\n";
		tos<<"        }\n";
		tos<<"      }\n";
		tos<<"    </script>\n";
	}
	tos<<"  </head>\n\n";

	tos<<"  <body>\n";
	tos<<"    <table border=\"1\">\n";	
	tos<<"      <caption>"<<protect2(gt.rules.institutionName)<<"</caption>\n";
	tos<<"      <thead>\n        <tr><td rowspan=\"2\"></td><th colspan=\""<<allStudentsNames.size()+1<<"\">"<<tr("Students -Teachers Matrix")<<"</th></tr>\n";
	tos<<"        <tr>\n          <!-- span -->\n";
	foreach(QString students, allStudentsNames){
		if(TIMETABLE_HTML_LEVEL>=2)
			tos<<"          <th class=\"xAxis\">";
		else
			tos<<"          <th>";
		tos<<protect2(students)<<"</th>\n";
	}
	if(TIMETABLE_HTML_LEVEL>=2)
		tos<<"          <th class=\"xAxis\">";
	else
		tos<<"          <th>";
	tos<<protect2(tr("Sum", "This means the sum of more values, the total"))<<"</th>\n";
	tos<<"        </tr>\n";
	tos<<"      </thead>\n";
	//workaround. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
	//tos<<"      <tfoot><tr><td></td><td colspan=\""<<numberOfStudentsNames+1<<"\">"<<StatisticsExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr></tfoot>\n";
	tos<<"      <tbody>\n";
	
	QProgressDialog progress(NULL);
	progress.setWindowTitle(tr("Exporting statistics", "Title of a progress dialog"));
	progress.setLabelText(tr("Processing students with teachers...please wait"));
	progress.setRange(0, allTeachersNames.count());
	progress.setModal(true);
	
	int ttt=0;
	
	foreach(QString teachers, allTeachersNames){
		progress.setValue(ttt);
		//pqapplication->processEvents();
		if(progress.wasCanceled()){
			QMessageBox::warning(NULL, tr("FET warning"), tr("Canceled"));
			return false;
		}
		ttt++;
	
		QList<int> tmpTeachers;
		QMultiHash<QString, int> tmpStudents;
		tmpTeachers.clear();
		tmpStudents.clear();
		tmpTeachers=teachersActivities.values(teachers);
		foreach(int aidx, tmpTeachers){
			Activity* act=gt.rules.activitiesList.at(aidx);
			foreach(QString students, act->studentsNames){
				tmpStudents.insert(students, aidx);
			}
		}
		tos<<"        <tr>\n";
		if(TIMETABLE_HTML_LEVEL>=2)
			tos<<"          <th class=\"yAxis\">";
		else
			tos<<"          <th>";
		tos<<protect2(teachers)<<"</th>\n";
		foreach(QString students, allStudentsNames){
			QList<int> tmpActivities;
			tmpActivities.clear();
			tmpActivities=tmpStudents.values(students);
			if(tmpActivities.isEmpty()){
				switch(TIMETABLE_HTML_LEVEL){
					case 3 : ;
					case 4 : tos<<"          <td class=\"empty\"><span class=\"empty\">"<<protect2(STRING_EMPTY_SLOT_STATISTICS)<<"</span></td>\n"; break;
					case 5 : ;
					case 6 : tos<<"          <td class=\"empty\"><span class=\"empty\" onmouseover=\"highlight('empty')\">"<<protect2(STRING_EMPTY_SLOT_STATISTICS)<<"</span></td>\n"; break;
					default: tos<<"          <td>"<<protect2(STRING_EMPTY_SLOT_STATISTICS)<<"</td>\n";
				}
			} else {
				QMap<QString, int> tmpSubjectDuration;	//not QHash, because i need the correct order of the activities
				foreach(int tmpAct, tmpActivities){
					Activity* act=gt.rules.activitiesList[tmpAct];
					int tmpD=act->duration;
					QString tmpS;
					if(act->subjectName.size()>0||act->activityTagsNames.size()>0){
						if(act->subjectName.size()>0)
							switch(TIMETABLE_HTML_LEVEL){
								case 3 : tmpS+="<span class=\"subject\">"+protect2(act->subjectName)+"</span>"; break;
								case 4 : tmpS+="<span class=\"subject\"><span class=\"s_"+hashSubjectIDsStatistics.value(act->subjectName)+"\">"+protect2(act->subjectName)+"</span></span>"; break;
								case 5 : ;
								case 6 : tmpS+="<span class=\"subject\"><span class=\"s_"+hashSubjectIDsStatistics.value(act->subjectName)+"\" onmouseover=\"highlight('s_"+hashSubjectIDsStatistics.value(act->subjectName)+"')\">"+protect2(act->subjectName)+"</span></span>"; break;
								default: tmpS+=protect2(act->subjectName); break;
							}
						for(QStringList::Iterator atn=act->activityTagsNames.begin(); atn!=act->activityTagsNames.end(); atn++){
							switch(TIMETABLE_HTML_LEVEL){
								case 3 : tmpS+=" <span class=\"activitytag\">"+protect2(*atn)+"</span>"; break;
								case 4 : tmpS+=" <span class=\"activitytag\"><span class=\"at_"+hashActivityTagIDsStatistics.value(*atn)+"\">"+protect2(*atn)+"</span></span>"; break;
								case 5 : ;
								case 6 : tmpS+=" <span class=\"activitytag\"><span class=\"at_"+hashActivityTagIDsStatistics.value(*atn)+"\" onmouseover=\"highlight('at_"+hashActivityTagIDsStatistics.value(*atn)+"')\">"+protect2(*atn)+"</span></span>"; break;
								default: tmpS+=" "+protect2(*atn); break;
							}
							if(atn!=act->activityTagsNames.end()-1)
								tmpS+=", ";
						}
					} else
						tmpS=" ";
					tmpD+=tmpSubjectDuration.value(tmpS);
					tmpSubjectDuration.insert(tmpS, tmpD);
				}
				if(TIMETABLE_HTML_LEVEL>=1)
					tos<<"          <td><table class=\"detailed\">";
				else
					tos<<"          <td><table>";
				if(TIMETABLE_HTML_LEVEL>=3)
					tos<<"<tr class=\"duration line1\">";
				else	tos<<"<tr>";
				QMapIterator<QString, int> it(tmpSubjectDuration);
				while(it.hasNext()){
					it.next();
					if(TIMETABLE_HTML_LEVEL>=1)
						tos<<"<td class=\"detailed\">";
					else
						tos<<"<td>";
					tos<<it.value()<<"</td>";	
				}
				tos<<"</tr>";
				if(TIMETABLE_HTML_LEVEL>=3)
					tos<<"<tr class=\"subject line2\">";
				else	tos<<"<tr>";
				QMapIterator<QString, int> it2(tmpSubjectDuration);	//do it with the same iterator
				while(it2.hasNext()){
					it2.next();
					if(TIMETABLE_HTML_LEVEL>=1)
						tos<<"<td class=\"detailed\">";
					else
						tos<<"<td>";
					tos<<it2.key()<<"</td>";	
				}
				tos<<"</tr>";
				tos<<"</table></td>\n";
			}
		}
		tos<<"          <th>";
		tos<<QString::number(teachersTotalNumberOfHours.value(teachers));
		if(teachersTotalNumberOfHours.value(teachers)!=teachersTotalNumberOfHours2.value(teachers))
			tos<<"<br />("<<QString::number(teachersTotalNumberOfHours2.value(teachers))<<")";
		tos<<"</th>\n";
		tos<<"        </tr>\n";
	}
	
	progress.setValue(allTeachersNames.count());
	
	tos<<"        <tr>\n";
	if(TIMETABLE_HTML_LEVEL>=2)
		tos<<"          <th class=\"xAxis\">";
	else
		tos<<"          <th>";
	tos<<protect2(tr("Sum", "This means the sum of more values, the total"))<<"</th>\n";
	foreach(QString students, allStudentsNames){
		tos<<"          <th>"<<QString::number(studentsTotalNumberOfHours.value(students));
		if(studentsTotalNumberOfHours.value(students)!=studentsTotalNumberOfHours2.value(students))
			tos<<"<br />("<<QString::number(studentsTotalNumberOfHours2.value(students))<<")";
		tos<<"</th>\n";
	}
	tos<<"          <th></th>\n        </tr>\n";
	//workaround begin. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
	tos<<"      <tr class=\"foot\"><td></td><td colspan=\""<<allStudentsNames.size()+1<<"\">"<<StatisticsExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr>\n";
	//workaround end.
	tos<<"      </tbody>\n";
	tos<<"    </table>\n";
	tos<<"  </body>\n</html>\n\n";

	if(file.error()>0){
		QMessageBox::critical(NULL, tr("FET critical"),
		 StatisticsExport::tr("Writing %1 gave error code %2, which means saving is compromised. Please check your disk's free space.").arg(htmlfilename).arg(file.error()));
		return false;
	}
	file.close();
	return true;
}



bool StatisticsExport::exportStatisticsSubjectsStudents(QString saveTime){
	assert(gt.rules.initialized);// && gt.rules.internalStructureComputed);
	QString s2=INPUT_FILENAME_XML.right(INPUT_FILENAME_XML.length()-INPUT_FILENAME_XML.findRev(FILE_SEP)-1);	//TODO: remove s2, because to long filenames!

	if(s2.right(4)==".fet")
		s2=s2.left(s2.length()-4);
	//else if(INPUT_FILENAME_XML!="")
	//	cout<<"Minor problem - input file does not end in .fet extension - might be a problem when saving the timetables"<<" (file:"<<__FILE__<<", line:"<<__LINE__<<")"<<endl;

	QString bar;
	if(INPUT_FILENAME_XML=="")
		bar="";
	else
		bar="_";

	QString htmlfilename=PREFIX_STATISTICS+s2+bar+SUBJECTS_STUDENTS_STATISTICS;

	QFile file(htmlfilename);
	if(!file.open(QIODevice::WriteOnly)){
		QMessageBox::critical(NULL, tr("FET critical"),
		 StatisticsExport::tr("Cannot open file %1 for writing. Please check your disk's free space. Saving of %1 aborted.").arg(htmlfilename));
		return false;
		assert(0);
	}
	QTextStream tos(&file);
	tos.setCodec("UTF-8");
	tos.setGenerateByteOrderMark(true);

	tos<<"<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\"\n";
	tos<<"  \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n\n";

	if(LANGUAGE_STYLE_RIGHT_TO_LEFT==false)
		tos<<"<html xmlns=\"http://www.w3.org/1999/xhtml\" lang=\""<<LANGUAGE_FOR_HTML<<"\" xml:lang=\""<<LANGUAGE_FOR_HTML<<"\">\n";
	else
		tos<<"<html xmlns=\"http://www.w3.org/1999/xhtml\" lang=\""<<LANGUAGE_FOR_HTML<<"\" xml:lang=\""<<LANGUAGE_FOR_HTML<<"\" dir=\"rtl\">\n";
	tos<<"  <head>\n";
	tos<<"    <title>"<<protect2(gt.rules.institutionName)<<"</title>\n";
	tos<<"    <meta http-equiv=\"Content-Type\" content=\"text/html;charset=utf-8\" />\n";
	if(TIMETABLE_HTML_LEVEL>=1){
		QString bar;
		if(INPUT_FILENAME_XML=="")
			bar="";
		else
			bar="_";
	
		QString cssfilename=s2+bar+STYLESHEET_STATISTICS;
		tos<<"    <link rel=\"stylesheet\" media=\"all\" href=\""<<cssfilename<<"\" type=\"text/css\" />\n";
	}
	if(TIMETABLE_HTML_LEVEL>=5){  // the following JavaScript code is pretty similar to an example of Les Richardson ( http://richtech.ca/openadmin/index.html )
		tos<<"    <meta http-equiv=\"Content-Script-Type\" content=\"text/javascript\" />\n";
		tos<<"    <script type=\"text/javascript\">\n";
		tos<<"      function highlight(classval) {\n";
		tos<<"        var spans = document.getElementsByTagName('span');\n";
		tos<<"        for(var i=0;spans.length>i;i++) {\n";
		tos<<"          if (spans[i].className == classval) {\n";
		tos<<"            spans[i].style.backgroundColor = 'lime';\n";
		tos<<"          } else {\n";
		tos<<"            spans[i].style.backgroundColor = 'white';\n";
		tos<<"          }\n";
		tos<<"        }\n";
		tos<<"      }\n";
		tos<<"    </script>\n";
	}
	tos<<"  </head>\n\n";

	tos<<"  <body>\n";
	tos<<"    <table border=\"1\">\n";	
	tos<<"      <caption>"<<protect2(gt.rules.institutionName)<<"</caption>\n";
	tos<<"      <thead>\n        <tr><td rowspan=\"2\"></td><th colspan=\""<<allSubjectsNames.size()+1<<"\">"<<tr("Subjects - Students Matrix")<<"</th></tr>\n";
	tos<<"        <tr>\n          <!-- span -->\n";
	foreach(QString subjects, allSubjectsNames){
		if(TIMETABLE_HTML_LEVEL>=2)
			tos<<"          <th class=\"xAxis\">";
		else
			tos<<"          <th>";
		tos<<protect2(subjects)<<"</th>\n";
	}
	if(TIMETABLE_HTML_LEVEL>=2)
		tos<<"          <th class=\"xAxis\">";
	else
		tos<<"          <th>";
	tos<<protect2(tr("Sum", "This means the sum of more values, the total"))<<"</th>\n";
	tos<<"        </tr>\n";
	tos<<"      </thead>\n";
	//workaround. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
	//tos<<"      <tfoot><tr><td></td><td colspan=\""<<allTeachersNames.size()+1<<"\">"<<StatisticsExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr></tfoot>\n";
	tos<<"      <tbody>\n";
	
	QProgressDialog progress(NULL);
	progress.setWindowTitle(tr("Exporting statistics", "Title of a progress dialog"));
	progress.setLabelText(tr("Processing subjects with students...please wait"));
	progress.setRange(0, allStudentsNames.count());
	progress.setModal(true);
	
	int ttt=0;
	
	foreach(QString students, allStudentsNames){
		progress.setValue(ttt);
		//pqapplication->processEvents();
		if(progress.wasCanceled()){
			QMessageBox::warning(NULL, tr("FET warning"), tr("Canceled"));
			return false;
		}
		ttt++;
	
		QList<int> tmpStudents;
		QMultiHash<QString, int> tmpSubjects;
		tmpStudents.clear();
		tmpSubjects.clear();
		tmpStudents=studentsActivities.values(students);
		foreach(int aidx, tmpStudents){
			Activity* act=gt.rules.activitiesList.at(aidx);
			tmpSubjects.insert(act->subjectName, aidx);
		}
		tos<<"        <tr>\n";
		if(TIMETABLE_HTML_LEVEL>=2)
			tos<<"          <th class=\"yAxis\">";
		else
			tos<<"          <th>";
		tos<<protect2(students)<<"</th>\n";
		foreach(QString subjects, allSubjectsNames){
			QList<int> tmpActivities;
			tmpActivities.clear();
			tmpActivities=tmpSubjects.values(subjects);
			if(tmpActivities.isEmpty()){
				switch(TIMETABLE_HTML_LEVEL){
					case 3 : ;
					case 4 : tos<<"          <td class=\"empty\"><span class=\"empty\">"<<protect2(STRING_EMPTY_SLOT_STATISTICS)<<"</span></td>\n"; break;
					case 5 : ;
					case 6 : tos<<"          <td class=\"empty\"><span class=\"empty\" onmouseover=\"highlight('empty')\">"<<protect2(STRING_EMPTY_SLOT_STATISTICS)<<"</span></td>\n"; break;
					default: tos<<"          <td>"<<protect2(STRING_EMPTY_SLOT_STATISTICS)<<"</td>\n";
				}
			} else {
				QMap<QString, int> tmpTeacherDuration;	//not QHash, because i need the correct order of the activities
				foreach(int tmpAct, tmpActivities){
					Activity* act=gt.rules.activitiesList[tmpAct];
					int tmpD=act->duration;
					QString tmpT;
					if(act->teachersNames.size()>0){
						for(QStringList::Iterator it=act->teachersNames.begin(); it!=act->teachersNames.end(); it++){
							switch(TIMETABLE_HTML_LEVEL){
								case 4 : tmpT+="<span class=\"t_"+hashTeacherIDsStatistics.value(*it)+"\">"+protect2(*it)+"</span>"; break;
								case 5 : ;
								case 6 : tmpT+="<span class=\"t_"+hashTeacherIDsStatistics.value(*it)+"\" onmouseover=\"highlight('t_"+hashTeacherIDsStatistics.value(*it)+"')\">"+protect2(*it)+"</span>"; break;
								default: tmpT+=protect2(*it); break;
							}
							if(it!=act->teachersNames.end()-1)
								tmpT+=", ";
						}
					} else
						tmpT=" ";
					tmpD+=tmpTeacherDuration.value(tmpT);
					tmpTeacherDuration.insert(tmpT, tmpD);
				}
				if(TIMETABLE_HTML_LEVEL>=1)
					tos<<"          <td><table class=\"detailed\">";
				else
					tos<<"          <td><table>";
				if(TIMETABLE_HTML_LEVEL>=3)
					tos<<"<tr class=\"duration line1\">";
				else	tos<<"<tr>";
				QMapIterator<QString, int> it(tmpTeacherDuration);
				while(it.hasNext()){
					it.next();
					if(TIMETABLE_HTML_LEVEL>=1)
						tos<<"<td class=\"detailed\">";
					else
						tos<<"<td>";
					tos<<it.value()<<"</td>";	
				}
				tos<<"</tr>";
				if(TIMETABLE_HTML_LEVEL>=3)
					tos<<"<tr class=\"teacher line2\">";
				else	tos<<"<tr>";
				QMapIterator<QString, int> it2(tmpTeacherDuration);	//do it with the same iterator
				while(it2.hasNext()){
					it2.next();
					if(TIMETABLE_HTML_LEVEL>=1)
						tos<<"<td class=\"detailed\">";
					else
						tos<<"<td>";
					tos<<it2.key()<<"</td>";	
				}
				tos<<"</tr>";
				tos<<"</table></td>\n";
			}
		}
		tos<<"          <th>";
		tos<<QString::number(studentsTotalNumberOfHours.value(students));
		if(studentsTotalNumberOfHours.value(students)!=studentsTotalNumberOfHours2.value(students))
			tos<<"<br />("<<QString::number(studentsTotalNumberOfHours2.value(students))<<")";
		tos<<"</th>\n";
		tos<<"        </tr>\n";
	}
	
	progress.setValue(allStudentsNames.count());

	tos<<"        <tr>\n";
	if(TIMETABLE_HTML_LEVEL>=2)
		tos<<"          <th class=\"xAxis\">";
	else
		tos<<"          <th>";
	tos<<protect2(tr("Sum", "This means the sum of more values, the total"))<<"</th>\n";
	foreach(QString subjects, allSubjectsNames){
		tos<<"          <th>"<<QString::number(subjectsTotalNumberOfHours.value(subjects));
		if(subjectsTotalNumberOfHours.value(subjects)!=subjectsTotalNumberOfHours4.value(subjects))
			tos<<"<br />("<<QString::number(subjectsTotalNumberOfHours4.value(subjects))<<")";
		tos<<"</th>\n";
	}
	tos<<"          <th></th>\n        </tr>\n";
	//workaround begin. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
	tos<<"      <tr class=\"foot\"><td></td><td colspan=\""<<allTeachersNames.size()+1<<"\">"<<StatisticsExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr>\n";
	//workaround end.
	tos<<"      </tbody>\n";
	tos<<"    </table>\n";
	tos<<"  </body>\n</html>\n\n";

	if(file.error()>0){
		QMessageBox::critical(NULL, tr("FET critical"),
		 StatisticsExport::tr("Writing %1 gave error code %2, which means saving is compromised. Please check your disk's free space.").arg(htmlfilename).arg(file.error()));
		return false;
	}
	file.close();
	return true;
}



bool StatisticsExport::exportStatisticsStudentsSubjects(QString saveTime){
	assert(gt.rules.initialized);// && gt.rules.internalStructureComputed);
	QString s2=INPUT_FILENAME_XML.right(INPUT_FILENAME_XML.length()-INPUT_FILENAME_XML.findRev(FILE_SEP)-1);	//TODO: remove s2, because to long filenames!

	if(s2.right(4)==".fet")
		s2=s2.left(s2.length()-4);
	//else if(INPUT_FILENAME_XML!="")
	//	cout<<"Minor problem - input file does not end in .fet extension - might be a problem when saving the timetables"<<" (file:"<<__FILE__<<", line:"<<__LINE__<<")"<<endl;

	QString bar;
	if(INPUT_FILENAME_XML=="")
		bar="";
	else
		bar="_";

	QString htmlfilename=PREFIX_STATISTICS+s2+bar+STUDENTS_SUBJECTS_STATISTICS;

	QFile file(htmlfilename);
	if(!file.open(QIODevice::WriteOnly)){
		QMessageBox::critical(NULL, tr("FET critical"),
		 StatisticsExport::tr("Cannot open file %1 for writing. Please check your disk's free space. Saving of %1 aborted.").arg(htmlfilename));
		return false;
		assert(0);
	}
	QTextStream tos(&file);
	tos.setCodec("UTF-8");
	tos.setGenerateByteOrderMark(true);
	tos<<"<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\"\n";
	tos<<"  \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n\n";

	if(LANGUAGE_STYLE_RIGHT_TO_LEFT==false)
		tos<<"<html xmlns=\"http://www.w3.org/1999/xhtml\" lang=\""<<LANGUAGE_FOR_HTML<<"\" xml:lang=\""<<LANGUAGE_FOR_HTML<<"\">\n";
	else
		tos<<"<html xmlns=\"http://www.w3.org/1999/xhtml\" lang=\""<<LANGUAGE_FOR_HTML<<"\" xml:lang=\""<<LANGUAGE_FOR_HTML<<"\" dir=\"rtl\">\n";
	tos<<"  <head>\n";
	tos<<"    <title>"<<protect2(gt.rules.institutionName)<<"</title>\n";
	tos<<"    <meta http-equiv=\"Content-Type\" content=\"text/html;charset=utf-8\" />\n";
	if(TIMETABLE_HTML_LEVEL>=1){
		QString bar;
		if(INPUT_FILENAME_XML=="")
			bar="";
		else
			bar="_";
	
		QString cssfilename=s2+bar+STYLESHEET_STATISTICS;
		tos<<"    <link rel=\"stylesheet\" media=\"all\" href=\""<<cssfilename<<"\" type=\"text/css\" />\n";
	}
	if(TIMETABLE_HTML_LEVEL>=5){  // the following JavaScript code is pretty similar to an example of Les Richardson ( http://richtech.ca/openadmin/index.html )
		tos<<"    <meta http-equiv=\"Content-Script-Type\" content=\"text/javascript\" />\n";
		tos<<"    <script type=\"text/javascript\">\n";
		tos<<"      function highlight(classval) {\n";
		tos<<"        var spans = document.getElementsByTagName('span');\n";
		tos<<"        for(var i=0;spans.length>i;i++) {\n";
		tos<<"          if (spans[i].className == classval) {\n";
		tos<<"            spans[i].style.backgroundColor = 'lime';\n";
		tos<<"          } else {\n";
		tos<<"            spans[i].style.backgroundColor = 'white';\n";
		tos<<"          }\n";
		tos<<"        }\n";
		tos<<"      }\n";
		tos<<"    </script>\n";
	}
	tos<<"  </head>\n\n";

	tos<<"  <body>\n";
	tos<<"    <table border=\"1\">\n";	
	tos<<"      <caption>"<<protect2(gt.rules.institutionName)<<"</caption>\n";
	tos<<"      <thead>\n        <tr><td rowspan=\"2\"></td><th colspan=\""<<allStudentsNames.size()+1<<"\">"<<tr("Students -Subjects Matrix")<<"</th></tr>\n";
	tos<<"        <tr>\n          <!-- span -->\n";

	foreach(QString students, allStudentsNames){
		if(TIMETABLE_HTML_LEVEL>=2)
			tos<<"          <th class=\"xAxis\">";
		else
			tos<<"          <th>";
		tos<<protect2(students)<<"</th>\n";
	}
	if(TIMETABLE_HTML_LEVEL>=2)
		tos<<"          <th class=\"xAxis\">";
	else
		tos<<"          <th>";
	tos<<protect2(tr("Sum", "This means the sum of more values, the total"))<<"</th>\n";
	tos<<"        </tr>\n";
	tos<<"      </thead>\n";
	//workaround. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
	//tos<<"      <tfoot><tr><td></td><td colspan=\""<<numberOfStudentsNames+1<<"\">"<<StatisticsExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr></tfoot>\n";
	tos<<"      <tbody>\n";
	
	QProgressDialog progress(NULL);
	progress.setWindowTitle(tr("Exporting statistics", "Title of a progress dialog"));
	progress.setLabelText(tr("Processing students with subjects...please wait"));
	progress.setRange(0, allSubjectsNames.count());
	progress.setModal(true);
	
	int ttt=0;
	
	foreach(QString subjects, allSubjectsNames){
		progress.setValue(ttt);
		//pqapplication->processEvents();
		if(progress.wasCanceled()){
			QMessageBox::warning(NULL, tr("FET warning"), tr("Canceled"));
			return false;
		}
		ttt++;
	
		QList<int> tmpSubjects;
		QMultiHash<QString, int> tmpStudents;
		tmpSubjects.clear();
		tmpStudents.clear();
		tmpSubjects=subjectsActivities.values(subjects);
		foreach(int aidx, tmpSubjects){
			Activity* act=gt.rules.activitiesList.at(aidx);
			foreach(QString students, act->studentsNames){
				tmpStudents.insert(students, aidx);
			}
		}
		tos<<"        <tr>\n";
		if(TIMETABLE_HTML_LEVEL>=2)
			tos<<"          <th class=\"yAxis\">";
		else
			tos<<"          <th>";
		tos<<protect2(subjects)<<"</th>\n";
		foreach(QString students, allStudentsNames){
			QList<int> tmpActivities;
			tmpActivities.clear();
			tmpActivities=tmpStudents.values(students);
			if(tmpActivities.isEmpty()){
				switch(TIMETABLE_HTML_LEVEL){
					case 3 : ;
					case 4 : tos<<"          <td class=\"empty\"><span class=\"empty\">"<<protect2(STRING_EMPTY_SLOT_STATISTICS)<<"</span></td>\n"; break;
					case 5 : ;
					case 6 : tos<<"          <td class=\"empty\"><span class=\"empty\" onmouseover=\"highlight('empty')\">"<<protect2(STRING_EMPTY_SLOT_STATISTICS)<<"</span></td>\n"; break;
					default: tos<<"          <td>"<<protect2(STRING_EMPTY_SLOT_STATISTICS)<<"</td>\n";
				}
			} else {
				QMap<QString, int> tmpTeacherDuration;	//not QHash, because i need the correct order of the activities
				foreach(int tmpAct, tmpActivities){
					Activity* act=gt.rules.activitiesList[tmpAct];
					int tmpD=act->duration;
					QString tmpT;
					if(act->teachersNames.size()>0){
						for(QStringList::Iterator it=act->teachersNames.begin(); it!=act->teachersNames.end(); it++){
							switch(TIMETABLE_HTML_LEVEL){
								case 4 : tmpT+="<span class=\"t_"+hashTeacherIDsStatistics.value(*it)+"\">"+protect2(*it)+"</span>"; break;
								case 5 : ;
								case 6 : tmpT+="<span class=\"t_"+hashTeacherIDsStatistics.value(*it)+"\" onmouseover=\"highlight('t_"+hashTeacherIDsStatistics.value(*it)+"')\">"+protect2(*it)+"</span>"; break;
								default: tmpT+=protect2(*it); break;
							}
							if(it!=act->teachersNames.end()-1)
								tmpT+=", ";
						}
					} else
						tmpT=" ";
					tmpD+=tmpTeacherDuration.value(tmpT);
					tmpTeacherDuration.insert(tmpT, tmpD);
				}
				if(TIMETABLE_HTML_LEVEL>=1)
					tos<<"          <td><table class=\"detailed\">";
				else
					tos<<"          <td><table>";
				if(TIMETABLE_HTML_LEVEL>=3)
					tos<<"<tr class=\"duration line1\">";
				else	tos<<"<tr>";
				QMapIterator<QString, int> it(tmpTeacherDuration);
				while(it.hasNext()){
					it.next();
					if(TIMETABLE_HTML_LEVEL>=1)
						tos<<"<td class=\"detailed\">";
					else
						tos<<"<td>";
					tos<<it.value()<<"</td>";	
				}
				tos<<"</tr>";
				if(TIMETABLE_HTML_LEVEL>=3)
					tos<<"<tr class=\"teacher line2\">";
				else	tos<<"<tr>";
				QMapIterator<QString, int> it2(tmpTeacherDuration);	//do it with the same iterator
				while(it2.hasNext()){
					it2.next();
					if(TIMETABLE_HTML_LEVEL>=1)
						tos<<"<td class=\"detailed\">";
					else
						tos<<"<td>";
					tos<<it2.key()<<"</td>";	
				}
				tos<<"</tr>";
				tos<<"</table></td>\n";
			}
		}
		tos<<"          <th>";
		tos<<QString::number(subjectsTotalNumberOfHours.value(subjects));
		if(subjectsTotalNumberOfHours.value(subjects)!=subjectsTotalNumberOfHours4.value(subjects))
			tos<<"<br />("<<QString::number(subjectsTotalNumberOfHours4.value(subjects))<<")";
		tos<<"</th>\n";
		tos<<"        </tr>\n";
	}
	
	progress.setValue(allSubjectsNames.count());

	tos<<"        <tr>\n";
	if(TIMETABLE_HTML_LEVEL>=2)
		tos<<"          <th class=\"xAxis\">";
	else
		tos<<"          <th>";
	tos<<protect2(tr("Sum", "This means the sum of more values, the total"))<<"</th>\n";
	foreach(QString students, allStudentsNames){
		tos<<"          <th>"<<QString::number(studentsTotalNumberOfHours.value(students));
		if(studentsTotalNumberOfHours.value(students)!=studentsTotalNumberOfHours2.value(students))
			tos<<"<br />("<<QString::number(studentsTotalNumberOfHours2.value(students))<<")";
		tos<<"</th>\n";	
	}
	tos<<"          <th></th>\n        </tr>\n";
	//workaround begin. compare http://www.openoffice.org/issues/show_bug.cgi?id=82600
	tos<<"      <tr class=\"foot\"><td></td><td colspan=\""<<allStudentsNames.size()+1<<"\">"<<StatisticsExport::tr("Timetable generated with FET %1 on %2", "%1 is FET version, %2 is the date and time of generation").arg(FET_VERSION).arg(saveTime)<<"</td></tr>\n";
	//workaround end.
	tos<<"      </tbody>\n";
	tos<<"    </table>\n";
	tos<<"  </body>\n</html>\n\n";

	if(file.error()>0){
		QMessageBox::critical(NULL, tr("FET critical"),
		 StatisticsExport::tr("Writing %1 gave error code %2, which means saving is compromised. Please check your disk's free space.").arg(htmlfilename).arg(file.error()));
		return false;
	}
	file.close();
	return true;
}
