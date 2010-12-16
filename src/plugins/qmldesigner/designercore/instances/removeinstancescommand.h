#ifndef REMOVEINSTANCESCOMMAND_H
#define REMOVEINSTANCESCOMMAND_H

#include <QMetaType>
#include <QVector>

#include "instancecontainer.h"

namespace QmlDesigner {

class RemoveInstancesCommand
{
    friend QDataStream &operator>>(QDataStream &in, RemoveInstancesCommand &command);

public:
    RemoveInstancesCommand();
    RemoveInstancesCommand(const QVector<qint32> &idVector);

    QVector<qint32> instanceIds() const;

private:
    QVector<qint32> m_instanceIdVector;
};

QDataStream &operator<<(QDataStream &out, const RemoveInstancesCommand &command);
QDataStream &operator>>(QDataStream &in, RemoveInstancesCommand &command);

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::RemoveInstancesCommand)

#endif // REMOVEINSTANCESCOMMAND_H
