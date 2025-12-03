if (IS_SUPPORTED_PROJECTSTORAGE_QT)
  find_package(Qt6 COMPONENTS QmlDomPrivate QmlCompiler REQUIRED)
  if (Qt6_VERSION VERSION_GREATER_EQUAL 6.9.0)
    find_package(Qt6 COMPONENTS QmlCompilerPrivate REQUIRED)
  endif()
endif()

add_qtc_library(QmlDesignerCore
  PROPERTIES SKIP_AUTOUIC ON
  DEPENDS
    Threads::Threads
    Utils
    Qt::Widgets
    Qt::Qml
    Qt::QmlPrivate
    Qt::Quick
    QmlJS
    TextEditorSupport
    QmlDesignerSettings
    AstCheck
    QmlDesignerUtils
  PUBLIC_DEPENDS
    QmlPuppetCommunication
    Sqlite
  DEFINES
     $<$<BOOL:${QTC_USE_QML_DESIGNER_LITE}>:QTC_USE_QML_DESIGNER_LITE>
  PUBLIC_INCLUDES
    ${CMAKE_CURRENT_LIST_DIR}
    include
)

extend_qtc_library(QmlDesignerCore
  PUBLIC_INCLUDES designercoreutils
  SOURCES_PREFIX designercoreutils
  SOURCES
    functional.h
    generatedcomponentutils.cpp
    generatedcomponentutils.h
    modelmerger.cpp
    modelmerger.h
    modelutils.cpp
    modelutils.h
    predicate.h
    stylesheetmerger.cpp
    stylesheetmerger.h
    uniquename.cpp
    uniquename.h
)

extend_qtc_library(QmlDesignerCore
  CONDITION ENABLE_PROJECT_STORAGE_TRACING OR ENABLE_SOURCE_PATH_STORAGE_TRACING OR ENABLE_IMAGE_CACHE_TRACING OR ENABLE_MODEL_TRACING OR ENABLE_FORM_EDITOR_TRACING OR ENABLE_PROPERTY_EDITOR_TRACING OR ENABLE_NAVIGATOR_TRACING OR ENABLE_ITEM_LIBRARY_TRACING OR ENABLE_REWRITER_TRACING OR ENABLE_PROJECT_MANAGING_TRACING OR ENABLE_COMPONENT_CORE_TRACING OR ENABLE_NODE_INSTANCE_TRACING
  PUBLIC_DEPENDS Nanotrace
  PUBLIC_DEFINES
    ENABLE_QMLDESIGNER_TRACING
)

extend_qtc_library(QmlDesignerCore
    CONDITION ENABLE_COMPILE_WARNING_AS_ERROR
    PROPERTIES COMPILE_WARNING_AS_ERROR ON
    PUBLIC_COMPILE_OPTIONS $<$<COMPILE_LANG_AND_ID:CXX,Clang,GNU>:-Wno-error=maybe-uninitialized>
)

extend_qtc_library(QmlDesignerCore
  CONDITION IS_SUPPORTED_PROJECTSTORAGE_QT
  DEPENDS Qt6::QmlDomPrivate Qt6::QmlCompilerPrivate
  PUBLIC_DEFINES QDS_BUILD_QMLPARSER
)
extend_qtc_library(QmlDesignerCore
  CONDITION UNIX AND NOT APPLE
  PUBLIC_DEPENDS rt
)

extend_qtc_library(QmlDesignerCore
  INCLUDES exceptions
  SOURCES_PREFIX exceptions
  SOURCES
    exception.cpp
    invalidargumentexception.cpp
    rewritingexception.cpp
)

extend_qtc_library(QmlDesignerCore
  INCLUDES filemanager
  SOURCES_PREFIX filemanager
  SOURCES
    addarraymembervisitor.cpp
    addarraymembervisitor.h
    addobjectvisitor.cpp
    addobjectvisitor.h
    addpropertyvisitor.cpp
    addpropertyvisitor.h
    astobjecttextextractor.cpp
    astobjecttextextractor.h
    changeimportsvisitor.cpp
    changeimportsvisitor.h
    changeobjecttypevisitor.cpp
    changeobjecttypevisitor.h
    changepropertyvisitor.cpp
    changepropertyvisitor.h
    firstdefinitionfinder.cpp
    firstdefinitionfinder.h
    moveobjectbeforeobjectvisitor.cpp
    moveobjectbeforeobjectvisitor.h
    moveobjectvisitor.cpp
    moveobjectvisitor.h
    objectlengthcalculator.cpp
    objectlengthcalculator.h
    qmlrefactoring.cpp
    qmlrefactoring.h
    qmlrewriter.cpp
    qmlrewriter.h
    removepropertyvisitor.cpp
    removepropertyvisitor.h
    removeuiobjectmembervisitor.cpp
    removeuiobjectmembervisitor.h
)

extend_qtc_library(QmlDesignerCore
  INCLUDES imagecache
  SOURCES_PREFIX imagecache
  PUBLIC_DEFINES
    $<$<BOOL:${ENABLE_IMAGE_CACHE_TRACING}>:ENABLE_IMAGE_CACHE_TRACING>
  SOURCES
    asynchronousexplicitimagecache.cpp
    asynchronousimagecache.cpp
    asynchronousimagefactory.cpp
    asynchronousimagefactory.h
    explicitimagecacheimageprovider.cpp
    explicitimagecacheimageprovider.h
    imagecachecollectorinterface.h
    imagecachedispatchcollector.h
    imagecachegenerator.cpp
    imagecachegenerator.h
    imagecachegeneratorinterface.h
    imagecacheimageresponse.cpp
    imagecacheimageresponse.h
    imagecachestorage.h
    imagecachestorageinterface.h
    midsizeimagecacheprovider.cpp
    midsizeimagecacheprovider.h
    smallimagecacheprovider.cpp
    smallimagecacheprovider.h
    synchronousimagecache.cpp
    taskqueue.h
    timestampprovider.cpp
    timestampprovider.h
    timestampproviderinterface.h
)

extend_qtc_library(QmlDesignerCore
  SOURCES_PREFIX tracing
  PUBLIC_DEFINES
    $<$<BOOL:${ENABLE_PROJECT_MANAGING_TRACING}>:ENABLE_PROJECT_MANAGING_TRACING>
  SOURCES
    qmldesignertracing.cpp qmldesignertracing.h
)

extend_qtc_library(QmlDesignerCore
  SOURCES_PREFIX include
  SOURCES
    abstractview.h
    bytearraymodifier.h
    componenttextmodifier.h
    designercoretr.h
    forwardview.h
    itemlibraryentry.h
    model.h
    nodehints.h
    plaintexteditmodifier.h
    propertyparser.h
    rewriterview.h
    textmodifier.h
)

extend_qtc_library(QmlDesignerCore
  SOURCES_PROPERTIES SKIP_AUTOGEN ON
  SOURCES_PREFIX include
  SOURCES
    abstractproperty.h
    annotation.h
    asynchronousexplicitimagecache.h
    asynchronousimagecache.h
    asynchronousimagecacheinterface.h
    auxiliarydata.h
    auxiliarydataproperties.h
    bindingproperty.h
    bytearraymodifier.h
    customnotifications.h
    customnotificationpackage.h
    documentmessage.h
    enumerationmetainfo.h
    exception.h
    externaldependenciesinterface.h
    imagecacheauxiliarydata.h
    import.h
    invalidargumentexception.h
    mathutils.h
    modelfwd.h
    modelnode.h
    modificationgroupexception.h
    modificationgrouptoken.h
    module.h
    nodeabstractproperty.h
    nodelistproperty.h
    nodemetainfo.h
    nodeproperty.h
    objectpropertybinding.h
    projectstorageids.h
    propertybinding.h
    propertycontainer.h
    propertymetainfo.h
    propertynode.h
    puppetstartdata.h
    qmldesignercorelib_exports.h
    qmldesignercorelib_global.h
    qmldesignercoreconstants.h
    rewritertransaction.h
    rewritingexception.h
    signalhandlerproperty.h
    sourcepathids.h
    synchronousimagecache.h
    variantproperty.h
    widgetregistration.h
    widgetinfo.h
)

extend_qtc_library(QmlDesignerCore
  INCLUDES
    metainfo
  SOURCES_PREFIX metainfo
  DEFINES SHARE_QML_PATH="${CMAKE_CURRENT_SOURCE_DIR}/../../../../../share/qtcreator/qmldesigner"
  SOURCES
    itemlibraryentry.cpp
    nodehints.cpp
    nodemetainfo.cpp
)

extend_qtc_library(QmlDesignerCore
  INCLUDES model
  SOURCES_PREFIX model
  PUBLIC_DEFINES
    $<$<BOOL:${ENABLE_MODEL_TRACING}>:ENABLE_MODEL_TRACING>
  SOURCES
    abstractproperty.cpp
    abstractview.cpp
    annotation.cpp
    auxiliarypropertystorageview.cpp auxiliarypropertystorageview.h
    bindingproperty.cpp
    documentmessage.cpp
    import.cpp
    internalbindingproperty.cpp
    internalbindingproperty.h
    internalnode.cpp
    internalnode_p.h
    internalnodeabstractproperty.cpp
    internalnodeabstractproperty.h
    internalnodelistproperty.cpp
    internalnodelistproperty.h
    internalnodeproperty.cpp
    internalnodeproperty.h
    internalproperty.cpp
    internalproperty.h
    internalsignalhandlerproperty.cpp
    internalsignalhandlerproperty.h
    internalvariantproperty.cpp
    internalvariantproperty.h
    model.cpp
    model_p.h
    modelnode.cpp
    modelresourcemanagementinterface.h
    modelresourcemanagementfwd.h
    modelresourcemanagement.cpp modelresourcemanagement.h
    nodeabstractproperty.cpp
    nodelistproperty.cpp
    nodeproperty.cpp
    signalhandlerproperty.cpp
    variantproperty.cpp
)

extend_qtc_library(QmlDesignerCore
  SOURCES_PREFIX modulesstorage
  SOURCES
    modulesstorage.cpp
    modulesstorage.h
    modulesstorageinterface.h
)

extend_qtc_library(QmlDesignerCore
  INCLUDES rewriter
  SOURCES_PREFIX rewriter
  PUBLIC_DEFINES
    $<$<BOOL:${ENABLE_REWRITER_TRACING}>:ENABLE_REWRITER_TRACING>
  SOURCES
    componenttextmodifier.cpp
    modelnodepositionrecalculator.cpp
    modelnodepositionrecalculator.h
    modelnodepositionstorage.cpp
    modelnodepositionstorage.h
    modeltotextmerger.cpp
    modeltotextmerger.h
    plaintexteditmodifier.cpp
    propertycontainer.cpp
    propertynode.cpp
    propertyparser.cpp
    qmltextgenerator.cpp
    qmltextgenerator.h
    rewriteaction.cpp
    rewriteaction.h
    rewriteactioncompressor.cpp
    rewriteactioncompressor.h
    rewritertransaction.cpp
    rewritertracing.h
    rewritertracing.cpp
    rewriterview.cpp
    textmodifier.cpp
    texttomodelmerger.cpp
    texttomodelmerger.h
)

extend_qtc_library(QmlDesignerCore
  SOURCES_PREFIX sourcepathstorage
  PUBLIC_INCLUDES sourcepathstorage
  PUBLIC_DEFINES
    $<$<BOOL:${ENABLE_SOURCE_PATH_STORAGE_TRACING}>:ENABLE_SOURCE_PATH_STORAGE_TRACING>
  SOURCES_PROPERTIES SKIP_AUTOGEN ON
  SOURCES
    nonlockingmutex.h
    sourcepath.h
    sourcepathcache.h
    sourcepathcacheinterface.h
    sourcepathcachetypes.h
    sourcepathexceptions.cpp
    sourcepathexceptions.h
    sourcepathstorage.cpp
    sourcepathstorage.h
    sourcepathstoragetracing.cpp
    sourcepathstoragetracing.h
    sourcepathview.h
    storagecache.h
    storagecacheentry.h
    storagecachefwd.h
)

extend_qtc_library(QmlDesignerCore
    CONDITION QT_BUILT_WITH_MSVC2019
    DEFINES QML_DOM_MSVC2019_COMPAT
)

extend_qtc_library(QmlDesignerCore
    CONDITION WITH_TESTS
    PUBLIC_DEFINES QDS_MODEL_USE_PROJECTSTORAGEINTERFACE
)

include(FetchContent)

FetchContent_Declare(
  efsw
  GIT_REPOSITORY https://github.com/SpartanJ/efsw.git
  GIT_TAG        1.5.0
)

FetchContent_MakeAvailable(efsw)

# Silence efsw warnings for all possible targets
foreach(t IN ITEMS efsw efsw-static efsw_shared)
  if(TARGET ${t})
    target_compile_options(${t} PRIVATE
      $<$<OR:$<COMPILE_LANG_AND_ID:C,Clang,AppleClang>,$<COMPILE_LANG_AND_ID:CXX,Clang,AppleClang>>:
        -Wno-unused-parameter -Wno-unused-variable -Wno-unused-but-set-variable
        $<$<COMPILE_LANGUAGE:CXX>:-Wno-deprecated-copy-with-user-provided-copy>>
      $<$<OR:$<COMPILE_LANG_AND_ID:C,GNU>,$<COMPILE_LANG_AND_ID:CXX,GNU>>:
        -Wno-unused-parameter -Wno-unused-variable -Wno-unused-but-set-variable>
      $<$<OR:$<COMPILE_LANG_AND_ID:C,MSVC>,$<COMPILE_LANG_AND_ID:CXX,MSVC>>:
        /wd4100 /wd4189>
    )
  endif()
endforeach()

extend_qtc_library(QmlDesignerCore
  SOURCES_PREFIX projectstorage
  DEPENDS efsw-static
  PUBLIC_INCLUDES projectstorage
  SOURCES
    efswfilesystemwatcher.cpp efswfilesystemwatcher.h
)

extend_qtc_library(QmlDesignerCore
  SOURCES_PREFIX projectstorage
  PUBLIC_INCLUDES projectstorage
  PUBLIC_DEFINES
    $<$<BOOL:${ENABLE_PROJECT_STORAGE_TRACING}>:ENABLE_PROJECT_STORAGE_TRACING>
  SOURCES_PROPERTIES SKIP_AUTOGEN ON
  SOURCES
    commontypecache.h
    directorypathcompressor.h
    filesysteminterface.h
    filesystem.cpp filesystem.h
    filestatus.h
    filestatuscache.cpp filestatuscache.h
    projectstorageexceptions.cpp projectstorageexceptions.h
    projectstorageinterface.h
    projectstoragefwd.h
    projectstorageinfotypes.h
    projectstorageobserver.h
    projectstoragepathwatcher.h
    projectstoragetriggerupdateinterface.h
    projectstoragepathwatcherinterface.h
    projectstoragepathwatchernotifierinterface.h
    projectstoragepathwatcher.h
    projectstoragepathwatchertypes.h
    projectstorageprinting.h
    projectstoragetracing.cpp projectstoragetracing.h
    projectstoragetypes.h
    projectstorageupdater.cpp projectstorageupdater.h
    projectstorage.cpp projectstorage.h
    projectstorageerrornotifierinterface.h
    typeannotationreader.cpp typeannotationreader.h
    qmldocumentparserinterface.h
    qmltypesparserinterface.h
    qmltypesparser.cpp qmltypesparser.h
    qmldocumentparser.cpp qmldocumentparser.h
)
