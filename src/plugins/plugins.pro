# USE .subdir AND .depends !
# OTHERWISE PLUGINS WILL BUILD IN WRONG ORDER (DIRECTORIES ARE COMPILED IN PARALLEL)

TEMPLATE  = subdirs

SUBDIRS   = plugin_coreplugin \
            plugin_welcome \
            plugin_find \
            plugin_texteditor \
            plugin_cppeditor \
            plugin_bineditor \
            plugin_imageviewer \
            plugin_bookmarks \
            plugin_projectexplorer \
            plugin_vcsbase \
            plugin_perforce \
            plugin_subversion \
            plugin_git \
            plugin_cvs \
            plugin_cpptools \
            plugin_qtsupport \
            plugin_qt4projectmanager \
            plugin_locator \
            plugin_debugger \
            plugin_help \
            plugin_cpaster \
            plugin_cmakeprojectmanager \
            plugin_autotoolsprojectmanager \
            plugin_fakevim \
            plugin_designer \
            plugin_resourceeditor \
            plugin_genericprojectmanager \
            plugin_qmljseditor \
            plugin_glsleditor \
            plugin_mercurial \
            plugin_bazaar \
            plugin_classview \
            plugin_tasklist \
            plugin_analyzerbase \
            plugin_qmljstools \
            plugin_macros \
            debugger/dumper.pro \
            plugin_qmlprofiler \
            plugin_remotelinux \
            plugin_android \
            plugin_madde \
            plugin_valgrind \
            plugin_todo \
            plugin_qnx

isEmpty(IDE_PACKAGE_MODE) {
    SUBDIRS += plugin_helloworld \
               plugin_updateinfo
} else:!isEmpty(UPDATEINFO_ENABLE) {
    SUBDIRS += plugin_updateinfo
}
linux-* {
     SUBDIRS += debugger/ptracepreload.pro
}

include(../../qtcreator.pri)

contains(QT_CONFIG, declarative)|contains(QT_CONFIG, quick1) {
    SUBDIRS += \
            plugin_qmlprojectmanager

    include(../private_headers.pri)
    exists($${QT_PRIVATE_HEADERS}/QtDeclarative/private/qdeclarativecontext_p.h) {

        minQtVersion(4, 7, 1) {
            SUBDIRS += plugin_qmldesigner
        } else {
            warning()
            warning("QmlDesigner plugin has been disabled.")
            warning("QmlDesigner requires Qt 4.7.1 or later.")
        }
    } else {
        warning()
        warning("QmlDesigner plugin has been disabled.")
        warning("The plugin depends on private headers from QtDeclarative module.")
        warning("To enable it, pass 'QT_PRIVATE_HEADERS=$QTDIR/include' to qmake, where $QTDIR is the source directory of qt.")
    }
}

include (debugger/lldb/guest/qtcreator-lldb.pri)

plugin_coreplugin.subdir = coreplugin

plugin_updateinfo.subdir = updateinfo
plugin_updateinfo.depends = plugin_coreplugin

plugin_welcome.subdir = welcome
plugin_welcome.depends = plugin_coreplugin
plugin_welcome.depends += plugin_projectexplorer

plugin_find.subdir = find
plugin_find.depends += plugin_coreplugin

plugin_texteditor.subdir = texteditor
plugin_texteditor.depends = plugin_find
plugin_texteditor.depends += plugin_locator
plugin_texteditor.depends += plugin_coreplugin

plugin_cppeditor.subdir = cppeditor
plugin_cppeditor.depends = plugin_texteditor
plugin_cppeditor.depends += plugin_coreplugin
plugin_cppeditor.depends += plugin_cpptools

plugin_bineditor.subdir = bineditor
plugin_bineditor.depends = plugin_texteditor
plugin_bineditor.depends += plugin_coreplugin

plugin_imageviewer.subdir = imageviewer
plugin_imageviewer.depends = plugin_coreplugin

plugin_designer.subdir = designer
plugin_designer.depends = plugin_coreplugin plugin_cpptools plugin_projectexplorer plugin_texteditor

plugin_vcsbase.subdir = vcsbase
plugin_vcsbase.depends = plugin_find
plugin_vcsbase.depends += plugin_texteditor
plugin_vcsbase.depends += plugin_coreplugin
plugin_vcsbase.depends += plugin_projectexplorer
plugin_vcsbase.depends += plugin_cpptools

plugin_perforce.subdir = perforce
plugin_perforce.depends = plugin_vcsbase
plugin_perforce.depends += plugin_projectexplorer
plugin_perforce.depends += plugin_coreplugin

plugin_git.subdir = git
plugin_git.depends = plugin_vcsbase
plugin_git.depends += plugin_projectexplorer
plugin_git.depends += plugin_coreplugin

plugin_cvs.subdir = cvs
plugin_cvs.depends = plugin_vcsbase
plugin_cvs.depends += plugin_projectexplorer
plugin_cvs.depends += plugin_coreplugin

plugin_subversion.subdir = subversion
plugin_subversion.depends = plugin_vcsbase
plugin_subversion.depends += plugin_projectexplorer
plugin_subversion.depends += plugin_coreplugin

plugin_projectexplorer.subdir = projectexplorer
plugin_projectexplorer.depends = plugin_locator
plugin_projectexplorer.depends += plugin_find
plugin_projectexplorer.depends += plugin_coreplugin
plugin_projectexplorer.depends += plugin_texteditor

plugin_qtsupport.subdir = qtsupport
plugin_qtsupport.depends = plugin_projectexplorer

plugin_qt4projectmanager.subdir = qt4projectmanager
plugin_qt4projectmanager.depends = plugin_texteditor
plugin_qt4projectmanager.depends += plugin_projectexplorer
plugin_qt4projectmanager.depends += plugin_qtsupport
plugin_qt4projectmanager.depends += plugin_cpptools
plugin_qt4projectmanager.depends += plugin_debugger
plugin_qt4projectmanager.depends += plugin_qmljseditor

plugin_remotelinux.subdir = remotelinux
plugin_remotelinux.depends += plugin_coreplugin
plugin_remotelinux.depends += plugin_debugger
plugin_remotelinux.depends += plugin_projectexplorer
plugin_remotelinux.depends += plugin_qt4projectmanager

plugin_android.subdir = android
plugin_android.depends = plugin_coreplugin
plugin_android.depends += plugin_debugger
plugin_android.depends += plugin_projectexplorer
plugin_android.depends += plugin_qt4projectmanager

plugin_madde.subdir = madde
plugin_madde.depends += plugin_remotelinux

plugin_locator.subdir = locator
plugin_locator.depends = plugin_coreplugin

plugin_cpptools.subdir = cpptools
plugin_cpptools.depends = plugin_projectexplorer
plugin_cpptools.depends += plugin_coreplugin
plugin_cpptools.depends += plugin_texteditor
plugin_cpptools.depends += plugin_find

plugin_bookmarks.subdir = bookmarks
plugin_bookmarks.depends = plugin_projectexplorer
plugin_bookmarks.depends += plugin_coreplugin
plugin_bookmarks.depends += plugin_texteditor

plugin_debugger.subdir = debugger
plugin_debugger.depends = plugin_projectexplorer
plugin_debugger.depends += plugin_coreplugin
plugin_debugger.depends += plugin_cpptools
plugin_debugger.depends += plugin_qmljstools

plugin_fakevim.subdir = fakevim
plugin_fakevim.depends = plugin_coreplugin
plugin_fakevim.depends += plugin_texteditor

plugin_qtestlib.subdir = qtestlib
plugin_qtestlib.depends = plugin_projectexplorer
plugin_qtestlib.depends += plugin_coreplugin

plugin_helloworld.subdir = helloworld
plugin_helloworld.depends = plugin_coreplugin

plugin_help.subdir = help
plugin_help.depends = plugin_find
plugin_help.depends += plugin_locator
plugin_help.depends += plugin_coreplugin

plugin_resourceeditor.subdir = resourceeditor
plugin_resourceeditor.depends = plugin_coreplugin
plugin_resourceeditor.depends = plugin_find

plugin_regexp.subdir = regexp
plugin_regexp.depends = plugin_coreplugin

plugin_cpaster.subdir = cpaster
plugin_cpaster.depends = plugin_texteditor
plugin_cpaster.depends += plugin_coreplugin

plugin_cmakeprojectmanager.subdir = cmakeprojectmanager
plugin_cmakeprojectmanager.depends = plugin_texteditor
plugin_cmakeprojectmanager.depends += plugin_projectexplorer
plugin_cmakeprojectmanager.depends += plugin_cpptools
plugin_cmakeprojectmanager.depends += plugin_qtsupport

plugin_autotoolsprojectmanager.subdir = autotoolsprojectmanager
plugin_autotoolsprojectmanager.depends = plugin_projectexplorer
plugin_autotoolsprojectmanager.depends += plugin_coreplugin
plugin_autotoolsprojectmanager.depends += plugin_cpptools
plugin_autotoolsprojectmanager.depends += plugin_qtsupport

plugin_genericprojectmanager.subdir = genericprojectmanager
plugin_genericprojectmanager.depends = plugin_texteditor
plugin_genericprojectmanager.depends += plugin_projectexplorer
plugin_genericprojectmanager.depends += plugin_cpptools
plugin_genericprojectmanager.depends += plugin_qtsupport

plugin_qmljseditor.subdir = qmljseditor
plugin_qmljseditor.depends = plugin_texteditor
plugin_qmljseditor.depends += plugin_coreplugin
plugin_qmljseditor.depends += plugin_projectexplorer
plugin_qmljseditor.depends += plugin_qmljstools

plugin_glsleditor.subdir = glsleditor
plugin_glsleditor.depends = plugin_texteditor
plugin_glsleditor.depends += plugin_coreplugin
plugin_glsleditor.depends += plugin_projectexplorer
plugin_glsleditor.depends += plugin_cpptools

plugin_qmlprojectmanager.subdir = qmlprojectmanager
plugin_qmlprojectmanager.depends = plugin_texteditor
plugin_qmlprojectmanager.depends += plugin_projectexplorer
plugin_qmlprojectmanager.depends += plugin_qmljseditor
plugin_qmlprojectmanager.depends += plugin_debugger
plugin_qmlprojectmanager.depends += plugin_qtsupport

plugin_qmldesigner.subdir = qmldesigner
plugin_qmldesigner.depends = plugin_coreplugin
plugin_qmldesigner.depends += plugin_texteditor
plugin_qmldesigner.depends += plugin_qmljseditor
plugin_qmldesigner.depends += plugin_projectexplorer
plugin_qmldesigner.depends += plugin_qt4projectmanager
plugin_qmldesigner.depends += plugin_qmlprojectmanager
plugin_qmldesigner.depends += plugin_cpptools

plugin_mercurial.subdir = mercurial
plugin_mercurial.depends = plugin_vcsbase
plugin_mercurial.depends += plugin_projectexplorer
plugin_mercurial.depends += plugin_coreplugin

plugin_bazaar.subdir = bazaar
plugin_bazaar.depends = plugin_vcsbase
plugin_bazaar.depends += plugin_projectexplorer
plugin_bazaar.depends += plugin_coreplugin

plugin_classview.subdir = classview
plugin_classview.depends = plugin_coreplugin
plugin_classview.depends += plugin_cpptools
plugin_classview.depends += plugin_projectexplorer
plugin_classview.depends += plugin_texteditor

plugin_tasklist.subdir = tasklist
plugin_tasklist.depends = plugin_coreplugin
plugin_tasklist.depends += plugin_projectexplorer

plugin_analyzerbase.subdir = analyzerbase
plugin_analyzerbase.depends = plugin_coreplugin
plugin_analyzerbase.depends += plugin_projectexplorer
plugin_analyzerbase.depends += plugin_remotelinux
plugin_analyzerbase.depends += plugin_cpptools

plugin_valgrind.subdir = valgrind
plugin_valgrind.depends = plugin_coreplugin
plugin_valgrind.depends += plugin_analyzerbase
plugin_valgrind.depends += plugin_remotelinux
plugin_valgrind.depends += plugin_texteditor

plugin_qmlprofiler.subdir = qmlprofiler
plugin_qmlprofiler.depends = plugin_coreplugin
plugin_qmlprofiler.depends += plugin_analyzerbase
plugin_qmlprofiler.depends += plugin_qmlprojectmanager
plugin_qmlprofiler.depends += plugin_qt4projectmanager
plugin_qmlprofiler.depends += plugin_remotelinux

plugin_qmljstools.subdir = qmljstools
plugin_qmljstools.depends = plugin_projectexplorer
plugin_qmljstools.depends += plugin_coreplugin
plugin_qmljstools.depends += plugin_texteditor
plugin_qmljstools.depends += plugin_qtsupport
plugin_qmljstools.depends += plugin_cpptools

plugin_macros.subdir = macros
plugin_macros.depends = plugin_texteditor
plugin_macros.depends += plugin_find
plugin_macros.depends += plugin_locator
plugin_macros.depends += plugin_coreplugin

plugin_todo.subdir = todo
plugin_todo.depends = plugin_coreplugin
plugin_todo.depends += plugin_projectexplorer
plugin_todo.depends += plugin_texteditor
plugin_todo.depends += plugin_cpptools

plugin_qnx.subdir = qnx
plugin_qnx.depends = plugin_remotelinux
plugin_qnx.depends += plugin_qt4projectmanager
plugin_qnx.depends += plugin_coreplugin

