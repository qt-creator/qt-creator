add_qtc_plugin(QmlJSTools
  DEPENDS LanguageUtils
  PUBLIC_DEPENDS QmlJS
  PLUGIN_DEPENDS Core CppEditor ProjectExplorer QtSupport TextEditor
  SOURCES
    qmlformatsettings.cpp qmlformatsettings.h
    qmljsbundleprovider.cpp qmljsbundleprovider.h
    qmljscodestylesettings.cpp qmljscodestylesettings.h
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
    qmljstoolsinternalconstants.h
    qmljstoolsplugin.cpp
)

extend_qtc_plugin(QmlJSTools
  CONDITION WITH_TESTS
  SOURCES
    qmljstools_test.cpp
    qmljstools_test.h
)
