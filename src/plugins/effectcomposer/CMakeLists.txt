add_qtc_plugin(EffectComposer
  CONDITION TARGET Qt::Quick AND TARGET QtCreator::QmlDesigner
  PLUGIN_DEPENDS
    QtCreator::QmlDesigner QtCreator::ProjectExplorer QtCreator::QmlProjectManager
  DEPENDS
    Qt::Core Qt::CorePrivate Qt::Widgets Qt::Qml Qt::QmlPrivate Qt::Quick
    QtCreator::Utils
  SOURCES
    effectcomposerplugin.cpp effectcomposerplugin.h
    effectcomposerwidget.cpp effectcomposerwidget.h
    effectcomposerview.cpp effectcomposerview.h
    effectcomposermodel.cpp effectcomposermodel.h
    effectcomposernodesmodel.cpp effectcomposernodesmodel.h
    effectcomposeruniformsmodel.cpp effectcomposeruniformsmodel.h
    effectnode.cpp effectnode.h
    effectnodescategory.cpp effectnodescategory.h
    compositionnode.cpp compositionnode.h
    uniform.cpp uniform.h
    effectutils.cpp effectutils.h
    effectcomposercontextobject.cpp effectcomposercontextobject.h
    shaderfeatures.cpp shaderfeatures.h
    syntaxhighlighterdata.cpp syntaxhighlighterdata.h
    propertyhandler.cpp propertyhandler.h
)
