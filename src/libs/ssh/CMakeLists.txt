add_qtc_library(QtcSsh
  DEPENDS Qt5::Core Qt5::Network Qt5::Widgets Utils
  SOURCES
    ssh.qrc
    ssh_global.h
    sshkeycreationdialog.cpp sshkeycreationdialog.h sshkeycreationdialog.ui
    sshparameters.cpp sshparameters.h
    sshsettings.cpp sshsettings.h
)
