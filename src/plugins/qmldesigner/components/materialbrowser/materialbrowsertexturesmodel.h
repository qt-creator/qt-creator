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
    Q_PROPERTY(int selectedIndex MEMBER m_selectedIndex NOTIFY selectedIndexChanged)
    Q_PROPERTY(bool hasSingleModelSelection READ hasSingleModelSelection NOTIFY hasSingleModelSelectionChanged)
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
    void deleteSelectedTexture();
    void updateSelectedTexture();
    void updateTextureSource(const ModelNode &texture);
    void updateAllTexturesSources();
    int textureIndex(const ModelNode &texture) const;
    ModelNode textureAt(int idx) const;
    ModelNode selectedTexture() const;

    bool hasSingleModelSelection() const;
    void setHasSingleModelSelection(bool b);

    bool hasSceneEnv() const;
    void setHasSceneEnv(bool b);

    bool isEmpty() const { return m_isEmpty; }

    void resetModel();

    Q_INVOKABLE void selectTexture(int idx, bool force = false);
    Q_INVOKABLE void addNewTexture();
    Q_INVOKABLE void duplicateTexture(int idx);
    Q_INVOKABLE void deleteTexture(int idx);
    Q_INVOKABLE void setTextureId(int idx, const QString &newId);
    Q_INVOKABLE void applyToSelectedMaterial(qint64 internalId);
    Q_INVOKABLE void applyToSelectedModel(qint64 internalId);
    Q_INVOKABLE void openTextureEditor();
    Q_INVOKABLE void updateSceneEnvState();
    Q_INVOKABLE void updateModelSelectionState();
    Q_INVOKABLE void applyAsLightProbe(qint64 internalId);
    Q_INVOKABLE bool isVisible(int idx) const;

signals:
    void isEmptyChanged();
    void hasSingleModelSelectionChanged();
    void selectedIndexChanged(int idx);
    void duplicateTextureTriggered(const QmlDesigner::ModelNode &texture);
    void applyToSelectedMaterialTriggered(const QmlDesigner::ModelNode &texture);
    void applyToSelectedModelTriggered(const QmlDesigner::ModelNode &texture);
    void addNewTextureTriggered();
    void updateSceneEnvStateRequested();
    void updateModelSelectionStateRequested();
    void hasSceneEnvChanged();
    void applyAsLightProbeRequested(const QmlDesigner::ModelNode &texture);

private:
    bool isValidIndex(int idx) const;

    QString m_searchText;
    QList<ModelNode> m_textureList;
    QHash<qint32, int> m_textureIndexHash; // internalId -> index

    int m_selectedIndex = 0;
    bool m_isEmpty = true;
    bool m_hasSingleModelSelection = false;
    bool m_hasSceneEnv = false;

    QPointer<MaterialBrowserView> m_view;

    enum {
        RoleTexHasDynamicProps = Qt::UserRole + 1,
        RoleTexInternalId,
        RoleTexId,
        RoleTexSource,
        RoleTexToolTip,
        RoleTexVisible
    };
};

} // namespace QmlDesigner
