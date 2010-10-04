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

# Define QMLJSDEBUGGER to enable basic debugging (setting breakpoints etc)
# Define QMLOBSERVER for advanced features (requires experimental QmlInspector plugin!)
#DEFINES += QMLJSDEBUGGER
#DEFINES += QMLOBSERVER

# The .cpp file which was generated for your project. Feel free to hack it.
SOURCES += main.cpp

# Please do not modify the following two lines. Required for deployment.
include(qmlapplicationviewer/qmlapplicationviewer.pri)
# INCLUDE_DEPLOYMENT_PRI (wizard will remove the include and append deployment.pri to qmlapplicationviewer.pri, instead) #
include(../shared/deployment.pri)
qtcAddDeployment()
