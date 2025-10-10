// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QAbstractListModel>

namespace QmlDesigner {

class ContentLibraryTexturesCategory;

class ContentLibraryTexturesModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(bool bundleExists READ bundleExists NOTIFY bundleChanged)
    Q_PROPERTY(bool isEmpty MEMBER m_isEmpty NOTIFY isEmptyChanged)
    Q_PROPERTY(bool hasSceneEnv READ hasSceneEnv NOTIFY hasSceneEnvChanged)

public:
    ContentLibraryTexturesModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    QHash<int, QByteArray> roleNames() const override;

    void setSearchText(const QString &searchText);
    void setModifiedFileEntries(const QVariantMap &files);
    void setNewFileEntries(const QStringList &newFiles);
    QString removeModifiedFileEntry(const QString &key);
    void markTextureHasNoUpdates(const QString &subcategory, const QString &textureKey);

    bool bundleExists() const;

    bool hasSceneEnv() const;
    void setHasSceneEnv(bool b);

    void resetModel();
    void loadTextureBundle(const QString &textureBundleUrl, const QString &textureBundlePath,
                           const QString &textureBundleIconPath, const QVariantMap &metaData);


signals:
    void isEmptyChanged();
    void materialVisibleChanged();
    void hasSceneEnvChanged();
    void bundleChanged();

private:
    bool isValidIndex(int idx) const;
    void updateIsEmpty();

    QString m_searchText;
    QList<ContentLibraryTexturesCategory *> m_bundleCategories;
    QVariantMap m_modifiedFiles;
    QSet<QString> m_newFiles;

    bool m_isEmpty = true;
    bool m_hasSceneEnv = false;
    bool m_hasModelSelection = false;
};

} // namespace QmlDesigner
