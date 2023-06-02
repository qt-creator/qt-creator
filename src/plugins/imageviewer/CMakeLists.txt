find_package(Qt6 COMPONENTS SvgWidgets QUIET)
if (TARGET Qt::SvgWidgets)
  set(SVG_WIDGETS Qt::SvgWidgets)
endif()

add_qtc_plugin(ImageViewer
  DEPENDS OptionalSvg ${SVG_WIDGETS}
  PLUGIN_DEPENDS Core
  SOURCES
    exportdialog.cpp exportdialog.h
    imageview.cpp imageview.h
    imageviewer.cpp imageviewer.h
    imageviewerconstants.h
    imageviewerfile.cpp imageviewerfile.h
    imageviewerplugin.cpp imageviewerplugin.h
    imageviewertr.h
    multiexportdialog.cpp multiexportdialog.h
  EXPLICIT_MOC imageviewer.h
)
