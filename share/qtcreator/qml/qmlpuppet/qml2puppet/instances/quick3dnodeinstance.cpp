/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "quick3dnodeinstance.h"
#include "qt5nodeinstanceserver.h"
#include "qt5informationnodeinstanceserver.h"
#include "quickitemnodeinstance.h"
#include "../editor3d/generalhelper.h"

#include <qmlprivategate.h>

#include <QDebug>
#include <QHash>
#include <QQmlExpression>
#include <QQmlProperty>

#include <cmath>

#ifdef QUICK3D_MODULE
#include <private/qquick3dobject_p.h>
#include <private/qquick3dnode_p.h>
#include <private/qquick3dmodel_p.h>
#include <private/qquick3dnode_p_p.h>
#include <private/qquick3drepeater_p.h>
#include <private/qquick3dloader_p.h>
#if defined(QUICK3D_ASSET_UTILS_MODULE) && QT_VERSION > QT_VERSION_CHECK(6, 2, 0)
#include <private/qquick3druntimeloader_p.h>
#endif
#include <private/qquickstategroup_p.h>
#endif


namespace QmlDesigner {
namespace Internal {

const QRectF preview3dBoundingRect(0, 0, 640, 480);

Quick3DNodeInstance::Quick3DNodeInstance(QObject *node)
   : ObjectNodeInstance(node)
{
}

Quick3DNodeInstance::~Quick3DNodeInstance()
{
    delete m_dummyRootView;
}

void Quick3DNodeInstance::initialize(const ObjectNodeInstance::Pointer &objectNodeInstance,
                                     InstanceContainer::NodeFlags flags)
{
#ifdef QUICK3D_MODULE
    QObject *obj = object();
    auto repObj = qobject_cast<QQuick3DRepeater *>(obj);
    auto loadObj = qobject_cast<QQuick3DLoader *>(obj);
#if defined(QUICK3D_ASSET_UTILS_MODULE) && QT_VERSION > QT_VERSION_CHECK(6, 2, 0)
    auto runLoadObj = qobject_cast<QQuick3DRuntimeLoader *>(obj);
    if (repObj || loadObj || runLoadObj) {
#else
    if (repObj || loadObj) {
#endif
        if (auto infoServer = qobject_cast<Qt5InformationNodeInstanceServer *>(nodeInstanceServer())) {
            if (repObj) {
                QObject::connect(repObj, &QQuick3DRepeater::objectAdded,
                                 infoServer, &Qt5InformationNodeInstanceServer::handleDynamicAddObject);
#if defined(QUICK3D_ASSET_UTILS_MODULE) && QT_VERSION > QT_VERSION_CHECK(6, 2, 0)
            } else if (runLoadObj) {
                QObject::connect(runLoadObj, &QQuick3DRuntimeLoader::statusChanged,
                                 infoServer, &Qt5InformationNodeInstanceServer::handleDynamicAddObject);
#endif
            } else {
                QObject::connect(loadObj, &QQuick3DLoader::loaded,
                                 infoServer, &Qt5InformationNodeInstanceServer::handleDynamicAddObject);
            }
        }
    }

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    // In case this is the scene root, we need to create a dummy View3D for the scene
    // in preview puppets
    if (instanceId() == 0 && (!nodeInstanceServer()->isInformationServer())) {
        auto helper = new QmlDesigner::Internal::GeneralHelper();
        engine()->rootContext()->setContextProperty("_generalHelper", helper);

        QQmlComponent component(engine());
        component.loadUrl(QUrl("qrc:/qtquickplugin/mockfiles/qt6/ModelNode3DImageView.qml"));
        m_dummyRootView = qobject_cast<QQuickItem *>(component.create());

        QMetaObject::invokeMethod(
                    m_dummyRootView, "createViewForNode",
                    Q_ARG(QVariant, QVariant::fromValue(object())));

        nodeInstanceServer()->setRootItem(m_dummyRootView);
    }
#endif // QT_VERSION
#endif // QUICK3D_MODULE
    ObjectNodeInstance::initialize(objectNodeInstance, flags);
}

QImage Quick3DNodeInstance::renderImage() const
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
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
#endif
    return {};
}

QImage Quick3DNodeInstance::renderPreviewImage(const QSize &previewImageSize) const
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
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
#else
    Q_UNUSED(previewImageSize)
#endif
    return {};
}

bool Quick3DNodeInstance::isRenderable() const
{
    return m_dummyRootView;
}

bool Quick3DNodeInstance::hasContent() const
{
    return true;
}

QRectF Quick3DNodeInstance::boundingRect() const
{
    //The information server has no m_dummyRootView therefore we use the hardcoded value
    if (nodeInstanceServer()->isInformationServer())
        return preview3dBoundingRect;

    if (m_dummyRootView)
        return m_dummyRootView->boundingRect();
    return ObjectNodeInstance::boundingRect();
}

QRectF Quick3DNodeInstance::contentItemBoundingBox() const
{
    return boundingRect();
}

QPointF Quick3DNodeInstance::position() const
{
    return QPointF(0, 0);
}

QSizeF Quick3DNodeInstance::size() const
{
    return boundingRect().size();
}

QList<ServerNodeInstance> Quick3DNodeInstance::stateInstances() const
{
    QList<ServerNodeInstance> instanceList;
#ifdef QUICK3D_MODULE
    if (auto obj3D = quick3DNode()) {
        const QList<QQuickState *> stateList = QQuick3DObjectPrivate::get(obj3D)->_states()->states();
        for (QQuickState *state : stateList) {
            if (state && nodeInstanceServer()->hasInstanceForObject(state))
                instanceList.append(nodeInstanceServer()->instanceForObject(state));
        }
    }
#endif
    return instanceList;
}

QQuickItem *Quick3DNodeInstance::contentItem() const
{
    return m_dummyRootView;
}

Qt5NodeInstanceServer *Quick3DNodeInstance::qt5NodeInstanceServer() const
{
    return qobject_cast<Qt5NodeInstanceServer *>(nodeInstanceServer());
}

QQuick3DNode *Quick3DNodeInstance::quick3DNode() const
{
#ifdef QUICK3D_MODULE
    return qobject_cast<QQuick3DNode *>(object());
#else
    return nullptr;
#endif
}

Quick3DNodeInstance::Pointer Quick3DNodeInstance::create(QObject *object)
{
    Pointer instance(new Quick3DNodeInstance(object));
    instance->populateResetHashes();
    return instance;
}

void Quick3DNodeInstance::setHiddenInEditor(bool b)
{
    ObjectNodeInstance::setHiddenInEditor(b);
#ifdef QUICK3D_MODULE
    QQuick3DNodePrivate *privateNode = QQuick3DNodePrivate::get(quick3DNode());
    if (privateNode)
        privateNode->setIsHiddenInEditor(b);
#endif
}

} // namespace Internal
} // namespace QmlDesigner

