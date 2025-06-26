add_qtc_plugin(Python
  PLUGIN_DEPENDS Core LanguageClient ProjectExplorer TextEditor QtSupport
  SOURCES
    ../../libs/3rdparty/toml11/toml.hpp
    pipsupport.cpp pipsupport.h
    pyprojecttoml.cpp pyprojecttoml.h
    pyside.cpp pyside.h
    pythonbuildconfiguration.cpp pythonbuildconfiguration.h
    pysideuicextracompiler.cpp pysideuicextracompiler.h
    pythonbuildsystem.cpp pythonbuildsystem.h
    pythonconstants.h
    pythoneditor.cpp pythoneditor.h
    pythonformattoken.h
    pythonhighlighter.cpp pythonhighlighter.h
    pythonindenter.cpp pythonindenter.h
    pythonkitaspect.h pythonkitaspect.cpp
    pythonlanguageclient.cpp pythonlanguageclient.h
    pythonplugin.cpp
    pythonproject.cpp pythonproject.h
    pythonrunconfiguration.cpp pythonrunconfiguration.h
    pythonscanner.cpp pythonscanner.h
    pythonsettings.cpp pythonsettings.h
    pythontr.h
    pythonutils.cpp pythonutils.h
    pythonwizardpage.cpp pythonwizardpage.h
)

extend_qtc_plugin(Python
  CONDITION WITH_TESTS
  SOURCES
    tests/pyprojecttoml_test.cpp
    tests/pyprojecttoml_test.h
)

qtc_add_resources(Python "images"
  PREFIX "/python"
  BASE "."
  FILES
    images/qtforpython_neon.png
    images/settingscategory_python.png
    images/settingscategory_python@2x.png
)
file(GLOB_RECURSE test_cases RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/tests/testfiles tests/testfiles/*)
qtc_add_resources(Python "testcases"
  PREFIX "unittests/Python"
  CONDITION WITH_TESTS
  FILES_PREFIX "tests/testfiles"
  FILES ${test_cases}
)
