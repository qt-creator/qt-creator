# Add more folders to ship with the application, here
# DEPLOYMENTFOLDERS #
folder_01.source = qml/app
folder_01.target = qml
DEPLOYMENTFOLDERS = folder_01
# DEPLOYMENTFOLDERS_END #

# Avoid auto screen rotation
# ORIENTATIONLOCK #
DEFINES += ORIENTATIONLOCK

# Needs to be defined for Symbian
# NETWORKACCESS #
DEFINES += NETWORKACCESS

# TARGETUID3 #
symbian:TARGET.UID3 = 0xE1111234

symbian:ICON = symbianicon.svg

# Define to enable the Qml Inspector in debug mode
# QMLINSPECTOR #
#DEFINES += QMLINSPECTOR

SOURCES += main.cpp

include(qmlapplicationviewer/qmlapplicationviewer.pri)
include(../shared/deployment.pri)

qtcAddDeployment()
