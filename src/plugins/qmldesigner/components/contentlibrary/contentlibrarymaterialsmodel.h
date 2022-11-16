// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "nodemetainfo.h"

#include <QAbstractListModel>
#include <QJsonObject>

namespace QmlDesigner {

class ContentLibraryMaterial;
class ContentLibraryMaterialsCategory;

namespace Internal {
class ContentLibraryBundleImporter;
}

class ContentLibraryMaterialsModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(bool matBundleExists READ matBundleExists NOTIFY matBundleExistsChanged)
    Q_PROPERTY(bool isEmpty MEMBER m_isEmpty NOTIFY isEmptyChanged)
    Q_PROPERTY(bool hasQuick3DImport READ hasQuick3DImport WRITE setHasQuick3DImport NOTIFY hasQuick3DImportChanged)
    Q_PROPERTY(bool hasModelSelection READ hasModelSelection WRITE setHasModelSelection NOTIFY hasModelSelectionChanged)
    Q_PROPERTY(bool hasMaterialRoot READ hasMaterialRoot WRITE setHasMaterialRoot NOTIFY hasMaterialRootChanged)
    Q_PROPERTY(bool importerRunning MEMBER m_importerRunning NOTIFY importerRunningChanged)

public:
    ContentLibraryMaterialsModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    QHash<int, QByteArray> roleNames() const override;

    void setSearchText(const QString &searchText);
    void updateImportedState(const QStringList &importedMats);

    void setQuick3DImportVersion(int major, int minor);

    bool hasQuick3DImport() const;
    void setHasQuick3DImport(bool b);

    bool hasMaterialRoot() const;
    void setHasMaterialRoot(bool b);

    bool matBundleExists() const;

    bool hasModelSelection() const;
    void setHasModelSelection(bool b);

    void resetModel();

    Internal::ContentLibraryBundleImporter *bundleImporter() const;

    Q_INVOKABLE void applyToSelected(QmlDesigner::ContentLibraryMaterial *mat, bool add = false);
    Q_INVOKABLE void addToProject(QmlDesigner::ContentLibraryMaterial *mat);
    Q_INVOKABLE void removeFromProject(QmlDesigner::ContentLibraryMaterial *mat);

signals:
    void isEmptyChanged();
    void hasQuick3DImportChanged();
    void hasModelSelectionChanged();
    void hasMaterialRootChanged();
    void materialVisibleChanged();
    void applyToSelectedTriggered(QmlDesigner::ContentLibraryMaterial *mat, bool add = false);
    void bundleMaterialImported(const QmlDesigner::NodeMetaInfo &metaInfo);
    void bundleMaterialAboutToUnimport(const QmlDesigner::TypeName &type);
    void bundleMaterialUnimported(const QmlDesigner::NodeMetaInfo &metaInfo);
    void importerRunningChanged();
    void matBundleExistsChanged();

private:
    void loadMaterialBundle();
    bool isValidIndex(int idx) const;

    QString m_searchText;
    QList<ContentLibraryMaterialsCategory *> m_bundleCategories;
    QJsonObject m_matBundleObj;
    Internal::ContentLibraryBundleImporter *m_importer = nullptr;

    bool m_isEmpty = true;
    bool m_hasMaterialRoot = false;
    bool m_hasQuick3DImport = false;
    bool m_matBundleLoaded = false;
    bool m_hasModelSelection = false;
    bool m_probeMatBundleDir = false;
    bool m_importerRunning = false;

    int m_quick3dMajorVersion = -1;
    int m_quick3dMinorVersion = -1;
};

} // namespace QmlDesigner
