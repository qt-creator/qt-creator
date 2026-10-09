
list(APPEND qtkeychain_LIBRARIES ${QTCORE_LIBRARIES})
set(qtkeychain_SOURCES
    keychain.cpp
    qkeychain_export.h
    keychain.h
)

if(MSVC)
    # CMake < 3.15 sneaks in /W# flags for us, so we need a replacement,
    # or we'll get a warning (cf. CMP0092)
    if (CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
        string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    endif()
else()
    # MSVC's STL / Qt headers are not MSVC -Wall clean, so don't enable it there
    add_definitions( -Wall -Werror=return-type )
endif()

if(WIN32)
    list(APPEND qtkeychain_SOURCES keychain_win.cpp)
    list(APPEND qtkeychain_LIBRARIES crypt32)
    if (NOT USE_CREDENTIAL_STORE)
        list(APPEND qtkeychain_SOURCES plaintextstore.cpp)
    endif()
    #FIXME: mingw bug; otherwise getting undefined refs to RtlSecureZeroMemory there
    if(MINGW)
        add_definitions( -O2 )
    endif()

    set(CMAKE_CXX_STANDARD 17)
    add_definitions(-DUNICODE)
    if (MSVC)
        add_definitions(/utf-8)
    elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        add_definitions(-finput-charset=UTF-8 -fexec-charset=UTF-8)
    endif()
endif()

if(APPLE)
    list(APPEND qtkeychain_SOURCES keychain_apple.mm)
    list(APPEND qtkeychain_LIBRARIES "-framework Foundation" "-framework Security")
endif()

if(HAIKU)
    list(APPEND qtkeychain_SOURCES keychain_haiku.cpp)

    find_library(BE_LIBRARY be REQUIRED)
    list(APPEND qtkeychain_LIBRARIES ${BE_LIBRARY})
endif()

if(UNIX AND NOT APPLE AND NOT ANDROID AND NOT HAIKU AND NOT EMSCRIPTEN)
    option(LIBSECRET_SUPPORT "Build with libsecret support" ON)

    if(LIBSECRET_SUPPORT)
        pkg_check_modules(LIBSECRET REQUIRED libsecret-1)
        add_definitions(-DHAVE_LIBSECRET=1)
        INCLUDE_DIRECTORIES(${LIBSECRET_INCLUDE_DIRS})
        LINK_DIRECTORIES(${LIBSECRET_LIBRARY_DIRS})
        list(APPEND qtkeychain_LIBRARIES_PRIVATE ${LIBSECRET_LIBRARIES})
    endif()

    add_definitions(-DKEYCHAIN_DBUS=1)
    list(APPEND qtkeychain_SOURCES keychain_unix.cpp gnomekeyring.cpp libsecret.cpp plaintextstore.cpp)
    qt_add_dbus_interface(qtkeychain_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/org.kde.KWallet.xml kwallet_interface KWalletInterface)
    list(APPEND qtkeychain_LIBRARIES ${QTDBUS_LIBRARIES} )
endif()

if(ANDROID)
    list(APPEND qtkeychain_SOURCES keychain_android.cpp androidkeystore.cpp plaintextstore.cpp)
    list(APPEND qtkeychain_LIBRARIES_PRIVATE ${QTANDROIDEXTRAS_LIBRARIES} )
endif()

if(EMSCRIPTEN)
    list(APPEND qtkeychain_SOURCES keychain_wasm.cpp)
endif()

QT_WRAP_CPP(qtkeychain_MOC_OUTFILES keychain.h keychain_p.h gnomekeyring_p.h)

add_library(${QTKEYCHAIN_TARGET_NAME} ${qtkeychain_SOURCES} ${qtkeychain_MOC_OUTFILES} ${qtkeychain_QM_FILES})

if(EMSCRIPTEN)
    # Qt's Wasm support uses Embind for C++/JS interaction.
    target_link_options(${QTKEYCHAIN_TARGET_NAME} INTERFACE "--bind")
endif()

if(WIN32)
    set_target_properties( ${QTKEYCHAIN_TARGET_NAME} PROPERTIES DEBUG_POSTFIX "d" )
endif()

target_link_libraries(${QTKEYCHAIN_TARGET_NAME} PUBLIC ${qtkeychain_LIBRARIES} PRIVATE ${qtkeychain_LIBRARIES_PRIVATE})
if(NOT INTERFACE_INCLUDE_SUFFIX)
    set(INTERFACE_INCLUDE_SUFFIX include)
endif()
# Where to find includes when building the library and using it uninstalled: in the parent dir, so that the code has to use <qtkeychain/keychain.h>
target_include_directories(${QTKEYCHAIN_TARGET_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..> $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>)
# Where to find includes when using the installed qtkeychain
target_include_directories(${QTKEYCHAIN_TARGET_NAME} INTERFACE $<INSTALL_INTERFACE:${INTERFACE_INCLUDE_SUFFIX}>)

generate_export_header(${QTKEYCHAIN_TARGET_NAME}
  EXPORT_FILE_NAME qkeychain_export.h
  EXPORT_MACRO_NAME QKEYCHAIN_EXPORT
)

set_target_properties(${QTKEYCHAIN_TARGET_NAME} PROPERTIES
    VERSION ${QTKEYCHAIN_VERSION}
    SOVERSION ${QTKEYCHAIN_SOVERSION}
    INSTALL_RPATH_USE_LINK_PATH TRUE
)

if (NOT APPLE)
	set_target_properties(${QTKEYCHAIN_TARGET_NAME} PROPERTIES
		INSTALL_NAME_DIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}"
		)
endif()

install(FILES keychain.h ${CMAKE_CURRENT_BINARY_DIR}/qkeychain_export.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/qt${QTKEYCHAIN_VERSION_INFIX}keychain/
)

install(TARGETS ${QTKEYCHAIN_TARGET_NAME}
  EXPORT Qt${QTKEYCHAIN_VERSION_INFIX}KeychainLibraryDepends
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
