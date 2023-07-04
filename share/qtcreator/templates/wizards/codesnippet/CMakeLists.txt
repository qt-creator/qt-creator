cmake_minimum_required(VERSION 3.5)

project(%{ProjectName} VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS %{QtModule})
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS %{QtModule})

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(%{ProjectName} MANUAL_FINALIZATION %{CppFileName})
else()
    if(ANDROID)
        add_library(%{ProjectName} SHARED %{CppFileName})
    else()
        add_executable(%{ProjectName} %{CppFileName})
    endif()
endif()

target_link_libraries(%{ProjectName} PRIVATE Qt${QT_VERSION_MAJOR}::%{QtModule})

include(GNUInstallDirs)
install(TARGETS %{ProjectName}
@if %{MacOSBundle}
    BUNDLE DESTINATION .
@endif
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

set_target_properties(%{ProjectName} PROPERTIES MACOSX_BUNDLE %{MacOSBundleValue})

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(%{ProjectName})
endif()
