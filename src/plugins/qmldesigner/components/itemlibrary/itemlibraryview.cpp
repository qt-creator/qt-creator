// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "itemlibraryview.h"
#include "itemlibraryassetimportdialog.h"
#include "itemlibrarywidget.h"
#include "metainfo.h"
#include <asynchronousimagecache.h>
#include <bindingproperty.h>
#include <componentcore_constants.h>
#include <coreplugin/icore.h>
#include <designeractionmanager.h>
#include <import.h>
#include <nodelistproperty.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/target.h>
#include <rewriterview.h>
#include <sqlitedatabase.h>
#include <utils/algorithm.h>
#include <qmldesignerconstants.h>
#include <qmldesignerplugin.h>
#include <qmlitemnode.h>

namespace QmlDesigner {

ItemLibraryView::ItemLibraryView(AsynchronousImageCache &imageCache,
                                 ExternalDependenciesInterface &externalDependencies)
    : AbstractView(externalDependencies)
    , m_imageCache(imageCache)
{}

ItemLibraryView::~ItemLibraryView()
{
}

bool ItemLibraryView::hasWidget() const
{
    return true;
}

WidgetInfo ItemLibraryView::widgetInfo()
{
    if (m_widget.isNull())
        m_widget = new ItemLibraryWidget{m_imageCache};

    return createWidgetInfo(m_widget.data(),
                            "Components",
                            WidgetInfo::LeftPane,
                            0,
                            tr("Components"),
                            tr("Components view"));
}

void ItemLibraryView::modelAttached(Model *model)
{
    AbstractView::modelAttached(model);

    m_widget->clearSearchFilter();
    m_widget->switchToComponentsView();
    m_widget->setModel(model);
    updateImports();
    if (model)
        m_widget->updatePossibleImports(set_difference(model->possibleImports(), model->imports()));
    m_hasErrors = !rewriterView()->errors().isEmpty();
    m_widget->setFlowMode(QmlItemNode(rootModelNode()).isFlowView());
}

void ItemLibraryView::modelAboutToBeDetached(Model *model)
{
    AbstractView::modelAboutToBeDetached(model);

    m_widget->setModel(nullptr);
}

void ItemLibraryView::importsChanged(const Imports &addedImports, const Imports &removedImports)
{
    DesignDocument *document = QmlDesignerPlugin::instance()->currentDesignDocument();
    for (const auto &import : addedImports)
        document->addSubcomponentManagerImport(import);

    updateImports();
    m_widget->updatePossibleImports(model()->possibleImports());

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

void ItemLibraryView::possibleImportsChanged(const Imports &possibleImports)
{
    DesignDocument *document = QmlDesignerPlugin::instance()->currentDesignDocument();
    for (const auto &import : possibleImports)
        document->addSubcomponentManagerImport(import);

    m_widget->updatePossibleImports(possibleImports);
}

void ItemLibraryView::usedImportsChanged(const Imports &usedImports)
{
    m_widget->updateUsedImports(usedImports);
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

        if (!m_importableExtensions3DMap.isEmpty())
            actionManager->unregisterAddResourceHandlers(ComponentCoreConstants::add3DAssetsDisplayString);

        m_importableExtensions3DMap = extMap;

        AddResourceOperation import3DModelOperation = [this](const QStringList &fileNames,
                                                             const QString &defaultDir,
                                                             bool showDialog) -> AddFilesResult {
            Q_UNUSED(showDialog)

            auto importDlg = new ItemLibraryAssetImportDialog(fileNames, defaultDir,
                                                              m_importableExtensions3DMap,
                                                              m_importOptions3DMap, {}, {},
                                                              Core::ICore::dialogParent());
            int result = importDlg->exec();

            return result == QDialog::Accepted ? AddFilesResult::succeeded() : AddFilesResult::cancelled();
        };

        auto add3DHandler = [&](const QString &group, const QString &ext) {
            const QString filter = QStringLiteral("*.%1").arg(ext);
            actionManager->registerAddResourceHandler(
                        AddResourceHandler(group, filter,
                                           import3DModelOperation, 10));
        };

        const QHash<QString, QString> groupNames {
            {"3D Scene",                  ComponentCoreConstants::add3DAssetsDisplayString},
            {"Qt 3D Studio Presentation", ComponentCoreConstants::addQt3DSPresentationsDisplayString}
        };

        const auto groups = extMap.keys();
        for (const auto &group : groups) {
            const QStringList exts = extMap[group].toStringList();
            const QString grp = groupNames.contains(group) ? groupNames.value(group) : group;
            for (const auto &ext : exts)
                add3DHandler(grp, ext);
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
