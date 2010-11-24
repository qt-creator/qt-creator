#ifndef REPARENTCONTAINER_H
#define REPARENTCONTAINER_H

#include <qmetatype.h>

namespace QmlDesigner {

class ReparentContainer
{
    friend QDataStream &operator>>(QDataStream &in, ReparentContainer &container);
public:
    ReparentContainer();
    ReparentContainer(qint32 instanceId,
                      qint32 oldParentInstanceId,
                      const QString &oldParentProperty,
                      qint32 newParentInstanceId,
                      const QString &newParentProperty);

    qint32 instanceId() const;
    qint32 oldParentInstanceId() const;
    QString oldParentProperty() const;
    qint32 newParentInstanceId() const;
    QString newParentProperty() const;

private:
    qint32 m_instanceId;
    qint32 m_oldParentInstanceId;
    QString m_oldParentProperty;
    qint32 m_newParentInstanceId;
    QString m_newParentProperty;
};

QDataStream &operator<<(QDataStream &out, const ReparentContainer &container);
QDataStream &operator>>(QDataStream &in, ReparentContainer &container);

} // namespace QmlDesigner

#endif // REPARENTCONTAINER_H
