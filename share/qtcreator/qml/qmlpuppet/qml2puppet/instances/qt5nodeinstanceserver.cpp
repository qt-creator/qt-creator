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

#include "qt5nodeinstanceserver.h"

#include <QSurfaceFormat>

#include <QQmlFileSelector>

#include <QQuickItem>
#include <QQuickView>
#include <QQuickWindow>

#include <designersupportdelegate.h>
#include <addimportcontainer.h>
#include <createscenecommand.h>
#include <reparentinstancescommand.h>
#include <clearscenecommand.h>

#include <QDebug>
#include <QOpenGLContext>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QtGui/private/qrhi_p.h>
#include <QtQuick/private/qquickwindow_p.h>
#include <QtQuick/private/qsgrenderer_p.h>
#include <QtQuick/private/qquickrendercontrol_p.h>
#include <QtQuick/private/qquickrendertarget_p.h>
#endif

namespace QmlDesigner {

Qt5NodeInstanceServer::Qt5NodeInstanceServer(NodeInstanceClientInterface *nodeInstanceClient)
    : NodeInstanceServer(nodeInstanceClient)
{
    DesignerSupport::activateDesignerMode();
}

Qt5NodeInstanceServer::~Qt5NodeInstanceServer()
{
    NodeInstanceServer::clearScene({});
    delete m_viewData.window.data();
}

QQuickView *Qt5NodeInstanceServer::quickView() const
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    return static_cast<QQuickView *>(m_viewData.window.data());
#else
    return nullptr;
#endif
}

QQuickWindow *Qt5NodeInstanceServer::quickWindow() const
{
    return m_viewData.window.data();
}

void Qt5NodeInstanceServer::initializeView()
{
    Q_ASSERT(!quickWindow());

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    auto view = new QQuickView;
    m_viewData.window = view;
    /* enables grab window without show */
    QSurfaceFormat surfaceFormat = view->requestedFormat();
    surfaceFormat.setVersion(4, 1);
    surfaceFormat.setProfile(QSurfaceFormat::CoreProfile);
    QSurfaceFormat::setDefaultFormat(surfaceFormat);
    view->setFormat(surfaceFormat);

    DesignerSupport::createOpenGLContext(view);
    m_qmlEngine = view->engine();
#else
    m_viewData.renderControl = new QQuickRenderControl;
    m_viewData.window = new QQuickWindow(m_viewData.renderControl);
    m_viewData.renderControl->initialize();
    m_qmlEngine = new QQmlEngine;
#endif

    if (qEnvironmentVariableIsSet("QML_FILE_SELECTORS")) {
        QQmlFileSelector *fileSelector = new QQmlFileSelector(engine(), engine());
        QStringList customSelectors = QString::fromUtf8(qgetenv("QML_FILE_SELECTORS")).split(',');
        fileSelector->setExtraSelectors(customSelectors);
    }

    initializeAuxiliaryViews();
}

QQmlView *Qt5NodeInstanceServer::declarativeView() const
{
    return nullptr;
}

QQuickItem *Qt5NodeInstanceServer::rootItem() const
{
    return m_viewData.rootItem;
}

void Qt5NodeInstanceServer::setRootItem(QQuickItem *item)
{
    m_viewData.rootItem = item;
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    DesignerSupport::setRootItem(quickView(), item);
#else
    quickWindow()->setGeometry(0, 0, item->width(), item->height());
    // Insert an extra item above the root to adjust root item position to 0,0 to make entire
    // item to be always rendered.
    if (!m_viewData.contentItem)
        m_viewData.contentItem = new QQuickItem(quickWindow()->contentItem());
    m_viewData.contentItem->setPosition(-item->position());
    item->setParentItem(m_viewData.contentItem);
#endif
}

QQmlEngine *Qt5NodeInstanceServer::engine() const
{
    return m_qmlEngine;
}

void Qt5NodeInstanceServer::resizeCanvasToRootItem()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    m_viewData.bufferDirty = true;
    if (m_viewData.contentItem)
        m_viewData.contentItem->setPosition(-m_viewData.rootItem->position());
#endif
    quickWindow()->resize(rootNodeInstance().boundingRect().size().toSize());
}

void Qt5NodeInstanceServer::resetAllItems()
{
    foreach (QQuickItem *item, allItems())
        DesignerSupport::resetDirty(item);
}

void Qt5NodeInstanceServer::setupScene(const CreateSceneCommand &command)
{
    setupMockupTypes(command.mockupTypes);
    setupFileUrl(command.fileUrl);
    setupImports(command.imports);
    setupDummyData(command.fileUrl);

    setupInstances(command);
    resizeCanvasToRootItem();
}

QList<QQuickItem*> subItems(QQuickItem *parentItem)
{
    QList<QQuickItem*> itemList;
    itemList.append(parentItem->childItems());

    foreach (QQuickItem *childItem, parentItem->childItems())
        itemList.append(subItems(childItem));

    return itemList;
}

QList<QQuickItem*> Qt5NodeInstanceServer::allItems() const
{
    if (rootNodeInstance().isValid())
        return rootNodeInstance().allItemsRecursive();

    return QList<QQuickItem*>();
}

bool Qt5NodeInstanceServer::initRhi(RenderViewData &viewData)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    if (!viewData.renderControl) {
        qWarning() << __FUNCTION__ << "Render control not created";
        return false;
    }

    if (!viewData.rhi) {
        QQuickRenderControlPrivate *rd = QQuickRenderControlPrivate::get(viewData.renderControl);
        viewData.rhi = rd->rhi;

        if (!viewData.rhi) {
            qWarning() << __FUNCTION__ << "Rhi is null";
            return false;
        }
    }

    auto cleanRhiResources = [&viewData]() {
        // Releasing cached resources is a workaround for bug QTBUG-88761
        auto renderer = QQuickWindowPrivate::get(viewData.window)->renderer;
        if (renderer)
            renderer->releaseCachedResources();

        if (viewData.rpDesc) {
            viewData.rpDesc->deleteLater();
            viewData.rpDesc = nullptr;
        }
        if (viewData.texTarget) {
            viewData.texTarget->deleteLater();
            viewData.texTarget = nullptr;
        }
        if (viewData.buffer) {
            viewData.buffer->deleteLater();
            viewData.buffer = nullptr;
        }
        if (viewData.texture) {
            viewData.texture->deleteLater();
            viewData.texture = nullptr;
        }
    };
    if (viewData.bufferDirty)
        cleanRhiResources();

    QSize size = viewData.window->size();
    if (size.isNull())
        size = QSize(2, 2); // Zero size buffer creation will fail, so make it some size always

    viewData.texture = viewData.rhi->newTexture(QRhiTexture::RGBA8, size, 1,
                                                QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource);
    if (!viewData.texture->create()) {
        qWarning() << __FUNCTION__ << "QRhiTexture creation failed";
        cleanRhiResources();
        return false;
    }

    viewData.buffer = viewData.rhi->newRenderBuffer(QRhiRenderBuffer::DepthStencil, size, 1);
    if (!viewData.buffer->create()) {
        qWarning() << __FUNCTION__ << "Depth/stencil buffer creation failed";
        cleanRhiResources();
        return false;
    }

    QRhiTextureRenderTargetDescription rtDesc(QRhiColorAttachment(viewData.texture));
    rtDesc.setDepthStencilBuffer(viewData.buffer);
    viewData.texTarget = viewData.rhi->newTextureRenderTarget(rtDesc);
    viewData.rpDesc = viewData.texTarget->newCompatibleRenderPassDescriptor();
    viewData.texTarget->setRenderPassDescriptor(viewData.rpDesc);
    if (!viewData.texTarget->create()) {
        qWarning() << __FUNCTION__ << "Texture render target creation failed";
        cleanRhiResources();
        return false;
    }

    // redirect Qt Quick rendering into our texture
    viewData.window->setRenderTarget(QQuickRenderTarget::fromRhiRenderTarget(viewData.texTarget));

    viewData.bufferDirty = false;
#else
    Q_UNUSED(viewData)
#endif
    return true;
}

QImage Qt5NodeInstanceServer::grabRenderControl(RenderViewData &viewData)
{
    QImage renderImage;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    if (viewData.bufferDirty && !initRhi(viewData))
        return renderImage;

    viewData.renderControl->polishItems();
    viewData.renderControl->beginFrame();
    viewData.renderControl->sync();
    viewData.renderControl->render();

    bool readCompleted = false;
    QRhiReadbackResult readResult;
    readResult.completed = [&] {
        readCompleted = true;
        QImage wrapperImage(reinterpret_cast<const uchar *>(readResult.data.constData()),
                            readResult.pixelSize.width(), readResult.pixelSize.height(),
                            QImage::Format_RGBA8888_Premultiplied);
        if (viewData.rhi->isYUpInFramebuffer())
            renderImage = wrapperImage.mirrored();
        else
            renderImage = wrapperImage.copy();
    };
    QRhiResourceUpdateBatch *readbackBatch = viewData.rhi->nextResourceUpdateBatch();
    readbackBatch->readBackTexture(viewData.texture, &readResult);

    QQuickRenderControlPrivate *rd = QQuickRenderControlPrivate::get(viewData.renderControl);
    rd->cb->resourceUpdate(readbackBatch);

    viewData.renderControl->endFrame();
#else
    Q_UNUSED(viewData)
#endif
    return renderImage;
}

// This method simply renders the window without grabbing it
bool Qt5NodeInstanceServer::renderWindow()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    if (!m_viewData.rootItem || (m_viewData.bufferDirty && !initRhi(m_viewData)))
        return false;

    m_viewData.renderControl->polishItems();
    m_viewData.renderControl->beginFrame();
    m_viewData.renderControl->sync();
    m_viewData.renderControl->render();
    m_viewData.renderControl->endFrame();
    return true;
#endif
    return false;
}

QImage Qt5NodeInstanceServer::grabWindow()
{
    if (m_viewData.rootItem)
        return grabRenderControl(m_viewData);
    return  {};
}

void Qt5NodeInstanceServer::refreshBindings()
{
    DesignerSupport::refreshExpressions(context());
}

DesignerSupport *Qt5NodeInstanceServer::designerSupport()
{
    return &m_designerSupport;
}

void Qt5NodeInstanceServer::createScene(const CreateSceneCommand &command)
{
    NodeInstanceServer::createScene(command);
}

void Qt5NodeInstanceServer::clearScene(const ClearSceneCommand &command)
{
    NodeInstanceServer::clearScene(command);
}

void Qt5NodeInstanceServer::reparentInstances(const ReparentInstancesCommand &command)
{
    NodeInstanceServer::reparentInstances(command.reparentInstances());
    startRenderTimer();
}


} // QmlDesigner
