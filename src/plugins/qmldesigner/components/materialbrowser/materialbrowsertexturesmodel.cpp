// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "materialbrowsertexturesmodel.h"

#include "designmodewidget.h"
#include "imageutils.h"
#include "materialbrowserview.h"
#include "qmldesignerplugin.h"
#include "qmlobjectnode.h"
#include "variantproperty.h"

#include <utils/qtcassert.h>

namespace QmlDesigner {

MaterialBrowserTexturesModel::MaterialBrowserTexturesModel(MaterialBrowserView *view, QObject *parent)
    : QAbstractListModel(parent)
    , m_view(view)
{
}

MaterialBrowserTexturesModel::~MaterialBrowserTexturesModel()
{
}

int MaterialBrowserTexturesModel::rowCount(const QModelIndex &) const
{
    return m_textureList.size();
}

QVariant MaterialBrowserTexturesModel::data(const QModelIndex &index, int role) const
{
    QTC_ASSERT(index.isValid() && index.row() < m_textureList.size(), return {});
    QTC_ASSERT(roleNames().contains(role), return {});

    if (role == RoleTexSource) {
        QString source = QmlObjectNode(m_textureList.at(index.row())).modelValue("source").toString();
        if (source.isEmpty())
            return {};
        if (Utils::FilePath::fromString(source).isAbsolutePath())
            return QVariant(source);
        return QVariant(QmlDesignerPlugin::instance()->documentManager().currentDesignDocument()
                        ->fileName().absolutePath().pathAppended(source).cleanPath().toString());
    }

    if (role == RoleTexVisible)
        return isVisible(index.row());

    if (role == RoleTexHasDynamicProps)
        return !m_textureList.at(index.row()).dynamicProperties().isEmpty();

    if (role == RoleTexInternalId)
        return m_textureList.at(index.row()).internalId();

    if (role == RoleTexId) {
        return m_textureList.at(index.row()).id();
    }

    if (role == RoleTexToolTip) {
        QString source = data(index, RoleTexSource).toString(); // absolute path
        if (source.isEmpty())
            return tr("Texture has no source image.");

        ModelNode texNode = m_textureList.at(index.row());
        QString info = ImageUtils::imageInfo(source);

        if (info.isEmpty())
            return tr("Texture has no data.");

        QString sourceRelative = QmlObjectNode(texNode).modelValue("source").toString();
        return QLatin1String("%1\n%2\n%3").arg(texNode.id(), sourceRelative, info);
    }

    return {};
}

bool MaterialBrowserTexturesModel::isVisible(int idx) const
{
    if (!isValidIndex(idx))
        return false;

    return m_searchText.isEmpty() || m_textureList.at(idx).variantProperty("source")
            .value().toString().contains(m_searchText, Qt::CaseInsensitive);
}

bool MaterialBrowserTexturesModel::isValidIndex(int idx) const
{
    return idx > -1 && idx < rowCount();
}

QHash<int, QByteArray> MaterialBrowserTexturesModel::roleNames() const
{
    static const QHash<int, QByteArray> roles {
        {RoleTexHasDynamicProps, "hasDynamicProperties"},
        {RoleTexInternalId,      "textureInternalId"},
        {RoleTexId,              "textureId"},
        {RoleTexSource,          "textureSource"},
        {RoleTexToolTip,         "textureToolTip"},
        {RoleTexVisible,         "textureVisible"}
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

    refreshSearch();
}

void MaterialBrowserTexturesModel::refreshSearch()
{
    bool isEmpty = false;

    // if selected texture goes invisible, select nearest one
    if (!isVisible(m_selectedIndex)) {
        int inc = 1;
        int incCap = m_textureList.size();
        while (!isEmpty && inc < incCap) {
            if (isVisible(m_selectedIndex - inc)) {
                selectTexture(m_selectedIndex - inc);
                break;
            } else if (isVisible(m_selectedIndex + inc)) {
                selectTexture(m_selectedIndex + inc);
                break;
            }
            ++inc;
            isEmpty = !isValidIndex(m_selectedIndex + inc)
                   && !isValidIndex(m_selectedIndex - inc);
        }
        if (!isVisible(m_selectedIndex)) // handles the case of a single item
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

void MaterialBrowserTexturesModel::updateTextureSource(const ModelNode &texture)
{
    int idx = textureIndex(texture);
    if (idx != -1)
        emit dataChanged(index(idx, 0), index(idx, 0), {RoleTexSource, RoleTexToolTip});
}

void MaterialBrowserTexturesModel::updateAllTexturesSources()
{
    emit dataChanged(index(0, 0), index(rowCount() - 1, 0), {RoleTexSource, RoleTexToolTip});
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

ModelNode MaterialBrowserTexturesModel::selectedTexture() const
{
    return textureAt(m_selectedIndex);
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

bool MaterialBrowserTexturesModel::hasSceneEnv() const
{
    return m_hasSceneEnv;
}

void MaterialBrowserTexturesModel::setHasSceneEnv(bool b)
{
    if (b == m_hasSceneEnv)
        return;

    m_hasSceneEnv = b;
    emit hasSceneEnvChanged();
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
    if (m_view && isValidIndex(idx)) {
        ModelNode node = m_textureList[idx];
        if (node.isValid()) {
            m_view->executeInTransaction(__FUNCTION__, [&] {
                node.destroy();
            });
        }
    }
}

void MaterialBrowserTexturesModel::setTextureId(int idx, const QString &newId)
{
    if (!isValidIndex(idx))
        return;

    ModelNode node = m_textureList[idx];
    if (!node.isValid())
        return;

    if (node.id() != newId) {
        node.setIdWithRefactoring(newId);
        emit dataChanged(index(idx, 0), index(idx, 0), {RoleTexId});
    }
}

void MaterialBrowserTexturesModel::applyToSelectedMaterial(qint64 internalId)
{
    int idx = m_textureIndexHash.value(internalId);
    if (idx != -1) {
        ModelNode tex = m_textureList.at(idx);
        emit applyToSelectedMaterialTriggered(tex);
    }
}

void MaterialBrowserTexturesModel::applyToSelectedModel(qint64 internalId)
{
    int idx = m_textureIndexHash.value(internalId);
    if (idx != -1) {
        ModelNode tex = m_textureList.at(idx);
        emit applyToSelectedModelTriggered(tex);
    }
}

void MaterialBrowserTexturesModel::openTextureEditor()
{
    QmlDesignerPlugin::instance()->mainWidget()->showDockWidget("TextureEditor", true);
}

void MaterialBrowserTexturesModel::updateSceneEnvState()
{
    emit updateSceneEnvStateRequested();
}

void MaterialBrowserTexturesModel::applyAsLightProbe(qint64 internalId)
{
    int idx = m_textureIndexHash.value(internalId);
    if (idx != -1) {
        ModelNode tex = m_textureList.at(idx);
        emit applyAsLightProbeRequested(tex);
    }
}

void MaterialBrowserTexturesModel::updateModelSelectionState()
{
    emit updateModelSelectionStateRequested();
}

} // namespace QmlDesigner
