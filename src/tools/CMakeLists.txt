add_subdirectory(3rdparty)
add_subdirectory(buildoutputparser)

function(add_qtc_cpp_tool name)
  add_qtc_executable(${name}
    SKIP_INSTALL
    DEFINES
      PATH_PREPROCESSOR_CONFIG=\"${CMAKE_CURRENT_SOURCE_DIR}/cplusplus-shared/pp-configuration.inc\"
      ${ARGN}
    DEPENDS CPlusPlus Utils Qt::Widgets
    INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/cplusplus-shared"
      "${CMAKE_CURRENT_SOURCE_DIR}/${name}"
    SOURCES
      cplusplus-shared/utils.cpp
      "${name}/${name}.cpp"
  )
endfunction()

add_qtc_cpp_tool(cplusplus-ast2png "")
add_qtc_cpp_tool(cplusplus-frontend "")
add_qtc_cpp_tool(cplusplus-mkvisitor PATH_AST_H=\"${CMAKE_CURRENT_SOURCE_DIR}/../libs/3rdparty/cplusplus/AST.h\")
add_qtc_cpp_tool(cplusplus-update-frontend PATH_CPP_FRONTEND=\"${CMAKE_CURRENT_SOURCE_DIR}/../libs/3rdparty/cplusplus\" PATH_DUMPERS_FILE=\"${CMAKE_CURRENT_SOURCE_DIR}/cplusplus-ast2png/dumpers.inc\")

if (APPLE)
  add_subdirectory(disclaim)
  add_subdirectory(iostool)
endif()

add_subdirectory(processlauncher)
if (WITH_QMLDESIGNER)
  add_subdirectory(qml2puppet)
endif()
add_subdirectory(qtcdebugger) ## windows only
# add_subdirectory(qtcrashhandler)
add_subdirectory(qtc-askpass)
add_subdirectory(qtcreatorcrashhandler)
# add_subdirectory(qtcreatorwidgets) ## qbs does not build this either
add_subdirectory(qtpromaker)
add_subdirectory(sdktool)
add_subdirectory(valgrindfake)
add_subdirectory(wininterrupt) ## windows only

if (EXISTS ${CMAKE_CURRENT_LIST_DIR}/perfparser/CMakeLists.txt)
  add_subdirectory(perfparser)
endif()

add_subdirectory(process_stub)
