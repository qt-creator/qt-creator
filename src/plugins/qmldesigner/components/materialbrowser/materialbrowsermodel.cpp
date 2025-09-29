// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "materialbrowsermodel.h"

#include "designmodewidget.h"
#include "materialbrowserview.h"
#include "qmldesignerplugin.h"
#include "qmlobjectnode.h"
#include "variantproperty.h"
#include "qmltimelinekeyframegroup.h"

#include <utils3d.h>

#include <utils/qtcassert.h>

namespace QmlDesigner {

MaterialBrowserModel::MaterialBrowserModel(MaterialBrowserView *view, QObject *parent)
    : QAbstractListModel(parent)
    , m_view(view)
{
}

MaterialBrowserModel::~MaterialBrowserModel()
{
}

int MaterialBrowserModel::rowCount(const QModelIndex &) const
{
    return m_materialList.size();
}

QVariant MaterialBrowserModel::data(const QModelIndex &index, int role) const
{
    QTC_ASSERT(index.isValid(), return {});

    switch (role) {
    case Roles::NameRole: {
        QVariant objName = m_materialList.at(index.row()).variantProperty("objectName").value();
        return objName.isValid() ? objName : "";
    } break;
    case Roles::InternalIdRole:
        return m_materialList.at(index.row()).internalId();
    case Roles::MatchedSearchRole:
        return isVisible(index.row());
    case Roles::SelectedRole:
        return m_materialList.at(index.row()).isSelected();
    case Roles::IsComponentRole:
        return m_materialList.at(index.row()).isComponent();
    case Roles::TypeRole: {
        QString matType = QString::fromLatin1(m_materialList.at(index.row()).type());
        if (matType.startsWith("QtQuick3D."))
            matType.remove("QtQuick3D.");
        return matType;
    } break;
    case Roles::HasDynamicPropertiesRole:
        return !m_materialList.at(index.row()).dynamicProperties().isEmpty();
    default:
        return {};
    };
}

bool MaterialBrowserModel::isVisible(int idx) const
{
    if (!isValidIndex(idx))
        return false;

    return m_searchText.isEmpty() || m_materialList.at(idx).variantProperty("objectName")
            .value().toString().contains(m_searchText, Qt::CaseInsensitive);
}

bool MaterialBrowserModel::isValidIndex(int idx) const
{
    return idx > -1 && idx < rowCount();
}

/**
 * @brief Loads and parses propertyGroups.json from QtQuick3D module's designer folder
 *
 * propertyGroups.json contains lists of QtQuick3D objects' properties grouped by sections
 *
 * @param path path to propertyGroups.json file
 * @return load successful
 */
bool MaterialBrowserModel::loadPropertyGroups(const QString &path)
{
    bool ok = true;

    QFile matPropsFile(path);
    if (!matPropsFile.open(QIODevice::ReadOnly)) {
        qWarning("Couldn't open propertyGroups.json");
        ok = false;
    }

    if (ok) {
        QJsonDocument matPropsJsonDoc = QJsonDocument::fromJson(matPropsFile.readAll());
        if (matPropsJsonDoc.isNull()) {
            qWarning("Invalid propertyGroups.json file");
            ok = false;
        } else {
            m_propertyGroupsObj = matPropsJsonDoc.object();
        }
    }

    m_defaultMaterialSections.clear();
    m_principledMaterialSections.clear();
    m_specularGlossyMaterialSections.clear();
    m_customMaterialSections.clear();
    if (ok) {
        m_defaultMaterialSections.append(m_propertyGroupsObj.value("DefaultMaterial").toObject().keys());
        m_principledMaterialSections.append(m_propertyGroupsObj.value("PrincipledMaterial").toObject().keys());
        m_specularGlossyMaterialSections.append(m_propertyGroupsObj.value("SpecularGlossyMaterial").toObject().keys());

        QStringList customMatSections = m_propertyGroupsObj.value("CustomMaterial").toObject().keys();
        if (customMatSections.size() > 1) // as of now custom material has only 1 section, so we don't add it
            m_customMaterialSections.append(customMatSections);
    } else {
        m_propertyGroupsObj = {};
    }
    emit materialSectionsChanged();

    return ok;
}

void MaterialBrowserModel::unloadPropertyGroups()
{
    if (!m_propertyGroupsObj.isEmpty()) {
        m_propertyGroupsObj = {};
        m_defaultMaterialSections.clear();
        m_principledMaterialSections.clear();
        m_specularGlossyMaterialSections.clear();
        m_customMaterialSections.clear();
        emit materialSectionsChanged();
    }
}

QHash<int, QByteArray> MaterialBrowserModel::roleNames() const
{
    static const QHash<int, QByteArray> roles{
        {Roles::NameRole, "materialName"},
        {Roles::InternalIdRole, "materialInternalId"},
        {Roles::MatchedSearchRole, "materialMatchedSearch"},
        {Roles::SelectedRole, "materialSelected"},
        {Roles::IsComponentRole, "materialIsComponent"},
        {Roles::TypeRole, "materialType"},
        {Roles::HasDynamicPropertiesRole, "hasDynamicProperties"},
    };
    return roles;
}

bool MaterialBrowserModel::hasQuick3DImport() const
{
    return m_hasQuick3DImport;
}

void MaterialBrowserModel::setHasQuick3DImport(bool b)
{
    if (b == m_hasQuick3DImport)
        return;

    m_hasQuick3DImport = b;
    emit hasQuick3DImportChanged();
}

bool MaterialBrowserModel::hasModelSelection() const
{
    return m_hasModelSelection;
}

void MaterialBrowserModel::setHasModelSelection(bool b)
{
    if (b == m_hasModelSelection)
        return;

    m_hasModelSelection = b;
    emit hasModelSelectionChanged();
}

bool MaterialBrowserModel::hasMaterialLibrary() const
{
    return m_hasMaterialLibrary;
}

void MaterialBrowserModel::setHasMaterialLibrary(bool b)
{
    if (m_hasMaterialLibrary == b)
        return;

    m_hasMaterialLibrary = b;
    emit hasMaterialLibraryChanged();
}

bool MaterialBrowserModel::isQt6Project() const
{
    return m_isQt6Project;
}

void MaterialBrowserModel::setIsQt6Project(bool b)
{
    if (m_isQt6Project == b)
        return;

    m_isQt6Project = b;
    emit isQt6ProjectChanged();
}

bool MaterialBrowserModel::isMcuProject() const
{
    return m_isMcuProject;
}

void MaterialBrowserModel::setIsMcuProject(bool b)
{
    if (m_isMcuProject == b)
        return;

    m_isMcuProject = b;
    emit isMcuProjectChanged();
}

QString MaterialBrowserModel::copiedMaterialType() const
{
    return m_copiedMaterialType;
}

void MaterialBrowserModel::setCopiedMaterialType(const QString &matType)
{
    if (matType == m_copiedMaterialType)
        return;

    m_copiedMaterialType = matType;
    emit copiedMaterialTypeChanged();
}

QList<ModelNode> MaterialBrowserModel::materials() const
{
    return m_materialList;
}

void MaterialBrowserModel::setSearchText(const QString &searchText)
{
    QString lowerSearchText = searchText.toLower();

    if (m_searchText == lowerSearchText)
        return;

    m_searchText = lowerSearchText;

    refreshSearch();
}

void MaterialBrowserModel::refreshSearch()
{
    bool isEmpty = true;

    for (int i = 0; i < m_materialList.size(); ++i) {
        if (isVisible(i)) {
            isEmpty = false;
            break;
        }
    }

    if (isEmpty != m_isEmpty) {
        m_isEmpty = isEmpty;
        emit isEmptyChanged();
    }

    resetModel();
}

void MaterialBrowserModel::setMaterials(const QList<ModelNode> &materials, bool hasQuick3DImport)
{
    m_materialList = materials;
    m_materialIndexHash.clear();
    for (int i = 0; i < materials.size(); ++i)
        m_materialIndexHash.insert(materials.at(i).internalId(), i);

    bool isEmpty = materials.size() == 0;
    if (isEmpty != m_isEmpty) {
        m_isEmpty = isEmpty;
        emit isEmptyChanged();
    }

    if (!m_searchText.isEmpty())
        refreshSearch();
    else
        resetModel();

    setHasQuick3DImport(hasQuick3DImport);
}

void MaterialBrowserModel::removeMaterial(const ModelNode &material)
{
    if (!m_materialIndexHash.contains(material.internalId()))
        return;

    m_materialList.removeOne(material);
    int idx = m_materialIndexHash.value(material.internalId());
    m_materialIndexHash.remove(material.internalId());

    // update index hash
    for (int i = idx; i < rowCount(); ++i)
        m_materialIndexHash.insert(m_materialList.at(i).internalId(), i);

    resetModel();

    if (m_materialList.isEmpty()) {
        m_isEmpty = true;
        emit isEmptyChanged();
    }
}

void MaterialBrowserModel::deleteSelectedMaterials()
{
    m_view->executeInTransaction(__FUNCTION__, [this] {
        QStack<int> selectedIndexes;
        for (int i = 0; i < m_materialList.size(); ++i) {
            if (m_materialList.at(i).isSelected())
                selectedIndexes << i;
        }

        while (!selectedIndexes.isEmpty())
            deleteMaterial(selectedIndexes.pop());
    });
}

void MaterialBrowserModel::updateMaterialName(const ModelNode &material)
{
    int idx = materialIndex(material);
    if (idx != -1)
        emit dataChanged(index(idx, 0), index(idx, 0), {roleNames().key("materialName")});
}

int MaterialBrowserModel::materialIndex(const ModelNode &material) const
{
    return m_materialIndexHash.value(material.internalId(), -1);
}

ModelNode MaterialBrowserModel::materialAt(int idx) const
{
    if (isValidIndex(idx))
        return m_materialList.at(idx);

    return {};
}

void MaterialBrowserModel::resetModel()
{
    beginResetModel();
    endResetModel();
}

void MaterialBrowserModel::notifySelectionChanges(const QList<ModelNode> &selectedNodes,
                                                  const QList<ModelNode> &deselectedNodes)
{
    QList<int> indices;
    indices.reserve(selectedNodes.size() + deselectedNodes.size());
    for (const ModelNode &node : selectedNodes)
        indices.append(materialIndex(node));

    for (const ModelNode &node : deselectedNodes)
        indices.append(materialIndex(node));

    using Bound = QPair<int, int>;
    const QList<Bound> &bounds = MaterialBrowserView::getSortedBounds(indices);

    for (const Bound &bound : bounds)
        emit dataChanged(index(bound.first), index(bound.second), {Roles::SelectedRole});
}

void MaterialBrowserModel::updateMaterialComponent(int idx)
{
    if (!isValidIndex(idx))
        return;

    const QModelIndex &mIdx = index(idx);
    emit dataChanged(mIdx, mIdx, {Roles::IsComponentRole});
}

void MaterialBrowserModel::selectMaterial(int idx, bool appendMat)
{
    if (!isValidIndex(idx))
        return;

    ModelNode mat = m_materialList.at(idx);
    QTC_ASSERT(mat, return);

    if (appendMat)
        mat.view()->selectModelNode(mat);
    else
        mat.selectNode();
}

void MaterialBrowserModel::duplicateMaterial(int idx)
{
    emit duplicateMaterialTriggered(m_materialList.at(idx));
}

void MaterialBrowserModel::copyMaterialProperties(int idx, const QString &section)
{
    m_copiedMaterial = m_materialList.at(idx);

    QTC_ASSERT(m_copiedMaterial.isValid(), return);

    QString matType = QString::fromLatin1(m_copiedMaterial.type());

    if (matType.startsWith("QtQuick3D."))
        matType.remove("QtQuick3D.");

    setCopiedMaterialType(matType);
    m_allPropsCopied = section == "All";
    bool dynamicPropsCopied = section == "Custom";
    QmlObjectNode mat(m_copiedMaterial);

    QSet<PropertyName> validProps;
    QHash<PropertyName, TypeName> dynamicProps;
    PropertyNameList copiedProps;

    if (dynamicPropsCopied || m_allPropsCopied) {
        // Dynamic properties must always be set in base state
        const QList<AbstractProperty> dynProps = m_copiedMaterial.dynamicProperties();
        for (const auto &prop : dynProps) {
            dynamicProps.insert(prop.name().toByteArray(), prop.dynamicTypeName());
            validProps.insert(prop.name().toByteArray());
        }
    }

    if (!dynamicPropsCopied) {
        // Base state properties are always valid
        const auto baseProps = m_copiedMaterial.propertyNames();
        for (const auto &baseProp : baseProps)
            validProps.insert(baseProp);

        if (!mat.isInBaseState()) {
            QmlPropertyChanges changes = mat.ensurePropertyChangeForCurrentState();
            if (changes.isValid()) {
                const QList<AbstractProperty> changedProps = changes.targetProperties();
                for (const auto &changedProp : changedProps)
                    validProps.insert(changedProp.name().toByteArray());
            }
        }

        if (mat.timelineIsActive()) {
            const QList<QmlTimelineKeyframeGroup> keyframeGroups
                    = mat.currentTimeline().keyframeGroupsForTarget(m_copiedMaterial);
            for (const auto &kfg : keyframeGroups)
                validProps.insert(kfg.propertyName());
        }
    }
    validProps.remove("objectName");
    validProps.remove("data");

    if (m_allPropsCopied || dynamicPropsCopied || m_propertyGroupsObj.empty()) {
        copiedProps = validProps.values();
    } else {
        QJsonObject propsSpecObj = m_propertyGroupsObj.value(m_copiedMaterialType).toObject();
        if (propsSpecObj.contains(section)) { // should always be true
           const QJsonArray propNames = propsSpecObj.value(section).toArray();
           for (const QJsonValueConstRef &propName : propNames)
               copiedProps.append(propName.toString().toLatin1());

           if (section == "Base") { // add QtQuick3D.Material base props as well
               QJsonObject propsMatObj = m_propertyGroupsObj.value("Material").toObject();
               const QJsonArray propNames = propsMatObj.value("Base").toArray();
               for (const QJsonValueConstRef &propName : propNames)
                   copiedProps.append(propName.toString().toLatin1());
           }
        }
    }

    m_copiedMaterialProps.clear();
    for (const PropertyName &propName : std::as_const(copiedProps)) {
        PropertyCopyData data;
        data.name = propName;
        data.isValid = m_allPropsCopied || validProps.contains(propName);
        if (data.isValid) {
            if (dynamicProps.contains(propName))
                data.dynamicTypeName = dynamicProps[propName];
            data.isBinding = mat.hasBindingProperty(propName);
            if (data.isBinding)
                data.value = mat.expression(propName);
            else
                data.value = mat.modelValue(propName);
        }
        m_copiedMaterialProps.append(data);
    }
}

void MaterialBrowserModel::pasteMaterialProperties(int idx)
{
    ModelNode targetMat = m_materialList.at(idx);
    if (targetMat.isValid() && m_copiedMaterial.isValid() && targetMat != m_copiedMaterial)
        emit pasteMaterialPropertiesTriggered(targetMat, m_copiedMaterialProps, m_allPropsCopied);
}

void MaterialBrowserModel::deleteMaterial(int idx)
{
    if (m_view && isValidIndex(idx)) {
        ModelNode node = m_materialList[idx];
        if (node.isValid()) {
            m_view->executeInTransaction(__FUNCTION__, [&] {
                node.destroy();
            });
        }
    }
}

void MaterialBrowserModel::renameMaterial(int idx, const QString &newName)
{
    ModelNode mat = m_materialList.at(idx);
    emit renameMaterialTriggered(mat, newName);
}

void MaterialBrowserModel::addNewMaterial()
{
    emit addNewMaterialTriggered();
}

void MaterialBrowserModel::applyToSelected(qint64 internalId, bool add)
{
    int idx = m_materialIndexHash.value(internalId);
    if (idx != -1) {
        ModelNode mat = m_materialList.at(idx);
        emit applyToSelectedTriggered(mat, add);
    }
}

// This is provided as invokable instead of property, as it is difficult to know when ModelNode
// becomes invalid. Much simpler to evaluate this on demand.
bool MaterialBrowserModel::isCopiedMaterialValid() const
{
    return m_copiedMaterial.isValid();
}

} // namespace QmlDesigner
