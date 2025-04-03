// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "modelnode.h"

#include <QAbstractListModel>
#include <QObject>

namespace QmlDesigner {

class MaterialBrowserView;

class MaterialBrowserTexturesModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(bool isEmpty MEMBER m_isEmpty NOTIFY isEmptyChanged)
    Q_PROPERTY(bool hasSingleModelSelection READ hasSingleModelSelection NOTIFY hasSingleModelSelectionChanged)
    Q_PROPERTY(bool onlyMaterialsSelected READ onlyMaterialsSelected NOTIFY onlyMaterialsSelectedChanged)
    Q_PROPERTY(bool hasSceneEnv READ hasSceneEnv NOTIFY hasSceneEnvChanged)

public:
    MaterialBrowserTexturesModel(MaterialBrowserView *view, QObject *parent = nullptr);
    ~MaterialBrowserTexturesModel() override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setSearchText(const QString &searchText);
    void refreshSearch();

    QList<ModelNode> textures() const;
    void setTextures(const QList<ModelNode> &textures);
    void removeTexture(const ModelNode &texture);
    void deleteSelectedTextures();
    void updateTextureSource(const ModelNode &texture);
    void updateTextureId(const ModelNode &texture);
    void updateTextureName(const ModelNode &texture);
    void updateAllTexturesSources();
    void notifySelectionChanges(const QList<ModelNode> &selectedNodes,
                                const QList<ModelNode> &deselectedNodes);
    int textureIndex(const ModelNode &texture) const;
    ModelNode textureAt(int idx) const;

    bool hasSingleModelSelection() const;
    void setHasSingleModelSelection(bool b);

    bool onlyMaterialsSelected() const;
    bool hasSceneEnv() const;
    void setHasSceneEnv(bool b);

    bool isEmpty() const { return m_isEmpty; }

    void resetModel();

    Q_INVOKABLE void selectTexture(int idx, bool appendTxt = false);
    Q_INVOKABLE void addNewTexture();
    Q_INVOKABLE void duplicateTexture(int idx);
    Q_INVOKABLE void deleteTexture(int idx);
    Q_INVOKABLE void setTextureName(int idx, const QString &newName);
    Q_INVOKABLE void applyToSelectedMaterial(qint64 internalId);
    Q_INVOKABLE void applyToSelectedModel(qint64 internalId);
    Q_INVOKABLE void updateSceneEnvState();
    Q_INVOKABLE void updateSelectionState();
    Q_INVOKABLE void applyAsLightProbe(qint64 internalId);
    Q_INVOKABLE bool isVisible(int idx) const;

signals:
    void isEmptyChanged();
    void hasSingleModelSelectionChanged();
    void onlyMaterialsSelectedChanged();
    void duplicateTextureTriggered(const QmlDesigner::ModelNode &texture);
    void applyToSelectedMaterialTriggered(const QmlDesigner::ModelNode &texture);
    void applyToSelectedModelTriggered(const QmlDesigner::ModelNode &texture);
    void addNewTextureTriggered();
    void updateSceneEnvStateRequested();
    void hasSceneEnvChanged();
    void applyAsLightProbeRequested(const QmlDesigner::ModelNode &texture);

private:
    bool isValidIndex(int idx) const;
    void setOnlyMaterialsSelected(bool value);

    QString m_searchText;
    QList<ModelNode> m_textureList;
    QHash<qint32, int> m_textureIndexHash; // internalId -> index

    bool m_isEmpty = true;
    bool m_hasSingleModelSelection = false;
    bool m_onlyMaterialsSelected = false;
    bool m_hasSceneEnv = false;

    QPointer<MaterialBrowserView> m_view;

    enum Roles {
        RoleTexHasDynamicProps = Qt::UserRole + 1,
        RoleTexInternalId,
        RoleTexName,
        RoleTexSource,
        RoleTexToolTip,
        RoleMatchedSearch,
        RoleTexSelected,
    };
};

} // namespace QmlDesigner
