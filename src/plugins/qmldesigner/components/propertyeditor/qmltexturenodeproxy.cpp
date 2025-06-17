// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmltexturenodeproxy.h"

#include "propertyeditortracing.h"

#include <abstractview.h>
#include <createtexture.h>
#include <designmodewidget.h>
#include <model.h>
#include <qmldesignerplugin.h>

#include <qqml.h>

namespace QmlDesigner {

using QmlDesigner::PropertyEditorTracing::category;

using namespace Qt::StringLiterals;

QmlTextureNodeProxy::QmlTextureNodeProxy()
{
    NanotraceHR::Tracer tracer{"qml texture node proxy constructor", category()};
}

QmlTextureNodeProxy::~QmlTextureNodeProxy()
{
    NanotraceHR::Tracer tracer{"qml texture node proxy destructor", category()};
}

void QmlTextureNodeProxy::setup(const QmlObjectNode &objectNode)
{
    NanotraceHR::Tracer tracer{"qml texture node proxy setup", category()};

    const QmlObjectNode texture = objectNode.metaInfo().isQtQuick3DTexture() ? objectNode
                                                                             : QmlObjectNode{};

    setTextureNode(texture);
    updateSelectionDetails();
}

void QmlTextureNodeProxy::updateSelectionDetails()
{
    NanotraceHR::Tracer tracer{"qml texture node proxy update selection details", category()};

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
    NanotraceHR::Tracer tracer{"qml texture node proxy handle property changed", category()};

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
    NanotraceHR::Tracer tracer{"qml texture node proxy handle binding property changed", category()};

    handlePropertyChanged(property);
}

void QmlTextureNodeProxy::handlePropertiesRemoved(const AbstractProperty &property)
{
    NanotraceHR::Tracer tracer{"qml texture node proxy handle properties removed", category()};

    handlePropertyChanged(property);
}

QmlObjectNode QmlTextureNodeProxy::textureNode() const
{
    NanotraceHR::Tracer tracer{"qml texture node proxy texture node", category()};

    return m_textureNode;
}

bool QmlTextureNodeProxy::hasTexture() const
{
    NanotraceHR::Tracer tracer{"qml texture node proxy has texture", category()};

    return textureNode().isValid();
}

bool QmlTextureNodeProxy::selectedNodeAcceptsMaterial() const
{
    NanotraceHR::Tracer tracer{"qml texture node proxy selected node accepts material", category()};

    return m_selectedNodeAcceptsMaterial;
}

QString QmlTextureNodeProxy::resolveResourcePath(const QString &path) const
{
    NanotraceHR::Tracer tracer{"qml texture node proxy resolve resource path", category()};

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
    NanotraceHR::Tracer tracer{"qml texture node proxy toolbar action", category()};

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
    NanotraceHR::Tracer tracer{"qml texture node proxy register declarative type", category()};

    qmlRegisterType<QmlTextureNodeProxy>("HelperWidgets", 2, 0, "QmlTextureNodeProxy");
}

void QmlTextureNodeProxy::setTextureNode(const QmlObjectNode &node)
{
    NanotraceHR::Tracer tracer{"qml texture node proxy set texture node", category()};

    if (m_textureNode == node)
        return;
    m_textureNode = node;
    emit textureNodeChanged();
}

void QmlTextureNodeProxy::setSelectedNodeAcceptsMaterial(bool value)
{
    NanotraceHR::Tracer tracer{"qml texture node proxy set selected node accepts material",
                               category()};

    if (m_selectedNodeAcceptsMaterial == value)
        return;
    m_selectedNodeAcceptsMaterial = value;
    emit selectedNodeAcceptsMaterialChanged();
}

bool QmlTextureNodeProxy::hasQuick3DImport() const
{
    NanotraceHR::Tracer tracer{"qml texture node proxy has quick3d import", category()};

    return textureNode().isValid() && textureNode().model()->hasImport("QtQuick3D"_L1);
}

} // namespace QmlDesigner
