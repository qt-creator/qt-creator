
add_qtc_executable(qtcreator_process_stub
  DEPENDS Qt::Core Qt::Network
  SOURCES
    main.cpp
)

if (WIN32)
  extend_qtc_executable(qtcreator_process_stub
    DEFINES _UNICODE UNICODE _CRT_SECURE_NO_WARNINGS
  )
endif()

qtc_add_version_resource(qtcreator_process_stub "${IDE_DISPLAY_NAME} Process Stub")
