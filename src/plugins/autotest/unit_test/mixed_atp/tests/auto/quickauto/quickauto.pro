TEMPLATE = app
TARGET = test_mal_qtquick

CONFIG += warn_on qmltestcase

DISTFILES += \
    tst_test1.qml \
    tst_test2.qml \
    TestDummy.qml \
    bar/tst_foo.qml \
    tst_test3.qml

SOURCES += \
    main.cpp
