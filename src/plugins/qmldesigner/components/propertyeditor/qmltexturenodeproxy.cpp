// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmltexturenodeproxy.h"

#include <abstractview.h>
#include <createtexture.h>
#include <designmodewidget.h>
#include <model.h>
#include <qmldesignerplugin.h>

#include <qqml.h>

namespace QmlDesigner {

using namespace Qt::StringLiterals;

QmlTextureNodeProxy::QmlTextureNodeProxy() = default;

QmlTextureNodeProxy::~QmlTextureNodeProxy() = default;

void QmlTextureNodeProxy::setup(const QmlObjectNode &objectNode)
{
    const QmlObjectNode texture = objectNode.metaInfo().isQtQuick3DTexture() ? objectNode
                                                                             : QmlObjectNode{};

    setTextureNode(texture);
    updateSelectionDetails();
}

void QmlTextureNodeProxy::updateSelectionDetails()
{
    QScopeGuard falseSetter{
        std::bind_front(&QmlTextureNodeProxy::setSelectedNodeAcceptsMaterial, this, false)};

    if (!textureNode())
        return;

    QmlObjectNode selectedNode = textureNode().view()->singleSelectedModelNode();
    if (!selectedNode)
        return;

    falseSetter.dismiss();
    setSelectedNodeAcceptsMaterial(selectedNode.hasBindingProperty("materials"));
}

void QmlTextureNodeProxy::handlePropertyChanged(const AbstractProperty &property)
{
    if (!textureNode())
        return;

    QmlObjectNode node = property.parentModelNode();
    if (!node)
        return;

    QmlObjectNode selectedNode = textureNode().view()->singleSelectedModelNode();
    if (!selectedNode)
        return;

    if (property.name() == "materials"_L1
        && (selectedNode == node || selectedNode.propertyChangeForCurrentState() == node)) {
        updateSelectionDetails();
    }
}

void QmlTextureNodeProxy::handleBindingPropertyChanged(const BindingProperty &property)
{
    handlePropertyChanged(property);
}

void QmlTextureNodeProxy::handlePropertiesRemoved(const AbstractProperty &property)
{
    handlePropertyChanged(property);
}

QmlObjectNode QmlTextureNodeProxy::textureNode() const
{
    return m_textureNode;
}

bool QmlTextureNodeProxy::hasTexture() const
{
    return textureNode().isValid();
}

bool QmlTextureNodeProxy::selectedNodeAcceptsMaterial() const
{
    return m_selectedNodeAcceptsMaterial;
}

QString QmlTextureNodeProxy::resolveResourcePath(const QString &path) const
{
    if (path.isEmpty())
        return ":/propertyeditor/images/texture_default.png";

    if (Utils::FilePath::fromString(path).isAbsolutePath())
        return path;

    return QmlDesignerPlugin::instance()
        ->documentManager()
        .currentDesignDocument()
        ->fileName()
        .absolutePath()
        .pathAppended(path)
        .cleanPath()
        .toUrlishString();
}

void QmlTextureNodeProxy::toolbarAction(int action)
{
    if (!hasQuick3DImport())
        return;

    switch (action) {
    case ToolBarAction::ApplyToSelected: {
        if (!textureNode())
            return;
        AbstractView *view = textureNode().view();
        QmlDesignerPlugin::instance()->mainWidget()->showDockWidget("MaterialBrowser");
        ModelNode targetNode = view->singleSelectedModelNode();
        view->emitCustomNotification("apply_texture_to_model3D", {targetNode, textureNode()});
    } break;

    case ToolBarAction::AddNewTexture: {
        if (!textureNode())
            break;
        ModelNode newTexture = CreateTexture(textureNode().view()).execute();
        QTimer::singleShot(0, this, [newTexture]() {
            if (newTexture)
                newTexture.model()->setSelectedModelNodes({newTexture});
        });
    } break;

    case ToolBarAction::DeleteCurrentTexture: {
        if (textureNode()) {
            AbstractView *view = textureNode().view();
            view->executeInTransaction(__FUNCTION__, [&] { textureNode().destroy(); });
        }
    } break;

    case ToolBarAction::OpenMaterialBrowser: {
        QmlDesignerPlugin::instance()->mainWidget()->showDockWidget("MaterialBrowser", true);
    } break;
    }
}

void QmlTextureNodeProxy::registerDeclarativeType()
{
    qmlRegisterType<QmlTextureNodeProxy>("HelperWidgets", 2, 0, "QmlTextureNodeProxy");
}

void QmlTextureNodeProxy::setTextureNode(const QmlObjectNode &node)
{
    if (m_textureNode == node)
        return;
    m_textureNode = node;
    emit textureNodeChanged();
}

void QmlTextureNodeProxy::setSelectedNodeAcceptsMaterial(bool value)
{
    if (m_selectedNodeAcceptsMaterial == value)
        return;
    m_selectedNodeAcceptsMaterial = value;
    emit selectedNodeAcceptsMaterialChanged();
}

bool QmlTextureNodeProxy::hasQuick3DImport() const
{
    return textureNode().isValid() && textureNode().model()->hasImport("QtQuick3D"_L1);
}

} // namespace QmlDesigner
