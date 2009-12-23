SUPPORT_QT_MAEMO = $$(QTCREATOR_WITH_MAEMO)
!isEmpty(SUPPORT_QT_MAEMO) { 
    message("Adding experimental support for Qt/Maemo applications.")
    DEFINES += QTCREATOR_WITH_MAEMO
#    DEFINES += USE_SSH_LIB
#    LIBS += -L/opt/ne7ssh/lib/ -lnet7ssh
    HEADERS += $$PWD/maemorunconfiguration.h \
        $$PWD/maemomanager.h \
        $$PWD/maemotoolchain.h \
        $$PWD/maemodeviceconfigurations.h \
        $$PWD/maemosettingspage.h \
        $$PWD/maemosshconnection.h \
        $$PWD/maemosshthread.h
    SOURCES += $$PWD/maemorunconfiguration.cpp \
        $$PWD/maemomanager.cpp \
        $$PWD/maemotoolchain.cpp \
        $$PWD/maemodeviceconfigurations.cpp \
        $$PWD/maemosettingspage.cpp \
        $$PWD/maemosshconnection.cpp \
        $$PWD/maemosshthread.cpp
    FORMS += $$PWD/maemosettingswidget.ui
    RESOURCES += $$PWD/qt-maemo.qrc
}
