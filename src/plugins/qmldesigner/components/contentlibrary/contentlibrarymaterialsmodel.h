// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "nodemetainfo.h"

#include <QAbstractListModel>
#include <QDir>
#include <QJsonObject>

namespace QmlDesigner {

class ContentLibraryMaterial;
class ContentLibraryMaterialsCategory;
class ContentLibraryWidget;

namespace Internal {
class ContentLibraryBundleImporter;
}

class ContentLibraryMaterialsModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(bool matBundleExists READ matBundleExists NOTIFY matBundleExistsChanged)
    Q_PROPERTY(bool isEmpty MEMBER m_isEmpty NOTIFY isEmptyChanged)
    Q_PROPERTY(bool hasRequiredQuick3DImport READ hasRequiredQuick3DImport NOTIFY hasRequiredQuick3DImportChanged)
    Q_PROPERTY(bool hasModelSelection READ hasModelSelection NOTIFY hasModelSelectionChanged)
    Q_PROPERTY(bool importerRunning MEMBER m_importerRunning NOTIFY importerRunningChanged)

public:
    ContentLibraryMaterialsModel(ContentLibraryWidget *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    QHash<int, QByteArray> roleNames() const override;

    void setSearchText(const QString &searchText);
    void updateImportedState(const QStringList &importedMats);

    void setQuick3DImportVersion(int major, int minor);

    bool hasRequiredQuick3DImport() const;

    bool matBundleExists() const;

    bool hasModelSelection() const;
    void setHasModelSelection(bool b);

    void resetModel();
    void updateIsEmpty();

    Internal::ContentLibraryBundleImporter *bundleImporter() const;

    Q_INVOKABLE void applyToSelected(QmlDesigner::ContentLibraryMaterial *mat, bool add = false);
    Q_INVOKABLE void addToProject(QmlDesigner::ContentLibraryMaterial *mat);
    Q_INVOKABLE void removeFromProject(QmlDesigner::ContentLibraryMaterial *mat);

signals:
    void isEmptyChanged();
    void hasRequiredQuick3DImportChanged();
    void hasModelSelectionChanged();
    void materialVisibleChanged();
    void applyToSelectedTriggered(QmlDesigner::ContentLibraryMaterial *mat, bool add = false);
#ifdef QDS_USE_PROJECTSTORAGE
    void bundleMaterialImported(const QmlDesigner::TypeName &typeName);
#else
    void bundleMaterialImported(const QmlDesigner::NodeMetaInfo &metaInfo);
#endif
    void bundleMaterialAboutToUnimport(const QmlDesigner::TypeName &type);
    void bundleMaterialUnimported(const QmlDesigner::NodeMetaInfo &metaInfo);
    void importerRunningChanged();
    void matBundleExistsChanged();

private:
    void loadMaterialBundle(const QDir &matBundleDir);
    bool fetchBundleIcons(const QDir &bundleDir);
    bool fetchBundleMetadata(const QDir &bundleDir);
    bool isValidIndex(int idx) const;
    void downloadSharedFiles(const QDir &targetDir, const QStringList &files);
    void createImporter(const QString &bundlePath, const QString &bundleId,
                        const QStringList &sharedFiles);

    ContentLibraryWidget *m_widget = nullptr;
    QString m_searchText;
    QList<ContentLibraryMaterialsCategory *> m_bundleCategories;
    QJsonObject m_matBundleObj;
    Internal::ContentLibraryBundleImporter *m_importer = nullptr;

    bool m_isEmpty = true;
    bool m_matBundleExists = false;
    bool m_hasModelSelection = false;
    bool m_importerRunning = false;

    int m_quick3dMajorVersion = -1;
    int m_quick3dMinorVersion = -1;

    QString m_downloadPath;
    QString m_baseUrl;

    QString m_importerBundlePath;
    QString m_importerBundleId;
    QStringList m_importerSharedFiles;
};

} // namespace QmlDesigner
