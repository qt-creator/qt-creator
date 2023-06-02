@if '%{Cpp:PragmaOnce}'
#pragma once
@else
#ifndef @COLLECTION_INCLUDE_GUARD@
#define @COLLECTION_INCLUDE_GUARD@
@endif

#include <QtDesigner>
#include <qplugin.h>

class @COLLECTION_PLUGIN_CLASS@ : public QObject, public QDesignerCustomWidgetCollectionInterface
{
    Q_OBJECT
    Q_INTERFACES(QDesignerCustomWidgetCollectionInterface)
    @COLLECTION_PLUGIN_METADATA@

public:
    explicit @COLLECTION_PLUGIN_CLASS@(QObject *parent = nullptr);

    QList<QDesignerCustomWidgetInterface*> customWidgets() const override;

private:
    QList<QDesignerCustomWidgetInterface*> m_widgets;
};

@if ! '%{Cpp:PragmaOnce}'
#endif // @COLLECTION_INCLUDE_GUARD@
@endif
