if(NOT DEFINED QTKEYCHAIN_TARGET_NAME)
	cmake_minimum_required(VERSION 3.16)
	project(TestAppExample LANGUAGES CXX)
endif()

include(GNUInstallDirs)

# Enable C++11
SET(CMAKE_CXX_STANDARD 11)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(BUNDLE_IDENTIFIER "org.qtkeychain.TestAppExample")

option(BUILD_WITH_QT5 "Build qtkeychain with Qt 5 (default is Qt 6)" OFF)
if (BUILD_WITH_QT5)
	set(QTx Qt5)
else()
	set(QTx Qt6)
endif()

if(DEFINED QTKEYCHAIN_TARGET_NAME)
	# Included as subdirectory from the top-level build (BUILD_QTQUICK_DEMO=ON)
	set(KEYCHAIN_TARGET ${QTKEYCHAIN_TARGET_NAME})
else()
	# Standalone build: pull in the library from source (one level up)
	get_filename_component(_qtkeychain_source_dir "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
	if(EXISTS "${_qtkeychain_source_dir}/qtkeychain/keychain.h")
		add_subdirectory("${_qtkeychain_source_dir}" "${CMAKE_CURRENT_BINARY_DIR}/qtkeychain_build")
		# QTKEYCHAIN_TARGET_NAME is set inside the subdirectory scope, derive it here
		if(BUILD_WITH_QT5)
			set(KEYCHAIN_TARGET qt5keychain)
		else()
			set(KEYCHAIN_TARGET qt6keychain)
		endif()
	else()
		# Fallback: find an already-installed QtKeychain
		find_package(${QTx}Keychain REQUIRED)
		set(KEYCHAIN_TARGET ${QTx}Keychain::${QTx}Keychain)
	endif()
endif()

find_package(${QTx} COMPONENTS Core Network Quick Qml REQUIRED)

qt_add_resources(QT_RESOURCES qml.qrc)

if(NOT BUILD_WITH_QT5)
	qt_add_executable(TestAppExample
		MANUAL_FINALIZATION
		keychainclass.h
		keychainclass.cpp
		main.cpp
		${QT_RESOURCES}
	)
else()
	add_executable(TestAppExample
		keychainclass.h
		keychainclass.cpp
		main.cpp
		${QT_RESOURCES}
	)
endif()

set_target_properties(TestAppExample PROPERTIES
	MACOSX_BUNDLE_GUI_IDENTIFIER ${BUNDLE_IDENTIFIER}
	MACOSX_BUNDLE_IDENTIFIER ${BUNDLE_IDENTIFIER}
	MACOSX_BUNDLE_BUNDLE_NAME TestAppExample
	MACOSX_BUNDLE TRUE
)

target_link_libraries(TestAppExample PRIVATE ${QTx}::Core ${QTx}::Network ${QTx}::Quick ${QTx}::Qml ${KEYCHAIN_TARGET})

if(NOT BUILD_WITH_QT5)
	qt_import_qml_plugins(TestAppExample)
	qt_finalize_target(TestAppExample)
endif()

install(TARGETS TestAppExample
	BUNDLE DESTINATION .
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
