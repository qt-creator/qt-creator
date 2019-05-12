add_qtc_plugin(ImageViewer
  DEPENDS OptionalSvg
  PLUGIN_DEPENDS Core
  SOURCES
    exportdialog.cpp exportdialog.h
    imageview.cpp imageview.h
    imageviewer.cpp imageviewer.h
    imageviewerconstants.h
    imageviewerfactory.cpp imageviewerfactory.h
    imageviewerfile.cpp imageviewerfile.h
    imageviewerplugin.cpp imageviewerplugin.h
    imageviewertoolbar.ui
    multiexportdialog.cpp multiexportdialog.h
  EXPLICIT_MOC imageviewer.h
)
