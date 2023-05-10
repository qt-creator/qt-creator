set(LIBSDIR "${PROJECT_SOURCE_DIR}/src/libs")
set(UTILSDIR "${PROJECT_SOURCE_DIR}/src/libs/utils")

add_qtc_executable(qtcreator_processlauncher
  INCLUDES "${LIBSDIR}"
  DEPENDS Qt::Core Qt::Network
  DEFINES UTILS_STATIC_LIBRARY
  SOURCES
    launcherlogging.cpp
    launcherlogging.h
    launchersockethandler.cpp
    launchersockethandler.h
    processlauncher-main.cpp
    ${UTILSDIR}/launcherpackets.cpp
    ${UTILSDIR}/launcherpackets.h
    ${UTILSDIR}/processenums.h
    ${UTILSDIR}/processreaper.cpp
    ${UTILSDIR}/processreaper.h
    ${UTILSDIR}/processutils.cpp
    ${UTILSDIR}/processutils.h
    ${UTILSDIR}/qtcassert.cpp
    ${UTILSDIR}/qtcassert.h
    ${UTILSDIR}/singleton.cpp
    ${UTILSDIR}/singleton.h
    ${UTILSDIR}/threadutils.cpp
    ${UTILSDIR}/threadutils.h
    )

if (MSVC)
    find_library(DbgHelpLib dbghelp)
endif()

extend_qtc_executable(qtcreator_processlauncher CONDITION MSVC
  DEPENDS ${DbgHelpLib}
)
