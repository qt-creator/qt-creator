QTC_PLUGIN_DEPENDS += coreplugin
QTC_LIB_DEPENDS += utils
include(../../qtcreatortool.pri)

SOURCES += \
    main.cpp\
    screenshotcropperwindow.cpp \
    cropimageview.cpp \
    ../../plugins/qtsupport/screenshotcropper.cpp

HEADERS += \
    screenshotcropperwindow.h \
    cropimageview.h \
    ../../plugins/qtsupport/screenshotcropper.h

INCLUDEPATH += \
    .

FORMS += \
    screenshotcropperwindow.ui
