add_qtc_plugin(QmlDesigner
  DEPENDS
    QmlJS LanguageUtils QmlEditorWidgets AdvancedDockingSystem
    Qt5::QuickWidgets Qt5::CorePrivate
  DEFINES
    DESIGNER_CORE_LIBRARY
    IDE_LIBRARY_BASENAME=\"${IDE_LIBRARY_BASE_PATH}\"
  PUBLIC_INCLUDES
    "${CMAKE_CURRENT_LIST_DIR}"
    "${CMAKE_CURRENT_LIST_DIR}/designercore/include"
  PLUGIN_DEPENDS
    Core ProjectExplorer QmlJSEditor QmakeProjectManager QmlProjectManager
    QtSupport TextEditor
  SOURCES
    designersettings.cpp designersettings.h
    designmodecontext.cpp designmodecontext.h
    designmodewidget.cpp designmodewidget.h
    documentmanager.cpp documentmanager.h
    documentwarningwidget.cpp documentwarningwidget.h
    generateresource.cpp generateresource.h
    openuiqmlfiledialog.cpp openuiqmlfiledialog.h openuiqmlfiledialog.ui
    qmldesignerconstants.h
    qmldesignericons.h
    qmldesignerplugin.cpp qmldesignerplugin.h
    settingspage.cpp settingspage.h settingspage.ui
    shortcutmanager.cpp shortcutmanager.h
    switchsplittabwidget.cpp switchsplittabwidget.h
  EXPLICIT_MOC
    components/propertyeditor/propertyeditorvalue.h
    components/connectioneditor/connectionviewwidget.h
  SKIP_DEBUG_CMAKE_FILE_CHECK
  EXTRA_TRANSLATIONS
    "${PROJECT_SOURCE_DIR}/share/qtcreator/qmldesigner"
)

set(QmlDesignerPluginInstallPrefix "${IDE_PLUGIN_PATH}/qmldesigner")
if (APPLE)
  set(QmlDesignerPluginInstallPrefix "${IDE_PLUGIN_PATH}/QmlDesigner")
endif()


add_qtc_plugin(assetexporterplugin
  CONDITION TARGET QmlDesigner
  DEPENDS Core ProjectExplorer QmlDesigner Utils Qt5::Qml
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
    assetexporterplugin/parsers/assetnodeparser.h assetexporterplugin/parsers/assetnodeparser.cpp
    assetexporterplugin/parsers/modelitemnodeparser.h assetexporterplugin/parsers/modelitemnodeparser.cpp
    assetexporterplugin/parsers/modelnodeparser.h assetexporterplugin/parsers/modelnodeparser.cpp
    assetexporterplugin/parsers/textnodeparser.h assetexporterplugin/parsers/textnodeparser.cpp
    assetexporterplugin/assetexporterplugin.qrc
  PLUGIN_PATH ${QmlDesignerPluginInstallPrefix}
  SKIP_DEBUG_CMAKE_FILE_CHECK
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
  SKIP_DEBUG_CMAKE_FILE_CHECK
)

add_qtc_plugin(qmlpreviewplugin
  CONDITION TARGET QmlDesigner
  DEPENDS Core ProjectExplorer QmlDesigner Utils Qt5::Qml
  SOURCES
    qmlpreviewplugin/qmlpreviewactions.cpp qmlpreviewplugin/qmlpreviewactions.h
    qmlpreviewplugin/qmlpreviewplugin.cpp qmlpreviewplugin/qmlpreviewplugin.h
    qmlpreviewplugin/qmlpreviewplugin.qrc
  PLUGIN_PATH ${QmlDesignerPluginInstallPrefix}
  SKIP_DEBUG_CMAKE_FILE_CHECK
)

add_qtc_plugin(qtquickplugin
  CONDITION TARGET QmlDesigner
  DEPENDS Core QmlDesigner Utils Qt5::Qml
  DEFINES QTQUICK_LIBRARY
  SOURCES
    qtquickplugin/qtquickplugin.cpp qtquickplugin/qtquickplugin.h
    qtquickplugin/qtquickplugin.qrc
  PLUGIN_PATH ${QmlDesignerPluginInstallPrefix}
  SKIP_DEBUG_CMAKE_FILE_CHECK
)

extend_qtc_plugin(QmlDesigner
  SOURCES_PREFIX ../../../share/qtcreator/qml/qmlpuppet/container
  SOURCES
    addimportcontainer.cpp addimportcontainer.h
    idcontainer.cpp idcontainer.h
    imagecontainer.cpp imagecontainer.h
    informationcontainer.cpp informationcontainer.h
    instancecontainer.cpp instancecontainer.h
    mockuptypecontainer.cpp mockuptypecontainer.h
    propertyabstractcontainer.cpp propertyabstractcontainer.h
    propertybindingcontainer.cpp propertybindingcontainer.h
    propertyvaluecontainer.cpp propertyvaluecontainer.h
    reparentcontainer.cpp reparentcontainer.h
    sharedmemory.h
)

if (UNIX)
  extend_qtc_plugin(QmlDesigner
    SOURCES_PREFIX ../../../share/qtcreator/qml/qmlpuppet/container
    SOURCES sharedmemory_unix.cpp
  )
  if (NOT APPLE)
    extend_qtc_plugin(QmlDesigner DEPENDS rt)
  endif()
else()
  extend_qtc_plugin(QmlDesigner
    SOURCES_PREFIX ../../../share/qtcreator/qml/qmlpuppet/container
    SOURCES sharedmemory_qt.cpp
  )
endif()

extend_qtc_plugin(QmlDesigner
  SOURCES_PREFIX ../../../share/qtcreator/qml/qmlpuppet/commands
  SOURCES
    changeauxiliarycommand.cpp changeauxiliarycommand.h
    changebindingscommand.cpp changebindingscommand.h
    changefileurlcommand.cpp changefileurlcommand.h
    changeidscommand.cpp changeidscommand.h
    changenodesourcecommand.cpp changenodesourcecommand.h
    changestatecommand.cpp changestatecommand.h
    changevaluescommand.cpp changevaluescommand.h
    childrenchangedcommand.cpp childrenchangedcommand.h
    clearscenecommand.cpp clearscenecommand.h
    completecomponentcommand.cpp completecomponentcommand.h
    componentcompletedcommand.cpp componentcompletedcommand.h
    createinstancescommand.cpp createinstancescommand.h
    createscenecommand.cpp createscenecommand.h
    debugoutputcommand.cpp debugoutputcommand.h
    endpuppetcommand.cpp endpuppetcommand.h
    informationchangedcommand.cpp informationchangedcommand.h
    pixmapchangedcommand.cpp pixmapchangedcommand.h
    puppetalivecommand.cpp puppetalivecommand.h
    removeinstancescommand.cpp removeinstancescommand.h
    removepropertiescommand.cpp removepropertiescommand.h
    removesharedmemorycommand.cpp removesharedmemorycommand.h
    reparentinstancescommand.cpp reparentinstancescommand.h
    statepreviewimagechangedcommand.cpp statepreviewimagechangedcommand.h
    synchronizecommand.cpp synchronizecommand.h
    changepreviewimagesizecommand.cpp changepreviewimagesizecommand.h
    changelanguagecommand.cpp changelanguagecommand.h
    tokencommand.cpp tokencommand.h
    valueschangedcommand.cpp valueschangedcommand.h
    changeselectioncommand.cpp changeselectioncommand.h
    update3dviewstatecommand.cpp update3dviewstatecommand.h
    puppettocreatorcommand.cpp puppettocreatorcommand.h
    inputeventcommand.cpp inputeventcommand.h
    view3dactioncommand.cpp view3dactioncommand.h
)

extend_qtc_plugin(QmlDesigner
  SOURCES_PREFIX ../../../share/qtcreator/qml/qmlpuppet/interfaces
  PUBLIC_INCLUDES ../../../share/qtcreator/qml/qmlpuppet/interfaces
  SOURCES
    nodeinstanceserverinterface.cpp
    commondefines.h
    nodeinstanceclientinterface.h
    nodeinstanceglobal.h
    nodeinstanceserverinterface.h
)

extend_qtc_plugin(QmlDesigner
  SOURCES_PREFIX ../../../share/qtcreator/qml/qmlpuppet/types
  PUBLIC_INCLUDES ../../../share/qtcreator/qml/qmlpuppet/types
  SOURCES enumeration.h
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
    findimplementation.cpp findimplementation.h
    layoutingridlayout.cpp layoutingridlayout.h
    modelnodecontextmenu.cpp modelnodecontextmenu.h
    modelnodecontextmenu_helper.cpp modelnodecontextmenu_helper.h
    modelnodeoperations.cpp modelnodeoperations.h
    qmldesignericonprovider.cpp qmldesignericonprovider.h
    selectioncontext.cpp selectioncontext.h
    theme.cpp theme.h
    zoomaction.cpp zoomaction.h
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
    rubberbandselectionmanipulator.cpp rubberbandselectionmanipulator.h
    scaleitem.cpp scaleitem.h
    scalemanipulator.cpp scalemanipulator.h
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
  SOURCES_PREFIX components/importmanager
  SOURCES
    importlabel.cpp importlabel.h
    importmanager.qrc
    importmanagercombobox.cpp importmanagercombobox.h
    importmanagerview.cpp importmanagerview.h
    importswidget.cpp importswidget.h
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
    customfilesystemmodel.cpp customfilesystemmodel.h
    itemlibrary.qrc
    itemlibraryimageprovider.cpp itemlibraryimageprovider.h
    itemlibraryitem.cpp itemlibraryitem.h
    itemlibrarymodel.cpp itemlibrarymodel.h
    itemlibraryresourceview.cpp itemlibraryresourceview.h
    itemlibrarysection.cpp itemlibrarysection.h
    itemlibrarysectionmodel.cpp itemlibrarysectionmodel.h
    itemlibraryview.cpp itemlibraryview.h
    itemlibrarywidget.cpp itemlibrarywidget.h
    itemlibraryassetimportdialog.cpp itemlibraryassetimportdialog.h
    itemlibraryassetimportdialog.ui
    itemlibraryassetimporter.cpp itemlibraryassetimporter.h
)

find_package(Qt5 COMPONENTS Quick3DAssetImport QUIET)
extend_qtc_plugin(QmlDesigner
  CONDITION TARGET Qt5::Quick3DAssetImport
  FEATURE_INFO "Qt Quick 3D asset import"
  DEPENDS Qt5::Quick3DAssetImportPrivate
  DEFINES IMPORT_QUICK3D_ASSETS
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
)

extend_qtc_plugin(QmlDesigner
  SOURCES_PREFIX components/propertyeditor
  SOURCES
    aligndistribute.cpp aligndistribute.h
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
    simplecolorpalette.cpp simplecolorpalette.h
    simplecolorpalettemodel.cpp simplecolorpalettemodel.h
    simplecolorpalettesingleton.cpp simplecolorpalettesingleton.h
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

    filemanager/addarraymembervisitor.cpp filemanager/addarraymembervisitor.h
    filemanager/addobjectvisitor.cpp filemanager/addobjectvisitor.h
    filemanager/addpropertyvisitor.cpp filemanager/addpropertyvisitor.h
    filemanager/astobjecttextextractor.cpp filemanager/astobjecttextextractor.h
    filemanager/changeimportsvisitor.cpp filemanager/changeimportsvisitor.h
    filemanager/changeobjecttypevisitor.cpp filemanager/changeobjecttypevisitor.h
    filemanager/changepropertyvisitor.cpp filemanager/changepropertyvisitor.h
    filemanager/firstdefinitionfinder.cpp filemanager/firstdefinitionfinder.h
    filemanager/moveobjectbeforeobjectvisitor.cpp filemanager/moveobjectbeforeobjectvisitor.h
    filemanager/moveobjectvisitor.cpp filemanager/moveobjectvisitor.h
    filemanager/objectlengthcalculator.cpp filemanager/objectlengthcalculator.h
    filemanager/qmlrefactoring.cpp filemanager/qmlrefactoring.h
    filemanager/qmlrewriter.cpp filemanager/qmlrewriter.h
    filemanager/removepropertyvisitor.cpp filemanager/removepropertyvisitor.h
    filemanager/removeuiobjectmembervisitor.cpp filemanager/removeuiobjectmembervisitor.h

    include/abstractproperty.h
    include/abstractview.h
    include/anchorline.h
    include/annotation.h
    include/basetexteditmodifier.h
    include/bindingproperty.h
    include/bytearraymodifier.h
    include/componenttextmodifier.h
    include/customnotifications.h
    include/documentmessage.h
    include/exception.h
    include/forwardview.h
    include/import.h
    include/invalidargumentexception.h
    include/invalididexception.h
    include/invalidmetainfoexception.h
    include/invalidmodelnodeexception.h
    include/invalidmodelstateexception.h
    include/invalidpropertyexception.h
    include/invalidqmlsourceexception.h
    include/invalidreparentingexception.h
    include/invalidslideindexexception.h
    include/itemlibraryinfo.h
    include/iwidgetplugin.h
    include/mathutils.h
    include/metainfo.h
    include/metainforeader.h
    include/model.h
    include/modelmerger.h
    include/modelnode.h
    include/modelnodepositionstorage.h
    include/modificationgroupexception.h
    include/modificationgrouptoken.h
    include/nodeabstractproperty.h
    include/nodeanchors.h
    include/nodehints.h
    include/nodeinstance.h
    include/nodeinstanceview.h
    include/nodelistproperty.h
    include/nodemetainfo.h
    include/nodeproperty.h
    include/notimplementedexception.h
    include/objectpropertybinding.h
    include/plaintexteditmodifier.h
    include/propertybinding.h
    include/propertycontainer.h
    include/propertynode.h
    include/propertyparser.h
    include/qmlanchors.h
    include/qmlchangeset.h
    include/qmldesignercorelib_global.h
    include/qmlitemnode.h
    include/qmlvisualnode.h
    include/qml3dnode.h
    include/qmlmodelnodefacade.h
    include/qmlobjectnode.h
    include/qmlstate.h
    include/qmltimeline.h
    include/qmltimelinekeyframegroup.h
    include/removebasestateexception.h
    include/rewriterview.h
    include/rewritingexception.h
    include/signalhandlerproperty.h
    include/stylesheetmerger.h
    include/subcomponentmanager.h
    include/textmodifier.h
    include/variantproperty.h
    include/viewmanager.h
)

extend_qtc_plugin(QmlDesigner
  SOURCES_PREFIX designercore/instances
  SOURCES
    nodeinstance.cpp
    nodeinstanceserverproxy.cpp nodeinstanceserverproxy.h
    nodeinstanceview.cpp
    puppetbuildprogressdialog.cpp puppetbuildprogressdialog.h puppetbuildprogressdialog.ui
    puppetcreator.cpp puppetcreator.h
    puppetdialog.cpp puppetdialog.h puppetdialog.ui
)

extend_qtc_plugin(QmlDesigner
  SOURCES_PREFIX designercore
  PUBLIC_INCLUDES designercore
  SOURCES
    metainfo/itemlibraryinfo.cpp
    metainfo/metainfo.cpp
    metainfo/metainforeader.cpp
    metainfo/nodehints.cpp
    metainfo/nodemetainfo.cpp
    metainfo/subcomponentmanager.cpp

    model/abstractproperty.cpp
    model/abstractview.cpp
    model/anchorline.cpp
    model/annotation.cpp
    model/basetexteditmodifier.cpp
    model/bindingproperty.cpp
    model/componenttextmodifier.cpp
    model/documentmessage.cpp
    model/import.cpp
    model/internalbindingproperty.cpp model/internalbindingproperty.h
    model/internalnode.cpp model/internalnode_p.h
    model/internalnodeabstractproperty.cpp model/internalnodeabstractproperty.h
    model/internalnodelistproperty.cpp model/internalnodelistproperty.h
    model/internalnodeproperty.cpp model/internalnodeproperty.h
    model/internalproperty.cpp model/internalproperty.h
    model/internalsignalhandlerproperty.cpp model/internalsignalhandlerproperty.h
    model/internalvariantproperty.cpp model/internalvariantproperty.h
    model/model.cpp model/model_p.h
    model/modelmerger.cpp
    model/modelnode.cpp
    model/modelnodepositionrecalculator.cpp model/modelnodepositionrecalculator.h
    model/modelnodepositionstorage.cpp
    model/modeltotextmerger.cpp model/modeltotextmerger.h
    model/nodeabstractproperty.cpp
    model/nodelistproperty.cpp
    model/nodeproperty.cpp
    model/plaintexteditmodifier.cpp
    model/propertycontainer.cpp
    model/propertynode.cpp
    model/propertyparser.cpp
    model/qmlanchors.cpp
    model/qmlchangeset.cpp
    model/qmlitemnode.cpp
    model/qmlvisualnode.cpp
    model/qml3dnode.cpp
    model/qmlmodelnodefacade.cpp
    model/qmlobjectnode.cpp
    model/qmlstate.cpp
    model/qmltextgenerator.cpp model/qmltextgenerator.h
    model/qmltimeline.cpp
    model/qmltimelinekeyframegroup.cpp
    model/rewriteaction.cpp model/rewriteaction.h
    model/rewriteactioncompressor.cpp model/rewriteactioncompressor.h
    model/rewriterview.cpp
    model/signalhandlerproperty.cpp
    model/stylesheetmerger.cpp
    model/textmodifier.cpp
    model/texttomodelmerger.cpp model/texttomodelmerger.h
    model/variantproperty.cpp
    model/viewmanager.cpp

    pluginmanager/widgetpluginmanager.cpp pluginmanager/widgetpluginmanager.h
    pluginmanager/widgetpluginpath.cpp pluginmanager/widgetpluginpath.h
    rewritertransaction.cpp rewritertransaction.h
)

extend_qtc_plugin(QmlDesigner
  SOURCES_PREFIX components/bindingeditor
  SOURCES bindingeditor.cpp bindingeditor.h
    actioneditor.cpp actioneditor.h
    bindingeditordialog.cpp bindingeditordialog.h
    bindingeditorwidget.cpp bindingeditorwidget.h
)

extend_qtc_plugin(QmlDesigner
  SOURCES_PREFIX components/colortool
  SOURCES colortool.cpp colortool.h
)

extend_qtc_plugin(QmlDesigner
  SOURCES_PREFIX components/annotationeditor
  SOURCES annotationcommenttab.cpp annotationcommenttab.h annotationcommenttab.ui
  annotationeditordialog.cpp annotationeditordialog.h annotationeditordialog.ui
  globalannotationeditordialog.cpp globalannotationeditordialog.h globalannotationeditordialog.ui
  annotationeditor.cpp annotationeditor.h
  annotationtool.cpp annotationtool.h
  globalannotationeditor.cpp globalannotationeditor.h
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
  SOURCES_PREFIX components/richtexteditor
  SOURCES
    hyperlinkdialog.cpp hyperlinkdialog.h hyperlinkdialog.ui
    richtexteditor.cpp richtexteditor.h hyperlinkdialog.ui
)

extend_qtc_plugin(QmlDesigner
  SOURCES_PREFIX components/timelineeditor
  SOURCES
    animationcurvedialog.cpp animationcurvedialog.h
    animationcurveeditormodel.cpp animationcurveeditormodel.h
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
    detail/utils.cpp detail/utils.h
)

# Do the file comparison at the end, due to all the extend_qtc_plugin calls
if (WITH_DEBUG_CMAKE)
  foreach(plugin QmlDesigner componentsplugin qtquickplugin)
    unset(plugin_sources)
    get_target_property(plugin_sources ${plugin} SOURCES)
    list(APPEND QmlDesignerSources ${plugin_sources})
  endforeach()
  compare_sources_with_existing_disk_files(QmlDesigner "${QmlDesignerSources}")
endif()
