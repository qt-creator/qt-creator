// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "datastoremodelnode.h"

#include "abstractview.h"
#include "collectioneditorconstants.h"
#include "collectioneditorutils.h"
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

#include <QRegularExpression>
#include <QRegularExpressionMatch>

namespace {

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

bool isValidCollectionPropertyName(const QString &collectionId)
{
    static const QmlDesigner::PropertyNameList reservedKeywords = {
        QmlDesigner::CollectionEditor::SOURCEFILE_PROPERTY,
        QmlDesigner::CollectionEditor::JSONBACKEND_TYPENAME,
        "backend",
        "models",
    };

    return QmlDesigner::ModelNode::isValidId(collectionId)
           && !reservedKeywords.contains(collectionId.toLatin1());
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

    const FilePath dataStoreQmlPath = CollectionEditor::dataStoreQmlFilePath();
    const FilePath dataStoreJsonPath = CollectionEditor::dataStoreJsonFilePath();
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

    if (forceUpdate)
        update();
}

QStringList DataStoreModelNode::collectionNames() const
{
    return m_collectionPropertyNames.keys();
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

    QSet<QString> collectionNamesToBeAdded;
    const QStringList allCollectionNames = m_collectionPropertyNames.keys();
    for (const QString &collectionName : allCollectionNames)
        collectionNamesToBeAdded << collectionName;

    const QList<AbstractProperty> formerPropertyNames = rootNode.dynamicProperties();

    // Remove invalid collection names from the properties
    for (const AbstractProperty &property : formerPropertyNames) {
        if (!property.isNodeProperty())
            continue;

        NodeProperty nodeProprty = property.toNodeProperty();
        if (!nodeProprty.hasDynamicTypeName(childNodeTypename))
            continue;

        ModelNode childNode = nodeProprty.modelNode();
        if (childNode.hasProperty(CollectionEditor::JSONCHILDMODELNAME_PROPERTY)) {
            QString modelName = childNode.property(CollectionEditor::JSONCHILDMODELNAME_PROPERTY)
                                    .toVariantProperty()
                                    .value()
                                    .toString();
            if (collectionNamesToBeAdded.contains(modelName)) {
                m_collectionPropertyNames.insert(modelName, property.name());
                collectionNamesToBeAdded.remove(modelName);
            } else {
                rootNode.removeProperty(property.name());
            }
        } else {
            rootNode.removeProperty(property.name());
        }
    }

    rootNode.setIdWithoutRefactoring("models");

    QStringList collectionNamesLeft = collectionNamesToBeAdded.values();
    Utils::sort(collectionNamesLeft);
    for (const QString &collectionName : std::as_const(collectionNamesLeft)) {
        PropertyName newPropertyName = getUniquePropertyName(collectionName);
        if (newPropertyName.isEmpty()) {
            qWarning() << __FUNCTION__ << __LINE__
                       << QString("The property name cannot be generated from \"%1\"").arg(collectionName);
            continue;
        }

        ModelNode collectionNode = model()->createModelNode(childNodeTypename);
        VariantProperty modelNameProperty = collectionNode.variantProperty(
            CollectionEditor::JSONCHILDMODELNAME_PROPERTY);
        modelNameProperty.setValue(collectionName);

        NodeProperty nodeProp = rootNode.nodeProperty(newPropertyName);
        nodeProp.setDynamicTypeNameAndsetModelNode(childNodeTypename, collectionNode);

        m_collectionPropertyNames.insert(collectionName, newPropertyName);
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

void DataStoreModelNode::update()
{
    updateDataStoreProperties();
    updateSingletonFile();
}

PropertyName DataStoreModelNode::getUniquePropertyName(const QString &collectionName)
{
    ModelNode dataStoreNode = modelNode();
    QTC_ASSERT(!collectionName.isEmpty() && dataStoreNode.isValid(), return {});

    QString newProperty;

    // convert to camel case
    QStringList nameWords = collectionName.split(' ');
    nameWords[0] = nameWords[0].at(0).toLower() + nameWords[0].mid(1);
    for (int i = 1; i < nameWords.size(); ++i)
        nameWords[i] = nameWords[i].at(0).toUpper() + nameWords[i].mid(1);
    newProperty = nameWords.join("");

    // if id starts with a number prepend an underscore
    if (newProperty.at(0).isDigit())
        newProperty.prepend('_');

    // If the new id is not valid (e.g. qml keyword match), prepend an underscore
    if (!isValidCollectionPropertyName(newProperty))
        newProperty.prepend('_');

    static const QRegularExpression rgx("\\d+$"); // matches a number at the end of a string
    while (dataStoreNode.hasProperty(newProperty.toLatin1())) { // id exists
        QRegularExpressionMatch match = rgx.match(newProperty);
        if (match.hasMatch()) {                                 // ends with a number, increment it
            QString numStr = match.captured();
            int num = numStr.toInt() + 1;
            newProperty = newProperty.mid(0, match.capturedStart()) + QString::number(num);
        } else {
            newProperty.append('1');
        }
    }

    return newProperty.toLatin1();
}

void DataStoreModelNode::setCollectionNames(const QStringList &newCollectionNames)
{
    m_collectionPropertyNames.clear();
    for (const QString &collectionName : newCollectionNames)
        m_collectionPropertyNames.insert(collectionName, {});
    update();
}

void DataStoreModelNode::renameCollection(const QString &oldName, const QString &newName)
{
    ModelNode dataStoreNode = modelNode();
    QTC_ASSERT(dataStoreNode.isValid(), return);

    if (m_collectionPropertyNames.contains(oldName)) {
        const PropertyName oldPropertyName = m_collectionPropertyNames.value(oldName);
        if (!oldPropertyName.isEmpty() && dataStoreNode.hasProperty(oldPropertyName)) {
            NodeProperty collectionNode = dataStoreNode.property(oldPropertyName).toNodeProperty();
            if (collectionNode.isValid()) {
                VariantProperty modelNameProperty = collectionNode.modelNode().variantProperty(
                    CollectionEditor::JSONCHILDMODELNAME_PROPERTY);
                modelNameProperty.setValue(newName);
                m_collectionPropertyNames.remove(oldName);
                m_collectionPropertyNames.insert(newName, collectionNode.name());
                update();
                return;
            }
            qWarning() << __FUNCTION__ << __LINE__
                       << "There is no valid node for the old collection name";
            return;
        }
        qWarning() << __FUNCTION__ << __LINE__ << QString("Invalid old property name")
                   << oldPropertyName;
        return;
    }
    qWarning() << __FUNCTION__ << __LINE__
               << QString("There is no old collection name registered with this name \"%1\"").arg(oldName);
}

void DataStoreModelNode::removeCollection(const QString &collectionName)
{
    if (m_collectionPropertyNames.contains(collectionName)) {
        m_collectionPropertyNames.remove(collectionName);
        update();
    }
}

void DataStoreModelNode::assignCollectionToNode(AbstractView *view,
                                                const ModelNode &targetNode,
                                                const QString &collectionName)
{
    QTC_ASSERT(targetNode.isValid(), return);

    if (!CollectionEditor::canAcceptCollectionAsModel(targetNode))
        return;

    if (!m_collectionPropertyNames.contains(collectionName)) {
        qWarning() << __FUNCTION__ << __LINE__ << "Collection doesn't exist in the DataStore"
                   << collectionName;
        return;
    }

    PropertyName propertyName = m_collectionPropertyNames.value(collectionName);

    const ModelNode dataStore = modelNode();
    VariantProperty sourceProperty = dataStore.variantProperty(propertyName);
    if (!sourceProperty.exists()) {
        qWarning() << __FUNCTION__ << __LINE__
                   << "The source property doesn't exist in the DataStore.";
        return;
    }

    BindingProperty modelProperty = targetNode.bindingProperty("model");

    QString identifier = QString("DataStore.%1").arg(QString::fromLatin1(sourceProperty.name()));

    view->executeInTransaction("assignCollectionToNode", [&modelProperty, &identifier]() {
        modelProperty.setExpression(identifier);
    });
}

} // namespace QmlDesigner
