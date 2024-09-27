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

if(NOT ${QT_INSTALLATION_DIR} STREQUAL "")
message(STATUS "Copying patched qml files to ${QT_INSTALLATION_DIR}")
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/3dfirst/QtQuick3D/ DESTINATION ${QT_INSTALLATION_DIR}/qml/QtQuick3D/)
endif()
