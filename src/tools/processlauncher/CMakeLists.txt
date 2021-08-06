set(UTILSDIR "${PROJECT_SOURCE_DIR}/src/libs/utils")

add_qtc_executable(qtcreator_processlauncher
  INCLUDES "${UTILSDIR}"
  DEPENDS Qt5::Core Qt5::Network
  SOURCES
    launcherlogging.cpp
    launcherlogging.h
    launchersockethandler.cpp
    launchersockethandler.h
    processlauncher-main.cpp
    ${UTILSDIR}/launcherpackets.cpp
    ${UTILSDIR}/launcherpackets.h
    ${UTILSDIR}/processutils.cpp
    ${UTILSDIR}/processutils.h
    )
