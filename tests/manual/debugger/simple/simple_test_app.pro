TEMPLATE = app
TARGET = simple_test_app
DEPENDPATH += .
INCLUDEPATH += .
DESTDIR = .

SOURCES +=  simple_test_app.cpp

QT += network xml
!isEmpty(QT.script.name): QT += script

osx {
    DEFINES += USE_CXX11
    QMAKE_CXXFLAGS += -stdlib=libc++
    QMAKE_LFLAGS += -lc++
}

contains(QT_CONFIG, webkit)|!isEmpty(QT.webkit.name) {
    QT += webkit
    greaterThan(QT_MAJOR_VERSION, 4) {
        QT += webkitwidgets
    }
}

greaterThan(QT_MAJOR_VERSION, 4) {
    QT += core-private
    QT *= widgets
}

false {
    QT -= gui webkit widgets
} else {
    DEFINES += USE_GUILIB
}

#unix: QMAKE_CXXFLAGS += -msse2
#DEFINES += USE_BOOST=1

maemo5 {
    target.path = /opt/usr/lib
    target.path = /opt
    INSTALLS += target
}

exists($$QMAKE_INCDIR_QT/QtCore/private/qobject_p.h):DEFINES += HAS_PRIVATE
exists(/usr/include/boost/optional.hpp): DEFINES += HAS_BOOST

exists(/usr/include/eigen2/Eigen/Core) {
    DEFINES += HAS_EIGEN2
    INCLUDEPATH += /usr/include/eigen2
}
exists(/usr/include/eigen3/Eigen/Core) {
    DEFINES += HAS_EIGEN3
    INCLUDEPATH += /usr/include/eigen3
}
exists(/usr/local/include/eigen2/Eigen/Core) {
    DEFINES += HAS_EIGEN2
    INCLUDEPATH += /usr/local/include/eigen2
}
exists(/usr/local/include/eigen3/Eigen/Core) {
    DEFINES += HAS_EIGEN3
    INCLUDEPATH += /usr/local/include/eigen3
}

msvc: DEFINES += _CRT_SECURE_NO_WARNINGS
# Use for semi-automated testing
#DEFINES += USE_AUTORUN=1

ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android
ANDROID_EXTRA_LIBS = $$OUT_PWD/libsimple_test_plugin.so
