depfile = $$replace(_PRO_FILE_PWD_, ([^/]+$), \\1/\\1_dependencies.pri)
exists($$depfile) {
    include($$depfile)
    isEmpty(QTC_PLUGIN_NAME): \
        error("$$basename(depfile) does not define QTC_PLUGIN_NAME.")
} else {
    isEmpty(QTC_PLUGIN_NAME): \
        error("QTC_PLUGIN_NAME is empty. Maybe you meant to create $$basename(depfile)?")
}
TARGET = $$QTC_PLUGIN_NAME

plugin_deps = $$QTC_PLUGIN_DEPENDS
plugin_recmds = $$QTC_PLUGIN_RECOMMENDS

include(../qtcreator.pri)

# for substitution in the .json
dependencyList =
for(dep, plugin_deps) {
    dependencies_file =
    for(dir, QTC_PLUGIN_DIRS) {
        exists($$dir/$$dep/$${dep}_dependencies.pri) {
            dependencies_file = $$dir/$$dep/$${dep}_dependencies.pri
            break()
        }
    }
    isEmpty(dependencies_file): \
        error("Plugin dependency $$dep not found")
    include($$dependencies_file)
    dependencyList += "        { \"Name\" : \"$$QTC_PLUGIN_NAME\", \"Version\" : \"$$QTCREATOR_VERSION\" }"
}
for(dep, plugin_recmds) {
    dependencies_file =
    for(dir, QTC_PLUGIN_DIRS) {
        exists($$dir/$$dep/$${dep}_dependencies.pri) {
            dependencies_file = $$dir/$$dep/$${dep}_dependencies.pri
            break()
        }
    }
    isEmpty(dependencies_file): \
        error("Plugin dependency $$dep not found")
    include($$dependencies_file)
    dependencyList += "        { \"Name\" : \"$$QTC_PLUGIN_NAME\", \"Version\" : \"$$QTCREATOR_VERSION\", \"Type\" : \"optional\" }"
}
dependencyList = $$join(dependencyList, ",$$escape_expand(\\n)")

dependencyList = "\"Dependencies\" : [$$escape_expand(\\n)$$dependencyList$$escape_expand(\\n)    ]"

# use gui precompiled header for plugins by default
isEmpty(PRECOMPILED_HEADER):PRECOMPILED_HEADER = $$PWD/shared/qtcreator_gui_pch.h

isEmpty(USE_USER_DESTDIR) {
    DESTDIR = $$IDE_PLUGIN_PATH
} else {
    win32 {
        DESTDIRAPPNAME = "qtcreator"
        DESTDIRBASE = "$$(LOCALAPPDATA)"
        isEmpty(DESTDIRBASE):DESTDIRBASE="$$(USERPROFILE)\Local Settings\Application Data"
    } else:macx {
        DESTDIRAPPNAME = "Qt Creator"
        DESTDIRBASE = "$$(HOME)/Library/Application Support"
    } else:unix {
        DESTDIRAPPNAME = "qtcreator"
        DESTDIRBASE = "$$(XDG_DATA_HOME)"
        isEmpty(DESTDIRBASE):DESTDIRBASE = "$$(HOME)/.local/share/data"
        else:DESTDIRBASE = "$$DESTDIRBASE/data"
    }
    DESTDIR = "$$DESTDIRBASE/QtProject/$$DESTDIRAPPNAME/plugins/$$QTCREATOR_VERSION"
}
LIBS += -L$$DESTDIR

# copy the plugin spec
isEmpty(TARGET) {
    error("qtcreatorplugin.pri: You must provide a TARGET")
}

defineReplace(stripOutDir) {
    return($$relative_path($$1, $$OUT_PWD))
}

PLUGINJSON = $$_PRO_FILE_PWD_/$${TARGET}.json
PLUGINJSON_IN = $${PLUGINJSON}.in
exists($$PLUGINJSON_IN) {
    OTHER_FILES += $$PLUGINJSON_IN
    QMAKE_SUBSTITUTES += $$PLUGINJSON_IN
    PLUGINJSON = $$OUT_PWD/$${TARGET}.json
} else {
    # need to support that for external plugins
    OTHER_FILES += $$PLUGINJSON
}

osx: QMAKE_LFLAGS_SONAME = -Wl,-install_name,@rpath/PlugIns/
include(rpath.pri)

contains(QT_CONFIG, reduce_exports):CONFIG += hide_symbols

TEMPLATE = lib
CONFIG += plugin plugin_with_soname
linux*:QMAKE_LFLAGS += $$QMAKE_LFLAGS_NOUNDEF

!macx {
    target.path = $$QTC_PREFIX/$$IDE_LIBRARY_BASENAME/qtcreator/plugins
    INSTALLS += target
}

MIMETYPES = $$_PRO_FILE_PWD_/$${TARGET}.mimetypes.xml
exists($$MIMETYPES):OTHER_FILES += $$MIMETYPES

TARGET = $$qtLibraryName($$TARGET)

