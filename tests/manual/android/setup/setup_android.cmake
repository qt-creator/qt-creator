# CMake script to download OpenJDK and Android Command Line Tools.
# Execute with: cmake -P setup_android.cmake

set(JDK_VERSION "8u292-b10")

set(ANDROID_CMDTOOLS_VERSION "6609375")
set(ANDROID_PLATFORM "android-30")
set(BUILD_TOOLS "30.0.2")
set(NDK_VERSION "21.3.6528147")

set(qtc_android_sdk_definitions "${CMAKE_CURRENT_LIST_DIR}/../../../../share/qtcreator/android/sdk_definitions.json")

if (EXISTS ${qtc_android_sdk_definitions} AND CMAKE_VERSION GREATER_EQUAL 3.19)
  file(READ ${qtc_android_sdk_definitions} sdk_definitions)

  string(JSON linux_url GET "${sdk_definitions}" common sdk_tools_url linux)
  string(REGEX REPLACE "^.*commandlinetools-linux-\([0-9]+\)_latest.zip" "\\1" ANDROID_CMDTOOLS_VERSION "${linux_url}")

  string(JSON essential_packages GET "${sdk_definitions}" common sdk_essential_packages default)
  string(REGEX REPLACE "^.*\"platforms;\(android-[0-9]+\)\",.*$" "\\1" ANDROID_PLATFORM "${essential_packages}")

  string(JSON sdk_essential_packages GET "${sdk_definitions}" specific_qt_versions 0 sdk_essential_packages)
  string(REGEX REPLACE "^.*\"build-tools;\([0-9.]+\)\".*$" "\\1" BUILD_TOOLS "${sdk_essential_packages}")
  string(REGEX REPLACE "^.*\"ndk;\([0-9.]+\)\".*$" "\\1" NDK_VERSION "${sdk_essential_packages}")

  foreach(var ANDROID_CMDTOOLS_VERSION ANDROID_PLATFORM BUILD_TOOLS NDK_VERSION)
    message("${var}: ${${var}}")
  endforeach()
endif()

function(download_jdk)
  string(REPLACE "-" "" version_no_dash ${JDK_VERSION})
  if (WIN32)
    set(jdk_suffix "windows_hotspot_${version_no_dash}.zip")
  elseif(APPLE)
    set(jdk_suffix "mac_hotspot_${version_no_dash}.tar.gz")
  else()
    set(jdk_suffix "linux_hotspot_${version_no_dash}.tar.gz")
  endif()

  set(jdk_url "https://github.com/AdoptOpenJDK/openjdk8-binaries/releases/download/jdk${JDK_VERSION}/OpenJDK8U-jdk_x64_${jdk_suffix}")

  message("Downloading: ${jdk_url}")
  file(DOWNLOAD ${jdk_url} ./jdk.zip SHOW_PROGRESS)
  execute_process(COMMAND ${CMAKE_COMMAND} -E tar xf ./jdk.zip)
endfunction()

function(download_android_commandline)
  if (WIN32)
    set(android_cmdtools_suffix "win")
  elseif(APPLE)
    set(android_cmdtools_suffix "mac")
  else()
    set(android_cmdtools_suffix "linux")
  endif()

  set(android_cmdtools_url "https://dl.google.com/android/repository/commandlinetools-${android_cmdtools_suffix}-${ANDROID_CMDTOOLS_VERSION}_latest.zip")

  message("Downloading: ${android_cmdtools_url}")
  file(DOWNLOAD ${android_cmdtools_url} ./android_commandline_tools.zip SHOW_PROGRESS)
  file(MAKE_DIRECTORY android-sdk)
  file(MAKE_DIRECTORY android-cmdlinetools)
  execute_process(COMMAND ${CMAKE_COMMAND} -E tar xf ../android_commandline_tools.zip WORKING_DIRECTORY android-cmdlinetools)
endfunction()

function(setup_android)
  set(sdkmanager "${CMAKE_CURRENT_LIST_DIR}/android-cmdlinetools/tools/bin/sdkmanager")
  if (WIN32)
    set(sdkmanager "${sdkmanager}.bat")
  endif()

  set(ENV{JAVA_HOME} "${CMAKE_CURRENT_LIST_DIR}/jdk${JDK_VERSION}")
  if (APPLE)
    set(ENV{JAVA_HOME} "$ENV{JAVA_HOME}/Contents/Home")
  endif()

  file(WRITE ${CMAKE_CURRENT_LIST_DIR}/accept_license.txt "y\ny\ny\ny\ny\ny\ny\ny\ny\ny\n")
  execute_process(
    INPUT_FILE ${CMAKE_CURRENT_LIST_DIR}/accept_license.txt
    COMMAND ${sdkmanager} --licenses --sdk_root=${CMAKE_CURRENT_LIST_DIR}/android-sdk)
   execute_process(
     COMMAND ${sdkmanager} --update --sdk_root=${CMAKE_CURRENT_LIST_DIR}/android-sdk)
   execute_process(
     COMMAND ${sdkmanager}
        "platforms;${ANDROID_PLATFORM}"
        "build-tools;${BUILD_TOOLS}"
        "ndk;${NDK_VERSION}"
        "platform-tools"
        "cmdline-tools;latest"
        "tools"
        "emulator"
        "ndk-bundle" --sdk_root=${CMAKE_CURRENT_LIST_DIR}/android-sdk)

  if (WIN32)
    execute_process(
      COMMAND ${sdkmanager}
        "extras;google;usb_driver"
        --sdk_root=${CMAKE_CURRENT_LIST_DIR}/android-sdk)
  endif()
endfunction()

download_jdk()
download_android_commandline()
setup_android()
