add_qtc_plugin(ClangFormat
  CONDITION TARGET ${CLANG_FORMAT_LIB} AND LLVM_PACKAGE_VERSION VERSION_GREATER_EQUAL 10.0.0 AND (QTC_CLANG_BUILDMODE_MATCH OR CLANGTOOLING_LINK_CLANG_DYLIB)
  DEPENDS Utils Qt::Widgets ${CLANG_FORMAT_LIB}
  PLUGIN_DEPENDS Core TextEditor CppEditor ProjectExplorer
  SOURCES
    clangformatbaseindenter.cpp clangformatbaseindenter.h
    clangformatchecks.cpp clangformatchecks.h
    clangformatconfigwidget.cpp clangformatconfigwidget.h
    clangformatglobalconfigwidget.cpp clangformatglobalconfigwidget.h
    clangformatconstants.h
    clangformatfile.cpp clangformatfile.h
    clangformatindenter.cpp clangformatindenter.h
    clangformatplugin.cpp clangformatplugin.h
    clangformatsettings.cpp clangformatsettings.h
    clangformattr.h
    clangformatutils.cpp clangformatutils.h
  EXPLICIT_MOC
    clangformatconfigwidget.cpp clangformatconfigwidget.h
)

if(TARGET ClangFormat)
  # "system" includes, so warnings are ignored
  target_include_directories(ClangFormat SYSTEM PRIVATE "${CLANG_INCLUDE_DIRS}")
endif()

if(MSVC AND TARGET ClangFormat)
  target_compile_options(ClangFormat PUBLIC /wd4267 /wd4624)
endif()

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
  EXPLICIT_MOC tests/clangformat-test.h
)
