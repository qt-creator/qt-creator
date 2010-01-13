TEMPLATE  = subdirs
CONFIG   += ordered

SUBDIRS   = \
    qtconcurrent \
    aggregation \
    extensionsystem \
    utils \
    utils/process_stub.pro \
    cplusplus \
    qml
    
SUPPORT_QT_MAEMO = $$(QTCREATOR_WITH_MAEMO)
!isEmpty(SUPPORT_QT_MAEMO) {
SUBDIRS += 3rdparty
message("Adding experimental ssh support for Qt/Maemo applications.")
}
