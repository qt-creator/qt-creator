CREATORSOURCEDIR = ../../../

DEFINES += \
    CREATORLESSTEST
APPSOURCEDIR = $$CREATORSOURCEDIR/src/plugins/qt4projectmanager/wizards
HEADERS += \
    $$APPSOURCEDIR/qmlstandaloneapp.h
SOURCES += \
    $$APPSOURCEDIR/qmlstandaloneapp.cpp \
    main.cpp
INCLUDEPATH += $$APPSOURCEDIR
