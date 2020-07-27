INCLUDEPATH += $$PWD/

HEADERS += $$PWD/../include/nodeinstance.h \
    $$PWD/baseconnectionmanager.h \
    $$PWD/capturingconnectionmanager.h \
    $$PWD/connectionmanager.h \
    $$PWD/connectionmanagerinterface.h \
    $$PWD/interactiveconnectionmanager.h \
    $$PWD/nodeinstanceserverproxy.h \
    $$PWD/puppetcreator.h \
    $$PWD/puppetbuildprogressdialog.h \
    $$PWD/qprocessuniqueptr.h

SOURCES +=  $$PWD/nodeinstanceserverproxy.cpp \
    $$PWD/baseconnectionmanager.cpp \
    $$PWD/capturingconnectionmanager.cpp \
    $$PWD/connectionmanager.cpp \
    $$PWD/connectionmanagerinterface.cpp \
    $$PWD/interactiveconnectionmanager.cpp \
    $$PWD/nodeinstance.cpp \
    $$PWD/nodeinstanceview.cpp \
    $$PWD/puppetcreator.cpp \
    $$PWD/puppetbuildprogressdialog.cpp

FORMS += $$PWD/puppetbuildprogressdialog.ui
