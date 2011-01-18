#ifndef SYNCHRONIZECOMMAND_H
#define SYNCHRONIZECOMMAND_H

#include <QMetaType>
#include <QVector>

#include "propertyvaluecontainer.h"

namespace QmlDesigner {

class SynchronizeCommand
{
    friend QDataStream &operator>>(QDataStream &in, SynchronizeCommand &command);

public:
    SynchronizeCommand();
    SynchronizeCommand(int synchronizeId);

    int synchronizeId() const;

private:
    int m_synchronizeId;
};

QDataStream &operator<<(QDataStream &out, const SynchronizeCommand &command);
QDataStream &operator>>(QDataStream &in, SynchronizeCommand &command);

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::SynchronizeCommand)

#endif // SYNCHRONIZECOMMAND_H
