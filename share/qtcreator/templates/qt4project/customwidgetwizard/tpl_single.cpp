#include "@WIDGET_HEADER@"
#include "@PLUGIN_HEADER@"

#include <QtPlugin>

@PLUGIN_CLASS@::@PLUGIN_CLASS@(QObject *parent)
    : QObject(parent)
{
}

void @PLUGIN_CLASS@::initialize(QDesignerFormEditorInterface * /* core */)
{
    if (m_initialized)
        return;

    // Add extension registrations, etc. here

    m_initialized = true;
}

bool @PLUGIN_CLASS@::isInitialized() const
{
    return m_initialized;
}

QWidget *@PLUGIN_CLASS@::createWidget(QWidget *parent)
{
    return new @WIDGET_CLASS@(parent);
}

QString @PLUGIN_CLASS@::name() const
{
    return QLatin1String("@WIDGET_CLASS@");
}

QString @PLUGIN_CLASS@::group() const
{
    return QLatin1String("@WIDGET_GROUP@");
}

QIcon @PLUGIN_CLASS@::icon() const
{
    return QIcon(@WIDGET_ICON@);
}

QString @PLUGIN_CLASS@::toolTip() const
{
    return QLatin1String("@WIDGET_TOOLTIP@");
}

QString @PLUGIN_CLASS@::whatsThis() const
{
    return QLatin1String("@WIDGET_WHATSTHIS@");
}

bool @PLUGIN_CLASS@::isContainer() const
{
    return @WIDGET_ISCONTAINER@;
}

QString @PLUGIN_CLASS@::domXml() const
{
    return QLatin1String(@WIDGET_DOMXML@);
}

QString @PLUGIN_CLASS@::includeFile() const
{
    return QLatin1String("@WIDGET_HEADER@");
}
