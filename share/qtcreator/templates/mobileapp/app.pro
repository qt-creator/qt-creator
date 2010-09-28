# Add files and directories to ship with the application 
# by adapting the examples below.
# file1.source = myfile
# dir1.source = mydir
DEPLOYMENTFOLDERS = # file1 dir1

# Avoid auto screen rotation
# ORIENTATIONLOCK #
DEFINES += ORIENTATIONLOCK

# Needs to be defined for Symbian
# NETWORKACCESS #
DEFINES += NETWORKACCESS

# TARGETUID3 #
symbian:TARGET.UID3 = 0xE1111234

# Smart Installer package's UID
# This UID is from the protected range 
# and therefore the package will fail to install if self-signed
symbian:DEPLOYMENT.installer_header = 0x2002CCCF

symbian:ICON = symbianicon.svg

SOURCES += main.cpp mainwindow.cpp
HEADERS += mainwindow.h
FORMS += mainwindow.ui

include(../shared/deployment.pri)

qtcAddDeployment()
