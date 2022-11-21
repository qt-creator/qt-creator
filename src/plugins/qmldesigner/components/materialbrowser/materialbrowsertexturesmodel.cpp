// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "materialbrowsertexturesmodel.h"

#include <bindingproperty.h>
#include <designmodewidget.h>
#include <qmldesignerplugin.h>
#include <qmlobjectnode.h>
#include <variantproperty.h>
#include <qmltimelinekeyframegroup.h>
#include "utils/qtcassert.h"

namespace QmlDesigner {

MaterialBrowserTexturesModel::MaterialBrowserTexturesModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

MaterialBrowserTexturesModel::~MaterialBrowserTexturesModel()
{
}

int MaterialBrowserTexturesModel::rowCount(const QModelIndex &) const
{
    return m_textureList.count();
}

QVariant MaterialBrowserTexturesModel::data(const QModelIndex &index, int role) const
{
    QTC_ASSERT(index.isValid() && index.row() < m_textureList.count(), return {});
    QTC_ASSERT(roleNames().contains(role), return {});

    QByteArray roleName = roleNames().value(role);
    if (roleName == "textureSource") {
        QString source = m_textureList.at(index.row()).variantProperty("source").value().toString();
        return QVariant(DocumentManager::currentResourcePath().path() + '/' + source);
    }

    if (roleName == "textureVisible")
        return isTextureVisible(index.row());

    if (roleName == "hasDynamicProperties")
        return !m_textureList.at(index.row()).dynamicProperties().isEmpty();

    return {};
}

bool MaterialBrowserTexturesModel::isTextureVisible(int idx) const
{
    if (!isValidIndex(idx))
        return false;

    return m_searchText.isEmpty() || m_textureList.at(idx).variantProperty("objectName")
            .value().toString().contains(m_searchText, Qt::CaseInsensitive);
}

bool MaterialBrowserTexturesModel::isValidIndex(int idx) const
{
    return idx > -1 && idx < rowCount();
}


QHash<int, QByteArray> MaterialBrowserTexturesModel::roleNames() const
{
    static const QHash<int, QByteArray> roles {
        {Qt::UserRole + 1, "textureSource"},
        {Qt::UserRole + 2, "textureVisible"},
        {Qt::UserRole + 3, "hasDynamicProperties"}
    };
    return roles;
}

QList<ModelNode> MaterialBrowserTexturesModel::textures() const
{
    return m_textureList;
}

void MaterialBrowserTexturesModel::setSearchText(const QString &searchText)
{
    QString lowerSearchText = searchText.toLower();

    if (m_searchText == lowerSearchText)
        return;

    m_searchText = lowerSearchText;

    bool isEmpty = false;

    // if selected texture goes invisible, select nearest one
    if (!isTextureVisible(m_selectedIndex)) {
        int inc = 1;
        int incCap = m_textureList.count();
        while (!isEmpty && inc < incCap) {
            if (isTextureVisible(m_selectedIndex - inc)) {
                selectTexture(m_selectedIndex - inc);
                break;
            } else if (isTextureVisible(m_selectedIndex + inc)) {
                selectTexture(m_selectedIndex + inc);
                break;
            }
            ++inc;
            isEmpty = !isValidIndex(m_selectedIndex + inc)
                   && !isValidIndex(m_selectedIndex - inc);
        }
        if (!isTextureVisible(m_selectedIndex)) // handles the case of a single item
            isEmpty = true;
    }

    if (isEmpty != m_isEmpty) {
        m_isEmpty = isEmpty;
        emit isEmptyChanged();
    }

    resetModel();
}

void MaterialBrowserTexturesModel::setTextures(const QList<ModelNode> &textures)
{
    m_textureList = textures;
    m_textureIndexHash.clear();
    for (int i = 0; i < textures.size(); ++i)
        m_textureIndexHash.insert(textures.at(i).internalId(), i);

    bool isEmpty = textures.size() == 0;
    if (isEmpty != m_isEmpty) {
        m_isEmpty = isEmpty;
        emit isEmptyChanged();
    }

    updateSelectedTexture();
    resetModel();
}

void MaterialBrowserTexturesModel::removeTexture(const ModelNode &texture)
{
    if (!m_textureIndexHash.contains(texture.internalId()))
        return;

    m_textureList.removeOne(texture);
    int idx = m_textureIndexHash.value(texture.internalId());
    m_textureIndexHash.remove(texture.internalId());

    // update index hash
    for (int i = idx; i < rowCount(); ++i)
        m_textureIndexHash.insert(m_textureList.at(i).internalId(), i);

    resetModel();

    if (m_textureList.isEmpty()) {
        m_isEmpty = true;
        emit isEmptyChanged();
    }
}

void MaterialBrowserTexturesModel::addNewTexture()
{
    emit addNewTextureTriggered();
}

void MaterialBrowserTexturesModel::deleteSelectedTexture()
{
    deleteTexture(m_selectedIndex);
}

void MaterialBrowserTexturesModel::updateSelectedTexture()
{
    selectTexture(m_selectedIndex, true);
}

int MaterialBrowserTexturesModel::textureIndex(const ModelNode &texture) const
{
    if (m_textureIndexHash.contains(texture.internalId()))
        return m_textureIndexHash.value(texture.internalId());

    return -1;
}

ModelNode MaterialBrowserTexturesModel::textureAt(int idx) const
{
    if (isValidIndex(idx))
        return m_textureList.at(idx);

    return {};
}

bool MaterialBrowserTexturesModel::hasSingleModelSelection() const
{
    return m_hasSingleModelSelection;
}

void MaterialBrowserTexturesModel::setHasSingleModelSelection(bool b)
{
    if (b == m_hasSingleModelSelection)
        return;

    m_hasSingleModelSelection = b;
    emit hasSingleModelSelectionChanged();
}

void MaterialBrowserTexturesModel::resetModel()
{
    beginResetModel();
    endResetModel();
}

void MaterialBrowserTexturesModel::selectTexture(int idx, bool force)
{
    if (m_textureList.size() == 0) {
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

void MaterialBrowserTexturesModel::duplicateTexture(int idx)
{
    emit duplicateTextureTriggered(m_textureList.at(idx));
}

void MaterialBrowserTexturesModel::deleteTexture(int idx)
{
    if (isValidIndex(idx)) {
        ModelNode node = m_textureList[idx];
        if (node.isValid())
            QmlObjectNode(node).destroy();
    }
}

} // namespace QmlDesigner
