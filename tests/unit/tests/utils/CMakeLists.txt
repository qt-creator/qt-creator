add_qtc_library(TestUtils OBJECT
  EXCLUDE_FROM_INSTALL
  PUBLIC_INCLUDES ${CMAKE_CURRENT_LIST_DIR}
  DEPENDS
    Qt::Core Googletest Utils
  SOURCES
    googletest.h
    google-using-declarations.h
    conditionally-disabled-tests.h
    eventspy.cpp
    eventspy.h
    fakeprocess.cpp
    fakeprocess.h
    notification.h
    processevents-utilities.cpp
    processevents-utilities.h
    spydummy.cpp
    spydummy.h
    unittest-utility-functions.h
)
