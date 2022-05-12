add_qtc_library(QtcSsh
  DEPENDS Qt5::Core Qt5::Network Qt5::Widgets Utils
  SOURCES
    ssh_global.h
    sshparameters.cpp sshparameters.h
    sshsettings.cpp sshsettings.h
)
