TEMPLATE = app

# Please do not modify the following line.
include(qtquick2applicationviewer/qtquick2applicationviewer.pri)

QT += qml quick

# Additional import path used to resolve QML modules in Creator's code model
QML_IMPORT_PATH =

# The .cpp file which was generated for your project. Feel free to hack it.
SOURCES += main.cpp

OTHER_FILES += bar-descriptor.xml \
               qml/main.qml
