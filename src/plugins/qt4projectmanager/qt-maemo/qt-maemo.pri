SUPPORT_QT_MAEMO = $$(QTCREATOR_WITH_MAEMO)
!isEmpty(SUPPORT_QT_MAEMO) {
    message("Adding experimental support for Qt/Maemo applications.")
    DEFINES += QTCREATOR_WITH_MAEMO
    HEADERS += \
        $$PWD/maemorunconfiguration.h \
        $$PWD/maemomanager.h \
        $$PWD/maemotoolchain.h
    SOURCES += \
        $$PWD/maemorunconfiguration.cpp \
        $$PWD/maemomanager.cpp \
        $$PWD/maemotoolchain.cpp
    RESOURCES += $$PWD/qt-maemo.qrc
}
