#ifndef BEHAVIORNODEINSTANCE_H
#define BEHAVIORNODEINSTANCE_H

#include "objectnodeinstance.h"

namespace QmlDesigner {
namespace Internal {

class BehaviorNodeInstance : public ObjectNodeInstance
{
public:
    typedef QSharedPointer<BehaviorNodeInstance> Pointer;
    typedef QWeakPointer<BehaviorNodeInstance> WeakPointer;

    BehaviorNodeInstance(QObject *object);

    static Pointer create(const NodeMetaInfo &metaInfo, QDeclarativeContext *context, QObject *objectToBeWrapped);

    void setPropertyVariant(const QString &name, const QVariant &value);
    void setPropertyBinding(const QString &name, const QString &expression);


    QVariant property(const QString &name) const;
    void resetProperty(const QString &name);

private:
    bool m_isEnabled;
};

} // namespace Internal
} // namespace QmlDesigner

#endif // BEHAVIORNODEINSTANCE_H
