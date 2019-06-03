/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "abstractview.h"
#include "qmlmodelnodeproxy.h"

#include <QtQml>

namespace QmlDesigner {

QmlModelNodeProxy::QmlModelNodeProxy(QObject *parent) :
    QObject(parent)
{
}

void QmlModelNodeProxy::setup(const QmlItemNode &itemNode)
{
    m_qmlItemNode = itemNode;

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

QmlItemNode QmlModelNodeProxy::qmlItemNode() const
{
    return m_qmlItemNode;
}

ModelNode QmlModelNodeProxy::modelNode() const
{
    return m_qmlItemNode.modelNode();
}

bool QmlModelNodeProxy::multiSelection() const
{
    if (!m_qmlItemNode.isValid())
        return false;

    return m_qmlItemNode.view()->selectedModelNodes().count() > 1;
}

QString QmlModelNodeProxy::nodeId() const
{
    if (!m_qmlItemNode.isValid())
        return {};

    if (multiSelection())
        return tr("multiselection");

    return m_qmlItemNode.id();
}

QString QmlModelNodeProxy::simplifiedTypeName() const
{
    if (!m_qmlItemNode.isValid())
        return {};

    if (multiSelection())
        return tr("multiselection");

    return m_qmlItemNode.simplifiedTypeName();
}

}
