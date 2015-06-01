QT += core network testlib
QT -= gui

TEMPLATE = app

CONFIG   += console c++14 testcase
CONFIG   -= app_bundle

include(../../../../qtcreator.pri)

LIBS += -L$$OUT_PWD/../codemodelbackendipc/lib/qtcreator -lCodemodelbackendipc
unix:LIBS += -ldl

INCLUDEPATH += $$PWD/../source

QMAKE_CXXFLAGS -= -O2
#unix:QMAKE_CXXFLAGS += -fprofile-arcs -ftest-coverage -O0
#unix:QMAKE_LDFLAGS += -fprofile-arcs -ftest-coverage
#unix:LIBS += -lgcov

DEFINES += QT_NO_CAST_FROM_ASCII
DEFINES += CODEMODELBACKENDPROCESS_TESTS
DEFINES += CODEMODELBACKENDPROCESSPATH=\\\"$$OUT_PWD/../codemodelbackendprocess/codemodelbackend\\\"
