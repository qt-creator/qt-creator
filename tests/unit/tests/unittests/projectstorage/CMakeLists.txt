# qmldesigner/designercore/projectstorage
extend_qtc_test(unittest
  SOURCES
    directorypathcompressor-test.cpp
    filestatuscache-test.cpp
    modulescanner-test.cpp
    projectstorage-test.cpp
    projectstoragepathwatcher-test.cpp
    projectstorageupdater-test.cpp
    typeannotationreader-test.cpp
)

extend_qtc_test(unittest
  CONDITION TARGET Qt6::QmlDomPrivate AND TARGET Qt6::QmlCompilerPrivate AND IS_SUPPORTED_PROJECTSTORAGE_QT
  DEPENDS Qt6::QmlDomPrivate Qt6::QmlCompilerPrivate
  SOURCES
    qmldocumentparser-test.cpp
    qmltypesparser-test.cpp
)

extend_qtc_test(unittest
  CONDITION TARGET Qt6::QmlDomPrivate AND TARGET Qt6::QmlCompilerPrivate AND IS_SUPPORTED_PROJECTSTORAGE_QT
  SOURCES_PREFIX "${QmlDesignerDir}/designercore"
  DEPENDS Qt6::QmlDomPrivate Qt6::QmlCompilerPrivate
  DEFINES QDS_BUILD_QMLPARSER QML_DOM_MSVC2019_COMPAT # QML_DOM_MSVC2019_COMPAT can be revmoved for Qt 6.8
  SOURCES
    projectstorage/qmldocumentparser.cpp projectstorage/qmldocumentparser.h
    projectstorage/qmltypesparser.cpp projectstorage/qmltypesparser.h
)

unittest_copy_data_folder()
