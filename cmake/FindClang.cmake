find_package(Clang CONFIG)

option(CLANGTOOLING_LINK_CLANG_DYLIB "Force linking of Clang tooling against clang-cpp" NO)

set(CLANG_FORMAT_LIB clangFormat)
if (TARGET clang-cpp AND CLANGTOOLING_LINK_CLANG_DYLIB)
  set(CLANG_FORMAT_LIB clang-cpp)
endif()

if (WIN32 AND TARGET clangTooling)
  # check that we can compile against clangTooling
  # which requires the build modes to be compatible
  # (debug vs release on Windows)
  include(CheckCXXSourceCompiles)

  set(CMAKE_TRY_COMPILE_CONFIGURATION ${CMAKE_BUILD_TYPE})
  set(CMAKE_REQUIRED_INCLUDES ${CLANG_INCLUDE_DIRS})
  set(CMAKE_REQUIRED_LIBRARIES clangTooling)

  check_cxx_source_compiles([=[
    #include <clang/Tooling/CommonOptionsParser.h>
    #include <clang/Tooling/Tooling.h>
    using namespace clang;
    using namespace clang::tooling;
    llvm::cl::OptionCategory CheckToolCategory("check tool options");
    int main(int argc, const char **argv)
    {
        class Parser : public CommonOptionsParser {
        public:
          Parser(int &argc, const char **argv, llvm::cl::OptionCategory &Category) :
            CommonOptionsParser(argc, argv, Category) {}
        };

        Parser OptionsParser(argc, argv, CheckToolCategory);
        ClangTool Tool(OptionsParser.getCompilations(),
                       OptionsParser.getSourcePathList());
        return 0;
    }
    ]=] QTC_CLANG_BUILDMODE_MATCH
  )
  unset(CMAKE_TRY_COMPILE_CONFIGURATION)
  unset(CMAKE_REQUIRED_INCLUDES)
  unset(CMAKE_REQUIRED_LIBRARIES)
elseif(TARGET clangTooling)
  set(QTC_CLANG_BUILDMODE_MATCH ON)
endif()

if (TARGET clangTooling AND NOT QTC_CLANG_BUILDMODE_MATCH)
  message("Clang build mode mismatch (debug vs release): limiting clangTooling")
endif()
