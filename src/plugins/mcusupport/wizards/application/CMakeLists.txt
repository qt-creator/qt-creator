cmake_minimum_required (VERSION 3.15)

project(%{CorrectedProjectName} VERSION 0.0.1 LANGUAGES C CXX ASM)

find_package(Qul)

qul_add_target(%{CorrectedProjectName})
qul_target_qml_sources(%{CorrectedProjectName} %{MainQmlFile})
app_target_setup_os(%{CorrectedProjectName})
app_target_default_entrypoint(%{CorrectedProjectName} %{RootItemName})
