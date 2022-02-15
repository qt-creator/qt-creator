cmake_minimum_required (VERSION 3.15)

project(%{ProjectName} VERSION 0.0.1 LANGUAGES C CXX ASM ASM_MASM)

find_package(Qul)

qul_add_target(%{ProjectName})
qul_target_qml_sources(%{ProjectName} %{MainQmlFile})
app_target_setup_os(%{ProjectName})
app_target_default_entrypoint(%{ProjectName} %{RootItemName})
