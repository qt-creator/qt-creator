find_package(Qt6 COMPONENTS QmlDomPrivate QmlCompilerPrivate)

set(QmlDesignerPluginInstallPrefix "${IDE_PLUGIN_PATH}/qmldesigner")
if (APPLE)
  set(QmlDesignerPluginInstallPrefix "${IDE_PLUGIN_PATH}/QmlDesigner")
endif()


env_with_default("QDS_USE_PROJECTSTORAGE" ENV_QDS_USE_PROJECTSTORAGE OFF)
option(USE_PROJECTSTORAGE "Use ProjectStorage" ${ENV_QDS_USE_PROJECTSTORAGE})
add_feature_info("ProjectStorage" ${USE_PROJECTSTORAGE} "")

add_qtc_library(QmlDesignerUtils STATIC
  DEPENDS
    Qt::Gui Utils Qt::QmlPrivate
  DEFINES QMLDESIGNERUTILS_LIBRARY
  PUBLIC_DEFINES $<$<BOOL:${QTC_STATIC_BUILD}>:QMLDESIGNER_STATIC_LIBRARY>

  PUBLIC_INCLUDES ${CMAKE_CURRENT_LIST_DIR}/utils
  SOURCES_PREFIX ${CMAKE_CURRENT_LIST_DIR}/utils
  SOURCES
    asset.cpp asset.h
    filedownloader.cpp filedownloader.h
    fileextractor.cpp fileextractor.h
    hdrimage.cpp hdrimage.h
    ktximage.cpp ktximage.h
    imageutils.cpp imageutils.h
    qmldesignerutils_global.h
)

add_qtc_library(QmlDesignerCore STATIC
  DEPENDS
    Threads::Threads
    Qt::CorePrivate
    CPlusPlus
    Utils
    Qt::Widgets
    Qt::Qml
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
  DEFINES QMLDESIGNERCORE_LIBRARY QMLDESIGNERUTILS_LIBRARY
  INCLUDES
    ${CMAKE_CURRENT_LIST_DIR}
  PUBLIC_INCLUDES
    ${CMAKE_CURRENT_LIST_DIR}/designercore
    ${CMAKE_CURRENT_LIST_DIR}/designercore/include
  SOURCES_PREFIX ${CMAKE_CURRENT_LIST_DIR}/designercore
  SOURCES
    rewritertransaction.cpp
    rewritertransaction.h
)

extend_qtc_library(QmlDesignerCore
  CONDITION TARGET Qt6::QmlDomPrivate AND TARGET Qt6::QmlCompilerPrivate AND Qt6_VERSION VERSION_GREATER_EQUAL 6.5.0

  DEPENDS Qt6::QmlDomPrivate Qt6::QmlCompilerPrivate
  DEFINES QDS_HAS_QMLDOM
)

extend_qtc_library(QmlDesignerCore
  CONDITION UNIX AND NOT APPLE
  PUBLIC_DEPENDS rt
)

set(UI_FILES
  ${CMAKE_CURRENT_LIST_DIR}/designercore/instances/puppetbuildprogressdialog.ui
  ${CMAKE_CURRENT_LIST_DIR}/designercore/instances/puppetdialog.ui
)
qt_wrap_ui(UI_SOURCES ${UI_FILES})

extend_qtc_library(QmlDesignerCore
  INCLUDES ${CMAKE_CURRENT_BINARY_DIR}
  SOURCES
    ${UI_SOURCES}
    ${UI_FILES}
)
set_source_files_properties(${UI_FILES} PROPERTIES SKIP_AUTOUIC ON)

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
    textureimagecachecollector.cpp
    textureimagecachecollector.h
    timestampprovider.cpp
    timestampprovider.h
    timestampproviderinterface.h
)

extend_qtc_library(QmlDesignerCore
  SOURCES_PREFIX ${CMAKE_CURRENT_LIST_DIR}/designercore/include
  SOURCES
    abstractproperty.h
    abstractview.h
    anchorline.h
    annotation.h
    asynchronousexplicitimagecache.h
    asynchronousimagecache.h
    auxiliarydata.h
    auxiliarydataproperties.h
    basetexteditmodifier.h
    bindingproperty.h
    componenttextmodifier.h
    customnotifications.h
    documentmessage.h
    enumerationmetainfo.h
    exception.h
    externaldependenciesinterface.h
    forwardview.h
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
    itemlibraryinfo.h
    mathutils.h
    metainfo.h
    metainforeader.h
    model.h
    modelmerger.h
    modelnode.h
    modelnodepositionstorage.h
    nodeabstractproperty.h
    nodehints.h
    nodeinstance.h
    nodeinstanceview.h
    nodelistproperty.h
    nodemetainfo.h
    nodeproperty.h
    notimplementedexception.h
    plaintexteditmodifier.h
    propertycontainer.h
    propertymetainfo.h
    propertynode.h
    propertyparser.h
    qmlanchors.h
    qmlchangeset.h
    qmlconnections.h
    qmldesignercorelib_global.h
    qmlitemnode.h
    qmlmodelnodefacade.h
    qmlobjectnode.h
    qmlstate.h
    qmltimeline.h
    qmltimelinekeyframegroup.h
    removebasestateexception.h
    rewriterview.h
    rewritingexception.h
    signalhandlerproperty.h
    stylesheetmerger.h
    subcomponentmanager.h
    synchronousimagecache.h
    textmodifier.h
    variantproperty.h
)

extend_qtc_library(QmlDesignerCore
  INCLUDES
    ${CMAKE_CURRENT_LIST_DIR}/designercore/metainfo
  SOURCES_PREFIX ${CMAKE_CURRENT_LIST_DIR}/designercore/metainfo
  SOURCES
    itemlibraryinfo.cpp
    metainfo.cpp
    metainforeader.cpp
    nodehints.cpp
    nodemetainfo.cpp
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
    puppetbuildprogressdialog.cpp
    puppetbuildprogressdialog.h
    puppetstarter.cpp
    puppetstarter.h
    puppetdialog.cpp
    puppetdialog.h
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
    bindingproperty.cpp
    componenttextmodifier.cpp
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
    modelmerger.cpp
    modelnode.cpp
    modelnodepositionrecalculator.cpp
    modelnodepositionrecalculator.h
    modelnodepositionstorage.cpp
    modeltotextmerger.cpp
    modeltotextmerger.h
    nodeabstractproperty.cpp
    nodelistproperty.cpp
    nodeproperty.cpp
    plaintexteditmodifier.cpp
    propertycontainer.cpp
    propertynode.cpp
    propertyparser.cpp
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
    rewriteaction.cpp
    rewriteaction.h
    rewriteactioncompressor.cpp
    rewriteactioncompressor.h
    rewriterview.cpp
    signalhandlerproperty.cpp
    stylesheetmerger.cpp
    textmodifier.cpp
    texttomodelmerger.cpp
    texttomodelmerger.h
    variantproperty.cpp
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
  SOURCES_PREFIX designercore/projectstorage
  PUBLIC_INCLUDES designercore/projectstorage
  SOURCES
    commontypecache.h
    directorypathcompressor.h
    filesysteminterface.h
    filesystem.cpp filesystem.h
    filestatus.h
    filestatuscache.cpp filestatuscache.h
    nonlockingmutex.h
    projectstorageinterface.h
    projectstoragefwd.h
    projectstorageinfotypes.h
    projectstoragepathwatcher.h
    projectstoragepathwatcherinterface.h
    projectstoragepathwatchernotifierinterface.h
    projectstoragepathwatcher.h
    projectstoragepathwatchertypes.h
    projectstorageprinting.h
    projectstoragetypes.h
    projectstorageupdater.cpp projectstorageupdater.h
    projectstorage.cpp projectstorage.h
    sourcepath.h
    sourcepathcache.h
    sourcepathcache.h
    sourcepathcachetypes.h
    sourcepathview.h
    storagecache.h
    storagecacheentry.h
    storagecachefwd.h
    qmldocumentparserinterface.h
    qmltypesparserinterface.h
    qmltypesparser.cpp qmltypesparser.h
    qmldocumentparser.cpp qmldocumentparser.h
)

file(GLOB PROJECTSTORAGE_EXCLUDED_SOURCES designercore/projectstorage/*.cpp)
set_property(SOURCE ${PROJECTSTORAGE_EXCLUDED_SOURCES} PROPERTY SKIP_AUTOMOC ON)

add_qtc_plugin(QmlDesigner
  CONDITION Qt6_VERSION VERSION_GREATER_EQUAL 6.2.0 AND TARGET Qt::QuickWidgets AND TARGET Qt::Svg
  DEPENDS
    QmlJS LanguageUtils QmlEditorWidgets AdvancedDockingSystem
    Qt::QuickWidgets Qt::CorePrivate Sqlite Qt::Xml Qt::Svg
  DEFINES
    IDE_LIBRARY_BASENAME=\"${IDE_LIBRARY_BASE_PATH}\"
    SHARE_QML_PATH="${CMAKE_CURRENT_SOURCE_DIR}/../../../share/qtcreator/qmldesigner"
    $<$<BOOL:${USE_PROJECTSTORAGE}>:QDS_USE_PROJECTSTORAGE>
    QMLDESIGNER_LIBRARY QMLDESIGNERCORE_LIBRARY QMLDESIGNERUTILS_LIBRARY
  INCLUDES
    ${CMAKE_CURRENT_LIST_DIR}/components
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
  PUBLIC_INCLUDES
    ${CMAKE_CURRENT_LIST_DIR}
  PUBLIC_DEPENDS
    QmlDesignerCore
  PLUGIN_DEPENDS
    Core ProjectExplorer QmlDesignerBase QmlJSEditor QmakeProjectManager QmlProjectManager
    QtSupport
  PLUGIN_RECOMMENDS QmlPreview
  SOURCES
    designmodecontext.cpp designmodecontext.h
    designmodewidget.cpp designmodewidget.h
    documentmanager.cpp documentmanager.h
    documentwarningwidget.cpp documentwarningwidget.h
    dynamiclicensecheck.h
    generateresource.cpp generateresource.h
    openuiqmlfiledialog.cpp openuiqmlfiledialog.h
    puppetenvironmentbuilder.cpp puppetenvironmentbuilder.h
    qmldesigner_global.h
    qmldesignerconstants.h
    qmldesignericons.h
    qmldesignerplugin.cpp qmldesignerplugin.h
    qmldesignerexternaldependencies.cpp qmldesignerexternaldependencies.h
    qmldesignerprojectmanager.cpp qmldesignerprojectmanager.h
    settingspage.cpp settingspage.h settingspage.ui
    shortcutmanager.cpp shortcutmanager.h
    designermcumanager.cpp designermcumanager.h
    richtexteditordialog.cpp richtexteditordialog.h
    editorproxy.cpp editorproxy.h
  EXPLICIT_MOC
    components/propertyeditor/propertyeditorvalue.h
    components/connectioneditor/connectionviewwidget.h
  EXTRA_TRANSLATIONS
    "${PROJECT_SOURCE_DIR}/share/qtcreator/qmldesigner"
  PROPERTIES
    QMLDESIGNER_PLUGIN_PATH "${QmlDesignerPluginInstallPrefix}"
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

add_qtc_plugin(assetexporterplugin
  PLUGIN_CLASS AssetExporterPlugin
  CONDITION TARGET QmlDesigner
  DEPENDS Core ProjectExplorer QmlDesigner Utils Qt::Qml Qt::QuickPrivate
  PUBLIC_INCLUDES assetexporterplugin
  SOURCES
    assetexporterplugin/assetexportdialog.h assetexporterplugin/assetexportdialog.cpp assetexporterplugin/assetexportdialog.ui
    assetexporterplugin/assetexporter.h assetexporterplugin/assetexporter.cpp
    assetexporterplugin/assetexporterplugin.h assetexporterplugin/assetexporterplugin.cpp
    assetexporterplugin/assetexporterview.h assetexporterplugin/assetexporterview.cpp
    assetexporterplugin/assetexportpluginconstants.h
    assetexporterplugin/componentexporter.h assetexporterplugin/componentexporter.cpp
    assetexporterplugin/exportnotification.h assetexporterplugin/exportnotification.cpp
    assetexporterplugin/filepathmodel.h assetexporterplugin/filepathmodel.cpp
    assetexporterplugin/dumpers/assetnodedumper.h assetexporterplugin/dumpers/assetnodedumper.cpp
    assetexporterplugin/dumpers/itemnodedumper.h assetexporterplugin/dumpers/itemnodedumper.cpp
    assetexporterplugin/dumpers/nodedumper.h assetexporterplugin/dumpers/nodedumper.cpp
    assetexporterplugin/dumpers/textnodedumper.h assetexporterplugin/dumpers/textnodedumper.cpp
    assetexporterplugin/assetexporterplugin.qrc
  PLUGIN_PATH ${QmlDesignerPluginInstallPrefix}
)

add_qtc_plugin(componentsplugin
  PLUGIN_CLASS ComponentsPlugin
  CONDITION TARGET QmlDesigner
  DEPENDS Core QmlDesigner Utils Qt::Qml
  DEFINES COMPONENTS_LIBRARY
  SOURCES
    componentsplugin/addtabdesigneraction.cpp componentsplugin/addtabdesigneraction.h
    componentsplugin/addtabtotabviewdialog.cpp componentsplugin/addtabtotabviewdialog.h
    componentsplugin/addtabtotabviewdialog.ui
    componentsplugin/componentsplugin.cpp componentsplugin/componentsplugin.h
    componentsplugin/componentsplugin.qrc
    componentsplugin/entertabdesigneraction.cpp componentsplugin/entertabdesigneraction.h
    componentsplugin/tabviewindexmodel.cpp componentsplugin/tabviewindexmodel.h
  PLUGIN_PATH ${QmlDesignerPluginInstallPrefix}
)

add_qtc_plugin(qmlpreviewplugin
  PLUGIN_CLASS QmlPreviewWidgetPlugin
  CONDITION TARGET QmlDesigner
  DEPENDS Core ProjectExplorer QmlDesigner Utils Qt::Qml
  SOURCES
    qmlpreviewplugin/qmlpreviewactions.cpp qmlpreviewplugin/qmlpreviewactions.h
    qmlpreviewplugin/qmlpreviewplugin.cpp qmlpreviewplugin/qmlpreviewplugin.h
    qmlpreviewplugin/qmlpreviewplugin.qrc
  PLUGIN_PATH ${QmlDesignerPluginInstallPrefix}
)

add_qtc_plugin(qtquickplugin
  PLUGIN_CLASS QtQuickPlugin
  CONDITION TARGET QmlDesigner
  DEPENDS Core QmlDesigner Utils Qt::Qml
  DEFINES QTQUICK_LIBRARY
  SOURCES
    qtquickplugin/qtquickplugin.cpp qtquickplugin/qtquickplugin.h
    qtquickplugin/qtquickplugin.qrc
  PLUGIN_PATH ${QmlDesignerPluginInstallPrefix}
)

add_subdirectory(studioplugin)

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
    qmldesignericonprovider.cpp qmldesignericonprovider.h
    qmleditormenu.cpp qmleditormenu.h
    selectioncontext.cpp selectioncontext.h
    theme.cpp theme.h
    zoomaction.cpp zoomaction.h
    anchoraction.cpp anchoraction.h
    svgpasteaction.cpp svgpasteaction.h
    viewmanager.cpp viewmanager.h
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
  edit3dvisibilitytogglesmenu.cpp edit3dvisibilitytogglesmenu.h
  backgroundcolorselection.cpp backgroundcolorselection.h
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
    contentnoteditableindicator.cpp contentnoteditableindicator.h
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
    assetimportupdatedialog.cpp assetimportupdatedialog.h
    assetimportupdatedialog.ui
    assetimportupdatetreeitem.cpp assetimportupdatetreeitem.h
    assetimportupdatetreeitemdelegate.cpp assetimportupdatetreeitemdelegate.h
    assetimportupdatetreemodel.cpp assetimportupdatetreemodel.h
    assetimportupdatetreeview.cpp assetimportupdatetreeview.h
    itemlibrary.qrc
    itemlibraryimageprovider.cpp itemlibraryimageprovider.h
    itemlibraryitem.cpp itemlibraryitem.h
    itemlibrarymodel.cpp itemlibrarymodel.h
    itemlibrarycategory.cpp itemlibrarycategory.h
    itemlibraryitemsmodel.cpp itemlibraryitemsmodel.h
    itemlibraryview.cpp itemlibraryview.h
    itemlibrarywidget.cpp itemlibrarywidget.h
    itemlibraryassetimportdialog.cpp itemlibraryassetimportdialog.h
    itemlibraryassetimportdialog.ui
    itemlibraryassetimporter.cpp itemlibraryassetimporter.h
    itemlibraryiconimageprovider.cpp itemlibraryiconimageprovider.h
    itemlibraryimport.cpp itemlibraryimport.h
    itemlibrarycategoriesmodel.cpp itemlibrarycategoriesmodel.h
    itemlibraryaddimportmodel.cpp itemlibraryaddimportmodel.h
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
    tooltip.cpp tooltip.h
    qmlanchorbindingproxy.cpp qmlanchorbindingproxy.h
    qmlmodelnodeproxy.cpp qmlmodelnodeproxy.h
    quick2propertyeditorview.cpp quick2propertyeditorview.h
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
)

extend_qtc_plugin(QmlDesigner
  SOURCES_PREFIX components/materialeditor
  SOURCES
    materialeditorcontextobject.cpp materialeditorcontextobject.h
    materialeditordynamicpropertiesproxymodel.cpp materialeditordynamicpropertiesproxymodel.h
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
)

extend_qtc_plugin(QmlDesigner
  SOURCES_PREFIX components
  SOURCES resources/resources_qmldesigner_components.qrc
)

extend_qtc_plugin(QmlDesigner
  SOURCES_PREFIX components/stateseditor
  SOURCES
    stateseditorimageprovider.cpp stateseditorimageprovider.h
    stateseditormodel.cpp stateseditormodel.h
    stateseditorview.cpp stateseditorview.h
    stateseditorwidget.cpp stateseditorwidget.h
)

extend_qtc_plugin(QmlDesigner
  SOURCES_PREFIX components/stateseditornew
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
    model/basetexteditmodifier.cpp
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
    connectioneditor.qrc
    connectionmodel.cpp connectionmodel.h
    connectionview.cpp connectionview.h
    connectionviewwidget.cpp connectionviewwidget.h connectionviewwidget.ui
    delegates.cpp delegates.h
    dynamicpropertiesmodel.cpp dynamicpropertiesmodel.h
    selectiondynamicpropertiesproxymodel.cpp selectiondynamicpropertiesproxymodel.h
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
    richtexteditor.cpp richtexteditor.h hyperlinkdialog.ui
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
    timelineanimationform.cpp timelineanimationform.h timelineanimationform.ui
    timelineconstants.h
    timelinecontext.cpp timelinecontext.h
    timelinecontrols.cpp timelinecontrols.h
    timelineform.cpp timelineform.h timelineform.ui
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
    timelinesettingsdialog.cpp
    timelinesettingsdialog.h timelinesettingsdialog.ui
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
  CONDITION TARGET Nanotrace
  DEPENDS Nanotrace
)
