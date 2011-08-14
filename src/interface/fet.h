/*
File fet.h
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


#ifndef FET_H
#define FET_H


#include "timetable_defs.h"
#include "timetable.h"

#include "matrix.h"

#include <fstream>
using namespace std;

#include <QTextStream>

class FetTranslate: public QObject{
	Q_OBJECT
};

/**
The one and only instantiation of the main class.
*/
extern Timetable gt;

/**
Log file
*/
extern ofstream logg;

/**
The name of the file from where the rules are read.
*/
extern QString INPUT_FILENAME_XML;
     

/**
The timetable for the teachers
*/
//extern qint16 teachers_timetable_weekly[MAX_TEACHERS][MAX_DAYS_PER_WEEK][MAX_HOURS_PER_DAY];
extern Matrix3D<qint16> teachers_timetable_weekly;

/**
The timetable for the students
*/
//extern qint16 students_timetable_weekly[MAX_TOTAL_SUBGROUPS][MAX_DAYS_PER_WEEK][MAX_HOURS_PER_DAY];
extern Matrix3D<qint16> students_timetable_weekly;

/**
The timetable for the rooms
*/
//extern qint16 rooms_timetable_weekly[MAX_ROOMS][MAX_DAYS_PER_WEEK][MAX_HOURS_PER_DAY];
extern Matrix3D<qint16> rooms_timetable_weekly;

void readSimulationParameters();
void writeSimulationParameters();
//void writeDefaultSimulationParameters();

void usage(QTextStream& out);

/**
The main function.
*/
int main(int argc, char **argv);


#endif
