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
    set(QMLDESIGNER_QT6_REQUIRED_VERSION 6.5.4)
    set(QMLDESIGNER_GCC_REQUIRED_VERSION 10.0)
    set(QMLDESIGNER_CLANG_REQUIRED_VERSION 13.0)
    set(QMLDESIGNER_APPLECLANG_REQUIRED_VERSION 15.0)

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
        else()
            set(QTC_WITH_QMLDESIGNER_DEFAULT ON)
        endif()
    else()
        set(QTC_WITH_QMLDESIGNER_DEFAULT OFF)
    endif()

    env_with_default("QTC_WITH_QMLDESIGNER" ENV_QTC_WITH_QMLDESIGNER ${QTC_WITH_QMLDESIGNER_DEFAULT})
    option(WITH_QMLDESIGNER "Build QmlDesigner" ${ENV_QTC_WITH_QMLDESIGNER})
    add_feature_info("WITH_QMLDESIGNER" ${WITH_QMLDESIGNER} "${QMLDESIGNER_FEATURE_DESC}")

    set(QTC_IS_SUPPORTED_PROJECTSTORAGE_QT_DEFAULT OFF)
    if(Qt6_VERSION VERSION_GREATER_EQUAL 6.7.3 AND Qt6_VERSION VERSION_LESS 6.8.0)
        set(QTC_IS_SUPPORTED_PROJECTSTORAGE_QT_DEFAULT ON)
    endif()
    env_with_default("QTC_IS_SUPPORTED_PROJECTSTORAGE_QT" ENV_QTC_IS_SUPPORTED_PROJECTSTORAGE_QT ${QTC_IS_SUPPORTED_PROJECTSTORAGE_QT_DEFAULT})
    option(IS_SUPPORTED_PROJECTSTORAGE_QT "IS_SUPPORTED_PROJECTSTORAGE_QT" ${ENV_QTC_IS_SUPPORTED_PROJECTSTORAGE_QT})
    add_feature_info("IS_SUPPORTED_PROJECTSTORAGE_QT" ${IS_SUPPORTED_PROJECTSTORAGE_QT} "is ${IS_SUPPORTED_PROJECTSTORAGE_QT}")
endfunction()
