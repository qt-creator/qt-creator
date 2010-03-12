#ifndef PLUGINCOLLECTION_H
#define PLUGINCOLLECTION_H

#include <QList>
#include <QString>
#include "extensionsystem_global.h"


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

}

#endif // PLUGINCOLLECTION_H
