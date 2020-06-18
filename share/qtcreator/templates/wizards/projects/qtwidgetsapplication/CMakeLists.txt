cmake_minimum_required(VERSION 3.5)

project(%{ProjectName} LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# QtCreator supports the following variables for Android, which are identical to qmake Android variables.
# Check http://doc.qt.io/qt-5/deployment-android.html for more information.
# They need to be set before the find_package(Qt5 ...) call.

#if(ANDROID)
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
#    if (ANDROID_ABI STREQUAL "armeabi-v7a")
#        set(ANDROID_EXTRA_LIBS
#            ${CMAKE_CURRENT_SOURCE_DIR}/path/to/libcrypto.so
#            ${CMAKE_CURRENT_SOURCE_DIR}/path/to/libssl.so)
#    endif()
#endif()

@if %{HasTranslation}
find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets LinguistTools REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets LinguistTools REQUIRED)

set(TS_FILES %{TsFileName})
@else
find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets REQUIRED)
@endif

if(ANDROID)
  add_library(%{ProjectName} SHARED
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
else()
  add_executable(%{ProjectName}
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
endif()

target_link_libraries(%{ProjectName} PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
@if %{HasTranslation}

qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
@endif
