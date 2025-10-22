set(IDE_VERSION "4.8.1")                              # The IDE version.
set(IDE_VERSION_COMPAT "4.8.1")                       # The IDE Compatibility version.
set(IDE_VERSION_DISPLAY "4.8")                        # The IDE display version.

set(IDE_SETTINGSVARIANT "QtProject")                  # The IDE settings variation.
set(IDE_COPY_SETTINGSVARIANT "Nokia")                 # The IDE settings to initially import.
set(IDE_DISPLAY_NAME "Qt Design Studio")              # The IDE display name.
set(IDE_ID "qtdesignstudio")                          # The IDE id (no spaces, lowercase!)
set(IDE_CASED_ID "QtDesignStudio")                    # The cased IDE id (no spaces!)
set(IDE_BUNDLE_IDENTIFIER "org.qt-project.${IDE_ID}") # The macOS application bundle identifier.
set(IDE_APP_ID "io.qt.${IDE_ID}")                     # The free desktop application identifier.
set(IDE_PUBLISHER "The Qt Company Ltd.")
set(IDE_AUTHOR "${IDE_PUBLISHER} and other contributors.")
set(IDE_COPYRIGHT "Copyright (C) ${IDE_AUTHOR}")

set(PROJECT_USER_FILE_EXTENSION .qtds)
set(IDE_DOC_FILE "qtdesignstudio/qtdesignstudio.qdocconf")
set(IDE_DOC_FILE_ONLINE "qtdesignstudio/qtdesignstudio-online.qdocconf")

set(IDE_ICON_PATH "${CMAKE_CURRENT_LIST_DIR}")
set(IDE_LOGO_PATH "${CMAKE_CURRENT_LIST_DIR}")

set(DESIGNSTUDIO_PLUGINS
    Android
    BareMetal
    Boot2Qt
    CMakeProjectManager
    CodePaster
    Core
    Copilot
    CppEditor
    Debugger
    Designer
    DiffEditor
    EffectComposer
    FakeVim
    Git
    Help
    ImageViewer
    Insight
    LanguageClient
    McuSupport
    MultiPropertyEditor
    ProjectExplorer
    QmakeProjectManager
    QmlDesigner
    QmlDesignerAi
    QmlDesignerBase
    QmlJSEditor
    QmlJSTools
    QmlPreview
    QmlProjectManager
    QtSupport
    RemoteLinux
    ResourceEditor
    ScxmlEditor
    StudioPlugin
    StudioWelcome
    Texteditor
    UpdateInfo
    VcsBase
    componentsplugin
    qmlpreviewplugin
    qtquickplugin)

if(DESIGNSTUDIO_EXTRAPLUGINS)
  list(APPEND DESIGNSTUDIO_PLUGINS ${DESIGNSTUDIO_EXTRAPLUGINS})
endif()

if(NOT BUILD_PLUGINS)
    set(BUILD_PLUGINS "${DESIGNSTUDIO_PLUGINS}" CACHE STRING "Build plugins" FORCE)
endif()
