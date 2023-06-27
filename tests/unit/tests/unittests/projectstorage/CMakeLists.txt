# qmldesigner/designercore/projectstorage
extend_qtc_test(unittest
  SOURCES
    directorypathcompressor-test.cpp
    filestatuscache-test.cpp
    modulescanner-test.cpp
    projectstorage-test.cpp
    projectstoragepathwatcher-test.cpp
    projectstorageupdater-test.cpp
    sourcepath-test.cpp
    sourcepathcache-test.cpp
    sourcepathview-test.cpp
    storagecache-test.cpp
)

extend_qtc_test(unittest
  CONDITION TARGET Qt6::QmlDomPrivate AND TARGET Qt6::QmlCompilerPrivate AND Qt6_VERSION VERSION_GREATER_EQUAL 6.5.0  AND Qt6_VERSION VERSION_LESS 6.6.0
  DEPENDS Qt6::QmlDomPrivate Qt6::QmlCompilerPrivate
  SOURCES
    qmldocumentparser-test.cpp
    qmltypesparser-test.cpp
)
extend_qtc_test(unittest
  CONDITION TARGET Qt6::QmlDomPrivate AND TARGET Qt6::QmlCompilerPrivate AND Qt6_VERSION VERSION_GREATER_EQUAL 6.5.0 AND Qt6_VERSION VERSION_LESS 6.6.0
  SOURCES_PREFIX "${QmlDesignerDir}/designercore"
  DEPENDS Qt6::QmlDomPrivate Qt6::QmlCompilerPrivate
  DEFINES QDS_BUILD_QMLPARSER
  SOURCES
    projectstorage/qmldocumentparser.cpp projectstorage/qmldocumentparser.h
    projectstorage/qmltypesparser.cpp projectstorage/qmltypesparser.h
)

unittest_copy_data_folder()
