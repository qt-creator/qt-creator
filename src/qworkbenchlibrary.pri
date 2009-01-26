IDE_BUILD_TREE = $$OUT_PWD/../../../
include(qworkbench.pri)

win32 {
	DLLDESTDIR = $$IDE_APP_PATH
}

DESTDIR = $$IDE_LIBRARY_PATH

include(rpath.pri)

TARGET = $$qtLibraryTarget($$TARGET)

contains(QT_CONFIG, reduce_exports):CONFIG += hide_symbols

linux-* {
	target.path = /lib/qtcreator
	INSTALLS += target
    }
