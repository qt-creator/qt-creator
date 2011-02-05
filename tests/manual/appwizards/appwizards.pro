CREATORSOURCEDIR = ../../../

DEFINES += \
    CREATORLESSTEST
APPSOURCEDIR = $$CREATORSOURCEDIR/src/plugins/qt4projectmanager/wizards
HEADERS += \
    $$APPSOURCEDIR/qmlstandaloneapp.h \
    $$APPSOURCEDIR/html5app.h \
    $$APPSOURCEDIR/abstractmobileapp.h
SOURCES += \
    $$APPSOURCEDIR/qmlstandaloneapp.cpp \
    $$APPSOURCEDIR/html5app.cpp \
    $$APPSOURCEDIR/abstractmobileapp.cpp \
    main.cpp \
    helpers.cpp
INCLUDEPATH += $$APPSOURCEDIR
