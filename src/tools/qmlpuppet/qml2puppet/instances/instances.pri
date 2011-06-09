INCLUDEPATH += $$PWD/

HEADERS += $$PWD/qt5nodeinstanceserver.h \
    instances/qt5informationnodeinstanceserver.h \
    instances/qt5rendernodeinstanceserver.h \
    instances/qt5previewnodeinstanceserver.h
HEADERS += $$PWD/qt5nodeinstanceclientproxy.h
HEADERS += $$PWD/sgitemnodeinstance.h

SOURCES += $$PWD/qt5nodeinstanceserver.cpp \
    instances/qt5informationnodeinstanceserver.cpp \
    instances/qt5rendernodeinstanceserver.cpp \
    instances/qt5previewnodeinstanceserver.cpp
SOURCES += $$PWD/qt5nodeinstanceclientproxy.cpp
SOURCES += $$PWD/sgitemnodeinstance.cpp
