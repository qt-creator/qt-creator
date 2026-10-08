
add_qtc_plugin(Terminal
  PLUGIN_DEPENDS Core ProjectExplorer
  DEPENDS TerminalLib
  SOURCES
    consolehost.cpp consolehost.h
    shellintegration.cpp shellintegration.h
    shellmodel.cpp shellmodel.h
    shortcutmap.cpp shortcutmap.h
    terminalconstants.h
    terminalicons.h
    terminalpane.cpp terminalpane.h
    terminalplugin.cpp
    terminalprocessimpl.cpp terminalprocessimpl.h
    terminalsettings.cpp terminalsettings.h
    terminaltr.h
    terminalwidget.cpp terminalwidget.h
  SBOM_ARGS
    USE_ATTRIBUTION_FILES
    CREATE_SBOM_FOR_EACH_ATTRIBUTION
    ATTRIBUTION_FILE_PATHS
      "${QtCreator_SOURCE_DIR}/qt_attributions.json"
    ATTRIBUTION_IDS
      terminal-shellintegrations
      terminal-shellintegrations-clink
)

qtc_add_resources(Terminal "images"
  PREFIX "/terminal"
  FILES
    images/keyboardlock.png
    images/keyboardlock@2x.png
    images/settingscategory_terminal.png
    images/settingscategory_terminal@2x.png
    images/terminal.png
    images/terminal@2x.png
    shellintegrations/shellintegration-bash.sh
    shellintegrations/shellintegration-env.zsh
    shellintegrations/shellintegration-login.zsh
    shellintegrations/shellintegration-profile.zsh
    shellintegrations/shellintegration-rc.zsh
    shellintegrations/shellintegration.fish
    shellintegrations/shellintegration.ps1
    shellintegrations/shellintegration-clink.lua
)
