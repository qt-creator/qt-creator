IDE_BUILD_TREE = $$OUT_PWD/../../

include(../qworkbench.pri)
include(../../shared/qtsingleapplication/qtsingleapplication.pri)

macx {
    CONFIG(debug, debug|release):LIBS *= -lExtensionSystem_debug -lAggregation_debug
    else:LIBS *= -lExtensionSystem -lAggregation
}
win32 {
    CONFIG(debug, debug|release):LIBS *= -lExtensionSystemd -lAggregationd
    else:LIBS *= -lExtensionSystem -lAggregation
}
linux-* {
    LIBS *= -lExtensionSystem -lAggregation
    ISGCC33=$$(GCC33)
    !equals(ISGCC33, 1):QT += svg dbus

}

TEMPLATE = app
TARGET = $$IDE_APP_TARGET
DESTDIR = ../../bin


SOURCES += main.cpp

macx {
        SNIPPETS.path = Contents/Resources
        SNIPPETS.files = $$IDE_SOURCE_TREE/bin/snippets
        TEMPLATES.path = Contents/Resources
        TEMPLATES.files = $$IDE_SOURCE_TREE/bin/templates
        DESIGNER.path = Contents/Resources
        DESIGNER.files = $$IDE_SOURCE_TREE/bin/designer
        SCHEMES.path = Contents/Resources
        SCHEMES.files = $$IDE_SOURCE_TREE/bin/schemes
        GDBDEBUGGER.path = Contents/Resources
        GDBDEBUGGER.files = $$IDE_SOURCE_TREE/bin/gdbmacros
        LICENSE.path = Contents/Resources
        LICENSE.files = $$IDE_SOURCE_TREE/bin/license.txt
        RUNINTERMINAL.path = Contents/Resources
        RUNINTERMINAL.files = $$IDE_SOURCE_TREE/bin/runInTerminal.command
        QMAKE_BUNDLE_DATA += SNIPPETS TEMPLATES DESIGNER SCHEMES GDBDEBUGGER LICENSE RUNINTERMINAL
        QMAKE_INFO_PLIST = $$PWD/Info.plist
}
!macx {
    # make sure the resources are in place
    !exists($$OUT_PWD/app.pro) {
        unix:SEPARATOR = ;
        win32:SEPARATOR = &
        # we are shadow build
        COPYSRC = snippets \
                   templates \
                   designer \
                   schemes \
                   gdbmacros
        COPYDEST = $${OUT_PWD}/../../bin
        win32:COPYDEST ~= s|/+|\|
        for(tmp,COPYSRC) {
          REALSRC = $$IDE_SOURCE_TREE/bin/$$tmp
          REALDEST = $$COPYDEST/$$tmp
          win32:tmp ~= s|/+|\|
          win32:REALSRC ~= s|/+|\|
          win32:REALDEST ~= s|/+|\|
          QMAKE_POST_LINK += $${QMAKE_COPY_DIR} $${REALSRC} $${REALDEST} $$SEPARATOR
        }
    }
}

linux-* {
    #do the rpath by hand since it's not possible to use ORIGIN in QMAKE_RPATHDIR
    QMAKE_RPATHDIR += \$\$ORIGIN/../lib
    IDE_PLUGIN_RPATH = $$join(QMAKE_RPATHDIR, ":")
    QMAKE_LFLAGS += -Wl,-z,origin \'-Wl,-rpath,$${IDE_PLUGIN_RPATH}\'
    QMAKE_RPATHDIR =
}

win32 {
        RC_FILE = qtcreator.rc
}

macx {
        ICON = qtcreator.icns
}
