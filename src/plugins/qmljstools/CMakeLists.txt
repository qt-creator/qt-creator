add_qtc_plugin(QmlJSTools
  DEPENDS LanguageUtils
  PUBLIC_DEPENDS QmlJS
  PLUGIN_DEPENDS Core CppEditor ProjectExplorer QtSupport TextEditor
  SOURCES
    qmlformatsettings.cpp qmlformatsettings.h
    qmljsbundleprovider.cpp qmljsbundleprovider.h
    qmljssettings.cpp qmljssettings.h
    qmljsfunctionfilter.cpp qmljsfunctionfilter.h
    qmljsindenter.cpp qmljsindenter.h
    qmljsmodelmanager.cpp qmljsmodelmanager.h
    qmljsqtstylecodeformatter.cpp qmljsqtstylecodeformatter.h
    qmljsrefactoringchanges.cpp qmljsrefactoringchanges.h
    qmljssemanticinfo.cpp qmljssemanticinfo.h
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

qtc_add_resources(QmlJSTools "images"
  PREFIX "/qmljstools"
  BASE "."
  FILES
  images/settingscategory_qml.png
  images/settingscategory_qml@2x.png
)
