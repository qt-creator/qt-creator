TEMPLATE = app

LIBS += -lbbdata -lbb -lbbcascades
QT += declarative xml

SOURCES += \
    src/main.cpp \
    src/applicationui.cpp \

HEADERS += \
    src/applicationui.h \

OTHER_FILES += \
    bar-descriptor.xml \
    assets/main.qml \


