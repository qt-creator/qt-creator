TEMPLATE = app

LIBS += -lbbdata -lbb -lbbcascades
QT += declarative xml

SOURCES += \
    src/main.%CppSourceSuffix% \
    src/applicationui.%CppSourceSuffix% \

HEADERS += \
    src/applicationui.%CppHeaderSuffix% \

OTHER_FILES += \
    bar-descriptor.xml \
    assets/main.qml \


