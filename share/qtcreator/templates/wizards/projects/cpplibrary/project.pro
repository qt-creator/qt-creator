@if '%{QtModule}' === 'none'
CONFIG -= qt
@elsif '%{QtModule}' === 'core'
QT -= gui
@else
QT += %{QtModule}
@endif

TEMPLATE = lib
@if %{IsStatic}
CONFIG += staticlib
@elsif %{IsQtPlugin}
CONFIG += plugin
@elsif %{IsShared}
DEFINES += %{LibraryDefine}
@endif

CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \\
    %{SrcFileName}

HEADERS += \\
@if %{IsShared}
    %{GlobalHdrFileName} \\
@endif
    %{HdrFileName}
@if %{HasTranslation}

TRANSLATIONS += \\
    %{TsFileName}
@endif
@if %{IsQtPlugin}

DISTFILES += %{PluginJsonFile}
@endif

@if '%{TargetInstallPath}' != ''
# Default rules for deployment.
unix {
    target.path = %{TargetInstallPath}
}
!isEmpty(target.path): INSTALLS += target
@endif
