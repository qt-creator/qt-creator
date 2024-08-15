#only if the plugin is requested by qtc_plugin_enabled continue if not stop as early as possible


find_package(Qt6 COMPONENTS QmlDomPrivate QmlCompilerPrivate QUIET)

set(QmlDesignerPluginInstallPrefix "${IDE_PLUGIN_PATH}/qmldesigner")
if (APPLE)
  set(QmlDesignerPluginInstallPrefix "${IDE_PLUGIN_PATH}/QmlDesigner")
endif()

set(BUILD_NOT_DESIGNSTUDIO NOT ${BUILD_NOT_DESIGNSTUDIO})
option(QTC_USE_QML_DESIGNER_LITE "Use Qml Designer Lite" ${BUILD_NOT_DESIGNSTUDIO})
add_feature_info("Qml Designer Lite" ${QTC_USE_QML_DESIGNER_LITE} "")

option(USE_PROJECTSTORAGE "Use ProjectStorage" ${QTC_USE_QML_DESIGNER_LITE})


env_with_default("QTC_ENABLE_PROJECT_STORAGE_TRACING" ENV_QTC_ENABLE_PROJECT_STORAGE_TRACING OFF)
option(ENABLE_PROJECT_STORAGE_TRACING "Enable project storage tracing" ${ENV_QTC_ENABLE_PROJECT_STORAGE_TRACING})
add_feature_info("Project storage tracing" ${ENABLE_PROJECT_STORAGE_TRACING} "")

env_with_default("QTC_ENABLE_SOURCE_PATH_STORAGE_TRACING" ENV_QTC_ENABLE_SOURCE_PATH_STORAGE_TRACING OFF)
option(ENABLE_SOURCE_PATH_STORAGE_TRACING "Enable source path storage tracing" ${ENV_QTC_ENABLE_SOURCE_PATH_STORAGE_TRACING})
add_feature_info("Source path storage tracing" ${ENABLE_SOURCE_PATH_STORAGE_TRACING} "")

env_with_default("QTC_ENABLE_IMAGE_CACHE_TRACING" ENV_QTC_ENABLE_IMAGE_CACHE_TRACING OFF)
option(ENABLE_IMAGE_CACHE_TRACING "Enable image cache tracing" ${ENV_QTC_ENABLE_IMAGE_CACHE_TRACING})
add_feature_info("Image cache tracing" ${ENABLE_IMAGE_CACHE_TRACING} "")

env_with_default("QTC_ENABLE_MODEL_TRACING" ENV_QTC_ENABLE_MODEL_TRACING OFF)
option(ENABLE_MODEL_TRACING "Enable model tracing" ${ENV_QTC_ENABLE_MODEL_TRACING})
add_feature_info("Model tracing" ${ENABLE_MODEL_TRACING} "")

env_with_default("QTC_ENABLE_METAINFO_TRACING" ENV_QTC_ENABLE_METAINFO_TRACING OFF)
option(ENABLE_METAINFO_TRACING "Enable meta info tracing" ${ENV_QTC_ENABLE_METAINFO_TRACING})
add_feature_info("Meta info tracing" ${ENABLE_METAINFO_TRACING} "")

add_qtc_library(QmlDesignerUtils STATIC
  DEPENDS
    Qt::Gui Utils Qt::QmlPrivate
  PUBLIC_INCLUDES ${CMAKE_CURRENT_LIST_DIR}/utils
  SOURCES_PREFIX ${CMAKE_CURRENT_LIST_DIR}/utils
  PUBLIC_COMPILE_OPTIONS
    $<$<COMPILE_LANG_AND_ID:CXX,Clang>:-Wno-unneeded-internal-declaration>
  SOURCES
    asset.cpp asset.h
    designeralgorithm.h
    filedownloader.cpp filedownloader.h
    multifiledownloader.cpp multifiledownloader.h
    fileextractor.cpp fileextractor.h
    hdrimage.cpp hdrimage.h
    ktximage.cpp ktximage.h
    imageutils.cpp imageutils.h
    qmldesignerutils_global.h
    version.cpp version.h
)

extend_qtc_library(QmlDesignerUtils
    CONDITION ENABLE_COMPILE_WARNING_AS_ERROR
    PROPERTIES COMPILE_WARNING_AS_ERROR ON
    PUBLIC_COMPILE_OPTIONS
      $<$<COMPILE_LANG_AND_ID:CXX,Clang,GNU>:-Wno-error=maybe-uninitialized>
)

add_qtc_library(QmlDesignerCore STATIC
  CONDITION TARGET QmlDesignerBase AND TARGET Qt6::QmlPrivate AND TARGET Qt6::QmlDomPrivate AND TARGET Qt6::QmlCompilerPrivate
  EXCLUDE_FROM_INSTALL
  PROPERTIES SKIP_AUTOUIC ON
  DEPENDS
    Threads::Threads
    Qt::CorePrivate
    CPlusPlus
    Utils
    Qt::Widgets
    Qt::Qml
    Qt::QmlPrivate
    Qt6::QmlDomPrivate
    Qt6::QmlCompilerPrivate
    Core
    ProjectExplorer
    QmakeProjectManager
    QmlJS
    QmlJSEditor
    QmlJSTools
    QmlProjectManager
    QtSupport
  PUBLIC_DEPENDS
    QmlDesignerBase
    QmlPuppetCommunication
    QmlDesignerUtils
    TextEditor
    Sqlite
  DEFINES
     $<$<BOOL:${USE_PROJECTSTORAGE}>:QDS_USE_PROJECTSTORAGE>
     $<$<BOOL:${QTC_USE_QML_DESIGNER_LITE}>:QTC_USE_QML_DESIGNER_LITE>
  INCLUDES
    ${CMAKE_CURRENT_LIST_DIR}
  PUBLIC_INCLUDES
    ${CMAKE_CURRENT_LIST_DIR}/designercore
    ${CMAKE_CURRENT_LIST_DIR}/designercore/include
)

extend_qtc_library(QmlDesignerCore
  PUBLIC_INCLUDES ${CMAKE_CURRENT_LIST_DIR}/designercore/designercoreutils
  SOURCES_PREFIX ${CMAKE_CURRENT_LIST_DIR}/designercore/designercoreutils
  SOURCES
    generatedcomponentutils.cpp generatedcomponentutils.h
    modelmerger.cpp modelmerger.h
    modelutils.cpp modelutils.h
    skipiterator.h
    stylesheetmerger.cpp stylesheetmerger.h
    uniquename.cpp uniquename.h
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
  PUBLIC_DEFINES QDS_BUILD_QMLPARSER
)
extend_qtc_library(QmlDesignerCore
  CONDITION UNIX AND NOT APPLE
  PUBLIC_DEPENDS rt
)

extend_qtc_library(QmlDesignerCore
  INCLUDES ${CMAKE_CURRENT_LIST_DIR}/designercore/exceptions
  SOURCES_PREFIX ${CMAKE_CURRENT_LIST_DIR}/designercore/exceptions
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
  INCLUDES ${CMAKE_CURRENT_LIST_DIR}/designercore/filemanager
  SOURCES_PREFIX ${CMAKE_CURRENT_LIST_DIR}/designercore/filemanager
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
  INCLUDES ${CMAKE_CURRENT_LIST_DIR}/designercore/imagecache
  SOURCES_PREFIX ${CMAKE_CURRENT_LIST_DIR}/designercore/imagecache
  SOURCES
    asynchronousexplicitimagecache.cpp
    asynchronousimagecache.cpp
    asynchronousimagefactory.cpp
    asynchronousimagefactory.h
    imagecachecollector.cpp
    imagecachecollector.h
    imagecachedispatchcollector.h
    imagecachecollectorinterface.h
    imagecacheconnectionmanager.cpp
    imagecacheconnectionmanager.h
    imagecachefontcollector.cpp
    imagecachefontcollector.h
    imagecachegenerator.cpp
    imagecachegenerator.h
    imagecachegeneratorinterface.h
    imagecachestorage.h
    imagecachestorageinterface.h
    meshimagecachecollector.cpp
    meshimagecachecollector.h
    synchronousimagecache.cpp
    taskqueue.h
    textureimagecachecollector.cpp
    textureimagecachecollector.h
    timestampprovider.cpp
    timestampprovider.h
    timestampproviderinterface.h
)

extend_qtc_library(QmlDesignerCore
  SOURCES_PREFIX ${CMAKE_CURRENT_LIST_DIR}/designercore/tracing
  SOURCES
    qmldesignertracing.cpp qmldesignertracing.h
)

extend_qtc_library(QmlDesignerCore
  SOURCES_PREFIX ${CMAKE_CURRENT_LIST_DIR}/designercore/include
  SOURCES
    abstractview.h
    basetexteditmodifier.h
    bytearraymodifier.h
    componenttextmodifier.h
    forwardview.h
    itemlibraryentry.h
    model.h
    nodehints.h
    plaintexteditmodifier.h
    nodeinstanceview.h
    propertyparser.h
    rewriterview.h
    textmodifier.h
)

extend_qtc_library(QmlDesignerCore
  CONDITION NOT USE_PROJECTSTORAGE
  SOURCES_PREFIX ${CMAKE_CURRENT_LIST_DIR}/designercore/include
  SOURCES
    itemlibraryinfo.h
    metainforeader.h
    subcomponentmanager.h
    metainfo.h
)

extend_qtc_library(QmlDesignerCore
  SOURCES_PROPERTIES SKIP_AUTOGEN ON
  SOURCES_PREFIX ${CMAKE_CURRENT_LIST_DIR}/designercore/include
  SOURCES
    abstractproperty.h
    anchorline.h
    annotation.h
    asynchronousexplicitimagecache.h
    asynchronousimagecache.h
    auxiliarydata.h
    auxiliarydataproperties.h
    bindingproperty.h
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
    mathutils.h
    modelfwd.h
    modelnode.h
    module.h
    nodeabstractproperty.h
    nodeinstance.h
    nodelistproperty.h
    nodemetainfo.h
    nodeproperty.h
    notimplementedexception.h
    propertycontainer.h
    propertymetainfo.h
    propertynode.h
    qmlanchors.h
    qmlchangeset.h
    qmlconnections.h
    qmldesignercorelib_exports.h
    qmldesignercorelib_global.h
    qmlitemnode.h
    qmlmodelnodefacade.h
    qmlobjectnode.h
    qmlstate.h
    qmltimeline.h
    qmltimelinekeyframegroup.h
    removebasestateexception.h
    rewritingexception.h
    rewritertransaction.h
    signalhandlerproperty.h
    stringutils.h
    synchronousimagecache.h
    variantproperty.h
)

extend_qtc_library(QmlDesignerCore
  INCLUDES
    ${CMAKE_CURRENT_LIST_DIR}/designercore/metainfo
  SOURCES_PREFIX ${CMAKE_CURRENT_LIST_DIR}/designercore/metainfo
  DEFINES SHARE_QML_PATH="${CMAKE_CURRENT_SOURCE_DIR}/../../../share/qtcreator/qmldesigner"
  SOURCES
    itemlibraryentry.cpp
    nodehints.cpp
    nodemetainfo.cpp
)

extend_qtc_library(QmlDesignerCore
  CONDITION NOT USE_PROJECTSTORAGE
  SOURCES_PREFIX ${CMAKE_CURRENT_LIST_DIR}/designercore/metainfo
  SOURCES
    itemlibraryinfo.cpp
    metainfo.cpp
    metainforeader.cpp
    subcomponentmanager.cpp
)


extend_qtc_library(QmlDesignerCore
  PUBLIC_INCLUDES ${CMAKE_CURRENT_LIST_DIR}/designercore/instances
  SOURCES_PREFIX ${CMAKE_CURRENT_LIST_DIR}/designercore/instances
  SOURCES
    baseconnectionmanager.cpp
    baseconnectionmanager.h
    connectionmanager.cpp
    connectionmanager.h
    connectionmanagerinterface.cpp
    connectionmanagerinterface.h
    nodeinstance.cpp
    nodeinstanceserverproxy.cpp
    nodeinstanceserverproxy.h
    nodeinstanceview.cpp
    puppetstartdata.h
    puppetstarter.cpp
    puppetstarter.h
    qprocessuniqueptr.h
)

extend_qtc_library(QmlDesignerCore
  INCLUDES ${CMAKE_CURRENT_LIST_DIR}/designercore/model
  SOURCES_PREFIX ${CMAKE_CURRENT_LIST_DIR}/designercore/model
  SOURCES
    abstractproperty.cpp
    abstractview.cpp
    anchorline.cpp
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
  INCLUDES ${CMAKE_CURRENT_LIST_DIR}/designercore/rewriter
  SOURCES_PREFIX ${CMAKE_CURRENT_LIST_DIR}/designercore/rewriter
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
  INCLUDES ${CMAKE_CURRENT_LIST_DIR}/designercore/qmltools
  SOURCES_PREFIX ${CMAKE_CURRENT_LIST_DIR}/designercore/qmltools
  SOURCES
    qml3dnode.cpp
    qmlanchors.cpp
    qmlchangeset.cpp
    qmlconnections.cpp
    qmlitemnode.cpp
    qmlmodelnodefacade.cpp
    qmlobjectnode.cpp
    qmlstate.cpp
    qmltextgenerator.cpp
    qmltextgenerator.h
    qmltimeline.cpp
    qmltimelinekeyframegroup.cpp
    qmlvisualnode.cpp
)

extend_qtc_library(QmlDesignerCore
  INCLUDES ${CMAKE_CURRENT_LIST_DIR}/designercore/pluginmanager
  SOURCES_PREFIX ${CMAKE_CURRENT_LIST_DIR}/designercore/pluginmanager
  SOURCES
    widgetpluginmanager.cpp
    widgetpluginmanager.h
    widgetpluginpath.cpp
    widgetpluginpath.h
)

extend_qtc_library(QmlDesignerCore
  SOURCES_PREFIX designercore/sourcepathstorage
  PUBLIC_INCLUDES designercore/sourcepathstorage
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
  SOURCES_PREFIX designercore/projectstorage
  PUBLIC_INCLUDES designercore/projectstorage
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
    qmltypesparser.cpp qmltypesparser.h
    qmldocumentparser.cpp qmldocumentparser.h
)

add_qtc_plugin(QmlDesigner
  PLUGIN_RECOMMENDS QmlPreview
  CONDITION TARGET QmlDesignerCore AND TARGET Qt::QuickWidgets AND TARGET Qt::Svg
  PLUGIN_DEPENDS
    Core ProjectExplorer QmlDesignerBase QmlJSEditor QmakeProjectManager QmlProjectManager
    QtSupport
  PLUGIN_MANUAL_DEPENDS LicenseChecker ${IDE_VERSION} optional
  DEPENDS
    QmlJS LanguageUtils QmlEditorWidgets AdvancedDockingSystem
    Qt::QuickWidgets Qt::CorePrivate Qt::Xml Qt::Svg QmlDesignerCore Sqlite Zip
  PUBLIC_DEPENDS
    QmlDesignerUtils QmlPuppetCommunication QmlDesignerBase
  DEFINES
    IDE_LIBRARY_BASENAME=\"${IDE_LIBRARY_BASE_PATH}\"
    SHARE_QML_PATH="${CMAKE_CURRENT_SOURCE_DIR}/../../../share/qtcreator/qmldesigner"
    $<$<BOOL:${USE_PROJECTSTORAGE}>:QDS_USE_PROJECTSTORAGE>
    $<$<BOOL:${QTC_USE_QML_DESIGNER_LITE}>:QTC_USE_QML_DESIGNER_LITE>
  INCLUDES
    ${CMAKE_CURRENT_LIST_DIR}/components
    ${CMAKE_CURRENT_LIST_DIR}/components/import3d
    ${CMAKE_CURRENT_LIST_DIR}/components/assetslibrary
    ${CMAKE_CURRENT_LIST_DIR}/components/debugview
    ${CMAKE_CURRENT_LIST_DIR}/components/edit3d
    ${CMAKE_CURRENT_LIST_DIR}/components/formeditor
    ${CMAKE_CURRENT_LIST_DIR}/components/integration
    ${CMAKE_CURRENT_LIST_DIR}/components/itemlibrary
    ${CMAKE_CURRENT_LIST_DIR}/components/materialbrowser
    ${CMAKE_CURRENT_LIST_DIR}/components/materialeditor
    ${CMAKE_CURRENT_LIST_DIR}/components/textureeditor
    ${CMAKE_CURRENT_LIST_DIR}/components/navigator
    ${CMAKE_CURRENT_LIST_DIR}/components/propertyeditor
    ${CMAKE_CURRENT_LIST_DIR}/components/stateseditor
    ${CMAKE_CURRENT_LIST_DIR}/components/texteditor
    ${CMAKE_CURRENT_LIST_DIR}/components/designsystem
  PUBLIC_INCLUDES
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/designercore #can not be a public dependency -> EXCLUDE_FROM_INSTALL in QmlDesignerCore
    ${CMAKE_CURRENT_LIST_DIR}/designercore/include #iwidgetplugin.h is used by other plugins
    ${CMAKE_CURRENT_LIST_DIR}/designercore/designercoreutils
  SOURCES
    designmodecontext.cpp designmodecontext.h
    designmodewidget.cpp designmodewidget.h
    documentmanager.cpp documentmanager.h
    documentwarningwidget.cpp documentwarningwidget.h
    dynamiclicensecheck.h
    openuiqmlfiledialog.cpp openuiqmlfiledialog.h
    puppetenvironmentbuilder.cpp puppetenvironmentbuilder.h
    qmldesigner_global.h
    qmldesignerconstants.h
    qmldesignericons.h
    qmldesignerplugin.cpp qmldesignerplugin.h
    qmldesignerexternaldependencies.cpp qmldesignerexternaldependencies.h
    qmldesignerprojectmanager.cpp qmldesignerprojectmanager.h
    settingspage.cpp settingspage.h
    shortcutmanager.cpp shortcutmanager.h
    designermcumanager.cpp designermcumanager.h
    richtexteditordialog.cpp richtexteditordialog.h
    editorproxy.cpp editorproxy.h
  EXPLICIT_MOC
    components/propertyeditor/propertyeditorvalue.h
    qmldesignerplugin.h
  EXTRA_TRANSLATIONS
    "${PROJECT_SOURCE_DIR}/share/qtcreator/qmldesigner"
  PROPERTIES
    QMLDESIGNER_PLUGIN_PATH "${QmlDesignerPluginInstallPrefix}"
)

extend_qtc_plugin(QmlDesigner
    CONDITION ENABLE_COMPILE_WARNING_AS_ERROR
    PROPERTIES COMPILE_WARNING_AS_ERROR ON
    PUBLIC_COMPILE_OPTIONS $<$<COMPILE_LANG_AND_ID:CXX,Clang,GNU>:-Wno-error=maybe-uninitialized>
)

function(get_and_add_as_subdirectory name repository git_tag build_dir source_dir source_subdir)
    # make the configuration in the build dir
    file(MAKE_DIRECTORY ${build_dir}/${name})
    file(WRITE ${build_dir}/${name}/CMakeLists.txt
    "    cmake_minimum_required(VERSION 3.16)\n"
    "    project(${name}-download NONE)\n"
    "    \n"
    "    include(ExternalProject)\n"
    "    ExternalProject_Add(${name}\n"
    "                        UPDATE_DISCONNECTED FALSE\n"
    "                        GIT_REPOSITORY \"${repository}\"\n"
    "                        GIT_SUBMODULES \"\"\n"
    "                        GIT_TAG \"${git_tag}\"\n"
    "                        SOURCE_SUBDIR \"\"\n"
    "                        GIT_SHALLOW TRUE\n"
    "                        GIT_PROGRESS TRUE\n"
    "                        SOURCE_DIR \"${source_dir}/${name}\"\n"
    "                        CONFIGURE_COMMAND \"\"\n"
    "                        BUILD_COMMAND \"\"\n"
    "                        INSTALL_COMMAND \"\"\n"
    "                        TEST_COMMAND \"\"\n"
    "                        USES_TERMINAL_DOWNLOAD YES\n"
    "                        USES_TERMINAL_UPDATE YES\n"
    "    )\n"
    )
    execute_process(COMMAND "${CMAKE_COMMAND}" -G "${CMAKE_GENERATOR}" "${build_dir}/${name}"
        WORKING_DIRECTORY "${build_dir}/${name}"
        ${QTC_COMMAND_ERROR_IS_FATAL}
    )
    execute_process(COMMAND "${CMAKE_COMMAND}" --build "${build_dir}/${name}"
        ${QTC_COMMAND_ERROR_IS_FATAL}
    )
    add_subdirectory(${source_dir}/${name}/${source_subdir} ${name})
endfunction()

if (QTC_STATIC_BUILD AND TARGET QmlDesigner)
  get_target_property(_designerType Qt::Designer TYPE)
  if (${_designerType} STREQUAL "STATIC_LIBRARY")
    extend_qtc_target(QmlDesigner PUBLIC_DEFINES QT_DESIGNER_STATIC)
  endif()
  extend_qtc_target(QmlDesigner PUBLIC_DEPENDS TextEditor)
endif()

extend_qtc_plugin(QmlDesigner
  SOURCES_PREFIX components
  PUBLIC_INCLUDES components
  DEFINES QMLDESIGNERCOMPONENTS_LIBRARY
  SOURCES
    createtexture.cpp createtexture.h
    qmldesignercomponents_global.h
)

extend_qtc_plugin(QmlDesigner
  SOURCES_PREFIX components/componentcore
  PUBLIC_INCLUDES components/componentcore
  SOURCES
    abstractaction.cpp abstractaction.h
    abstractactiongroup.cpp abstractactiongroup.h
    actioninterface.h
    addimagesdialog.cpp addimagesdialog.h
    addsignalhandlerdialog.cpp addsignalhandlerdialog.h addsignalhandlerdialog.ui
    changestyleaction.cpp changestyleaction.h
    componentcore.qrc
    componentcore_constants.h
    crumblebar.cpp crumblebar.h
    designeractionmanager.cpp designeractionmanager.h
    designeractionmanagerview.cpp designeractionmanagerview.h
    designericons.cpp designericons.h
    findimplementation.cpp findimplementation.h
    groupitemaction.cpp groupitemaction.h
    layoutingridlayout.cpp layoutingridlayout.h
    modelnodecontextmenu.cpp modelnodecontextmenu.h
    modelnodecontextmenu_helper.cpp modelnodecontextmenu_helper.h
    modelnodeoperations.cpp modelnodeoperations.h
    formatoperation.cpp formatoperation.h
    navigation2d.cpp navigation2d.h
    propertyeditorcomponentgenerator.cpp propertyeditorcomponentgenerator.h
    propertycomponentgenerator.cpp propertycomponentgenerator.h
    propertycomponentgeneratorinterface.h
    qmldesignericonprovider.cpp qmldesignericonprovider.h
    qmleditormenu.cpp qmleditormenu.h
    resourcegenerator.cpp resourcegenerator.h
    selectioncontext.cpp selectioncontext.h
    theme.cpp theme.h
    zoomaction.cpp zoomaction.h
    anchoraction.cpp anchoraction.h
    svgpasteaction.cpp svgpasteaction.h
    viewmanager.cpp viewmanager.h
    utils3d.cpp utils3d.h
    dialogutils.cpp dialogutils.h
)

extend_qtc_plugin(QmlDesigner
  SOURCES_PREFIX components/debugview
  SOURCES
    debugview.cpp debugview.h
    debugviewwidget.cpp debugviewwidget.h debugviewwidget.ui
)

extend_qtc_plugin(QmlDesigner
  SOURCES_PREFIX components/edit3d
  SOURCES
  edit3dview.cpp edit3dview.h
  edit3dviewconfig.h
  edit3dwidget.cpp edit3dwidget.h
  edit3dcanvas.cpp edit3dcanvas.h
  edit3dactions.cpp edit3dactions.h
  edit3dmaterialsaction.cpp edit3dmaterialsaction.h
  edit3dtoolbarmenu.cpp edit3dtoolbarmenu.h
  backgroundcolorselection.cpp backgroundcolorselection.h
  bakelights.cpp bakelights.h
  indicatoractionwidget.cpp indicatoractionwidget.h
  snapconfiguration.cpp snapconfiguration.h
  cameraspeedconfiguration.cpp cameraspeedconfiguration.h
  bakelightsdatamodel.cpp bakelightsdatamodel.h
  bakelightsconnectionmanager.cpp bakelightsconnectionmanager.h
  edit3d.qrc
)

extend_qtc_plugin(QmlDesigner
  SOURCES_PREFIX components/formeditor
  PUBLIC_INCLUDES components/formeditor
  SOURCES
    abstractcustomtool.cpp abstractcustomtool.h
    abstractformeditortool.cpp abstractformeditortool.h
    anchorindicator.cpp anchorindicator.h
    anchorindicatorgraphicsitem.cpp anchorindicatorgraphicsitem.h
    backgroundaction.cpp backgroundaction.h
    bindingindicator.cpp bindingindicator.h
    bindingindicatorgraphicsitem.cpp bindingindicatorgraphicsitem.h
    controlelement.cpp controlelement.h
    dragtool.cpp dragtool.h
    formeditor.qrc
    formeditorannotationicon.cpp formeditorannotationicon.h
    formeditorgraphicsview.cpp formeditorgraphicsview.h
    formeditoritem.cpp formeditoritem.h
    formeditorscene.cpp formeditorscene.h
    formeditorsubwindow.h
    formeditortoolbutton.cpp formeditortoolbutton.h
    formeditorview.cpp formeditorview.h
    formeditorwidget.cpp formeditorwidget.h
    itemutilfunctions.cpp itemutilfunctions.h
    layeritem.cpp layeritem.h
    lineeditaction.cpp lineeditaction.h
    movemanipulator.cpp movemanipulator.h
    movetool.cpp movetool.h
    onedimensionalcluster.cpp onedimensionalcluster.h
    resizecontroller.cpp resizecontroller.h
    resizehandleitem.cpp resizehandleitem.h
    resizeindicator.cpp resizeindicator.h
    resizemanipulator.cpp resizemanipulator.h
    resizetool.cpp resizetool.h
    rotationtool.cpp rotationtool.h
    rotationindicator.cpp rotationindicator.h
    rotationcontroller.cpp rotationcontroller.h
    rotationhandleitem.cpp rotationhandleitem.h
    rotationmanipulator.cpp rotationmanipulator.h
    rubberbandselectionmanipulator.cpp rubberbandselectionmanipulator.h
    scaleitem.cpp scaleitem.h
    scalemanipulator.cpp scalemanipulator.h
    seekerslider.cpp seekerslider.h
    selectionindicator.cpp selectionindicator.h
    selectionrectangle.cpp selectionrectangle.h
    selectiontool.cpp selectiontool.h
    singleselectionmanipulator.cpp singleselectionmanipulator.h
    snapper.cpp snapper.h
    snappinglinecreator.cpp snappinglinecreator.h
    toolbox.cpp toolbox.h
    transitiontool.cpp transitiontool.h
    view3dtool.cpp view3dtool.h
)

extend_qtc_plugin(QmlDesigner
  SOURCES_PREFIX components/integration
  PUBLIC_INCLUDES components/integration
  SOURCES
    componentaction.cpp componentaction.h
    componentview.cpp componentview.h
    designdocument.cpp designdocument.h
    designdocumentview.cpp designdocumentview.h
    stackedutilitypanelcontroller.cpp stackedutilitypanelcontroller.h
    utilitypanelcontroller.cpp utilitypanelcontroller.h
)

extend_qtc_plugin(QmlDesigner
  SOURCES_PREFIX components/itemlibrary
  SOURCES
    itemlibrary.qrc
    itemlibraryconstants.h
    itemlibraryimageprovider.cpp itemlibraryimageprovider.h
    itemlibraryitem.cpp itemlibraryitem.h
    itemlibrarymodel.cpp itemlibrarymodel.h
    itemlibrarycategory.cpp itemlibrarycategory.h
    itemlibraryitemsmodel.cpp itemlibraryitemsmodel.h
    itemlibraryview.cpp itemlibraryview.h
    itemlibrarywidget.cpp itemlibrarywidget.h
    itemlibraryiconimageprovider.cpp itemlibraryiconimageprovider.h
    itemlibraryimport.cpp itemlibraryimport.h
    itemlibrarycategoriesmodel.cpp itemlibrarycategoriesmodel.h
    itemlibraryaddimportmodel.cpp itemlibraryaddimportmodel.h
)

extend_qtc_plugin(QmlDesigner
  SOURCES_PREFIX components/import3d
  SOURCES
    assetimportupdatedialog.cpp assetimportupdatedialog.h
    assetimportupdatedialog.ui
    assetimportupdatetreeitem.cpp assetimportupdatetreeitem.h
    assetimportupdatetreeitemdelegate.cpp assetimportupdatetreeitemdelegate.h
    assetimportupdatetreemodel.cpp assetimportupdatetreemodel.h
    assetimportupdatetreeview.cpp assetimportupdatetreeview.h
    import3dcanvas.cpp import3dcanvas.h
    import3dconnectionmanager.cpp import3dconnectionmanager.h
    import3ddialog.cpp import3ddialog.h
    import3ddialog.ui
    import3dimporter.cpp import3dimporter.h
)

extend_qtc_plugin(QmlDesigner
  SOURCES_PREFIX components/assetslibrary
  SOURCES
    assetslibrary.qrc
    assetslibraryview.cpp assetslibraryview.h
    assetslibrarywidget.cpp assetslibrarywidget.h
    assetslibrarymodel.cpp assetslibrarymodel.h
    assetslibraryiconprovider.cpp assetslibraryiconprovider.h
)

extend_qtc_plugin(QmlDesigner
  SOURCES_PREFIX components/navigator
  SOURCES
    iconcheckboxitemdelegate.cpp iconcheckboxitemdelegate.h
    nameitemdelegate.cpp nameitemdelegate.h
    navigator.qrc
    navigatormodelinterface.h
    navigatorsearchwidget.cpp navigatorsearchwidget.h
    navigatortreemodel.cpp navigatortreemodel.h
    navigatortreeview.cpp navigatortreeview.h
    navigatorview.cpp navigatorview.h
    navigatorwidget.cpp navigatorwidget.h
    choosefrompropertylistdialog.cpp choosefrompropertylistdialog.h
    choosefrompropertylistdialog.ui
    previewtooltip.cpp previewtooltip.h
    previewtooltip.ui
)

extend_qtc_plugin(QmlDesigner
  SOURCES_PREFIX components/propertyeditor
  SOURCES
    aligndistribute.cpp aligndistribute.h
    assetimageprovider.cpp assetimageprovider.h
    colorpalettebackend.cpp colorpalettebackend.h
    designerpropertymap.cpp designerpropertymap.h
    fileresourcesmodel.cpp fileresourcesmodel.h
    itemfiltermodel.cpp itemfiltermodel.h
    listvalidator.cpp listvalidator.h
    gradientmodel.cpp gradientmodel.h
    dynamicpropertiesproxymodel.cpp dynamicpropertiesproxymodel.h
    gradientpresetcustomlistmodel.cpp gradientpresetcustomlistmodel.h
    gradientpresetdefaultlistmodel.cpp gradientpresetdefaultlistmodel.h
    gradientpresetitem.cpp gradientpresetitem.h
    gradientpresetlistmodel.cpp gradientpresetlistmodel.h
    propertyeditorcontextobject.cpp propertyeditorcontextobject.h
    propertyeditorqmlbackend.cpp propertyeditorqmlbackend.h
    propertyeditortransaction.cpp propertyeditortransaction.h
    propertyeditorvalue.cpp propertyeditorvalue.h
    propertyeditorview.cpp propertyeditorview.h
    propertyeditorwidget.cpp propertyeditorwidget.h
    propertynamevalidator.cpp propertynamevalidator.h
    tooltip.cpp tooltip.h
    qmlanchorbindingproxy.cpp qmlanchorbindingproxy.h
    qmlmodelnodeproxy.cpp qmlmodelnodeproxy.h
    quick2propertyeditorview.cpp quick2propertyeditorview.h
    propertyeditorutils.cpp propertyeditorutils.h
)

extend_qtc_plugin(QmlDesigner
  SOURCES_PREFIX components/contentlibrary
  SOURCES
    contentlibrarybundleimporter.cpp contentlibrarybundleimporter.h
    contentlibraryview.cpp contentlibraryview.h
    contentlibrarywidget.cpp contentlibrarywidget.h
    contentlibrarytexturesmodel.cpp contentlibrarytexturesmodel.h
    contentlibrarytexturescategory.cpp contentlibrarytexturescategory.h
    contentlibrarytexture.cpp contentlibrarytexture.h
    contentlibrarymaterialsmodel.cpp contentlibrarymaterialsmodel.h
    contentlibrarymaterialscategory.cpp contentlibrarymaterialscategory.h
    contentlibrarymaterial.cpp contentlibrarymaterial.h
    contentlibraryiconprovider.cpp contentlibraryiconprovider.h
    contentlibraryitem.cpp contentlibraryitem.h
    contentlibraryeffectscategory.cpp contentlibraryeffectscategory.h
    contentlibraryeffectsmodel.cpp contentlibraryeffectsmodel.h
    contentlibraryusermodel.cpp contentlibraryusermodel.h
    usercategory.cpp usercategory.h
    useritemcategory.cpp useritemcategory.h
    usertexturecategory.cpp usertexturecategory.h
)

extend_qtc_plugin(QmlDesigner
  SOURCES_PREFIX components/materialeditor
  SOURCES
    materialeditorcontextobject.cpp materialeditorcontextobject.h
    materialeditordynamicpropertiesproxymodel.cpp materialeditordynamicpropertiesproxymodel.h
    materialeditorimageprovider.cpp materialeditorimageprovider.h
    materialeditorqmlbackend.cpp materialeditorqmlbackend.h
    materialeditortransaction.cpp materialeditortransaction.h
    materialeditorview.cpp materialeditorview.h
    materialeditor.qrc
)

extend_qtc_plugin(QmlDesigner
  SOURCES_PREFIX components/textureeditor
  SOURCES
    textureeditorcontextobject.cpp textureeditorcontextobject.h
    textureeditordynamicpropertiesproxymodel.cpp textureeditordynamicpropertiesproxymodel.h
    textureeditorqmlbackend.cpp textureeditorqmlbackend.h
    textureeditortransaction.cpp textureeditortransaction.h
    textureeditorview.cpp textureeditorview.h
    textureeditor.qrc
)

extend_qtc_plugin(QmlDesigner
  SOURCES_PREFIX components/materialbrowser
  SOURCES
    materialbrowserview.cpp materialbrowserview.h
    materialbrowserwidget.cpp materialbrowserwidget.h
    materialbrowsermodel.cpp materialbrowsermodel.h
    materialbrowsertexturesmodel.cpp materialbrowsertexturesmodel.h
    materialutils.cpp materialutils.h
)

extend_qtc_plugin(QmlDesigner
  SOURCES_PREFIX components
  SOURCES resources/resources_qmldesigner_components.qrc
)

extend_qtc_plugin(QmlDesigner
  SOURCES_PREFIX components/stateseditor
  SOURCES
    propertychangesmodel.cpp propertychangesmodel.h
    propertymodel.cpp propertymodel.h
    stateseditorimageprovider.cpp stateseditorimageprovider.h
    stateseditormodel.cpp stateseditormodel.h
    stateseditorview.cpp stateseditorview.h
    stateseditorwidget.cpp stateseditorwidget.h
)

extend_qtc_plugin(QmlDesigner
  SOURCES_PREFIX components/texteditor
  SOURCES
    texteditor.qrc
    texteditorstatusbar.cpp texteditorstatusbar.h
    texteditorview.cpp texteditorview.h
    texteditorwidget.cpp texteditorwidget.h
)

extend_qtc_plugin(QmlDesigner
  SOURCES_PREFIX components/listmodeleditor
  SOURCES
    listmodeleditordialog.cpp listmodeleditordialog.h
    listmodeleditormodel.cpp listmodeleditormodel.h
)

extend_qtc_plugin(QmlDesigner
  SOURCES_PREFIX designercore
  SOURCES
    include/asynchronousimagecacheinterface.h
    include/bytearraymodifier.h
    include/iwidgetplugin.h
    include/modificationgroupexception.h
    include/modificationgrouptoken.h
    include/nodeanchors.h
    include/objectpropertybinding.h
    include/projectstorageids.h
    include/propertybinding.h
    include/sourcepathids.h
    include/qml3dnode.h
    include/qmlvisualnode.h
)

extend_qtc_plugin(QmlDesigner
  SOURCES_PREFIX designercore/instances
  SOURCES
    capturingconnectionmanager.cpp capturingconnectionmanager.h
    interactiveconnectionmanager.cpp interactiveconnectionmanager.h
)

extend_qtc_plugin(QmlDesigner
  SOURCES_PREFIX designercore/imagecache
  SOURCES
    explicitimagecacheimageprovider.cpp
    explicitimagecacheimageprovider.h
    imagecacheimageresponse.cpp
    imagecacheimageresponse.h
    midsizeimagecacheprovider.cpp
    midsizeimagecacheprovider.h
    smallimagecacheprovider.cpp
    smallimagecacheprovider.h
)

extend_qtc_plugin(QmlDesigner
  SOURCES_PREFIX designercore
  PUBLIC_INCLUDES designercore
  SOURCES
    rewriter/basetexteditmodifier.cpp
)

extend_qtc_plugin(QmlDesigner
  SOURCES_PREFIX components/bindingeditor
  SOURCES bindingeditor.cpp bindingeditor.h
    actioneditor.cpp actioneditor.h
    abstracteditordialog.cpp abstracteditordialog.h
    actioneditordialog.cpp actioneditordialog.h
    bindingeditordialog.cpp bindingeditordialog.h
    bindingeditorwidget.cpp bindingeditorwidget.h
    connectionvisitor.cpp connectionvisitor.h
    signallist.cpp signallist.h
    signallistdialog.cpp signallistdialog.h
    signallistdelegate.cpp signallistdelegate.h
)

extend_qtc_plugin(QmlDesigner
  SOURCES_PREFIX components/colortool
  SOURCES colortool.cpp colortool.h
)

extend_qtc_plugin(QmlDesigner
  SOURCES_PREFIX components/annotationeditor
  SOURCES annotationcommenttab.cpp annotationcommenttab.h annotationcommenttab.ui
  annotationeditordialog.cpp annotationeditordialog.h
  annotationeditorwidget.cpp annotationeditorwidget.h annotationeditorwidget.ui
  annotationeditor.cpp annotationeditor.h
  annotationlist.cpp annotationlist.h
  annotationlistwidget.cpp annotationlistwidget.h
  globalannotationeditor.cpp globalannotationeditor.h
  globalannotationdialog.cpp globalannotationdialog.h
  defaultannotations.cpp defaultannotations.h
  annotationtableview.cpp annotationtableview.h
  annotationtabwidget.cpp annotationtabwidget.h
  annotationeditor.qrc
)

extend_qtc_plugin(QmlDesigner
  SOURCES_PREFIX components/connectioneditor
  SOURCES
    addnewbackenddialog.cpp addnewbackenddialog.h addnewbackenddialog.ui
    backendmodel.cpp backendmodel.h
    bindingmodel.cpp bindingmodel.h
    bindingmodelitem.cpp bindingmodelitem.h
    connectioneditor.qrc
    connectioneditorevaluator.cpp connectioneditorevaluator.h
    connectioneditorstatements.cpp connectioneditorstatements.h
    connectionmodel.cpp connectionmodel.h
    connectionview.cpp connectionview.h
    dynamicpropertiesmodel.cpp dynamicpropertiesmodel.h
    dynamicpropertiesitem.cpp dynamicpropertiesitem.h
    connectioneditorutils.cpp connectioneditorutils.h
    selectiondynamicpropertiesproxymodel.cpp selectiondynamicpropertiesproxymodel.h
    propertytreemodel.cpp propertytreemodel.h
)

extend_qtc_plugin(QmlDesigner
  SOURCES_PREFIX components
  SOURCES
    pathtool/controlpoint.cpp pathtool/controlpoint.h
    pathtool/cubicsegment.cpp pathtool/cubicsegment.h
    pathtool/pathitem.cpp pathtool/pathitem.h
    pathtool/pathselectionmanipulator.cpp pathtool/pathselectionmanipulator.h
    pathtool/pathtool.cpp pathtool/pathtool.h
    pathtool/pathtoolview.cpp pathtool/pathtoolview.h

    sourcetool/sourcetool.cpp sourcetool/sourcetool.h

    texttool/textedititem.cpp texttool/textedititem.h
    texttool/textedititemwidget.cpp texttool/textedititemwidget.h
    texttool/texttool.cpp texttool/texttool.h
)

extend_qtc_plugin(QmlDesigner
  SOURCES_PREFIX components/previewtooltip
  SOURCES
    previewimagetooltip.cpp
    previewimagetooltip.h
    previewimagetooltip.ui
    previewtooltipbackend.cpp
    previewtooltipbackend.h
)

extend_qtc_plugin(QmlDesigner
  SOURCES_PREFIX components/richtexteditor
  SOURCES
    hyperlinkdialog.cpp hyperlinkdialog.h hyperlinkdialog.ui
    richtexteditor.cpp richtexteditor.h
    richtexteditorproxy.cpp richtexteditorproxy.h
)

extend_qtc_plugin(QmlDesigner
  SOURCES_PREFIX components/timelineeditor
  SOURCES
    canvas.cpp canvas.h
    canvasstyledialog.cpp canvasstyledialog.h
    easingcurve.cpp easingcurve.h
    easingcurvedialog.cpp easingcurvedialog.h
    preseteditor.cpp preseteditor.h
    setframevaluedialog.cpp setframevaluedialog.h
    splineeditor.cpp splineeditor.h
    timeline.qrc
    timelineabstracttool.cpp timelineabstracttool.h
    timelineactions.cpp timelineactions.h
    timelineanimationform.cpp timelineanimationform.h
    timelineconstants.h
    timelinecontext.cpp timelinecontext.h
    timelinecontrols.cpp timelinecontrols.h
    timelineform.cpp timelineform.h
    timelinegraphicslayout.cpp timelinegraphicslayout.h
    timelinegraphicsscene.cpp timelinegraphicsscene.h
    timelineicons.h
    timelineitem.cpp timelineitem.h
    timelinemovableabstractitem.cpp timelinemovableabstractitem.h
    timelinemovetool.cpp timelinemovetool.h
    timelineplaceholder.cpp timelineplaceholder.h
    timelinepropertyitem.cpp timelinepropertyitem.h
    timelinesectionitem.cpp timelinesectionitem.h
    timelineselectiontool.cpp timelineselectiontool.h
    timelinesettingsdialog.cpp timelinesettingsdialog.h
    timelinesettingsmodel.cpp timelinesettingsmodel.h
    timelinetoolbar.cpp timelinetoolbar.h
    timelinetoolbutton.cpp timelinetoolbutton.h
    timelinetooldelegate.cpp timelinetooldelegate.h
    timelineutils.cpp timelineutils.h
    timelineview.cpp timelineview.h
    timelinewidget.cpp timelinewidget.h
)

extend_qtc_plugin(QmlDesigner
  SOURCES_PREFIX components/transitioneditor
  SOURCES
    transitioneditorview.cpp transitioneditorview.h
    transitioneditorwidget.cpp transitioneditorwidget.h
    transitioneditortoolbar.cpp transitioneditortoolbar.h
    transitioneditorgraphicsscene.cpp transitioneditorgraphicsscene.h
    transitioneditorgraphicslayout.cpp transitioneditorgraphicslayout.h
    transitioneditorsectionitem.cpp transitioneditorsectionitem.h
    transitioneditorpropertyitem.cpp transitioneditorpropertyitem.h
    transitioneditorsettingsdialog.cpp transitioneditorsettingsdialog.h
    transitioneditorsettingsdialog.ui
    transitionform.cpp transitionform.h
    transitioneditor.qrc
)

extend_qtc_plugin(QmlDesigner
  SOURCES_PREFIX components/curveeditor
  SOURCES
    curveeditor.qrc
    curveeditorview.cpp curveeditorview.h
    animationcurve.cpp animationcurve.h
    curveeditor.cpp curveeditor.h
    curveeditorconstants.h
    curveeditortoolbar.cpp curveeditortoolbar.h
    curveeditormodel.cpp curveeditormodel.h
    curveeditorstyle.h
    curvesegment.cpp curvesegment.h
    keyframe.cpp keyframe.h
    treeitem.cpp treeitem.h
    detail/axis.cpp detail/axis.h
    detail/colorcontrol.cpp detail/colorcontrol.h
    detail/curveeditorstyledialog.cpp detail/curveeditorstyledialog.h
    detail/curveitem.cpp detail/curveitem.h
    detail/graphicsscene.cpp detail/graphicsscene.h
    detail/graphicsview.cpp detail/graphicsview.h
    detail/handleitem.cpp detail/handleitem.h
    detail/keyframeitem.cpp detail/keyframeitem.h
    detail/playhead.cpp detail/playhead.h
    detail/selectableitem.cpp detail/selectableitem.h
    detail/selectionmodel.cpp detail/selectionmodel.h
    detail/selector.cpp detail/selector.h
    detail/shortcut.cpp detail/shortcut.h
    detail/treeitemdelegate.cpp detail/treeitemdelegate.h
    detail/treemodel.cpp detail/treemodel.h
    detail/treeview.cpp detail/treeview.h
    detail/curveeditorutils.cpp detail/curveeditorutils.h
)

extend_qtc_plugin(QmlDesigner
  SOURCES_PREFIX components/eventlist
  SOURCES
    eventlistplugin.qrc
    eventlistpluginview.cpp eventlistpluginview.h
    assigneventdialog.cpp assigneventdialog.h
    eventlist.cpp eventlist.h
    eventlistactions.cpp eventlistactions.h
    eventlistdelegate.cpp eventlistdelegate.h
    eventlistdialog.cpp eventlistdialog.h
    eventlistview.cpp eventlistview.h
    eventlistutils.cpp eventlistutils.h
    filterlinewidget.cpp filterlinewidget.h
    nodelistdelegate.cpp nodelistdelegate.h
    nodelistview.cpp nodelistview.h
    nodeselectionmodel.cpp nodeselectionmodel.h
    connectsignaldialog.cpp connectsignaldialog.h
    shortcutwidget.cpp shortcutwidget.h
)

extend_qtc_plugin(QmlDesigner
  SOURCES_PREFIX components/toolbar
  SOURCES
    toolbar.cpp
    toolbar.h
    toolbarbackend.cpp
    toolbarbackend.h
)

extend_qtc_plugin(QmlDesigner
  SOURCES_PREFIX components/designsystem
  SOURCES
  dsconstants.h
  dsthememanager.h dsthememanager.cpp
  dsthemegroup.cpp dsthemegroup.h
)

add_qtc_plugin(assetexporterplugin
  PLUGIN_CLASS AssetExporterPlugin
  CONDITION TARGET QmlDesigner
  PLUGIN_DEPENDS
    Core ProjectExplorer QmlDesigner
  DEPENDS Utils Qt::Qml Qt::QuickPrivate
  PUBLIC_INCLUDES assetexporterplugin
  PLUGIN_PATH ${QmlDesignerPluginInstallPrefix}
)

extend_qtc_plugin(assetexporterplugin
    CONDITION ENABLE_COMPILE_WARNING_AS_ERROR
    PROPERTIES COMPILE_WARNING_AS_ERROR ON
)

extend_qtc_plugin(assetexporterplugin
  SOURCES_PREFIX assetexporterplugin
  SOURCES
    assetexportdialog.h assetexportdialog.cpp
    assetexporter.h assetexporter.cpp
    assetexporterplugin.h assetexporterplugin.cpp
    assetexporterview.h assetexporterview.cpp
    assetexportpluginconstants.h
    componentexporter.h componentexporter.cpp
    exportnotification.h exportnotification.cpp
    filepathmodel.h filepathmodel.cpp
    dumpers/assetnodedumper.h dumpers/assetnodedumper.cpp
    dumpers/itemnodedumper.h dumpers/itemnodedumper.cpp
    dumpers/nodedumper.h dumpers/nodedumper.cpp
    dumpers/textnodedumper.h dumpers/textnodedumper.cpp
    assetexporterplugin.qrc
)

add_qtc_plugin(componentsplugin
  PLUGIN_CLASS ComponentsPlugin
  CONDITION TARGET QmlDesigner
  PLUGIN_DEPENDS Core QmlDesigner
  DEPENDS Utils Qt::Qml
  DEFINES COMPONENTS_LIBRARY

  PLUGIN_PATH ${QmlDesignerPluginInstallPrefix}
)

extend_qtc_plugin(componentsplugin
    CONDITION ENABLE_COMPILE_WARNING_AS_ERROR
    PROPERTIES COMPILE_WARNING_AS_ERROR ON
)

extend_qtc_plugin(componentsplugin
  SOURCES_PREFIX componentsplugin
  SOURCES
    componentsplugin.cpp componentsplugin.h
    componentsplugin.qrc
)

add_qtc_plugin(qmlpreviewplugin
  PLUGIN_CLASS QmlPreviewWidgetPlugin
  CONDITION TARGET QmlDesigner
  PLUGIN_DEPENDS Core ProjectExplorer QmlDesigner
  DEPENDS Utils Qt::Qml
  PLUGIN_PATH ${QmlDesignerPluginInstallPrefix}
)

extend_qtc_plugin(qmlpreviewplugin
    CONDITION ENABLE_COMPILE_WARNING_AS_ERROR
    PROPERTIES COMPILE_WARNING_AS_ERROR ON
)

extend_qtc_plugin(qmlpreviewplugin
    SOURCES_PREFIX qmlpreviewplugin
    SOURCES
      qmlpreviewactions.cpp qmlpreviewactions.h
      qmlpreviewplugin.cpp qmlpreviewplugin.h
      qmlpreviewplugin.qrc
)

add_qtc_plugin(qtquickplugin
  PLUGIN_CLASS QtQuickPlugin
  CONDITION TARGET QmlDesigner
  PLUGIN_DEPENDS Core QmlDesigner
  DEPENDS Utils Qt::Qml
  DEFINES QTQUICK_LIBRARY
  PLUGIN_PATH ${QmlDesignerPluginInstallPrefix}
)

extend_qtc_plugin(qtquickplugin
    CONDITION ENABLE_COMPILE_WARNING_AS_ERROR
    PROPERTIES COMPILE_WARNING_AS_ERROR ON
)

extend_qtc_plugin(qtquickplugin
    SOURCES_PREFIX qtquickplugin
    SOURCES
      qtquickplugin.cpp qtquickplugin.h
      qtquickplugin.qrc
)

if (BUILD_DESIGNSTUDIO)
    configure_file(
        "${QtCreator_SOURCE_DIR}/dist/branding/qtdesignstudio/QtProject/QtDesignStudio.ini"
        "${PROJECT_BINARY_DIR}/${IDE_DATA_PATH}/QtProject/${IDE_CASED_ID}.ini"
        COPYONLY
    )
    install(FILES
        ${QtCreator_SOURCE_DIR}/dist/branding/qtdesignstudio/QtProject/QtDesignStudio.ini
        DESTINATION ${IDE_DATA_PATH}/QtProject
        RENAME ${IDE_CASED_ID}.ini
    )
endif ()
