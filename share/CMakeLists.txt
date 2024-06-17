add_subdirectory(qtcreator)

if (NOT APPLE AND NOT WIN32)
  set(DATE_ATTRIBUTE)
  if(SHOW_BUILD_DATE)
    string(TIMESTAMP timestamp "%Y-%m-%d")
    set(DATE_ATTRIBUTE " date=\"${timestamp}\"")
  endif()
  configure_file(metainfo/${IDE_APP_ID}.appdata.xml.cmakein metainfo/${IDE_APP_ID}.appdata.xml)

  install(
    FILES
      applications/${IDE_APP_ID}.desktop
    DESTINATION
      ${CMAKE_INSTALL_DATAROOTDIR}/applications/
  )
  install(
    FILES
      ${CMAKE_CURRENT_BINARY_DIR}/metainfo/${IDE_APP_ID}.appdata.xml
    DESTINATION
      ${CMAKE_INSTALL_DATAROOTDIR}/metainfo/
  )
endif()
