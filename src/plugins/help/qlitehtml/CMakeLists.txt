if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/litehtml/CMakeLists.txt)
    set(ORIG_FPIC ${CMAKE_POSITION_INDEPENDENT_CODE})
    if (WIN32)
      set(LITEHTML_UTF8 ON CACHE BOOL "")
    endif()
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)

    # suppress warning about VERSION being overwritten or not
    # and add subdirectory
    set(ORIG_VERSION ${PROJECT_VERSION})
    set(ORIG_MAJOR ${PROJECT_VERSION_MAJOR})
    set(ORIG_MINOR ${PROJECT_VERSION_MINOR})
    set(ORIG_PATCH ${PROJECT_VERSION_PATCH})
    set(PROJECT_VERSION "")
    set(PROJECT_VERSION_MAJOR "")
    set(PROJECT_VERSION_MINOR "")
    set(PROJECT_VERSION_PATCH "")
    add_subdirectory(litehtml EXCLUDE_FROM_ALL)
    set(PROJECT_VERSION ${ORIG_VERSION})
    set(PROJECT_VERSION_MAJOR ${ORIG_MAJOR})
    set(PROJECT_VERSION_MINOR ${ORIG_MINOR})
    set(PROJECT_VERSION_PATCH ${ORIG_PATCH})

    set(CMAKE_POSITION_INDEPENDENT_CODE "${ORIG_FPIC}")
    # force optimized litehtml even in debug
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
      # except for windows
      if (NOT WIN32)
        target_compile_options(gumbo PRIVATE -O2)
        target_compile_options(litehtml PRIVATE -O2)
      endif()
    endif()
else()
    find_package(litehtml REQUIRED)
endif()

add_qtc_library(qlitehtml
  PUBLIC_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}
  DEPENDS Qt5::Widgets litehtml
  PROPERTIES
    POSITION_INDEPENDENT_CODE ON
  SOURCES
    container_qpainter.cpp container_qpainter.h container_qpainter_p.h
    qlitehtml_global.h
    qlitehtmlwidget.cpp qlitehtmlwidget.h
)
