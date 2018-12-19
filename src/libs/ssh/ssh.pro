QT += gui network
DEFINES += QTCSSH_LIBRARY

include(../../qtcreatorlibrary.pri)

SOURCES = \
    sftpdefs.cpp \
    sftpfilesystemmodel.cpp \
    sftpsession.cpp \
    sftptransfer.cpp \
    sshconnection.cpp \
    sshconnectionmanager.cpp \
    sshkeycreationdialog.cpp \
    sshlogging.cpp \
    sshprocess.cpp \
    sshremoteprocess.cpp \
    sshremoteprocessrunner.cpp \
    sshsettings.cpp

HEADERS = \
    sftpdefs.h \
    sftpfilesystemmodel.h \
    sftpsession.h \
    sftptransfer.h \
    sshconnection.h \
    sshconnectionmanager.h \
    sshkeycreationdialog.h \
    sshlogging_p.h \
    sshprocess.h \
    sshremoteprocess.h \
    sshremoteprocessrunner.h \
    sshsettings.h \
    ssh_global.h

FORMS = $$PWD/sshkeycreationdialog.ui

RESOURCES += $$PWD/ssh.qrc
