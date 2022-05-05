add_qtc_plugin(GitLab
  PLUGIN_CLASS GitLabPlugin
  PLUGIN_DEPENDS Core ProjectExplorer Git VcsBase
  DEPENDS Utils
  SOURCES
    gitlaboptionspage.cpp gitlaboptionspage.h
    gitlabparameters.cpp gitlabparameters.h
    gitlabplugin.cpp gitlabplugin.h
    gitlabprojectsettings.cpp gitlabprojectsettings.h
    queryrunner.cpp queryrunner.h
    resultparser.cpp resultparser.h
)
