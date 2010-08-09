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

# MAINQML #
OTHER_FILES = qml/app/app.qml

# TARGETUID3 #
symbian:TARGET.UID3 = 0xE1111234

# Define to enable the Qml Inspector in debug mode
# QMLINSPECTOR #
#DEFINES += QMLINSPECTOR

include($$PWD/qmlapplication.pri)
