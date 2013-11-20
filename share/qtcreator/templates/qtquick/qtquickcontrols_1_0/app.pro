# Add more folders to ship with the application, here
# DEPLOYMENTFOLDERS #
folder_01.source = qml/app
folder_01.target = qml
DEPLOYMENTFOLDERS = folder_01
# DEPLOYMENTFOLDERS_END #

# Additional import path used to resolve QML modules in Creator's code model
# QML_IMPORT_PATH #
QML_IMPORT_PATH =

# The .cpp file which was generated for your project. Feel free to hack it.
SOURCES += main.cpp

# Installation path
# target.path =

# Please do not modify the following two lines. Required for deployment.
include(qtquick2controlsapplicationviewer/qtquick2controlsapplicationviewer.pri)
# REMOVE_NEXT_LINE (wizard will remove the include and append deployment.pri to qmlapplicationviewer.pri, instead) #
include(../../shared/deployment.pri)
qtcAddDeployment()
