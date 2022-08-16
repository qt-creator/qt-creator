add_qtc_plugin(QmlJSTools
  DEPENDS LanguageUtils
  PUBLIC_DEPENDS QmlJS
  PLUGIN_DEPENDS Core CppEditor ProjectExplorer QtSupport TextEditor
  SOURCES
    qmljsbundleprovider.cpp qmljsbundleprovider.h
    qmljscodestylepreferencesfactory.cpp qmljscodestylepreferencesfactory.h
    qmljscodestylepreferences.cpp qmljscodestylepreferences.h
    qmljscodestylepreferenceswidget.cpp qmljscodestylepreferenceswidget.h
    qmljscodestylesettings.cpp qmljscodestylesettings.h
    qmljscodestylesettingswidget.cpp qmljscodestylesettingswidget.h
    qmljscodestylesettingspage.cpp qmljscodestylesettingspage.h
    qmljsfunctionfilter.cpp qmljsfunctionfilter.h
    qmljsindenter.cpp qmljsindenter.h
    qmljslocatordata.cpp qmljslocatordata.h
    qmljsmodelmanager.cpp qmljsmodelmanager.h
    qmljsqtstylecodeformatter.cpp qmljsqtstylecodeformatter.h
    qmljsrefactoringchanges.cpp qmljsrefactoringchanges.h
    qmljssemanticinfo.cpp qmljssemanticinfo.h
    qmljstools.qrc
    qmljstools_global.h
    qmljstoolstr.h
    qmljstoolsconstants.h
    qmljstoolsplugin.cpp qmljstoolsplugin.h
    qmljstoolssettings.cpp qmljstoolssettings.h
)

extend_qtc_plugin(QmlJSTools
  CONDITION WITH_TESTS
  SOURCES qmljstools_test.cpp
)
