// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlrenderer.h"

#ifdef QUICK3D_MODULE
#include "../editor3d/generalhelper.h"

#include <private/qquick3dmodel_p.h>
#include <private/qquick3dnode_p.h>
#include <private/qquick3dviewport_p.h>
#endif

#include <private/qquickdesignersupport_p.h>
#include <private/qquickrendercontrol_p.h>
#include <private/qquickrendertarget_p.h>
#include <private/qrhi_p.h>

#include <QFileInfo>
#include <QQmlComponent>
#include <QQmlEngine>

void QmlRenderer::initCoreApp()
{
#if defined QT_WIDGETS_LIB
    createCoreApp<QApplication>();
#else
    createCoreApp<QGuiApplication>();
#endif //QT_WIDGETS_LIB
}

void QmlRenderer::populateParser()
{
    m_argParser.addOptions({
        {QStringList() << "i" << "importpath",
         "Prepend the given path to the import paths.",
         "path"},

        {QStringList() << "o" << "outfile",
         "Output image file path.",
         "path"},

        // "h" is reserved arg for help, so use capital letters for height/width
        {QStringList() << "H" << "height",
         "Height of the final rendered image.",
         "pixels"},

        {QStringList() << "W" << "width",
         "Width of the final rendered image.",
         "pixels"},

        {QStringList() << "v" << "verbose", "Display additional output."}
    });

    m_argParser.addPositionalArgument("file", "QML file to render.", "file");
}

void QmlRenderer::initQmlRunner()
{
    if (m_argParser.isSet("importpath"))
        m_importPaths = m_argParser.value("importpath").split(";");
    if (m_argParser.isSet("width"))
        m_requestedSize.setWidth(m_argParser.value("width").toInt());
    if (m_argParser.isSet("height"))
        m_requestedSize.setHeight(m_argParser.value("height").toInt());
    if (m_argParser.isSet("verbose"))
        m_verbose = true;

    QStringList posArgs = m_argParser.positionalArguments();
    if (!posArgs.isEmpty())
        m_sourceFile = posArgs[0];

    QFileInfo sourceInfo(m_sourceFile);
    if (!sourceInfo.exists())
        error("Source QML file must be specified and exist.");

    if (m_argParser.isSet("outfile"))
        m_outFile = m_argParser.value("outfile");
    else
        m_outFile = m_sourceFile + ".png";

    if (m_requestedSize.width() <= 0)
        m_requestedSize.setWidth(150);
    if (m_requestedSize.height() <= 0)
        m_requestedSize.setHeight(150);

    if (m_verbose) {
        info(QString("Import path    = %1").arg(m_importPaths.join(";")));
        info(QString("Requested size = %1 x %2").arg(m_requestedSize.width())
                                                .arg(m_requestedSize.height()));
        info(QString("Source file    = %1").arg(m_sourceFile));
        info(QString("Output file    = %1").arg(m_outFile));
    }

    if (setupRenderer()) {
        render();
        asyncQuit(0);
    }
}

bool QmlRenderer::setupRenderer()
{
    info("Setting up renderer.");

    QQuickDesignerSupport::activateDesignerMode();

    QQmlEngine *engine = new QQmlEngine;

    for (const QString &path : std::as_const(m_importPaths))
        engine->addImportPath(path);

    m_renderControl = new QQuickRenderControl;
    m_window = new QQuickWindow(m_renderControl);
    m_window->setDefaultAlphaBuffer(true);
    m_window->setColor(Qt::transparent);
    m_renderControl->initialize();

    QQmlComponent component(engine);
    component.loadUrl(QUrl::fromLocalFile(m_sourceFile));
    QObject *renderObj = component.create();

    if (renderObj) {
#ifdef QUICK3D_MODULE
        QQuickItem *contentItem3D = nullptr;
        if (qobject_cast<QQuick3DObject *>(renderObj)) {
            auto helper = new QmlDesigner::Internal::GeneralHelper();
            engine->rootContext()->setContextProperty("_generalHelper", helper);

            QQmlComponent component(engine);
            component.loadUrl(QUrl("qrc:/qtquickplugin/mockfiles/qt6/ModelNode3DImageView.qml"));
            m_containerItem = qobject_cast<QQuickItem *>(component.create());
            if (!m_containerItem) {
                error("Failed to create 3D container view.");
                return false;
            }
            m_containerItem->setParentItem(m_window->contentItem());

            contentItem3D = QQmlProperty::read(m_containerItem, "contentItem").value<QQuickItem *>();
            if (qobject_cast<QQuick3DNode *>(renderObj)) {
                QMetaObject::invokeMethod(
                    m_containerItem, "createViewForNode",
                    Q_ARG(QVariant, QVariant::fromValue(renderObj)));
                m_fit3D = true;
            } else {
                m_fit3D = qobject_cast<QQuick3DModel *>(renderObj);
                QMetaObject::invokeMethod(
                    m_containerItem, "createViewForObject",
                    Q_ARG(QVariant, QVariant::fromValue(renderObj)),
                    Q_ARG(QVariant, ""), Q_ARG(QVariant, ""), Q_ARG(QVariant, ""));
            }

            m_is3D = true;
            m_renderSize = m_requestedSize;
            contentItem3D->setSize(m_requestedSize);
        } else
#endif // QUICK3D_MODULE
        if (auto renderItem = qobject_cast<QQuickItem *>(renderObj)) {
            m_renderSize = renderItem->size().toSize();
            if (m_renderSize.width() <= 0)
                m_renderSize.setWidth(m_requestedSize.width());
            if (m_renderSize.height() <= 0)
                m_renderSize.setHeight(m_requestedSize.height());
            renderItem->setSize(m_renderSize);
            renderItem->setParentItem(m_window->contentItem());
            // When rendering 2D scenes, we just render the given QML without extra container
            m_containerItem = renderItem;
        } else if (auto renderWindow = qobject_cast<QQuickWindow *>(renderObj)) {
            // Hack to render Window items: reparent window content to m_window->contentItem()
            m_renderSize = renderWindow->size();
            renderWindow->setVisible(false);
            m_containerItem = m_window->contentItem();
            const QList<QQuickItem *> childItems = renderWindow->contentItem()->childItems();
            for (QQuickItem *item : childItems)
                item->setParentItem(m_window->contentItem());
        }

        if ((m_containerItem) && (contentItem3D || !m_is3D)) {
            m_window->setGeometry(0, 0, m_renderSize.width(), m_renderSize.height());
            m_window->contentItem()->setSize(m_renderSize);
            m_containerItem->setSize(m_renderSize);

            if (!initRhi())
                return false;
        } else {
            error("Failed to initialize content.");
            return false;
        }
    } else {
        error("Failed to initialize content.");
        return false;
    }

    return true;
}

bool QmlRenderer::initRhi()
{
    if (!m_rhi) {
        QQuickRenderControlPrivate *rd = QQuickRenderControlPrivate::get(m_renderControl);
        m_rhi = rd->rhi;
        if (!m_rhi) {
            error("Rhi is null.");
            return false;
        }
    }

    m_texTarget = nullptr;
    m_rpDesc = nullptr;
    m_buffer = nullptr;
    m_texture = nullptr;

    m_texture = m_rhi->newTexture(QRhiTexture::RGBA8, m_renderSize, 1,
                                  QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource);
    if (!m_texture->create()) {
        error("QRhiTexture creation failed.");
        return false;
    }

    m_buffer = m_rhi->newRenderBuffer(QRhiRenderBuffer::DepthStencil, m_renderSize, 1);
    if (!m_buffer->create()) {
        error("Depth/stencil buffer creation failed.");
        return false;
    }

    QRhiTextureRenderTargetDescription rtDesc {QRhiColorAttachment(m_texture)};
    rtDesc.setDepthStencilBuffer(m_buffer);
    m_texTarget = m_rhi->newTextureRenderTarget(rtDesc);
    m_rpDesc = m_texTarget->newCompatibleRenderPassDescriptor();
    m_texTarget->setRenderPassDescriptor(m_rpDesc);
    if (!m_texTarget->create()) {
        error("Texture render target creation failed.");
        return false;
    }

    // redirect Qt Quick rendering into our texture
    m_window->setRenderTarget(QQuickRenderTarget::fromRhiRenderTarget(m_texTarget));

    return true;
}

void QmlRenderer::render()
{
    info(QString("Rendering: %1").arg(m_sourceFile));

    std::function<void (QQuickItem *)> updateNodesRecursive;
    updateNodesRecursive = [&updateNodesRecursive](QQuickItem *item) {
        const auto childItems = item->childItems();
        for (QQuickItem *childItem : childItems)
            updateNodesRecursive(childItem);
        if (item->flags() & QQuickItem::ItemHasContents)
            item->update();
    };

    QImage renderImage;

    // Need to render fitted 3D views twice, first render updates spatial node geometries
    const int renderCount = m_fit3D ? 2 : 1;
    for (int i = 0; i < renderCount; ++i) {
        if (m_fit3D && i == 1)
            QMetaObject::invokeMethod(m_containerItem, "fitToViewPort", Qt::DirectConnection);

        updateNodesRecursive(m_containerItem);
        m_renderControl->polishItems();
        m_renderControl->beginFrame();
        m_renderControl->sync();
        m_renderControl->render();

        if (m_fit3D && i == 0)
            m_renderControl->endFrame();
    }

    bool readCompleted = false;
    QRhiReadbackResult readResult;
    readResult.completed = [&] {
        readCompleted = true;
        QImage wrapperImage(reinterpret_cast<const uchar *>(readResult.data.constData()),
                            readResult.pixelSize.width(), readResult.pixelSize.height(),
                            QImage::Format_RGBA8888_Premultiplied);
        if (m_rhi->isYUpInFramebuffer())
            renderImage = wrapperImage.mirrored().scaled(m_requestedSize);
        else
            renderImage = wrapperImage.copy().scaled(m_requestedSize);
    };
    QRhiResourceUpdateBatch *readbackBatch = m_rhi->nextResourceUpdateBatch();
    readbackBatch->readBackTexture(m_texture, &readResult);

    QQuickRenderControlPrivate *rd = QQuickRenderControlPrivate::get(m_renderControl);
    rd->cb->resourceUpdate(readbackBatch);

    m_renderControl->endFrame();

    info(QString("Saving render result: %1").arg(m_outFile));

    QFileInfo fi(m_outFile);
    if (fi.suffix().isEmpty())
        renderImage.save(m_outFile, "PNG");
    else
        renderImage.save(m_outFile);
}

void QmlRenderer::info(const QString &msg)
{
    if (m_verbose)
        qInfo().noquote() << "QmlRenderer -" << msg;
}

void QmlRenderer::error(const QString &msg)
{
    qCritical().noquote() << "QmlRenderer -" << msg;
    asyncQuit(1);
}

void QmlRenderer::asyncQuit(int errorCode)
{
    QTimer::singleShot(0, qGuiApp, [errorCode]() {
        exit(errorCode);
    });
}
