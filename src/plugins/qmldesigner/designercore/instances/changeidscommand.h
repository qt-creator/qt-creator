#ifndef CHANGEIDSCOMMAND_H
#define CHANGEIDSCOMMAND_H

#include <QMetaType>
#include <QVector>


#include "idcontainer.h"

namespace QmlDesigner {

class ChangeIdsCommand
{
    friend QDataStream &operator>>(QDataStream &in, ChangeIdsCommand &command);
public:
    ChangeIdsCommand();
    ChangeIdsCommand(const QVector<IdContainer> &idVector);

    QVector<IdContainer> ids() const;

private:
    QVector<IdContainer> m_idVector;
};

QDataStream &operator<<(QDataStream &out, const ChangeIdsCommand &command);
QDataStream &operator>>(QDataStream &in, ChangeIdsCommand &command);

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::ChangeIdsCommand);

#endif // CHANGEIDSCOMMAND_H
