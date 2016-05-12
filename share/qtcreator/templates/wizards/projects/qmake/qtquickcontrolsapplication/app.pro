TEMPLATE = app

@if %{UseQApplication}
QT += qml quick widgets
@else
QT += qml quick
@endif

CONFIG += c++11

SOURCES += %{MainCppFileName}

RESOURCES += qml.qrc

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
