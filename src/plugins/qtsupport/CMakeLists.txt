add_qtc_plugin(QtSupport
  DEPENDS Qt5::Xml
  PUBLIC_DEPENDS ProParser
  PLUGIN_DEPENDS Core CppTools ProjectExplorer ResourceEditor
  SOURCES
    baseqtversion.cpp baseqtversion.h
    codegenerator.cpp codegenerator.h
    codegensettings.cpp codegensettings.h
    codegensettingspage.cpp codegensettingspage.h
    codegensettingspagewidget.ui
    desktopqtversion.cpp desktopqtversion.h
    exampleslistmodel.cpp exampleslistmodel.h
    gettingstartedwelcomepage.cpp gettingstartedwelcomepage.h
    profilereader.cpp profilereader.h
    qmldumptool.cpp qmldumptool.h
    qscxmlcgenerator.cpp qscxmlcgenerator.h
    qtconfigwidget.cpp qtconfigwidget.h
    qtcppkitinfo.cpp qtcppkitinfo.h
    qtkitinformation.cpp qtkitinformation.h
    qtoptionspage.cpp qtoptionspage.h
    qtoutputformatter.cpp qtoutputformatter.h
    qtparser.cpp qtparser.h
    qtprojectimporter.cpp qtprojectimporter.h
    qtsupport.qrc
    qtsupport_global.h
    qtsupportconstants.h
    qtsupportplugin.cpp qtsupportplugin.h
    qttestparser.cpp qttestparser.h
    qtversionfactory.cpp qtversionfactory.h
    qtversioninfo.ui
    qtversionmanager.cpp qtversionmanager.h qtversionmanager.ui
    screenshotcropper.cpp screenshotcropper.h
    showbuildlog.ui
    uicgenerator.cpp uicgenerator.h
)
