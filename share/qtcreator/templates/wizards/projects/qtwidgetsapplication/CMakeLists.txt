cmake_minimum_required(VERSION 3.5)

project(%{ProjectName} VERSION 0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

@if %{HasTranslation}
find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets LinguistTools REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets LinguistTools REQUIRED)

set(TS_FILES %{TsFileName})
@else
find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets REQUIRED)
@endif

set(PROJECT_SOURCES
        %{MainFileName}
        %{SrcFileName}
        %{HdrFileName}
    @if %{GenerateForm}
        %{FormFileName}
    @endif
    @if %{HasTranslation}
        ${TS_FILES}
    @endif
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(%{ProjectName}
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET %{ProjectName} APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
@if %{HasTranslation}

    qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
@endif
else()
    if(ANDROID)
        add_library(%{ProjectName} SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(%{ProjectName}
            ${PROJECT_SOURCES}
        )
    endif()
@if %{HasTranslation}

    qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
@endif
endif()

target_link_libraries(%{ProjectName} PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

set_target_properties(%{ProjectName} PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(%{ProjectName})
endif()
