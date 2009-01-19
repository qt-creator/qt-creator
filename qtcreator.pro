#version check qt
TOO_OLD_LIST=$$find(QT_VERSION, ^4\.[0-4])
count(TOO_OLD_LIST, 1) {
    message("Cannot build the Qt Creator with a Qt version that old:" $$QT_VERSION)
    error("Use at least Qt 4.5.")
}


linux-* {
	isEmpty( LOCATION )  {
		LOCATION = /usr/share
	}
	documentation.files     += doc/qtcreator.qch
	documentation.path       = $$LOCATION/share/qtcreator/doc/qtcreator

	share.files         += share/qtcreator/*
	share.parth			 = $$LOCATION/share/qtcreator


	INSTALLS += \
		documentation \
		share		
}

TEMPLATE  = subdirs
CONFIG   += ordered

SUBDIRS = src

include(doc/doc.pri)
