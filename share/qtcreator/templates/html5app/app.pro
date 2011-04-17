# Add more folders to ship with the application, here
# DEPLOYMENTFOLDERS #
folder_01.source = html
DEPLOYMENTFOLDERS = folder_01
# DEPLOYMENTFOLDERS_END #

# Define TOUCH_OPTIMIZED_NAVIGATION for touch optimization and flicking
# TOUCH_OPTIMIZED_NAVIGATION #
DEFINES += TOUCH_OPTIMIZED_NAVIGATION

# TARGETUID3 #
symbian:TARGET.UID3 = 0xE1111234

# Allow network access on Symbian
# NETWORKACCESS #
symbian:TARGET.CAPABILITY += NetworkServices

# Smart Installer package's UID
# This UID is from the protected range and therefore the package will
# fail to install if self-signed. By default qmake uses the unprotected
# range value if unprotected UID is defined for the application and
# 0x2002CCCF value if protected UID is given to the application
#symbian:DEPLOYMENT.installer_header = 0x2002CCCF

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
