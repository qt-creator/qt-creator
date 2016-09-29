INCLUDEPATH += $$PWD/

HEADERS += $$PWD/addimportcontainer.h
HEADERS += $$PWD/mockuptypecontainer.h
HEADERS += $$PWD/sharedmemory.h
HEADERS += $$PWD/imagecontainer.h
HEADERS += $$PWD/idcontainer.h
HEADERS += $$PWD/informationcontainer.h
HEADERS += $$PWD/instancecontainer.h
HEADERS += $$PWD/reparentcontainer.h
HEADERS += $$PWD/propertyabstractcontainer.h
HEADERS += $$PWD/propertybindingcontainer.h
HEADERS += $$PWD/propertyvaluecontainer.h

SOURCES += $$PWD/addimportcontainer.cpp
SOURCES += $$PWD/mockuptypecontainer.cpp
unix:SOURCES += $$PWD/sharedmemory_unix.cpp
!unix:SOURCES += $$PWD/sharedmemory_qt.cpp
SOURCES += $$PWD/imagecontainer.cpp
SOURCES += $$PWD/idcontainer.cpp
SOURCES += $$PWD/informationcontainer.cpp
SOURCES += $$PWD/instancecontainer.cpp
SOURCES += $$PWD/reparentcontainer.cpp
SOURCES += $$PWD/propertyabstractcontainer.cpp
SOURCES += $$PWD/propertybindingcontainer.cpp
SOURCES += $$PWD/propertyvaluecontainer.cpp
