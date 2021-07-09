/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "editorproxy.h"
#include "qmlmodelnodeproxy.h"

#include <QWidget>

namespace QmlDesigner {

EditorProxy::EditorProxy(QObject *parent)
    : QObject(parent)
{}

EditorProxy::~EditorProxy()
{
    EditorProxy::hideWidget();
}

void EditorProxy::showWidget()
{
    if ((m_widget = createWidget())) {
        m_widget->setAttribute(Qt::WA_DeleteOnClose);
        m_widget->show();
        m_widget->raise();
    }
}

void EditorProxy::showWidget(int x, int y)
{
    showWidget();
    if (m_widget) {
        m_widget->move(x, y);
    }
}

void EditorProxy::hideWidget()
{
    if (m_widget)
        m_widget->close();
    m_widget = nullptr;
}

QWidget *EditorProxy::widget() const
{
    return m_widget;
}

ModelNodeEditorProxy::ModelNodeEditorProxy(QObject *parent)
    : EditorProxy(parent)
{}

ModelNodeEditorProxy::~ModelNodeEditorProxy() {}

ModelNode ModelNodeEditorProxy::modelNode() const
{
    return m_modelNode;
}

void ModelNodeEditorProxy::setModelNode(const ModelNode &modelNode)
{
    m_modelNodeBackend = {};
    m_modelNode = modelNode;
}

void ModelNodeEditorProxy::setModelNodeBackend(const QVariant &modelNodeBackend)
{
    if (!modelNodeBackend.isNull() && modelNodeBackend.isValid()) {
        const auto modelNodeBackendObject = modelNodeBackend.value<QObject *>();
        const auto backendObjectCasted = qobject_cast<const QmlDesigner::QmlModelNodeProxy *>(
            modelNodeBackendObject);

        if (backendObjectCasted)
            m_modelNode = backendObjectCasted->qmlObjectNode().modelNode();
        m_modelNodeBackend = modelNodeBackend;

        emit modelNodeBackendChanged();
    }
}

QVariant ModelNodeEditorProxy::modelNodeBackend() const
{
    return m_modelNodeBackend;
}

bool ModelNodeEditorProxy::hasCustomId() const
{
    return m_modelNode.isValid() ? m_modelNode.hasCustomId() : false;
}

bool ModelNodeEditorProxy::hasAnnotation() const
{
    return m_modelNode.isValid() ? m_modelNode.hasAnnotation() : false;
}

} // namespace QmlDesigner
