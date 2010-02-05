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
    symbianutils

SUPPORT_QT_MAEMO = $$(QTCREATOR_WITH_MAEMO)
!isEmpty(SUPPORT_QT_MAEMO) {
SUBDIRS += 3rdparty
message("Adding experimental ssh support for Qt/Maemo applications.")
}
