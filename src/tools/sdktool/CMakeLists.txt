# TODO: Do not depend on Utils! Include relevant files instead.

add_qtc_executable(sdktool
  DEFINES DATA_PATH=\"${IDE_DATA_PATH}\"
  DEPENDS Qt5::Core Utils app_version
  SOURCES
    addabiflavor.cpp addabiflavor.h
    addcmakeoperation.cpp addcmakeoperation.h
    adddebuggeroperation.cpp adddebuggeroperation.h
    adddeviceoperation.cpp adddeviceoperation.h
    addkeysoperation.cpp addkeysoperation.h
    addkitoperation.cpp addkitoperation.h
    addqtoperation.cpp addqtoperation.h
    addtoolchainoperation.cpp addtoolchainoperation.h
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
