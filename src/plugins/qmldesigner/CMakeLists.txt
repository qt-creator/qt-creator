set(QmlDesignerPluginInstallPrefix "${IDE_PLUGIN_PATH}/qmldesigner")
if (APPLE)
  set(QmlDesignerPluginInstallPrefix "${IDE_PLUGIN_PATH}/QmlDesigner")
endif()

get_filename_component(
    QMLDOM_STANDALONE_CMAKELISTS
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../qmldom_standalone/src/qmldom/standalone/"
    ABSOLUTE
)

if(EXISTS ${QMLDOM_STANDALONE_CMAKELISTS} AND Qt6_FOUND AND NOT TARGET qmldomlib)
  add_subdirectory(
    ../../../../qmldom_standalone/src/qmldom/standalone
    ${CMAKE_BINARY_DIR}/qmldom_standalone)

  set_target_properties(qmldomlib PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "$<TARGET_PROPERTY:QmlJS,RUNTIME_OUTPUT_DIRECTORY>"
    LIBRARY_OUTPUT_DIRECTORY "$<TARGET_PROPERTY:QmlJS,LIBRARY_OUTPUT_DIRECTORY>")
endif()

add_qtc_plugin(QmlDesigner
  CONDITION Qt5_VERSION VERSION_GREATER_EQUAL 6.2.0 AND TARGET Qt5::QuickWidgets AND TARGET Qt5::Svg AND TARGET qmldomlib
  DEPENDS
    QmlJS LanguageUtils QmlEditorWidgets AdvancedDockingSystem
    Qt5::QuickWidgets Qt5::CorePrivate Sqlite Qt5::Xml Qt5::Svg
    qmldomlib
  DEFINES
    DESIGNER_CORE_LIBRARY
    IDE_LIBRARY_BASENAME=\"${IDE_LIBRARY_BASE_PATH}\"
    SHARE_QML_PATH="${CMAKE_CURRENT_SOURCE_DIR}/../../../share/qtcreator/qmldesigner"
  PUBLIC_INCLUDES
    "${CMAKE_CURRENT_LIST_DIR}"
    "${CMAKE_CURRENT_LIST_DIR}/designercore/include"
    "${CMAKE_CURRENT_LIST_DIR}/../../../share/qtcreator/qml/qmlpuppet/interfaces"
    "${CMAKE_CURRENT_LIST_DIR}/../../../share/qtcreator/qml/qmlpuppet/types"
  PLUGIN_DEPENDS
    Core ProjectExplorer QmlJSEditor QmakeProjectManager QmlProjectManager
    QtSupport TextEditor
  PLUGIN_RECOMMENDS QmlPreview
  SOURCES
    designersettings.cpp designersettings.h
    designmodecontext.cpp designmodecontext.h
    designmodewidget.cpp designmodewidget.h
    documentmanager.cpp documentmanager.h
    documentwarningwidget.cpp documentwarningwidget.h
    checkablefiletreeitem.cpp checkablefiletreeitem.h
    cmakegeneratordialog.cpp cmakegeneratordialog.h
    cmakegeneratordialogtreemodel.cpp cmakegeneratordialogtreemodel.h
    generateresource.cpp generateresource.h
    generatecmakelists.cpp generatecmakelists.h
    generatecmakelistsconstants.h
    openuiqmlfiledialog.cpp openuiqmlfiledialog.h openuiqmlfiledialog.ui
    qmldesignerconstants.h
    qmldesignericons.h
    qmldesignerplugin.cpp qmldesignerplugin.h
    qmldesignerprojectmanager.cpp qmldesignerprojectmanager.h
    settingspage.cpp settingspage.h settingspage.ui
    shortcutmanager.cpp shortcutmanager.h
    designermcumanager.cpp designermcumanager.h
    richtexteditordialog.cpp richtexteditordialog.h
    editorproxy.cpp editorproxy.h
    boilerplate.qrc
  EXPLICIT_MOC
    components/propertyeditor/propertyeditorvalue.h
    components/connectioneditor/connectionviewwidget.h
  EXTRA_TRANSLATIONS
    "${PROJECT_SOURCE_DIR}/share/qtcreator/qmldesigner"
  PROPERTIES
    QMLDESIGNER_PLUGIN_PATH "${QmlDesignerPluginInstallPrefix}"
)

include(qmldesignercore.cmake)
extend_with_qmldesigner_core(QmlDesigner)

add_qtc_plugin(assetexporterplugin
  CONDITION TARGET QmlDesigner
  DEPENDS Core ProjectExplorer QmlDesigner Utils Qt5::Qml Qt5::QuickPrivate
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
  CONDITION TARGET QmlDesigner
  DEPENDS Core QmlDesigner Utils Qt5::Qml
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
  CONDITION TARGET QmlDesigner
  DEPENDS Core ProjectExplorer QmlDesigner Utils Qt5::Qml
  SOURCES
    qmlpreviewplugin/qmlpreviewactions.cpp qmlpreviewplugin/qmlpreviewactions.h
    qmlpreviewplugin/qmlpreviewplugin.cpp qmlpreviewplugin/qmlpreviewplugin.h
    qmlpreviewplugin/qmlpreviewplugin.qrc
  PLUGIN_PATH ${QmlDesignerPluginInstallPrefix}
)

add_qtc_plugin(qtquickplugin
  CONDITION TARGET QmlDesigner
  DEPENDS Core QmlDesigner Utils Qt5::Qml
  DEFINES QTQUICK_LIBRARY
  SOURCES
    qtquickplugin/qtquickplugin.cpp qtquickplugin/qtquickplugin.h
    qtquickplugin/qtquickplugin.qrc
  PLUGIN_PATH ${QmlDesignerPluginInstallPrefix}
)

add_subdirectory(studioplugin)

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
    findimplementation.cpp findimplementation.h
    layoutingridlayout.cpp layoutingridlayout.h
    modelnodecontextmenu.cpp modelnodecontextmenu.h
    modelnodecontextmenu_helper.cpp modelnodecontextmenu_helper.h
    modelnodeoperations.cpp modelnodeoperations.h
    formatoperation.cpp formatoperation.h
    navigation2d.cpp navigation2d.h
    qmldesignericonprovider.cpp qmldesignericonprovider.h
    selectioncontext.cpp selectioncontext.h
    theme.cpp theme.h
    zoomaction.cpp zoomaction.h
    hdrimage.cpp hdrimage.h
    svgpasteaction.cpp svgpasteaction.h
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
  edit3dwidget.cpp edit3dwidget.h
  edit3dcanvas.cpp edit3dcanvas.h
  edit3dactions.cpp edit3dactions.h
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
    itemlibraryassetsmodel.cpp itemlibraryassetsmodel.h
    itemlibraryassetsiconprovider.cpp itemlibraryassetsiconprovider.h
    itemlibraryassetsdir.cpp itemlibraryassetsdir.h
    itemlibraryassetsdirsmodel.cpp itemlibraryassetsdirsmodel.h
    itemlibraryassetsfilesmodel.cpp itemlibraryassetsfilesmodel.h
)

extend_qtc_plugin(QmlDesigner
  SOURCES_PREFIX components/navigator
  SOURCES
    iconcheckboxitemdelegate.cpp iconcheckboxitemdelegate.h
    nameitemdelegate.cpp nameitemdelegate.h
    navigator.qrc
    navigatormodelinterface.h
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
    colorpalettebackend.cpp colorpalettebackend.h
    designerpropertymap.cpp designerpropertymap.h
    fileresourcesmodel.cpp fileresourcesmodel.h
    itemfiltermodel.cpp itemfiltermodel.h
    gradientmodel.cpp gradientmodel.h
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
  SOURCES_PREFIX components
  SOURCES resources/resources.qrc
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
    include/propertybinding.h
    include/qml3dnode.h
    include/qmlvisualnode.h

)

extend_qtc_plugin(QmlDesigner
  SOURCES_PREFIX designercore/projectstorage
  SOURCES
    directorypathcompressor.h
    filesysteminterface.h
    filesystem.cpp filesystem.h
    filestatus.h
    filestatuscache.cpp filestatuscache.h
    nonlockingmutex.h
    projectstorageinterface.h
    projectstorage.h
    projectstoragepathwatcher.h
    projectstoragepathwatcherinterface.h
    projectstoragepathwatchernotifierinterface.h
    projectstoragepathwatcher.h
    projectstoragepathwatchertypes.h
    projectstoragetypes.h
    projectstorageupdater.cpp projectstorageupdater.h
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
    setframevaluedialog.cpp setframevaluedialog.h setframevaluedialog.ui
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
  CONDITION TARGET Nanotrace
  DEPENDS Nanotrace
)
