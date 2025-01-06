// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

bool DummyContextObject::runningInDesigner() const
{
    return true;
}

} // namespace QmlDesigner
