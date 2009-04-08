IDE_BUILD_TREE = $$OUT_PWD/../../

include(../qworkbench.pri)
include(../shared/qtsingleapplication/qtsingleapplication.pri)

TEMPLATE = app
TARGET = $$IDE_APP_TARGET
DESTDIR = ../../bin


SOURCES += main.cpp

include(../rpath.pri)

win32 {
        RC_FILE = qtcreator.rc
}

macx {
        ICON = qtcreator.icns
}

macx {
    CONFIG(debug, debug|release):LIBS *= -lExtensionSystem_debug -lAggregation_debug
    else:LIBS *= -lExtensionSystem -lAggregation
}
win32 {
    CONFIG(debug, debug|release):LIBS *= -lExtensionSystemd -lAggregationd
    else:LIBS *= -lExtensionSystem -lAggregation
}
unix:!macx {
    LIBS *= -lExtensionSystem -lAggregation

    # make sure the wrapper is in place
    !exists($$OUT_PWD/app.pro) {
        # we are shadow build
        COPYSRC = $$PWD/$$DESTDIR/$$IDE_APP_WRAPPER
        COPYDEST = $$OUT_PWD/$$DESTDIR/$$IDE_APP_WRAPPER
        win32:COPYSRC ~= s|/+|\|
        win32:COPYDEST ~= s|/+|\|
        unix:SEPARATOR = ;
        win32:SEPARATOR = &
        QMAKE_POST_LINK += $${QMAKE_COPY_FILE} $${COPYSRC} $${COPYDEST} $$SEPARATOR
    }

    target.files += $$OUT_PWD/$$DESTDIR/$$IDE_APP_WRAPPER
    target.files += $$OUT_PWD/$$DESTDIR/$$IDE_APP_TARGET
    target.path  = /bin
    INSTALLS    += target

}

include(../../share/share.pri)
