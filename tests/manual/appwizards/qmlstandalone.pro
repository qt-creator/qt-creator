CREATORSOURCEDIR = ../../../

DEFINES += \
    CREATORLESSTEST
APPSOURCEDIR = $$CREATORSOURCEDIR/src/plugins/qt4projectmanager/wizards
HEADERS += \
    $$APPSOURCEDIR/qmlstandaloneapp.h \
    $$APPSOURCEDIR/abstractmobileapp.h
SOURCES += \
    $$APPSOURCEDIR/qmlstandaloneapp.cpp \
    $$APPSOURCEDIR/abstractmobileapp.cpp \
    main.cpp \
    helpers.cpp
INCLUDEPATH += $$APPSOURCEDIR
