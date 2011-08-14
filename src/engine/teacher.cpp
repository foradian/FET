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

#include "teacher.h"
#include "rules.h"

Teacher::Teacher()
{
}

Teacher::~Teacher()
{
}

QString Teacher::getXmlDescription()
{
	QString s="<Teacher>\n";
	s+="	<Name>"+protect(this->name)+"</Name>\n";
	s+="</Teacher>\n";

	return s;
}

QString Teacher::getDetailedDescription()
{
	QString s=tr("Teacher");
	s+="\n";
	s+=tr("Name=%1", "The name of the teacher").arg(this->name);
	s+="\n";

	return s;
}

QString Teacher::getDetailedDescriptionWithConstraints(Rules& r)
{
	QString s=this->getDetailedDescription();

	s+="--------------------------------------------------\n";
	s+=tr("Time constraints directly related to this teacher:");
	s+="\n";
	for(int i=0; i<r.timeConstraintsList.size(); i++){
		TimeConstraint* c=r.timeConstraintsList[i];
		if(c->isRelatedToTeacher(this)){
			s+="\n";
			s+=c->getDetailedDescription(r);
		}
	}

	s+="--------------------------------------------------\n";
	s+=tr("Space constraints directly related to this teacher:");
	s+="\n";
	for(int i=0; i<r.spaceConstraintsList.size(); i++){
		SpaceConstraint* c=r.spaceConstraintsList[i];
		if(c->isRelatedToTeacher(this)){
			s+="\n";
			s+=c->getDetailedDescription(r);
		}
	}
	s+="--------------------------------------------------\n";

	return s;
}

int teachersAscending(const Teacher* t1, const Teacher* t2)
{
	return t1->name < t2->name;
}
