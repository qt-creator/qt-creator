include(../../qtcreator.pri)

TEMPLATE  = subdirs

SUBDIRS   = \
    coreplugin \
    welcome \
    find \
    texteditor \
    cppeditor \
    bineditor \
    diffeditor \
    imageviewer \
    bookmarks \
    projectexplorer \
    vcsbase \
    perforce \
    subversion \
    git \
    cvs \
    cpptools \
    qtsupport \
    qt4projectmanager \
    locator \
    debugger \
    help \
    cpaster \
    cmakeprojectmanager \
    autotoolsprojectmanager \
    fakevim \
    designer \
    resourceeditor \
    genericprojectmanager \
    qmljseditor \
    glsleditor \
    pythoneditor \
    mercurial \
    bazaar \
    classview \
    tasklist \
    analyzerbase \
    qmljstools \
    macros \
    remotelinux \
    android \
    madde \
    valgrind \
    todo \
    qnx

exists(../shared/qbs/qbs.pro): \
    SUBDIRS += \
        qbsprojectmanager

isEmpty(IDE_PACKAGE_MODE) {
    SUBDIRS += \
        helloworld \
        updateinfo
} else:!isEmpty(UPDATEINFO_ENABLE) {
    SUBDIRS += \
        updateinfo
}

!macx: \
    SUBDIRS += \
        clearcase

contains(QT_CONFIG, declarative)|!isEmpty(QT.declarative.name) {
    SUBDIRS += \
        qmlprojectmanager \
        qmlprofiler

    greaterThan(QT_MAJOR_VERSION, 4) {
        SUBDIRS += \
            qmldesigner
    } else {
        include(../private_headers.pri)
        exists($${QT_PRIVATE_HEADERS}/QtDeclarative/private/qdeclarativecontext_p.h) {
            SUBDIRS += \
                qmldesigner
        } else {
            warning("QmlDesigner plugin has been disabled.")
            warning("The plugin depends on private headers from QtDeclarative module.")
            warning("To enable it, pass 'QT_PRIVATE_HEADERS=$QTDIR/include' to qmake, where $QTDIR is the source directory of qt.")
        }
    }
} else {
    warning("QmlProjectManager, QmlProfiler and QmlDesigner plugins have been disabled: The plugins require QtDeclarative")
}

for(p, SUBDIRS) {
    QTC_PLUGIN_DEPENDS =
    include($$p/$${p}_dependencies.pri)
    pv = $${p}.depends
    $$pv = $$QTC_PLUGIN_DEPENDS
}

SUBDIRS += debugger/dumper.pro
linux-* {
     SUBDIRS += debugger/ptracepreload.pro
}

include (debugger/lldblib/guest/qtcreator-lldb.pri)
