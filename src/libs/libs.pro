TEMPLATE  = subdirs
CONFIG   += ordered
QT += core gui

SUBDIRS   = \
    qtconcurrent \
    aggregation \
    extensionsystem \
    utils \
    utils/process_stub.pro \
    languageutils \
    cplusplus \
    qmljs \
    glsl \
    qmleditorwidgets \
    symbianutils \
    3rdparty

# Windows: Compile Qt Creator CDB extension if Debugging tools can be detected.    
win32 {
    include(qtcreatorcdbext/cdb_detect.pri)
    !isEmpty(CDB_PATH):SUBDIRS += qtcreatorcdbext
}
