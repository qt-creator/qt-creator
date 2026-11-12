if (WIN32 AND NOT QTC_STATIC_BUILD)
  # Windows has no rpath to say where the plugins the tool links are. A launcher
  # finds them for it, and the tool is a library next to the launcher.
  add_qtc_library(QtProfilerCli
    CONDITION TARGET Profiler
    DEPENDS app_version Utils QtTaskTree
    SOURCES
      qtprofilercli.cpp
  )
  if (TARGET QtProfilerCli)
    # Not a library others link, but the tool's program, so it may use a plugin,
    # which add_qtc_library's DEPENDS refuses.
    target_link_libraries(QtProfilerCli PRIVATE Profiler)
  endif()
  add_qtc_executable(qtprofiler-cli
    CONDITION TARGET QtProfilerCli
    DESTINATION "${IDE_LIBEXEC_PATH}"
    SOURCES
      qtprofilercli_win.cpp
  )
  if (TARGET qtprofiler-cli)
    add_dependencies(qtprofiler-cli QtProfilerCli)
    target_compile_definitions(qtprofiler-cli PRIVATE
      QTPROFILERCLI_DLL="$<TARGET_FILE_NAME:QtProfilerCli>")
  endif()
else()
  add_qtc_executable(qtprofiler-cli
    CONDITION TARGET Profiler
    DEPENDS app_version Profiler Utils QtTaskTree
    DESTINATION "${IDE_LIBEXEC_PATH}"
    SOURCES
      qtprofilercli.cpp
  )
endif()

# The call-stack sampler needs the debugger entitlement here just as much as in
# Qt Creator itself.
if (APPLE AND TARGET qtprofiler-cli)
  qtc_sign_with_entitlements(qtprofiler-cli
    "${QtCreator_SOURCE_DIR}/dist/installer/mac/Qt Profiler.entitlements")
endif()
