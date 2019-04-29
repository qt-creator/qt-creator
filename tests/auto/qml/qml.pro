TEMPLATE = subdirs

SUBDIRS +=  \
            qmleditor \
            qmljssimplereader \
            qmlprojectmanager \
            codemodel \
            reformatter \
            qrcparser \
            persistenttrie

QTC_DO_NOT_BUILD_QMLDESIGNER = $$(QTC_DO_NOT_BUILD_QMLDESIGNER)
isEmpty(QTC_DO_NOT_BUILD_QMLDESIGNER):qtHaveModule(quick-private) {
    SUBDIRS += qmldesigner
} else {
    !qtHaveModule(quick-private) {
        warning("QmlDesigner plugin has been disabled since the Qt Quick module is not available.")
    } else {
        warning("QmlDesigner plugin has been disabled since QTC_DO_NOT_BUILD_QMLDESIGNER is set.")
    }
}
