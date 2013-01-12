TEMPLATE = lib
TARGET = AutotoolsProjectManager
#PROVIDER = Openismus

include(../../qtcreatorplugin.pri)
include(autotoolsprojectmanager_dependencies.pri)

HEADERS = autotoolsprojectplugin.h\
          autotoolsopenprojectwizard.h\
          autotoolsmanager.h\
          autotoolsprojectfile.h\
          autotoolsprojectnode.h\
          autotoolsproject.h\
          autotoolsbuildsettingswidget.h\
          autotoolsbuildconfiguration.h\
          autotoolsprojectconstants.h\
          makestep.h\
          autogenstep.h\
          autoreconfstep.h\
          configurestep.h\
          makefileparserthread.h\
          makefileparser.h
SOURCES = autotoolsprojectplugin.cpp\
          autotoolsopenprojectwizard.cpp\
          autotoolsmanager.cpp\
          autotoolsprojectfile.cpp\
          autotoolsprojectnode.cpp\
          autotoolsproject.cpp\
          autotoolsbuildsettingswidget.cpp\
          autotoolsbuildconfiguration.cpp\
          makestep.cpp\
          autogenstep.cpp\
          autoreconfstep.cpp\
          configurestep.cpp\
          makefileparserthread.cpp\
          makefileparser.cpp
RESOURCES += autotoolsproject.qrc
