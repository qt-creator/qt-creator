add_qtc_library(QrCodeGenerator STATIC
    CONDITION BUILD_DESIGNSTUDIO
    PUBLIC_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/src
    DEPENDS
      Qt6::Qml Qt6::Quick Qt6::Svg Qt6::QuickWidgets
    SOURCES
    src/qrcodegen.cpp
    src/qrcodegen.h
    src/qrcodeimageprovider.cpp
    src/qrcodeimageprovider.h
)
