#todo
# - handle if there is no debug python lib python35_d
# - needs to be tested

if (MINGW)
  message(STATUS "MinGW detected. Removing qtcreatorcdbext from build.")
  return()
endif()

include(CheckIncludeFile)
check_include_file(wdbgexts.h HAVE_WDBGEXTS_H)
if (NOT HAVE_WDBGEXTS_H)
  message(WARNING "wdbgexts.h not found. Removing qtcreatorcdbext from build.")
  return()
endif()

find_package(PythonLibs 3.5)
if (NOT ${PYTHONLIBS_FOUND})
  message(WARNING "PythonLibs 3.5 not found. Removing qtcreatorcdbext from build.")
  return()
endif()

add_qtc_library(qtcreatorcdbext
  DEPENDS ${PYTHON_LIBRARIES}
  INCLUDES ${PYTHON_INCLUDE_DIR}
  DEFINES WITH_PYTHON=1
  SOURCES
    common.cpp common.h
    containers.cpp containers.h
    eventcallback.cpp eventcallback.h
    extensioncontext.cpp extensioncontext.h
    gdbmihelpers.cpp gdbmihelpers.h
    iinterfacepointer.h
    knowntype.h
    outputcallback.cpp outputcallback.h
    pycdbextmodule.cpp pycdbextmodule.h
    pyfield.cpp pyfield.h
    pystdoutredirect.cpp pystdoutredirect.h
    pytype.cpp pytype.h
    pyvalue.cpp pyvalue.h
    qtcreatorcdbextension.cpp
    stringutils.cpp stringutils.h
    symbolgroup.cpp symbolgroup.h
    symbolgroupnode.cpp symbolgroupnode.h
    symbolgroupvalue.cpp symbolgroupvalue.h
)
