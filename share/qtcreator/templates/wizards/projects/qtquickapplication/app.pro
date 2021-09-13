@if "%{UseVirtualKeyboard}" == "true"
QT += quick virtualkeyboard
@else
QT += quick
@endif
@if !%{IsQt6}

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0
@endif

SOURCES += \\
        %{MainCppFileName}

@if %{IsQt6}
resources.files = main.qml %{AdditionalQmlFiles}
resources.prefix = /$${TARGET}
RESOURCES += resources
@else
RESOURCES += qml.qrc
@endif
@if %{HasTranslation}

TRANSLATIONS += \\
    %{TsFileName}
CONFIG += lrelease
CONFIG += embed_translations
@endif

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Additional import path used to resolve QML modules just for Qt Quick Designer
QML_DESIGNER_IMPORT_PATH =

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
