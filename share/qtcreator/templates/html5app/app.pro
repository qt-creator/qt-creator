greaterThan(QT_MAJOR_VERSION, 4):QT += widgets webkitwidgets

# Add more folders to ship with the application, here
# DEPLOYMENTFOLDERS #
folder_01.source = html
DEPLOYMENTFOLDERS = folder_01
# DEPLOYMENTFOLDERS_END #

# Define TOUCH_OPTIMIZED_NAVIGATION for touch optimization and flicking
# TOUCH_OPTIMIZED_NAVIGATION #
DEFINES += TOUCH_OPTIMIZED_NAVIGATION

# If your application uses the Qt Mobility libraries, uncomment the following
# lines and add the respective components to the MOBILITY variable.
# CONFIG += mobility
# MOBILITY +=

# The .cpp file which was generated for your project. Feel free to hack it.
SOURCES += main.cpp

# Please do not modify the following two lines. Required for deployment.
include(html5applicationviewer/html5applicationviewer.pri)
# REMOVE_NEXT_LINE (wizard will remove the include and merge the touchnavigation code into html5applicationviewer.cpp, instead) #
include(html5applicationviewer/touchnavigation/touchnavigation.pri)
# REMOVE_NEXT_LINE (wizard will remove the include and append deployment.pri to qmlapplicationviewer.pri, instead) #
include(../shared/deployment.pri)
qtcAddDeployment()
