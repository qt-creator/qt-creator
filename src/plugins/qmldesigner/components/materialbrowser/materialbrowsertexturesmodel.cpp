// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "materialbrowsertexturesmodel.h"

#include "designmodewidget.h"
#include "imageutils.h"
#include "materialbrowserview.h"
#include "qmldesignerplugin.h"
#include "qmlobjectnode.h"
#include "variantproperty.h"

#include <utils3d.h>

#include <utils/qtcassert.h>

namespace QmlDesigner {

static bool isMaterial(const ModelNode &node)
{
    return node.metaInfo().isQtQuick3DMaterial();
}

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

    switch (role) {
    case RoleMatchedSearch:
        return isVisible(index.row());
    case RoleTexSelected:
        return m_textureList.at(index.row()).isSelected();
    case RoleTexHasDynamicProps:
        return !m_textureList.at(index.row()).dynamicProperties().isEmpty();
    case RoleTexInternalId:
        return m_textureList.at(index.row()).internalId();
    case RoleTexName:
        return m_textureList.at(index.row()).variantProperty("objectName").value();
    case RoleTexSource: {
        QString source = QmlObjectNode(m_textureList.at(index.row())).modelValue("source").toString();
        if (source.isEmpty())
            return {};
        if (Utils::FilePath::fromString(source).isAbsolutePath())
            return QVariant(source);
        return QVariant(QmlDesignerPlugin::instance()
                            ->documentManager()
                            .currentDesignDocument()
                            ->fileName()
                            .absolutePath()
                            .pathAppended(source)
                            .toFSPathString());
    };
    case RoleTexToolTip: {
        QString source = data(index, RoleTexSource).toString(); // absolute path
        if (source.isEmpty())
            return tr("Texture has no source image.");

        ModelNode texNode = m_textureList.at(index.row());
        QString info = ImageUtils::imageInfoString(source);

        if (info.isEmpty())
            return tr("Texture has no data.");

        QString textName = data(index, RoleTexName).toString();
        QString sourceRelative = QmlObjectNode(texNode).modelValue("source").toString();
        return QLatin1String("%1 (%2)\n%3\n%4").arg(textName, texNode.id(), sourceRelative, info);
    };
    default:
        return {};
    }
}

bool MaterialBrowserTexturesModel::isVisible(int idx) const
{
    if (!isValidIndex(idx))
        return false;

    if (m_searchText.isEmpty())
        return true;

    const ModelNode &texture = m_textureList.at(idx);

    auto propertyHasMatch = [&](const PropertyName &property) -> bool {
        return texture.variantProperty(property).value().toString().contains(m_searchText,
                                                                             Qt::CaseInsensitive);
    };

    return propertyHasMatch("objectName") || propertyHasMatch("source");
}

bool MaterialBrowserTexturesModel::isValidIndex(int idx) const
{
    return idx > -1 && idx < rowCount();
}

void MaterialBrowserTexturesModel::setOnlyMaterialsSelected(bool value)
{
    if (m_onlyMaterialsSelected == value)
        return;

    m_onlyMaterialsSelected = value;
    emit onlyMaterialsSelectedChanged();
}

QHash<int, QByteArray> MaterialBrowserTexturesModel::roleNames() const
{
    static const QHash<int, QByteArray> roles{
        {RoleTexHasDynamicProps, "hasDynamicProperties"},
        {RoleTexInternalId, "textureInternalId"},
        {RoleTexName, "textureName"},
        {RoleTexSource, "textureSource"},
        {RoleTexToolTip, "textureToolTip"},
        {RoleMatchedSearch, "textureMatchedSearch"},
        {RoleTexSelected, "textureSelected"},
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
    bool isEmpty = true;

    for (int i = 0; i < m_textureList.size(); ++i) {
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

void MaterialBrowserTexturesModel::deleteSelectedTextures()
{
    m_view->executeInTransaction(__FUNCTION__, [this] {
        QStack<int> selectedIndexes;
        for (int i = 0; i < m_textureList.size(); ++i) {
            if (m_textureList.at(i).isSelected())
                selectedIndexes << i;
        }

        while (!selectedIndexes.isEmpty())
            deleteTexture(selectedIndexes.pop());
    });
}

void MaterialBrowserTexturesModel::updateTextureSource(const ModelNode &texture)
{
    int idx = textureIndex(texture);
    if (idx != -1)
        emit dataChanged(index(idx, 0), index(idx, 0), {RoleTexSource, RoleTexToolTip});
}

void MaterialBrowserTexturesModel::updateTextureId(const ModelNode &texture)
{
    int idx = textureIndex(texture);
    if (idx != -1)
        emit dataChanged(index(idx, 0), index(idx, 0), {RoleTexToolTip});
}

void MaterialBrowserTexturesModel::updateTextureName(const ModelNode &texture)
{
    int idx = textureIndex(texture);
    if (idx != -1)
        emit dataChanged(index(idx, 0), index(idx, 0), {RoleTexName, RoleTexToolTip});
}

void MaterialBrowserTexturesModel::updateAllTexturesSources()
{
    emit dataChanged(index(0, 0), index(rowCount() - 1, 0), {RoleTexSource, RoleTexToolTip});
}

void MaterialBrowserTexturesModel::notifySelectionChanges(const QList<ModelNode> &selectedNodes,
                                                          const QList<ModelNode> &deselectedNodes)
{
    QList<int> indices;
    indices.reserve(selectedNodes.size() + deselectedNodes.size());
    for (const ModelNode &node : selectedNodes)
        indices.append(textureIndex(node));

    for (const ModelNode &node : deselectedNodes)
        indices.append(textureIndex(node));

    using Bound = QPair<int, int>;
    const QList<Bound> &bounds = MaterialBrowserView::getSortedBounds(indices);

    for (const Bound &bound : bounds)
        emit dataChanged(index(bound.first), index(bound.second), {Roles::RoleTexSelected});
}

int MaterialBrowserTexturesModel::textureIndex(const ModelNode &texture) const
{
    return m_textureIndexHash.value(texture.internalId(), -1);
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

bool MaterialBrowserTexturesModel::onlyMaterialsSelected() const
{
    return m_onlyMaterialsSelected;
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

void MaterialBrowserTexturesModel::selectTexture(int idx, bool appendTxt)
{
    if (!isValidIndex(idx))
        return;

    ModelNode texture = m_textureList.at(idx);
    QTC_ASSERT(texture, return);

    if (appendTxt)
        texture.view()->selectModelNode(texture);
    else
        texture.selectNode();
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

void MaterialBrowserTexturesModel::setTextureName(int idx, const QString &newName)
{
    if (!isValidIndex(idx))
        return;

    QmlObjectNode(m_textureList[idx]).setNameAndId(newName, "texture");
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

void MaterialBrowserTexturesModel::updateSceneEnvState()
{
    emit updateSceneEnvStateRequested();
}

void MaterialBrowserTexturesModel::updateSelectionState()
{
    setHasSingleModelSelection(
        m_view->hasSingleSelectedModelNode()
        && Utils3D::getMaterialOfModel(m_view->singleSelectedModelNode()).isValid());

    setOnlyMaterialsSelected(Utils::allOf(m_view->selectedModelNodes(), isMaterial));
}

void MaterialBrowserTexturesModel::applyAsLightProbe(qint64 internalId)
{
    int idx = m_textureIndexHash.value(internalId);
    if (idx != -1) {
        ModelNode tex = m_textureList.at(idx);
        emit applyAsLightProbeRequested(tex);
    }
}

} // namespace QmlDesigner
