//
//
// Description: This file is part of FET
//
//
// Author: Liviu Lalescu <Please see http://lalescu.ro/liviu/ for details about contacting Liviu Lalescu (in particular, you can find here the e-mail address)>
// Copyright (C) 2003 Liviu Lalescu <http://lalescu.ro/liviu/>
//
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
//
//

#ifndef TEACHER_H
#define TEACHER_H

#include <QCoreApplication>

#include "timetable_defs.h"

#include <QString>
#include <QList>

class Teacher;
class Rules;

#include <QList>

typedef QList<Teacher*> TeachersList;

/**
@author Liviu Lalescu
*/
class Teacher
{
	Q_DECLARE_TR_FUNCTIONS(Teacher)
	
public:
	QList<qint16> activitiesForTeacher;

	QString name;

	Teacher();
	~Teacher();

	QString getXmlDescription();
	QString getDetailedDescription();
	QString getDetailedDescriptionWithConstraints(Rules& r);
};

int teachersAscending(const Teacher* t1, const Teacher* t2);

#endif
