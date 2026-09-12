add_qtc_executable(qtpromaker DEPENDS Qt::Core SOURCES main.cpp)

qtc_add_version_resource(qtpromaker "${IDE_DISPLAY_NAME} Project File Generator")
