add_qtc_plugin(QmlJSTools
  DEPENDS LanguageUtils
  PUBLIC_DEPENDS QmlJS
  PLUGIN_DEPENDS Core CppEditor ProjectExplorer QtSupport TextEditor
  SOURCES
    qmlformatsettings.cpp qmlformatsettings.h
    qmljsbundleprovider.cpp qmljsbundleprovider.h
    qmljscodestylepreferenceswidget.cpp qmljscodestylepreferenceswidget.h
    qmljscodestylesettings.cpp qmljscodestylesettings.h
    qmljscodestylesettingspage.cpp qmljscodestylesettingspage.h
    qmljscustomformatterwidget.cpp qmljscustomformatterwidget.h
    qmlformatsettingswidget.cpp qmlformatsettingswidget.h
    qmljsformatterselectionwidget.cpp qmljsformatterselectionwidget.h
    qmljsfunctionfilter.cpp qmljsfunctionfilter.h
    qmljsindenter.cpp qmljsindenter.h
    qmljsmodelmanager.cpp qmljsmodelmanager.h
    qmljsqtstylecodeformatter.cpp qmljsqtstylecodeformatter.h
    qmljsrefactoringchanges.cpp qmljsrefactoringchanges.h
    qmljssemanticinfo.cpp qmljssemanticinfo.h
    qmljstools.qrc
    qmljstools_global.h
    qmljstoolstr.h
    qmljstoolsconstants.h
    qmljstoolsplugin.cpp
    qmljstoolssettings.cpp qmljstoolssettings.h
)

extend_qtc_plugin(QmlJSTools
  CONDITION WITH_TESTS
  SOURCES
    qmljstools_test.cpp
    qmljstools_test.h
)
