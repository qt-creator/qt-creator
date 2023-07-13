// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "nodemetainfo.h"

#include <QAbstractListModel>
#include <QDir>
#include <QJsonObject>

namespace QmlDesigner {

class ContentLibraryEffect;
class ContentLibraryEffectsCategory;
class ContentLibraryWidget;

namespace Internal {
class ContentLibraryBundleImporter;
}

class ContentLibraryEffectsModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(bool bundleExists READ bundleExists NOTIFY bundleExistsChanged)
    Q_PROPERTY(bool isEmpty MEMBER m_isEmpty NOTIFY isEmptyChanged)
    Q_PROPERTY(bool hasRequiredQuick3DImport READ hasRequiredQuick3DImport NOTIFY hasRequiredQuick3DImportChanged)
    Q_PROPERTY(bool importerRunning MEMBER m_importerRunning NOTIFY importerRunningChanged)

public:
    ContentLibraryEffectsModel(ContentLibraryWidget *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    QHash<int, QByteArray> roleNames() const override;

    void loadBundle();
    void setSearchText(const QString &searchText);
    void updateImportedState(const QStringList &importedItems);

    void setQuick3DImportVersion(int major, int minor);

    bool hasRequiredQuick3DImport() const;

    bool bundleExists() const;

    void resetModel();
    void updateIsEmpty();

    Internal::ContentLibraryBundleImporter *bundleImporter() const;

    Q_INVOKABLE void addInstance(QmlDesigner::ContentLibraryEffect *bundleItem);
    Q_INVOKABLE void removeFromProject(QmlDesigner::ContentLibraryEffect *bundleItem);

signals:
    void isEmptyChanged();
    void hasRequiredQuick3DImportChanged();
#ifdef QDS_USE_PROJECTSTORAGE
    void bundleItemImported(const QmlDesigner::TypeName &typeName);
#else
    void bundleItemImported(const QmlDesigner::NodeMetaInfo &metaInfo);
#endif
    void bundleItemAboutToUnimport(const QmlDesigner::TypeName &type);
    void bundleItemUnimported(const QmlDesigner::NodeMetaInfo &metaInfo);
    void importerRunningChanged();
    void bundleExistsChanged();

private:
    bool isValidIndex(int idx) const;
    void createImporter(const QString &bundlePath, const QString &bundleId,
                        const QStringList &sharedFiles);

    ContentLibraryWidget *m_widget = nullptr;
    QString m_searchText;
    QList<ContentLibraryEffectsCategory *> m_bundleCategories;
    QJsonObject m_bundleObj;
    Internal::ContentLibraryBundleImporter *m_importer = nullptr;

    bool m_isEmpty = true;
    bool m_bundleExists = false;
    bool m_probeBundleDir = false;
    bool m_importerRunning = false;

    int m_quick3dMajorVersion = -1;
    int m_quick3dMinorVersion = -1;

    QString m_importerBundlePath;
    QString m_importerBundleId;
    QStringList m_importerSharedFiles;
};

} // namespace QmlDesigner
