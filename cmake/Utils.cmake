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
    get_target_property(_qmake_binary Qt5::qmake IMPORTED_LOCATION)
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
        set(_ide_app_target \"\${CMAKE_INSTALL_PREFIX}/${IDE_APP_PATH}/${IDE_APP_TARGET}${CMAKE_EXECUTABLE_SUFFIX}\")
        if (NOT EXISTS \"\${_ide_app_target}\")
          # The component CPack generators (WIX, NSIS64, IFW) install every component with their own CMAKE_INSTALL_PREFIX
          # directory and since deployqt.py needs the path to IDE_APP_TARGET the line below is needeed
          string(REPLACE \"Dependencies\" \"${CMAKE_INSTALL_DEFAULT_COMPONENT_NAME}\" _ide_app_target \"\${_ide_app_target}\")
        endif()
        if (NOT EXISTS \"\${_ide_app_target}\")
          # On Linux with the DEB generator the CMAKE_INSTALL_PREFIX is relative and the DESTDIR environment variable is needed
          # to point out to the IDE_APP_TARGET binary
          set(_ide_app_target \"\$ENV{DESTDIR}/\${CMAKE_INSTALL_PREFIX}/${IDE_APP_PATH}/${IDE_APP_TARGET}${CMAKE_EXECUTABLE_SUFFIX}\")
        endif()
        execute_process(COMMAND
          \"${Python3_EXECUTABLE}\"
          \"${CMAKE_CURRENT_LIST_DIR}/scripts/deployqt.py\"
          ${_llvm_arg}
          ${_elfutils_arg}
          \"\${_ide_app_target}\"
          \"${_qmake_binary}\"
          COMMAND_ECHO STDOUT
          )
      "
      COMPONENT Dependencies
      EXCLUDE_FROM_ALL
    )
  endif()
endfunction()
