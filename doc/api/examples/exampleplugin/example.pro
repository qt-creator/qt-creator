#! [1]
TARGET = Example
TEMPLATE = lib

DEFINES += EXAMPLE_LIBRARY
#! [1]

# Example files

#! [2]
SOURCES += exampleplugin.cpp

HEADERS += exampleplugin.h\
        example_global.h\
        exampleconstants.h
#! [2]

# Qt Creator linking

#! [3]
## set the QTC_SOURCE environment variable to override the setting here
QTCREATOR_SOURCES = $$(QTC_SOURCE)
isEmpty(QTCREATOR_SOURCES):QTCREATOR_SOURCES=/Users/example/qtcreator-src

## set the QTC_BUILD environment variable to override the setting here
IDE_BUILD_TREE = $$(QTC_BUILD)
isEmpty(IDE_BUILD_TREE):IDE_BUILD_TREE=/Users/example/qtcreator-build
#! [3]

#! [4]
## uncomment to build plugin into user config directory
## <localappdata>/plugins/<ideversion>
##    where <localappdata> is e.g.
##    "%LOCALAPPDATA%\QtProject\qtcreator" on Windows Vista and later
##    "$XDG_DATA_HOME/data/QtProject/qtcreator" or "~/.local/share/data/QtProject/qtcreator" on Linux
##    "~/Library/Application Support/QtProject/Qt Creator" on Mac
# USE_USER_DESTDIR = yes
#! [4]

#![5]
PROVIDER = MyCompany
#![5]

#![6]
include($$QTCREATOR_SOURCES/src/qtcreatorplugin.pri)
include($$QTCREATOR_SOURCES/src/plugins/coreplugin/coreplugin.pri)

LIBS += -L$$IDE_PLUGIN_PATH/QtProject
#![6]

