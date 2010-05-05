#ifndef NODEINSTANCESIGNALSPY_H
#define NODEINSTANCESIGNALSPY_H

#include <QObject>
#include <QHash>
#include <QSharedPointer>

namespace QmlDesigner {
namespace Internal {

class ObjectNodeInstance;
typedef QSharedPointer<ObjectNodeInstance> ObjectNodeInstancePointer;
typedef QWeakPointer<ObjectNodeInstance> ObjectNodeInstanceWeakPointer;

class NodeInstanceSignalSpy : public QObject
{
public:
    explicit NodeInstanceSignalSpy();

    void setObjectNodeInstance(const ObjectNodeInstancePointer &nodeInstance);

    virtual int qt_metacall(QMetaObject::Call, int, void **);

protected:
    void registerObject(QObject *spiedObject, const QString &prefix = QString());

private:
    int methodeOffset;
    QHash<int, QString> m_indexPropertyHash;
    ObjectNodeInstanceWeakPointer m_objectNodeInstance;
};

} // namespace Internal
} // namespace QmlDesigner

#endif // NODEINSTANCESIGNALSPY_H
