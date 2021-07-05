/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "iconrenderer.h"
#include "../editor3d/selectionboxgeometry.h"
#include "../editor3d/generalhelper.h"

#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlproperty.h>
#include <QtQml/qqmlcontext.h>
#include <QtQuick/qquickview.h>
#include <QtQuick/qquickitem.h>
#include <QtGui/qsurfaceformat.h>
#include <QtGui/qimage.h>
#include <QtGui/qguiapplication.h>
#include <QtCore/qtimer.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qdir.h>

#ifdef QUICK3D_MODULE
#include <QtQuick3D/private/qquick3dnode_p.h>
#include <QtQuick3D/private/qquick3dviewport_p.h>
#endif

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QtGui/private/qrhi_p.h>
#include <QtQuick/private/qquickrendercontrol_p.h>
#include <QtQuick/private/qquickrendertarget_p.h>
#endif

#include <private/qquickdesignersupportitems_p.h>

IconRenderer::IconRenderer(int size, const QString &filePath, const QString &source)
    : QObject(nullptr)
    , m_size(size)
    , m_filePath(filePath)
    , m_source(source)
{
}

void IconRenderer::setupRender()
{
    DesignerSupport::activateDesignerMode();
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    DesignerSupport::activateDesignerWindowManager();
#endif

    QQmlEngine *engine = nullptr;
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    auto view = new QQuickView;
    engine = view->engine();
    m_window = view;
    QSurfaceFormat surfaceFormat = view->requestedFormat();
    surfaceFormat.setVersion(4, 1);
    surfaceFormat.setProfile(QSurfaceFormat::CoreProfile);
    view->setFormat(surfaceFormat);
    DesignerSupport::createOpenGLContext(view);
#else
    engine = new QQmlEngine;
    m_renderControl = new QQuickRenderControl;
    m_window = new QQuickWindow(m_renderControl);
    m_window->setDefaultAlphaBuffer(true);
    m_window->setColor(Qt::transparent);
    m_renderControl->initialize();
#endif

    QQmlComponent component(engine);
    component.loadUrl(QUrl::fromLocalFile(m_source));
    QObject *iconItem = component.create();

    if (iconItem) {
#ifdef QUICK3D_MODULE
        if (auto scene = qobject_cast<QQuick3DNode *>(iconItem)) {
            qmlRegisterType<QmlDesigner::Internal::SelectionBoxGeometry>("SelectionBoxGeometry", 1, 0, "SelectionBoxGeometry");
            QQmlComponent component(engine);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            component.loadUrl(QUrl("qrc:/qtquickplugin/mockfiles/qt5/IconRenderer3D.qml"));
            m_containerItem = qobject_cast<QQuickItem *>(component.create());
            DesignerSupport::setRootItem(view, m_containerItem);
#else
            component.loadUrl(QUrl("qrc:/qtquickplugin/mockfiles/qt6/IconRenderer3D.qml"));
            m_containerItem = qobject_cast<QQuickItem *>(component.create());
            m_window->contentItem()->setSize(m_containerItem->size());
            m_window->setGeometry(0, 0, m_containerItem->width(), m_containerItem->height());
            m_containerItem->setParentItem(m_window->contentItem());
#endif

            auto helper = new QmlDesigner::Internal::GeneralHelper();
            engine->rootContext()->setContextProperty("_generalHelper", helper);

            m_contentItem = QQmlProperty::read(m_containerItem, "view3D").value<QQuickItem *>();
            auto view3D = qobject_cast<QQuick3DViewport *>(m_contentItem);
            view3D->setImportScene(scene);
            m_is3D = true;
        } else
#endif
        if (auto scene = qobject_cast<QQuickItem *>(iconItem)) {
            m_contentItem = scene;
            m_containerItem = new QQuickItem();
            m_containerItem->setSize(QSizeF(1024, 1024));
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            DesignerSupport::setRootItem(view, m_containerItem);
#else
            m_window->contentItem()->setSize(m_containerItem->size());
            m_window->setGeometry(0, 0, m_containerItem->width(), m_containerItem->height());
            m_containerItem->setParentItem(m_window->contentItem());
#endif
            m_contentItem->setParentItem(m_containerItem);
        }

        if (m_containerItem && m_contentItem) {
            resizeContent(m_size);
            if (!initRhi())
                QTimer::singleShot(0, qGuiApp, &QGuiApplication::quit);
            QTimer::singleShot(0, this, &IconRenderer::startCreateIcon);
        } else {
            QTimer::singleShot(0, qGuiApp, &QGuiApplication::quit);
        }
    } else {
        QTimer::singleShot(0, qGuiApp, &QGuiApplication::quit);
    }
}

void IconRenderer::startCreateIcon()
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    m_designerSupport.refFromEffectItem(m_containerItem, false);
#endif
    QQuickDesignerSupportItems::disableNativeTextRendering(m_containerItem);

    if (m_is3D)
        QTimer::singleShot(0, this, &IconRenderer::focusCamera);
    else
        QTimer::singleShot(0, this, &IconRenderer::finishCreateIcon);
}

void IconRenderer::focusCamera()
{
#ifdef QUICK3D_MODULE
    if (m_focusStep >= 10) {
        QTimer::singleShot(0, this, &IconRenderer::finishCreateIcon);
        return;
    }

    render({});

    if (m_focusStep == 0) {
        QMetaObject::invokeMethod(m_containerItem, "setSceneToBox");
    } else if (m_focusStep > 1 && m_focusStep < 10) {
        QMetaObject::invokeMethod(m_containerItem, "fitAndHideBox");
    }
    ++m_focusStep;
    QTimer::singleShot(0, this, &IconRenderer::focusCamera);
#endif
}

void IconRenderer::finishCreateIcon()
{
    QFileInfo fi(m_filePath);

    // Render regular size image
    render(fi.absoluteFilePath());

    // Render @2x image
    resizeContent(m_size * 2);
    if (!initRhi())
        QTimer::singleShot(1000, qGuiApp, &QGuiApplication::quit);

    QString saveFile;
    saveFile = fi.absolutePath() + '/' + fi.completeBaseName() + "@2x";
    if (!fi.suffix().isEmpty())
        saveFile += '.' + fi.suffix();

    fi.absoluteDir().mkpath(".");

    render(saveFile);

    // Allow little time for file operations to finish
    QTimer::singleShot(1000, qGuiApp, &QGuiApplication::quit);
}

void IconRenderer::render(const QString &fileName)
{
    std::function<void (QQuickItem *)> updateNodesRecursive;
    updateNodesRecursive = [&updateNodesRecursive](QQuickItem *item) {
        const auto childItems = item->childItems();
        for (QQuickItem *childItem : childItems)
            updateNodesRecursive(childItem);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        DesignerSupport::updateDirtyNode(item);
#else
        if (item->flags() & QQuickItem::ItemHasContents)
            item->update();
#endif
    };
    updateNodesRecursive(m_containerItem);

    QRect rect(QPoint(), m_contentItem->size().toSize());
    QImage renderImage;
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    renderImage = m_designerSupport.renderImageForItem(m_containerItem, rect, rect.size());
#else
    m_renderControl->polishItems();
    m_renderControl->beginFrame();
    m_renderControl->sync();
    m_renderControl->render();

    bool readCompleted = false;
    QRhiReadbackResult readResult;
    readResult.completed = [&] {
        readCompleted = true;
        QImage wrapperImage(reinterpret_cast<const uchar *>(readResult.data.constData()),
                            readResult.pixelSize.width(), readResult.pixelSize.height(),
                            QImage::Format_RGBA8888_Premultiplied);
        if (m_rhi->isYUpInFramebuffer())
            renderImage = wrapperImage.mirrored().copy(0, 0, rect.width(), rect.height());
        else
            renderImage = wrapperImage.copy(0, 0, rect.width(), rect.height());
    };
    QRhiResourceUpdateBatch *readbackBatch = m_rhi->nextResourceUpdateBatch();
    readbackBatch->readBackTexture(m_texture, &readResult);

    QQuickRenderControlPrivate *rd = QQuickRenderControlPrivate::get(m_renderControl);
    rd->cb->resourceUpdate(readbackBatch);

    m_renderControl->endFrame();
#endif
    if (!fileName.isEmpty()) {
        QFileInfo fi(fileName);
        if (fi.suffix().isEmpty())
            renderImage.save(fileName, "PNG");
        else
            renderImage.save(fileName);
    }
}

void IconRenderer::resizeContent(int dimensions)
{
    QSizeF size(dimensions, dimensions);
    m_contentItem->setSize(size);
    if (m_contentItem->width() > m_containerItem->width())
        m_containerItem->setWidth(m_contentItem->width());
    if (m_contentItem->height() > m_containerItem->height())
        m_containerItem->setHeight(m_contentItem->height());
}

bool IconRenderer::initRhi()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    if (!m_rhi) {
        QQuickRenderControlPrivate *rd = QQuickRenderControlPrivate::get(m_renderControl);
        m_rhi = rd->rhi;
        if (!m_rhi) {
            qWarning() << __FUNCTION__ << "Rhi is null";
            return false;
        }
    }

    // Don't bother deleting old ones as iconrender is a single shot executable
    m_texTarget = nullptr;
    m_rpDesc = nullptr;
    m_buffer = nullptr;
    m_texture = nullptr;

    const QSize size = m_containerItem->size().toSize();
    m_texture = m_rhi->newTexture(QRhiTexture::RGBA8, size, 1,
                                  QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource);
    if (!m_texture->create()) {
        qWarning() << __FUNCTION__ << "QRhiTexture creation failed";
        return false;
    }

    m_buffer = m_rhi->newRenderBuffer(QRhiRenderBuffer::DepthStencil, size, 1);
    if (!m_buffer->create()) {
        qWarning() << __FUNCTION__ << "Depth/stencil buffer creation failed";
        return false;
    }

    QRhiTextureRenderTargetDescription rtDesc {QRhiColorAttachment(m_texture)};
    rtDesc.setDepthStencilBuffer(m_buffer);
    m_texTarget = m_rhi->newTextureRenderTarget(rtDesc);
    m_rpDesc = m_texTarget->newCompatibleRenderPassDescriptor();
    m_texTarget->setRenderPassDescriptor(m_rpDesc);
    if (!m_texTarget->create()) {
        qWarning() << __FUNCTION__ << "Texture render target creation failed";
        return false;
    }

    // redirect Qt Quick rendering into our texture
    m_window->setRenderTarget(QQuickRenderTarget::fromRhiRenderTarget(m_texTarget));
#endif
    return true;
}
