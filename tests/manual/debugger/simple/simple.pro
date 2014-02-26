
TEMPLATE = subdirs

SUBDIRS += simple_test_app.pro simple_test_plugin.pro

exists($$QMAKE_INCDIR_QT/QtCore/private/qobject_p.h):DEFINES += HAS_PRIVATE
exists(/usr/include/boost/optional.hpp): DEFINES += HAS_BOOST
exists(/usr/include/eigen2/Eigen/Core): DEFINES += HAS_EIGEN

*g++* {
    DEFINES += USE_CXX11
    QMAKE_CXXFLAGS += -std=c++0x
}
