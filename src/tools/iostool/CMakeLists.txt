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
    gdbrunner.cpp gdbrunner.h
    iosdevicemanager.cpp iosdevicemanager.h
    iostool.cpp iostool.h
    iostooltypes.h
    main.cpp
    mobiledevicelib.cpp mobiledevicelib.h
    relayserver.cpp relayserver.h
)

if (TARGET iostool)
  target_link_options(iostool PRIVATE "-Wl,-sectcreate,__TEXT,__info_plist,${CMAKE_CURRENT_SOURCE_DIR}/Info.plist")
endif()
