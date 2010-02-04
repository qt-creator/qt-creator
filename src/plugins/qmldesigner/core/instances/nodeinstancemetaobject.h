#ifndef NODEINSTANCEMETAOBJECT_H
#define NODEINSTANCEMETAOBJECT_H

#include <private/qmlopenmetaobject_p.h>
#include <private/qmlcontext_p.h>

namespace QmlDesigner {
namespace Internal {

class NodeInstanceMetaObject : public QmlOpenMetaObject
{
public:
    NodeInstanceMetaObject(QObject *object, QmlContext *context);
    void createNewProperty(const QString &name);

protected:
    int metaCall(QMetaObject::Call _c, int _id, void **_a);
    void propertyWritten(int);

private:
    QWeakPointer<QmlContext> m_context;
};

} // namespace Internal
} // namespace QmlDesigner

#endif // NODEINSTANCEMETAOBJECT_H
