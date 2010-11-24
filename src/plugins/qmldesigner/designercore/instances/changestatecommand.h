#ifndef CHANGESTATECOMMAND_H
#define CHANGESTATECOMMAND_H

#include <QMetaType>
#include <QVector>

#include "propertyvaluecontainer.h"

namespace QmlDesigner {

class ChangeStateCommand
{
    friend QDataStream &operator>>(QDataStream &in, ChangeStateCommand &command);

public:
    ChangeStateCommand();
    ChangeStateCommand(qint32 stateInstanceId);

    qint32 stateInstanceId() const;

private:
    qint32 m_stateInstanceId;
};

QDataStream &operator<<(QDataStream &out, const ChangeStateCommand &command);
QDataStream &operator>>(QDataStream &in, ChangeStateCommand &command);

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::ChangeStateCommand);

#endif // CHANGESTATECOMMAND_H
