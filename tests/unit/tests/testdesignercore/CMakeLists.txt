add_qtc_library(TestDesignerCore OBJECT
  CONDITION TARGET TextEditorSupport
  EXCLUDE_FROM_INSTALL
  PUBLIC_INCLUDES ${CMAKE_CURRENT_LIST_DIR}
  PROPERTIES SKIP_AUTOGEN ON
  DEPENDS
    Utils
    Qt::Widgets
    Qt::Qml
    Qt::QmlPrivate
    Qt6::QmlDomPrivate
    Qt6::QmlCompilerPrivate
    QmlJS
  PUBLIC_DEPENDS
    Threads::Threads
    Qt::Quick
    QmlPuppetCommunication
    QmlDesignerUtils
    Sqlite
    QmlDesignerSettings
    TextEditorSupport
  SOURCES_PREFIX ${QML_DESIGNER_DIRECTORY}/libs/designercore
  PUBLIC_INCLUDES
    ${UNITTEST_STUB_DIRECTORY}
    ${UNITTEST_STUB_DIRECTORY}/qmldesigner/libs/designercore/include
    ${QT_CREATOR_LIBS_DIRECTORY}
    ${QT_CREATOR_PLUGINS_DIRECTORY}
    ${QML_DESIGNER_DIRECTORY}
    ${QML_DESIGNER_DIRECTORY}/libs
    ${QML_DESIGNER_CORE_DIRECTORY}
    ${QML_DESIGNER_CORE_DIRECTORY}/include
    ${QML_DESIGNER_CORE_DIRECTORY}/imagecache
    ${QML_DESIGNER_CORE_DIRECTORY}/designercoreutils
  PUBLIC_DEFINES
    UNIT_TESTS
    DONT_CHECK_MESSAGE_COUNTER
    QTC_RESOURCE_DIR="${QtCreatorResourcesDir}"
    QDS_MODEL_USE_PROJECTSTORAGEINTERFACE
    QDS_USE_PROJECTSTORAGE
    QMLDESIGNERCORE_STATIC_LIBRARY
    QMLDESIGNER_STATIC_LIBRARY
)

extend_qtc_library(TestDesignerCore
  CONDITION IS_SUPPORTED_PROJECTSTORAGE_QT
  PUBLIC_DEFINES QDS_BUILD_QMLPARSER
)

extend_qtc_library(TestDesignerCore
  PUBLIC_INCLUDES ${QML_DESIGNER_CORE_DIRECTORY}/designercoreutils
  SOURCES_PREFIX ${QML_DESIGNER_CORE_DIRECTORY}/designercoreutils
  SOURCES
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

extend_qtc_library(TestDesignerCore
  INCLUDES ${QML_DESIGNER_CORE_DIRECTORY}/exceptions
  SOURCES_PREFIX ${QML_DESIGNER_CORE_DIRECTORY}/exceptions
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

extend_qtc_library(TestDesignerCore
  INCLUDES ${QML_DESIGNER_CORE_DIRECTORY}/filemanager
  SOURCES_PREFIX ${QML_DESIGNER_CORE_DIRECTORY}/filemanager
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

extend_qtc_library(TestDesignerCore
  INCLUDES ${QML_DESIGNER_CORE_DIRECTORY}/imagecache
  SOURCES_PREFIX ${QML_DESIGNER_CORE_DIRECTORY}/imagecache
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

extend_qtc_library(TestDesignerCore
  SOURCES_PREFIX ${QML_DESIGNER_CORE_DIRECTORY}/tracing
  SOURCES
    qmldesignertracing.cpp qmldesignertracing.h
)

extend_qtc_library(TestDesignerCore
  SOURCES_PREFIX ${QML_DESIGNER_CORE_DIRECTORY}/include
  SOURCES
    abstractview.h
    bytearraymodifier.h
    componenttextmodifier.h
    forwardview.h
    itemlibraryentry.h
    model.h
    nodehints.h
    plaintexteditmodifier.h
    propertyparser.h
    rewriterview.h
    textmodifier.h
)

extend_qtc_library(TestDesignerCore
  SOURCES_PROPERTIES SKIP_AUTOGEN ON
  SOURCES_PREFIX ${QML_DESIGNER_CORE_DIRECTORY}/include
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
    customnotificationpackage.h
    customnotifications.h
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

extend_qtc_library(TestDesignerCore
  INCLUDES
    metainfo
  SOURCES_PREFIX ${QML_DESIGNER_CORE_DIRECTORY}/metainfo
  DEFINES SHARE_QML_PATH="${CMAKE_CURRENT_SOURCE_DIR}/../../../../../share/qtcreator/qmldesigner"
  SOURCES
    itemlibraryentry.cpp
    nodehints.cpp
    nodemetainfo.cpp
)

extend_qtc_library(TestDesignerCore
  INCLUDES ${QML_DESIGNER_CORE_DIRECTORY}/model
  SOURCES_PREFIX ${QML_DESIGNER_CORE_DIRECTORY}/model
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

extend_qtc_library(TestDesignerCore
  INCLUDES ${QML_DESIGNER_CORE_DIRECTORY}/rewriter
  SOURCES_PREFIX ${QML_DESIGNER_CORE_DIRECTORY}/rewriter
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

extend_qtc_library(TestDesignerCore
  INCLUDES ${QML_DESIGNER_CORE_DIRECTORY}/pluginmanager
  SOURCES_PREFIX ${QML_DESIGNER_CORE_DIRECTORY}/pluginmanager
  SOURCES
    widgetpluginmanager.cpp
    widgetpluginmanager.h
    widgetpluginpath.cpp
    widgetpluginpath.h
)

extend_qtc_library(TestDesignerCore
  SOURCES_PREFIX ${QML_DESIGNER_CORE_DIRECTORY}/sourcepathstorage
  PUBLIC_INCLUDES ${QML_DESIGNER_CORE_DIRECTORY}/sourcepathstorage
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

extend_qtc_library(TestDesignerCore
  SOURCES_PREFIX ${QML_DESIGNER_CORE_DIRECTORY}/projectstorage
  PUBLIC_INCLUDES ${QML_DESIGNER_CORE_DIRECTORY}/projectstorage
  SOURCES_PROPERTIES SKIP_AUTOGEN ON
  DEFINES QML_DOM_MSVC2019_COMPAT # can be removed for Qt 6.8
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
)
