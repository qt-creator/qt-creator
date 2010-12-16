#ifndef COMPLETECOMPONENT_H
#define COMPLETECOMPONENT_H

#include <QMetaType>
#include <QVector>

namespace QmlDesigner {

class CompleteComponentCommand
{
    friend QDataStream &operator>>(QDataStream &in, CompleteComponentCommand &command);

public:
    CompleteComponentCommand();
    CompleteComponentCommand(const QVector<qint32> &container);

    QVector<qint32> instances() const;

private:
    QVector<qint32> m_instanceVector;
};

QDataStream &operator<<(QDataStream &out, const CompleteComponentCommand &command);
QDataStream &operator>>(QDataStream &in, CompleteComponentCommand &command);

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::CompleteComponentCommand)

#endif // COMPLETECOMPONENT_H
