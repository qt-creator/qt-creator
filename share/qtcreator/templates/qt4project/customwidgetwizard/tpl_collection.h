#ifndef @COLLECTION_INCLUDE_GUARD@
#define @COLLECTION_INCLUDE_GUARD@

#include <QtDesigner>
#include <qplugin.h>

class @COLLECTION_PLUGIN_CLASS@ : public QObject, public QDesignerCustomWidgetCollectionInterface
{
    Q_OBJECT
    Q_INTERFACES(QDesignerCustomWidgetCollectionInterface)
    @COLLECTION_PLUGIN_METADATA@

public:
    explicit @COLLECTION_PLUGIN_CLASS@(QObject *parent = 0);

    virtual QList<QDesignerCustomWidgetInterface*> customWidgets() const;

private:
    QList<QDesignerCustomWidgetInterface*> m_widgets;
};

#endif
