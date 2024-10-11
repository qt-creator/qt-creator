// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "assetslibraryview.h"

#include "assetslibrarywidget.h"
#include "designmodecontext.h"
#include "qmldesignerplugin.h"

#include <asynchronousimagecache.h>
#include <bindingproperty.h>
#include <coreplugin/icore.h>
#include <imagecache/imagecachegenerator.h>
#include <imagecache/imagecachestorage.h>
#include <imagecache/timestampprovider.h>
#include <imagecachecollectors/imagecachecollector.h>
#include <imagecachecollectors/imagecacheconnectionmanager.h>
#include <imagecachecollectors/imagecachefontcollector.h>
#include <nodelistproperty.h>
#include <qmldesignerconstants.h>
#include <utils3d.h>

#include <projectexplorer/kit.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/target.h>
#include <qmlitemnode.h>
#include <rewriterview.h>
#include <sqlitedatabase.h>
#include <synchronousimagecache.h>
#include <utils/algorithm.h>
#include <utils/filepath.h>

namespace QmlDesigner {

class AssetsLibraryView::ImageCacheData
{
public:
    Sqlite::Database database{Utils::PathString{
                                  Core::ICore::cacheResourcePath("fontimagecache.db").toString()},
                              Sqlite::JournalMode::Wal,
                              Sqlite::LockingMode::Normal};
    ImageCacheStorage<Sqlite::Database> storage{database};
    ImageCacheFontCollector fontCollector;
    ImageCacheGenerator fontGenerator{fontCollector, storage};
    TimeStampProvider timeStampProvider;
    AsynchronousImageCache asynchronousFontImageCache{storage, fontGenerator, timeStampProvider};
    SynchronousImageCache synchronousFontImageCache{storage, timeStampProvider, fontCollector};
    AsynchronousImageCache *mainImageCache = {};
};

AssetsLibraryView::AssetsLibraryView(AsynchronousImageCache &imageCache,
                                     ExternalDependenciesInterface &externalDependencies)
    : AbstractView{externalDependencies}
{
    imageCacheData()->mainImageCache = &imageCache;

    m_matSyncTimer.callOnTimeout(this, &AssetsLibraryView::syncMaterialsMetaData);
    m_matSyncTimer.setInterval(500);
    m_matSyncTimer.setSingleShot(true);

    m_3dImportsSyncTimer.callOnTimeout(this, &AssetsLibraryView::sync3dImportsMetaData);
    m_3dImportsSyncTimer.setInterval(500);
    m_3dImportsSyncTimer.setSingleShot(true);
}

AssetsLibraryView::~AssetsLibraryView()
{}

bool AssetsLibraryView::hasWidget() const
{
    return true;
}

WidgetInfo AssetsLibraryView::widgetInfo()
{
    if (!m_widget) {
        m_widget = Utils::makeUniqueObjectPtr<AssetsLibraryWidget>(
            *imageCacheData()->mainImageCache,
            imageCacheData()->asynchronousFontImageCache,
            imageCacheData()->synchronousFontImageCache,
            this);

        auto context = new Internal::AssetsLibraryContext(m_widget.get());
        Core::ICore::addContextObject(context);
    }

    return createWidgetInfo(m_widget.get(), "Assets", WidgetInfo::LeftPane, tr("Assets"));
}

void AssetsLibraryView::customNotification(const AbstractView * /*view*/,
                                           const QString &identifier,
                                           const QList<ModelNode> & /*nodeList*/,
                                           const QList<QVariant> & /*data*/)
{
    if (identifier == "delete_selected_assets")
        m_widget->deleteSelectedAssets();
    else if (identifier == "asset_import_finished")
        m_3dImportsSyncTimer.start();
}

void AssetsLibraryView::modelAttached(Model *model)
{
    AbstractView::modelAttached(model);

    m_widget->clearSearchFilter();

    setResourcePath(DocumentManager::currentResourcePath().toFSPathString());

    m_matLibRetries = 0;
    m_matSyncTimer.start();
    m_3dImportsSyncTimer.start();
}

void AssetsLibraryView::modelAboutToBeDetached(Model *model)
{
    AbstractView::modelAboutToBeDetached(model);
}

void AssetsLibraryView::modelNodePreviewPixmapChanged(const ModelNode &node,
                                                      const QPixmap &pixmap,
                                                      const QByteArray &requestId)
{
    if (!node.metaInfo().isQtQuick3DMaterial())
        return;

    // There might be multiple requests for different preview pixmap sizes.
    // Here only the one with the default size is picked.
    if (requestId.isEmpty())
        m_widget->updateAssetPreview(node.id(), pixmap, "mat");
}

void AssetsLibraryView::nodeReparented(const ModelNode &node,
                                       const NodeAbstractProperty &newPropertyParent,
                                       const NodeAbstractProperty &oldPropertyParent,
                                       PropertyChangeFlags propertyChange)
{
    if (node.metaInfo().isQtQuick3DMaterial())
        m_matSyncTimer.start();
}

void AssetsLibraryView::nodeAboutToBeRemoved(const ModelNode &removedNode)
{
    if (removedNode.metaInfo().isQtQuick3DMaterial())
        m_matSyncTimer.start();
}

void AssetsLibraryView::nodeIdChanged(const ModelNode &node, const QString &newId,
                                      const QString &oldId)
{
    if (node.metaInfo().isQtQuick3DMaterial())
        m_matSyncTimer.start();
}

void AssetsLibraryView::setResourcePath(const QString &resourcePath)
{
    if (resourcePath == m_lastResourcePath)
        return;

    m_lastResourcePath = resourcePath;

    if (!m_widget) {
        m_widget = Utils::makeUniqueObjectPtr<AssetsLibraryWidget>(
            *imageCacheData()->mainImageCache,
            imageCacheData()->asynchronousFontImageCache,
            imageCacheData()->synchronousFontImageCache,
            this);
    }

    m_widget->setResourcePath(resourcePath);
}

void AssetsLibraryView::syncMaterialsMetaData()
{
    if (!model())
        return;

    // TODO: PoC implementation expects there to be material library node, so it won't properly
    //       clean up if material library node is removed from the project
    ModelNode matLib = Utils3D::materialLibraryNode(this);

    if (!matLib) {
        // For some reason material library isn't valid immediately after model attach
        if (++m_matLibRetries > 20) {
            qWarning() << __FUNCTION__ << "Could not resolve material library node.";
            return;
        }
        m_matSyncTimer.start();
        return;
    }

    Utils::FilePath resPath = DocumentManager::currentResourcePath();

    QHash<QString, Utils::FilePath> matFiles = collectFiles(resPath, "mat");

    const QList<ModelNode> matLibNodes = matLib.directSubModelNodes();
    for (const ModelNode &node : matLibNodes) {
        if (node && node.metaInfo().isQtQuick3DMaterial()) {
            if (matFiles.contains(node.id())) {
                matFiles.remove(node.id());
            } else {
                Utils::FilePath newFile = resPath.pathAppended("/" + node.id() + ".mat");
                newFile.writeFileContents(node.id().toLatin1());
            }
        }
    }

    // Remove .mat files that do not match any id
    for (const Utils::FilePath &f : std::as_const(matFiles))
        f.removeFile();
}

// recursively find files of certain suffix in a dir
QHash<QString, Utils::FilePath> AssetsLibraryView::collectFiles(const Utils::FilePath &dirPath,
                                                                const QString &suffix)
{
    if (dirPath.isEmpty())
        return {};

    QHash<QString, Utils::FilePath> files;

    Utils::FilePaths entryList = dirPath.dirEntries(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
    for (const Utils::FilePath &entry : entryList) {
        if (entry.isDir()) {
            files.insert(collectFiles(entry.absoluteFilePath(), suffix));
        } else if (entry.suffix() == suffix) {
            QString baseName = entry.baseName();
            files.insert(baseName, entry);
        }
    }

    return files;
}

void AssetsLibraryView::sync3dImportsMetaData()
{
    if (!model())
        return;

    // Sync generated 3d imports to .3d files in project content
    auto import3dPath = Utils::FilePath::fromString(
        QmlDesignerPlugin::instance()->documentManager()
            .generatedComponentUtils().import3dTypePath());
    auto projPath = Utils::FilePath::fromString(externalDependencies().currentProjectDirPath());
    auto fullPath = projPath.resolvePath(import3dPath);

    if (fullPath.isEmpty())
        return;

    const Utils::FilePaths dirs = fullPath.dirEntries(QDir::Dirs | QDir::NoDotAndDotDot);

    Utils::FilePath resPath = DocumentManager::currentResourcePath();

    QHash<QString, Utils::FilePath> files = collectFiles(resPath, "3d");

    for (const Utils::FilePath &dir : dirs) {
        const QString dirStr = dir.fileName();
        if (files.contains(dirStr)) {
            files.remove(dirStr);
        } else {
            Utils::FilePath newFile = resPath.pathAppended("/" + dirStr + ".3d");
            newFile.writeFileContents(files[dirStr].toFSPathString().toLatin1());
        }
    }

    // Remove .3d files that do not match any imported 3d
    for (const Utils::FilePath &f : std::as_const(files))
        f.removeFile();
}

AssetsLibraryView::ImageCacheData *AssetsLibraryView::imageCacheData()
{
    std::call_once(imageCacheFlag,
                   [this] { m_imageCacheData = std::make_unique<ImageCacheData>(); });
    return m_imageCacheData.get();
}

} // namespace QmlDesigner
