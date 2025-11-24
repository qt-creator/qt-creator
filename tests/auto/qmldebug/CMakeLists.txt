add_qtc_test(tst_qmlevent
  DEPENDS QmlDebug
  SOURCES
    tst_qmlevent.cpp
    tst_qmlevent.h
)

add_qtc_test(tst_qmleventlocation
  DEPENDS QmlDebug
  SOURCES
    tst_qmleventlocation.cpp
    tst_qmleventlocation.h
)

add_qtc_test(tst_qmleventtype
  DEPENDS QmlDebug
  SOURCES
    tst_qmleventtype.cpp
    tst_qmleventtype.h
)

add_qtc_test(tst_qmlprofilertraceclient
  DEPENDS QmlDebug Utils Tracing Qt::Network
  SOURCES
    tst_qmlprofilertraceclient.cpp
    tst_qmlprofilertraceclient.h
    tests.qrc
)
