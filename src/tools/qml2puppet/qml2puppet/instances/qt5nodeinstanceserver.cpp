// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qt5nodeinstanceserver.h"

#include <QSurfaceFormat>

#include <QQmlFileSelector>

#include <QQuickItem>
#include <QQuickView>
#include <QQuickWindow>

#include <private/qquickdesignersupport_p.h>
#include <addimportcontainer.h>
#include <createscenecommand.h>
#include <reparentinstancescommand.h>
#include <clearscenecommand.h>

// Nanotrace headers are not exported to build dir at all if the feature is disabled, so
// runtime puppet build can't find them.
#if NANOTRACE_ENABLED
#include "nanotrace/nanotrace.h"
#else
#define NANOTRACE_SCOPE(cat, name)
#endif

#include <QDebug>
#include <QOpenGLContext>

#include <QtGui/private/qrhi_p.h>
#include <QtQuick/private/qquickitem_p.h>
#include <QtQuick/private/qquickrendercontrol_p.h>
#include <QtQuick/private/qquickrendertarget_p.h>
#include <QtQuick/private/qquickwindow_p.h>
#include <QtQuick/private/qsgcontext_p.h>
#include <QtQuick/private/qsgrenderer_p.h>
#include <QtQuick/private/qsgrhilayer_p.h>

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 1)
#include <QDir>
#include <QFile>
#include <QQuickGraphicsConfiguration>
#include <QStandardPaths>
#include <QTimer>
#define USE_PIPELINE_CACHE 1

#if defined(QUICK3D_MODULE) && QT_VERSION >= QT_VERSION_CHECK(6, 5, 2)
#if QT_VERSION < QT_VERSION_CHECK(6, 7, 0)
#include <QtQuick3DRuntimeRender/private/qssgrendercontextcore_p.h>
#else
#include <QtQuick3DRuntimeRender/private/qssgrendershadercache_p.h>
#include <QtQuick3DRuntimeRender/ssg/qssgrendercontextcore.h>
#endif
#include <QtQuick3D/private/qquick3dscenemanager_p.h>
#define USE_SHADER_CACHE 1
#endif
#endif

namespace QmlDesigner {

Qt5NodeInstanceServer::Qt5NodeInstanceServer(NodeInstanceClientInterface *nodeInstanceClient)
    : NodeInstanceServer(nodeInstanceClient)
    , m_designerSupport(new QQuickDesignerSupport)
{
    if (!ViewConfig::isParticleViewMode())
        QQuickDesignerSupport::activateDesignerMode();
}

Qt5NodeInstanceServer::~Qt5NodeInstanceServer()
{
    NodeInstanceServer::clearScene({});
    delete m_viewData.window.data();
}

QQuickView *Qt5NodeInstanceServer::quickView() const
{
    return nullptr;
}

QQuickWindow *Qt5NodeInstanceServer::quickWindow() const
{
    return m_viewData.window.data();
}

void Qt5NodeInstanceServer::initializeView()
{
    Q_ASSERT(!quickWindow());

    m_viewData.renderControl = new QQuickRenderControl;
    m_viewData.window = new QQuickWindow(m_viewData.renderControl);
    setPipelineCacheConfig(m_viewData.window);
    m_viewData.renderControl->initialize();
    m_qmlEngine = new QQmlEngine;

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
    quickWindow()->setGeometry(0, 0, item->width(), item->height());
    // Insert an extra item above the root to adjust root item position to 0,0 to make entire
    // item to be always rendered.
    if (!m_viewData.contentItem)
        m_viewData.contentItem = new QQuickItem(quickWindow()->contentItem());
    m_viewData.contentItem->setPosition(-item->position());
    item->setParentItem(m_viewData.contentItem);
}

QQmlEngine *Qt5NodeInstanceServer::engine() const
{
    return m_qmlEngine;
}

void Qt5NodeInstanceServer::resizeCanvasToRootItem()
{
    m_viewData.bufferDirty = true;
    if (m_viewData.contentItem)
        m_viewData.contentItem->setPosition(-m_viewData.rootItem->position());
    quickWindow()->resize(rootNodeInstance().boundingRect().size().toSize());
    QQuickDesignerSupport::addDirty(rootNodeInstance().rootQuickItem(), QQuickDesignerSupport::Size);
}

void Qt5NodeInstanceServer::resetAllItems()
{
    for (QQuickItem *item : allItems())
        QQuickDesignerSupport::resetDirty(item);
}

void Qt5NodeInstanceServer::setupScene(const CreateSceneCommand &command)
{
    setupMockupTypes(command.mockupTypes);
    setupFileUrl(command.fileUrl);
    setupImports(command.imports);
    setupDummyData(command.fileUrl);

    setupInstances(command);
    resizeCanvasToRootItem();

#ifdef USE_PIPELINE_CACHE
    if (!m_pipelineCacheLocation.isEmpty()) {
        QString fileId = command.fileUrl.toLocalFile();
        fileId.remove(':');
        fileId.remove('/');
        fileId.remove('.');
        m_pipelineCacheFile = QStringLiteral("%1/%2").arg(m_pipelineCacheLocation, fileId);

        QFile cacheFile(m_pipelineCacheFile);
        if (cacheFile.open(QIODevice::ReadOnly))
            m_pipelineCacheData = cacheFile.readAll();

#ifdef USE_SHADER_CACHE
        m_shaderCacheFile = m_pipelineCacheFile + ".qsbc";
#endif
    }
#endif
}

QList<QQuickItem*> subItems(QQuickItem *parentItem)
{
    QList<QQuickItem*> itemList;
    itemList.append(parentItem->childItems());

    const QList<QQuickItem *> childItems = parentItem->childItems();
    for (QQuickItem *childItem : childItems)
        itemList.append(subItems(childItem));

    return itemList;
}

const QList<QQuickItem*> Qt5NodeInstanceServer::allItems() const
{
    if (rootNodeInstance().isValid())
        return rootNodeInstance().allItemsRecursive();

    return {};
}

bool Qt5NodeInstanceServer::rootIsRenderable3DObject() const
{
    return rootNodeInstance().isSubclassOf("QQuick3DNode")
           || rootNodeInstance().isSubclassOf("QQuick3DMaterial");
}

void Qt5NodeInstanceServer::savePipelineCacheData()
{
#ifdef USE_PIPELINE_CACHE
    if (!m_viewData.rhi)
        return;

    QByteArray pipelineData = m_viewData.rhi->pipelineCacheData();

    if (pipelineData.isEmpty())
        return;

    char count = 0;
    if (!m_pipelineCacheData.isEmpty())
        count = m_pipelineCacheData[m_pipelineCacheData.size() - 1];
    pipelineData.append(++count);

    const bool needWrite = m_pipelineCacheData.size() != pipelineData.size()
                           && !m_pipelineCacheFile.isEmpty();

    if (needWrite) {
        m_pipelineCacheData = pipelineData;

        QTimer::singleShot(0, this, [this]() {
            QFile cacheFile(m_pipelineCacheFile);

            // Cache file can grow indefinitely, so let's just purge it every so often.
            // The count is stored as the last char in the data.
            char count = m_pipelineCacheData[m_pipelineCacheData.size() - 1];
            const char maxCount = 25;
            if (count > maxCount)
                cacheFile.remove();
            else if (cacheFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
                cacheFile.write(m_pipelineCacheData);

#ifdef USE_SHADER_CACHE
            auto wa = QQuick3DSceneManager::getOrSetWindowAttachment(*m_viewData.window);
            auto context = wa ? wa->rci().get() : nullptr;
            if (context && context->shaderCache()) {
                if (count > maxCount) {
                    QFile shaderCacheFile(m_shaderCacheFile);
                    shaderCacheFile.remove();
                } else {
                    context->shaderCache()->persistentShaderBakingCache().save(m_shaderCacheFile);
                }
            }
#endif
        });
    }
#endif
}

void Qt5NodeInstanceServer::setPipelineCacheConfig([[maybe_unused]] QQuickWindow *w)
{
#ifdef USE_PIPELINE_CACHE
    // This dummy file is not actually used for cache as we manage cache save/load ourselves,
    // but some file needs to be set to enable pipeline caching
    const QString cachePath = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    m_pipelineCacheLocation = QStringLiteral("%1/%2").arg(cachePath, "pipecache");
    QDir(m_pipelineCacheLocation).mkpath(".");
    const QString dummyCache = m_pipelineCacheLocation + "/dummycache";

    QQuickGraphicsConfiguration config = w->graphicsConfiguration();
    config.setPipelineCacheSaveFile(dummyCache);
    config.setAutomaticPipelineCache(false);
    w->setGraphicsConfiguration(config);

#ifdef USE_SHADER_CACHE
    QtQuick3DEditorHelpers::ShaderCache::setAutomaticDiskCache(false);
    auto wa = QQuick3DSceneManager::getOrSetWindowAttachment(*w);
    connect(wa, &QQuick3DWindowAttachment::renderContextInterfaceChanged,
            this, &Qt5NodeInstanceServer::handleRciSet);
#endif
#endif
}

void Qt5NodeInstanceServer::handleRciSet()
{
#ifdef USE_SHADER_CACHE
    auto wa = qobject_cast<QQuick3DWindowAttachment *>(sender());
    auto context = wa ? wa->rci().get() : nullptr;
    if (context && context->shaderCache())
        context->shaderCache()->persistentShaderBakingCache().load(m_shaderCacheFile);
#endif
}

bool Qt5NodeInstanceServer::initRhi([[maybe_unused]] RenderViewData &viewData)
{
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
#ifdef USE_PIPELINE_CACHE
        if (!m_pipelineCacheData.isEmpty())
            viewData.rhi->setPipelineCacheData(m_pipelineCacheData.left(m_pipelineCacheData.size() - 1));
#endif
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

    return true;
}

QImage Qt5NodeInstanceServer::grabRenderControl([[maybe_unused]] RenderViewData &viewData)
{
    NANOTRACE_SCOPE("Update", "GrabRenderControl");

    QImage renderImage;
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

    return renderImage;
}

// This method simply renders the window without grabbing it
bool Qt5NodeInstanceServer::renderWindow()
{
    if (!m_viewData.rootItem || (m_viewData.bufferDirty && !initRhi(m_viewData)))
        return false;

    m_viewData.renderControl->polishItems();
    m_viewData.renderControl->beginFrame();
    m_viewData.renderControl->sync();
    m_viewData.renderControl->render();
    m_viewData.renderControl->endFrame();
    return true;
}

QImage Qt5NodeInstanceServer::grabWindow()
{
    if (m_viewData.rootItem)
        return grabRenderControl(m_viewData);
    return  {};
}

static bool hasEffect(QQuickItem *item)
{
    QQuickItemPrivate *pItem = QQuickItemPrivate::get(item);
    return pItem && pItem->layer() && pItem->layer()->enabled() && pItem->layer()->effect();
}

QQuickItem *Qt5NodeInstanceServer::parentEffectItem(QQuickItem *item)
{
    QQuickItem *parent = item->parentItem();
    while (parent) {
        if (hasEffect(parent))
            return parent;
        parent = parent->parentItem();
    }
    return nullptr;
}

static bool isEffectItem(QQuickItem *item, QQuickShaderEffectSource *sourceItem)
{
    QQuickItemPrivate *pItem = QQuickItemPrivate::get(sourceItem);

    if (!pItem || !pItem->layer())
        return false;

    const auto propName = pItem->layer()->name();

    QQmlProperty prop(item, QString::fromLatin1(propName));
    if (!prop.isValid())
        return false;

    return prop.read().value<QQuickShaderEffectSource *>() == sourceItem;
}

static bool isLayerEnabled(QQuickItemPrivate *item)
{
    return item && item->layer() && item->layer()->enabled();
}

QImage Qt5NodeInstanceServer::grabItem([[maybe_unused]] QQuickItem *item)
{
    QImage renderImage;
    if (!m_viewData.rootItem || (m_viewData.bufferDirty && !initRhi(m_viewData)))
        return {};

    QQuickItemPrivate *pItem = QQuickItemPrivate::get(item);

    const bool renderEffects = qEnvironmentVariableIsSet("QMLPUPPET_RENDER_EFFECTS");
    const bool smoothRendering = qEnvironmentVariableIsSet("QMLPUPPET_SMOOTH_RENDERING");

    if (renderEffects) {
        if (parentEffectItem(item))
            return renderImage;

        // Effects are actually implemented as a separate item we have to find first
        if (hasEffect(item)) {
            if (auto parent = item->parentItem()) {
                const auto siblings = parent->childItems();
                for (auto sibling : siblings) {
                    if (isEffectItem(sibling, pItem->layer()->effectSource()))
                        return grabItem(sibling);
                }
            }
        }
    }

    if (!isLayerEnabled(pItem))
        pItem->refFromEffectItem(false);

    ServerNodeInstance instance;
    if (hasInstanceForObject(item))
        instance = instanceForObject(item);

    const bool rootIs3DObject = rootIsRenderable3DObject();

    // Setting layer enabled to false messes up the bounding rect.
    // Therefore we calculate it upfront.
    QRectF renderBoundingRect;
    if (instance.isValid())
        renderBoundingRect = instance.boundingRect();
    else if (rootIs3DObject)
        renderBoundingRect = item->boundingRect();
    else
        renderBoundingRect = ServerNodeInstance::effectAdjustedBoundingRect(item);

    const int scaleFactor = (smoothRendering && !rootIs3DObject) ? 2 : 1;

    // Hide immediate children that have instances and are QQuickItems so we get only
    // the parent item's content, as compositing is handled on creator side.
    QSet<QQuickItem *> layerChildren;

    if (instance.isValid()) { //Not valid for effect
        const auto childInstances = instance.childItems();
        for (const auto &childInstance : childInstances) {
            QQuickItem *childItem = qobject_cast<QQuickItem *>(childInstance.internalObject());
            if (childItem) {
                QQuickItemPrivate *pChild = QQuickItemPrivate::get(childItem);
                if (pChild->layer() && pChild->layer()->enabled()) {
                    layerChildren.insert(childItem);
                    pChild->layer()->setEnabled(false);
                }
                pChild->refFromEffectItem(true);
            }
        }
    }

    m_viewData.renderControl->polishItems();
    m_viewData.renderControl->beginFrame();
    m_viewData.renderControl->sync();

    // Connection to afterRendering is necessary, as this needs to be done before
    // call to endNextRhiFrame which happens inside QQuickRenderControl::render()
    QMetaObject::Connection connection = QObject::connect(m_viewData.window.data(),
                                                          &QQuickWindow::afterRendering,
                                                          this, [&]() {
        // To get only the single item, we need to make a layer out of it, which enables
        // us to render it to a texture that we can grab to an image.
        QSGRenderContext *rc = QQuickWindowPrivate::get(m_viewData.window.data())->context;
        QSGLayer *layer = rc->sceneGraphContext()->createLayer(rc);
        if (smoothRendering)
            layer->setSamples(4);
        layer->setItem(pItem->itemNode());

        layer->setRect(QRectF(renderBoundingRect.x(),
                              renderBoundingRect.y() + renderBoundingRect.height(),
                              renderBoundingRect.width(),
                              -renderBoundingRect.height()));

        const QSize minSize = rc->sceneGraphContext()->minimumFBOSize();
        layer->setSize(QSize(qMax(minSize.width(), int(renderBoundingRect.width() * scaleFactor)),
                             qMax(minSize.height(), int(renderBoundingRect.height() * scaleFactor))));
        layer->scheduleUpdate();

        if (layer->updateTexture())
            renderImage = layer->toImage().convertToFormat(QImage::Format_ARGB32);
        else
            qWarning() << __FUNCTION__ << "Failed to update layer texture";

        delete layer;
        layer = nullptr;

        renderImage.setDevicePixelRatio(scaleFactor);
    });

    m_viewData.renderControl->render();

    QObject::disconnect(connection);

    m_viewData.renderControl->endFrame();

    if (instance.isValid()) { //Not valid for effect
        const auto childInstances = instance.childItems();

        // Restore visibility of immediate children that have instances and are QQuickItems
        for (const auto &childInstance : childInstances) {
            QQuickItem *childItem = qobject_cast<QQuickItem *>(childInstance.internalObject());
            if (childItem) {
                QQuickItemPrivate *pChild = QQuickItemPrivate::get(childItem);
                pChild->derefFromEffectItem(true);
                if (pChild->layer() && layerChildren.contains(childItem))
                    pChild->layer()->setEnabled(true);
            }
        }
    }

    if (!isLayerEnabled(pItem))
        pItem->derefFromEffectItem(false);

    return renderImage;
}

void Qt5NodeInstanceServer::refreshBindings()
{
    QQuickDesignerSupport::refreshExpressions(context());
}

QQuickDesignerSupport *Qt5NodeInstanceServer::designerSupport()
{
    return m_designerSupport.get();
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
