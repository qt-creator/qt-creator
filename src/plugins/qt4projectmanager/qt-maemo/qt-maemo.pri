SUPPORT_QT_MAEMO = $$(QTCREATOR_WITH_MAEMO)
!isEmpty(SUPPORT_QT_MAEMO) { 
    message("Adding experimental support for Qt/Maemo applications.")
    DEFINES += QTCREATOR_WITH_MAEMO
    HEADERS += $$PWD/maemorunconfiguration.h \
        $$PWD/maemomanager.h \
        $$PWD/maemotoolchain.h \
        $$PWD/maemodeviceconfigurations.h \
        $$PWD/maemosettingspage.h
    SOURCES += $$PWD/maemorunconfiguration.cpp \
        $$PWD/maemomanager.cpp \
        $$PWD/maemotoolchain.cpp \
        $$PWD/maemodeviceconfigurations.cpp \
        $$PWD/maemosettingspage.cpp
     FORMS += $$PWD/maemosettingswidget.ui

    RESOURCES += $$PWD/qt-maemo.qrc
}
