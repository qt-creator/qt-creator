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
    $$CREATORSOURCEDIR/tests/manual/qmlstandalone/helpers.cpp
INCLUDEPATH += $$APPSOURCEDIR
OTHER_FILES = qtquickapps.xml
RESOURCES += \
    qtquickappblaster.qrc
