// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "abstractview.h"
#include "qmlmodelnodeproxy.h"

#include <QtQml>

namespace QmlDesigner {

QmlModelNodeProxy::QmlModelNodeProxy(QObject *parent) :
    QObject(parent)
{
}

void QmlModelNodeProxy::setup(const QmlObjectNode &objectNode)
{
    m_qmlObjectNode = objectNode;

    emit modelNodeChanged();
}

void QmlModelNodeProxy::registerDeclarativeType()
{
    qmlRegisterType<QmlModelNodeProxy>("HelperWidgets",2,0,"ModelNodeProxy");
}

void QmlModelNodeProxy::emitSelectionToBeChanged()
{
    emit selectionToBeChanged();
}

void QmlModelNodeProxy::emitSelectionChanged()
{
    emit selectionChanged();
}

QmlObjectNode QmlModelNodeProxy::qmlObjectNode() const
{
    return m_qmlObjectNode;
}

ModelNode QmlModelNodeProxy::modelNode() const
{
    return m_qmlObjectNode.modelNode();
}

bool QmlModelNodeProxy::multiSelection() const
{
    if (!m_qmlObjectNode.isValid())
        return false;

    return m_qmlObjectNode.view()->selectedModelNodes().size() > 1;
}

QString QmlModelNodeProxy::nodeId() const
{
    if (!m_qmlObjectNode.isValid())
        return {};

    if (multiSelection())
        return tr("multiselection");

    return m_qmlObjectNode.id();
}

QString QmlModelNodeProxy::simplifiedTypeName() const
{
    if (!m_qmlObjectNode.isValid())
        return {};

    if (multiSelection())
        return tr("multiselection");

    return m_qmlObjectNode.simplifiedTypeName();
}

}
