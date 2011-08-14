/*
File fet.cpp - program using the main engine "timetable"
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
along with timetable; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "fet.h"

#include "matrix.h"

#include <QMessageBox>

#include <QLocale>
#include <QTime>
#include <QDate>
#include <QDateTime>

//#include <QtGlobal>
//(for qVersion())

#include <ctime>

#include "timetableexport.h"
#include "generate.h"

#include "timetable_defs.h"
#include "timetable.h"

#include <QCoreApplication>
#include <QMutex>
#include <QString>
#include <QTranslator>

#include <QDir>
#include <QTranslator>

#include <QSettings>

#include <QRect>
 QRect mainFormSettingsRect;
 int MAIN_FORM_SHORTCUTS_TAB_POSITION;
const QString COMPANY="fet";
const QString PROGRAM="fettimetabling";
#include <fstream>
#include <iostream>
using namespace std;

#include <QTextStream>
#include <QFile>

extern Solution best_solution;

extern bool students_schedule_ready, teachers_schedule_ready, rooms_schedule_ready;

extern QMutex mutex;
bool USE_GUI_COLORS=false;

bool SHOW_SHORTCUTS_ON_MAIN_WINDOW=true;

bool ENABLE_ACTIVITY_TAG_MAX_HOURS_DAILY=false;
bool ENABLE_STUDENTS_MAX_GAPS_PER_DAY=false;

bool SHOW_WARNING_FOR_NOT_PERFECT_CONSTRAINTS=true;


bool ENABLE_STUDENTS_MIN_HOURS_DAILY_WITH_ALLOW_EMPTY_DAYS=false;

bool SHOW_WARNING_FOR_STUDENTS_MIN_HOURS_DAILY_WITH_ALLOW_EMPTY_DAYS;

bool CONFIRM_ACTIVITY_PLANNING=true;
bool CONFIRM_SPREAD_ACTIVITIES=true;
bool CONFIRM_REMOVE_REDUNDANT=true;
void writeDefaultSimulationParameters();

QTranslator translator;

/**
The one and only instantiation of the main class.
*/
Timetable gt;

/**
Log file.
*/
ofstream logg;

/**
The name of the file from where the rules are read.
*/
QString INPUT_FILENAME_XML;

/**
The working directory
*/
QString WORKING_DIRECTORY;


/**
The import directory
*/
QString IMPORT_DIRECTORY;


/*qint16 teachers_timetable_weekly[MAX_TEACHERS][MAX_DAYS_PER_WEEK][MAX_HOURS_PER_DAY];
qint16 students_timetable_weekly[MAX_TOTAL_SUBGROUPS][MAX_DAYS_PER_WEEK][MAX_HOURS_PER_DAY];
qint16 rooms_timetable_weekly[MAX_ROOMS][MAX_DAYS_PER_WEEK][MAX_HOURS_PER_DAY];*/
Matrix3D<qint16> teachers_timetable_weekly;
Matrix3D<qint16> students_timetable_weekly;
Matrix3D<qint16> rooms_timetable_weekly;
//QList<qint16> teachers_free_periods_timetable_weekly[TEACHERS_FREE_PERIODS_N_CATEGORIES][MAX_DAYS_PER_WEEK][MAX_HOURS_PER_DAY];
Matrix3D<QList<qint16> > teachers_free_periods_timetable_weekly;

QCoreApplication* pqapplication=NULL;

extern int XX;
extern int YY;

void usage(QTextStream& out, const QString& error)
{
	QString s="";
	
	s+=QString("Incorrect command line parameters (%1).").arg(error);
	
	s+="\n\n";
	
	s+=QString("Command line usage: \"fet --inputfile=x [--outputdir=d] [--timelimitseconds=y] [--htmllevel=z] [--language=t] [--printnotavailable=u] [--printbreak=b] "
		"[--dividetimeaxisbydays=v] [--printsimultaneousactivities=w] [--randomseedx=rx --randomseedy=ry] [--warnifusingnotperfectconstraints=s]"
		" [--warnifusingstudentsminhoursdailywithallowemptydays=p]\",\n"
		"where:\nx is the input file, for instance \"data.fet\"\n"
		"d is the path to results directory, without trailing slash or backslash (default is current working path). "
		"Make sure you have write permissions there.\n"
		"y is integer (seconds) (default 2000000000, which is practically infinite).\n"
		"z is integer from 0 to 6 and represents the detail level for the generated html timetables "
		"(default 2, larger values have more details/facilities and larger file sizes).\n"
		"t is one of en_US, ar, ca, da, de, el, es, fa, fr, gl, he, hu, id, it, lt, mk, ms, nl, pl, pt_BR, ro, ru, si, sk, sr, tr, uk (default en_US).\n"
		"u is either \"true\" or \"false\" and represents if you want -x- (for true) or --- (for false) in the generated timetables for the "
		"not available slots (default true).\n"
		"b is either \"true\" or \"false\" and represents if you want -X- (for true) or --- (for false) in the generated timetables for the "
		"break slots (default true).\n"
		"v is either true or false, represents if you want html timetables with time-axis divided by days (default false).\n"
		"w is either true or false, represents if you want html timetables to show related activities which have constraints with same starting time (default false).\n"
		"(for instance, if A1 (T1, G1) and A2 (T2, G2) have constraint activities same starting time, then in T1's timetable will appear also A2, at the same slot "
		"as A1).\n"
		"rx is the random seed X component, minimum 1 to maximum 2147483646, ry is the random seed Y component, minimum 1 to maximum 2147483398"
		" (you can get the same timetable if the input file is identical, if the FET version is the same and if the random seed X and Y components are the same).\n"
		"s is either true or false, represents whether you want a message box to be shown, with a warning, if the input file contains not perfect constraints "
		"(activity tag max hours daily or students max gaps per day) (default true).\n"
		"p is either true or false, represents whether you want a message box to be shown, with a warning, if the input file contains non standard constraints "
		"students min hours daily with allow empty days (default true).\n"
		"\n"
		"Alternatively, you can run \"fet --version [--outputdir=d]\" to get the current FET version. "
		"where:\nd is the path to results directory, without trailing slash or backslash (default is current working path). "
		"Make sure you have write permissions there.\n"
		"(If you specify the \"--version\" argument, FET just prints version number on the command line prompt and in the output directory and exits.)");
	
	//s+="\n\n";
	//s+=QString("You can find the command line FET usage also in the README file");
	
	cout<<qPrintable(s)<<endl;
	out<<qPrintable(s)<<endl;
}

void readSimulationParameters(){
	const QString predefDir=QDir::homePath()+FILE_SEP+"fet-results";

	QSettings newSettings(COMPANY, PROGRAM);

	if(newSettings.contains("version")){
		if(!newSettings.contains("output-directory")){
			OUTPUT_DIR=predefDir;
				
			/*QString s;
			s+=FetTranslate::tr("You upgraded FET to a new version. Beginning with FET-5.10.0 (June 2009),"
			 " you can change the output directory of FET (see the 'Settings' menu).")
			 +"\n\n"+FetTranslate::tr("For the moment, the output (results) directory will be set to the predefined value %1")
			 .arg(QDir::toNativeSeparators(OUTPUT_DIR))
			 +" . "+ FetTranslate::tr("You can change it later", "It refers to the output directory");

			QMessageBox::information(NULL, FetTranslate::tr("FET important note"), s, FetTranslate::tr("OK, I have read this"));
	*/	}
		else{
			OUTPUT_DIR=newSettings.value("output-directory", predefDir).toString();
			QDir dir;
			if(!dir.exists(OUTPUT_DIR)){
				bool t=dir.mkpath(OUTPUT_DIR);
				if(!t){
					/*QMessageBox::warning(NULL, FetTranslate::tr("FET warning"), FetTranslate::tr("Output directory %1 does not exist and cannot be"
					 " created - output directory will be made the default value %2")
					 .arg(QDir::toNativeSeparators(OUTPUT_DIR)).arg(QDir::toNativeSeparators(predefDir)));
					OUTPUT_DIR=predefDir;
				*/}
			}
		}
	}
	else{
		//OUTPUT_DIR=newSettings.value("output-directory", predefDir).toString();
		OUTPUT_DIR=predefDir;
	}

	FET_LANGUAGE=newSettings.value("language", "en_US").toString();
	if(FET_LANGUAGE=="en_GB") //because older versions of FET used en_GB. I changed it to more usual en_US
		FET_LANGUAGE="en_US";
	WORKING_DIRECTORY=newSettings.value("working-directory", "examples").toString();
	IMPORT_DIRECTORY=newSettings.value("import-directory", OUTPUT_DIR).toString();
	
	QDir d(WORKING_DIRECTORY);
	if(!d.exists())
		WORKING_DIRECTORY="examples";
	QDir d2(WORKING_DIRECTORY);
	if(!d2.exists())
		WORKING_DIRECTORY=QDir::homePath();
	else
		WORKING_DIRECTORY=d2.absolutePath();

	QDir i(IMPORT_DIRECTORY);
	if(!i.exists())
		IMPORT_DIRECTORY=OUTPUT_DIR;	// IMPORT_DIRECTORY="import";
	
	checkForUpdates=newSettings.value("check-for-updates", "-1").toInt();
	QString ver=newSettings.value("version", "-1").toString();
	
	if(!newSettings.contains("html-level"))
		TIMETABLE_HTML_LEVEL=newSettings.value("timetable-html-level", "2").toInt();
	else
		TIMETABLE_HTML_LEVEL=newSettings.value("html-level", "2").toInt();

	PRINT_ACTIVITIES_WITH_SAME_STARTING_TIME=newSettings.value("print-activities-with-same-starting-time", "0").toInt();

	int tmp=newSettings.value("print-not-available", "1").toInt();
	PRINT_NOT_AVAILABLE_TIME_SLOTS=tmp;

	tmp=newSettings.value("print-break", "1").toInt();
	PRINT_BREAK_TIME_SLOTS=tmp;

	int tmp2=newSettings.value("divide-html-timetables-with-time-axis-by-days", "0").toInt();
	DIVIDE_HTML_TIMETABLES_WITH_TIME_AXIS_BY_DAYS=tmp2;
	
	int tt=newSettings.value("use-gui-colors", "0").toInt();
	if(tt==0)
		USE_GUI_COLORS=false;
	else
		USE_GUI_COLORS=true;

	tt=newSettings.value("show-shortcuts-on-main-window", "1").toInt();
	if(tt==0)
		SHOW_SHORTCUTS_ON_MAIN_WINDOW=false;
	else
		SHOW_SHORTCUTS_ON_MAIN_WINDOW=true;


/////////confirmations
	tt=newSettings.value("confirm-activity-planning", "1").toInt();
	if(tt==0)
		CONFIRM_ACTIVITY_PLANNING=false;
	else
		CONFIRM_ACTIVITY_PLANNING=true;

	tt=newSettings.value("confirm-spread-activities", "1").toInt();
	if(tt==0)
		CONFIRM_SPREAD_ACTIVITIES=false;
	else
		CONFIRM_SPREAD_ACTIVITIES=true;

	tt=newSettings.value("confirm-remove-redundant", "1").toInt();
	if(tt==0)
		CONFIRM_REMOVE_REDUNDANT=false;
	else
		CONFIRM_REMOVE_REDUNDANT=true;

/////////


	tt=newSettings.value("enable-activity-tag-max-hours-daily", "0").toInt();
	if(tt==0)
		ENABLE_ACTIVITY_TAG_MAX_HOURS_DAILY=false;
	else
		ENABLE_ACTIVITY_TAG_MAX_HOURS_DAILY=true;

	tt=newSettings.value("enable-students-max-gaps-per-day", "0").toInt();
	if(tt==0)
		ENABLE_STUDENTS_MAX_GAPS_PER_DAY=false;
	else
		ENABLE_STUDENTS_MAX_GAPS_PER_DAY=true;

	tt=newSettings.value("warn-if-using-not-perfect-constraints", "1").toInt();
	if(tt==0)
		SHOW_WARNING_FOR_NOT_PERFECT_CONSTRAINTS=false;
	else
		SHOW_WARNING_FOR_NOT_PERFECT_CONSTRAINTS=true;
	
	tt=newSettings.value("enable-students-min-hours-daily-with-allow-empty-days", "0").toInt();
	if(tt==0)
		ENABLE_STUDENTS_MIN_HOURS_DAILY_WITH_ALLOW_EMPTY_DAYS=false;
	else
		ENABLE_STUDENTS_MIN_HOURS_DAILY_WITH_ALLOW_EMPTY_DAYS=true;
	
	tt=newSettings.value("warn-if-using-students-min-hours-daily-with-allow-empty-days", "1").toInt();
	if(tt==0)
		SHOW_WARNING_FOR_STUDENTS_MIN_HOURS_DAILY_WITH_ALLOW_EMPTY_DAYS=false;
	else
		SHOW_WARNING_FOR_STUDENTS_MIN_HOURS_DAILY_WITH_ALLOW_EMPTY_DAYS=true;
	
	
	//main form
	QRect rect=newSettings.value("main-form-geometry", QRect(0,0,0,0)).toRect();
	if(!rect.isValid())
		rect=newSettings.value("fetmainformgeometry", QRect(0,0,0,0)).toRect();
	mainFormSettingsRect=rect;
	MAIN_FORM_SHORTCUTS_TAB_POSITION=newSettings.value("main-form-shortcuts-tab-position", "0").toInt();
	
	cout<<"Settings read"<<endl;
}

void writeSimulationParameters(){
	QSettings settings(COMPANY, PROGRAM);
	settings.setValue("output-directory", OUTPUT_DIR);
	settings.setValue("language", FET_LANGUAGE);
	settings.setValue("working-directory", WORKING_DIRECTORY);
	settings.setValue("import-directory", IMPORT_DIRECTORY);
	settings.setValue("version", FET_VERSION);
	settings.setValue("check-for-updates", checkForUpdates);
	settings.setValue("html-level", TIMETABLE_HTML_LEVEL);
	
	int qq;
	if(PRINT_ACTIVITIES_WITH_SAME_STARTING_TIME)
		qq=1;
	else
		qq=0;
	settings.setValue("print-activities-with-same-starting-time", qq);

	int k;
	if(DIVIDE_HTML_TIMETABLES_WITH_TIME_AXIS_BY_DAYS)
		k=1;
	else
		k=0;
	settings.setValue("divide-html-timetables-with-time-axis-by-days", k);
	
	int tmp;
	if(!PRINT_NOT_AVAILABLE_TIME_SLOTS)
		tmp=0;
	else
		tmp=1;
	settings.setValue("print-not-available", tmp);
	
	if(!PRINT_BREAK_TIME_SLOTS)
		tmp=0;
	else
		tmp=1;
	settings.setValue("print-break", tmp);
	
	int tt;
	if(!USE_GUI_COLORS)
		tt=0;
	else
		tt=1;
	settings.setValue("use-gui-colors", tt);
	
	if(!SHOW_SHORTCUTS_ON_MAIN_WINDOW)
		tt=0;
	else
		tt=1;
	settings.setValue("show-shortcuts-on-main-window", tt);


///////////confirmations
	if(!CONFIRM_ACTIVITY_PLANNING)
		tt=0;
	else
		tt=1;
	settings.setValue("confirm-activity-planning", tt);

	if(!CONFIRM_SPREAD_ACTIVITIES)
		tt=0;
	else
		tt=1;
	settings.setValue("confirm-spread-activities", tt);

	if(!CONFIRM_REMOVE_REDUNDANT)
		tt=0;
	else
		tt=1;
	settings.setValue("confirm-remove-redundant", tt);
///////////

	if(!ENABLE_ACTIVITY_TAG_MAX_HOURS_DAILY)
		tt=0;
	else
		tt=1;
	settings.setValue("enable-activity-tag-max-hours-daily", tt);

	if(!ENABLE_STUDENTS_MAX_GAPS_PER_DAY)
		tt=0;
	else
		tt=1;
	settings.setValue("enable-students-max-gaps-per-day", tt);

	if(!SHOW_WARNING_FOR_NOT_PERFECT_CONSTRAINTS)
		tt=0;
	else
		tt=1;
	settings.setValue("warn-if-using-not-perfect-constraints", tt);

	if(!ENABLE_STUDENTS_MIN_HOURS_DAILY_WITH_ALLOW_EMPTY_DAYS)
		tt=0;
	else
		tt=1;
	settings.setValue("enable-students-min-hours-daily-with-allow-empty-days", tt);

	if(!SHOW_WARNING_FOR_STUDENTS_MIN_HOURS_DAILY_WITH_ALLOW_EMPTY_DAYS)
		tt=0;
	else
		tt=1;
	settings.setValue("warn-if-using-students-min-hours-daily-with-allow-empty-days", tt);

	//main form
	settings.setValue("main-form-geometry", mainFormSettingsRect);
	settings.setValue("main-form-shortcuts-tab-position", MAIN_FORM_SHORTCUTS_TAB_POSITION);
}

void setLanguage(QCoreApplication& qapplication)
{
	//translator stuff
	QDir d("/usr/share/fet/translations");
	
	bool translation_loaded=false;
	
	//this is one place (out of 2) in which you need to add a new language
	if(FET_LANGUAGE=="ar" || FET_LANGUAGE=="ca" || FET_LANGUAGE=="de" || FET_LANGUAGE=="es"
	 || FET_LANGUAGE=="el" || FET_LANGUAGE=="fr" || FET_LANGUAGE=="hu" || FET_LANGUAGE=="mk"
	 || FET_LANGUAGE=="ms" || FET_LANGUAGE=="nl" || FET_LANGUAGE=="pl" || FET_LANGUAGE=="ro"
	 || FET_LANGUAGE=="tr" || FET_LANGUAGE=="id" || FET_LANGUAGE=="it" || FET_LANGUAGE=="lt"
	 || FET_LANGUAGE=="ru" || FET_LANGUAGE=="fa" || FET_LANGUAGE=="uk" || FET_LANGUAGE=="pt_BR"
	 || FET_LANGUAGE=="da" || FET_LANGUAGE=="si" || FET_LANGUAGE=="sk" || FET_LANGUAGE=="he"
	 || FET_LANGUAGE=="sr" || FET_LANGUAGE=="gl"){

		translation_loaded=translator.load("fet_"+FET_LANGUAGE, qapplication.applicationDirPath());
		if(!translation_loaded){
			translation_loaded=translator.load("fet_"+FET_LANGUAGE, qapplication.applicationDirPath()+"/translations");
			if(!translation_loaded){
				if(d.exists()){
					translation_loaded=translator.load("fet_"+FET_LANGUAGE, "/usr/share/fet/translations");
				}
			}
		}
	}
	else{
		if(FET_LANGUAGE!="en_US"){
			/*QMessageBox::warning(NULL, FetTranslate::tr("FET warning"), 
			 FetTranslate::tr("Specified language is incorrect - making it en_US (US English)"));
			FET_LANGUAGE="en_US";
		*/}
		
		assert(FET_LANGUAGE=="en_US");
		
		translation_loaded=true;
	}
	
	if(!translation_loaded){
		/*QMessageBox::warning(NULL, FetTranslate::tr("FET warning"), 
		 FetTranslate::tr("Translation for specified language not loaded - maybe translation file is missing - making language en_US (US English)")
		+"\n\n"+
		FetTranslate::tr("FET searched for translation file %1 in directories %2 and %3 (and %4 under UNIX like systems), but could not find it.")
		 .arg("fet_"+FET_LANGUAGE+".qm")
		 .arg(QDir::toNativeSeparators(qapplication.applicationDirPath()))
		 .arg(QDir::toNativeSeparators(qapplication.applicationDirPath()+"/translations"))
		 .arg("/usr/share/fet/translations")
		 );*/
		FET_LANGUAGE="en_US";
	}
	
	if(FET_LANGUAGE=="ar" || FET_LANGUAGE=="he" || FET_LANGUAGE=="fa" || FET_LANGUAGE=="ur" /* or others??? */){
		LANGUAGE_STYLE_RIGHT_TO_LEFT=true;
	}
	else{
		LANGUAGE_STYLE_RIGHT_TO_LEFT=false;
	}
	
	if(FET_LANGUAGE=="zh_CN"){
		LANGUAGE_FOR_HTML="zh-Hans";
	}
	else if(FET_LANGUAGE=="zh_TW"){
		LANGUAGE_FOR_HTML="zh-Hant";
	}
	else if(FET_LANGUAGE=="en_US"){
		LANGUAGE_FOR_HTML=FET_LANGUAGE.left(2);
	}
	else{
		LANGUAGE_FOR_HTML=FET_LANGUAGE;
		LANGUAGE_FOR_HTML.replace(QString("_"), QString("-"));
	}
		
	qapplication.installTranslator(&translator);
	
	/*QTranslator qtTranslator;
	qtTranslator.load("qt_" + FET_LANGUAGE, qapplication.applicationDirPath());
	qapplication.installTranslator(&qtTranslator);*/
//	if(LANGUAGE_STYLE_RIGHT_TO_LEFT==true)
		//qapplication.setLayoutDirection(Qt::RightToLeft);
}

/**
FET starts here
*/
int main(int argc, char **argv)
{
	QCoreApplication qapplication(argc, argv);
	
	QObject::connect(&qapplication, SIGNAL(lastWindowClosed()), &qapplication, SLOT(quit()));

	srand(unsigned(time(NULL))); //useless, I use randomKnuth(), but just in case I use somewhere rand() by mistake...

	initRandomKnuth();

	OUTPUT_DIR=QDir::homePath()+FILE_SEP+"fet-results";

	if(argc==1){
		readSimulationParameters();
	
		QDir dir;
	
		bool t=true;

		//make sure that the output directory exists
		if(!dir.exists(OUTPUT_DIR))
			t=dir.mkpath(OUTPUT_DIR);

		if(!t){
			/*QMessageBox::critical(NULL, FetTranslate::tr("FET critical"), FetTranslate::tr("Cannot create or use %1 directory - FET will now abort").arg(QDir::toNativeSeparators(OUTPUT_DIR)));
			assert(0);
			exit(1);*/
			/*QMessageBox::critical(NULL, FetTranslate::tr("FET critical"), FetTranslate::tr("Cannot create or use %1 directory (where the results should be stored) - you can continue operation, but you might not be able to work with FET."
			 " Maybe you can try to change the output directory from the 'Settings' menu. If this is a bug - please report it.").arg(QDir::toNativeSeparators(OUTPUT_DIR)));
		*/}
		
		QString testFileName=OUTPUT_DIR+FILE_SEP+"test_write_permissions_1.tmp";
		QFile test(testFileName);
		bool existedBefore=test.exists();
		bool t_t=test.open(QIODevice::ReadWrite);
		//if(!test.exists())
		//	t_t=false;
		if(!t_t){
			/*QMessageBox::critical(NULL, FetTranslate::tr("FET critical"), FetTranslate::tr("You don't have write permissions in the output directory "
			 "(FET cannot open or create file %1) - you might not be able to work correctly with FET. Maybe you can try to change the output directory from the 'Settings' menu."
			 " If this is a bug - please report it.").arg(testFileName));
		*/}
		else{
			test.close();
			if(!existedBefore)
				test.remove();
		}
	}

	students_schedule_ready=0;
	teachers_schedule_ready=0;
	rooms_schedule_ready=0;

	if(argc==1 && checkForUpdates==-1){
		/*int t=QMessageBox::question(NULL, FetTranslate::tr("FET question"),
		 FetTranslate::tr("Would you like FET to inform you of available new version by checking the FET web page?\n\n"
		 "This setting can be changed later from Settings menu\n\n"
		 ""),
		 FetTranslate::tr("&Yes"), FetTranslate::tr("&No"), QString(),
		 0, 1 );
		
		if(t==0){ //yes
			cout<<"Pressed yes"<<endl;
			checkForUpdates=1;
		}
		else{
			assert(t==1);
			cout<<"Pressed no"<<endl;
			checkForUpdates=0;
		}*/
		checkForUpdates=0;
	}


	/////////////////////////////////////////////////
	//begin command line
	if(argc>1){
		int randomSeedX=-1;
		int randomSeedY=-1;
		bool randomSeedXSpecified=false;
		bool randomSeedYSpecified=false;
	
		QString outputDirectory="";
	
		INPUT_FILENAME_XML="";
		
		QString filename="";
		
		int secondsLimit=2000000000;
		
		TIMETABLE_HTML_LEVEL=2;
		
		FET_LANGUAGE="en_US";
		
		PRINT_NOT_AVAILABLE_TIME_SLOTS=true;
		
		PRINT_BREAK_TIME_SLOTS=true;
		
		DIVIDE_HTML_TIMETABLES_WITH_TIME_AXIS_BY_DAYS=false;

		PRINT_ACTIVITIES_WITH_SAME_STARTING_TIME=false;
		
		QStringList unrecognizedOptions;
                
                SHOW_WARNING_FOR_STUDENTS_MIN_HOURS_DAILY_WITH_ALLOW_EMPTY_DAYS=true;
		
		SHOW_WARNING_FOR_NOT_PERFECT_CONSTRAINTS=true;


		
		bool showVersion=false;

		for(int i=1; i<argc; i++){
			QString s=argv[i];
			
			if(s.left(12)=="--inputfile=")
				filename=s.right(s.length()-12);
			else if(s.left(19)=="--timelimitseconds=")
				secondsLimit=s.right(s.length()-19).toInt();
			else if(s.left(21)=="--timetablehtmllevel=")
				TIMETABLE_HTML_LEVEL=s.right(s.length()-21).toInt();
			else if(s.left(12)=="--htmllevel=")
				TIMETABLE_HTML_LEVEL=s.right(s.length()-12).toInt();
			else if(s.left(11)=="--language=")
				FET_LANGUAGE=s.right(s.length()-11);
			else if(s.left(20)=="--printnotavailable="){
				if(s.right(5)=="false")
					PRINT_NOT_AVAILABLE_TIME_SLOTS=false;
				else
					PRINT_NOT_AVAILABLE_TIME_SLOTS=true;
			}
			else if(s.left(13)=="--printbreak="){
				if(s.right(5)=="false")
					PRINT_BREAK_TIME_SLOTS=false;
				else
					PRINT_BREAK_TIME_SLOTS=true;
			}
			else if(s.left(23)=="--dividetimeaxisbydays="){
				if(s.right(5)=="false")
					DIVIDE_HTML_TIMETABLES_WITH_TIME_AXIS_BY_DAYS=false;
				else
					DIVIDE_HTML_TIMETABLES_WITH_TIME_AXIS_BY_DAYS=true;
			}
			else if(s.left(12)=="--outputdir="){
				outputDirectory=s.right(s.length()-12);
			}
			else if(s.left(30)=="--printsimultaneousactivities="){
				if(s.right(5)=="false")
					PRINT_ACTIVITIES_WITH_SAME_STARTING_TIME=false;
				else
					PRINT_ACTIVITIES_WITH_SAME_STARTING_TIME=true;
			}
			else if(s.left(14)=="--randomseedx="){
				randomSeedXSpecified=true;
				randomSeedX=s.right(s.length()-14).toInt();
			}
			else if(s.left(14)=="--randomseedy="){
				randomSeedYSpecified=true;
				randomSeedY=s.right(s.length()-14).toInt();
			}
			else if(s.left(35)=="--warnifusingnotperfectconstraints="){
				if(s.right(5)=="false")
					SHOW_WARNING_FOR_NOT_PERFECT_CONSTRAINTS=false;
			}
			else if(s.left(53)=="--warnifusingstudentsminhoursdailywithallowemptydays="){
				if(s.right(5)=="false")
					SHOW_WARNING_FOR_STUDENTS_MIN_HOURS_DAILY_WITH_ALLOW_EMPTY_DAYS=false;
			}
			else if(s=="--version"){
				showVersion=true;
			}
			else
				unrecognizedOptions.append(s);
		}
		
		INPUT_FILENAME_XML=filename;
		
		QString initialDir=outputDirectory;
		if(initialDir!="")
			initialDir.append(FILE_SEP);
		
		if(outputDirectory!="")
			outputDirectory.append(FILE_SEP);
		outputDirectory.append("timetables");

		//////////
		if(INPUT_FILENAME_XML!=""){
			outputDirectory.append(FILE_SEP);
			outputDirectory.append(INPUT_FILENAME_XML.right(INPUT_FILENAME_XML.length()-INPUT_FILENAME_XML.findRev(FILE_SEP)-1));
			if(outputDirectory.right(4)==".fet")
				outputDirectory=outputDirectory.left(outputDirectory.length()-4);
			//else if(INPUT_FILENAME_XML!="")
			//	cout<<"Minor problem - input file does not end in .fet extension - might be a problem when saving the timetables"<<" (file:"<<__FILE__<<", line:"<<__LINE__<<")"<<endl;
		}
		//////////
		
		QDir dir;
		QString logsDir=initialDir+"logs";
		if(!dir.exists(logsDir))
			dir.mkpath(logsDir);
		logsDir.append(FILE_SEP);
		
		////////
		QFile logFile(logsDir+"result.txt");
		bool tttt=logFile.open(QIODevice::WriteOnly);
		if(!tttt){
			cout<<"FET critical - you don't have write permissions in the output directory - (FET cannot open or create file "<<qPrintable(logsDir)<<"result.txt)."
			 " If this is a bug - please report it."<<endl;
			return 1;
		}
		QTextStream out(&logFile);
		//ofstream out(logsDir+"result.txt");
		///////
		
		setLanguage(qapplication);
		
		if(showVersion){
			out<<"This file contains the result (log) of last operation"<<endl<<endl;
		
			QDate dat=QDate::currentDate();
			QTime tim=QTime::currentTime();
			QLocale loc(FET_LANGUAGE);
			QString sTime=loc.toString(dat, QLocale::ShortFormat)+" "+loc.toString(tim, QLocale::ShortFormat);
			out<<"FET command line request for version started on "<<qPrintable(sTime)<<endl<<endl;
	
			//QString qv=qVersion();
			out<<"FET version "<<qPrintable(FET_VERSION)<<endl;
			out<<"Free timetabling software, licensed under GNU GPL v2 or later"<<endl;
			out<<"Copyright (C) 2002-2010 Liviu Lalescu"<<endl;
			out<<"Homepage: http://lalescu.ro/liviu/fet/"<<endl;
			//out<<" (Using Qt version "<<qPrintable(qv)<<")"<<endl;
			cout<<"FET version "<<qPrintable(FET_VERSION)<<endl;
			cout<<"Free timetabling software, licensed under GNU GPL v2 or later"<<endl;
			cout<<"Copyright (C) 2002-2010 Liviu Lalescu"<<endl;
			cout<<"Homepage: http://lalescu.ro/liviu/fet/"<<endl;
			//cout<<" (Using Qt version "<<qPrintable(qv)<<")"<<endl;

			if(unrecognizedOptions.count()>0){
				out<<endl;
				cout<<endl;
				foreach(QString s, unrecognizedOptions){
					cout<<"Unrecognized option: "<<qPrintable(s)<<endl;
					out<<"Unrecognized option: "<<qPrintable(s)<<endl;
				}
			}

			logFile.close();
			return 0;
		}
		
		QFile maxPlacedActivityFile(logsDir+"max_placed_activities.txt");
		maxPlacedActivityFile.open(QIODevice::WriteOnly);
		QTextStream maxPlacedActivityStream(&maxPlacedActivityFile);
		maxPlacedActivityStream.setCodec("UTF-8");
		maxPlacedActivityStream.setGenerateByteOrderMark(true);
		//maxPlacedActivityStream<<FetTranslate::tr("This is the list of max placed activities, chronologically. If FET could reach maximum n-th activity, look at the n+1-st activity"
//			" in the initial order of the activities")<<endl<<endl;
				
		QFile initialOrderFile(logsDir+"initial_order.txt");
		initialOrderFile.open(QIODevice::WriteOnly);
		QTextStream initialOrderStream(&initialOrderFile);
		initialOrderStream.setCodec("UTF-8");
		initialOrderStream.setGenerateByteOrderMark(true);
						
		out<<"This file contains the result (log) of last operation"<<endl<<endl;
		
		QDate dat=QDate::currentDate();
		QTime tim=QTime::currentTime();
		QLocale loc(FET_LANGUAGE);
		QString sTime=loc.toString(dat, QLocale::ShortFormat)+" "+loc.toString(tim, QLocale::ShortFormat);
		out<<"FET command line simulation started on "<<qPrintable(sTime)<<endl<<endl;
		
		if(unrecognizedOptions.count()>0){
			foreach(QString s, unrecognizedOptions){
				cout<<"Unrecognized option: "<<qPrintable(s)<<endl;
				out<<"Unrecognized option: "<<qPrintable(s)<<endl;
			}
			cout<<endl;
			out<<endl;
		}
		
		if(outputDirectory!="")
			if(!dir.exists(outputDirectory))
				dir.mkpath(outputDirectory);
		
		if(outputDirectory!="")
			outputDirectory.append(FILE_SEP);
			
		QFile test(outputDirectory+"test_write_permissions_2.tmp");
		bool existedBefore=test.exists();
		bool t_t=test.open(QIODevice::ReadWrite);
		if(!t_t){
			cout<<"fet: critical error - you don't have write permissions in the output directory - (FET cannot open or create file "<<qPrintable(outputDirectory)<<"test_write_permissions_2.tmp)."
			 " If this is a bug - please report it."<<endl;
			out<<"fet: critical error - you don't have write permissions in the output directory - (FET cannot open or create file "<<qPrintable(outputDirectory)<<"test_write_permissions_2.tmp)."
			 " If this is a bug - please report it."<<endl;
			//test.close();
			return 1;
		}
		else{
			test.close();
			if(!existedBefore)
				test.remove();
		}

		if(filename==""){
			usage(out, QString("Input file not specified"));
			logFile.close();
			return 1;
		}
		if(secondsLimit==0){
			usage(out, QString("Time limit is 0 seconds"));
			logFile.close();
			return 1;
		}
		if(TIMETABLE_HTML_LEVEL>6 || TIMETABLE_HTML_LEVEL<0){
			usage(out, QString("Html level must be 0, 1, 2, 3, 4, 5 or 6"));
			logFile.close();
			return 1;
		}
		if(randomSeedXSpecified != randomSeedYSpecified){
			if(randomSeedXSpecified){
				usage(out, QString("If you want to specify the random seed, you need to specify both the X and the Y components, not only the X component"));
			}
			else{
				assert(randomSeedYSpecified);
				usage(out, QString("If you want to specify the random seed, you need to specify both the X and the Y components, not only the Y component"));
			}
			logFile.close();
			return 1;
		}
		assert(randomSeedXSpecified==randomSeedYSpecified);
		if(randomSeedXSpecified){
			if(randomSeedX<=0 || randomSeedX>=MM){
				usage(out, QString("Random seed X component must be at least 1 and at most %1").arg(MM-1));
				logFile.close();
				return 1;
			}
		}
		if(randomSeedYSpecified){
			if(randomSeedY<=0 || randomSeedY>=MMM){
				usage(out, QString("Random seed Y component must be at least 1 and at most %1").arg(MMM-1));
				logFile.close();
				return 1;
			}
		}
		
		if(randomSeedXSpecified){
			assert(randomSeedYSpecified);
			if(randomSeedX>0 && randomSeedX<MM && randomSeedY>0 && randomSeedY<MMM){
				XX=randomSeedX;
				YY=randomSeedY;
			}
		}
		
		if(TIMETABLE_HTML_LEVEL>6 || TIMETABLE_HTML_LEVEL<0)
			TIMETABLE_HTML_LEVEL=2;
		bool t=gt.rules.read(filename, true, initialDir);
		if(!t){
			cout<<"fet: cannot read input file (not existing or in use) - aborting"<<endl;
			out<<"Cannot read input file (not existing or in use) - aborting"<<endl;
			logFile.close();
			return 1;
		}
		t=gt.rules.computeInternalStructure();
		if(!t){
			cout<<"Cannot compute internal structure - aborting"<<endl;
			out<<"Cannot compute internal structure - aborting"<<endl;
			logFile.close();
			return 1;
		}
	
		Generate gen;
	
		gen.abortOptimization=false;
		bool ok=gen.precompute(&initialOrderStream);
		
		initialOrderFile.close();
		
		if(!ok){
			cout<<"Cannot precompute - data is wrong - aborting"<<endl;
			out<<"Cannot precompute - data is wrong - aborting"<<endl;
			logFile.close();
			return 1;
		}
	
		bool impossible, timeExceeded;
		cout<<"secondsLimit=="<<secondsLimit<<endl;
		//out<<"secondsLimit=="<<secondsLimit<<endl;
		TimetableExport::writeRandomSeedCommandLine(outputDirectory);

		gen.generate(secondsLimit, impossible, timeExceeded, false, &maxPlacedActivityStream); //false means no thread
		
		maxPlacedActivityFile.close();
	
		if(impossible){
			cout<<"Impossible"<<endl;
			out<<"Impossible"<<endl;
		}
		else if(timeExceeded){
			cout<<"Time exceeded"<<endl;
			out<<"Time exceeded"<<endl;
		}
		else{
			cout<<"Simulation successful"<<endl;
			out<<"Simulation successful"<<endl;
		
			Solution& c=gen.c;

			//needed to find the conflicts strings
			QString tmp;
			c.fitness(gt.rules, &tmp);
			
			TimetableExport::getStudentsTimetable(c);
			TimetableExport::getTeachersTimetable(c);
			TimetableExport::getRoomsTimetable(c);

			TimetableExport::writeSimulationResultsCommandLine(outputDirectory);
		}
	
		logFile.close();
		return 0;
	}
	//end command line
	/////////////////////////////////////////////////

	setLanguage(qapplication);

	pqapplication=&qapplication;
	//FetMainForm fetMainForm;
	//qapplication.setMainWidget(&fetMainForm);
	//fetMainForm.show();
	//fetMainForm.updateLogo();
	/*fetMainForm.resize(fetMainForm.size().width(), fetMainForm.size().height()+1); //to show correctly the logo
	fetMainForm.resize(fetMainForm.size().width(), fetMainForm.size().height()-1);*/

	int tmp2=qapplication.exec();
	
	writeSimulationParameters();
	
	cout<<"Settings saved"<<endl;
	
	return tmp2;
}
