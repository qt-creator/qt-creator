add_subdirectory(source)

add_qtc_executable(clangbackend
  DEPENDS Qt5::Core Qt5::Network clangbackend_lib Sqlite ClangSupport libclang
  SOURCES
    ../qtcreatorcrashhandler/crashhandlersetup.cpp ../qtcreatorcrashhandler/crashhandlersetup.h
    clangbackendmain.cpp
)
