// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "datastoremodelnode.h"

#include "collectioneditorconstants.h"
#include "model/qmltextgenerator.h"

#include <model.h>
#include <nodemetainfo.h>
#include <nodeproperty.h>
#include <variantproperty.h>

#include <qmljstools/qmljscodestylepreferences.h>
#include <qmljstools/qmljstoolssettings.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectmanager.h>

#include <utils/fileutils.h>
#include <utils/qtcassert.h>

namespace {

Utils::FilePath findFile(const Utils::FilePath &path, const QString &fileName)
{
    QDirIterator it(path.toString(), QDirIterator::Subdirectories);

    while (it.hasNext()) {
        QFileInfo file(it.next());
        if (file.isDir())
            continue;

        if (file.fileName() == fileName)
            return Utils::FilePath::fromFileInfo(file);
    }
    return {};
}

QmlDesigner::PropertyNameList createNameList(const QmlDesigner::ModelNode &node)
{
    using QmlDesigner::AbstractProperty;
    using QmlDesigner::PropertyName;
    using QmlDesigner::PropertyNameList;
    static PropertyNameList defaultsNodeProps = {"id",
                                                 QmlDesigner::CollectionEditor::SOURCEFILE_PROPERTY,
                                                 QmlDesigner::CollectionEditor::JSONCHILDMODELNAME_PROPERTY,
                                                 "backend"};
    PropertyNameList dynamicPropertyNames = Utils::transform(
        node.dynamicProperties(),
        [](const AbstractProperty &property) -> PropertyName { return property.name(); });

    Utils::sort(dynamicPropertyNames);

    return defaultsNodeProps + dynamicPropertyNames;
}

} // namespace

namespace QmlDesigner {

DataStoreModelNode::DataStoreModelNode()
{
    reloadModel();
}

void DataStoreModelNode::reloadModel()
{
    using Utils::FilePath;
    if (!ProjectExplorer::ProjectManager::startupProject()) {
        reset();
        return;
    }
    bool forceUpdate = false;

    const FilePath projectFilePath = ProjectExplorer::ProjectManager::startupProject()->projectDirectory();
    const FilePath importsPath = FilePath::fromString(projectFilePath.path() + "/imports");
    FilePath dataStoreQmlPath = findFile(importsPath, "DataStore.qml");
    FilePath dataStoreJsonPath = findFile(importsPath, "DataStore.json");
    QUrl dataStoreQmlUrl = dataStoreQmlPath.toUrl();

    if (dataStoreQmlPath.exists() && dataStoreJsonPath.exists()) {
        if (!m_model.get() || m_model->fileUrl() != dataStoreQmlUrl) {
            m_model = Model::create(CollectionEditor::JSONCOLLECTIONMODEL_TYPENAME, 1, 1);
            forceUpdate = true;
            Import import = Import::createLibraryImport(CollectionEditor::COLLECTIONMODEL_IMPORT);
            try {
                if (!m_model->hasImport(import, true, true))
                    m_model->changeImports({import}, {});
            } catch (const Exception &) {
                QTC_ASSERT(false, return);
            }
        }
    } else {
        reset();
    }

    QTC_ASSERT(m_model.get(), return);
    m_model->setFileUrl(dataStoreQmlUrl);

    m_dataRelativePath = dataStoreJsonPath.relativePathFrom(dataStoreQmlPath).toFSPathString();

    if (forceUpdate) {
        updateDataStoreProperties();
        updateSingletonFile();
    }
}

QStringList DataStoreModelNode::collectionNames() const
{
    return m_collectionNames;
}

Model *DataStoreModelNode::model() const
{
    return m_model.get();
}

ModelNode DataStoreModelNode::modelNode() const
{
    QTC_ASSERT(m_model.get(), return {});
    return m_model->rootModelNode();
}

QString DataStoreModelNode::getModelQmlText()
{
    ModelNode node = modelNode();
    QTC_ASSERT(node, return {});

    Internal::QmlTextGenerator textGen(createNameList(node),
                                       QmlJSTools::QmlJSToolsSettings::globalCodeStyle()->tabSettings());

    QString genText = textGen(node);
    return genText;
}

void DataStoreModelNode::reset()
{
    if (m_model)
        m_model.reset();

    m_dataRelativePath.clear();
    setCollectionNames({});
}

void DataStoreModelNode::updateDataStoreProperties()
{
    QTC_ASSERT(model(), return);

    ModelNode rootNode = modelNode();
    QTC_ASSERT(rootNode.isValid(), return);

    static TypeName childNodeTypename = "ChildListModel";

    const QList<AbstractProperty> formerPropertyNames = rootNode.dynamicProperties();
    for (const AbstractProperty &property : formerPropertyNames)
        rootNode.removeProperty(property.name());

    rootNode.setIdWithoutRefactoring("models");

    for (const QString &collectionName : std::as_const(m_collectionNames)) {
        PropertyName newName = collectionName.toLatin1();

        ModelNode collectionNode = model()->createModelNode(childNodeTypename);

        VariantProperty modelNameProperty = collectionNode.variantProperty(
            CollectionEditor::JSONCHILDMODELNAME_PROPERTY);
        modelNameProperty.setValue(newName);

        NodeProperty nodeProp = rootNode.nodeProperty(newName);
        nodeProp.setDynamicTypeNameAndsetModelNode(childNodeTypename, collectionNode);
    }

    // Backend Property
    ModelNode backendNode = model()->createModelNode(CollectionEditor::JSONBACKEND_TYPENAME);
    NodeProperty backendProperty = rootNode.nodeProperty("backend");
    backendProperty.setDynamicTypeNameAndsetModelNode(CollectionEditor::JSONBACKEND_TYPENAME,
                                                      backendNode);
    // Source Property
    VariantProperty sourceProp = rootNode.variantProperty(CollectionEditor::SOURCEFILE_PROPERTY);
    sourceProp.setValue(m_dataRelativePath);
}

void DataStoreModelNode::updateSingletonFile()
{
    using Utils::FilePath;
    using Utils::FileSaver;
    QTC_ASSERT(m_model.get(), return);

    const QString pragmaSingleTone = "pragma Singleton\n";
    QString imports;

    for (const Import &import : m_model->imports())
        imports += QStringLiteral("import %1\n").arg(import.toString(true));

    QString content = pragmaSingleTone + imports + getModelQmlText();
    QUrl modelUrl = m_model->fileUrl();
    FileSaver file(FilePath::fromUserInput(modelUrl.isLocalFile() ? modelUrl.toLocalFile()
                                                                  : modelUrl.toString()));
    file.write(content.toLatin1());
    file.finalize();
}

void DataStoreModelNode::setCollectionNames(const QStringList &newCollectionNames)
{
    if (m_collectionNames != newCollectionNames) {
        m_collectionNames = newCollectionNames;
        updateDataStoreProperties();
        updateSingletonFile();
    }
}

} // namespace QmlDesigner
