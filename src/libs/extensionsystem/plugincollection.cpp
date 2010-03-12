#include "plugincollection.h"
#include "pluginspec.h"

namespace ExtensionSystem {

PluginCollection::PluginCollection(const QString& name) :
    m_name(name)
{

}

PluginCollection::~PluginCollection()
{

}

QString PluginCollection::name() const
{
    return m_name;
}

void PluginCollection::addPlugin(PluginSpec *spec)
{
    m_plugins.append(spec);
}

void PluginCollection::removePlugin(PluginSpec *spec)
{
    m_plugins.removeOne(spec);
}

QList<PluginSpec *> PluginCollection::plugins() const
{
    return m_plugins;
}

}
