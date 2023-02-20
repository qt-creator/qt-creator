cmake_minimum_required (VERSION 3.21.1)

project(%{ProjectName} VERSION 0.0.1 LANGUAGES C CXX ASM)

if (NOT TARGET Qul::Core)
    find_package(Qul)
endif()

if (Qul_VERSION VERSION_GREATER_EQUAL "2.4")
    qul_add_target(%{ProjectName} QML_PROJECT %{ProjectName}.qmlproject GENERATE_ENTRYPOINT)
    app_target_setup_os(%{ProjectName})
else()
    if (Qul_VERSION VERSION_GREATER_EQUAL "1.7")
        qul_add_target(%{ProjectName})
    else()
        add_executable(%{ProjectName})
        target_link_libraries(%{ProjectName}
            Qul::QuickUltralite
            Qul::QuickUltralitePlatform)
    endif()

    if (Qul_VERSION VERSION_GREATER_EQUAL "2.0")
        file(GLOB_RECURSE fontSources "${CMAKE_CURRENT_SOURCE_DIR}/fonts/*.ttf")
        set_property(TARGET %{ProjectName} APPEND PROPERTY QUL_FONT_FILES ${fontSources})
    elseif (Qul_VERSION VERSION_GREATER_EQUAL "1.7")
        set_property(TARGET %{ProjectName} APPEND PROPERTY QUL_FONTS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/fonts")
    else()
        set(QUL_FONTS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/fonts,${QUL_FONTS_DIR}")
    endif()

    # Using recurse search to find image files in project directory
    # Excluding MCUDefaultStyle because it exists for compatibility purposes with QDS
    file(GLOB_RECURSE imgSources "*.png" "*.svg" "*.jpg" "*.jpeg")
    list(FILTER imgSources EXCLUDE REGEX ".*/MCUDefaultStyle/.*")

    if(imgSources)
        qul_add_resource(%{ProjectName} FILES ${imgSources})
    endif()

    # Registering singletons as qml module
    qul_add_qml_module(ConstantsModule
        URI Constants
        QML_FILES
            imports/Constants/Constants.qml
    )

    message(WARNING "It is recommended to replace the recursive search with the actual list of .qml files in your project.")
    file(GLOB_RECURSE qmlSources "*.qml")
    # Excluding Constants folder because it is part of another qml module
    list(FILTER qmlSources EXCLUDE REGEX ".*/imports/Constants/.*")
    # Excluding MCUDefaultStyle because it exists for compatibility purposes with QDS
    list(FILTER qmlSources EXCLUDE REGEX ".*/MCUDefaultStyle/.*")
    # Excluding binary directory because it can break builds in source dir
    list(FILTER qmlSources EXCLUDE REGEX "${CMAKE_CURRENT_BINARY_DIR}/.*")
    qul_target_qml_sources(%{ProjectName} ${qmlSources})

    if (Qul_VERSION VERSION_GREATER_EQUAL "2.0")
        target_link_libraries(%{ProjectName} PRIVATE
            Qul::Timeline
            Qul::Controls
            Qul::Shapes
            ConstantsModule)
    else()
        target_link_libraries(%{ProjectName}
            Qul::QuickUltraliteTimeline
            Qul::QuickUltraliteControlsStyleDefault
            ConstantsModule)

        if (Qul_VERSION VERSION_GREATER_EQUAL "1.8")
            target_link_libraries(%{ProjectName}
                Qul::QuickUltraliteShapes)
        endif()
    endif()

    app_target_setup_os(%{ProjectName})

    if (Qul_VERSION VERSION_GREATER_EQUAL "1.7")
        app_target_default_entrypoint(%{ProjectName} %{RootItemName})
    else()
        app_target_default_main(%{ProjectName} %{RootItemName})
    endif()
endif()
