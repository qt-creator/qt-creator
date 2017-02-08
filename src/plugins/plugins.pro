include(../../qtcreator.pri)

TEMPLATE  = subdirs

SUBDIRS   = \
    autotest \
    clangstaticanalyzer \
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
    cpaster \
    cmakeprojectmanager \
    autotoolsprojectmanager \
    fakevim \
    emacskeys \
    resourceeditor \
    genericprojectmanager \
    qmljseditor \
    qmlprojectmanager \
    glsleditor \
    pythoneditor \
    nim \
    mercurial \
    bazaar \
    classview \
    tasklist \
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
    beautifier \
    modeleditor \
    qmakeandroidsupport \
    winrt \
    qmlprofiler \
    updateinfo \
    scxmleditor \
    welcome

qtHaveModule(help) {
    SUBDIRS += help
} else {
    warning("Help plugin has been disabled.")
}

qtHaveModule(designercomponents_private) {
    SUBDIRS += designer
} else {
    warning("Qt Widget Designer plugin has been disabled.")
}

DO_NOT_BUILD_QMLDESIGNER = $$(DO_NOT_BUILD_QMLDESIGNER)
isEmpty(DO_NOT_BUILD_QMLDESIGNER) {
    SUBDIRS += qmldesigner
} else {
    warning("QmlDesigner plugin has been disabled.")
}


isEmpty(QBS_INSTALL_DIR): QBS_INSTALL_DIR = $$(QBS_INSTALL_DIR)
exists(../shared/qbs/qbs.pro)|!isEmpty(QBS_INSTALL_DIR): \
    SUBDIRS += \
        qbsprojectmanager

# prefer qmake variable set on command line over env var
isEmpty(LLVM_INSTALL_DIR):LLVM_INSTALL_DIR=$$(LLVM_INSTALL_DIR)
exists($$LLVM_INSTALL_DIR) {
    SUBDIRS += clangcodemodel

    QTC_NO_CLANG_LIBTOOLING=$$(QTC_NO_CLANG_LIBTOOLING)
    win32-msvc2015:lessThan(QT_CL_PATCH_VERSION, 24210): QTC_NO_CLANG_LIBTOOLING = 1
    isEmpty(QTC_NO_CLANG_LIBTOOLING) {
        SUBDIRS += clangrefactoring
        SUBDIRS += clangpchmanager
    } else {
        warning("Building the Clang refactoring and the pch manager plugins are disabled.")
    }
} else {
    warning("Set LLVM_INSTALL_DIR to build the Clang Code Model. " \
            "For details, see doc/src/editors/creator-clang-codemodel.qdoc.")
}

isEmpty(IDE_PACKAGE_MODE) {
    SUBDIRS += \
        helloworld
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
