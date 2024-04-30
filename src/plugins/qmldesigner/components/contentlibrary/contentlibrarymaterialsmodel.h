// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QAbstractListModel>
#include <QJsonObject>

QT_FORWARD_DECLARE_CLASS(QDir)

namespace QmlDesigner {

class ContentLibraryMaterial;
class ContentLibraryMaterialsCategory;
class ContentLibraryWidget;

class ContentLibraryMaterialsModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(bool matBundleExists READ matBundleExists NOTIFY matBundleExistsChanged)
    Q_PROPERTY(bool isEmpty MEMBER m_isEmpty NOTIFY isEmptyChanged)
    Q_PROPERTY(bool hasRequiredQuick3DImport READ hasRequiredQuick3DImport NOTIFY hasRequiredQuick3DImportChanged)
    Q_PROPERTY(bool hasModelSelection READ hasModelSelection NOTIFY hasModelSelectionChanged)

public:
    ContentLibraryMaterialsModel(ContentLibraryWidget *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    QHash<int, QByteArray> roleNames() const override;

    void setSearchText(const QString &searchText);
    void updateImportedState(const QStringList &importedItems);

    void setQuick3DImportVersion(int major, int minor);

    bool hasRequiredQuick3DImport() const;

    bool matBundleExists() const;

    bool hasModelSelection() const;
    void setHasModelSelection(bool b);

    void resetModel();
    void updateIsEmpty();
    void loadBundle();

    Q_INVOKABLE void applyToSelected(QmlDesigner::ContentLibraryMaterial *mat, bool add = false);
    Q_INVOKABLE void addToProject(QmlDesigner::ContentLibraryMaterial *mat);
    Q_INVOKABLE void removeFromProject(QmlDesigner::ContentLibraryMaterial *mat);

    QString bundleId() const;

signals:
    void isEmptyChanged();
    void hasRequiredQuick3DImportChanged();
    void hasModelSelectionChanged();
    void materialVisibleChanged();
    void applyToSelectedTriggered(QmlDesigner::ContentLibraryMaterial *mat, bool add = false);
    void matBundleExistsChanged();

private:
    void loadMaterialBundle(const QDir &matBundleDir);
    bool fetchBundleIcons(const QDir &bundleDir);
    bool fetchBundleMetadata(const QDir &bundleDir);
    bool isValidIndex(int idx) const;
    void downloadSharedFiles(const QDir &targetDir, const QStringList &files);

    ContentLibraryWidget *m_widget = nullptr;
    QString m_searchText;
    QString m_bundleId;
    QStringList m_bundleSharedFiles;
    QList<ContentLibraryMaterialsCategory *> m_bundleCategories;
    QJsonObject m_matBundleObj;

    bool m_isEmpty = true;
    bool m_matBundleExists = false;
    bool m_hasModelSelection = false;

    int m_quick3dMajorVersion = -1;
    int m_quick3dMinorVersion = -1;

    QString m_downloadPath;
    QString m_baseUrl;
};

} // namespace QmlDesigner
