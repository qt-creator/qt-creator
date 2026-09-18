cmake_minimum_required(VERSION 3.16)

project(HelloQt VERSION 0.1 LANGUAGES CXX)

qt_standard_project_setup(REQUIRES 6.5)

qt_add_executable(HelloQt
    main.cpp
)

set_target_properties(HelloQt PROPERTIES AUTORCC ON)

target_sources(HelloQt
    PRIVATE
        assets.qrc
)

qt_add_qml_module(HelloQt
    URI HelloQt
    QML_FILES Main.qml
)

target_link_libraries(HelloQt PRIVATE Qt6::Quick)
