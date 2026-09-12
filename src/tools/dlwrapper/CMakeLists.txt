
add_qtc_executable(dlwrapper
  DEPENDS Qt::Core Qt::Network QtTaskTree Utils
  SOURCES
    dlwrapper.cpp
)

if (WIN32)
  extend_qtc_executable(dlwrapper
    DEFINES _UNICODE UNICODE _CRT_SECURE_NO_WARNINGS
  )
endif()

qtc_add_version_resource(dlwrapper "${IDE_DISPLAY_NAME} Download Wrapper")
