cmake_minimum_required(VERSION 3.16)

project(%{ProjectName} VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
@if %{GenerateQmllsIniCondition} && %{GenerateQmllsIni}
set(QT_QML_GENERATE_QMLLS_INI ON)
@endif

find_package(Qt6 REQUIRED COMPONENTS Quick)

qt_standard_project_setup(REQUIRES %{MinimumSupportedQtVersion})

qt_add_executable(%{TargetName}
    main.cpp
)

qt_add_qml_module(%{TargetName}
    URI %{ProjectName}
    QML_FILES
        Main.qml
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(%{TargetName} PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.%{TargetName}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(%{TargetName}
    PRIVATE Qt6::Quick
)

install(TARGETS %{TargetName}
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
