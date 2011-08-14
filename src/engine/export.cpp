/*
File export.cpp
*/

/***************************************************************************
                                FET
                          -------------------
   copyright            : (C) by Lalescu Liviu
    email                : Please see http://lalescu.ro/liviu/ for details about contacting Liviu Lalescu (in particular, you can find here the e-mail address)
 ***************************************************************************
                          export.cpp  -  description
                             -------------------
    begin                : Mar 2008
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

//TODO: convert all export funtions like this: tosExport<<qPrintable(QString::number(actiNext->duration))
//TODO: protect export strings. textquote must be dubbled
//TODO: count skipped min day constraints?
//TODO: add cancel button
//TODO: REMOVE OR COMMENT OUT qPrintable, just output QString::number

//#include "centerwidgetonscreen.h"
class QWidget;
void centerWidgetOnScreen(QWidget* widget);

#include "timetable_defs.h"		//needed, because of QString s2=INPUT_FILENAME_XML.right(INPUT_FILENAME_XML.length()-INPUT_FILENAME_XML.findRev(FILE_SEP)-1);
#include "export.h"
#include "solution.h"
#include <QtGui>
#include <QHash>
#include <QSet>

extern Timetable gt;
extern Solution best_solution;
extern bool teachers_schedule_ready;
extern bool students_schedule_ready;
extern bool rooms_schedule_ready;

const char CSVActivities[]="activities.csv";
const char CSVActivitiesStatistic[]="statistics_activities.csv";
const char CSVActivityTags[]="activity_tags.csv";
const char CSVRoomsAndBuildings[]="rooms_and_buildings.csv";
const char CSVSubjects[]="subjects.csv";
const char CSVTeachers[]="teachers.csv";
const char CSVStudents[]="students.csv";
const char CSVTimetable[]="timetable.csv";
QString DIRECTORY_CSV;
QString PREFIX_CSV;

#include <iostream>
using namespace std;


Export::Export()
{
}

Export::~Export()
{
}


bool Export::okToWrite(const QString& file, QMessageBox::StandardButton& msgBoxButton)
{
	if(QFile::exists(file)){
		if(msgBoxButton==QMessageBox::YesToAll){
			return true;
		}
		else if(msgBoxButton==QMessageBox::NoToAll){
			return false;
		}
		else if(msgBoxButton==QMessageBox::No ||
		 msgBoxButton==QMessageBox::Yes ||
		 msgBoxButton==QMessageBox::NoButton){
			msgBoxButton=QMessageBox::warning(NULL, tr("FET warning"),
			 tr("File %1 exists - are you sure you want to overwrite existing file?")
			 .arg(file),
			 QMessageBox::Yes|QMessageBox::No|QMessageBox::YesToAll|QMessageBox::NoToAll,
			 QMessageBox::Yes
			 );
			if(msgBoxButton==QMessageBox::No || msgBoxButton==QMessageBox::NoToAll)
				return false;
			else
				return true;
		}
		else{
			assert(0);
			return false;
		}
	}
	else
		return true;
}


void Export::exportCSV(){
	QString fieldSeparator=",";
	QString textquote="\"";
	QString setSeparator="+";
	bool head=true;
	bool ok=true;

	DIRECTORY_CSV=OUTPUT_DIR+FILE_SEP+"csv";


	QString s2;
	if(INPUT_FILENAME_XML=="")
		s2="unnamed";
	else{
		s2=INPUT_FILENAME_XML.right(INPUT_FILENAME_XML.length()-INPUT_FILENAME_XML.findRev(FILE_SEP)-1);	//TODO: remove s2, because to long filenames!

		if(s2.right(4)==".fet")
			s2=s2.left(s2.length()-4);
		else if(INPUT_FILENAME_XML!="")
			cout<<"Minor problem - input file does not end in .fet extension - might be a problem when saving the timetables"<<" (file:"<<__FILE__<<", line:"<<__LINE__<<")"<<endl;
	}
	DIRECTORY_CSV.append(FILE_SEP);
	DIRECTORY_CSV.append(s2);
	

	PREFIX_CSV=DIRECTORY_CSV+FILE_SEP;

	QDir dir;
	if(!dir.exists(OUTPUT_DIR))
		dir.mkpath(OUTPUT_DIR);
	if(!dir.exists(DIRECTORY_CSV))
		dir.mkpath(DIRECTORY_CSV);

	ok=selectSeparatorAndTextquote(textquote, fieldSeparator, head);
	QString lastWarnings;
	if(!ok){
		lastWarnings.insert(0,Export::tr("Export aborted")+"\n");
	} else {
		bool okat, okr, oks, okt, okst, okact, okacts, oktim;

		QMessageBox::StandardButton msgBoxButton=QMessageBox::NoButton;

		okat=exportCSVActivityTags(lastWarnings, textquote, head, setSeparator, msgBoxButton);
		okr=exportCSVRoomsAndBuildings(lastWarnings, textquote, fieldSeparator, head, msgBoxButton);
		oks=exportCSVSubjects(lastWarnings, textquote, head, msgBoxButton);
		okt=exportCSVTeachers(lastWarnings, textquote, head, setSeparator, msgBoxButton);
		okst=exportCSVStudents(lastWarnings, textquote, fieldSeparator, head, setSeparator, msgBoxButton);
		okact=exportCSVActivities(lastWarnings, textquote, fieldSeparator, head, msgBoxButton);
		okacts=exportCSVActivitiesStatistic(lastWarnings, textquote, fieldSeparator, head, msgBoxButton);
		oktim=exportCSVTimetable(lastWarnings, textquote, fieldSeparator, head, msgBoxButton);
		
		ok=okat && okr && oks && okt && okst && okact && okacts && oktim;
			
		lastWarnings.insert(0,Export::tr("CSV files were exported to directory %1.").arg(QDir::toNativeSeparators(DIRECTORY_CSV))+"\n");
		if(ok)
			lastWarnings.insert(0,Export::tr("Exported complete")+"\n");
		else
			lastWarnings.insert(0,Export::tr("Export incomplete")+"\n");
	}

	LastWarningsDialogE lwd(lastWarnings);
	lwd.setWindowFlags(lwd.windowFlags() | Qt::WindowMinMaxButtonsHint);
	//lwd.show();
	int w=lwd.sizeHint().width();
	int h=lwd.sizeHint().height();
	lwd.setGeometry(0,0,w,h);
	centerWidgetOnScreen(&lwd);

	ok=lwd.exec();
}



QString Export::protectCSV(const QString& str){
	QString p=str;
	p.replace("\"", "\"\"");
	return p;
}



bool Export::checkSetSeparator(const QString& str, const QString setSeparator){
	if(str.contains(setSeparator))
		return false;
	return true;
}

bool Export::isActivityNotManualyEdited(const int activityIndex, bool& diffTeachers, bool& diffSubject, bool& diffActivityTags, bool& diffStudents, bool& diffCompNStud, bool& diffNStud, bool& diffActive){ //similar to ActivitiesForm::modifyActivity() by Liviu Lalescu, but added diffActive
	diffTeachers=diffSubject=diffActivityTags=diffStudents=diffCompNStud=diffNStud=diffActive=false;

	assert(activityIndex>=0);
	assert(activityIndex<gt.rules.activitiesList.size());

	Activity* act=gt.rules.activitiesList[activityIndex];
	assert(act!=NULL);
	
	QStringList teachers=act->teachersNames;
	QString subject=act->subjectName;
	QStringList activityTags=act->activityTagsNames;
	QStringList students=act->studentsNames;
	
	int nTotalStudents=act->nTotalStudents;
	
	bool computeNTotalStudents=act->computeNTotalStudents;
	bool active=act->active;

	if(act->isSplit()){
		for(int i=activityIndex; i<gt.rules.activitiesList.size(); i++){	//possible speed improvement: not i=0. do i=act->activityGroupId
			Activity* act2=gt.rules.activitiesList[i];			//possible speed improvement: if(act2->activityGroupId changed) break;
			if(act2->activityGroupId!=0 && act2->activityGroupId==act->activityGroupId){
				if(teachers!=act2->teachersNames){
					//return false;
					diffTeachers=true;
				}
				if(subject!=act2->subjectName){
					//return false;
					diffSubject=true;
				}
				if(activityTags!=act2->activityTagsNames){
					diffActivityTags=true;
					//return false;
				}
				if(students!=act2->studentsNames){
					diffStudents=true;
					//return false;
				}
				if( /* !computeNTotalStudents && !act2->computeNTotalStudents && */ nTotalStudents!=act2->nTotalStudents){
					diffNStud=true;
					//return false;
				}
				if(computeNTotalStudents!=act2->computeNTotalStudents){
					diffCompNStud=true;
					//return false;
				}
				if(active!=act2->active){
					diffActive=true;
					//return false;	
				}
			}
			else
				i=gt.rules.activitiesList.size();
		}
	}
	if(!diffTeachers && !diffSubject && !diffActivityTags && !diffStudents && !diffCompNStud && !diffNStud && !diffActive)
		return true;
	else
		return false;
}


bool Export::selectSeparatorAndTextquote(QString& textquote, QString& fieldSeparator, bool& head){
	assert(gt.rules.initialized);

	QStringList separators;
	QStringList textquotes;
	separators<<","<<";"<<"|";
	//textquotes<<"\""<<"'"<<Export::tr("no textquote", "The translated field must contain at least 2 characters (normally it should), otherwise the export filter does not work");
	const QString NO_TEXTQUOTE_TRANSLATED=Export::tr("no textquote");
	textquotes<<"\""<<"'"<<NO_TEXTQUOTE_TRANSLATED;
	const int NO_TEXTQUOTE_POS=2; //if you modify line above, modify also this variable to be the position of the no textquote (starts from 0)
	//also, if you add textquotes longer than one character, take care of line 309 (later in the same function) (assert textquote.size()==1)
	//it is permitted for position NO_TEXTQUOTE_POS to have a string longer than 1 QChar
	
	/*if(textquotes[2].size()<=1){
		QMessageBox::warning(NULL, tr("FET warning"), tr("Translation is wrong, because translation of 'no textquote' is too short - falling back to English words. Please report bug"));
		textquote=QString("no textquote");
	}
	assert(textquotes[2].size()>1);*/

	QDialog separatorsDialog(NULL);
	separatorsDialog.setWindowTitle(tr("FET question"));
	QVBoxLayout* separatorsMainLayout=new QVBoxLayout(&separatorsDialog);

	QHBoxLayout* top=new QHBoxLayout();
	QLabel* topText=new QLabel();
	topText->setText(Export::tr("Please keep the default settings.\nImport of data will be easier with these settings."));
	top->addWidget(topText);

	QGroupBox* separatorsGroupBox = new QGroupBox(Export::tr("Please specify the separator between fields:"));
	QComboBox* separatorsCB=NULL;
	if(separators.size()>1){
		QHBoxLayout* separatorBoxChoose=new QHBoxLayout();
		separatorsCB=new QComboBox();

		QLabel* separatorTextChoose=new QLabel();
		separatorTextChoose->setText(Export::tr("Use field separator:"));
		separatorsCB->insertItems(0,separators);
		separatorBoxChoose->addWidget(separatorTextChoose);
		separatorBoxChoose->addWidget(separatorsCB);
		separatorsGroupBox->setLayout(separatorBoxChoose);
	}

	QGroupBox* textquoteGroupBox = new QGroupBox(Export::tr("Please specify the text quote of text fields:"));
	QComboBox* textquoteCB=NULL;
	if(separators.size()>1){
		QHBoxLayout* textquoteBoxChoose=new QHBoxLayout();
		textquoteCB=new QComboBox();
		
		QLabel* textquoteTextChoose=new QLabel();
		textquoteTextChoose->setText(Export::tr("Use textquote:"));
		textquoteCB->insertItems(0,textquotes);
		textquoteBoxChoose->addWidget(textquoteTextChoose);
		textquoteBoxChoose->addWidget(textquoteCB);
		textquoteGroupBox->setLayout(textquoteBoxChoose);
	}

	QGroupBox* firstLineGroupBox = new QGroupBox(Export::tr("Please specify the content of the first line:"));
	QVBoxLayout* firstLineChooseBox=new QVBoxLayout();
	QRadioButton* firstLineRadio1 = new QRadioButton(Export::tr("The first line is the heading."));
	QRadioButton* firstLineRadio2 = new QRadioButton(Export::tr("The first line contains data. Don't export heading."));
	firstLineRadio1->setChecked(true);
	firstLineChooseBox->addWidget(firstLineRadio1);
	firstLineChooseBox->addWidget(firstLineRadio2);
	firstLineGroupBox->setLayout(firstLineChooseBox);

	QPushButton* pb=new QPushButton(tr("OK"));
	QPushButton* cancelpb=new QPushButton(tr("Cancel"));
	QHBoxLayout* hl=new QHBoxLayout();
	hl->addStretch();
	hl->addWidget(pb);
	hl->addWidget(cancelpb);
	
	separatorsMainLayout->addLayout(top);
	separatorsMainLayout->addWidget(separatorsGroupBox);
	separatorsMainLayout->addWidget(textquoteGroupBox);

	separatorsMainLayout->addWidget(firstLineGroupBox);
	separatorsMainLayout->addLayout(hl);

	pb->setDefault(true);
	pb->setFocus();
	
	QObject::connect(pb, SIGNAL(clicked()), &separatorsDialog, SLOT(accept()));
	QObject::connect(cancelpb, SIGNAL(clicked()), &separatorsDialog, SLOT(reject()));
		
	separatorsDialog.setWindowFlags(separatorsDialog.windowFlags() | Qt::WindowMinMaxButtonsHint);

	//separatorsDialog.show();
	int w=separatorsDialog.sizeHint().width();
	int h=separatorsDialog.sizeHint().height();
	separatorsDialog.setGeometry(0,0,w,h);
	centerWidgetOnScreen(&separatorsDialog);

	int ok=separatorsDialog.exec();
	if(ok!=QDialog::Accepted) return false;

	// TODO: if is always true. maybe clean source (also 2 previous if)
	if(separators.size()>1){
		assert(separatorsCB!=NULL);
		assert(textquoteCB!=NULL);
		fieldSeparator=separatorsCB->currentText();
		textquote=textquoteCB->currentText();
		if(textquoteCB->currentItem()==NO_TEXTQUOTE_POS){
			assert(textquote==NO_TEXTQUOTE_TRANSLATED);
			textquote=QString("no tquote"); //must have length >= 2
		}
		else{
			assert(textquote.size()==1);
			//assert(textquote=="\"" || textquote=="'");
		}
	}
	else{
		assert(separatorsCB==NULL);
		assert(textquoteCB==NULL);
		fieldSeparator="";
		textquote="";
	}

	if(textquote.size()!=1)
		textquote="";

	if(firstLineRadio1->isChecked())
		head=true;
	else head=false;
	return true;
}



LastWarningsDialogE::LastWarningsDialogE(QString lastWarning, QWidget *parent): QDialog(parent){
	this->setWindowTitle(tr("FET - export comment", "The comment of the exporting operation"));
	QVBoxLayout* lastWarningsMainLayout=new QVBoxLayout(this);

	QTextEdit* lastWarningsText=new QTextEdit();
	lastWarningsText->setMinimumWidth(500);				//width
	lastWarningsText->setMinimumHeight(250);
	lastWarningsText->setReadOnly(true);
	lastWarningsText->setWordWrapMode(QTextOption::NoWrap);
	lastWarningsText->setText(lastWarning);

	//Start Buttons
	QPushButton* pb1=new QPushButton(tr("&Ok"));
	//pb1->setAutoDefault(true);

	QHBoxLayout* hl=new QHBoxLayout();
	hl->addStretch();
	hl->addWidget(pb1);

	//Start adding all into main layout
	lastWarningsMainLayout->addWidget(lastWarningsText);
	lastWarningsMainLayout->addLayout(hl);

	QObject::connect(pb1, SIGNAL(clicked()), this, SLOT(accept()));
	
	//pb1->setDefault(true);

	pb1->setDefault(true);
	pb1->setFocus();
	
	this->setWindowFlags(this->windowFlags() | Qt::WindowMinMaxButtonsHint);
}


bool Export::exportCSVActivityTags(QString& lastWarnings, const QString textquote, const bool head, const QString setSeparator, QMessageBox::StandardButton& msgBoxButton){
	QString s2=INPUT_FILENAME_XML.right(INPUT_FILENAME_XML.length()-INPUT_FILENAME_XML.findRev(FILE_SEP)-1);	//TODO: remove s2, because to long filenames!

	if(s2.right(4)==".fet")
		s2=s2.left(s2.length()-4);
	//else if(INPUT_FILENAME_XML!="")
	//	cout<<"Minor problem - input file does not end in .fet extension - might be a problem when saving the timetables"<<" (file:"<<__FILE__<<", line:"<<__LINE__<<")"<<endl;

	QString UNDERSCORE="_";
	if(INPUT_FILENAME_XML=="")
		UNDERSCORE="";
	QString file=PREFIX_CSV+s2+UNDERSCORE+CSVActivityTags;
	
	if(!Export::okToWrite(file, msgBoxButton))
		return false;
	
	QFile fileExport(file);
	if(!fileExport.open(QIODevice::WriteOnly)){
		lastWarnings+=Export::tr("FET critical. Cannot open file %1 for writing. Please check your disk's free space. Saving of %1 aborted.").arg(file)+"\n";
		return false;
		assert(0);
	}
	QTextStream tosExport(&fileExport);
	tosExport.setCodec("UTF-8");
	tosExport.setGenerateByteOrderMark(true);	//default is "true", but openOffice have problems to open those files

	if(head)
		tosExport<<textquote<<"Activity Tag"<<textquote<<endl;

	foreach(ActivityTag* a, gt.rules.activityTagsList){
		tosExport<<textquote<<protectCSV(a->name)<<textquote<<endl;
		if(!checkSetSeparator(a->name, setSeparator))
			lastWarnings+=Export::tr("Warning! Import of activities will fail, because %1 include set separator +.").arg(a->name)+"\n";
	}

	lastWarnings+=Export::tr("%1 activity tags exported.").arg(gt.rules.activityTagsList.size())+"\n";
	if(fileExport.error()>0){
		lastWarnings+=Export::tr("FET critical. Writing %1 gave error code %2, which means saving is compromised. Please check your disk's free space.").arg(file).arg(fileExport.error()+"\n");
		return false;
	}
	fileExport.close();
	return true;
}



bool Export::exportCSVRoomsAndBuildings(QString& lastWarnings, const QString textquote, const QString fieldSeparator, const bool head, QMessageBox::StandardButton& msgBoxButton){
	QString s2=INPUT_FILENAME_XML.right(INPUT_FILENAME_XML.length()-INPUT_FILENAME_XML.findRev(FILE_SEP)-1);	//TODO: remove s2, because to long filenames!

	if(s2.right(4)==".fet")
		s2=s2.left(s2.length()-4);
	//else if(INPUT_FILENAME_XML!="")
	//	cout<<"Minor problem - input file does not end in .fet extension - might be a problem when saving the timetables"<<" (file:"<<__FILE__<<", line:"<<__LINE__<<")"<<endl;

	QString UNDERSCORE="_";
	if(INPUT_FILENAME_XML=="")
		UNDERSCORE="";
	QString file=PREFIX_CSV+s2+UNDERSCORE+CSVRoomsAndBuildings;

	if(!Export::okToWrite(file, msgBoxButton))
		return false;
	
	QFile fileExport(file);
	if(!fileExport.open(QIODevice::WriteOnly)){
		lastWarnings+=Export::tr("FET critical. Cannot open file %1 for writing. Please check your disk's free space. Saving of %1 aborted.").arg(file)+"\n";
		return false;
		assert(0);
	}
	QTextStream tosExport(&fileExport);
	tosExport.setCodec("UTF-8");
	tosExport.setGenerateByteOrderMark(true);	//default is "true", but openOffice have problems to open those files
	
	if(head)
		tosExport	<<textquote<<"Room"<<textquote<<fieldSeparator
				<<textquote<<"Room Capacity"<<textquote<<fieldSeparator
				<<textquote<<"Building"<<textquote<<endl;

	QStringList checkBuildings;
	foreach(Room* r, gt.rules.roomsList){
		tosExport	<<textquote<<protectCSV(r->name)<<textquote<<fieldSeparator
				<</*qPrintable(*/QString::number(r->capacity)/*)*/<<fieldSeparator
				<<textquote<<protectCSV(r->building)<<textquote<<endl;
		if(!checkBuildings.contains(r->building)&&!r->building.isEmpty())
			checkBuildings<<r->building;
	}

	lastWarnings+=Export::tr("%1 rooms (with buildings) exported.").arg(gt.rules.roomsList.size())+"\n";
	if(gt.rules.buildingsList.size()!=checkBuildings.size()){
		lastWarnings+=Export::tr("Warning! Only %1 of %2 building names are exported, because %3 buildings don't contain any room.").arg(checkBuildings.size()).arg(gt.rules.buildingsList.size()).arg(gt.rules.buildingsList.size()-checkBuildings.size())+"\n";
	}
	
	if(fileExport.error()>0){
		lastWarnings+=Export::tr("FET critical. Writing %1 gave error code %2, which means saving is compromised. Please check your disk's free space.").arg(file).arg(fileExport.error()+"\n");
		return false;
	}
	fileExport.close();
	return true;
}



bool Export::exportCSVSubjects(QString& lastWarnings, const QString textquote, const bool head, QMessageBox::StandardButton& msgBoxButton){
	QString s2=INPUT_FILENAME_XML.right(INPUT_FILENAME_XML.length()-INPUT_FILENAME_XML.findRev(FILE_SEP)-1);	//TODO: remove s2, because to long filenames!

	if(s2.right(4)==".fet")
		s2=s2.left(s2.length()-4);
	//else if(INPUT_FILENAME_XML!="")
	//	cout<<"Minor problem - input file does not end in .fet extension - might be a problem when saving the timetables"<<" (file:"<<__FILE__<<", line:"<<__LINE__<<")"<<endl;

	QString UNDERSCORE="_";
	if(INPUT_FILENAME_XML=="")
		UNDERSCORE="";
	QString file=PREFIX_CSV+s2+UNDERSCORE+CSVSubjects;

	if(!Export::okToWrite(file, msgBoxButton))
		return false;
	
	QFile fileExport(file);
	if(!fileExport.open(QIODevice::WriteOnly)){
		lastWarnings+=Export::tr("FET critical. Cannot open file %1 for writing. Please check your disk's free space. Saving of %1 aborted.").arg(file)+"\n";
		return false;
		assert(0);
	}
	QTextStream tosExport(&fileExport);
	tosExport.setCodec("UTF-8");
	tosExport.setGenerateByteOrderMark(true);	//default is "true", but openOffice have problems to open those files
	
	if(head)
		tosExport<<textquote<<"Subject"<<textquote<<endl;

	foreach(Subject* s, gt.rules.subjectsList){
		tosExport<<textquote<<protectCSV(s->name)<<textquote<<endl;
	}

	lastWarnings+=Export::tr("%1 subjects exported.").arg(gt.rules.subjectsList.size())+"\n";	
	if(fileExport.error()>0){
		lastWarnings+=Export::tr("FET critical. Writing %1 gave error code %2, which means saving is compromised. Please check your disk's free space.").arg(file).arg(fileExport.error()+"\n");
		return false;
	}
	fileExport.close();
	return true;
}



bool Export::exportCSVTeachers(QString& lastWarnings, const QString textquote, const bool head, const QString setSeparator, QMessageBox::StandardButton& msgBoxButton){
	QString s2=INPUT_FILENAME_XML.right(INPUT_FILENAME_XML.length()-INPUT_FILENAME_XML.findRev(FILE_SEP)-1);	//TODO: remove s2, because to long filenames!

	if(s2.right(4)==".fet")
		s2=s2.left(s2.length()-4);
	//else if(INPUT_FILENAME_XML!="")
	//	cout<<"Minor problem - input file does not end in .fet extension - might be a problem when saving the timetables"<<" (file:"<<__FILE__<<", line:"<<__LINE__<<")"<<endl;

	QString UNDERSCORE="_";
	if(INPUT_FILENAME_XML=="")
		UNDERSCORE="";
	QString file=PREFIX_CSV+s2+UNDERSCORE+CSVTeachers;

	if(!Export::okToWrite(file, msgBoxButton))
		return false;
	
	QFile fileExport(file);
	if(!fileExport.open(QIODevice::WriteOnly)){
		lastWarnings+=Export::tr("FET critical. Cannot open file %1 for writing. Please check your disk's free space. Saving of %1 aborted.").arg(file)+"\n";
		return false;
		assert(0);
	}
	QTextStream tosExport(&fileExport);
	tosExport.setCodec("UTF-8");
	tosExport.setGenerateByteOrderMark(true);	//default is "true", but openOffice have problems to open those files
	
	if(head)
		tosExport<<textquote<<"Teacher"<<textquote<<endl;

	foreach(Teacher* t, gt.rules.teachersList){
		tosExport<<textquote<<protectCSV(t->name)<<textquote<<endl;
		if(!checkSetSeparator(t->name, setSeparator))
			lastWarnings+=Export::tr("Warning! Import of activities will fail, because %1 include set separator +.").arg(t->name)+"\n";
	}

	lastWarnings+=Export::tr("%1 teachers exported.").arg(gt.rules.teachersList.size())+"\n";
	if(fileExport.error()>0){
		lastWarnings+=Export::tr("FET critical. Writing %1 gave error code %2, which means saving is compromised. Please check your disk's free space.").arg(file).arg(fileExport.error()+"\n");
		return false;
	}
	fileExport.close();
	return true;
}



bool Export::exportCSVStudents(QString& lastWarnings, const QString textquote, const QString fieldSeparator, const bool head, const QString setSeparator
 , QMessageBox::StandardButton& msgBoxButton){
	QString s2=INPUT_FILENAME_XML.right(INPUT_FILENAME_XML.length()-INPUT_FILENAME_XML.findRev(FILE_SEP)-1);	//TODO: remove s2, because to long filenames!

	if(s2.right(4)==".fet")
		s2=s2.left(s2.length()-4);
	//else if(INPUT_FILENAME_XML!="")
	//	cout<<"Minor problem - input file does not end in .fet extension - might be a problem when saving the timetables"<<" (file:"<<__FILE__<<", line:"<<__LINE__<<")"<<endl;

	QString UNDERSCORE="_";
	if(INPUT_FILENAME_XML=="")
		UNDERSCORE="";
	QString file=PREFIX_CSV+s2+UNDERSCORE+CSVStudents;

	if(!Export::okToWrite(file, msgBoxButton))
		return false;
	
	QFile fileExport(file);
	if(!fileExport.open(QIODevice::WriteOnly)){
		lastWarnings+=Export::tr("FET critical. Cannot open file %1 for writing. Please check your disk's free space. Saving of %1 aborted.").arg(file)+"\n";
		return false;
		assert(0);
	}
	QTextStream tosExport(&fileExport);
	tosExport.setCodec("UTF-8");
	tosExport.setGenerateByteOrderMark(true);	//default is "true", but openOffice have problems to open those files
	
	if(head)
		tosExport	<<textquote<<"Year"<<textquote<<fieldSeparator
				<<textquote<<"Number of Students per Year"<<textquote<<fieldSeparator
				<<textquote<<"Group"<<textquote<<fieldSeparator
				<<textquote<<"Number of Students per Group"<<textquote<<fieldSeparator
				<<textquote<<"Subgroup"<<textquote<<fieldSeparator
				<<textquote<<"Number of Students per Subgroup"<<textquote<<endl;

	int ig=0;
	int is=0;
	foreach(StudentsYear* sty, gt.rules.yearsList){
		tosExport<<textquote<<protectCSV(sty->name)<<textquote<<fieldSeparator
					<</*qPrintable(*/QString::number(sty->numberOfStudents)/*)*/<<fieldSeparator<<fieldSeparator<<fieldSeparator<<fieldSeparator<<endl;
		if(!checkSetSeparator(sty->name, setSeparator))
			lastWarnings+=Export::tr("Warning! Import of activities will fail, because %1 include set separator +.").arg(sty->name)+"\n";
		foreach(StudentsGroup* stg, sty->groupsList){
			ig++;
			tosExport	<<textquote<<protectCSV(sty->name)<<textquote<<fieldSeparator
					<</*qPrintable(*/QString::number(sty->numberOfStudents)/*)*/<<fieldSeparator
					<<textquote<<protectCSV(stg->name)<<textquote<<fieldSeparator
					<</*qPrintable(*/QString::number(stg->numberOfStudents)/*)*/<<fieldSeparator<<fieldSeparator<<endl;
			if(!checkSetSeparator(stg->name, setSeparator))
				lastWarnings+=Export::tr("Warning! Import of activities will fail, because %1 include set separator +.").arg(stg->name)+"\n";
			foreach(StudentsSubgroup* sts, stg->subgroupsList){
				is++;
				tosExport	<<textquote<<protectCSV(sty->name)<<textquote<<fieldSeparator
						<</*qPrintable(*/QString::number(sty->numberOfStudents)/*)*/<<fieldSeparator
						<<textquote<<protectCSV(stg->name)<<textquote<<fieldSeparator
						<</*qPrintable(*/QString::number(stg->numberOfStudents)/*)*/<<fieldSeparator
						<<textquote<<protectCSV(sts->name)<<textquote<<fieldSeparator
						<</*qPrintable(*/QString::number(sts->numberOfStudents)/*)*/<<endl;
				if(!checkSetSeparator(sts->name, setSeparator))
					lastWarnings+=Export::tr("Warning! Import of activities will fail, because %1 include set separator +.").arg(sts->name)+"\n";
			}
		}
	}

	lastWarnings+=Export::tr("%1 years exported.").arg(gt.rules.yearsList.size())+"\n";
	lastWarnings+=Export::tr("%1 groups exported.").arg(ig)+"\n";
	lastWarnings+=Export::tr("%1 subgroups exported.").arg(is)+"\n";
	if(fileExport.error()>0){
		lastWarnings+=Export::tr("FET critical. Writing %1 gave error code %2, which means saving is compromised. Please check your disk's free space.").arg(file).arg(fileExport.error()+"\n");
		return false;
	}
	fileExport.close();
	return true;
}



bool Export::exportCSVActivities(QString& lastWarnings, const QString textquote, const QString fieldSeparator, const bool head, QMessageBox::StandardButton& msgBoxButton){
	QString s2=INPUT_FILENAME_XML.right(INPUT_FILENAME_XML.length()-INPUT_FILENAME_XML.findRev(FILE_SEP)-1);	//TODO: remove s2, because to long filenames!

	if(s2.right(4)==".fet")
		s2=s2.left(s2.length()-4);
	//else if(INPUT_FILENAME_XML!="")
	//	cout<<"Minor problem - input file does not end in .fet extension - might be a problem when saving the timetables"<<" (file:"<<__FILE__<<", line:"<<__LINE__<<")"<<endl;

	QString UNDERSCORE="_";
	if(INPUT_FILENAME_XML=="")
		UNDERSCORE="";
	QString file=PREFIX_CSV+s2+UNDERSCORE+CSVActivities;

	if(!Export::okToWrite(file, msgBoxButton))
		return false;
	
	QFile fileExport(file);
	if(!fileExport.open(QIODevice::WriteOnly)){
		lastWarnings+=Export::tr("FET critical. Cannot open file %1 for writing. Please check your disk's free space. Saving of %1 aborted.").arg(file)+"\n";
		return false;
		assert(0);
	}
	QTextStream tosExport(&fileExport);
	tosExport.setCodec("UTF-8");
	tosExport.setGenerateByteOrderMark(true);	//default is "true", but openOffice have problems to open those files
	
	if(head)
		tosExport	<<textquote<<"Students Sets"<<textquote<<fieldSeparator
				<<textquote<<"Subject"<<textquote<<fieldSeparator
				<<textquote<<"Teachers"<<textquote<<fieldSeparator
				<<textquote<<"Activity Tags"<<textquote<<fieldSeparator
				<<textquote<<"Total Duration"<<textquote<<fieldSeparator
				<<textquote<<"Split Duration"<<textquote<<fieldSeparator
				<<textquote<<"Min Days"<<textquote<<fieldSeparator
				<<textquote<<"Weight"<<textquote<<fieldSeparator
				<<textquote<<"Consecutive"<<textquote<<endl;

	//code by Liviu Lalescu (begin)
	//better detection of min day constraint
	QHash<int, int> activitiesRepresentant;
	QHash<int, int> activitiesNumberOfSubactivities;
	QHash<int, ConstraintMinDaysBetweenActivities*>activitiesConstraints;
	
	activitiesRepresentant.clear();
	activitiesNumberOfSubactivities.clear();
	activitiesConstraints.clear();
	
	foreach(Activity* act, gt.rules.activitiesList){
		assert(!activitiesRepresentant.contains(act->id));
		activitiesRepresentant.insert(act->id, act->activityGroupId); //act->id is key, act->agid is value
	
		if(act->activityGroupId>0){
			int n=activitiesNumberOfSubactivities.value(act->activityGroupId, 0); //0 here means default value
			n++;
			activitiesNumberOfSubactivities.insert(act->activityGroupId, n); //overwrites old value
		}
	}
	foreach(TimeConstraint* tc, gt.rules.timeConstraintsList){
		if(tc->type==CONSTRAINT_MIN_DAYS_BETWEEN_ACTIVITIES){
			ConstraintMinDaysBetweenActivities* c=(ConstraintMinDaysBetweenActivities*) tc;
	
			QSet<int> aset;
			int repres=-1;
	
			for(int i=0; i<c->n_activities; i++){
				int aid=c->activitiesId[i];
				aset.insert(aid);
	
				if(activitiesRepresentant.value(aid,0)==aid)
					repres=aid; //does not matter if there are more representants in this constraint, the constraint will be skipped anyway in this case
			}
	
			bool oktmp=false;
	
			if(repres>0){
				if(aset.count()==activitiesNumberOfSubactivities.value(repres, 0)){
					oktmp=true;
					foreach(int aid, aset)
						if(activitiesRepresentant.value(aid, 0)!=repres){
							oktmp=false;
							break;
						}
				}
			}
	
			if(!oktmp){
				lastWarnings+=Export::tr("Note: Constraint")+" "+c->getDescription(gt.rules)+" "+tr("was skipped, because"
				" it refers not to a whole larger container activity")+"\n";
			}
	
			if(oktmp){
				ConstraintMinDaysBetweenActivities* oldc=activitiesConstraints.value(repres, NULL);
				if(oldc!=NULL){
					if(oldc->weightPercentage < c->weightPercentage){
						activitiesConstraints.insert(repres, c); //overwrites old value
						lastWarnings+=Export::tr("Note: Constraint")+" "+oldc->getDescription(gt.rules)+" "+tr("was skipped, because"
							" there exists another constraint of this type with larger weight percentage, referring to the same activities")+"\n";
					}
					else if(oldc->weightPercentage > c->weightPercentage){
						lastWarnings+=Export::tr("Note: Constraint")+" "+c->getDescription(gt.rules)+" "+tr("was skipped, because"
							" there exists another constraint of this type with larger weight percentage, referring to the same activities")+"\n";
					}
	
					//equal weights - choose the lowest number of min days
					else if(oldc->minDays > c->minDays){
						lastWarnings+=Export::tr("Note: Constraint")+" "+c->getDescription(gt.rules)+" "+tr("was skipped, because"
							" there exists another constraint of this type with same weight percentage and higher number of min days, referring to the same activities")+"\n";
					}
					else if(oldc->minDays < c->minDays){
						activitiesConstraints.insert(repres, c); //overwrites old value
						lastWarnings+=Export::tr("Note: Constraint")+" "+oldc->getDescription(gt.rules)+" "+tr("was skipped, because"
							" there exists another constraint of this type with same weight percentage and higher number of min days, referring to the same activities")+"\n";
					}
	
					//equal weights and min days - choose the one with consecutive is same day true
					else if(oldc->consecutiveIfSameDay==true){
						lastWarnings+=Export::tr("Note: Constraint")+" "+c->getDescription(gt.rules)+" "+tr("was skipped, because"
							" there exists another constraint of this type with same weight percentage and same number of min days and"
							" consecutive if same day true, referring to the same activities")+"\n";
					}
					else if(c->consecutiveIfSameDay==true){
						activitiesConstraints.insert(repres, c); //overwrites old value
						lastWarnings+=Export::tr("Note: Constraint")+" "+oldc->getDescription(gt.rules)+" "+tr("was skipped, because"
							" there exists another constraint of this type with same weight percentage and same number of min days and"
							" consecutive if same day true, referring to the same activities")+"\n";
					}
	
				}
				else
					activitiesConstraints.insert(repres, c);
			}
		}
	}
	//code by Liviu Lalescu (end)
	
	bool manuallyEdited=false;

	Activity* acti;
	Activity* actiNext;
	int countExportedActivities=0;
	for(int ai=0; ai<gt.rules.activitiesList.size(); ai++){
		acti=gt.rules.activitiesList[ai];
		//if(acti->active){
			if((acti->activityGroupId==acti->id)||(acti->activityGroupId==0)){
				bool diffTeachers, diffSubject, diffActivityTag, diffStudents, diffCompNStud, diffNStud, diffActive;
				if(isActivityNotManualyEdited(ai, diffTeachers, diffSubject, diffActivityTag, diffStudents, diffCompNStud, diffNStud, diffActive)){
				}
				else{
					QStringList s;
					if(diffTeachers)
						s.append(tr("different teachers"));
					if(diffSubject)
						s.append(tr("different subject"));
					if(diffActivityTag)
						s.append(tr("different activity tags"));
					if(diffStudents)
						s.append(tr("different students"));
					if(diffCompNStud)
						s.append(tr("different boolean variable 'must compute n total students'"));
					if(diffNStud)
						s.append(tr("different number of students"));
					if(diffActive)
						s.append(tr("different active flag"));
					
					manuallyEdited=true;
					
					lastWarnings+=tr("Subactivities with activity group id %1 are different between themselves (they were separately edited),"
						" so the export will not be very accurate. The fields which are different will be considered those of the representative subactivity. Fields which were"
						" different are: %2").arg(QString::number(acti->activityGroupId)).arg(s.join(", "))+"\n";
				}
				if(!acti->active){
					if(acti->activityGroupId==0)
						lastWarnings+=tr("Activity with id %1 has disabled active flag but it is exported.").arg(QString::number(acti->id))+"\n";
					else
						lastWarnings+=tr("Subactivities with activity group id %1 have disabled active flag but they are exported.").arg(QString::number(acti->activityGroupId))+"\n";
				}
				
				countExportedActivities++;
				//students set
				tosExport<<textquote;
				for(int s=0; s<acti->studentsNames.size(); s++){
					if(s!=0)
						tosExport<<"+";
					tosExport<<protectCSV(acti->studentsNames[s]);
				}
				tosExport<<textquote<<fieldSeparator<<textquote;
				//subject
				tosExport<<protectCSV(acti->subjectName);
				tosExport<<textquote<<fieldSeparator<<textquote;
				//teachers
				for(int t=0; t<acti->teachersNames.size(); t++){
					if(t!=0)
						tosExport<<"+";
					tosExport<<protectCSV(acti->teachersNames[t]);
				}
				tosExport<<textquote<<fieldSeparator<<textquote;
				//activity tags
				for(int s=0; s<acti->activityTagsNames.size(); s++){
					if(s!=0)
						tosExport<<"+";
					tosExport<<protectCSV(acti->activityTagsNames[s]);
				}
				tosExport<<textquote<<fieldSeparator;
				//total duration
				tosExport<</*qPrintable(*/QString::number(acti->totalDuration)/*)*/;
				tosExport<<fieldSeparator<<textquote;
				//split duration
				for(int aiNext=ai; aiNext<gt.rules.activitiesList.size(); aiNext++){
					actiNext=gt.rules.activitiesList[aiNext];
					if(acti->activityGroupId!=0&&actiNext->activityGroupId==acti->activityGroupId){
						if(aiNext!=ai)
							tosExport<<"+";
						tosExport<<actiNext->duration;
					} else {
						if(acti->activityGroupId==0&&actiNext->activityGroupId==acti->activityGroupId){
							assert(ai==aiNext);
							assert(actiNext->duration==actiNext->totalDuration);
							if(actiNext->duration>1)
								tosExport<<actiNext->duration;
						}	
						aiNext=gt.rules.activitiesList.size();
					}	
				}
				tosExport<<textquote<<fieldSeparator;
				//min days
				//start new code, because of Livius detection
				bool careAboutMinDay=false;
				ConstraintMinDaysBetweenActivities* tcmd=activitiesConstraints.value(acti->id, NULL);
				if(acti->id==acti->activityGroupId){
					if(tcmd!=NULL){
						careAboutMinDay=true;
					}
				}
				//end new code
				if(careAboutMinDay){
					assert(tcmd->type==CONSTRAINT_MIN_DAYS_BETWEEN_ACTIVITIES);
					tosExport<</*qPrintable(*/QString::number(tcmd->minDays)/*)*/;
				}
				tosExport<<fieldSeparator;
				//min days weight
				if(careAboutMinDay)
					tosExport<</*qPrintable(*/QString::number(tcmd->weightPercentage)/*)*/;
				tosExport<<fieldSeparator;
				//min days consecutive
				if(careAboutMinDay)
					tosExport<<tcmd->consecutiveIfSameDay;
				tosExport<<endl;
			}
		//}
	}
	
	if(manuallyEdited){
		QMessageBox::warning(NULL, tr("FET warning"), tr("There are subactivities which were modified separately - so the "
			"components had different values for subject, activity tags, teachers, students or number of students from the representative subactivity. The export was done, but it is not very accurate."));
	}

	lastWarnings+=Export::tr("%1 activities exported.").arg(countExportedActivities)+"\n";
	if(fileExport.error()>0){
		lastWarnings+=Export::tr("FET critical. Writing %1 gave error code %2, which means saving is compromised. Please check your disk's free space.").arg(file).arg(fileExport.error())+"\n";
		return false;
	}
	fileExport.close();
	return true;
}




bool Export::exportCSVActivitiesStatistic(QString& lastWarnings, const QString textquote, const QString fieldSeparator, const bool head, QMessageBox::StandardButton& msgBoxButton){
	QString s2=INPUT_FILENAME_XML.right(INPUT_FILENAME_XML.length()-INPUT_FILENAME_XML.findRev(FILE_SEP)-1);	//TODO: remove s2, because to long filenames!

	if(s2.right(4)==".fet")
		s2=s2.left(s2.length()-4);
	//else if(INPUT_FILENAME_XML!="")
	//	cout<<"Minor problem - input file does not end in .fet extension - might be a problem when saving the timetables"<<" (file:"<<__FILE__<<", line:"<<__LINE__<<")"<<endl;

	QString UNDERSCORE="_";
	if(INPUT_FILENAME_XML=="")
		UNDERSCORE="";
	QString file=PREFIX_CSV+s2+UNDERSCORE+CSVActivitiesStatistic;

	if(!Export::okToWrite(file, msgBoxButton))
		return false;
	
	QFile fileExport(file);
	if(!fileExport.open(QIODevice::WriteOnly)){
		lastWarnings+=Export::tr("FET critical. Cannot open file %1 for writing. Please check your disk's free space. Saving of %1 aborted.").arg(file)+"\n";
		return false;
		assert(0);
	}
	QTextStream tosExport(&fileExport);
	tosExport.setCodec("UTF-8");
	tosExport.setGenerateByteOrderMark(true);	//default is "true", but openOffice have problems to open those files
	
	if(head)
		tosExport	<<textquote<<"Students Sets"<<textquote<<fieldSeparator
				<<textquote<<"Subject"<<textquote<<fieldSeparator
				<<textquote<<"Teachers"<<textquote<<fieldSeparator
				<<textquote<<"Total Duration"<<textquote<<"\n";



	Activity* acti;
	int countExportedActivities=0;
	QMap<QString, int> tmpIdentDuration;	//not QHash, because i want a nice order of the activities
	for(int ai=0; ai<gt.rules.activitiesList.size(); ai++){
		acti=gt.rules.activitiesList[ai];
		if(acti->active){
			int tmpD=acti->duration;
			QString tmpIdent=textquote;
			if(acti->studentsNames.size()>0){
				for(QStringList::Iterator it=acti->studentsNames.begin(); it!=acti->studentsNames.end(); it++){
					tmpIdent+=protectCSV(*it);
					if(it!=acti->studentsNames.end()-1)
						tmpIdent+="+";
				}
			}
			tmpIdent+=textquote+fieldSeparator+textquote+protectCSV(acti->subjectName)+textquote+fieldSeparator+textquote;
			if(acti->teachersNames.size()>0){
				for(QStringList::Iterator it=acti->teachersNames.begin(); it!=acti->teachersNames.end(); it++){
					tmpIdent+=protectCSV(*it);
					if(it!=acti->teachersNames.end()-1)
						tmpIdent+="+";
				}
			}
			tmpIdent+=textquote+fieldSeparator;
			tmpD+=tmpIdentDuration.value(tmpIdent);
			tmpIdentDuration.insert(tmpIdent, tmpD);
		}
	}
	QMapIterator<QString, int> it(tmpIdentDuration);
	while(it.hasNext()){
		countExportedActivities++;
		it.next();
		tosExport<<it.key();
		tosExport<<textquote<<QString::number(it.value())<<textquote<<"\n";
	}

	lastWarnings+=Export::tr("%1 active activities statistics exported.").arg(countExportedActivities)+"\n";
	if(fileExport.error()>0){
		lastWarnings+=Export::tr("FET critical. Writing %1 gave error code %2, which means saving is compromised. Please check your disk's free space.").arg(file).arg(fileExport.error())+"\n";
		return false;
	}
	fileExport.close();
	return true;
}



bool Export::exportCSVTimetable(QString& lastWarnings, const QString textquote, const QString fieldSeparator, const bool head, QMessageBox::StandardButton& msgBoxButton){
	QString s2=INPUT_FILENAME_XML.right(INPUT_FILENAME_XML.length()-INPUT_FILENAME_XML.findRev(FILE_SEP)-1);	//TODO: remove s2, because to long filenames!

	if(s2.right(4)==".fet")
		s2=s2.left(s2.length()-4);
	//else if(INPUT_FILENAME_XML!="")
	//	cout<<"Minor problem - input file does not end in .fet extension - might be a problem when saving the timetables"<<" (file:"<<__FILE__<<", line:"<<__LINE__<<")"<<endl;

	QString UNDERSCORE="_";
	if(INPUT_FILENAME_XML=="")
		UNDERSCORE="";
	QString file=PREFIX_CSV+s2+UNDERSCORE+CSVTimetable;
	
	if(!Export::okToWrite(file, msgBoxButton))
		return false;
	
	QFile fileExport(file);
	if(!fileExport.open(QIODevice::WriteOnly)){
		lastWarnings+=Export::tr("FET critical. Cannot open file %1 for writing. Please check your disk's free space. Saving of %1 aborted.").arg(file)+"\n";
		return false;
		assert(0);
	}
	QTextStream tosExport(&fileExport);
	tosExport.setCodec("UTF-8");
	tosExport.setGenerateByteOrderMark(true);	//default is "true", but openOffice have problems to open those files
	
	//section "Activity Id" was added by Liviu on 2010-01-26, as suggested on the forum
	if(head)
		tosExport
				<<textquote<<"Activity Id"<<textquote<<fieldSeparator
				<<textquote<<"Day"<<textquote<<fieldSeparator
				<<textquote<<"Hour"<<textquote<<fieldSeparator
				<<textquote<<"Students Sets"<<textquote<<fieldSeparator
				<<textquote<<"Subject"<<textquote<<fieldSeparator
				<<textquote<<"Teachers"<<textquote<<fieldSeparator
				<<textquote<<"Activity Tags"<<textquote<<fieldSeparator
				<<textquote<<"Room"<<textquote<<endl;

	if(gt.rules.initialized && gt.rules.internalStructureComputed
	 && students_schedule_ready && teachers_schedule_ready && rooms_schedule_ready){
		Activity *act;
		int exportedActivities=0;
		for(int i=0; i<gt.rules.nInternalActivities; i++){
			if(best_solution.times[i]!=UNALLOCATED_TIME) {
				exportedActivities++;
				act=&gt.rules.internalActivitiesList[i];
				int hour=best_solution.times[i]/gt.rules.nDaysPerWeek;
				int day=best_solution.times[i]%gt.rules.nDaysPerWeek;
				int r=best_solution.rooms[i];
				for(int dd=0; dd < act->duration; dd++){
					assert(hour+dd<gt.rules.nHoursPerDay);
					
					//Activity id - added by Liviu on 2010-01-26
					tosExport<<textquote<<QString::number(act->id)<<textquote<<fieldSeparator;
					
					//Day
					tosExport<<textquote<<protectCSV(gt.rules.daysOfTheWeek[day])<<textquote<<fieldSeparator;
					//Period
					tosExport<<textquote<<protectCSV(gt.rules.hoursOfTheDay[hour+dd])<<textquote<<fieldSeparator<<textquote;
					//Students Sets
					for(int s=0; s<act->studentsNames.size(); s++){
						if(s!=0)
							tosExport<<"+";
						tosExport<<protectCSV(act->studentsNames[s]);
					}
					tosExport<<textquote<<fieldSeparator<<textquote;
					//Subject
					tosExport<<protectCSV(act->subjectName);
					tosExport<<textquote<<fieldSeparator<<textquote;
					//Teachers
					for(int t=0; t<act->teachersNames.size(); t++){
						if(t!=0)
							tosExport<<"+";
						tosExport<<protectCSV(act->teachersNames[t]);
					}
					tosExport<<textquote<<fieldSeparator<<textquote;
					//Activity Tags
					for(int s=0; s<act->activityTagsNames.size(); s++){
						if(s!=0)
							tosExport<<"+";
						tosExport<<protectCSV(act->activityTagsNames[s]);
					}
					tosExport<<textquote<<fieldSeparator;
					//Room
					if(best_solution.rooms[i] != UNSPECIFIED_ROOM && best_solution.rooms[i] != UNALLOCATED_SPACE){
						assert(best_solution.rooms[i]>=0 && best_solution.rooms[i]<gt.rules.nInternalRooms);
						tosExport<<textquote<<protectCSV(gt.rules.internalRoomsList[r]->name)<<textquote;
					}
					else{ //added by Liviu on 2010-01-26
						tosExport<<textquote<<textquote;
					}
					tosExport<<endl;
				}
			}
		}	
		lastWarnings+=Export::tr("%1 scheduled activities exported.").arg(exportedActivities)+"\n";
	} else {
		lastWarnings+=Export::tr("0 scheduled activities exported, because no timetable was generated.")+"\n";
	}
	if(fileExport.error()>0){
		lastWarnings+=Export::tr("FET critical. Writing %1 gave error code %2, which means saving is compromised. Please check your disk's free space.").arg(file).arg(fileExport.error())+"\n";
		return false;
	}
	fileExport.close();
	return true;
}
