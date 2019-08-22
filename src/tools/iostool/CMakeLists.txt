add_qtc_executable(iostool
  DESTINATION ${IDE_LIBEXEC_PATH}/ios
  DEPENDS
    Qt5::Gui
    Qt5::Xml
    Qt5::Network
    ${FWCoreFoundation}
    ${FWCoreServices}
    ${FWIOKit}
    ${FWSecurity}
    ${FWSystemConfiguration}
  SOURCES
    Info.plist
    main.cpp
    iosdevicemanager.cpp iosdevicemanager.h
)

if (TARGET iostool)
  if (CMAKE_VERSION VERSION_LESS 3.13)
    target_link_libraries(iostool PRIVATE "-Wl,-sectcreate,__TEXT,__info_plist,${CMAKE_CURRENT_SOURCE_DIR}/Info.plist")
  else()
    target_link_options(iostool PRIVATE "-Wl,-sectcreate,__TEXT,__info_plist,${CMAKE_CURRENT_SOURCE_DIR}/Info.plist")
  endif()
endif()
