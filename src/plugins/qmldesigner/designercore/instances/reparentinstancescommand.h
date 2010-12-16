#ifndef REPARENTINSTANCESCOMMAND_H
#define REPARENTINSTANCESCOMMAND_H

#include <QMetaType>
#include <QVector>

#include "reparentcontainer.h"

namespace QmlDesigner {

class ReparentInstancesCommand
{
    friend QDataStream &operator>>(QDataStream &in, ReparentInstancesCommand &command);

public:
    ReparentInstancesCommand();
    ReparentInstancesCommand(const QVector<ReparentContainer> &container);

    QVector<ReparentContainer> reparentInstances() const;

private:
    QVector<ReparentContainer> m_reparentInstanceVector;
};

QDataStream &operator<<(QDataStream &out, const ReparentInstancesCommand &command);
QDataStream &operator>>(QDataStream &in, ReparentInstancesCommand &command);

} //

Q_DECLARE_METATYPE(QmlDesigner::ReparentInstancesCommand)

#endif // REPARENTINSTANCESCOMMAND_H
