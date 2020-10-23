find_package(Qt5 COMPONENTS SvgWidgets QUIET)
if (TARGET Qt5::SvgWidgets)
  set(SVG_WIDGETS Qt5::SvgWidgets)
endif()

add_qtc_plugin(ImageViewer
  DEPENDS OptionalSvg ${SVG_WIDGETS}
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
