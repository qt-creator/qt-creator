add_qtc_plugin(ScreenRecorder
  PLUGIN_DEPENDS Core
  DEPENDS Spinner
  SOURCES
    cropandtrim.cpp cropandtrim.h
    export.cpp export.h
    ffmpegutils.cpp ffmpegutils.h
    record.cpp record.h
    screenrecorder.qrc
    screenrecorderconstants.h
    screenrecorderplugin.cpp
    screenrecordersettings.cpp screenrecordersettings.h
)

extend_qtc_plugin(ScreenRecorder
  CONDITION WITH_TESTS
  SOURCES
    screenrecorder_test.cpp screenrecorder_test.h
  EXPLICIT_MOC screenrecorder_test.h
)
