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
    winrt \
    updateinfo \
    scxmleditor \
    welcome \
    silversearcher \
    languageclient \
    cppcheck \
    compilationdatabaseprojectmanager \
    qmlpreview

qtHaveModule(serialport) {
    SUBDIRS += serialterminal
} else {
    warning("SerialTerminal plugin has been disabled since the Qt SerialPort module is not available.")
}

qtHaveModule(quick) {
    SUBDIRS += qmlprofiler perfprofiler
} else {
    warning("QmlProfiler and PerfProfiler plugins have been disabled since the Qt Quick module is not available.")
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

DO_NOT_BUILD_QMLDESIGNER = $$(DO_NOT_BUILD_QMLDESIGNER)
isEmpty(DO_NOT_BUILD_QMLDESIGNER):qtHaveModule(quick-private) {
    exists($$[QT_INSTALL_QML]/QtQuick/Controls/qmldir) {
       SUBDIRS += qmldesigner
    } else {
        warning("QmlDesigner plugin has been disabled since Qt Quick Controls 1 are not installed.")
    }
} else {
    !qtHaveModule(quick-private) {
        warning("QmlDesigner plugin has been disabled since the Qt Quick module is not available.")
    } else {
        warning("QmlDesigner plugin has been disabled since DO_NOT_BUILD_QMLDESIGNER is set.")
    }
}


isEmpty(QBS_INSTALL_DIR): QBS_INSTALL_DIR = $$(QBS_INSTALL_DIR)
exists(../shared/qbs/qbs.pro)|!isEmpty(QBS_INSTALL_DIR): \
    SUBDIRS += \
        qbsprojectmanager

SUBDIRS += \
    clangcodemodel

QTC_ENABLE_CLANG_LIBTOOLING=$$(QTC_ENABLE_CLANG_LIBTOOLING)
!isEmpty(QTC_ENABLE_CLANG_LIBTOOLING) {
    SUBDIRS += clangrefactoring
    SUBDIRS += clangpchmanager
} else {
    warning("Not building the clang refactoring plugin and the pch manager plugin.")
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
