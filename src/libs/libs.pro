TEMPLATE  = subdirs
CONFIG   += ordered
QT += core gui

SUBDIRS   = \
    qtconcurrent \
    aggregation \
    extensionsystem \
    utils \
    utils/process_stub.pro \
    cplusplus \
    qmljs \
    glsl \
    qmleditorwidgets \
    symbianutils \
    3rdparty

win32:SUBDIRS += qtcreatorcdbext
