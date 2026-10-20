find_package(Qt6 COMPONENTS DBus QUIET)

add_qtc_plugin(ScreenRecorder
  PLUGIN_DEPENDS Core
  DEPENDS Utils
  SOURCES
    cropandtrim.cpp cropandtrim.h
    export.cpp export.h
    ffmpegutils.cpp ffmpegutils.h
    portalcapture.cpp portalcapture.h
    record.cpp record.h
    screenrecorderconstants.h
    screenrecorderplugin.cpp
    screenrecordersettings.cpp screenrecordersettings.h
    screenrecordertr.h
)

extend_qtc_plugin(ScreenRecorder
  FEATURE_INFO "Screen cast portal support"
  CONDITION UNIX AND NOT APPLE AND TARGET Qt::DBus
  DEFINES QTC_SCREENRECORDER_WITH_PORTAL
  DEPENDS Qt::DBus
)

extend_qtc_plugin(ScreenRecorder
  CONDITION WITH_TESTS
  SOURCES
    screenrecorder_test.cpp screenrecorder_test.h
  EXPLICIT_MOC screenrecorder_test.h
)
