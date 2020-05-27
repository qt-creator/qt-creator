cmake_minimum_required (VERSION 3.13)

project(%{ProjectName} VERSION 0.0.1 LANGUAGES C CXX ASM)

find_package(Qul)

add_executable(%{ProjectName} ${OS}/main.cpp)
qul_target_qml_sources(%{ProjectName} %{MainQmlFile})

target_link_libraries(%{ProjectName} Qul::QuickUltralite)
app_target_setup_os(%{ProjectName})
