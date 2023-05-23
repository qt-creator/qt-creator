add_qtc_plugin(Insight
    CONDITION TARGET QtCreator::QmlDesigner
    PLUGIN_DEPENDS
        QtCreator::Core QtCreator::QtSupport QtCreator::QmlDesigner QtCreator::QmlProjectManager QtCreator::ProjectExplorer
    DEPENDS
        QtCreator::Utils
        Qt6::Core Qt6::CorePrivate Qt6::Widgets
        Qt6::Qml Qt6::QmlPrivate Qt6::Quick Qt6::QuickWidgets
    SOURCES
        insightplugin.cpp insightplugin.h
        insightmodel.cpp insightmodel.h
        insightview.cpp insightview.h
        insightwidget.cpp insightwidget.h
)
