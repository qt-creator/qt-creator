add_qtc_executable(screenshotcropper
  SKIP_INSTALL
  DEPENDS Qt5::Widgets Utils Core QtSupport
  SOURCES
    main.cpp
    cropimageview.cpp
    cropimageview.h
    screenshotcropperwindow.h screenshotcropperwindow.cpp screenshotcropperwindow.ui
)
