contains(CONFIG, dll) {
    DEFINES += QML_BUILD_LIB
} else {
    DEFINES += QML_BUILD_STATIC_LIB
}

INCLUDEPATH += $$PWD

include($$PWD/../../shared/qml/qml.pri)

##contains(QT, gui) {
##HEADERS += \
##    $$PWD/Nothing.h
##
##SOURCES += \
##    $$PWD/Nothing.cpp
##}

#HEADERS += \
#    $$PWD/qmlsymbol.h

#SOURCES += \
#    $$PWD/qmlsymbol.cpp

