// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "assetslibrarywidget.h"

#include "assetslibraryiconprovider.h"
#include "assetslibrarymodel.h"
#include "assetslibraryview.h"

#include <designeractionmanager.h>
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

#include <coreplugin/fileutils.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagebox.h>

#include <utils/algorithm.h>
#include <qmldesignerutils/asset.h>
#include <utils/environment.h>
#include <utils/filepath.h>
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

namespace QmlDesigner {

static QString propertyEditorResourcesPath()
{
#ifdef SHARE_QML_PATH
    if (Utils::qtcEnvironmentVariableIsSet("LOAD_QML_FROM_SOURCE"))
        return QLatin1String(SHARE_QML_PATH) + "/propertyEditorQmlSources";
#endif
    return Core::ICore::resourcePath("qmldesigner/propertyEditorQmlSources").toString();
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

                QList<QUrl> urlsToDrag = Utils::transform(m_assetsToDrag, [](const QString &path) {
                    return QUrl::fromLocalFile(path);
                });

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

AssetsLibraryWidget::AssetsLibraryWidget(AsynchronousImageCache &asynchronousFontImageCache,
                                         SynchronousImageCache &synchronousFontImageCache,
                                         AssetsLibraryView *view)
    : m_itemIconSize{24, 24}
    , m_fontImageCache{synchronousFontImageCache}
    , m_assetsIconProvider{new AssetsLibraryIconProvider(synchronousFontImageCache)}
    , m_assetsModel{new AssetsLibraryModel(this)}
    , m_assetsView{view}
    , m_createTextures{view}
    , m_assetsWidget{Utils::makeUniqueObjectPtr<StudioQuickWidget>(this)}
{
    setWindowTitle(tr("Assets Library", "Title of assets library widget"));
    setMinimumWidth(250);

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

    connect(m_assetsModel, &AssetsLibraryModel::effectsDeleted,
            this, &AssetsLibraryWidget::handleDeleteEffects);

    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(m_assetsWidget.get());

    updateSearch();

    setStyleSheet(Theme::replaceCssColors(
        QString::fromUtf8(Utils::FileReader::fetchQrc(":/qmldesigner/stylesheet.css"))));

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

bool AssetsLibraryWidget::canCreateEffects() const
{
    if (!Core::ICore::isQtDesignStudio())
        return false;

#ifdef LICENSECHECKER
    return checkLicense() == FoundLicense::enterprise;
#else
    return true;
#endif
}

void AssetsLibraryWidget::showInGraphicalShell(const QString &path)
{
    Core::FileUtils::showInGraphicalShell(Core::ICore::dialogParent(), Utils::FilePath::fromString(path));
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
        m_createTextures.execute(filePaths,
                                 AddTextureMode::Texture,
                                 Utils3D::active3DSceneId(m_assetsView->model()));
    });
}

void AssetsLibraryWidget::addLightProbe(const QString &filePath)
{
    m_assetsView->executeInTransaction(__FUNCTION__, [&] {
        m_createTextures.execute({filePath},
                                 AddTextureMode::LightProbe,
                                 Utils3D::active3DSceneId(m_assetsView->model()));
    });
}

void AssetsLibraryWidget::updateContextMenuActionsEnableState()
{
    setHasMaterialLibrary(Utils3D::materialLibraryNode(m_assetsView).isValid()
                          && m_assetsView->model()->hasImport("QtQuick3D"));

    ModelNode activeSceneEnv = m_createTextures.resolveSceneEnv(
        Utils3D::active3DSceneId(m_assetsView->model()));
    setHasSceneEnv(activeSceneEnv.isValid());
}

void AssetsLibraryWidget::setHasMaterialLibrary(bool enable)
{
    if (m_hasMaterialLibrary == enable)
        return;

    m_hasMaterialLibrary = enable;
    emit hasMaterialLibraryChanged();
}

void AssetsLibraryWidget::setHasSceneEnv(bool b)
{
    if (b == m_hasSceneEnv)
        return;

    m_hasSceneEnv = b;
    emit hasSceneEnvChanged();
}

void AssetsLibraryWidget::handleDeleteEffects([[maybe_unused]] const QStringList &effectNames)
{
#ifdef QDS_USE_PROJECTSTORAGE
// That code has to rewritten with modules. Seem try to find all effects nodes.
#else
    DesignDocument *document = QmlDesignerPlugin::instance()->currentDesignDocument();
    if (!document)
        return;

    bool clearStacks = false;

    // Remove usages of deleted effects from the current document
    m_assetsView->executeInTransaction(__FUNCTION__, [&]() {
        QList<ModelNode> allNodes = m_assetsView->allModelNodes();
        const QString typeTemplate = "%1.%2.%2";
        const QString importUrlTemplate = "%1.%2";
        const Imports imports = m_assetsView->model()->imports();
        Imports removedImports;
        const QString typePrefix = QmlDesignerPlugin::instance()->documentManager()
                                       .generatedComponentUtils().composedEffectsTypePrefix();
        for (const QString &effectName : effectNames) {
            if (effectName.isEmpty())
                continue;
            const TypeName type = typeTemplate.arg(typePrefix, effectName).toUtf8();
            for (ModelNode &node : allNodes) {
                if (node.metaInfo().typeName() == type) {
                    clearStacks = true;
                    node.destroy();
                }
            }

            const QString importPath = importUrlTemplate.arg(typePrefix, effectName);
            Import removedImport = Utils::findOrDefault(imports, [&importPath](const Import &import) {
                return import.url() == importPath;
            });
            if (!removedImport.isEmpty())
                removedImports.append(removedImport);
        }

        if (!removedImports.isEmpty()) {
            m_assetsView->model()->changeImports({}, removedImports);
            clearStacks = true;
        }
    });

    // The size check here is to weed out cases where project path somehow resolves
    // to just slash. Shortest legal currentProjectDirPath() would be "/a/".
    if (m_assetsModel->currentProjectDirPath().size() < 3)
        return;

    Utils::FilePath effectsDir = ModelNodeOperations::getEffectsImportDirectory();

    // Delete the effect modules
    for (const QString &effectName : effectNames) {
        Utils::FilePath eDir = effectsDir.pathAppended(effectName);
        if (eDir.exists() && eDir.toString().startsWith(m_assetsModel->currentProjectDirPath())) {
            QString error;
            eDir.removeRecursively(&error);
            if (!error.isEmpty()) {
                QMessageBox::warning(Core::ICore::dialogParent(),
                                     tr("Failed to Delete Effect Resources"),
                                     tr("Could not delete \"%1\".").arg(eDir.toString()));
            }
        }
    }

    // Reset undo stack as removing effect components cannot be undone, and thus the stack will
    // contain only unworkable states.
    if (clearStacks)
        document->clearUndoRedoStacks();

    m_assetsView->emitCustomNotification("effectcomposer_effects_deleted", {}, {effectNames});
#endif
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

    QString resourceFolder = DocumentManager::currentResourcePath().toString();

    if (destDir.isFile())
        destDir = destDir.parentDir();

    QMessageBox mb;
    mb.setInformativeText("What would you like to do with the existing asset?");
    mb.addButton("Keep Both", QMessageBox::AcceptRole);
    mb.addButton("Replace", QMessageBox::ResetRole);
    mb.addButton("Cancel", QMessageBox::RejectRole);

    for (const QUrl &url : urls) {
        Utils::FilePath src = Utils::FilePath::fromUrl(url);
        Utils::FilePath dest = destDir.pathAppended(src.fileName());

        if (destDir == src.parentDir() || !src.startsWith(resourceFolder))
            continue;

        if (dest.exists()) {
            mb.setText("An asset named " + dest.fileName() + " already exists.");
            mb.exec();
            int userAction = mb.buttonRole(mb.clickedButton());

            if (userAction == QMessageBox::AcceptRole) { // "Keep Both"
                dest = Utils::FilePath::fromString(UniqueName::generatePath(dest.toString()));
            } else if (userAction == QMessageBox::ResetRole && dest.exists()) { // "Replace"
                if (!dest.removeFile()) {
                    qWarning() << __FUNCTION__ << "Failed to remove existing file" << dest;
                    continue;
                }
            } else if (userAction == QMessageBox::RejectRole) { // "Cancel"
                continue;
            }
        }

        if (!src.renameFile(dest))
            qWarning() << __FUNCTION__ << "Failed to move asset from" << src << "to" << dest;
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
    if (filterText == m_filterText || (!m_assetsModel->hasFiles()
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
    auto toLocalFile = [](const QUrl &url) { return url.toLocalFile(); };

    QStringList simpleFilePathStrings = Utils::transform<QStringList>(simpleFilePaths, toLocalFile);
    QStringList complexFilePathStrings = Utils::transform<QStringList>(complexFilePaths, toLocalFile);

    if (!simpleFilePathStrings.isEmpty()) {
        if (targetDirPath.isEmpty()) {
            addResources(simpleFilePathStrings, false);
        } else {
            AddFilesResult result = ModelNodeOperations::addFilesToProject(simpleFilePathStrings,
                                                                           targetDirPath,
                                                                           false);
            if (result.status() == AddFilesResult::Failed) {
                QWidget *w = Core::AsynchronousMessageBox::warning(tr("Failed to Add Files"),
                                                                   tr("Could not add %1 to project.")
                                                                       .arg(simpleFilePathStrings.join(' ')));
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

QString AssetsLibraryWidget::qmlSourcesPath()
{
#ifdef SHARE_QML_PATH
    if (Utils::qtcEnvironmentVariableIsSet("LOAD_QML_FROM_SOURCE"))
        return QLatin1String(SHARE_QML_PATH) + "/assetsLibraryQmlSources";
#endif
    return Core::ICore::resourcePath("qmldesigner/assetsLibraryQmlSources").toString();
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

        QStringList filters { tr("All Files (%1)").arg("*.*") };
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
        const QString currentDir = lastDir.isEmpty() ? document->fileName().parentDir().toString() : lastDir;

        fileNames = QFileDialog::getOpenFileNames(Core::ICore::dialogParent(),
                                                  tr("Add Assets"),
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
                                              document->fileName().parentDir().toString(), showDialog);
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
        QWidget *w = Core::AsynchronousMessageBox::warning(tr("Failed to Add Files"),
                                                           tr("Could not add %1 to project.")
                                                               .arg(failedOpsFiles.join(' ')));
        // Avoid multiple modal dialogs open at the same time
        auto mb = qobject_cast<QMessageBox *>(w);
        if (mb && !unsupportedFiles.isEmpty())
            mb->exec();
    }
    if (!unsupportedFiles.isEmpty()) {
        Core::AsynchronousMessageBox::warning(tr("Failed to Add Files"),
                                              tr("Could not add %1 to project. Unsupported file format.")
                                                  .arg(unsupportedFiles.join(' ')));
    }
}

void AssetsLibraryWidget::addAssetsToContentLibrary(const QStringList &assetPaths)
{
    QmlDesignerPlugin::instance()->mainWidget()->showDockWidget("ContentLibrary");
    m_assetsView->emitCustomNotification("add_assets_to_content_lib", {}, {assetPaths});
}

} // namespace QmlDesigner
