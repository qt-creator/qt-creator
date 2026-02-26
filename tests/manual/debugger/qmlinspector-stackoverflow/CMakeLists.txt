cmake_minimum_required(VERSION 3.16)

project(qmlinspector-stackoverflow VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Quick QuickControls2 QmlPrivate)

qt_standard_project_setup(REQUIRES 6.5)

qt_add_executable(qmlinspector-stackoverflow
    main.cpp
)

qt_add_qml_module(qmlinspector-stackoverflow
    URI DeepNesting
    VERSION 1.0
    QML_FILES
        Main.qml
)

target_link_libraries(qmlinspector-stackoverflow
    PRIVATE Qt6::Quick Qt6::QuickControls2 Qt6::QmlPrivate
)
