add_qtc_plugin(EffectMakerNew
  CONDITION TARGET Qt::Quick AND TARGET QtCreator::QmlDesigner
  PLUGIN_DEPENDS
    QtCreator::QmlDesigner QtCreator::ProjectExplorer QtCreator::QmlProjectManager
  DEPENDS
    Qt::Core Qt::CorePrivate Qt::Widgets Qt::Qml Qt::QmlPrivate Qt::Quick
    QtCreator::Utils
  SOURCES
    effectmakerplugin.cpp
    effectmakerwidget.cpp effectmakerwidget.h
    effectmakerview.cpp effectmakerview.h
    effectmakermodel.cpp effectmakermodel.h
    effectmakernodesmodel.cpp effectmakernodesmodel.h
    effectmakeruniformsmodel.cpp effectmakeruniformsmodel.h
    effectnode.cpp effectnode.h
    effectnodescategory.cpp effectnodescategory.h
    compositionnode.cpp compositionnode.h
    uniform.cpp uniform.h
    effectutils.cpp effectutils.h
    effectmakercontextobject.cpp effectmakercontextobject.h
    shaderfeatures.cpp shaderfeatures.h
    syntaxhighlighterdata.cpp syntaxhighlighterdata.h
    propertyhandler.cpp propertyhandler.h
)
