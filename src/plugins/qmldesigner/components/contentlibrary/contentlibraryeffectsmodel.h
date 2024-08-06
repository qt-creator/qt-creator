// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QAbstractListModel>
#include <QJsonObject>

namespace QmlDesigner {

class ContentLibraryItem;
class ContentLibraryEffectsCategory;
class ContentLibraryWidget;

class ContentLibraryEffectsModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(bool bundleExists READ bundleExists NOTIFY bundleExistsChanged)
    Q_PROPERTY(bool isEmpty MEMBER m_isEmpty NOTIFY isEmptyChanged)
    Q_PROPERTY(bool hasRequiredQuick3DImport READ hasRequiredQuick3DImport NOTIFY hasRequiredQuick3DImportChanged)

public:
    ContentLibraryEffectsModel(ContentLibraryWidget *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    QHash<int, QByteArray> roleNames() const override;

    void loadBundle(bool force = false);
    void setSearchText(const QString &searchText);
    void updateImportedState(const QStringList &importedItems);

    void setQuick3DImportVersion(int major, int minor);

    bool hasRequiredQuick3DImport() const;

    bool bundleExists() const;

    void resetModel();
    void updateIsEmpty();

    Q_INVOKABLE void addInstance(QmlDesigner::ContentLibraryItem *bundleItem);
    Q_INVOKABLE void removeFromProject(QmlDesigner::ContentLibraryItem *bundleItem);

    QString bundleId() const;

signals:
    void isEmptyChanged();
    void hasRequiredQuick3DImportChanged();
    void bundleExistsChanged();

private:
    bool isValidIndex(int idx) const;

    ContentLibraryWidget *m_widget = nullptr;
    QString m_searchText;
    QString m_bundlePath;
    QString m_bundleId;
    QStringList m_bundleSharedFiles;
    QList<ContentLibraryEffectsCategory *> m_bundleCategories;
    QJsonObject m_bundleObj;

    bool m_isEmpty = true;
    bool m_bundleExists = false;
    bool m_probeBundleDir = false;

    int m_quick3dMajorVersion = -1;
    int m_quick3dMinorVersion = -1;
};

} // namespace QmlDesigner
