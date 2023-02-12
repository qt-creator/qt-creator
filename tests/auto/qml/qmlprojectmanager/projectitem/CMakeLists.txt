set(WITH_TESTS ON)

find_package(Googletest MODULE)

set(QmlProjectItemDir "${PROJECT_SOURCE_DIR}/src/plugins/qmlprojectmanager/buildsystem/projectitem")

add_qtc_test(tst_qml_projectitem
  DEPENDS QmlProjectManager Utils QmlJS Googletest
  INCLUDES "${QmlProjectItemDir}"
  DEFINES
    QT_CREATOR
    SRCDIR="${CMAKE_CURRENT_SOURCE_DIR}"
    TESTDATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
    TESTS_ENABLED_QMLPROJECTITEM
  SOURCES
    tst_projectitem.cpp
    test-getters.cpp
    test-setters.cpp
    test-converters.cpp
    test-filefilters.cpp
    common.h
    "${QmlProjectItemDir}/qmlprojectitem.cpp"
    "${QmlProjectItemDir}/converters.cpp"
    "${QmlProjectItemDir}/filefilteritems.cpp"
)
