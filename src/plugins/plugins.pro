include(../../qtcreator.pri)

TEMPLATE  = subdirs

SUBDIRS   = \
    autotest \
    clangformat \
    clangtools \
    coreplugin \
    texteditor \
    cppeditor \
    bineditor \
    boot2qt \
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
    mesonprojectmanager \
    autotoolsprojectmanager \
    fakevim \
    emacskeys \
    resourceeditor \
    genericprojectmanager \
    qmljseditor \
    qmlprojectmanager \
    glsleditor \
    python \
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
    winrt \
    updateinfo \
    scxmleditor \
    welcome \
    silversearcher \
    languageclient \
    cppcheck \
    compilationdatabaseprojectmanager \
    qmlpreview \
    studiowelcome \
    webassembly \
    mcusupport \
    marketplace \
    incredibuild

qtHaveModule(serialport) {
    SUBDIRS += serialterminal
} else {
    warning("SerialTerminal plugin has been disabled since the Qt SerialPort module is not available.")
}

qtHaveModule(quick) {
    SUBDIRS += qmlprofiler perfprofiler ctfvisualizer
} else {
    warning("QmlProfiler, PerfProfiler and CTF Visualizer plugins have been disabled since the Qt Quick module is not available.")
}

qtHaveModule(help) {
    SUBDIRS += help
} else {
    warning("Help plugin has been disabled since the Qt Help module is not available.")
}

qtHaveModule(designercomponents_private) {
    SUBDIRS += designer
} else {
    warning("Qt Widget Designer plugin has been disabled since the Qt Designer module is not available.")
}

QTC_DO_NOT_BUILD_QMLDESIGNER = $$(QTC_DO_NOT_BUILD_QMLDESIGNER)
isEmpty(QTC_DO_NOT_BUILD_QMLDESIGNER):qtHaveModule(quick-private) {
    exists($$[QT_INSTALL_QML]/QtQuick/Controls/qmldir) {
       SUBDIRS += qmldesigner
    } else {
        warning("QmlDesigner plugin has been disabled since Qt Quick Controls 1 are not installed.")
    }
} else {
    !qtHaveModule(quick-private) {
        warning("QmlDesigner plugin has been disabled since the Qt Quick module is not available.")
    } else {
        warning("QmlDesigner plugin has been disabled since QTC_DO_NOT_BUILD_QMLDESIGNER is set.")
    }
}


isEmpty(QBS_INSTALL_DIR): QBS_INSTALL_DIR = $$(QBS_INSTALL_DIR)
exists(../shared/qbs/qbs.pro)|!isEmpty(QBS_INSTALL_DIR): \
    SUBDIRS += \
        qbsprojectmanager

SUBDIRS += \
    clangcodemodel

QTC_ENABLE_CLANG_REFACTORING=$$(QTC_ENABLE_CLANG_REFACTORING)
!isEmpty(QTC_ENABLE_CLANG_REFACTORING) {
    SUBDIRS += clangrefactoring
    SUBDIRS += clangpchmanager
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

QMAKE_EXTRA_TARGETS += deployqt # dummy
