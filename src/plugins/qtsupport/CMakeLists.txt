add_qtc_plugin(QtSupport
  DEPENDS Qt::Xml
  PUBLIC_DEPENDS ProParser
  PLUGIN_DEPENDS Core ProjectExplorer ResourceEditor
  SOURCES
    baseqtversion.cpp baseqtversion.h
    codegenerator.cpp codegenerator.h
    codegensettings.cpp codegensettings.h
    exampleslistmodel.cpp exampleslistmodel.h
    examplesparser.cpp
    examplesparser.h
    externaleditors.cpp externaleditors.h
    gettingstartedwelcomepage.cpp gettingstartedwelcomepage.h
    profilereader.cpp profilereader.h
    qscxmlcgenerator.cpp qscxmlcgenerator.h
    qtbuildaspects.cpp qtbuildaspects.h
    qtconfigwidget.cpp qtconfigwidget.h
    qtcppkitinfo.cpp qtcppkitinfo.h
    qtkitaspect.cpp qtkitaspect.h
    qtoptionspage.cpp qtoptionspage.h
    qtoutputformatter.cpp qtoutputformatter.h
    qtparser.cpp qtparser.h
    qtprojectimporter.cpp qtprojectimporter.h
    qtsupport.qrc
    qtsupport_global.h
    qtsupporttr.h
    qtsupportconstants.h
    qtsupportplugin.cpp qtsupportplugin.h
    qttestparser.cpp qttestparser.h
    qtversionfactory.h
    qtversionmanager.cpp qtversionmanager.h
    qtversions.cpp qtversions.h
    translationwizardpage.cpp translationwizardpage.h
    uicgenerator.cpp uicgenerator.h
)
