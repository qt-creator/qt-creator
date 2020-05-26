add_subdirectory(3rdparty)
add_subdirectory(buildoutputparser)
if (NOT Clang_FOUND)
  message(WARNING "Could not find Clang installation - disabling clangbackend.")
else ()
  add_subdirectory(clangbackend)
  add_subdirectory(clangrefactoringbackend)
  add_subdirectory(clangpchmanagerbackend)
endif ()

option(BUILD_CPLUSPLUS_TOOLS "Build CPlusPlus tools" OFF)

function(add_qtc_cpp_tool name)
  add_qtc_executable(${name}
    DEFINES
      PATH_PREPROCESSOR_CONFIG=\"${CMAKE_CURRENT_SOURCE_DIR}/cplusplus-shared/pp-configuration.inc\"
      ${ARGN}
    DEPENDS CPlusPlus Utils Qt5::Widgets
    INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/cplusplus-shared"
      "${CMAKE_CURRENT_SOURCE_DIR}/${name}"
    SOURCES
      cplusplus-shared/utils.cpp
      "${name}/${name}.cpp"
  )
endfunction()

if (BUILD_CPLUSPLUS_TOOLS)
  add_qtc_cpp_tool(cplusplus-ast2png "")
  add_qtc_cpp_tool(cplusplus-frontend "")
  add_qtc_cpp_tool(cplusplus-mkvisitor PATH_AST_H=\"${CMAKE_CURRENT_SOURCE_DIR}/../libs/3rdparty/cplusplus/AST.h\")
  add_qtc_cpp_tool(cplusplus-update-frontend PATH_CPP_FRONTEND=\"${CMAKE_CURRENT_SOURCE_DIR}/../libs/3rdparty/cplusplus\" PATH_DUMPERS_FILE=\"${CMAKE_CURRENT_SOURCE_DIR}/cplusplus-ast2png/dumpers.inc\")
endif()

if (APPLE)
  add_subdirectory(iostool)
endif()

add_subdirectory(qml2puppet)
add_subdirectory(qtcdebugger) ## windows only
# add_subdirectory(qtcrashhandler)
add_subdirectory(qtc-askpass)
add_subdirectory(qtcreatorcrashhandler)
# add_subdirectory(qtcreatorwidgets) ## qbs does not build this either
add_subdirectory(qtpromaker)
# add_subdirectory(screenshotcropper) ## qbs does not build this either
add_subdirectory(sdktool)
add_subdirectory(valgrindfake)
add_subdirectory(wininterrupt) ## windows only
add_subdirectory(winrtdebughelper) ## windows only

if (EXISTS ${CMAKE_CURRENT_LIST_DIR}/perfparser/CMakeLists.txt)
  add_subdirectory(perfparser)
endif()
