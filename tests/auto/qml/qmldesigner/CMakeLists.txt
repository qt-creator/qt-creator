if (${Qt5_VERSION} VERSION_GREATER_EQUAL "6.2.0")
  add_subdirectory(coretests)
  add_subdirectory(wizard)
endif()
