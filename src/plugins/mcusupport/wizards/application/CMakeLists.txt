cmake_minimum_required (VERSION 3.15)

project(%{ProjectName} VERSION 0.0.1 LANGUAGES C CXX ASM)

find_package(Qul)

if(Qul_VERSION VERSION_GREATER_EQUAL "1.7")
    qul_add_target(%{ProjectName})
else()
    add_executable(%{ProjectName})
    target_link_libraries(%{ProjectName}
        Qul::QuickUltralite
        Qul::QuickUltralitePlatform)
endif()

qul_target_qml_sources(%{ProjectName} %{MainQmlFile})

app_target_setup_os(%{ProjectName})
app_target_default_main(%{ProjectName} %{RootItemName})
