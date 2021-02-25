add_subdirectory(qtcreator)

if (NOT APPLE AND NOT WIN32)
  install(
    DIRECTORY
      applications
      metainfo
    DESTINATION
      ${CMAKE_INSTALL_DATAROOTDIR}
  )
endif()
