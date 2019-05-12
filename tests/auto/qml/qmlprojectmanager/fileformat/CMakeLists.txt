get_target_property(QmlProjectManagerSources QmlProjectManager SOURCES)
foreach(source IN LISTS QmlProjectManagerSources)
  if (source MATCHES "fileformat")
    list(APPEND fileformat_sources "${CMAKE_SOURCE_DIR}/src/plugins/qmlprojectmanager/${source}")
  endif()
endforeach()

add_qtc_test(tst_qml_fileformat
  DEPENDS qmljs Utils
  INCLUDES "${CMAKE_SOURCE_DIR}/src/plugins/qmlprojectmanager/fileformat"
  DEFINES
    QT_CREATOR
    SRCDIR="${CMAKE_CURRENT_SOURCE_DIR}"
  SOURCES tst_fileformat.cpp ${fileformat_sources}
)
