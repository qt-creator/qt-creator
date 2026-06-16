cmake_minimum_required(VERSION 3.16)
project(qmlmixtest LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
find_package(Qt6 REQUIRED COMPONENTS Quick)
qt_standard_project_setup(REQUIRES 6.5)

qt_add_executable(qmlmixtest main.cpp)
qt_add_qml_module(qmlmixtest
    URI MixTest
    VERSION 1.0
    QML_FILES Main.qml
    SOURCES backend.h backend.cpp
)
target_compile_definitions(qmlmixtest PRIVATE QT_QML_DEBUG)
target_link_libraries(qmlmixtest PRIVATE Qt6::Quick)
