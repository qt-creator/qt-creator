TEMPLATE  = subdirs
CONFIG   += ordered
QT += core gui

# aggregation and extensionsystem are directly in src.pro
# because of dependencies of app
SUBDIRS   = \
    utils \
    utils/process_stub.pro \
    languageutils \
    cplusplus \
    qmljs \
    qmldebug \
    glsl \
    qmleditorwidgets \
    qtcomponents/styleitem \
    ssh \
    zeroconf

exists(../shared/qbs/qbs.pro):SUBDIRS += \
    ../shared/qbs/src/lib \
    ../shared/qbs/src/plugins \
    ../shared/qbs/static.pro

win32:SUBDIRS += utils/process_ctrlc_stub.pro

# Windows: Compile Qt Creator CDB extension if Debugging tools can be detected.    
win32 {
    include(qtcreatorcdbext/cdb_detect.pri)
    exists($$CDB_PATH):SUBDIRS += qtcreatorcdbext
}
