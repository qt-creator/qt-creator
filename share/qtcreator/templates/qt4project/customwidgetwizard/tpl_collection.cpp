@PLUGIN_INCLUDES@

@COLLECTION_PLUGIN_CLASS@::@COLLECTION_PLUGIN_CLASS@(QObject *parent)
        : QObject(parent)
{
@PLUGIN_ADDITIONS@
}

QList<QDesignerCustomWidgetInterface*> @COLLECTION_PLUGIN_CLASS@::customWidgets() const
{
    return m_widgets;
}
