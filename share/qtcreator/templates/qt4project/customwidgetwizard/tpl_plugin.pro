CONFIG      += plugin debug_and_release
TARGET      = $$qtLibraryTarget(@PLUGIN_NAME@)
TEMPLATE    = lib

HEADERS     =@PLUGIN_HEADERS@
SOURCES     =@PLUGIN_SOURCES@
RESOURCES   = @PLUGIN_RESOURCES@
LIBS        += -L. @WIDGET_LIBS@

greaterThan(QT_MAJOR_VERSION, 4) {
    QT += designer
} else {
    CONFIG += designer
}

target.path = $$[QT_INSTALL_PLUGINS]/designer
INSTALLS    += target

@INCLUSIONS@
