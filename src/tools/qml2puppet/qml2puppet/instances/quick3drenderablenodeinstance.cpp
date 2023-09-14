// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "quick3drenderablenodeinstance.h"
#include "qt5nodeinstanceserver.h"
#include "quickitemnodeinstance.h"
#include "../editor3d/generalhelper.h"

#ifdef QUICK3D_MODULE
#include <private/qquick3dobject_p.h>
#include <private/qquickstategroup_p.h>
#endif


namespace QmlDesigner {
namespace Internal {

const QRectF preview3dBoundingRect(0, 0, 640, 480);

Quick3DRenderableNodeInstance::Quick3DRenderableNodeInstance(QObject *node)
   : ObjectNodeInstance(node)
{
}

Quick3DRenderableNodeInstance::~Quick3DRenderableNodeInstance()
{
    delete m_dummyRootView;
}

void Quick3DRenderableNodeInstance::initialize(const ObjectNodeInstance::Pointer &objectNodeInstance,
                                               InstanceContainer::NodeFlags flags)
{
#ifdef QUICK3D_MODULE
    // In case this is the scene root, we need to create a dummy View3D for the scene
    // in preview puppets
    if (instanceId() == 0 && (!nodeInstanceServer()->isInformationServer())) {
        nodeInstanceServer()->quickWindow()->setDefaultAlphaBuffer(true);
        nodeInstanceServer()->quickWindow()->setColor(Qt::transparent);

        auto helper = new QmlDesigner::Internal::GeneralHelper();
        engine()->rootContext()->setContextProperty("_generalHelper", helper);

        QQmlComponent component(engine());
        component.loadUrl(QUrl("qrc:/qtquickplugin/mockfiles/qt6/ModelNode3DImageView.qml"));
        m_dummyRootView = qobject_cast<QQuickItem *>(component.create());

        invokeDummyViewCreate();

        nodeInstanceServer()->setRootItem(m_dummyRootView);
    }
#endif // QUICK3D_MODULE
    ObjectNodeInstance::initialize(objectNodeInstance, flags);
}

QImage Quick3DRenderableNodeInstance::renderImage() const
{
    if (!isRootNodeInstance() || !m_dummyRootView)
        return {};

    QSize size = preview3dBoundingRect.size().toSize();
    nodeInstanceServer()->quickWindow()->resize(size);
    m_dummyRootView->setSize(size);

    // Just render the window once to update spatial nodes
    nodeInstanceServer()->renderWindow();

    QMetaObject::invokeMethod(m_dummyRootView, "fitToViewPort", Qt::DirectConnection);

    QRectF renderBoundingRect = m_dummyRootView->boundingRect();
    QImage renderImage;

    if (QuickItemNodeInstance::unifiedRenderPath()) {
        renderImage = nodeInstanceServer()->grabWindow();
        renderImage = renderImage.copy(renderBoundingRect.toRect());
    } else {
        renderImage = nodeInstanceServer()->grabItem(m_dummyRootView);
    }

    // When grabbing an offscreen window the device pixel ratio is 1
    renderImage.setDevicePixelRatio(1);

    return renderImage;
}

QImage Quick3DRenderableNodeInstance::renderPreviewImage(
    [[maybe_unused]] const QSize &previewImageSize) const
{
    if (!isRootNodeInstance() || !m_dummyRootView)
        return {};

    nodeInstanceServer()->quickWindow()->resize(previewImageSize);
    m_dummyRootView->setSize(previewImageSize);

    // Just render the window once to update spatial nodes
    nodeInstanceServer()->renderWindow();

    QMetaObject::invokeMethod(m_dummyRootView, "fitToViewPort", Qt::DirectConnection);

    QRectF previewItemBoundingRect = boundingRect();

    if (previewItemBoundingRect.isValid()) {
        const QSize size = previewImageSize;
        if (m_dummyRootView->isVisible()) {
            QImage image;
            image = nodeInstanceServer()->grabWindow();
            image = image.copy(previewItemBoundingRect.toRect());
            image = image.scaledToWidth(size.width());
            return image;
        } else {
            QImage transparentImage(size, QImage::Format_ARGB32_Premultiplied);
            transparentImage.fill(Qt::transparent);
            return transparentImage;
        }
    }
    return {};
}

bool Quick3DRenderableNodeInstance::isRenderable() const
{
    return m_dummyRootView;
}

bool Quick3DRenderableNodeInstance::hasContent() const
{
    return true;
}

QRectF Quick3DRenderableNodeInstance::boundingRect() const
{
    //The information server has no m_dummyRootView therefore we use the hardcoded value
    if (nodeInstanceServer()->isInformationServer())
        return preview3dBoundingRect;

    if (m_dummyRootView)
        return m_dummyRootView->boundingRect();
    return ObjectNodeInstance::boundingRect();
}

QRectF Quick3DRenderableNodeInstance::contentItemBoundingBox() const
{
    return boundingRect();
}

QPointF Quick3DRenderableNodeInstance::position() const
{
    return QPointF(0, 0);
}

QSizeF Quick3DRenderableNodeInstance::size() const
{
    return boundingRect().size();
}

QList<ServerNodeInstance> Quick3DRenderableNodeInstance::stateInstances() const
{
    QList<ServerNodeInstance> instanceList;
#ifdef QUICK3D_MODULE
    if (auto obj3D = qobject_cast<QQuick3DObject *>(object())) {
        const QList<QQuickState *> stateList = QQuick3DObjectPrivate::get(obj3D)->_states()->states();
        for (QQuickState *state : stateList) {
            if (state && nodeInstanceServer()->hasInstanceForObject(state))
                instanceList.append(nodeInstanceServer()->instanceForObject(state));
        }
    }
#endif
    return instanceList;
}

QQuickItem *Quick3DRenderableNodeInstance::contentItem() const
{
    return m_dummyRootView;
}

void Quick3DRenderableNodeInstance::setPropertyVariant(const PropertyName &name,
                                                       const QVariant &value)
{
    if (m_dummyRootView && name == "isLibraryIcon")
        QMetaObject::invokeMethod(m_dummyRootView, "setIconMode", Q_ARG(QVariant, value));
    ObjectNodeInstance::setPropertyVariant(name, value);
}

Qt5NodeInstanceServer *Quick3DRenderableNodeInstance::qt5NodeInstanceServer() const
{
    return qobject_cast<Qt5NodeInstanceServer *>(nodeInstanceServer());
}

void Quick3DRenderableNodeInstance::invokeDummyViewCreate() const
{
}

} // namespace Internal
} // namespace QmlDesigner

