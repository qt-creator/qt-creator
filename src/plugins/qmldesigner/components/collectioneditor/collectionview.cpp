// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "collectionview.h"

#include "collectiondatatypemodel.h"
#include "collectiondetailsmodel.h"
#include "collectioneditorconstants.h"
#include "collectioneditorutils.h"
#include "collectionlistmodel.h"
#include "collectionwidget.h"
#include "datastoremodelnode.h"
#include "designmodecontext.h"
#include "nodeabstractproperty.h"
#include "nodemetainfo.h"
#include "nodeproperty.h"
#include "qmldesignerplugin.h"
#include "variantproperty.h"

#include <designercore/generatedcomponentutils.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectmanager.h>
#include <qmljs/qmljsmodelmanagerinterface.h>

#include <coreplugin/icore.h>
#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QTimer>

namespace {

bool isStudioCollectionModel(const QmlDesigner::ModelNode &node)
{
    return node.metaInfo().isQtQuickStudioUtilsJsonListModel();
}

inline bool isProjectImport(const QmlDesigner::Import &import)
{
    ProjectExplorer::Project *currentProject = ProjectExplorer::ProjectManager::startupProject();
    return currentProject && import.toString() == currentProject->displayName();
}

inline void setVariantPropertyValue(const QmlDesigner::ModelNode &node,
                                    const QmlDesigner::PropertyName &propertyName,
                                    const QVariant &value)
{
    QmlDesigner::VariantProperty property = node.variantProperty(propertyName);
    property.setValue(value);
}

inline void setNodePropertyValue(const QmlDesigner::ModelNode &node,
                                 const QmlDesigner::PropertyName &propertyName,
                                 const QmlDesigner::ModelNode &value)
{
    QmlDesigner::NodeProperty nodeProperty = node.nodeProperty(propertyName);
    // Remove the old model node if is available
    if (nodeProperty.modelNode())
        nodeProperty.modelNode().destroy();

    nodeProperty.setModelNode(value);
}

inline void setBindingPropertyExpression(const QmlDesigner::ModelNode &node,
                                         const QmlDesigner::PropertyName &propertyName,
                                         const QString &expression)
{
    QmlDesigner::BindingProperty property = node.bindingProperty(propertyName);
    property.setExpression(expression);
}

} // namespace

namespace QmlDesigner {

CollectionView::CollectionView(ExternalDependenciesInterface &externalDependencies)
    : AbstractView(externalDependencies)
    , m_dataStore(std::make_unique<DataStoreModelNode>())

{}

CollectionView::~CollectionView() = default;

bool CollectionView::hasWidget() const
{
    return true;
}

QmlDesigner::WidgetInfo CollectionView::widgetInfo()
{
    if (!m_widget) {
        m_widget = Utils::makeUniqueObjectPtr<CollectionWidget>(this);
        m_widget->setMinimumSize(m_widget->minimumSizeHint());
        connect(ProjectExplorer::ProjectManager::instance(),
                &ProjectExplorer::ProjectManager::startupProjectChanged, m_widget.get(), [&] {
            resetDataStoreNode();
            m_widget->collectionDetailsModel()->removeAllCollections();
        });

        auto collectionEditorContext = new Internal::CollectionEditorContext(m_widget.get());
        Core::ICore::addContextObject(collectionEditorContext);
        CollectionListModel *listModel = m_widget->listModel().data();

        connect(listModel,
                &CollectionListModel::selectedCollectionNameChanged,
                this,
                [this](const QString &collection) {
                    m_widget->collectionDetailsModel()->loadCollection(dataStoreNode(), collection);
                });

        connect(listModel, &CollectionListModel::isEmptyChanged, this, [this](bool isEmpty) {
            if (isEmpty)
                m_widget->collectionDetailsModel()->loadCollection({}, {});
        });

        connect(listModel, &CollectionListModel::modelReset, this, [this] {
            CollectionListModel *listModel = m_widget->listModel().data();
            if (listModel->sourceNode() == dataStoreNode())
                m_dataStore->setCollectionNames(listModel->collections());
        });

        connect(listModel,
                &CollectionListModel::collectionAdded,
                this,
                [this](const QString &collectionName) { m_dataStore->addCollection(collectionName); });

        connect(listModel,
                &CollectionListModel::collectionNameChanged,
                this,
                [this](const QString &oldName, const QString &newName) {
                    m_dataStore->renameCollection(oldName, newName);
                    m_widget->collectionDetailsModel()->renameCollection(dataStoreNode(),
                                                                         oldName,
                                                                         newName);
                });

        connect(listModel,
                &CollectionListModel::collectionsRemoved,
                this,
                [this](const QStringList &collectionNames) {
                    m_dataStore->removeCollections(collectionNames);
                    for (const QString &collectionName : collectionNames) {
                        m_widget->collectionDetailsModel()->removeCollection(dataStoreNode(),
                                                                             collectionName);
                    }
                });
    }

    return createWidgetInfo(m_widget.get(),
                            "CollectionEditor",
                            WidgetInfo::LeftPane,
                            0,
                            tr("Model Editor [beta]"),
                            tr("Model Editor view"));
}

void CollectionView::modelAttached(Model *model)
{
    AbstractView::modelAttached(model);
    m_widget->setProjectImportExists(Utils::anyOf(model->imports(), isProjectImport));
    resetDataStoreNode();
}

void CollectionView::modelAboutToBeDetached([[maybe_unused]] Model *model)
{
    unloadDataStore();
    m_widget->setProjectImportExists(false);
}

void CollectionView::selectedNodesChanged(const QList<ModelNode> &selectedNodeList,
                                          [[maybe_unused]] const QList<ModelNode> &lastSelectedNodeList)
{
    if (!m_widget)
        return;

    QList<ModelNode> selectedCollectionNodes = Utils::filtered(selectedNodeList,
                                                               &isStudioCollectionModel);

    bool singleNonCollectionNodeSelected = selectedNodeList.size() == 1
                                           && selectedCollectionNodes.isEmpty();

    bool singleSelectedHasModelProperty = false;
    if (singleNonCollectionNodeSelected) {
        const ModelNode selectedNode = selectedNodeList.first();
        singleSelectedHasModelProperty = CollectionEditorUtils::canAcceptCollectionAsModel(
            selectedNode);
    }

    m_widget->setTargetNodeSelected(singleSelectedHasModelProperty);
}

void CollectionView::importsChanged(const Imports &addedImports, const Imports &removedImports)
{
    if (Utils::anyOf(addedImports, isProjectImport)) {
        m_widget->setProjectImportExists(true);
        resetDataStoreNode();
    } else if (Utils::anyOf(removedImports, isProjectImport)) {
        m_widget->setProjectImportExists(false);
        unloadDataStore();
    }
}

void CollectionView::customNotification(const AbstractView *,
                                        const QString &identifier,
                                        const QList<ModelNode> &nodeList,
                                        const QList<QVariant> &data)
{
    if (!m_widget)
        return;

    if (identifier == QLatin1String("item_library_created_by_drop") && !nodeList.isEmpty())
        onItemLibraryNodeCreated(nodeList.first());
    else if (identifier == QLatin1String("open_collection_by_id") && !data.isEmpty())
        m_widget->openCollection(collectionNameFromDataStoreChildren(data.first().toByteArray()));
    else if (identifier == "delete_selected_collection")
        m_widget->deleteSelectedCollection();
}

void CollectionView::addResource(const QUrl &url, const QString &name)
{
    executeInTransaction(Q_FUNC_INFO, [this, &url, &name]() {
        ensureStudioModelImport();
        QString sourceAddress;
        if (url.isLocalFile()) {
            Utils::FilePath fp = QmlDesignerPlugin::instance()->currentDesignDocument()->fileName().parentDir();
            sourceAddress = Utils::FilePath::calcRelativePath(url.toLocalFile(),
                                                              fp.absoluteFilePath().toString());
        } else {
            sourceAddress = url.toString();
        }
#ifdef QDS_USE_PROJECTSTORAGE
        ModelNode resourceNode = createModelNode("JsonListModel");
#else
        const NodeMetaInfo resourceMetaInfo = jsonCollectionMetaInfo();
        ModelNode resourceNode = createModelNode(resourceMetaInfo.typeName(),
                                                 resourceMetaInfo.majorVersion(),
                                                 resourceMetaInfo.minorVersion());
#endif
        VariantProperty sourceProperty = resourceNode.variantProperty(
            CollectionEditorConstants::SOURCEFILE_PROPERTY);
        VariantProperty nameProperty = resourceNode.variantProperty("objectName");
        sourceProperty.setValue(sourceAddress);
        nameProperty.setValue(name);
        resourceNode.setIdWithoutRefactoring(model()->generateIdFromName(name, "model"));
        rootModelNode().defaultNodeAbstractProperty().reparentHere(resourceNode);
    });
}

void CollectionView::addProjectImport()
{
    if (!m_widget)
        return;

    ProjectExplorer::Project *currentProject = ProjectExplorer::ProjectManager::startupProject();
    if (!currentProject)
        return;

    executeInTransaction(__FUNCTION__, [&] {
        Import import = Import::createLibraryImport(currentProject->displayName());
        if (!model()->hasImport(import, true, true))
            model()->changeImports({import}, {});
    });
}

void CollectionView::assignCollectionToNode(const QString &collectionName, const ModelNode &node)
{
    if (!m_widget)
        return;

    executeInTransaction("CollectionView::assignCollectionToNode", [&]() {
        m_dataStore->assignCollectionToNode(
            this,
            node,
            collectionName,
            [&](const QString &collectionName, const QString &columnName) -> bool {
                const CollectionReference reference{dataStoreNode(), collectionName};
                return m_widget->collectionDetailsModel()->collectionHasColumn(reference, columnName);
            },
            [&](const QString &collectionName) -> QString {
                const CollectionReference reference{dataStoreNode(), collectionName};
                return m_widget->collectionDetailsModel()->getFirstColumnName(reference);
            });

        // Create and assign a delegate to the list view item
        if (node.metaInfo().isQtQuickListView()) {
            ::setNodePropertyValue(node, "delegate", createListViewDelegate(collectionName));
        } else if (node.metaInfo().isQtQuickGridView()) {
            QSize delegateSize;
            ::setNodePropertyValue(node,
                                   "delegate",
                                   createGridViewDelegate(collectionName, delegateSize));
            ::setVariantPropertyValue(node, "cellWidth", delegateSize.width());
            ::setVariantPropertyValue(node, "cellHeight", delegateSize.height());
        }
    });
}

void CollectionView::assignCollectionToSelectedNode(const QString &collectionName)
{
    QTC_ASSERT(dataStoreNode() && hasSingleSelectedModelNode(), return);
    assignCollectionToNode(collectionName, singleSelectedModelNode());
}

void CollectionView::addNewCollection(const QString &collectionName, const QJsonObject &localCollection)
{
    if (!m_widget)
        return;

    addTask(QSharedPointer<CollectionTask>(
        new AddCollectionTask(this, m_widget->listModel(), localCollection, collectionName)));
}

void CollectionView::openCollection(const QString &collectionName)
{
    if (!m_widget)
        return;

    m_widget->openCollection(collectionName);
}

void CollectionView::registerDeclarativeType()
{
    CollectionDetails::registerDeclarativeType();
    CollectionDataTypeModel::registerDeclarativeType();
}

void CollectionView::resetDataStoreNode()
{
    if (!m_widget)
        return;

    auto compUtils = QmlDesignerPlugin::instance()->documentManager().generatedComponentUtils();
    m_dataStore->reloadModel(compUtils.projectModulePath());

    ModelNode dataStore = dataStoreNode();
    m_widget->setDataStoreExists(dataStore.isValid());
    if (!dataStore || m_widget->listModel()->sourceNode() == dataStore)
        return;

    bool dataStoreSingletonFound = m_dataStoreTypeFound;
    if (!dataStoreSingletonFound && rewriterView() && rewriterView()->isAttached()) {
        const QList<QmlTypeData> types = rewriterView()->getQMLTypes();
        for (const QmlTypeData &cppTypeData : types) {
            if (cppTypeData.isSingleton && cppTypeData.typeName == "DataStore") {
                dataStoreSingletonFound = true;
                break;
            }
        }
        if (!dataStoreSingletonFound && !m_rewriterAmended) {
            rewriterView()->forceAmend();
            m_rewriterAmended = true;
        }
    }

    if (dataStoreSingletonFound) {
        m_widget->listModel()->setDataStoreNode(dataStore);
        m_widget->collectionDetailsModel()->setJsonFilePath(m_dataStore->jsonFilePath());
        m_dataStoreTypeFound = true;
        resetPuppet();

        while (!m_delayedTasks.isEmpty())
            m_delayedTasks.takeFirst()->process();
    } else if (++m_reloadCounter < 50) {
        QTimer::singleShot(200, this, &CollectionView::resetDataStoreNode);
    } else {
        QTC_ASSERT(false, m_delayedTasks.clear());
    }
}

ModelNode CollectionView::dataStoreNode() const
{
    return m_dataStore->modelNode();
}

void CollectionView::ensureDataStoreExists()
{
    bool filesJustCreated = false;
    bool filesExist = createDataStore(filesJustCreated);
    if (filesExist && filesJustCreated) {
        // Force code model reset to notice changes to existing module
        if (auto modelManager = QmlJS::ModelManagerInterface::instance())
            modelManager->resetCodeModel();
        resetDataStoreNode();
    }
}

QString CollectionView::collectionNameFromDataStoreChildren(const PropertyName &childPropertyName) const
{
    return dataStoreNode()
        .nodeProperty(childPropertyName)
        .modelNode()
        .property(CollectionEditorConstants::JSONCHILDMODELNAME_PROPERTY)
        .toVariantProperty()
        .value()
        .toString();
}

NodeMetaInfo CollectionView::jsonCollectionMetaInfo() const
{
    return model()->metaInfo(CollectionEditorConstants::JSONCOLLECTIONMODEL_TYPENAME);
}

void CollectionView::unloadDataStore()
{
    m_reloadCounter = 0;
    m_rewriterAmended = false;
    m_dataStoreTypeFound = false;
    QTC_ASSERT(m_delayedTasks.isEmpty(), m_delayedTasks.clear());
    if (m_widget) {
        m_widget->setDataStoreExists(dataStoreNode().isValid());
        m_widget->listModel()->setDataStoreNode();
    }
}

bool CollectionView::createDataStore(bool &justCreated) const
{
    using Utils::FilePath;
    using Utils::FileReader;
    using Utils::FileSaver;
    using namespace Qt::StringLiterals;

    auto compUtils = QmlDesignerPlugin::instance()->documentManager().generatedComponentUtils();
    FilePath projectModulePath = compUtils.projectModulePath(true);

    FilePath qmlTargetPath = projectModulePath.resolvePath(
        CollectionEditorConstants::DEFAULT_DATASTORE_QML_FILENAME.toString());
    justCreated = false;

    auto extractDependency = [&justCreated](const FilePath &filePath) -> bool {
        if (filePath.exists())
            return true;

        const QString templateFileName = filePath.fileName() + u".tpl";
        const FilePath templatePath = CollectionEditorUtils::findFile(Core::ICore::resourcePath(),
                                                                      templateFileName);
        if (!templatePath.exists()) {
            qWarning() << Q_FUNC_INFO << __LINE__ << templateFileName << "does not exist";
            return false;
        }

        if (!filePath.parentDir().ensureWritableDir()) {
            qWarning() << Q_FUNC_INFO << __LINE__ << "Cannot create directory"
                       << filePath.parentDir();
            return false;
        }

        if (templatePath.copyFile(filePath)) {
            justCreated = true;
            return true;
        }

        qWarning() << Q_FUNC_INFO << __LINE__ << "Cannot copy" << templateFileName << "to" << filePath;
        return false;
    };

    if (!extractDependency(projectModulePath.resolvePath(
            CollectionEditorConstants::DEFAULT_MODELS_JSON_FILENAME.toString()))) {
        return false;
    }

    if (!extractDependency(projectModulePath.resolvePath(
            CollectionEditorConstants::DEFAULT_DATA_JSON_FILENAME.toString()))) {
        return false;
    }

    if (!extractDependency(projectModulePath.resolvePath(
            CollectionEditorConstants::DEFAULT_JSONDATA_QML_FILENAME.toString()))) {
        return false;
    }

    if (!qmlTargetPath.exists()) {
        if (qmlTargetPath.ensureExistingFile()) {
            justCreated = true;
        } else {
            qWarning() << Q_FUNC_INFO << __LINE__ << "Can't create DataStore Qml File";
            return false;
        }
    }

    FilePath qmlDirPath = projectModulePath.resolvePath("qmldir"_L1);
    qmlDirPath.ensureExistingFile();

    FileReader qmlDirReader;
    if (!qmlDirReader.fetch(qmlDirPath)) {
        qWarning() << Q_FUNC_INFO << __LINE__ << "Can't read the content of the qmldir";
        return false;
    }

    QByteArray qmlDirContent = qmlDirReader.data();
    const QList<QByteArray> qmlDirLines = qmlDirContent.split('\n');
    for (const QByteArray &line : qmlDirLines) {
        if (line.startsWith("singleton DataStore "))
            return true;
    }

    if (!qmlDirContent.isEmpty() && qmlDirContent.back() != '\n')
        qmlDirContent.append("\n");
    qmlDirContent.append("singleton DataStore 1.0 DataStore.qml\n");

    FileSaver qmlDirSaver(qmlDirPath);
    qmlDirSaver.write(qmlDirContent);

    if (qmlDirSaver.finalize()) {
        justCreated = true;
        return true;
    }

    qWarning() << Q_FUNC_INFO << __LINE__ << "Can't write to the qmldir file";
    return false;
}

void CollectionView::ensureStudioModelImport()
{
    executeInTransaction(__FUNCTION__, [&] {
        Import import = Import::createLibraryImport(CollectionEditorConstants::COLLECTIONMODEL_IMPORT);
        try {
            if (!model()->hasImport(import, true, true))
                model()->changeImports({import}, {});
        } catch (const Exception &) {
            QTC_ASSERT(false, return);
        }
    });
}

void CollectionView::onItemLibraryNodeCreated(const ModelNode &node)
{
    if (!m_widget)
        return;

    if (node.metaInfo().isListOrGridView()) {
        addTask(QSharedPointer<CollectionTask>(
            new DropListViewTask(this, m_widget->listModel(), node)));
    }
}

void CollectionView::addTask(QSharedPointer<CollectionTask> task)
{
    ensureDataStoreExists();
    if (m_dataStoreTypeFound)
        task->process();
    else if (dataStoreNode())
        m_delayedTasks << task;
}

ModelNode CollectionView::createListViewDelegate(const QString &collectionName)
{
    using DataType = CollectionDetails::DataType;
    using namespace Qt::StringLiterals;

    CollectionDetails collection = m_widget->collectionDetailsModel()->upToDateConstCollection(
        {dataStoreNode(), collectionName});

    ModelNode rowItem(createModelNode("QtQuick.Row"));
    ::setVariantPropertyValue(rowItem, "spacing", 5);

    const int columnsCount = collection.columns();
    for (int column = 0; column < columnsCount; ++column) {
        const DataType dataType = collection.typeAt(column);
        const QString columnName = collection.propertyAt(column);
        ModelNode cellItem;
        if (dataType == DataType::Color) {
            cellItem = createModelNode("QtQuick.Rectangle");
            ::setBindingPropertyExpression(cellItem, "color", columnName);
            ::setVariantPropertyValue(cellItem, "height", 20);
        } else {
            cellItem = createModelNode("QtQuick.Text");
            ::setBindingPropertyExpression(cellItem, "text", columnName);
            ::setVariantPropertyValue(cellItem, "clip", true);
            ::setBindingPropertyExpression(cellItem, "elide", "Text.ElideRight"_L1);
            ::setVariantPropertyValue(cellItem, "leftPadding", 1);
            ::setVariantPropertyValue(cellItem, "rightPadding", 1);
        }
        ::setVariantPropertyValue(cellItem, "width", 100);
        rowItem.defaultNodeAbstractProperty().reparentHere(cellItem);
    }
    return rowItem;
}

ModelNode CollectionView::createGridViewDelegate(const QString &collectionName, QSize &delegateSize)
{
    using DataType = CollectionDetails::DataType;
    using namespace Qt::StringLiterals;

    CollectionDetails collection = m_widget->collectionDetailsModel()->upToDateConstCollection(
        {dataStoreNode(), collectionName});

    constexpr int spacing = 5;
    int delegateWidth = 70;
    int delegateHeight = 0;

    ModelNode rectItem(createModelNode("QtQuick.Rectangle"));
    ::setVariantPropertyValue(rectItem, "clip", true);
    ::setVariantPropertyValue(rectItem, "border.color", QColor(Qt::blue));
    ::setVariantPropertyValue(rectItem, "border.width", 1);

    ModelNode colItem(createModelNode("QtQuick.Column"));
    ::setVariantPropertyValue(colItem, "spacing", spacing);
    ::setVariantPropertyValue(colItem, "topPadding", spacing);
    ::setBindingPropertyExpression(colItem, "anchors.fill", "parent"_L1);

    const int columnsCount = collection.columns();
    for (int column = 0; column < columnsCount; ++column) {
        const DataType dataType = collection.typeAt(column);
        const QString columnName = collection.propertyAt(column);
        ModelNode cellItem;
        if (dataType == DataType::Color) {
            cellItem = createModelNode("QtQuick.Rectangle");
            ::setBindingPropertyExpression(cellItem, "color", columnName);
            ::setVariantPropertyValue(cellItem, "width", 40);
            ::setVariantPropertyValue(cellItem, "height", 40);
            delegateHeight += 40;
        } else {
            cellItem = createModelNode("QtQuick.Text");
            ::setBindingPropertyExpression(cellItem, "width", "parent.width"_L1);
            ::setVariantPropertyValue(cellItem, "height", 20);
            ::setBindingPropertyExpression(cellItem, "text", columnName);
            ::setVariantPropertyValue(cellItem, "clip", true);
            ::setBindingPropertyExpression(cellItem, "elide", "Text.ElideRight"_L1);
            ::setVariantPropertyValue(cellItem, "leftPadding", 1);
            ::setVariantPropertyValue(cellItem, "rightPadding", 1);
            ::setBindingPropertyExpression(cellItem, "horizontalAlignment", "Text.AlignHCenter"_L1);
            delegateHeight += 20;
        }
        delegateHeight += spacing;
        ::setBindingPropertyExpression(cellItem,
                                       "anchors.horizontalCenter",
                                       "parent.horizontalCenter"_L1);
        colItem.defaultNodeAbstractProperty().reparentHere(cellItem);
    }

    rectItem.defaultNodeAbstractProperty().reparentHere(colItem);
    ::setVariantPropertyValue(rectItem, "width", delegateWidth);
    ::setVariantPropertyValue(rectItem, "height", delegateHeight);
    delegateSize.setWidth(delegateWidth);
    delegateSize.setHeight(delegateHeight);

    return rectItem;
}

CollectionTask::CollectionTask(CollectionView *view, CollectionListModel *listModel)
    : m_collectionView(view)
    , m_listModel(listModel)
{}

DropListViewTask::DropListViewTask(CollectionView *view,
                                   CollectionListModel *listModel,
                                   const ModelNode &node)
    : CollectionTask(view, listModel)
    , m_node(node)
{}

void DropListViewTask::process()
{
    AbstractView *view = m_node.view();
    if (!m_node || !m_collectionView || !m_listModel || !view)
        return;

    const QString newCollectionName = m_listModel->getUniqueCollectionName("ListModel");
    m_listModel->addCollection(newCollectionName, CollectionEditorUtils::defaultColorCollection());
    m_collectionView->openCollection(newCollectionName);
    m_collectionView->assignCollectionToNode(newCollectionName, m_node);
}

AddCollectionTask::AddCollectionTask(CollectionView *view,
                                     CollectionListModel *listModel,
                                     const QJsonObject &localJsonObject,
                                     const QString &collectionName)
    : CollectionTask(view, listModel)
    , m_localJsonObject(localJsonObject)
    , m_name(collectionName)
{}

void AddCollectionTask::process()
{
    if (!m_listModel)
        return;

    const QString newCollectionName = m_listModel->collectionExists(m_name)
                                          ? m_listModel->getUniqueCollectionName(m_name)
                                          : m_name;

    m_listModel->addCollection(newCollectionName, m_localJsonObject);
}

} // namespace QmlDesigner
