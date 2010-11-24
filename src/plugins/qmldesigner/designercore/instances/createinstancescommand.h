#ifndef CREATEINSTANCESCOMMAND_H
#define CREATEINSTANCESCOMMAND_H

#include <QMetaType>
#include <QVector>

#include "instancecontainer.h"

namespace QmlDesigner {

class CreateInstancesCommand
{
    friend QDataStream &operator>>(QDataStream &in, CreateInstancesCommand &command);

public:
    CreateInstancesCommand();
    CreateInstancesCommand(const QVector<InstanceContainer> &container);

    QVector<InstanceContainer> instances() const;

private:
    QVector<InstanceContainer> m_instanceVector;
};

QDataStream &operator<<(QDataStream &out, const CreateInstancesCommand &command);
QDataStream &operator>>(QDataStream &in, CreateInstancesCommand &command);

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::CreateInstancesCommand);

#endif // CREATEINSTANCESCOMMAND_H
