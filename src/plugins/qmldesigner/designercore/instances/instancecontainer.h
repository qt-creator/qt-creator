#ifndef INSTANCECONTAINER_H
#define INSTANCECONTAINER_H

#include <qmetatype.h>
#include <QString>

namespace QmlDesigner {

class InstanceContainer;

QDataStream &operator<<(QDataStream &out, const InstanceContainer &container);
QDataStream &operator>>(QDataStream &in, InstanceContainer &container);

class InstanceContainer
{
    friend QDataStream &operator>>(QDataStream &in, InstanceContainer &container);

public:
    InstanceContainer();
    InstanceContainer(qint32 instanceId, const QString &type, int majorNumber, int minorNumber, const QString &componentPath);

    qint32 instanceId() const;
    QString type() const;
    int majorNumber() const;
    int minorNumber() const;
    QString componentPath() const;

private:
    qint32 m_instanceId;
    QString m_type;
    int m_majorNumber;
    int m_minorNumber;
    QString m_componentPath;
};

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::InstanceContainer);
#endif // INSTANCECONTAINER_H
