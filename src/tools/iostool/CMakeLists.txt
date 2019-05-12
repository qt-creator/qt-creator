add_qtc_executable(iostool
  DEPENDS Qt5::Network Qt5::Widgets Qt5::Xml
  SOURCES
    Info.plist
    main.cpp
    iosdevicemanager.cpp iosdevicemanager.h
)
