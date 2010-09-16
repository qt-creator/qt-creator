#ifndef PLUGINCOLLECTION_H
#define PLUGINCOLLECTION_H

#include "extensionsystem_global.h"

#include <QtCore/QList>
#include <QtCore/QString>

namespace ExtensionSystem {
class PluginSpec;

class EXTENSIONSYSTEM_EXPORT PluginCollection
{

public:
    explicit PluginCollection(const QString& name);
    ~PluginCollection();

    QString name() const;
    void addPlugin(PluginSpec *spec);
    void removePlugin(PluginSpec *spec);
    QList<PluginSpec *> plugins() const;
private:
    QString m_name;
    QList<PluginSpec *> m_plugins;

};

} // namespace ExtensionSystem

#endif // PLUGINCOLLECTION_H
