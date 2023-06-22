cmake_minimum_required (VERSION 3.21.1)

project(%{CorrectedProjectName} VERSION 0.0.1 LANGUAGES C CXX ASM)

find_package(Qul)

qul_add_target(%{CorrectedProjectName} QML_PROJECT %{QmlProjectFile} GENERATE_ENTRYPOINT)
app_target_setup_os(%{CorrectedProjectName})
