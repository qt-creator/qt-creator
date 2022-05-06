add_qtc_library(QtcSsh
  DEPENDS Qt5::Core Qt5::Network Qt5::Widgets Utils
  SOURCES
    sftpdefs.cpp sftpdefs.h
    sftptransfer.cpp sftptransfer.h
    ssh.qrc
    ssh_global.h
    sshconnection.cpp sshconnection.h
    sshconnectionmanager.cpp sshconnectionmanager.h
    sshkeycreationdialog.cpp sshkeycreationdialog.h sshkeycreationdialog.ui
    sshlogging.cpp sshlogging_p.h
    sshconnection.cpp sshconnection.h
    sshconnectionmanager.cpp sshconnectionmanager.h
    sshkeycreationdialog.cpp sshkeycreationdialog.h sshkeycreationdialog.ui
    sshlogging.cpp sshlogging_p.h
    sshsettings.cpp sshsettings.h
)
