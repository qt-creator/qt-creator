cmake_minimum_required (VERSION 3.13)

project(%{ProjectName} VERSION 0.0.1 LANGUAGES CXX)

find_package(Qul)

add_executable(%{ProjectName})
qul_target_qml_sources(%{ProjectName} %{MainQmlFile})

target_link_libraries(%{ProjectName} Qul::QuickUltralite)
