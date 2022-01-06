cmake_minimum_required(VERSION 3.16)

project(accelbubble VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.2 COMPONENTS Quick Sensors Svg Xml REQUIRED)

qt_add_executable(appaccelbubble
    main.cpp
    MANUAL_FINALIZATION
)
set_target_properties(appaccelbubble PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist"
)
set_property(TARGET appaccelbubble APPEND PROPERTY
    QT_ANDROID_PACKAGE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/android
)
qt_add_qml_module(appaccelbubble
    URI accelbubble
    VERSION 1.0
    QML_FILES main.qml
    RESOURCES Bluebubble.svg
)

target_compile_definitions(appaccelbubble
    PRIVATE $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>)
target_link_libraries(appaccelbubble
    PRIVATE Qt6::Quick Qt6::Sensors Qt6::Svg Qt6::Xml)

qt_finalize_executable(appaccelbubble)
