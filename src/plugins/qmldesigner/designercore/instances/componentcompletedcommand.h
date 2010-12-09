#ifndef COMPONENTCOMPLETEDCOMMAND_H
#define COMPONENTCOMPLETEDCOMMAND_H
#include <QMetaType>
#include <QVector>

namespace QmlDesigner {

class ComponentCompletedCommand
{
    friend QDataStream &operator>>(QDataStream &in, ComponentCompletedCommand &command);

public:
    ComponentCompletedCommand();
    ComponentCompletedCommand(const QVector<qint32> &container);

    QVector<qint32> instances() const;

private:
    QVector<qint32> m_instanceVector;
};

QDataStream &operator<<(QDataStream &out, const ComponentCompletedCommand &command);
QDataStream &operator>>(QDataStream &in, ComponentCompletedCommand &command);

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::ComponentCompletedCommand);

#endif // COMPONENTCOMPLETEDCOMMAND_H
