#ifndef NODEINSTANCEMETAOBJECT_H
#define NODEINSTANCEMETAOBJECT_H

#include <QDeclarativeContext>
#include <private/qdeclarativeopenmetaobject_p.h>

namespace QmlDesigner {
namespace Internal {

class ObjectNodeInstance;
typedef QSharedPointer<ObjectNodeInstance> ObjectNodeInstancePointer;
typedef QWeakPointer<ObjectNodeInstance> ObjectNodeInstanceWeakPointer;

class NodeInstanceMetaObject : public QDeclarativeOpenMetaObject
{
public:
    NodeInstanceMetaObject(const ObjectNodeInstancePointer &nodeInstance);
    NodeInstanceMetaObject(const ObjectNodeInstancePointer &nodeInstance, QObject *object, const QString &prefix);
    void createNewProperty(const QString &name);

protected:
    int metaCall(QMetaObject::Call _c, int _id, void **_a);
    void dynamicPropertyWritten(int);
    void notifyPropertyChange(int id);

private:
    ObjectNodeInstanceWeakPointer m_nodeInstance;
    QString m_prefix;
    QWeakPointer<QDeclarativeContext> m_context;
};

} // namespace Internal
} // namespace QmlDesigner

#endif // NODEINSTANCEMETAOBJECT_H
