#ifndef DUMMYCONTEXTOBJECT_H
#define DUMMYCONTEXTOBJECT_H

#include <QObject>
#include <QWeakPointer>
#include <qdeclarative.h>

namespace QmlDesigner {

class DummyContextObject : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QObject * parent READ parentDummy WRITE setParentDummy NOTIFY parentDummyChanged DESIGNABLE false FINAL)

public:
    explicit DummyContextObject(QObject *parent = 0);

    QObject *parentDummy() const;
    void setParentDummy(QObject *parentDummy);

signals:
    void parentDummyChanged();

private:
    Q_DISABLE_COPY(DummyContextObject)
    QWeakPointer<QObject> m_dummyParent;

};

} // namespace QmlDesigner

QML_DECLARE_TYPE(QmlDesigner::DummyContextObject)
#endif // DUMMYCONTEXTOBJECT_H
