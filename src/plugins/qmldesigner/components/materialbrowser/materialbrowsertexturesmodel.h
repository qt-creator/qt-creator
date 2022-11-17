// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <modelnode.h>

#include <QAbstractListModel>
#include <QObject>

namespace QmlDesigner {

class MaterialBrowserTexturesModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(bool isEmpty MEMBER m_isEmpty NOTIFY isEmptyChanged)
    Q_PROPERTY(int selectedIndex MEMBER m_selectedIndex NOTIFY selectedIndexChanged)
    Q_PROPERTY(bool hasSingleModelSelection READ hasSingleModelSelection
               WRITE setHasSingleModelSelection NOTIFY hasSingleModelSelectionChanged)

public:
    MaterialBrowserTexturesModel(QObject *parent = nullptr);
    ~MaterialBrowserTexturesModel() override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setSearchText(const QString &searchText);

    QList<ModelNode> textures() const;
    void setTextures(const QList<ModelNode> &textures);
    void removeTexture(const ModelNode &texture);
    void deleteSelectedTexture();
    void updateSelectedTexture();
    void updateTextureSource(const ModelNode &texture);
    int textureIndex(const ModelNode &material) const;
    ModelNode textureAt(int idx) const;

    bool hasSingleModelSelection() const;
    void setHasSingleModelSelection(bool b);

    void resetModel();

    Q_INVOKABLE void selectTexture(int idx, bool force = false);
    Q_INVOKABLE void addNewTexture();
    Q_INVOKABLE void duplicateTexture(int idx);
    Q_INVOKABLE void deleteTexture(int idx);
    Q_INVOKABLE void applyToSelectedMaterial(qint64 internalId);
    Q_INVOKABLE void applyToSelectedModel(qint64 internalId);

signals:
    void isEmptyChanged();
    void hasSingleModelSelectionChanged();
    void selectedIndexChanged(int idx);
    void duplicateTextureTriggered(const QmlDesigner::ModelNode &texture);
    void applyToSelectedMaterialTriggered(const QmlDesigner::ModelNode &texture);
    void applyToSelectedModelTriggered(const QmlDesigner::ModelNode &texture);
    void addNewTextureTriggered();

private:
    bool isTextureVisible(int idx) const;
    bool isValidIndex(int idx) const;

    QString m_searchText;
    QList<ModelNode> m_textureList;
    ModelNode m_copiedMaterial;
    QHash<qint32, int> m_textureIndexHash; // internalId -> index

    int m_selectedIndex = 0;
    bool m_isEmpty = true;
    bool m_hasSingleModelSelection = false;
};

} // namespace QmlDesigner
