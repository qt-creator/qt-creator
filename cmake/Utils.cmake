function (env_with_default envName varToSet default)
  if(DEFINED ENV{${envName}})
    set(${varToSet} $ENV{${envName}} PARENT_SCOPE)
  else()
    set(${varToSet} ${default} PARENT_SCOPE)
  endif()
endfunction()

function(setup_dependencies_component)
  find_package(Python3 COMPONENTS Interpreter)
  if (NOT Python3_Interpreter_FOUND)
    message("No python interpreter found, skipping \"Dependencies\" install component.")
  else()
    get_target_property(_qmake_binary Qt::qmake IMPORTED_LOCATION)
    set(_llvm_arg)
    if (LLVM_INSTALL_PREFIX)
      set(_llvm_arg "--llvm \"${LLVM_INSTALL_PREFIX}\"")
    endif()
    set(_elfutils_arg)
    if (ELFUTILS_INCLUDE_DIR)
      get_filename_component(_elfutils_path ${ELFUTILS_INCLUDE_DIR} DIRECTORY)
      set(_elfutils_arg "--elfutils \"${_elfutils_path}\"")
    endif()
    install(CODE "
        if (CMAKE_VERSION GREATER_EQUAL 3.19)
          set(QTC_COMMAND_ERROR_IS_FATAL COMMAND_ERROR_IS_FATAL ANY)
        endif()
        # DESTDIR is set for e.g. the cpack DEB generator, but is empty in other situations
        if(DEFINED ENV{DESTDIR})
          set(DESTDIR_WITH_SEP \"\$ENV{DESTDIR}/\")
        else()
          set(DESTDIR_WITH_SEP \"\")
        endif()
        set(_default_app_target \"\${DESTDIR_WITH_SEP}\${CMAKE_INSTALL_PREFIX}/${IDE_APP_PATH}/${IDE_APP_TARGET}${CMAKE_EXECUTABLE_SUFFIX}\")
        set(_ide_app_target \"\${_default_app_target}\")
        if (NOT EXISTS \"\${_ide_app_target}\")
          # The component CPack generators (WIX, NSIS64, IFW) install every component with their own CMAKE_INSTALL_PREFIX
          # directory and since deploy.py needs the path to IDE_APP_TARGET the line below is needeed
          string(REPLACE \"Dependencies\" \"${CMAKE_INSTALL_DEFAULT_COMPONENT_NAME}\" _ide_app_target \"\${_ide_app_target}\")
        endif()
        if (NOT EXISTS \"\${_ide_app_target}\")
          # something went wrong, reset to default and hope for the best
          set(_ide_app_target \"\${_default_app_target}\")
        endif()
        execute_process(COMMAND
          \"${Python3_EXECUTABLE}\"
          \"-u\"
          \"${CMAKE_CURRENT_LIST_DIR}/scripts/deploy.py\"
          ${_llvm_arg}
          ${_elfutils_arg}
          \"\${_ide_app_target}\"
          \"${_qmake_binary}\"
          COMMAND_ECHO STDOUT
          ${QTC_COMMAND_ERROR_IS_FATAL}
          )
      "
      COMPONENT Dependencies
      EXCLUDE_FROM_ALL
    )
  endif()
endfunction()

function(configure_qml_designer Qt6_VERSION)
    set(QMLDESIGNER_QT6_REQUIRED_VERSION 6.7.3)
    set(QMLDESIGNER_GCC_REQUIRED_VERSION 10.0)
    set(QMLDESIGNER_CLANG_REQUIRED_VERSION 13.0)
    set(QMLDESIGNER_APPLECLANG_REQUIRED_VERSION 15.0)
    set(QMLDESIGNER_MSVC_REQUIRED_VERSION 19.30) # means MSVC 2022
    set(PROJECTSTORAGE_QT_MIN_VERSION 6.8.1)
    set(PROJECTSTORAGE_QT_MAX_VERSION 6.9.0)

    string(CONCAT QMLDESIGNER_FEATURE_DESC
           "Needs a Qt ${QMLDESIGNER_QT6_REQUIRED_VERSION} or newer")

    if (Qt6_VERSION VERSION_GREATER_EQUAL ${QMLDESIGNER_QT6_REQUIRED_VERSION})
        if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND
            CMAKE_CXX_COMPILER_VERSION VERSION_LESS ${QMLDESIGNER_GCC_REQUIRED_VERSION})
            set(QTC_WITH_QMLDESIGNER_DEFAULT OFF)
            string(APPEND QMLDESIGNER_FEATURE_DESC " and at least GCC ${QMLDESIGNER_GCC_REQUIRED_VERSION}")
        elseif (CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang" AND
                CMAKE_CXX_COMPILER_VERSION VERSION_LESS ${QMLDESIGNER_APPLECLANG_REQUIRED_VERSION})
            set(QTC_WITH_QMLDESIGNER_DEFAULT OFF)
            string(APPEND QMLDESIGNER_FEATURE_DESC " and at least AppleClang ${QMLDESIGNER_APPLECLANG_REQUIRED_VERSION}")
        elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND
                CMAKE_CXX_COMPILER_VERSION VERSION_LESS ${QMLDESIGNER_CLANG_REQUIRED_VERSION})
            set(QTC_WITH_QMLDESIGNER_DEFAULT OFF)
            string(APPEND QMLDESIGNER_FEATURE_DESC " and at least Clang ${QMLDESIGNER_CLANG_REQUIRED_VERSION}")
        elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" AND
                CMAKE_CXX_COMPILER_VERSION VERSION_LESS ${QMLDESIGNER_MSVC_REQUIRED_VERSION})
            set(QTC_WITH_QMLDESIGNER_DEFAULT OFF)
            string(APPEND QMLDESIGNER_FEATURE_DESC " and at least MSVC 2022(ver. ${QMLDESIGNER_MSVC_REQUIRED_VERSION})")
        else()
            set(QTC_WITH_QMLDESIGNER_DEFAULT ON)
        endif()
        if (NOT QTC_WITH_QMLDESIGNER_DEFAULT)
            string(APPEND QMLDESIGNER_FEATURE_DESC ", found ${CMAKE_CXX_COMPILER_VERSION}")
        endif()
    else()
        set(QTC_WITH_QMLDESIGNER_DEFAULT OFF)
    endif()

    if(NOT TARGET Qt::Quick)
        set(QTC_WITH_QMLDESIGNER_DEFAULT OFF)
    endif()

    env_with_default("QTC_WITH_QMLDESIGNER" ENV_QTC_WITH_QMLDESIGNER ${QTC_WITH_QMLDESIGNER_DEFAULT})
    option(WITH_QMLDESIGNER "Build QmlDesigner" ${ENV_QTC_WITH_QMLDESIGNER})
    add_feature_info("WITH_QMLDESIGNER" ${WITH_QMLDESIGNER} "${QMLDESIGNER_FEATURE_DESC}")

    if(Qt6_VERSION VERSION_GREATER_EQUAL ${PROJECTSTORAGE_QT_MIN_VERSION} AND Qt6_VERSION VERSION_LESS ${PROJECTSTORAGE_QT_MAX_VERSION})
        set(IS_SUPPORTED_PROJECTSTORAGE_QT ON)
    else()
        set(IS_SUPPORTED_PROJECTSTORAGE_QT OFF)
    endif()
    set(IS_SUPPORTED_PROJECTSTORAGE_QT "${IS_SUPPORTED_PROJECTSTORAGE_QT}" PARENT_SCOPE)

    if(USE_PROJECTSTORAGE AND NOT IS_SUPPORTED_PROJECTSTORAGE_QT)
        if(BUILD_DESIGNSTUDIO)
            set(_level FATAL_ERROR)
        else()
            set(_level WARNING)
        endif()
        message(${_level}
            "USE_PROJECTSTORAGE is enabled, but current Qt ${Qt6_VERSION} is not supported by the project storage "
            "(required: ${PROJECTSTORAGE_QT_MIN_VERSION} - ${PROJECTSTORAGE_QT_MAX_VERSION})."
        )
    endif()

    # to enable define QML_DOM_MSVC2019_COMPAT if necessary, see QTBUG-127761
    if(NOT DEFINED QT_BUILT_WITH_MSVC2019)
        get_target_property(QT_QMAKE_EXECUTABLE Qt6::qmake IMPORTED_LOCATION)

        execute_process(
            COMMAND dumpbin /headers ${QT_QMAKE_EXECUTABLE} | findstr /c:"linker version"
            OUTPUT_VARIABLE DUMPBIN_OUTPUT
        )

        string(FIND "${DUMPBIN_OUTPUT}" "14.2" QT_BUILT_WITH_MSVC2019)

        if(QT_BUILT_WITH_MSVC2019 GREATER -1)
            set(QT_BUILT_WITH_MSVC2019 TRUE CACHE BOOL "Qt was built with MSVC 2019")
        else()
            set(QT_BUILT_WITH_MSVC2019 FALSE CACHE BOOL "Qt was not built with MSVC 2019")
        endif()
    endif()
endfunction()
