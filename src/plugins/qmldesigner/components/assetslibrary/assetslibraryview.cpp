// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "assetslibraryview.h"

#include "assetslibrarywidget.h"
#include "qmldesignerplugin.h"

#include <asynchronousimagecache.h>
#include <bindingproperty.h>
#include <coreplugin/icore.h>
#include <imagecache/imagecachegenerator.h>
#include <imagecache/imagecachestorage.h>
#include <imagecache/timestampprovider.h>
#include <imagecachecollectors/imagecachecollector.h>
#include <imagecachecollectors/imagecachefontcollector.h>
#include <modelnodeoperations.h>
#include <nodelistproperty.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/target.h>
#include <qmlitemnode.h>
#include <rewriterview.h>
#include <sqlitedatabase.h>
#include <synchronousimagecache.h>
#include <utils/algorithm.h>
#include <utils3d.h>

namespace QmlDesigner {

class AssetsLibraryView::ImageCacheData
{
public:
    Sqlite::Database database{Utils::PathString{
                                  Core::ICore::cacheResourcePath("fontimagecache.db").toUrlishString()},
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
    m_3dImportsSyncTimer.callOnTimeout(this, &AssetsLibraryView::sync3dImports);
    m_3dImportsSyncTimer.setInterval(1000);
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
    }

    return createWidgetInfo(m_widget.get(), "Assets", WidgetInfo::LeftPane, tr("Assets"));
}

void AssetsLibraryView::customNotification(const AbstractView * /*view*/,
                                           const QString &identifier,
                                           const QList<ModelNode> & /*nodeList*/,
                                           const QList<QVariant> & /*data*/)
{
    if (identifier == "delete_selected_assets") {
        m_widget->deleteSelectedAssets();
    } else if (identifier == "asset_import_finished") {
        // TODO: This custom notification should be removed once QDS-15163 is fixed and
        //       exportedTypeNamesChanged notification is reliable
        m_3dImportsSyncTimer.start();
    }
}

void AssetsLibraryView::modelAttached(Model *model)
{
    AbstractView::modelAttached(model);

    m_widget->clearSearchFilter();

    setResourcePath(DocumentManager::currentResourcePath().toFSPathString());

    m_3dImportsSyncTimer.start();
}

void AssetsLibraryView::modelAboutToBeDetached(Model *model)
{
    AbstractView::modelAboutToBeDetached(model);

    m_3dImportsSyncTimer.stop();
}

void AssetsLibraryView::exportedTypeNamesChanged(const ExportedTypeNames &added,
                                                 const ExportedTypeNames &removed)
{
    if (Utils3D::hasImported3dType(this, added, removed))
        m_3dImportsSyncTimer.start();
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

void AssetsLibraryView::sync3dImports()
{
    if (!model())
        return;

    // Sync generated 3d imports to .q3d files in project content
    const GeneratedComponentUtils &compUtils
        = QmlDesignerPlugin::instance()->documentManager().generatedComponentUtils();
    auto projPath = Utils::FilePath::fromString(externalDependencies().currentProjectDirPath());

    const QList<Utils::FilePath> qmlFiles = compUtils.imported3dComponents();

    Utils::FilePath resPath = DocumentManager::currentResourcePath();
    QHash<QString, Utils::FilePath> files = collectFiles(resPath, "q3d");

    const QString pathTemplate("/%1.q3d");

    for (const Utils::FilePath &qmlFile : qmlFiles) {
        const QString fileStr = qmlFile.baseName();
        if (files.contains(fileStr)) {
            files.remove(fileStr);
        } else {
            Utils::FilePath targetPath = ModelNodeOperations::getImported3dDefaultDirectory();
            if (!targetPath.isAbsolutePath() || !targetPath.exists())
                targetPath = resPath;
            Utils::FilePath newFile = targetPath.pathAppended(pathTemplate.arg(fileStr));
            QByteArray data;
            data.append(qmlFile.relativePathFromDir(projPath).toFSPathString().toLatin1());
            newFile.writeFileContents(data);
        }
    }

    // Remove .q3d files that do not match any imported 3d
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
