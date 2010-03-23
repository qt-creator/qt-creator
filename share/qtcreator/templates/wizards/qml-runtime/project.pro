TEMPLATE = lib
TARGET  = %ProjectName%
QT += declarative
CONFIG += qt plugin

TARGET = $$qtLibraryTarget($$TARGET)
DESTDIR = %ProjectName%

# Input
SOURCES += \
    %ProjectName%.cpp \
    %ObjectName%.cpp

OTHER_FILES=%ProjectName%/qmldir

HEADERS += \
    %ProjectName%.h \
    %ObjectName%.h
