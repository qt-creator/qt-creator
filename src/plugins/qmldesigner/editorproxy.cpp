// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
