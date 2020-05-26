add_qtc_plugin(ScxmlEditor
  INCLUDES common plugin_interface outputpane
  PLUGIN_DEPENDS Core ProjectExplorer QtSupport TextEditor
  SOURCES
    common/colorpicker.cpp common/colorpicker.h common/colorpicker.ui
    common/colorsettings.cpp common/colorsettings.h common/colorsettings.ui
    common/colorthemedialog.cpp common/colorthemedialog.h common/colorthemedialog.ui
    common/colorthemes.cpp common/colorthemes.h
    common/colorthemeview.cpp common/colorthemeview.h
    common/colortoolbutton.cpp common/colortoolbutton.h
    common/common.qrc
    common/dragshapebutton.cpp common/dragshapebutton.h
    common/graphicsview.cpp common/graphicsview.h
    common/magnifier.cpp common/magnifier.h common/magnifier.ui
    common/mainwidget.cpp common/mainwidget.h
    common/movableframe.cpp common/movableframe.h
    common/navigator.cpp common/navigator.h
    common/navigatorgraphicsview.cpp common/navigatorgraphicsview.h
    common/navigatorslider.cpp common/navigatorslider.h common/navigatorslider.ui
    common/search.cpp common/search.h common/search.ui
    common/searchmodel.cpp common/searchmodel.h
    common/shapegroupwidget.cpp common/shapegroupwidget.h
    common/shapestoolbox.cpp common/shapestoolbox.h common/shapestoolbox.ui
    common/sizegrip.cpp common/sizegrip.h
    common/stateproperties.cpp common/stateproperties.h
    common/stateview.cpp common/stateview.h common/stateview.ui
    common/statistics.cpp common/statistics.h common/statistics.ui
    common/statisticsdialog.cpp common/statisticsdialog.h common/statisticsdialog.ui
    common/structure.cpp common/structure.h
    common/structuremodel.cpp common/structuremodel.h
    common/treeview.cpp common/treeview.h
    outputpane/errorwidget.cpp outputpane/errorwidget.h
    outputpane/outputpane.h
    outputpane/outputtabwidget.cpp outputpane/outputtabwidget.h
    outputpane/tableview.cpp outputpane/tableview.h
    outputpane/warning.cpp outputpane/warning.h
    outputpane/warningmodel.cpp outputpane/warningmodel.h
    plugin_interface/actionhandler.cpp plugin_interface/actionhandler.h
    plugin_interface/actionprovider.h
    plugin_interface/attributeitemdelegate.cpp plugin_interface/attributeitemdelegate.h
    plugin_interface/attributeitemmodel.cpp plugin_interface/attributeitemmodel.h
    plugin_interface/baseitem.cpp plugin_interface/baseitem.h
    plugin_interface/connectableitem.cpp plugin_interface/connectableitem.h
    plugin_interface/cornergrabberitem.cpp plugin_interface/cornergrabberitem.h
    plugin_interface/finalstateitem.cpp plugin_interface/finalstateitem.h
    plugin_interface/genericscxmlplugin.cpp plugin_interface/genericscxmlplugin.h
    plugin_interface/graphicsitemprovider.h
    plugin_interface/graphicsscene.cpp plugin_interface/graphicsscene.h
    plugin_interface/highlightitem.cpp plugin_interface/highlightitem.h
    plugin_interface/historyitem.cpp plugin_interface/historyitem.h
    plugin_interface/idwarningitem.cpp plugin_interface/idwarningitem.h
    plugin_interface/imageprovider.cpp plugin_interface/imageprovider.h
    plugin_interface/initialstateitem.cpp plugin_interface/initialstateitem.h
    plugin_interface/initialwarningitem.cpp plugin_interface/initialwarningitem.h
    plugin_interface/isceditor.h
    plugin_interface/layoutitem.cpp plugin_interface/layoutitem.h
    plugin_interface/mytypes.h
    plugin_interface/parallelitem.cpp plugin_interface/parallelitem.h
    plugin_interface/quicktransitionitem.cpp plugin_interface/quicktransitionitem.h
    plugin_interface/scattributeitemdelegate.cpp plugin_interface/scattributeitemdelegate.h
    plugin_interface/scattributeitemmodel.cpp plugin_interface/scattributeitemmodel.h
    plugin_interface/sceneutils.cpp plugin_interface/sceneutils.h
    plugin_interface/scgraphicsitemprovider.cpp plugin_interface/scgraphicsitemprovider.h
    plugin_interface/scshapeprovider.cpp plugin_interface/scshapeprovider.h
    plugin_interface/scutilsprovider.cpp plugin_interface/scutilsprovider.h
    plugin_interface/scxmldocument.cpp plugin_interface/scxmldocument.h
    plugin_interface/scxmlnamespace.cpp plugin_interface/scxmlnamespace.h
    plugin_interface/scxmltag.cpp plugin_interface/scxmltag.h
    plugin_interface/scxmltagutils.cpp plugin_interface/scxmltagutils.h
    plugin_interface/scxmltypes.h
    plugin_interface/scxmluifactory.cpp plugin_interface/scxmluifactory.h
    plugin_interface/serializer.cpp plugin_interface/serializer.h
    plugin_interface/shapeprovider.cpp plugin_interface/shapeprovider.h
    plugin_interface/snapline.cpp plugin_interface/snapline.h
    plugin_interface/stateitem.cpp plugin_interface/stateitem.h
    plugin_interface/statewarningitem.cpp plugin_interface/statewarningitem.h
    plugin_interface/tagtextitem.cpp plugin_interface/tagtextitem.h
    plugin_interface/textitem.cpp plugin_interface/textitem.h
    plugin_interface/transitionitem.cpp plugin_interface/transitionitem.h
    plugin_interface/transitionwarningitem.cpp plugin_interface/transitionwarningitem.h
    plugin_interface/undocommands.cpp plugin_interface/undocommands.h
    plugin_interface/utilsprovider.cpp plugin_interface/utilsprovider.h
    plugin_interface/warningitem.cpp plugin_interface/warningitem.h
    plugin_interface/warningprovider.h
    scxmleditor_global.h
    scxmleditorconstants.h
    scxmleditordata.cpp scxmleditordata.h
    scxmleditordocument.cpp scxmleditordocument.h
    scxmleditorfactory.cpp scxmleditorfactory.h
    scxmleditorplugin.cpp scxmleditorplugin.h
    scxmleditorstack.cpp scxmleditorstack.h
    scxmltexteditor.cpp scxmltexteditor.h
)
