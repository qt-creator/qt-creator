exists($$PWD/litehtml/CMakeLists.txt) {
    !build_pass|win32 {
        LITEHTML_BUILD_PATH = "$${OUT_PWD}/litehtml/build"
        LITEHTML_SOURCE_PATH = "$${PWD}/litehtml"
        LITEHTML_INSTALL_PATH = "$${OUT_PWD}/litehtml/install"

        BUILD_TYPE = RelWithDebInfo
        CONFIG(release, debug|release): BUILD_TYPE = Release

        # Create build directory
        system("$$sprintf($$QMAKE_MKDIR_CMD, $$shell_path($${LITEHTML_BUILD_PATH}))")

        macos: CMAKE_DEPLOYMENT_TARGET = -DCMAKE_OSX_DEPLOYMENT_TARGET=$${QMAKE_MACOSX_DEPLOYMENT_TARGET}
        win32: LITEHTML_UTF8 = -DLITEHTML_UTF8=ON
        LITEHTML_CMAKE_CMD = \
            "$$QMAKE_CD $$system_quote($$shell_path($${LITEHTML_BUILD_PATH})) && \
             cmake -DCMAKE_BUILD_TYPE=$$BUILD_TYPE \
                   -DCMAKE_INSTALL_PREFIX=$$system_quote($$shell_path($${LITEHTML_INSTALL_PATH})) \
                   -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
                   $$LITEHTML_UTF8 \
                   $$CMAKE_DEPLOYMENT_TARGET \
                   -G $$system_quote('NMake Makefiles') \
                   $$system_quote($$shell_path($${LITEHTML_SOURCE_PATH}))"
        message("$${LITEHTML_CMAKE_CMD}")
        system("$${LITEHTML_CMAKE_CMD}")

        buildlitehtml.commands = "cmake --build $$system_quote($$shell_path($${LITEHTML_BUILD_PATH})) --target install"
        win32: buildlitehtml.target = $$LITEHTML_INSTALL_PATH/lib/litehtml.lib
        else:unix: buildlitehtml.target = $$LITEHTML_INSTALL_PATH/lib/liblitehtml.a
        dummygumbo.depends = buildlitehtml
        win32: dummygumbo.target = $$LITEHTML_INSTALL_PATH/lib/gumbo.lib
        else:unix: dummygumbo.target = $$LITEHTML_INSTALL_PATH/lib/libgumbo.a
        QMAKE_EXTRA_TARGETS += buildlitehtml dummygumbo
        PRE_TARGETDEPS += $$buildlitehtml.target $$dummygumo.target
    }
    LITEHTML_INCLUDE_DIRS = $$LITEHTML_SOURCE_PATH/include $$LITEHTML_SOURCE_PATH/src
    LITEHTML_LIB_DIR = $$LITEHTML_INSTALL_PATH/lib
} else {
    LITEHTML_INCLUDE_DIRS = $$LITEHTML_INSTALL_DIR/include $$LITEHTML_INSTALL_DIR/include/litehtml
    LITEHTML_LIB_DIR = $$LITEHTML_INSTALL_DIR/lib
}

HEADERS += \
    $$PWD/container_qpainter.h \
    $$PWD/qlitehtmlwidget.h

SOURCES += \
    $$PWD/container_qpainter.cpp \
    $$PWD/qlitehtmlwidget.cpp

INCLUDEPATH += $$PWD $$LITEHTML_INCLUDE_DIRS
LIBS += -L$$LITEHTML_LIB_DIR -llitehtml -lgumbo

DEFINES += LITEHTML_UTF8

win32: PRE_TARGETDEPS += $$LITEHTML_LIB_DIR/litehtml.lib $$LITEHTML_LIB_DIR/gumbo.lib
else:unix: PRE_TARGETDEPS += $$LITEHTML_LIB_DIR/liblitehtml.a $$LITEHTML_LIB_DIR/libgumbo.a
