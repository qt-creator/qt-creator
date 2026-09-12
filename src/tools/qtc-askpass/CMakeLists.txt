add_qtc_executable(qtc-askpass
  DEPENDS Qt::Network Qt::Widgets
  SOURCES
    qtc-askpass-main.cpp
)

qtc_add_version_resource(qtc-askpass "${IDE_DISPLAY_NAME} Askpass Helper")
