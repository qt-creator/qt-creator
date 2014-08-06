TEMPLATE = app

LIBS += -lbbdata -lbb -lbbcascades
QT += declarative xml

SOURCES += \
    src/main.%CppSourceSuffix% \
    src/applicationui.%CppSourceSuffix% \

HEADERS += \
    src/applicationui.%CppHeaderSuffix% \

DISTFILES += \
    bar-descriptor.xml \
    assets/main.qml \


