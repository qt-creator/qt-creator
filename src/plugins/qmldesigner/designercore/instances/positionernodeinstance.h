#ifndef POSITIONERNODEINSTANCE_H
#define POSITIONERNODEINSTANCE_H

#include "qmlgraphicsitemnodeinstance.h"

QT_BEGIN_NAMESPACE
class QDeclarativeBasePositioner;
QT_END_NAMESPACE

namespace QmlDesigner {
namespace Internal {

class PositionerNodeInstance : public QmlGraphicsItemNodeInstance
{
public:
    typedef QSharedPointer<PositionerNodeInstance> Pointer;
    typedef QWeakPointer<PositionerNodeInstance> WeakPointer;

    static Pointer create(QObject *objectToBeWrapped);

    void setPropertyVariant(const QString &name, const QVariant &value);
    void setPropertyBinding(const QString &name, const QString &expression);

    bool isPositioner() const;

    bool isResizable() const;


protected:
    PositionerNodeInstance(QDeclarativeBasePositioner *item);
};

} // namespace Internal
} // namespace QmlDesigner

#endif // POSITIONERNODEINSTANCE_H
