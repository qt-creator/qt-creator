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

#include <QFileInfo>
#include <QQmlComponent>
#include <QQuickItem>
#include <QTimer>

constexpr int DEFAULT_RENDER_DIM = 300;
constexpr int DEFAULT_MAX_DIM = 1024;

void QmlRenderer::initCoreApp()
{
#ifdef Q_OS_MACOS
    qputenv("QT_MAC_DISABLE_FOREGROUND_APPLICATION_TRANSFORM", "true");
#endif
#if defined QT_WIDGETS_LIB
    createCoreApp<QApplication>();
#else
    createCoreApp<QGuiApplication>();
#endif //QT_WIDGETS_LIB
}

void QmlRenderer::populateParser()
{
    m_argParser.addOptions({
        {QStringList{"i", "importpath"},
         "Prepend the given path to the import paths.",
         "path"},

        {QStringList{"o", "outfile"},
         "Output image file path.",
         "path"},

        // The qml component is rendered at its preferred size if available
        // Min/max dimensions specify a range of acceptable final scaled sizes
        // If the size of rendered item is outside the min/max range, it is cropped in final scaling
        {QStringList{"minW"},
         "Minimum width of the final scaled rendered image.",
         "pixels"},

        {QStringList{"minH"},
         "Minimum height of the final scaled rendered image.",
         "pixels"},

        {QStringList{"maxW"},
         QString("Maximum width of the final scaled rendered image."),
         "pixels"},

        {QStringList{"maxH"},
         "Maximum height of the final scaled rendered image.",
         "pixels"},

        {QStringList{"libIcon"},
         "Render library icon rather than regular preview."
         "It zooms 3D objects bit more aggressively and suppresses the background."},

        {QStringList{"v", "verbose"},
         "Display additional output."}
    });

    m_argParser.addPositionalArgument("file", "QML file to render.", "file");
}

void QmlRenderer::initQmlRunner()
{
    if (m_argParser.isSet("importpath"))
        m_importPaths = m_argParser.value("importpath").split(";");
    if (m_argParser.isSet("minW"))
        m_reqMinSize.setWidth(m_argParser.value("minW").toInt());
    if (m_argParser.isSet("minH"))
        m_reqMinSize.setHeight(m_argParser.value("minH").toInt());
    if (m_argParser.isSet("maxW"))
        m_reqMaxSize.setWidth(m_argParser.value("maxW").toInt());
    if (m_argParser.isSet("maxH"))
        m_reqMaxSize.setHeight(m_argParser.value("maxH").toInt());
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

    if (m_argParser.isSet("libIcon"))
        m_isLibIcon = true;

    if (m_verbose) {
        info(QString("Import path        = %1").arg(m_importPaths.join(";")));
        info(QString("Requested min size = %1 x %2").arg(m_reqMinSize.width())
                                                    .arg(m_reqMinSize.height()));
        info(QString("Requested max size = %1 x %2").arg(m_reqMaxSize.width())
                                                    .arg(m_reqMaxSize.height()));
        info(QString("Source file        = %1").arg(m_sourceFile));
        info(QString("Output file        = %1").arg(m_outFile));
        info(QString("Is library icon    = %1").arg(m_isLibIcon ? QString("true") : QString("false")));
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

    m_engine = std::make_unique<QQmlEngine>();

    for (const QString &path : std::as_const(m_importPaths))
        m_engine->addImportPath(path);

    m_renderControl = std::make_unique<QQuickRenderControl>();
    m_window = std::make_unique<QQuickWindow>(m_renderControl.get());
    m_window->setDefaultAlphaBuffer(true);
    m_window->setColor(Qt::transparent);
    m_renderControl->initialize();

    QQmlComponent component(m_engine.get());
    component.loadUrl(QUrl::fromLocalFile(m_sourceFile));

    if (component.isError()) {
        error(QString("Failed to load url: %1").arg(m_sourceFile));
        error(component.errorString());
        return false;
    }

    // Window components will flash the window briefly if we don't initialize visible to false
    QVariantMap initialProps;
    initialProps["visible"] = false;
    QObject *renderObj = component.createWithInitialProperties(initialProps);

    if (renderObj) {
        if (!qobject_cast<QWindow *>(renderObj))
            renderObj->setProperty("visible", true);
        QQuickItem *contentItem3D = nullptr;
#ifdef QUICK3D_MODULE
        renderObj->setParent(m_window->contentItem());
        if (qobject_cast<QQuick3DObject *>(renderObj)) {
            m_helper = std::make_unique<QmlDesigner::Internal::GeneralHelper>();
            m_engine->rootContext()->setContextProperty("_generalHelper", m_helper.get());

            QQmlComponent component(m_engine.get());
            component.loadUrl(QUrl("qrc:/qtquickplugin/mockfiles/qt6/ModelNode3DImageView.qml"));
            m_containerItem = qobject_cast<QQuickItem *>(component.create());
            if (!m_containerItem) {
                error("Failed to create 3D container view.");
                return false;
            }
            m_containerItem->setParentItem(m_window->contentItem());

            if (m_isLibIcon)
                QMetaObject::invokeMethod(m_containerItem, "setIconMode", Q_ARG(QVariant, true));

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
            setRenderSize({});
            contentItem3D->setSize(m_renderSize);
        } else
#endif // QUICK3D_MODULE
        if (auto renderItem = qobject_cast<QQuickItem *>(renderObj)) {
            setRenderSize(renderItem->size().toSize());
            renderItem->setSize(m_renderSize);
            renderItem->setParentItem(m_window->contentItem());
            // When rendering 2D scenes, we just render the given QML without extra container
            m_containerItem = renderItem;
        } else if (auto renderWindow = qobject_cast<QQuickWindow *>(renderObj)) {
            // Hack to render Window items: reparent window content to m_window->contentItem()
            setRenderSize(renderWindow->size());
            m_containerItem = m_window->contentItem();
            const QList<QQuickItem *> childItems = renderWindow->contentItem()->childItems();
            for (QQuickItem *item : childItems) {
                item->setParent(m_window->contentItem());
                item->setParentItem(m_window->contentItem());
            }
        } else {
            error("Invalid root object type.");
            return false;
        }

        if (m_containerItem && (contentItem3D || !m_is3D)) {
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
        QQuickRenderControlPrivate *rd = QQuickRenderControlPrivate::get(m_renderControl.get());
        m_rhi = rd->rhi;
        if (!m_rhi) {
            error("Rhi is null.");
            return false;
        }
    }

    m_texture.reset(m_rhi->newTexture(QRhiTexture::RGBA8, m_renderSize, 1,
                                      QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource));
    if (!m_texture->create()) {
        error("QRhiTexture creation failed.");
        return false;
    }

    m_buffer.reset(m_rhi->newRenderBuffer(QRhiRenderBuffer::DepthStencil, m_renderSize, 1));
    if (!m_buffer->create()) {
        error("Depth/stencil buffer creation failed.");
        return false;
    }

    QRhiTextureRenderTargetDescription rtDesc {QRhiColorAttachment(m_texture.get())};
    rtDesc.setDepthStencilBuffer(m_buffer.get());
    m_texTarget.reset(m_rhi->newTextureRenderTarget(rtDesc));
    m_rpDesc.reset(m_texTarget->newCompatibleRenderPassDescriptor());
    m_texTarget->setRenderPassDescriptor(m_rpDesc.get());
    if (!m_texTarget->create()) {
        error("Texture render target creation failed.");
        return false;
    }

    // redirect Qt Quick rendering into our texture
    m_window->setRenderTarget(QQuickRenderTarget::fromRhiRenderTarget(m_texTarget.get()));

    return true;
}

void QmlRenderer::render()
{
    info(QString("Rendering: %1").arg(m_sourceFile));

    QImage renderImage;

    // Need to render fitted 3D views twice, first render updates spatial node geometries
    const int renderCount = m_fit3D ? 2 : 1;
    for (int i = 0; i < renderCount; ++i) {
        if (m_fit3D && i == 1)
            QMetaObject::invokeMethod(m_containerItem, "fitToViewPort", Qt::DirectConnection);

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

        QSize maxSize = m_reqMaxSize;
        if (maxSize.width() <= 0) {
            m_reqMinSize.width() > DEFAULT_MAX_DIM ? maxSize.setWidth(m_reqMinSize.width())
                                                   : maxSize.setWidth(DEFAULT_MAX_DIM);
        }
        if (maxSize.height() <= 0) {
            m_reqMinSize.height() > DEFAULT_MAX_DIM ? maxSize.setHeight(m_reqMinSize.height())
                                                    : maxSize.setHeight(DEFAULT_MAX_DIM);
        }

        QSize scaledSize = m_renderSize.scaled(m_renderSize.expandedTo(m_reqMinSize).boundedTo(maxSize),
                                               Qt::KeepAspectRatio);

        info(QString("Rendered size = %1 x %2").arg(m_renderSize.width())
                                               .arg(m_renderSize.height()));
        info(QString("Scaled size = %1 x %2").arg(scaledSize.width())
                                             .arg(scaledSize.height()));

        if (m_rhi->isYUpInFramebuffer()) {
            renderImage = wrapperImage.mirrored().scaled(scaledSize, Qt::IgnoreAspectRatio,
                                                         Qt::SmoothTransformation);
        } else {
            renderImage = wrapperImage.copy().scaled(scaledSize, Qt::IgnoreAspectRatio,
                                                     Qt::SmoothTransformation);
        }
    };
    QRhiResourceUpdateBatch *readbackBatch = m_rhi->nextResourceUpdateBatch();
    readbackBatch->readBackTexture(m_texture.get(), &readResult);

    QQuickRenderControlPrivate *rd = QQuickRenderControlPrivate::get(m_renderControl.get());
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

void QmlRenderer::setRenderSize(QSize size)
{
    m_renderSize = size;
    if (m_renderSize.width() <= 0) {
        m_reqMaxSize.width() > 0 ? m_renderSize.setWidth(m_reqMaxSize.width())
                                 : m_renderSize.setWidth(DEFAULT_RENDER_DIM);
    }
    if (m_renderSize.height() <= 0) {
        m_reqMaxSize.height() > 0 ? m_renderSize.setHeight(m_reqMaxSize.height())
                                  : m_renderSize.setHeight(DEFAULT_RENDER_DIM);
    }
}
