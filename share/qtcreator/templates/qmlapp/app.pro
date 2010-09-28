# Add more folders to ship with the application, here
# DEPLOYMENTFOLDERS #
folder_01.source = qml/app
folder_01.target = qml
DEPLOYMENTFOLDERS = folder_01
# DEPLOYMENTFOLDERS_END #

# Additional import path used to resolve Qml modules in Creator's code model
# QML_IMPORT_PATH #
QML_IMPORT_PATH =

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

# Define QMLJSDEBUGGER to enable basic debugging (setting breakpoints etc)
# Define QMLOBSERVER for advanced features (requires experimental QmlInspector plugin!)
#DEFINES += QMLJSDEBUGGER
#DEFINES += QMLOBSERVER

SOURCES += main.cpp

include(qmlapplicationviewer/qmlapplicationviewer.pri)
include(../shared/deployment.pri)

qtcAddDeployment()
