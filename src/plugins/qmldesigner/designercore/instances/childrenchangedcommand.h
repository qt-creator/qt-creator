#ifndef CHILDRENCHANGEDCOMMAND_H
#define CHILDRENCHANGEDCOMMAND_H

#include <QMetaType>
#include <QVector>


namespace QmlDesigner {

class ChildrenChangedCommand
{
    friend QDataStream &operator>>(QDataStream &in, ChildrenChangedCommand &command);
public:
    ChildrenChangedCommand();
    ChildrenChangedCommand(qint32 parentInstanceId, const QVector<qint32> &childrenInstances);

    QVector<qint32> childrenInstances() const;
    qint32 parentInstanceId() const;

private:
    qint32 m_parentInstanceId;
    QVector<qint32> m_childrenVector;
};

QDataStream &operator<<(QDataStream &out, const ChildrenChangedCommand &command);
QDataStream &operator>>(QDataStream &in, ChildrenChangedCommand &command);

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::ChildrenChangedCommand)

#endif // CHILDRENCHANGEDCOMMAND_H
