#include "%{ProjectNameLowerCase}plugin.h"
#include <QStringList>

using namespace Qt::StringLiterals;

%{ProjectNameCap}Plugin::%{ProjectNameCap}Plugin(QObject *parent)
    : QObject(parent)
{
    m_%{Feature}Backend = new %{Feature}Backend(this);
}

QStringList %{ProjectNameCap}Plugin::interfaces() const
{
    QStringList list;
    list << QStringLiteral(%{ProjectNameCap}Module_%{Feature}_iid);
    return list;
}

QIfFeatureInterface *%{ProjectNameCap}Plugin::interfaceInstance(const QString &interface) const
{
    if (interface == QStringLiteral(%{ProjectNameCap}Module_%{Feature}_iid))
        return m_%{Feature}Backend;

    return nullptr;
}

QString %{ProjectNameCap}Plugin::id() const
{
    return u"%{ProjectNameCap}.%{ProjectNameCap}Module_backend"_s;
}

QString %{ProjectNameCap}Plugin::configurationId() const
{
    return u"%{ProjectNameCap}.%{ProjectNameCap}Module"_s;
}

void %{ProjectNameCap}Plugin::updateServiceSettings(const QVariantMap &settings)
{
    Q_UNUSED(settings);
}
