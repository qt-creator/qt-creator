find_package(Qt6 OPTIONAL_COMPONENTS Gui Quick ShaderTools)

add_qtc_plugin(EffectMakerNew
  CONDITION TARGET QmlDesigner AND TARGET Qt::ShaderTools
  PLUGIN_DEPENDS
    QtCreator::Core QtCreator::QmlDesigner
  DEPENDS
    Qt::Core
    QtCreator::Utils Qt::CorePrivate Qt::Widgets Qt::Qml Qt::QmlPrivate Qt::Quick Qt::ShaderTools Qt::ShaderToolsPrivate
  SOURCES
    effectmakerplugin.cpp effectmakerplugin.h
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
  BUILD_DEFAULT OFF
)
