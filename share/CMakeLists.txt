add_subdirectory(qtcreator)

if (NOT APPLE AND NOT WIN32)
  set(DATE_ATTRIBUTE)
  if(SHOW_BUILD_DATE)
    string(TIMESTAMP timestamp "%Y-%m-%d")
    set(DATE_ATTRIBUTE " date=\"${timestamp}\"")
  endif()
  configure_file(metainfo/org.qt-project.qtcreator.appdata.xml.cmakein metainfo/org.qt-project.qtcreator.appdata.xml)

  install(
    DIRECTORY
      applications
    DESTINATION
      ${CMAKE_INSTALL_DATAROOTDIR}
  )
  install(
    FILES
      ${CMAKE_CURRENT_BINARY_DIR}/metainfo/org.qt-project.qtcreator.appdata.xml
    DESTINATION
      ${CMAKE_INSTALL_DATAROOTDIR}/metainfo/
  )
endif()
