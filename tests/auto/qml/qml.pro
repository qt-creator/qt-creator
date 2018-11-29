TEMPLATE = subdirs

SUBDIRS +=  \
            qmleditor \
            qmljssimplereader \
            qmlprojectmanager \
            codemodel \
            reformatter \
            qrcparser \
            persistenttrie

DO_NOT_BUILD_QMLDESIGNER = $$(DO_NOT_BUILD_QMLDESIGNER)
isEmpty(DO_NOT_BUILD_QMLDESIGNER):qtHaveModule(quick-private) {
    SUBDIRS += qmldesigner
} else {
    !qtHaveModule(quick-private) {
        warning("QmlDesigner plugin has been disabled since the Qt Quick module is not available.")
    } else {
        warning("QmlDesigner plugin has been disabled since DO_NOT_BUILD_QMLDESIGNER is set.")
    }
}
