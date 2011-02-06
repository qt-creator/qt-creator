CREATORSOURCEDIR = ../../../

DEFINES += \
    CREATORLESSTEST
APPSOURCEDIR = $$CREATORSOURCEDIR/src/plugins/qt4projectmanager/wizards
HEADERS += \
    $$APPSOURCEDIR/qtquickapp.h \
    $$APPSOURCEDIR/html5app.h \
    $$APPSOURCEDIR/abstractmobileapp.h
SOURCES += \
    $$APPSOURCEDIR/qtquickapp.cpp \
    $$APPSOURCEDIR/html5app.cpp \
    $$APPSOURCEDIR/abstractmobileapp.cpp \
    main.cpp \
    $$CREATORSOURCEDIR/tests/manual/appwizards/helpers.cpp
INCLUDEPATH += $$APPSOURCEDIR
OTHER_FILES = qtquickapps.xml
RESOURCES += \
    qtquickappblaster.qrc
