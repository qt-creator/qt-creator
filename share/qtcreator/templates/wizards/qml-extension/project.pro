TEMPLATE = lib
TARGET = %ProjectName%
QT += declarative
CONFIG += qt plugin

TARGET = $$qtLibraryTarget($$TARGET)

# Input
SOURCES += \
    %ProjectName:l%_plugin.%CppSourceSuffix% \
    %ObjectName:l%.%CppSourceSuffix%

HEADERS += \
    %ProjectName:l%_plugin.%CppHeaderSuffix% \
    %ObjectName:l%.%CppHeaderSuffix%

OTHER_FILES = qmldir
