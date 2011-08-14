SOURCES += \
		   engine/timetableexport.cpp \
           engine/activity.cpp \
           engine/solution.cpp \
           engine/timetable.cpp \
           engine/rules.cpp \
           engine/generate_pre.cpp \
           engine/timeconstraint.cpp \
           engine/spaceconstraint.cpp \
           engine/studentsset.cpp \
           engine/teacher.cpp \
           engine/subject.cpp \
           engine/activitytag.cpp \
           engine/room.cpp \
           engine/building.cpp \
           engine/timetable_defs.cpp \
		   engine/generate.cpp \
		   engine/statisticsexport.cpp \
		   engine/lockunlock.cpp \
\
           interface/fet.cpp 
HEADERS += \
		   engine/timetableexport.h \
           engine/activity.h \
           engine/solution.h \
           engine/timetable.h \
           engine/rules.h \
           engine/generate_pre.h \
           engine/timeconstraint.h \
           engine/spaceconstraint.h \
           engine/timetable_defs.h \
           engine/studentsset.h \
           engine/teacher.h \
           engine/subject.h \
           engine/activitytag.h \
           engine/room.h \
           engine/building.h \
		   engine/generate.h \
		   engine/statisticsexport.h \
		   engine/lockunlock.h \
		   engine/matrix.h \
			\
			interface/fet.h \

TRANSLATIONS += \
		../translations/fet_ar.ts \
		../translations/fet_ca.ts \
		../translations/fet_da.ts \
		../translations/fet_de.ts \
		../translations/fet_el.ts \
		../translations/fet_es.ts \
		../translations/fet_fa.ts \
		../translations/fet_fr.ts \
		../translations/fet_gl.ts \
		../translations/fet_he.ts \
		../translations/fet_hu.ts \
		../translations/fet_id.ts \
		../translations/fet_it.ts \
		../translations/fet_lt.ts \
		../translations/fet_mk.ts \
		../translations/fet_ms.ts \
		../translations/fet_nl.ts \
		../translations/fet_pl.ts \
		../translations/fet_pt_BR.ts \
		../translations/fet_ro.ts \
		../translations/fet_ru.ts \
		../translations/fet_si.ts \
		../translations/fet_sk.ts \
		../translations/fet_sr.ts \
		../translations/fet_tr.ts \
		../translations/fet_uk.ts \
		../translations/fet_untranslated.ts

TEMPLATE = app
CONFIG += release \
          warn_on \
          qt \
          thread
DESTDIR = ..
TARGET = fet
OBJECTS_DIR = ../tmp
UI_DIR = ../tmp
MOC_DIR = ../tmp
RCC_DIR = ../tmp
INCLUDEPATH += engine interface
DEPENDPATH += engine interface
QT += xml qt3support network
