cmake_minimum_required(VERSION 3.16)

project(%{ProjectName} VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

find_package(Qt6 COMPONENTS Core Gui Quick Quick3D Quick3DXr)

qt_standard_project_setup(REQUIRES 6.8)
qt6_policy(SET QTP0002 NEW)

qt_add_executable(${CMAKE_PROJECT_NAME}
    MANUAL_FINALIZATION
    main.cpp
    android/AndroidManifest.xml
)

qt_add_qml_module(${CMAKE_PROJECT_NAME}
    URI ${CMAKE_PROJECT_NAME}
    VERSION 1.0
    QML_FILES
        Main.qml
)

if (APPLE AND CMAKE_SYSTEM_NAME STREQUAL "visionOS")
    set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE_GUI_IDENTIFIER io.qt.${CMAKE_PROJECT_NAME}
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/visionos/MacOSXBundleInfo.plist.in
    )
endif()

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
        Qt::Core
        Qt::Gui
        Qt::Quick
        Qt::Quick3D
        Qt::Quick3DXr
)

if(ANDROID)
    set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES
            QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android"
            QT_ANDROID_PACKAGE_NAME "org.qtproject.example.${CMAKE_PROJECT_NAME}"
            QT_ANDROID_APP_NAME "${CMAKE_PROJECT_NAME}"
            QT_ANDROID_TARGET_SDK_VERSION 32
            QT_ANDROID_MIN_SDK_VERSION 32
            QT_ANDROID_VERSION_NAME "1.0")
endif()

qt_finalize_executable(${CMAKE_PROJECT_NAME})
install(TARGETS ${CMAKE_PROJECT_NAME}
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

