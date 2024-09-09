set(IDE_VERSION "14.0.82")                            # The IDE version.
set(IDE_VERSION_COMPAT "14.0.82")                     # The IDE Compatibility version.
set(IDE_VERSION_DISPLAY "15.0.0-beta1")               # The IDE display version.

set(IDE_SETTINGSVARIANT "QtProject")                  # The IDE settings variation.
set(IDE_DISPLAY_NAME "Qt Creator")                    # The IDE display name.
set(IDE_ID "qtcreator")                               # The IDE id (no spaces, lowercase!)
set(IDE_CASED_ID "QtCreator")                         # The cased IDE id (no spaces!)
set(IDE_BUNDLE_IDENTIFIER "org.qt-project.${IDE_ID}") # The macOS application bundle identifier.
set(IDE_APP_ID "org.qt-project.${IDE_ID}")            # The free desktop application identifier.
set(IDE_AUTHOR "The Qt Company Ltd. and other contributors.")
set(IDE_COPYRIGHT "Copyright (C) ${IDE_AUTHOR}")

set(PROJECT_USER_FILE_EXTENSION .user)
set(IDE_DOC_FILE "qtcreator/qtcreator.qdocconf")
set(IDE_DOC_FILE_ONLINE "qtcreator/qtcreator-online.qdocconf")

# Absolute, or relative to <qtcreator>/src/app
# Should contain qtcreator.ico, qtcreator.xcassets
set(IDE_ICON_PATH "")
# Absolute, or relative to <qtcreator>/src/app
# Should contain images/logo/(16|24|32|48|64|128|256|512)/QtProject-${IDE_ID}.png
set(IDE_LOGO_PATH "")
