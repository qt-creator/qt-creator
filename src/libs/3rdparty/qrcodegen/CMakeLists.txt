add_qtc_library(QrCodeGenerator STATIC
  CONDITION TARGET Qt6::Quick AND TARGET Qt6::Svg
  PUBLIC_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/src
  DEFINES QRCODEGENERATOR_LIBRARY
  DEPENDS
    Qt6::Qml Qt6::Quick Qt6::Svg
  SOURCES
    src/qrcodegen.cpp
    src/qrcodegen.h
    src/qrcodeimageprovider.cpp
    src/qrcodeimageprovider.h
  SBOM_ARGS
    SBOM_ENTITY_TYPE THIRD_PARTY_LIBRARY_WITH_FILES
    USE_ATTRIBUTION_FILES
    ATTRIBUTION_FILE_PATHS
      "${QtCreator_SOURCE_DIR}/qt_attributions.json"
    ATTRIBUTION_IDS
      qtqrcodegenerator
)
