cmake_minimum_required(VERSION 3.16)

set(QTKEYCHAIN_VERSION 0.17.0)
set(QTKEYCHAIN_SOVERSION 1)

project(qtkeychain VERSION ${QTKEYCHAIN_VERSION} LANGUAGES CXX)

include(FindPkgConfig)

###
# write binaries and libraries into a shared folder, this simplifies the execution of tests on Windows
# see https://github.com/KDE/extra-cmake-modules/blob/b3a13868f7f54ab0b7ac19fd26b6dfead8907d25/kde-modules/KDECMakeSettings.cmake#L256C1-L258C69
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/Modules")
include(GNUInstallDirs)
include(GenerateExportHeader)
include(CMakePackageConfigHelpers)
include(ECMSetupVersion)
include(ECMGeneratePriFile)
include(CMakeDependentOption)

if(CMAKE_SYSTEM_NAME STREQUAL Android)
    set(ANDROID 1)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL Haiku)
    set(HAIKU 1)
endif()

option(BUILD_WITH_QT5 "Build qtkeychain with Qt 5 (default is Qt 6)" OFF)
option(BUILD_TEST_APPLICATION "Build test application (CLI)" OFF)
option(BUILD_QTQUICK_DEMO "Build QtQuick demo application" OFF)
option(BUILD_TRANSLATIONS "Build translations" ON)
CMAKE_DEPENDENT_OPTION(BUILD_SHARED_LIBS "Build dynamic library" ON "NOT IOS" OFF)
if(QTKEYCHAIN_STATIC)
    set(BUILD_SHARED_LIBS OFF)
    message(WARNING "QTKEYCHAIN_STATIC is deprecated. Use BUILD_SHARED_LIBS=OFF instead.")
endif()
CMAKE_DEPENDENT_OPTION(BUILD_TRANSLATIONS_AS_RESOURCES "Bundle translations with the library" OFF
                       "BUILD_TRANSLATIONS" OFF)

if (WIN32)
    option(USE_CREDENTIAL_STORE "Build with windows CredentialStore support" ON)

    if (USE_CREDENTIAL_STORE)
        add_definitions(-DUSE_CREDENTIAL_STORE=1)
    endif()

    option(USE_COMPAT_NAMING_SCHEME "store entries without the service name" OFF)
    if (USE_COMPAT_NAMING_SCHEME)
        add_definitions(-DUSE_COMPAT_NAMING_SCHEME=1)
    endif()
endif()

if(BUILD_WITH_QT5)
    set(QT_MAJOR_VERSION 5)
    set(CMAKE_CXX_STANDARD 11)
else()
    set(QT_MAJOR_VERSION 6)
    set(CMAKE_CXX_STANDARD 17)
endif()

find_package(Qt${QT_MAJOR_VERSION} COMPONENTS Core REQUIRED)
set(QTKEYCHAIN_VERSION_INFIX ${QT_MAJOR_VERSION})

if (QT_MAJOR_VERSION EQUAL 5)
  if(ANDROID)
    if(Qt5Core_VERSION VERSION_LESS 5.7)
        find_package(Qt5 COMPONENTS Core REQUIRED Private)
        include_directories(${Qt5Core_PRIVATE_INCLUDE_DIRS})
    endif()

    find_package(Qt5 COMPONENTS AndroidExtras REQUIRED)
    include_directories(${Qt5AndroidExtras_INCLUDE_DIRS})
    set(QTANDROIDEXTRAS_LIBRARIES ${Qt5AndroidExtras_LIBRARIES})
  endif()

  if(UNIX AND NOT APPLE AND NOT ANDROID AND NOT HAIKU AND NOT EMSCRIPTEN)
    find_package(Qt5 COMPONENTS DBus REQUIRED)
    include_directories(${Qt5DBus_INCLUDE_DIRS})
    set(QTDBUS_LIBRARIES ${Qt5DBus_LIBRARIES})
    macro(qt_add_dbus_interface)
      qt5_add_dbus_interface(${ARGN})
    endmacro()
  endif()

  if(BUILD_TRANSLATIONS)
    find_package(Qt5 COMPONENTS LinguistTools REQUIRED)
    macro(qt_add_translation)
      qt5_add_translation(${ARGN})
    endmacro(qt_add_translation)
    macro(qt_create_translation)
      qt5_create_translation(${ARGN})
    endmacro(qt_create_translation)
  endif()

  macro(qt_wrap_cpp)
    qt5_wrap_cpp(${ARGN})
  endmacro()

  set(QTCORE_LIBRARIES ${Qt5Core_LIBRARIES})
  include_directories(${Qt5Core_INCLUDE_DIRS})
else()
  if(UNIX AND NOT APPLE AND NOT ANDROID AND NOT HAIKU AND NOT EMSCRIPTEN)
    find_package(Qt6 COMPONENTS DBus REQUIRED)
    set(QTDBUS_LIBRARIES Qt6::DBus)
    macro(qt_add_dbus_interface)
      qt6_add_dbus_interface(${ARGN})
    endmacro()
  endif()

  if(BUILD_TRANSLATIONS)
    find_package(Qt6 COMPONENTS LinguistTools REQUIRED)
    macro(qt_add_translation)
      qt6_add_translation(${ARGN})
    endmacro(qt_add_translation)
    macro(qt_create_translation)
      qt6_create_translation(${ARGN})
    endmacro(qt_create_translation)
  endif()

  macro(qt_wrap_cpp)
    qt6_wrap_cpp(${ARGN})
  endmacro()

  set(QTCORE_LIBRARIES Qt6::Core)
endif()

set(QTKEYCHAIN_TARGET_NAME qt${QTKEYCHAIN_VERSION_INFIX}keychain)

add_subdirectory(qtkeychain)

###
### Translations
###

set(qtkeychain_TR_FILES
      translations/qtkeychain_de.ts
      translations/qtkeychain_fr.ts
      translations/qtkeychain_ka.ts
      translations/qtkeychain_ro.ts
      translations/qtkeychain_ru.ts
      translations/qtkeychain_sv.ts
      translations/qtkeychain_zh.ts
)

file(GLOB qtkeychain_TR_SOURCES qtkeychain/*.cpp qtkeychain/*.h qtkeychain/*.ui)
if ( BUILD_TRANSLATIONS )
    qt_create_translation(qtkeychain_MESSAGES ${qtkeychain_TR_SOURCES} ${qtkeychain_TR_FILES})
    qt_add_translation(qtkeychain_QM_FILES ${qtkeychain_TR_FILES})
    add_custom_target(messages DEPENDS ${qtkeychain_MESSAGES})
    add_custom_target(translations DEPENDS ${qtkeychain_QM_FILES} messages)
    # https://github.com/frankosterfeld/qtkeychain/issues/185
    add_dependencies(${QTKEYCHAIN_TARGET_NAME} translations)

    if (BUILD_TRANSLATIONS_AS_RESOURCES)
        set(QM_FILE_LIST "")
        foreach(FILE ${qtkeychain_QM_FILES})
            list(APPEND QM_FILE_LIST "<file>${FILE}</file>")
        endforeach()
        string(REPLACE ";" "" QM_FILE_LIST ${QM_FILE_LIST})
        configure_file(${CMAKE_CURRENT_SOURCE_DIR}/translations/translations.qrc.in ${CMAKE_CURRENT_BINARY_DIR}/translations.qrc)
        target_sources(${QTKEYCHAIN_TARGET_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/translations.qrc)
    else()
        if(QTKEYCHAIN_VERSION_INFIX EQUAL 5 AND QT_TRANSLATIONS_DIR AND NOT QTKEYCHAIN_TRANSLATIONS_DIR)
            # Back compatibility with pre-0.11 versions
            message (WARNING "QT_TRANSLATIONS_DIR is deprecated, use QTKEYCHAIN_TRANSLATIONS_DIR instead")
            set(QTKEYCHAIN_TRANSLATIONS_DIR ${QT_TRANSLATIONS_DIR}
                CACHE PATH "The location of the QtKeychain translations" FORCE)
        else()
            set(QTKEYCHAIN_TRANSLATIONS_DIR
                ${CMAKE_INSTALL_DATADIR}/qt${QTKEYCHAIN_VERSION_INFIX}keychain/translations
                CACHE PATH "The location of the QtKeychain translations" )
        endif()

        install(FILES ${qtkeychain_QM_FILES} DESTINATION ${QTKEYCHAIN_TRANSLATIONS_DIR})
    endif()
endif( BUILD_TRANSLATIONS )


###
### Test applications
###

if(BUILD_TEST_APPLICATION)
    set( testclient_LIBRARIES ${QTKEYCHAIN_TARGET_NAME} )

    if(APPLE)
        list(APPEND testclient_LIBRARIES "-framework Cocoa")

        find_package(Qt${QT_MAJOR_VERSION} COMPONENTS Gui REQUIRED)
        list(APPEND testclient_LIBRARIES Qt${QT_MAJOR_VERSION}::Gui)

    endif()
    add_executable( testclient testclient.cpp )
    target_link_libraries( testclient ${testclient_LIBRARIES})
endif()

if(BUILD_QTQUICK_DEMO)
    add_subdirectory(TestAppExample)
endif()

include(CTest)
if(BUILD_TESTING AND NOT ANDROID AND NOT IOS)
    add_subdirectory(autotest)
endif()



###
### CMake config file
###

configure_package_config_file("${CMAKE_CURRENT_SOURCE_DIR}/QtKeychainConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/Qt${QTKEYCHAIN_VERSION_INFIX}KeychainConfig.cmake"
  INSTALL_DESTINATION Qt${QTKEYCHAIN_VERSION_INFIX}Keychain)

ecm_setup_version("${QTKEYCHAIN_VERSION}" VARIABLE_PREFIX SNORE
  PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/Qt${QTKEYCHAIN_VERSION_INFIX}KeychainConfigVersion.cmake"
  SOVERSION ${QTKEYCHAIN_VERSION})

if(UNIX AND NOT APPLE AND NOT ANDROID AND NOT HAIKU)
    set(PRI_EXTRA_DEPS "dbus")
endif()
ecm_generate_pri_file(BASE_NAME Qt${QTKEYCHAIN_VERSION_INFIX}Keychain
  LIB_NAME ${QTKEYCHAIN_TARGET_NAME}
  DEPS "core ${PRI_EXTRA_DEPS}"
  INCLUDE_INSTALL_DIR ${CMAKE_INSTALL_INCLUDEDIR}
  FILENAME_VAR pri_filename)

install(FILES ${pri_filename} DESTINATION ${ECM_MKSPECS_INSTALL_DIR})


install(EXPORT Qt${QTKEYCHAIN_VERSION_INFIX}KeychainLibraryDepends
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/Qt${QTKEYCHAIN_VERSION_INFIX}Keychain"
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/Qt${QTKEYCHAIN_VERSION_INFIX}KeychainConfig.cmake
              ${CMAKE_CURRENT_BINARY_DIR}/Qt${QTKEYCHAIN_VERSION_INFIX}KeychainConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Qt${QTKEYCHAIN_VERSION_INFIX}Keychain
)

