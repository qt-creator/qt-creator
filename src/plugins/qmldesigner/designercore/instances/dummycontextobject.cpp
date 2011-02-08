#include "dummycontextobject.h"

namespace QmlDesigner {

DummyContextObject::DummyContextObject(QObject *parent) :
    QObject(parent)
{
}

QObject *DummyContextObject::parentDummy() const
{
    return m_dummyParent.data();
}

void DummyContextObject::setParentDummy(QObject *parentDummy)
{
    if (m_dummyParent.data() != parentDummy) {
        m_dummyParent = parentDummy;
        emit parentDummyChanged();
    }
}

} // namespace QmlDesigner
