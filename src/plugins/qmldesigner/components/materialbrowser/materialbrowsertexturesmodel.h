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
    int textureIndex(const ModelNode &material) const;
    ModelNode textureAt(int idx) const;

    void resetModel();

    Q_INVOKABLE void selectTexture(int idx, bool force = false);
    Q_INVOKABLE void duplicateTexture(int idx);
    Q_INVOKABLE void deleteTexture(int idx);

signals:
    void isEmptyChanged();
    void materialSectionsChanged();
    void selectedIndexChanged(int idx);
    void duplicateTextureTriggered(const QmlDesigner::ModelNode &material);

private:
    bool isTextureVisible(int idx) const;
    bool isValidIndex(int idx) const;

    QString m_searchText;
    QList<ModelNode> m_textureList;
    ModelNode m_copiedMaterial;
    QHash<qint32, int> m_textureIndexHash; // internalId -> index

    int m_selectedIndex = 0;
    bool m_isEmpty = true;
};

} // namespace QmlDesigner
