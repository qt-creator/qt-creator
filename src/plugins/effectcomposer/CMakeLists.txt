add_qtc_plugin(EffectComposer
  CONDITION TARGET Qt::Quick AND TARGET QtCreator::QmlDesigner
  PLUGIN_DEPENDS
    QtCreator::QmlDesigner QtCreator::ProjectExplorer QtCreator::QmlProjectManager
  DEPENDS
    Qt::Core Qt::CorePrivate Qt::Widgets Qt::Qml Qt::QmlPrivate Qt::Quick
    QtCreator::Utils
    GLSL
  SOURCES
    effectcomposereditablenodesmodel.cpp effectcomposereditablenodesmodel.h
    effectcodeeditorwidget.cpp effectcodeeditorwidget.h
    effectcomposerplugin.cpp
    effectcomposertr.h
    effectcomposerwidget.cpp effectcomposerwidget.h
    effectcomposerview.cpp effectcomposerview.h
    effectcomposermodel.cpp effectcomposermodel.h
    effectcomposernodesmodel.cpp effectcomposernodesmodel.h
    effectcomposeruniformsmodel.cpp effectcomposeruniformsmodel.h
    effectcomposeruniformstablemodel.cpp effectcomposeruniformstablemodel.h
    effectsautocomplete.cpp effectsautocomplete.h
    effectshaderscodeeditor.cpp effectshaderscodeeditor.h
    effectnode.cpp effectnode.h
    effectnodescategory.cpp effectnodescategory.h
    compositionnode.cpp compositionnode.h
    syntaxhighlighter.h syntaxhighlighter.cpp
    tableheaderlengthmodel.cpp tableheaderlengthmodel.h
    uniform.cpp uniform.h
    effectutils.cpp effectutils.h
    effectcomposercontextobject.cpp effectcomposercontextobject.h
    listmodelwidthcalculator.cpp listmodelwidthcalculator.h
    shaderfeatures.cpp shaderfeatures.h
    syntaxhighlighterdata.cpp syntaxhighlighterdata.h
    propertyhandler.cpp propertyhandler.h
)
