# The ETW capture tools are used by the Windows call-stack sampler only.
if (NOT WIN32)
  return()
endif()

add_qtc_executable(etwcapture
  CONDITION TARGET Profiler
  DEPENDS Profiler Utils Qt::Core
  SOURCES
    etwcapture.cpp
)

# etwcapture.exe is not next to the Profiler plugin it links, so the launcher
# puts the plugin directory on the PATH it hands to it.
file(RELATIVE_PATH etwcapture_plugin_path "/${IDE_LIBEXEC_PATH}" "/${IDE_PLUGIN_PATH}")

# Pure Win32/C++ with no Qt dependencies. Its resource script embeds the
# requireAdministrator manifest, which is what makes ShellExecuteEx elevate it.
add_qtc_executable(etwcapture-launcher
  CONDITION TARGET etwcapture
  SKIP_PCH
  DEFINES ETWCAPTURE_PLUGIN_PATH=L\"${etwcapture_plugin_path}\"
  PROPERTIES WIN32_EXECUTABLE ON qt_no_entrypoint ON
  SOURCES
    etwcapture-launcher.cpp
    etwcapture-launcher.rc
)

if (MSVC AND TARGET etwcapture-launcher)
  target_link_options(etwcapture-launcher PRIVATE "LINKER:/MANIFEST:NO")
endif()
