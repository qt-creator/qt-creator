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

#include "itemlibraryview.h"
#include "itemlibrarywidget.h"
#include "itemlibraryassetimportdialog.h"
#include "metainfo.h"
#include <asynchronousimagecache.h>
#include <bindingproperty.h>
#include <coreplugin/icore.h>
#include <imagecache/imagecachecollector.h>
#include <imagecache/imagecacheconnectionmanager.h>
#include <imagecache/imagecachefontcollector.h>
#include <imagecache/imagecachegenerator.h>
#include <imagecache/imagecachestorage.h>
#include <imagecache/timestampprovider.h>
#include <import.h>
#include <nodelistproperty.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/project.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>
#include <rewriterview.h>
#include <sqlitedatabase.h>
#include <synchronousimagecache.h>
#include <utils/algorithm.h>
#include <qmldesignerplugin.h>
#include <qmlitemnode.h>
#include <qmldesignerconstants.h>

namespace QmlDesigner {

namespace {
ProjectExplorer::Target *activeTarget(ProjectExplorer::Project *project)
{
    if (project)
        return project->activeTarget();

    return {};
}
} // namespace

class ImageCacheData
{
public:
    Sqlite::Database database{
        Utils::PathString{Core::ICore::cacheResourcePath() + "/imagecache-v2.db"}};
    ImageCacheStorage<Sqlite::Database> storage{database};
    ImageCacheConnectionManager connectionManager;
    ImageCacheCollector collector{connectionManager};
    ImageCacheFontCollector fontCollector;
    ImageCacheGenerator generator{collector, storage};
    ImageCacheGenerator fontGenerator{fontCollector, storage};
    TimeStampProvider timeStampProvider;
    AsynchronousImageCache cache{storage, generator, timeStampProvider};
    AsynchronousImageCache asynchronousFontImageCache{storage, fontGenerator, timeStampProvider};
    SynchronousImageCache synchronousFontImageCache{storage, timeStampProvider, fontCollector};
};

ItemLibraryView::ItemLibraryView(QObject* parent)
    : AbstractView(parent)

{
    m_imageCacheData = std::make_unique<ImageCacheData>();

    auto setTargetInImageCache =
        [imageCacheData = m_imageCacheData.get()](ProjectExplorer::Target *target) {
            if (target == imageCacheData->collector.target())
                return;

            if (target)
                imageCacheData->cache.clean();

            imageCacheData->collector.setTarget(target);
        };

    if (auto project = ProjectExplorer::SessionManager::startupProject(); project) {
        m_imageCacheData->collector.setTarget(project->activeTarget());
        connect(project, &ProjectExplorer::Project::activeTargetChanged, this, setTargetInImageCache);
    }

    connect(ProjectExplorer::SessionManager::instance(),
            &ProjectExplorer::SessionManager::startupProjectChanged,
            this,
            [=](ProjectExplorer::Project *project) { setTargetInImageCache(activeTarget(project)); });
}

ItemLibraryView::~ItemLibraryView()
{
}

bool ItemLibraryView::hasWidget() const
{
    return true;
}

WidgetInfo ItemLibraryView::widgetInfo()
{
    if (m_widget.isNull()) {
        m_widget = new ItemLibraryWidget{m_imageCacheData->cache,
                                         m_imageCacheData->asynchronousFontImageCache,
                                         m_imageCacheData->synchronousFontImageCache};
    }

    return createWidgetInfo(m_widget.data(),
                            new WidgetInfo::ToolBarWidgetDefaultFactory<ItemLibraryWidget>(m_widget.data()),
                            QStringLiteral("Library"),
                            WidgetInfo::LeftPane,
                            0,
                            tr("Library"));
}

void ItemLibraryView::modelAttached(Model *model)
{
    AbstractView::modelAttached(model);

    m_widget->clearSearchFilter();
    m_widget->setModel(model);
    updateImports();
    m_hasErrors = !rewriterView()->errors().isEmpty();
    m_widget->setFlowMode(QmlItemNode(rootModelNode()).isFlowView());
    setResourcePath(DocumentManager::currentResourcePath().toFileInfo().absoluteFilePath());
}

void ItemLibraryView::modelAboutToBeDetached(Model *model)
{
    AbstractView::modelAboutToBeDetached(model);

    m_widget->setModel(nullptr);
}

void ItemLibraryView::importsChanged(const QList<Import> &addedImports, const QList<Import> &removedImports)
{
    updateImports();

    // TODO: generalize the logic below to allow adding/removing any Qml component when its import is added/removed
    bool simulinkImportAdded = std::any_of(addedImports.cbegin(), addedImports.cend(), [](const Import &import) {
        return import.url() == "SimulinkConnector";
    });
    if (simulinkImportAdded) {
        // add SLConnector component when SimulinkConnector import is added
        ModelNode node = createModelNode("SLConnector", 1, 0);
        node.bindingProperty("root").setExpression(rootModelNode().validId());
        rootModelNode().defaultNodeListProperty().reparentHere(node);
    } else {
        bool simulinkImportRemoved = std::any_of(removedImports.cbegin(), removedImports.cend(), [](const Import &import) {
            return import.url() == "SimulinkConnector";
        });

        if (simulinkImportRemoved) {
            // remove SLConnector component when SimulinkConnector import is removed
            const QList<ModelNode> slConnectors = Utils::filtered(rootModelNode().directSubModelNodes(),
                                                                  [](const ModelNode &node) {
                return node.type() == "SLConnector" || node.type() == "SimulinkConnector.SLConnector";
            });

            for (ModelNode node : slConnectors)
                node.destroy();

            resetPuppet();
        }
    }
}

void ItemLibraryView::possibleImportsChanged(const QList<Import> &possibleImports)
{
    m_widget->updatePossibleImports(possibleImports);
}

void ItemLibraryView::usedImportsChanged(const QList<Import> &usedImports)
{
    m_widget->updateUsedImports(usedImports);
}

void ItemLibraryView::setResourcePath(const QString &resourcePath)
{
    if (m_widget.isNull())
        m_widget = new ItemLibraryWidget{m_imageCacheData->cache,
                                         m_imageCacheData->asynchronousFontImageCache,
                                         m_imageCacheData->synchronousFontImageCache};

    m_widget->setResourcePath(resourcePath);
}

AsynchronousImageCache &ItemLibraryView::imageCache()
{
    return m_imageCacheData->cache;
}

void ItemLibraryView::documentMessagesChanged(const QList<DocumentMessage> &errors, const QList<DocumentMessage> &)
{
    if (m_hasErrors && errors.isEmpty())
        updateImports();

     m_hasErrors = !errors.isEmpty();
}

void ItemLibraryView::updateImports()
{
    m_widget->delayedUpdateModel();
}

void ItemLibraryView::updateImport3DSupport(const QVariantMap &supportMap)
{
    QVariantMap extMap = qvariant_cast<QVariantMap>(supportMap.value("extensions"));
    if (m_importableExtensions3DMap != extMap) {
        DesignerActionManager *actionManager =
                 &QmlDesignerPlugin::instance()->viewManager().designerActionManager();

        // All things importable by QSSGAssetImportManager are considered to be in the same category
        // so we don't get multiple separate import dialogs when different file types are imported.
        const QString category = tr("3D Assets");

        if (!m_importableExtensions3DMap.isEmpty())
            actionManager->unregisterAddResourceHandlers(category);

        m_importableExtensions3DMap = extMap;

        auto handle3DModel = [this](const QStringList &fileNames, const QString &defaultDir) -> bool {
            auto importDlg = new ItemLibraryAssetImportDialog(fileNames, defaultDir,
                                                              m_importableExtensions3DMap,
                                                              m_importOptions3DMap, {}, {},
                                                              Core::ICore::mainWindow());
            importDlg->show();
            return true;
        };

        auto add3DHandler = [&](const QString &category, const QString &ext) {
            const QString filter = QStringLiteral("*.%1").arg(ext);
            actionManager->registerAddResourceHandler(
                        AddResourceHandler(category, filter, handle3DModel, 10));
        };

        const auto groups = extMap.keys();
        for (const auto &group : groups) {
            const QStringList exts = extMap[group].toStringList();
            for (const auto &ext : exts)
                add3DHandler(category, ext);
        }
    }

    m_importOptions3DMap = qvariant_cast<QVariantMap>(supportMap.value("options"));
}

void ItemLibraryView::customNotification(const AbstractView *view, const QString &identifier,
                                         const QList<ModelNode> &nodeList, const QList<QVariant> &data)
{
    if (identifier == "UpdateImported3DAsset" && nodeList.size() > 0) {
        ItemLibraryAssetImportDialog::updateImport(nodeList[0], m_importableExtensions3DMap,
                                                   m_importOptions3DMap);
    } else {
        AbstractView::customNotification(view, identifier, nodeList, data);
    }
}

} // namespace QmlDesigner
