add_qtc_library(QmlEditorWidgets
  DEPENDS QmlJS Utils Qt5::Widgets
  SOURCES
    colorbox.cpp colorbox.h
    colorbutton.cpp colorbutton.h
    contextpanetext.ui
    contextpanetextwidget.cpp contextpanetextwidget.h
    contextpanewidget.cpp contextpanewidget.h
    contextpanewidgetborderimage.ui
    contextpanewidgetimage.cpp contextpanewidgetimage.h contextpanewidgetimage.ui
    contextpanewidgetrectangle.cpp contextpanewidgetrectangle.h contextpanewidgetrectangle.ui
    customcolordialog.cpp customcolordialog.h
    easingpane/easingcontextpane.cpp easingpane/easingcontextpane.h easingpane/easingcontextpane.ui
    easingpane/easinggraph.cpp easingpane/easinggraph.h
    easingpane/easingpane.qrc
    filewidget.cpp filewidget.h
    fontsizespinbox.cpp fontsizespinbox.h
    gradientline.cpp gradientline.h
    huecontrol.cpp huecontrol.h
    qmleditorwidgets_global.h
    resources.qrc
)
