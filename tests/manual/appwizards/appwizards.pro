CREATORSOURCEDIR = ../../../
include ($$CREATORSOURCEDIR/qtcreator.pri)

DEFINES += \
    CREATORLESSTEST
APPSOURCEDIR = $$CREATORSOURCEDIR/src/plugins/qt4projectmanager/wizards
LIBS *= -L$$IDE_LIBRARY_PATH -lUtils
HEADERS += \
    $$APPSOURCEDIR/qtquickapp.h \
    $$APPSOURCEDIR/html5app.h \
    $$APPSOURCEDIR/abstractmobileapp.h
SOURCES += \
    $$APPSOURCEDIR/qtquickapp.cpp \
    $$APPSOURCEDIR/html5app.cpp \
    $$APPSOURCEDIR/abstractmobileapp.cpp \
    main.cpp \
    helpers.cpp
INCLUDEPATH += $$APPSOURCEDIR
