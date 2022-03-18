/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include "materialbrowsermodel.h"
#include "variantproperty.h"
#include <designmodewidget.h>
#include <qmldesignerplugin.h>

namespace QmlDesigner {

MaterialBrowserModel::MaterialBrowserModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

MaterialBrowserModel::~MaterialBrowserModel()
{
}

int MaterialBrowserModel::rowCount(const QModelIndex &) const
{
    return m_materialList.count();
}

QVariant MaterialBrowserModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_materialList.count()) {
        qWarning() << Q_FUNC_INFO << "invalid index requested";
        return {};
    }

    if (roleNames().value(role) == "materialName") {
        QVariant objName = m_materialList.at(index.row()).variantProperty("objectName").value();
        return objName.isValid() ? objName : "";
    }

    if (roleNames().value(role) == "materialInternalId")
        return m_materialList.at(index.row()).internalId();

    if (roleNames().value(role) == "materialVisible")
        return isMaterialVisible(index.row());

    if (!roleNames().contains(role))
        qWarning() << Q_FUNC_INFO << "invalid role requested";

    return {};
}

bool MaterialBrowserModel::isMaterialVisible(int idx) const
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

QHash<int, QByteArray> MaterialBrowserModel::roleNames() const
{
    static const QHash<int, QByteArray> roles {
        {Qt::UserRole + 1, "materialName"},
        {Qt::UserRole + 2, "materialInternalId"},
        {Qt::UserRole + 3, "materialVisible"},
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

void MaterialBrowserModel::setSearchText(const QString &searchText)
{
    QString lowerSearchText = searchText.toLower();

    if (m_searchText == lowerSearchText)
        return;

    m_searchText = lowerSearchText;

    bool isEmpty = false;

    // if selected material goes invisible, select nearest material
    if (!isMaterialVisible(m_selectedIndex)) {
        int inc = 1;
        int incCap = m_materialList.count();
        while (!isEmpty && inc < incCap) {
            if (isMaterialVisible(m_selectedIndex - inc)) {
                selectMaterial(m_selectedIndex - inc);
                break;
            } else if (isMaterialVisible(m_selectedIndex + inc)) {
                selectMaterial(m_selectedIndex + inc);
                break;
            }
            ++inc;
            isEmpty = !isValidIndex(m_selectedIndex + inc)
                   && !isValidIndex(m_selectedIndex - inc);
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

    if (hasQuick3DImport != m_hasQuick3DImport) {
        m_hasQuick3DImport = hasQuick3DImport;
        emit hasQuick3DImportChanged();
    }

    updateSelectedMaterial();
    resetModel();
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

void MaterialBrowserModel::deleteMaterial(qint32 internalId)
{
    int idx = m_materialIndexHash.value(internalId);
    if (isValidIndex(idx))
        m_materialList[idx].destroy();
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
    QmlDesignerPlugin::instance()->mainWidget()->showDockWidget("MaterialEditor");
}

} // namespace QmlDesigner
