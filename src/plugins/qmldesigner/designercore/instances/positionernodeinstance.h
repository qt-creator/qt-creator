#ifndef POSITIONERNODEINSTANCE_H
#define POSITIONERNODEINSTANCE_H

#include "qmlgraphicsitemnodeinstance.h"

class QDeclarativeBasePositioner;

namespace QmlDesigner {
namespace Internal {

class PositionerNodeInstance : public QmlGraphicsItemNodeInstance
{
public:
    typedef QSharedPointer<PositionerNodeInstance> Pointer;
    typedef QWeakPointer<PositionerNodeInstance> WeakPointer;

    static Pointer create(const NodeMetaInfo &metaInfo, QDeclarativeContext *context, QObject *objectToBeWrapped);

    void setPropertyVariant(const QString &name, const QVariant &value);
    void setPropertyBinding(const QString &name, const QString &expression);

    bool isPositioner() const;

    bool isResizable() const;


protected:
    PositionerNodeInstance(QDeclarativeBasePositioner *item, bool hasContent);
};

} // namespace Internal
} // namespace QmlDesigner

#endif // POSITIONERNODEINSTANCE_H
