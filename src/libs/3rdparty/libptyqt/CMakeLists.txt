set(SOURCES
    iptyprocess.h
    ptyqt.cpp ptyqt.h
)

if (WIN32)
    list(APPEND SOURCES
        winptyprocess.cpp winptyprocess.h
        conptyprocess.cpp conptyprocess.h
    )
else()
    list(APPEND SOURCES unixptyprocess.cpp unixptyprocess.h)
endif()

add_library(ptyqt STATIC ${SOURCES})
target_link_libraries(ptyqt PUBLIC Qt::Core)

if (WIN32)
  target_link_libraries(ptyqt PRIVATE winpty Qt::Network)
  #target_compile_definitions(ptyqt PRIVATE PTYQT_DEBUG)
endif()

set_target_properties(ptyqt
  PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}
    QT_COMPILE_OPTIONS_DISABLE_WARNINGS ON
    POSITION_INDEPENDENT_CODE ON
)
