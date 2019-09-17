get_target_property(UtilsSourcesDir Utils SOURCES_DIR)

add_qtc_executable(sdktool
  DEFINES DATA_PATH=\"${IDE_DATA_PATH}\"
  DEPENDS Qt5::Core app_version
  INCLUDES "${UtilsSourcesDir}/../"
  SOURCES
    addabiflavor.cpp addabiflavor.h
    addcmakeoperation.cpp addcmakeoperation.h
    adddebuggeroperation.cpp adddebuggeroperation.h
    adddeviceoperation.cpp adddeviceoperation.h
    addkeysoperation.cpp addkeysoperation.h
    addkitoperation.cpp addkitoperation.h
    addqtoperation.cpp addqtoperation.h
    addtoolchainoperation.cpp addtoolchainoperation.h
    addvalueoperation.cpp addvalueoperation.h
    findkeyoperation.cpp findkeyoperation.h
    findvalueoperation.cpp findvalueoperation.h
    getoperation.cpp getoperation.h
    main.cpp
    operation.cpp operation.h
    rmcmakeoperation.cpp rmcmakeoperation.h
    rmdebuggeroperation.cpp rmdebuggeroperation.h
    rmdeviceoperation.cpp rmdeviceoperation.h
    rmkeysoperation.cpp rmkeysoperation.h
    rmkitoperation.cpp rmkitoperation.h
    rmqtoperation.cpp rmqtoperation.h
    rmtoolchainoperation.cpp rmtoolchainoperation.h
    settings.cpp settings.h
)

extend_qtc_executable(sdktool
  SOURCES_PREFIX "${UtilsSourcesDir}"
  DEFINES QTCREATOR_UTILS_STATIC_LIB
  SOURCES
    environment.cpp environment.h
    fileutils.cpp fileutils.h
    hostosinfo.cpp hostosinfo.h
    namevaluedictionary.cpp namevaluedictionary.h
    namevalueitem.cpp namevalueitem.h
    persistentsettings.cpp persistentsettings.h
    qtcassert.cpp qtcassert.h
    qtcprocess.cpp qtcprocess.h
    savefile.cpp savefile.h
    stringutils.cpp stringutils.h
)

extend_qtc_executable(sdktool CONDITION APPLE
  SOURCES_PREFIX "${UtilsSourcesDir}"
  SOURCES
    fileutils_mac.mm fileutils_mac.h
  DEPENDS
    ${FWFoundation}
)

extend_qtc_executable(sdktool CONDITION WIN32
  DEPENDS
    user32 iphlpapi ws2_32 shell32
  DEFINES
    _UNICODE UNICODE
    _CRT_SECURE_NO_WARNINGS _SCL_SECURE_NO_WARNINGS
)
