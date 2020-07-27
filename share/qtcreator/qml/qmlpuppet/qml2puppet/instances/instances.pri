INCLUDEPATH += $$PWD/

versionAtLeast(QT_VERSION, 5.15.0):qtHaveModule(quick3d) {
    QT *= quick3d-private
    DEFINES *= QUICK3D_MODULE
}

HEADERS += $$PWD/qt5nodeinstanceserver.h \
    $$PWD/qt5capturenodeinstanceserver.h \
    $$PWD/qt5testnodeinstanceserver.h \
    $$PWD/qt5informationnodeinstanceserver.h \
    $$PWD/qt5rendernodeinstanceserver.h \
    $$PWD/qt5previewnodeinstanceserver.h \
    $$PWD/qt5nodeinstanceclientproxy.h \
    $$PWD/quickitemnodeinstance.h \
    $$PWD/behaviornodeinstance.h \
    $$PWD/dummycontextobject.h \
    $$PWD/childrenchangeeventfilter.h \
    $$PWD/componentnodeinstance.h \
    $$PWD/dummynodeinstance.h \
    $$PWD/nodeinstanceserver.h \
    $$PWD/nodeinstancesignalspy.h \
    $$PWD/objectnodeinstance.h \
    $$PWD/qmlpropertychangesnodeinstance.h \
    $$PWD/qmlstatenodeinstance.h \
    $$PWD/qmltransitionnodeinstance.h \
    $$PWD/servernodeinstance.h \
    $$PWD/anchorchangesnodeinstance.h \
    $$PWD/positionernodeinstance.h \
    $$PWD/layoutnodeinstance.h \
    $$PWD/qt3dpresentationnodeinstance.h \
    $$PWD/quick3dnodeinstance.h

SOURCES += $$PWD/qt5nodeinstanceserver.cpp \
    $$PWD/qt5capturenodeinstanceserver.cpp \
    $$PWD/qt5testnodeinstanceserver.cpp \
    $$PWD/qt5informationnodeinstanceserver.cpp \
    $$PWD/qt5rendernodeinstanceserver.cpp \
    $$PWD/qt5previewnodeinstanceserver.cpp \
    $$PWD/qt5nodeinstanceclientproxy.cpp \
    $$PWD/quickitemnodeinstance.cpp \
    $$PWD/behaviornodeinstance.cpp \
    $$PWD/dummycontextobject.cpp \
    $$PWD/childrenchangeeventfilter.cpp \
    $$PWD/componentnodeinstance.cpp \
    $$PWD/dummynodeinstance.cpp \
    $$PWD/nodeinstanceserver.cpp \
    $$PWD/nodeinstancesignalspy.cpp \
    $$PWD/objectnodeinstance.cpp \
    $$PWD/qmlpropertychangesnodeinstance.cpp \
    $$PWD/qmlstatenodeinstance.cpp \
    $$PWD/qmltransitionnodeinstance.cpp \
    $$PWD/servernodeinstance.cpp \
    $$PWD/anchorchangesnodeinstance.cpp \
    $$PWD/positionernodeinstance.cpp \
    $$PWD/layoutnodeinstance.cpp \
    $$PWD/qt3dpresentationnodeinstance.cpp \
    $$PWD/quick3dnodeinstance.cpp
