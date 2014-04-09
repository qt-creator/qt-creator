INCLUDEPATH += $$PWD/

HEADERS += $$PWD/../include/nodeinstance.h
HEADERS += $$PWD/nodeinstanceserverproxy.h
HEADERS += $$PWD/puppetcreator.h
HEADERS += $$PWD/puppetbuildprogressdialog.h

SOURCES +=  $$PWD/nodeinstanceserverproxy.cpp
SOURCES +=  $$PWD/nodeinstance.cpp
SOURCES +=  $$PWD/nodeinstanceview.cpp
SOURCES += $$PWD/puppetcreator.cpp
SOURCES += $$PWD/puppetbuildprogressdialog.cpp

FORMS += $$PWD/puppetbuildprogressdialog.ui
