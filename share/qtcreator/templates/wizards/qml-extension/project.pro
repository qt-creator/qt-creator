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

copy_qmldir.target = $$OUT_PWD/qmldir
copy_qmldir.depends = $$PWD/qmldir
copy_qmldir.commands = $(COPY_FILE) $$copy_qmldir.depends $$copy_qmldir.target
QMAKE_EXTRA_TARGETS += copy_qmldir
PRE_TARGETDEPS += $$copy_qmldir.target
