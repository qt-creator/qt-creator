set(UTILSDIR "${PROJECT_SOURCE_DIR}/src/libs/utils")

add_qtc_executable(qtcreator_processlauncher
  INCLUDES "${UTILSDIR}"
  DEPENDS Qt5::Core Qt5::Network
  DEFINES QTCREATOR_UTILS_STATIC_LIB
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
    )
