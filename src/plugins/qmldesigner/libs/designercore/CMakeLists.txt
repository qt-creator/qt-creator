if (IS_SUPPORTED_PROJECTSTORAGE_QT)
  find_package(Qt6 COMPONENTS QmlDomPrivate QmlCompiler REQUIRED)
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
  PUBLIC_DEPENDS
    QmlPuppetCommunication
    QmlDesignerUtils
    Sqlite
  DEFINES
     $<$<BOOL:${USE_PROJECTSTORAGE}>:QDS_USE_PROJECTSTORAGE>
     $<$<BOOL:${QTC_USE_QML_DESIGNER_LITE}>:QTC_USE_QML_DESIGNER_LITE>
     $<$<BOOL:${DETACH_DISABLED_VIEWS}>:DETACH_DISABLED_VIEWS>
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
  CONDITION ENABLE_PROJECT_STORAGE_TRACING OR ENABLE_SOURCE_PATH_STORAGE_TRACING OR ENABLE_IMAGE_CACHE_TRACING OR ENABLE_MODEL_TRACING OR ENABLE_METAINFO_TRACING
  PUBLIC_DEPENDS Nanotrace
  PUBLIC_DEFINES
    ENABLE_QMLDESIGNER_TRACING
  DEFINES
    $<$<BOOL:${ENABLE_PROJECT_STORAGE_TRACING}>:ENABLE_PROJECT_STORAGE_TRACING>
    $<$<BOOL:${ENABLE_SOURCE_PATH_STORAGE_TRACING}>:ENABLE_SOURCE_PATH_STORAGE_TRACING>
    $<$<BOOL:${ENABLE_IMAGE_CACHE_TRACING}>:ENABLE_IMAGE_CACHE_TRACING>
    $<$<BOOL:${ENABLE_MODEL_TRACING}>:ENABLE_MODEL_TRACING>
    $<$<BOOL:${ENABLE_METAINFO_TRACING}>:ENABLE_METAINFO_TRACING>
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
    invalididexception.cpp
    invalidmetainfoexception.cpp
    invalidmodelnodeexception.cpp
    invalidmodelstateexception.cpp
    invalidpropertyexception.cpp
    invalidqmlsourceexception.cpp
    invalidreparentingexception.cpp
    invalidslideindexexception.cpp
    notimplementedexception.cpp
    removebasestateexception.cpp
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
  CONDITION NOT USE_PROJECTSTORAGE
  SOURCES_PREFIX include
  SOURCES
    itemlibraryinfo.h
    metainforeader.h
    subcomponentmanager.h
    metainfo.h
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
    invalididexception.h
    invalidmetainfoexception.h
    invalidmodelnodeexception.h
    invalidmodelstateexception.h
    invalidpropertyexception.h
    invalidqmlsourceexception.h
    invalidreparentingexception.h
    invalidslideindexexception.h
    iwidgetplugin.h
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
    notimplementedexception.h
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
    removebasestateexception.h
    rewritertransaction.h
    rewritingexception.h
    signalhandlerproperty.h
    sourcepathids.h
    stringutils.h
    synchronousimagecache.h
    variantproperty.h
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
  CONDITION NOT USE_PROJECTSTORAGE
  SOURCES_PREFIX metainfo
  SOURCES
    itemlibraryinfo.cpp
    metainfo.cpp
    metainforeader.cpp
    subcomponentmanager.cpp
)

extend_qtc_library(QmlDesignerCore
  INCLUDES model
  SOURCES_PREFIX model
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
  INCLUDES rewriter
  SOURCES_PREFIX rewriter
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
    rewriterview.cpp
    textmodifier.cpp
    texttomodelmerger.cpp
    texttomodelmerger.h
)

extend_qtc_library(QmlDesignerCore
  INCLUDES pluginmanager
  SOURCES_PREFIX pluginmanager
  SOURCES
    widgetpluginmanager.cpp
    widgetpluginmanager.h
    widgetpluginpath.cpp
    widgetpluginpath.h
)

extend_qtc_library(QmlDesignerCore
  SOURCES_PREFIX sourcepathstorage
  PUBLIC_INCLUDES sourcepathstorage
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

extend_qtc_library(QmlDesignerCore
  SOURCES_PREFIX projectstorage
  PUBLIC_INCLUDES projectstorage
  SOURCES_PROPERTIES SKIP_AUTOGEN ON
  SOURCES
    commontypecache.h
    directorypathcompressor.h
    filesysteminterface.h
    filesystem.cpp filesystem.h
    filestatus.h
    filestatuscache.cpp filestatuscache.h
    modulescanner.cpp modulescanner.h
    projectstorageexceptions.cpp projectstorageexceptions.h
    projectstorageinterface.h
    projectstoragefwd.h
    projectstorageinfotypes.h
    projectstorageobserver.h
    projectstoragepathwatcher.h
    projectstoragepathwatcherinterface.h
    projectstoragepathwatchernotifierinterface.h
    projectstoragepathwatcher.h
    projectstoragepathwatchertypes.h
    projectstorageprinting.h
    projectstoragetypes.h
    projectstorageupdater.cpp projectstorageupdater.h
    projectstorage.cpp projectstorage.h
    projectstorageerrornotifierinterface.h
    projectstorageerrornotifier.cpp projectstorageerrornotifier.h
    typeannotationreader.cpp typeannotationreader.h
    qmldocumentparserinterface.h
    qmltypesparserinterface.h
    qmltypesparser.cpp qmltypesparser.h
    qmldocumentparser.cpp qmldocumentparser.h
)
