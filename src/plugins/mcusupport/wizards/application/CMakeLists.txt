cmake_minimum_required (VERSION 3.13)

project(%{ProjectName} VERSION 0.0.1 LANGUAGES CXX)

find_package(Qul)

add_executable(%{ProjectName})
QUL_ADD_QML(%{ProjectName} %{MainQmlFile})

target_link_libraries(%{ProjectName} Qul::QuickUltralite)
