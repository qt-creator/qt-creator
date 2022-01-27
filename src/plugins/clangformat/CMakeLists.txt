add_qtc_plugin(ClangFormat
  CONDITION TARGET libclang AND LLVM_PACKAGE_VERSION VERSION_GREATER_EQUAL 10.0.0 AND QTC_CLANG_BUILDMODE_MATCH
  DEPENDS Utils Qt5::Widgets ${CLANG_FORMAT_LIB}
  INCLUDES "${CLANG_INCLUDE_DIRS}"
  PLUGIN_DEPENDS Core TextEditor CppEditor ProjectExplorer
  SOURCES
    clangformatbaseindenter.cpp clangformatbaseindenter.h
    clangformatchecks.ui
    clangformatconfigwidget.cpp clangformatconfigwidget.h clangformatconfigwidget.ui
    clangformatconstants.h
    clangformatfile.cpp clangformatfile.h
    clangformatindenter.cpp clangformatindenter.h
    clangformatplugin.cpp clangformatplugin.h
    clangformatsettings.cpp clangformatsettings.h
    clangformatutils.cpp clangformatutils.h
)

extend_qtc_plugin(ClangFormat
  CONDITION UNIX AND NOT APPLE
  PROPERTIES LINK_FLAGS "-Wl,--exclude-libs,ALL"
)

extend_qtc_plugin(ClangFormat
  CONDITION WITH_TESTS
  SOURCES
    tests/clangformat-test.cpp
    tests/clangformat-test.h
  DEFINES
    TESTDATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/data"
)
