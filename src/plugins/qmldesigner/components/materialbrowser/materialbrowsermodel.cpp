// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "materialbrowsermodel.h"

#include "designmodewidget.h"
#include "materialbrowserview.h"
#include "qmldesignerplugin.h"
#include "qmlobjectnode.h"
#include "variantproperty.h"
#include "qmltimelinekeyframegroup.h"

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
    QTC_ASSERT(index.isValid() && index.row() < m_materialList.size(), return {});
    QTC_ASSERT(roleNames().contains(role), return {});

    QByteArray roleName = roleNames().value(role);
    if (roleName == "materialName") {
        QVariant objName = m_materialList.at(index.row()).variantProperty("objectName").value();
        return objName.isValid() ? objName : "";
    }

    if (roleName == "materialInternalId")
        return m_materialList.at(index.row()).internalId();

    if (roleName == "materialVisible")
        return isVisible(index.row());

    if (roleName == "materialType") {
        QString matType = QString::fromLatin1(m_materialList.at(index.row()).type());
        if (matType.startsWith("QtQuick3D."))
            matType.remove("QtQuick3D.");
        return matType;
    }

    if (roleName == "hasDynamicProperties")
        return !m_materialList.at(index.row()).dynamicProperties().isEmpty();

    return {};
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
    static const QHash<int, QByteArray> roles {
        {Qt::UserRole + 1, "materialName"},
        {Qt::UserRole + 2, "materialInternalId"},
        {Qt::UserRole + 3, "materialVisible"},
        {Qt::UserRole + 4, "materialType"},
        {Qt::UserRole + 5, "hasDynamicProperties"}
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
    bool isEmpty = false;

    // if selected material goes invisible, select nearest material
    if (!isVisible(m_selectedIndex)) {
        int inc = 1;
        int incCap = m_materialList.size();
        while (!isEmpty && inc < incCap) {
            if (isVisible(m_selectedIndex - inc)) {
                selectMaterial(m_selectedIndex - inc);
                break;
            } else if (isVisible(m_selectedIndex + inc)) {
                selectMaterial(m_selectedIndex + inc);
                break;
            }
            ++inc;
            isEmpty = !isValidIndex(m_selectedIndex + inc)
                   && !isValidIndex(m_selectedIndex - inc);
        }
        if (!isVisible(m_selectedIndex)) // handles the case of a single material
            isEmpty = true;
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

    updateSelectedMaterial();
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

void MaterialBrowserModel::deleteSelectedMaterial()
{
    deleteMaterial(m_selectedIndex);
}

void MaterialBrowserModel::updateSelectedMaterial()
{
    selectMaterial(m_selectedIndex, true);
}

void MaterialBrowserModel::updateMaterialName(const ModelNode &material)
{
    int idx = materialIndex(material);
    if (idx != -1)
        emit dataChanged(index(idx, 0), index(idx, 0), {roleNames().key("materialName")});
}

int MaterialBrowserModel::materialIndex(const ModelNode &material) const
{
    if (m_materialIndexHash.contains(material.internalId()))
        return m_materialIndexHash.value(material.internalId());

    return -1;
}

ModelNode MaterialBrowserModel::materialAt(int idx) const
{
    if (isValidIndex(idx))
        return m_materialList.at(idx);

    return {};
}

ModelNode MaterialBrowserModel::selectedMaterial() const
{
    if (isValidIndex(m_selectedIndex))
        return m_materialList[m_selectedIndex];
    return {};
}

void MaterialBrowserModel::resetModel()
{
    beginResetModel();
    endResetModel();
}

void MaterialBrowserModel::selectMaterial(int idx, bool force)
{
    if (m_materialList.size() == 0) {
        m_selectedIndex = -1;
        emit selectedIndexChanged(m_selectedIndex);
        return;
    }

    idx = std::max(0, std::min(idx, rowCount() - 1));

    if (idx != m_selectedIndex || force) {
        m_selectedIndex = idx;
        emit selectedIndexChanged(idx);
    }
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
            dynamicProps.insert(prop.name(), prop.dynamicTypeName());
            validProps.insert(prop.name());
        }
    }

    if (!dynamicPropsCopied) {
        // Base state properties are always valid
        const auto baseProps = m_copiedMaterial.propertyNames();
        for (const auto &baseProp : baseProps)
            validProps.insert(baseProp);

        if (!mat.isInBaseState()) {
            QmlPropertyChanges changes = mat.propertyChangeForCurrentState();
            if (changes.isValid()) {
                const QList<AbstractProperty> changedProps = changes.targetProperties();
                for (const auto &changedProp : changedProps)
                    validProps.insert(changedProp.name());
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
           // auto == QJsonValueConstRef after 04dc959d49e5e3 / Qt 6.4, QJsonValueRef before
           for (const auto &propName : propNames)
               copiedProps.append(propName.toString().toLatin1());

           if (section == "Base") { // add QtQuick3D.Material base props as well
               QJsonObject propsMatObj = m_propertyGroupsObj.value("Material").toObject();
               const QJsonArray propNames = propsMatObj.value("Base").toArray();
               // auto == QJsonValueConstRef after 04dc959d49e5e3 / Qt 6.4, QJsonValueRef before
               for (const auto &propName : propNames)
                   copiedProps.append(propName.toString().toLatin1());
           }
        }
    }

    m_copiedMaterialProps.clear();
    for (const auto &propName : copiedProps) {
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

void MaterialBrowserModel::openMaterialEditor()
{
    QmlDesignerPlugin::instance()->mainWidget()->showDockWidget("MaterialEditor", true);
}

// This is provided as invokable instead of property, as it is difficult to know when ModelNode
// becomes invalid. Much simpler to evaluate this on demand.
bool MaterialBrowserModel::isCopiedMaterialValid() const
{
    return m_copiedMaterial.isValid();
}

} // namespace QmlDesigner
