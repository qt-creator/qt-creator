# Transcribes on this computer with whisper.cpp. src/libs/3rdparty builds it
# from its submodule; without that, an installation is looked for.
if(NOT TARGET whisper)
  find_package(whisper CONFIG QUIET)
endif()
find_package(Qt6 COMPONENTS Multimedia QUIET)

set(_voiceinput_attributions whisper.cpp ggml)

add_qtc_library(VoiceInput
  CONDITION TARGET whisper AND TARGET Qt::Multimedia
  DEPENDS Qt::Multimedia Qt::Network whisper
  PUBLIC_DEPENDS Utils Qt::Widgets
  SOURCES
    speechactivity.cpp speechactivity.h
    speechmodel.cpp speechmodel.h
    transcriber.cpp transcriber.h
    voiceinput_global.h
    voiceinputbutton.cpp voiceinputbutton.h
    voiceinputtr.h
    voicelevelmeter.cpp voicelevelmeter.h
    voicesettings.cpp voicesettings.h
  SBOM_ARGS
    USE_ATTRIBUTION_FILES
    CREATE_SBOM_FOR_EACH_ATTRIBUTION
    ATTRIBUTION_FILE_PATHS
      "${QtCreator_SOURCE_DIR}/qt_attributions.json"
    ATTRIBUTION_IDS
      ${_voiceinput_attributions}
)

qtc_add_resources(VoiceInput "images"
  PREFIX "/voiceinput"
  BASE "."
  FILES
    images/microphone.png
    images/microphone@2x.png
)
