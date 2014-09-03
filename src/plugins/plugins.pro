include(../../qtcreator.pri)

TEMPLATE  = subdirs

SUBDIRS   = \
    coreplugin \
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
    qmakeprojectmanager \
    debugger \
    help \
    cpaster \
    cmakeprojectmanager \
    autotoolsprojectmanager \
    fakevim \
    emacskeys \
    designer \
    resourceeditor \
    genericprojectmanager \
    qmljseditor \
    qmlprojectmanager \
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
    valgrind \
    todo \
    qnx \
    clearcase \
    baremetal \
    ios \
    beautifier

minQtVersion(5, 0, 0) {
    SUBDIRS += winrt

    isEmpty(QBS_INSTALL_DIR): QBS_INSTALL_DIR = $$(QBS_INSTALL_DIR)
    exists(../shared/qbs/qbs.pro)|!isEmpty(QBS_INSTALL_DIR): \
        SUBDIRS += \
            qbsprojectmanager
}

# prefer qmake variable set on command line over env var
isEmpty(LLVM_INSTALL_DIR):LLVM_INSTALL_DIR=$$(LLVM_INSTALL_DIR)
!isEmpty(LLVM_INSTALL_DIR) {
    SUBDIRS += clangcodemodel
}

isEmpty(IDE_PACKAGE_MODE) {
    SUBDIRS += \
        helloworld #\
        #updateinfo
#} else:!isEmpty(UPDATEINFO_ENABLE) {
#    SUBDIRS += \
#        updateinfo
}

minQtVersion(5, 3, 1) {
    SUBDIRS += qmldesigner
} else {
     warning("QmlDesigner plugin has been disabled.")
     warning("This plugin requires Qt 5.3.1 or newer.")
}

minQtVersion(5, 2, 0) {
    SUBDIRS += \
        qmlprofiler \
        welcome
} else {
     warning("QmlProfiler plugin has been disabled.")
     warning("Welcome plugin has been disabled.")
     warning("These plugins need at least Qt 5.2.")
}

for(p, SUBDIRS) {
    QTC_PLUGIN_DEPENDS =
    include($$p/$${p}_dependencies.pri)
    pv = $${p}.depends
    $$pv = $$QTC_PLUGIN_DEPENDS
}

linux-* {
     SUBDIRS += debugger/ptracepreload.pro
}
