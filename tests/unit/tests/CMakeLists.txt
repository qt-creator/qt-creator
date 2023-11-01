find_package(Qt6 COMPONENTS QmlDomPrivate QmlCompilerPrivate QUIET)

if (MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj")
elseif (MINGW)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wa,-mbig-obj")
endif()

set(QtCreatorLibsDir "${QtCreator_SOURCE_DIR}/src/libs")
set(QtCreatorPluginsDir "${QtCreator_SOURCE_DIR}/src/plugins")
set(QtCreatorResourcesDir "${QtCreator_SOURCE_DIR}/share/qtcreator")
set(QmlDesignerDir "${QtCreatorPluginsDir}/qmldesigner")

set(UnittestStubsDir "${CMAKE_CURRENT_SOURCE_DIR}/stubs")
set(UnittestUtilsDir "${CMAKE_CURRENT_SOURCE_DIR}/utils")
set(UnittestPrintersDir "${CMAKE_CURRENT_SOURCE_DIR}/printers")

add_subdirectory(testdesignercore)
add_subdirectory(utils)
add_subdirectory(mocks)
add_subdirectory(printers)
add_subdirectory(matchers)
add_subdirectory(unittests)
