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

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
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
