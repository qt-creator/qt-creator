add_qtc_library(TestDesignerCore OBJECT
  EXCLUDE_FROM_INSTALL
  PUBLIC_INCLUDES ${CMAKE_CURRENT_LIST_DIR}
  PROPERTIES SKIP_AUTOGEN ON
  DEPENDS
    Qt::Core Qt::Network Qt::Widgets
    Qt::Xml Qt::Concurrent Qt::QmlPrivate Qt::Gui
    Qt::Core5Compat Utils QmlJS Sqlite
  PUBLIC_DEPENDS
    QmlPuppetCommunication
  SOURCES_PREFIX ${QmlDesignerDir}/designercore
  PUBLIC_INCLUDES
    ${UnittestStubsDir}
    ${UnittestStubsDir}/qmldesigner/designercore/include
    ${QtCreatorLibsDir}
    ${QtCreatorPluginsDir}
    ${QmlDesignerDir}
    ${QmlDesignerDir}/designercore
    ${QmlDesignerDir}/designercore/include
    ${QmlDesignerDir}/designercore/imagecache
  PUBLIC_DEFINES
    UNIT_TESTS
    DONT_CHECK_MESSAGE_COUNTER
    QTC_RESOURCE_DIR="${QtCreatorResourcesDir}"
    QDS_MODEL_USE_PROJECTSTORAGEINTERFACE
    QDS_USE_PROJECTSTORAGE
    QMLDESIGNERCORE_STATIC_LIBRARY
    QMLDESIGNER_STATIC_LIBRARY
  SOURCES
    exceptions/exception.cpp
    exceptions/invalidargumentexception.cpp
    exceptions/invalididexception.cpp
    exceptions/invalidmetainfoexception.cpp
    exceptions/invalidmodelnodeexception.cpp
    exceptions/invalidmodelstateexception.cpp
    exceptions/invalidpropertyexception.cpp
    exceptions/invalidqmlsourceexception.cpp
    exceptions/invalidreparentingexception.cpp
    exceptions/invalidslideindexexception.cpp
    exceptions/notimplementedexception.cpp
    exceptions/removebasestateexception.cpp
    exceptions/rewritingexception.cpp
    imagecache/asynchronousexplicitimagecache.cpp
    imagecache/asynchronousimagecache.cpp
    imagecache/asynchronousimagefactory.cpp
    imagecache/asynchronousimagefactory.h
    imagecache/imagecachecollectorinterface.h
    imagecache/imagecachegenerator.cpp
    imagecache/imagecachegenerator.h
    imagecache/imagecachegeneratorinterface.h
    imagecache/imagecachestorage.h
    imagecache/imagecachedispatchcollector.h
    imagecache/imagecachestorageinterface.h
    imagecache/synchronousimagecache.cpp
    imagecache/taskqueue.h
    imagecache/timestampproviderinterface.h
    include/abstractproperty.h
    include/asynchronousexplicitimagecache.h
    include/asynchronousimagecache.h
    include/asynchronousimagecacheinterface.h
    include/bindingproperty.h
    include/imagecacheauxiliarydata.h
    include/import.h
    include/itemlibraryinfo.h
    include/metainfo.h
    include/metainforeader.h
    include/modelnode.h
    include/module.h
    include/nodeabstractproperty.h
    include/nodelistproperty.h
    include/nodemetainfo.h
    include/nodeproperty.h
    include/projectstorageids.h
    include/propertymetainfo.h
    include/propertycontainer.h
    include/propertyparser.h
    include/qmldesignercorelib_global.h
    include/signalhandlerproperty.h
    include/synchronousimagecache.h
    include/variantproperty.h
    metainfo/itemlibraryinfo.cpp
    metainfo/metainfo.cpp
    metainfo/metainforeader.cpp
    metainfo/nodemetainfo.cpp
    model/abstractproperty.cpp
    model/abstractview.cpp
    model/annotation.cpp
    model/bindingproperty.cpp
    model/import.cpp
    model/internalbindingproperty.cpp
    model/internalbindingproperty.h
    model/internalnode.cpp
    model/internalnode_p.h
    model/internalnodeabstractproperty.cpp
    model/internalnodeabstractproperty.h
    model/internalnodelistproperty.cpp
    model/internalnodelistproperty.h
    model/internalnodeproperty.cpp
    model/internalnodeproperty.h
    model/internalproperty.cpp
    model/internalproperty.h
    model/internalsignalhandlerproperty.cpp
    model/internalsignalhandlerproperty.h
    model/internalvariantproperty.cpp
    model/internalvariantproperty.h
    model/model.cpp
    model/model_p.h
    model/modelnode.cpp
    model/modelresourcemanagementinterface.h
    model/modelresourcemanagement.cpp model/modelresourcemanagement.h
    model/modelutils.cpp model/modelutils.h
    model/propertycontainer.cpp
    model/propertyparser.cpp
    model/nodeabstractproperty.cpp
    model/nodelistproperty.cpp
    model/nodeproperty.cpp
    model/signalhandlerproperty.cpp
    model/variantproperty.cpp
    pluginmanager/widgetpluginmanager.h pluginmanager/widgetpluginmanager.cpp
    pluginmanager/widgetpluginpath.h pluginmanager/widgetpluginpath.cpp
    projectstorage/directorypathcompressor.h
    projectstorage/filesysteminterface.h
    projectstorage/filesystem.cpp projectstorage/filesystem.h
    projectstorage/filestatus.h
    projectstorage/filestatuscache.cpp projectstorage/filestatuscache.h
    projectstorage/modulescanner.cpp projectstorage/modulescanner.h
    projectstorage/nonlockingmutex.h
    projectstorage/projectstorageexceptions.cpp projectstorage/projectstorageexceptions.h
    projectstorage/projectstorageinterface.h
    projectstorage/projectstorage.cpp projectstorage/projectstorage.h
    projectstorage/projectstoragepathwatcher.h
    projectstorage/projectstoragepathwatcherinterface.h
    projectstorage/projectstoragepathwatchernotifierinterface.h
    projectstorage/projectstoragepathwatcher.h
    projectstorage/projectstoragepathwatchertypes.h
    projectstorage/projectstoragetypes.h
    projectstorage/projectstorageupdater.cpp projectstorage/projectstorageupdater.h
    projectstorage/sourcepath.h
    projectstorage/sourcepathcache.h
    projectstorage/sourcepathcache.h
    projectstorage/sourcepathcachetypes.h
    projectstorage/sourcepathview.h
    projectstorage/storagecache.h
    projectstorage/storagecacheentry.h
    projectstorage/storagecachefwd.h
    projectstorage/qmldocumentparserinterface.h
    projectstorage/qmltypesparserinterface.h
    rewritertransaction.cpp
    rewritertransaction.h
)

extend_qtc_library(TestDesignerCore
  SOURCES_PREFIX ${QmlDesignerDir}/designercore/include
  SOURCES_PROPERTIES AUTOMOC ON
  SOURCES
    model.h
    abstractview.h
)

extend_qtc_library(TestDesignerCore
  SOURCES_PROPERTIES AUTOMOC ON
  SOURCES_PREFIX ../stubs/qmldesigner/designercore/include
  SOURCES
    nodeinstanceview.h
    rewriterview.h
)
