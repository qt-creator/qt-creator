#ifndef IDCONTAINER_H
#define IDCONTAINER_H

#include <QDataStream>
#include <qmetatype.h>
#include <QString>


namespace QmlDesigner {

class IdContainer
{
    friend QDataStream &operator>>(QDataStream &in, IdContainer &container);

public:
    IdContainer();
    IdContainer(qint32 instanceId, const QString &id);

    qint32 instanceId() const;
    QString id() const;

private:
    qint32 m_instanceId;
    QString m_id;
};

QDataStream &operator<<(QDataStream &out, const IdContainer &container);
QDataStream &operator>>(QDataStream &in, IdContainer &container);

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::IdContainer);

#endif // IDCONTAINER_H
