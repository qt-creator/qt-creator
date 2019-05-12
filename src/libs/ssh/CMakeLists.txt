add_qtc_library(QtcSsh
  DEPENDS Qt5::Core Qt5::Network Qt5::Widgets Utils
  SOURCES
    sftpdefs.cpp sftpdefs.h
    sftpfilesystemmodel.cpp sftpfilesystemmodel.h
    sftpsession.cpp sftpsession.h
    sftptransfer.cpp sftptransfer.h
    ssh.qrc
    ssh_global.h
    sshconnection.cpp sshconnection.h
    sshconnectionmanager.cpp sshconnectionmanager.h
    sshkeycreationdialog.cpp sshkeycreationdialog.h sshkeycreationdialog.ui
    sshlogging.cpp sshlogging_p.h
    sshprocess.cpp sshprocess.h
    sshconnection.cpp sshconnection.h
    sshconnectionmanager.cpp sshconnectionmanager.h
    sshkeycreationdialog.cpp sshkeycreationdialog.h sshkeycreationdialog.ui
    sshlogging.cpp sshlogging_p.h
    sshremoteprocess.cpp sshremoteprocess.h
    sshremoteprocessrunner.cpp sshremoteprocessrunner.h
    sshsettings.cpp sshsettings.h
)
