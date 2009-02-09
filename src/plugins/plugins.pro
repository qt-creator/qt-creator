# USE .subdir AND .depends !
# OTHERWISE PLUGINS WILL BUILD IN WRONG ORDER (DIRECTORIES ARE COMPILED IN PARALLEL)

TEMPLATE  = subdirs

SUBDIRS   = plugin_coreplugin \
            plugin_find \
            plugin_texteditor \
            plugin_cppeditor \
            plugin_bineditor \
            plugin_bookmarks \
            plugin_projectexplorer \
            plugin_vcsbase \
            plugin_perforce \
            plugin_subversion \
            plugin_git \
            plugin_cpptools \
            plugin_qt4projectmanager \
#            plugin_snippets \ # buggy and annoying
            plugin_quickopen \
            plugin_debugger \
#            plugin_qtestlib \ # this seems to be dead
#            plugin_helloworld \ # sample plugin
            plugin_help \
#            plugin_regexp \ # don't know what to do with this
            plugin_qtscripteditor \
#            plugin_cpaster \
            plugin_cmakeprojectmanager \
            plugin_fakevim

# These two plugins require private headers from Qt and therefore don't work
# with an installed/released version of Qt.
exists($$(QTDIR)/.qmake.cache) {
    SUBDIRS += plugin_designer plugin_resourceeditor
} else {
    message(Designer and Resource Editor plugins are not build! They require private headers and do not compile with your released/installed version of Qt)
}

plugin_coreplugin.subdir = coreplugin

plugin_find.subdir = find
plugin_find.depends += plugin_coreplugin

plugin_texteditor.subdir = texteditor
plugin_texteditor.depends = plugin_find
plugin_texteditor.depends += plugin_quickopen
plugin_texteditor.depends += plugin_coreplugin

plugin_cppeditor.subdir = cppeditor
plugin_cppeditor.depends = plugin_texteditor
plugin_cppeditor.depends += plugin_coreplugin
plugin_cppeditor.depends += plugin_cpptools

plugin_bineditor.subdir = bineditor
plugin_bineditor.depends = plugin_texteditor
plugin_bineditor.depends += plugin_coreplugin

plugin_designer.subdir = designer
plugin_designer.depends = plugin_coreplugin plugin_cppeditor plugin_projectexplorer

plugin_vcsbase.subdir = vcsbase
plugin_vcsbase.depends = plugin_find
plugin_vcsbase.depends += plugin_texteditor
plugin_vcsbase.depends += plugin_coreplugin
plugin_vcsbase.depends += plugin_projectexplorer

plugin_perforce.subdir = perforce
plugin_perforce.depends = plugin_vcsbase
plugin_perforce.depends += plugin_projectexplorer
plugin_perforce.depends += plugin_coreplugin

plugin_git.subdir = git
plugin_git.depends = plugin_texteditor
plugin_git.depends = plugin_vcsbase
plugin_git.depends += plugin_projectexplorer
plugin_git.depends += plugin_coreplugin

plugin_subversion.subdir = subversion
plugin_subversion.depends = plugin_vcsbase
plugin_subversion.depends += plugin_projectexplorer
plugin_subversion.depends += plugin_coreplugin

plugin_projectexplorer.subdir = projectexplorer
plugin_projectexplorer.depends = plugin_quickopen
plugin_projectexplorer.depends += plugin_find
plugin_projectexplorer.depends += plugin_coreplugin
plugin_projectexplorer.depends += plugin_texteditor

plugin_qt4projectmanager.subdir = qt4projectmanager
plugin_qt4projectmanager.depends = plugin_texteditor
plugin_qt4projectmanager.depends += plugin_projectexplorer
plugin_qt4projectmanager.depends += plugin_cpptools
plugin_qt4projectmanager.depends += plugin_cppeditor
plugin_qt4projectmanager.depends += plugin_help

plugin_quickopen.subdir = quickopen
plugin_quickopen.depends = plugin_coreplugin

plugin_cpptools.subdir = cpptools
plugin_cpptools.depends = plugin_projectexplorer
plugin_cpptools.depends += plugin_coreplugin
plugin_cpptools.depends += plugin_texteditor

plugin_bookmarks.subdir = bookmarks
plugin_bookmarks.depends = plugin_projectexplorer
plugin_bookmarks.depends += plugin_coreplugin
plugin_bookmarks.depends += plugin_texteditor

plugin_snippets.subdir = snippets
plugin_snippets.depends = plugin_projectexplorer
plugin_snippets.depends += plugin_coreplugin
plugin_snippets.depends += plugin_texteditor

plugin_debugger.subdir = debugger
plugin_debugger.depends = plugin_projectexplorer
plugin_debugger.depends += plugin_coreplugin
plugin_debugger.depends += plugin_cppeditor

plugin_fakevim.subdir = fakevim
plugin_fakevim.depends = plugin_projectexplorer
plugin_fakevim.depends += plugin_coreplugin
plugin_fakevim.depends += plugin_cppeditor

plugin_qtestlib.subdir = qtestlib
plugin_qtestlib.depends = plugin_projectexplorer
plugin_qtestlib.depends += plugin_coreplugin

plugin_helloworld.subdir = helloworld
plugin_helloworld.depends += plugin_coreplugin

plugin_help.subdir = help
plugin_help.depends = plugin_find
plugin_help.depends += plugin_quickopen
plugin_help.depends += plugin_coreplugin

plugin_resourceeditor.subdir = resourceeditor
plugin_resourceeditor.depends = plugin_coreplugin

plugin_regexp.subdir = regexp
plugin_regexp.depends = plugin_coreplugin

plugin_qtscripteditor.subdir = qtscripteditor
plugin_qtscripteditor.depends = plugin_texteditor
plugin_qtscripteditor.depends += plugin_coreplugin

plugin_cpaster.subdir = cpaster
plugin_cpaster.depends += plugin_texteditor
plugin_cpaster.depends += plugin_coreplugin
plugin_cpaster.depends += plugin_projectexplorer

plugin_cmakeprojectmanager.subdir = cmakeprojectmanager
plugin_cmakeprojectmanager.depends = plugin_texteditor
plugin_cmakeprojectmanager.depends += plugin_projectexplorer
plugin_cmakeprojectmanager.depends += plugin_cpptools
plugin_cmakeprojectmanager.depends += plugin_cppeditor
plugin_cmakeprojectmanager.depends += plugin_help
