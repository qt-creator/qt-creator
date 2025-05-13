// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "assetslibrarywidget.h"

#include "assetslibraryiconprovider.h"
#include "assetslibrarymodel.h"
#include "assetslibraryview.h"
#include <qmldesignertr.h>

#include <asynchronousimagecache.h>
#include <createtexture.h>
#include <designeractionmanager.h>
#include <designermcumanager.h>
#include <designerpaths.h>
#include <designmodewidget.h>
#include <hdrimage.h>
#include <import.h>
#include <modelnodeoperations.h>
#include <nodemetainfo.h>
#include <qmldesignerconstants.h>
#include <qmldesignerplugin.h>
#include <studioquickwidget.h>
#include <theme.h>
#include <uniquename.h>
#include <utils3d.h>

#include <coreplugin/documentmanager.h>
#include <coreplugin/fileutils.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagebox.h>

#include <utils/algorithm.h>
#include <qmldesignerutils/asset.h>
#include <utils/environment.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QFileDialog>
#include <QFileInfo>
#include <QImageReader>
#include <QMessageBox>
#include <QMimeData>
#include <QMouseEvent>
#include <QPointF>
#include <QQmlContext>
#include <QQuickItem>
#include <QShortcut>
#include <QToolButton>
#include <QVBoxLayout>

#include <memory>

using namespace Core;

namespace QmlDesigner {

static QString propertyEditorResourcesPath()
{
#ifdef SHARE_QML_PATH
    if (Utils::qtcEnvironmentVariableIsSet("LOAD_QML_FROM_SOURCE"))
        return QLatin1String(SHARE_QML_PATH) + "/propertyEditorQmlSources";
#endif
    return Core::ICore::resourcePath("qmldesigner/propertyEditorQmlSources").toUrlishString();
}

bool AssetsLibraryWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::FocusOut) {
        if (obj == m_assetsWidget->quickWidget())
            QMetaObject::invokeMethod(m_assetsWidget->rootObject(), "handleViewFocusOut");
    } else if (event->type() == QMouseEvent::MouseMove) {
        if (!m_assetsToDrag.isEmpty() && m_assetsView->model()) {
            QMouseEvent *me = static_cast<QMouseEvent *>(event);

            if ((me->globalPosition().toPoint() - m_dragStartPoint).manhattanLength() > 10) {
                auto mimeData = std::make_unique<QMimeData>();
                mimeData->setData(Constants::MIME_TYPE_ASSETS, m_assetsToDrag.join(',').toUtf8());

                QList<QUrl> urlsToDrag = Utils::transform(m_assetsToDrag, &QUrl::fromLocalFile);

                QString draggedAsset = m_assetsToDrag[0];

                m_assetsToDrag.clear();
                mimeData->setUrls(urlsToDrag);

                m_assetsView->model()->startDrag(std::move(mimeData),
                                                 m_assetsIconProvider->requestPixmap(draggedAsset,
                                                                                     nullptr,
                                                                                     {128, 128}),
                                                 this);
            }
        }
    } else if (event->type() == QMouseEvent::MouseButtonRelease) {
        m_assetsToDrag.clear();
        setIsDragging(false);
    }

    return QObject::eventFilter(obj, event);
}

AssetsLibraryWidget::AssetsLibraryWidget(AsynchronousImageCache &mainImageCache,
                                         AsynchronousImageCache &asynchronousFontImageCache,
                                         SynchronousImageCache &synchronousFontImageCache,
                                         AssetsLibraryView *view)
    : m_itemIconSize{24, 24}
    , m_mainImageCache{mainImageCache}
    , m_fontImageCache{synchronousFontImageCache}
    , m_assetsIconProvider{new AssetsLibraryIconProvider(synchronousFontImageCache)}
    , m_assetsModel{new AssetsLibraryModel(this)}
    , m_assetsView{view}
    , m_assetsWidget{Utils::makeUniqueObjectPtr<StudioQuickWidget>(this)}
{
    setWindowTitle(Tr::tr("Assets Library", "Title of assets library widget"));
    setMinimumWidth(250);

    connect(m_assetsIconProvider, &AssetsLibraryIconProvider::asyncAssetPreviewRequested,
            this, [this](const QString &assetId, const QString &assetFile) {
        Asset asset{assetFile};
        if (!asset.isImported3D())
            return;

        Utils::FilePath fullPath = QmlDesignerPlugin::instance()->documentManager()
                                       .generatedComponentUtils().getImported3dQml(assetFile);

        if (!fullPath.exists())
            return;

        m_mainImageCache.requestImage(
            Utils::PathString{fullPath.toFSPathString()},
            [this, assetId](const QImage &image) {
                QMetaObject::invokeMethod(this, [this, assetId, image] {
                    updateAssetPreview(assetId, QPixmap::fromImage(image), "q3d");
                }, Qt::QueuedConnection);
            },
            [assetFile](ImageCache::AbortReason abortReason) {
                if (abortReason == ImageCache::AbortReason::Abort) {
                    qWarning() << QLatin1String(
                                      "AssetsLibraryIconProvider::asyncAssetPreviewRequested(): preview generation "
                                      "failed for path %1, reason: Abort").arg(assetFile);
                } else if (abortReason == ImageCache::AbortReason::Failed) {
                    qWarning() << QLatin1String(
                                      "AssetsLibraryIconProvider::asyncAssetPreviewRequested(): preview generation "
                                      "failed for path %1, reason: Failed").arg(assetFile);
                } else if (abortReason == ImageCache::AbortReason::NoEntry) {
                    qWarning() << QLatin1String(
                                      "AssetsLibraryIconProvider::asyncAssetPreviewRequested(): preview generation "
                                      "failed for path %1, reason: NoEntry").arg(assetFile);
                }
            },
            "libIcon",
            ImageCache::LibraryIconAuxiliaryData{true});
    });

    m_assetsWidget->quickWidget()->installEventFilter(this);

    m_fontPreviewTooltipBackend = std::make_unique<PreviewTooltipBackend>(asynchronousFontImageCache);
    // We want font images to have custom size, so don't scale them in the tooltip
    m_fontPreviewTooltipBackend->setScaleImage(false);
    // Note: Though the text specified here appears in UI, it shouldn't be translated, as it's
    // a commonly used sentence to preview the font glyphs in latin fonts.
    // For fonts that do not have latin glyphs, the font family name will have to suffice for preview.
    m_fontPreviewTooltipBackend->setAuxiliaryData(
        ImageCache::FontCollectorSizeAuxiliaryData{QSize{300, 150},
                                                   Theme::getColor(Theme::DStextColor).name(),
                                                   QStringLiteral("The quick brown fox jumps\n"
                                                                  "over the lazy dog\n"
                                                                  "1234567890")});
    // create assets widget
    m_assetsWidget->quickWidget()->setObjectName(Constants::OBJECT_NAME_ASSET_LIBRARY);
    m_assetsWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    Theme::setupTheme(m_assetsWidget->engine());
    m_assetsWidget->engine()->addImportPath(propertyEditorResourcesPath() + "/imports");
    m_assetsWidget->setClearColor(Theme::getColor(Theme::Color::QmlDesigner_BackgroundColorDarkAlternate));
    m_assetsWidget->engine()->addImageProvider("qmldesigner_assets", m_assetsIconProvider);

    connect(m_assetsModel, &AssetsLibraryModel::fileChanged,
            QmlDesignerPlugin::instance(), &QmlDesignerPlugin::assetChanged);

    connect(m_assetsModel, &AssetsLibraryModel::generatedAssetsDeleted,
            this, &AssetsLibraryWidget::handleDeletedGeneratedAssets);

    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(m_assetsWidget.get());

    updateSearch();

    setStyleSheet(Theme::replaceCssColors(
        Utils::FileUtils::fetchQrc(":/qmldesigner/stylesheet.css")));

    m_qmlSourceUpdateShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_F6), this);
    connect(m_qmlSourceUpdateShortcut, &QShortcut::activated, this, &AssetsLibraryWidget::reloadQmlSource);
    connect(this,
            &AssetsLibraryWidget::extFilesDrop,
            this,
            &AssetsLibraryWidget::handleExtFilesDrop,
            Qt::QueuedConnection);

    QmlDesignerPlugin::trackWidgetFocusTime(this, Constants::EVENT_ASSETSLIBRARY_TIME);

    auto map = m_assetsWidget->registerPropertyMap("AssetsLibraryBackend");

    map->setProperties({{"assetsModel", QVariant::fromValue(m_assetsModel)},
                        {"rootView", QVariant::fromValue(this)},
                        {"tooltipBackend", QVariant::fromValue(m_fontPreviewTooltipBackend.get())}});

    // init the first load of the QML UI elements
    reloadQmlSource();

    setFocusProxy(m_assetsWidget->quickWidget());

    IContext::attach(this,
                     Context(Constants::qmlAssetsLibraryContextId, Constants::qtQuickToolsMenuContextId),
                     [this](const IContext::HelpCallback &callback) { contextHelp(callback); });
}

AssetsLibraryWidget::~AssetsLibraryWidget() = default;

void AssetsLibraryWidget::contextHelp(const Core::IContext::HelpCallback &callback) const
{
    if (m_assetsView)
        QmlDesignerPlugin::contextHelp(callback, m_assetsView->contextHelpId());
    else
        callback({});
}

void AssetsLibraryWidget::deleteSelectedAssets()
{
    emit deleteSelectedAssetsRequested();
}

QString AssetsLibraryWidget::getUniqueEffectPath(const QString &parentFolder, const QString &effectName)
{
    QString effectsDir = ModelNodeOperations::getEffectsDefaultDirectory(parentFolder);
    QString effectPath = QLatin1String("%1/%2.qep").arg(effectsDir, effectName);

    return UniqueName::generatePath(effectPath);
}

bool AssetsLibraryWidget::createNewEffect(const QString &effectPath, bool openInEffectComposer)
{
    bool created = QFile(effectPath).open(QIODevice::WriteOnly);

    if (created && openInEffectComposer) {
        openEffectComposer(effectPath);
        emit directoryCreated(QFileInfo(effectPath).absolutePath());
    }

    return created;
}

bool AssetsLibraryWidget::isEffectsCreationAllowed() const
{
    if (!Core::ICore::isQtDesignStudio() || DesignerMcuManager::instance().isMCUProject())
        return false;

#ifdef LICENSECHECKER
    return checkLicense() == FoundLicense::enterprise;
#else
    return true;
#endif
}

void AssetsLibraryWidget::showInGraphicalShell(const QString &path)
{
    Core::FileUtils::showInGraphicalShell(Utils::FilePath::fromString(path));
}

QString AssetsLibraryWidget::showInGraphicalShellMsg() const
{
    return Core::FileUtils::msgGraphicalShellAction();
}

int AssetsLibraryWidget::qtVersion() const
{
    return QT_VERSION;
}

void AssetsLibraryWidget::addTextures(const QStringList &filePaths)
{
    m_assetsView->executeInTransaction(__FUNCTION__, [&] {
        CreateTexture(m_assetsView)
            .execute(filePaths,
                     AddTextureMode::Texture,
                     Utils3D::active3DSceneId(m_assetsView->model()));
    });
}

void AssetsLibraryWidget::addLightProbe(const QString &filePath)
{
    m_assetsView->executeInTransaction(__FUNCTION__, [&] {
        CreateTexture(m_assetsView)
            .execute(filePath,
                     AddTextureMode::LightProbe,
                     Utils3D::active3DSceneId(m_assetsView->model()));
    });
}

void AssetsLibraryWidget::updateContextMenuActionsEnableState()
{
    setHasMaterialLibrary(Utils3D::materialLibraryNode(m_assetsView).isValid()
                          && m_assetsView->model()->hasImport("QtQuick3D"));

    ModelNode activeSceneEnv = Utils3D::resolveSceneEnv(m_assetsView,
                                                        Utils3D::active3DSceneId(
                                                            m_assetsView->model()));
    setHasSceneEnv(activeSceneEnv.isValid());

    setCanCreateEffects(isEffectsCreationAllowed());
}

void AssetsLibraryWidget::setHasMaterialLibrary(bool enable)
{
    if (m_hasMaterialLibrary == enable)
        return;

    m_hasMaterialLibrary = enable;
    emit hasMaterialLibraryChanged();
}

bool AssetsLibraryWidget::hasMaterialLibrary() const
{
    return m_hasMaterialLibrary;
}

void AssetsLibraryWidget::setHasSceneEnv(bool b)
{
    if (b == m_hasSceneEnv)
        return;

    m_hasSceneEnv = b;
    emit hasSceneEnvChanged();
}

void AssetsLibraryWidget::handleDeletedGeneratedAssets(const QHash<QString, Utils::FilePath> &assetData)
{
    // assetData key: full type name including import, value: import dir

    // This method removes all nodes of the deleted type (assetData.keys())
    // and removes the import statement for that type

    DesignDocument *document = QmlDesignerPlugin::instance()->currentDesignDocument();
    if (!document)
        return;

    bool clearStacks = false;

    const Imports imports = m_assetsView->model()->imports();
    const GeneratedComponentUtils &compUtils = QmlDesignerPlugin::instance()->documentManager()
                                             .generatedComponentUtils();
    QString effectPrefix = compUtils.composedEffectsTypePrefix();
    QStringList effectNames;

    // Remove usages of deleted assets from the current document
    m_assetsView->executeInTransaction(__FUNCTION__, [&]() {
        QList<ModelNode> allNodes = m_assetsView->allModelNodes();

        QList<Import> removedImports;

        const QStringList assetTypes = assetData.keys();
        for (const QString &assetType : assetTypes) {
            QString removedImportUrl;
            int idx = assetType.lastIndexOf('.');
            if (idx >= 0) {
                if (assetType.startsWith(effectPrefix))
                    effectNames.append(assetType.sliced(idx + 1));
                removedImportUrl = assetType.first(idx);
#ifdef QDS_USE_PROJECTSTORAGE
                auto module = m_assetsView->model()->module(removedImportUrl.toUtf8(),
                                                            Storage::ModuleKind::QmlLibrary);
                auto metaInfo = m_assetsView->model()->metaInfo(module, assetType.sliced(idx + 1).toUtf8());
                for (ModelNode &node : allNodes) {
                    if (node.metaInfo() == metaInfo) {
#else
                TypeName type = assetType.toUtf8();
                for (ModelNode &node : allNodes) {
                    if (node.metaInfo().typeName() == type) {
#endif
                        clearStacks = true;
                        node.destroy();
                    }
                }
                if (!removedImportUrl.isEmpty()) {
                    Import removedImport = Utils::findOrDefault(imports,
                                                                [&removedImportUrl](const Import &import) {
                        return import.url() == removedImportUrl;
                    });
                    if (!removedImport.isEmpty())
                        removedImports.append(removedImport);
                }
            }
        }

        if (!removedImports.isEmpty()) {
            m_assetsView->model()->changeImports({}, removedImports);
            clearStacks = true;
        }
    });

    // The size check here is to weed out cases where project path somehow resolves
    // to just slash or drive + slash. (Shortest legal currentProjectDirPath() would be "/a/")
    if (m_assetsModel->currentProjectDirPath().size() < 4)
        return;

    Utils::FilePath effectsDir = ModelNodeOperations::getEffectsImportDirectory();

    // Delete the asset modules
    for (const Utils::FilePath &dir : assetData) {
        if (dir.exists() && dir.toFSPathString().startsWith(m_assetsModel->currentProjectDirPath())) {
            if (!dir.removeRecursively()) {
                QMessageBox::warning(Core::ICore::dialogParent(),
                                     Tr::tr("Failed to Delete Effect Resources"),
                                     Tr::tr("Could not delete \"%1\".").arg(dir.toUserOutput()));
            }
        }
    }

    // Reset undo stack as removing effect components cannot be undone, and thus the stack will
    // contain only unworkable states.
    if (clearStacks)
        document->clearUndoRedoStacks();

    m_assetsView->emitCustomNotification("effectcomposer_effects_deleted", {}, {effectNames});
    m_assetsView->emitCustomNotification("assets_deleted");
}

void AssetsLibraryWidget::updateAssetPreview(const QString &id, const QPixmap &pixmap,
                                             const QString &suffix)
{
    const QString thumb = m_assetsIconProvider->setPixmap(id, pixmap, suffix);

    if (!thumb.isEmpty())
        emit m_assetsModel->fileChanged(thumb);
}

void AssetsLibraryWidget::invalidateThumbnail(const QString &id)
{
    m_assetsIconProvider->invalidateThumbnail(id);
}

QSize AssetsLibraryWidget::imageSize(const QString &id)
{
    return m_assetsIconProvider->imageSize(id);
}

QString AssetsLibraryWidget::assetFileSize(const QString &id)
{
    qint64 fileSize = m_assetsIconProvider->fileSize(id);
    return QLocale::system().formattedDataSize(fileSize, 2, QLocale::DataSizeTraditionalFormat);
}

bool AssetsLibraryWidget::assetIsImageOrTexture(const QString &id)
{
    return Asset(id).isValidTextureSource();
}

bool AssetsLibraryWidget::assetIsImported3d(const QString &id)
{
    return Asset(id).isImported3D();
}

// needed to deal with "Object 0xXXXX destroyed while one of its QML signal handlers is in progress..." error which would lead to a crash
void AssetsLibraryWidget::invokeAssetsDrop(const QList<QUrl> &urls, const QString &targetDir)
{
    QMetaObject::invokeMethod(this, "handleAssetsDrop", Qt::QueuedConnection, Q_ARG(QList<QUrl>, urls), Q_ARG(QString, targetDir));
}

void AssetsLibraryWidget::handleAssetsDrop(const QList<QUrl> &urls, const QString &targetDir)
{
    if (urls.isEmpty() || targetDir.isEmpty())
        return;

    Utils::FilePath destDir = Utils::FilePath::fromUserInput(targetDir);

    QString resourceFolder = DocumentManager::currentResourcePath().toUrlishString();

    if (destDir.isFile())
        destDir = destDir.parentDir();

    QMessageBox msgBox;
    msgBox.setInformativeText("What would you like to do with the existing asset?");
    msgBox.addButton("Keep Both", QMessageBox::AcceptRole);
    msgBox.addButton("Replace", QMessageBox::ResetRole);
    msgBox.addButton("Cancel", QMessageBox::RejectRole);

    for (const QUrl &url : urls) {
        Utils::FilePath src = Utils::FilePath::fromUrl(url);
        Utils::FilePath dest = destDir.pathAppended(src.fileName());

        if (destDir == src.parentDir() || !src.startsWith(resourceFolder))
            continue;

        if (dest.exists()) {
            msgBox.setText("An asset named " + dest.fileName() + " already exists.");
            msgBox.exec();
            int userAction = msgBox.buttonRole(msgBox.clickedButton());

            if (userAction == QMessageBox::AcceptRole) { // "Keep Both"
                dest = Utils::FilePath::fromString(UniqueName::generatePath(dest.toUrlishString()));
            } else if (userAction == QMessageBox::ResetRole && dest.exists()) { // "Replace"
                if (!dest.removeFile()) {
                    qWarning() << __FUNCTION__ << "Failed to remove existing file" << dest;
                    continue;
                }
            } else if (userAction == QMessageBox::RejectRole) { // "Cancel"
                continue;
            }
        }

        bool isDir = src.isDir();

        if (src.renameFile(dest)) {
            if (isDir)
                m_assetsModel->updateExpandPath(src, dest);
        } else if (isDir) {
            Core::AsynchronousMessageBox::warning(
                Tr::tr("Folder move failure"),
                Tr::tr("Failed to move folder \"%1\". The folder might contain subfolders or one "
                       "of its files is in use.")
                    .arg(src.fileName()));
        }
    }

    if (m_assetsView->model())
        m_assetsView->model()->endDrag();
}

QList<QToolButton *> AssetsLibraryWidget::createToolBarWidgets()
{
    return {};
}

void AssetsLibraryWidget::handleSearchFilterChanged(const QString &filterText)
{
    if (filterText == m_filterText || (m_assetsModel->isEmpty()
                                       && filterText.contains(m_filterText, Qt::CaseInsensitive)))
        return;

    m_filterText = filterText;
    updateSearch();
}

void AssetsLibraryWidget::handleAddAsset()
{
    addResources({});
}

void AssetsLibraryWidget::emitExtFilesDrop(const QList<QUrl> &simpleFilePaths,
                                           const QList<QUrl> &complexFilePaths,
                                           const QString &targetDirPath)
{
    // workaround for but QDS-8010: we need to postpone the call to handleExtFilesDrop, otherwise
    // the TreeViewDelegate might be recreated (therefore, destroyed) while we're still in a handler
    // of a QML DropArea which is a child of the delegate being destroyed - this would cause a crash.
    emit extFilesDrop(simpleFilePaths, complexFilePaths, targetDirPath);
}

void AssetsLibraryWidget::handleExtFilesDrop(const QList<QUrl> &simpleFilePaths,
                                             const QList<QUrl> &complexFilePaths,
                                             const QString &targetDirPath)
{
    QStringList simpleFilePathStrings = Utils::transform<QStringList>(simpleFilePaths,
                                                                      &QUrl::toLocalFile);
    QStringList complexFilePathStrings = Utils::transform<QStringList>(complexFilePaths,
                                                                       &QUrl::toLocalFile);

    if (!simpleFilePathStrings.isEmpty()) {
        if (targetDirPath.isEmpty()) {
            addResources(simpleFilePathStrings, false);
        } else {
            AddFilesResult result = ModelNodeOperations::addFilesToProject(simpleFilePathStrings,
                                                                           targetDirPath,
                                                                           false);
            if (result.status() == AddFilesResult::Failed) {
                QWidget *w = Core::AsynchronousMessageBox::warning(
                    Tr::tr("Failed to Add Files"),
                    Tr::tr("Could not add %1 to project.").arg(simpleFilePathStrings.join(' ')));
                // Avoid multiple modal dialogs open at the same time
                auto mb = qobject_cast<QMessageBox *>(w);
                if (mb && !complexFilePathStrings.empty())
                    mb->exec();
            }
        }
    }

    if (!complexFilePathStrings.empty())
        addResources(complexFilePathStrings, false);

    m_assetsView->model()->endDrag();
}

QSet<QString> AssetsLibraryWidget::supportedAssetSuffixes(bool complex)
{
    const QList<AddResourceHandler> handlers = QmlDesignerPlugin::instance()->viewManager()
                                                   .designerActionManager().addResourceHandler();

    QSet<QString> suffixes;
    for (const AddResourceHandler &handler : handlers) {
        if (Asset::isSupported(handler.filter) != complex)
            suffixes.insert(handler.filter);
    }

    return suffixes;
}

void AssetsLibraryWidget::openEffectComposer(const QString &filePath)
{
    ModelNodeOperations::openEffectComposer(filePath);
}

void AssetsLibraryWidget::editAssetComponent(const QString &filePath)
{
    Utils::FilePath fullPath = QmlDesignerPlugin::instance()->documentManager()
                                   .generatedComponentUtils().getImported3dQml(filePath);
    if (fullPath.exists())
        DocumentManager::goIntoComponent(fullPath.toFSPathString());
}

void AssetsLibraryWidget::updateAssetComponent(const QString &filePath)
{
    Utils::FilePath qml = QmlDesignerPlugin::instance()->documentManager()
                              .generatedComponentUtils().getImported3dQml(filePath);
    if (qml.exists())
        m_assetsView->emitCustomNotification("UpdateImported3DAsset", {}, {qml.toFSPathString()});
}

QString AssetsLibraryWidget::qmlSourcesPath()
{
#ifdef SHARE_QML_PATH
    if (Utils::qtcEnvironmentVariableIsSet("LOAD_QML_FROM_SOURCE"))
        return QLatin1String(SHARE_QML_PATH) + "/assetsLibraryQmlSources";
#endif
    return Core::ICore::resourcePath("qmldesigner/assetsLibraryQmlSources").toUrlishString();
}

void AssetsLibraryWidget::clearSearchFilter()
{
    QMetaObject::invokeMethod(m_assetsWidget->rootObject(), "clearSearchFilter");
}

void AssetsLibraryWidget::reloadQmlSource()
{
    const QString assetsQmlPath = qmlSourcesPath() + "/Assets.qml";
    QTC_ASSERT(QFileInfo::exists(assetsQmlPath), return);
    m_assetsWidget->setSource(QUrl::fromLocalFile(assetsQmlPath));
}

void AssetsLibraryWidget::updateSearch()
{
    m_assetsModel->setSearchText(m_filterText);
}

void AssetsLibraryWidget::setIsDragging(bool val)
{
    if (m_isDragging != val) {
        m_isDragging = val;
        emit isDraggingChanged();
    }
}

void AssetsLibraryWidget::setResourcePath(const QString &resourcePath)
{
    m_assetsModel->setRootPath(resourcePath);
    m_assetsIconProvider->clearCache();
    updateSearch();
}

void AssetsLibraryWidget::startDragAsset(const QStringList &assetPaths, const QPointF &mousePos)
{
    // Actual drag is created after mouse has moved to avoid a QDrag bug that causes drag to stay
    // active (and blocks mouse release) if mouse is released at the same spot of the drag start.
    m_assetsToDrag = assetPaths;
    m_dragStartPoint = mousePos.toPoint();
    setIsDragging(true);
}

QPair<QString, QByteArray> AssetsLibraryWidget::getAssetTypeAndData(const QString &assetPath)
{
    Asset asset(assetPath);
    if (asset.hasSuffix()) {
        if (asset.isImage()) {
            // Data: Image format (suffix)
            return {Constants::MIME_TYPE_ASSET_IMAGE, asset.suffix().toUtf8()};
        } else if (asset.isFont()) {
            // Data: Font family name
            QRawFont font(assetPath, 10);
            QString fontFamily = font.isValid() ? font.familyName() : "";
            return {Constants::MIME_TYPE_ASSET_FONT, fontFamily.toUtf8()};
        } else if (asset.isShader()) {
            // Data: shader type, frament (f) or vertex (v)
            return {Constants::MIME_TYPE_ASSET_SHADER,
                asset.isFragmentShader() ? "f" : "v"};
        } else if (asset.isAudio()) {
            // No extra data for sounds
            return {Constants::MIME_TYPE_ASSET_SOUND, {}};
        } else if (asset.isVideo()) {
            // No extra data for videos
            return {Constants::MIME_TYPE_ASSET_VIDEO, {}};
        } else if (asset.isTexture3D()) {
            // Data: Image format (suffix)
            return {Constants::MIME_TYPE_ASSET_TEXTURE3D, asset.suffix().toUtf8()};
        } else if (asset.isEffect()) {
            // Data: Effect Composer format (suffix)
            return {Constants::MIME_TYPE_ASSET_EFFECT, asset.suffix().toUtf8()};
        } else if (asset.isImported3D()) {
            // Data: Imported 3D component (suffix)
            return {Constants::MIME_TYPE_ASSET_IMPORTED3D, asset.suffix().toUtf8()};
        }
    }
    return {};
}

static QHash<QByteArray, QStringList> allImageFormats()
{
    const QList<QByteArray> mimeTypes = QImageReader::supportedMimeTypes();
    auto transformer = [](const QByteArray& format) -> QString { return QString("*.") + format; };
    QHash<QByteArray, QStringList> imageFormats;
    for (const auto &mimeType : mimeTypes)
        imageFormats.insert(mimeType, Utils::transform(QImageReader::imageFormatsForMimeType(mimeType), transformer));
    imageFormats.insert("image/vnd.radiance", {"*.hdr"});
    imageFormats.insert("image/ktx", {"*.ktx"});

    return imageFormats;
}

void AssetsLibraryWidget::addResources(const QStringList &files, bool showDialog)
{
    clearSearchFilter();

    DesignDocument *document = QmlDesignerPlugin::instance()->currentDesignDocument();

    QTC_ASSERT(document, return);

    const QList<AddResourceHandler> handlers = QmlDesignerPlugin::instance()->viewManager()
                                                   .designerActionManager().addResourceHandler();

    QStringList fileNames = files;
    if (fileNames.isEmpty()) { // if no files, show the "add assets" dialog
        QMultiMap<QString, QString> map;
        QHash<QString, int> priorities;
        for (const AddResourceHandler &handler : handlers) {
            map.insert(handler.category, handler.filter);
            priorities.insert(handler.category, handler.piority);
        }

        QStringList sortedKeys = map.uniqueKeys();
        Utils::sort(sortedKeys, [&priorities](const QString &first, const QString &second) {
            return priorities.value(first) < priorities.value(second);
        });

        QStringList filters{Core::DocumentManager::allFilesFilterString()};
        QString filterTemplate = "%1 (%2)";
        for (const QString &key : std::as_const(sortedKeys)) {
            const QStringList values = map.values(key);
            if (values.contains("*.png")) { // Avoid long filter for images by splitting
                const QHash<QByteArray, QStringList> imageFormats = allImageFormats();
                QHash<QByteArray, QStringList>::const_iterator i = imageFormats.constBegin();
                while (i != imageFormats.constEnd()) {
                    filters.append(filterTemplate.arg(key + QString::fromLatin1(i.key()), i.value().join(' ')));
                    ++i;
                }
            } else {
                filters.append(filterTemplate.arg(key, values.join(' ')));
            }
        }

        static QString lastDir;
        const QString currentDir = lastDir.isEmpty() ? document->fileName().parentDir().toUrlishString() : lastDir;

        fileNames = QFileDialog::getOpenFileNames(Core::ICore::dialogParent(),
                                                  Tr::tr("Add Assets"),
                                                  currentDir,
                                                  filters.join(";;"));

        if (!fileNames.isEmpty())
            lastDir = QFileInfo(fileNames.first()).absolutePath();
    }

    QHash<QString, QString> filterToCategory;
    QHash<QString, AddResourceOperation> categoryToOperation;
    for (const AddResourceHandler &handler : handlers) {
        filterToCategory.insert(handler.filter, handler.category);
        categoryToOperation.insert(handler.category, handler.operation);
    }

    QMultiMap<QString, QString> categoryFileNames; // filenames grouped by category

    for (const QString &fileName : std::as_const(fileNames)) {
        const QString suffix = "*." + QFileInfo(fileName).suffix().toLower();
        const QString category = filterToCategory.value(suffix);
        categoryFileNames.insert(category, fileName);
    }

    QStringList unsupportedFiles;
    QStringList failedOpsFiles;

    for (const QString &category : categoryFileNames.uniqueKeys()) {
        QStringList fileNames = categoryFileNames.values(category);
        AddResourceOperation operation = categoryToOperation.value(category);
        QmlDesignerPlugin::emitUsageStatistics(Constants::EVENT_RESOURCE_IMPORTED + category);
        if (operation) {
            AddFilesResult result = operation(fileNames,
                                              document->fileName().parentDir().toUrlishString(), showDialog);
            if (result.status() == AddFilesResult::Failed) {
                failedOpsFiles.append(fileNames);
            } else {
                if (!result.directory().isEmpty()) {
                    emit directoryCreated(result.directory());
                } else if (result.haveDelayedResult()) {
                    QObject *delayedResult = result.delayedResult();
                    QObject::connect(delayedResult, &QObject::destroyed, this, [this, delayedResult]() {
                        QVariant propValue = delayedResult->property(AddFilesResult::directoryPropName);
                        QString directory = propValue.toString();
                        if (!directory.isEmpty())
                            emit directoryCreated(directory);
                    });
                }
            }
        } else {
            unsupportedFiles.append(fileNames);
        }
    }

    if (!failedOpsFiles.isEmpty()) {
        QWidget *w = Core::AsynchronousMessageBox::warning(Tr::tr("Failed to Add Files"),
                                                           Tr::tr("Could not add %1 to project.")
                                                               .arg(failedOpsFiles.join(' ')));
        // Avoid multiple modal dialogs open at the same time
        auto mb = qobject_cast<QMessageBox *>(w);
        if (mb && !unsupportedFiles.isEmpty())
            mb->exec();
    }
    if (!unsupportedFiles.isEmpty()) {
        Core::AsynchronousMessageBox::warning(
            Tr::tr("Failed to Add Files"),
            Tr::tr("Could not add %1 to project. Unsupported file format.")
                .arg(unsupportedFiles.join(' ')));
    }
}

void AssetsLibraryWidget::addAssetsToContentLibrary(const QStringList &assetPaths)
{
    QmlDesignerPlugin::instance()->mainWidget()->showDockWidget("ContentLibrary");
    m_assetsView->emitCustomNotification("add_assets_to_content_lib", {}, {assetPaths});
}

void AssetsLibraryWidget::setCanCreateEffects(bool newVal)
{
    if (m_canCreateEffects == newVal)
        return;

    m_canCreateEffects = newVal;
    emit canCreateEffectsChanged();
}

bool AssetsLibraryWidget::canCreateEffects() const
{
    return m_canCreateEffects;
}

} // namespace QmlDesigner
