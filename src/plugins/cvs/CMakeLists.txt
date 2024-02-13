add_qtc_plugin(CVS
  PLUGIN_CLASS CvsPlugin
  PLUGIN_DEPENDS Core TextEditor VcsBase
  SOURCES
    cvseditor.cpp
    cvseditor.h
    cvsplugin.cpp
    cvssettings.cpp
    cvssettings.h
    cvssubmiteditor.cpp
    cvssubmiteditor.h
    cvstr.h
    cvsutils.cpp
    cvsutils.h
)
