extend_qtc_test(unittest
  SOURCES
    dsthemegroup-test.cpp
    dsthememgr-test.cpp
    dsthemeqml-test.cpp
)

extend_qtc_test(unittest
  SOURCES_PREFIX "${QML_DESIGNER_DIRECTORY}/libs/designsystem"
  INCLUDES "${QML_DESIGNER_DIRECTORY}/libs/designsystem"
  DEFINES DESIGNSYSTEM_STATIC_LIBRARY
  DEPENDS
    Qt::Core Qt::Widgets QmlDesignerUtils TestDesignerCore
  SOURCES
    dsconstants.h
    dsthemegroup.h dsthemegroup.cpp
    dsthememanager.h dsthememanager.cpp
)
