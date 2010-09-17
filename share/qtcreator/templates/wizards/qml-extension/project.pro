TEMPLATE = lib
TARGET  = %ProjectName%
QT += declarative
CONFIG += qt plugin

TARGET = $$qtLibraryTarget($$TARGET)

# Input
SOURCES += \
    %ProjectName:l%.%CppSourceSuffix% \
    %ObjectName:l%.%CppSourceSuffix%

OTHER_FILES=qmldir

HEADERS += \
    %ProjectName:l%.%CppHeaderSuffix% \
    %ObjectName:l%.%CppHeaderSuffix%
