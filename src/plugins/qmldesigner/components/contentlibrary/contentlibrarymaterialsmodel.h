// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmldesignerutils/version.h>
#include <utils/filepath.h>

#include <QAbstractListModel>
#include <QJsonObject>

namespace QmlDesigner {

class ContentLibraryMaterial;
class ContentLibraryMaterialsCategory;
class ContentLibraryWidget;

class ContentLibraryMaterialsModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(bool bundleExists READ bundleExists NOTIFY bundleExistsChanged)
    Q_PROPERTY(bool isEmpty MEMBER m_isEmpty NOTIFY isEmptyChanged)
    Q_PROPERTY(bool hasRequiredQuick3DImport READ hasRequiredQuick3DImport NOTIFY hasRequiredQuick3DImportChanged)
    Q_PROPERTY(QString baseWebUrl MEMBER m_baseUrl CONSTANT)
    Q_PROPERTY(QString bundlePath READ bundlePath CONSTANT)

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
    bool bundleExists() const;

    QString bundlePath() const;

    void resetModel();
    void updateIsEmpty();
    void loadBundle();

    Q_INVOKABLE void applyToSelected(QmlDesigner::ContentLibraryMaterial *mat, bool add = false);
    Q_INVOKABLE void addToProject(QmlDesigner::ContentLibraryMaterial *mat);
    Q_INVOKABLE void removeFromProject(QmlDesigner::ContentLibraryMaterial *mat);
    Q_INVOKABLE bool isMaterialDownloaded(QmlDesigner::ContentLibraryMaterial *mat) const;

    QString bundleId() const;
    void setBundleExists(bool exists);

signals:
    void isEmptyChanged();
    void hasRequiredQuick3DImportChanged();
    void materialVisibleChanged();
    void applyToSelectedTriggered(QmlDesigner::ContentLibraryMaterial *mat, bool add = false);
    void bundleExistsChanged();

private:
    void loadMaterialBundle(bool forceReload = false);
    bool fetchBundleIcons();
    bool fetchBundleJsonFile();
    bool isValidIndex(int idx) const;
    void downloadSharedFiles();

    ContentLibraryWidget *m_widget = nullptr;
    QString m_searchText;
    QString m_bundleId;
    QStringList m_bundleSharedFiles;
    QList<ContentLibraryMaterialsCategory *> m_bundleCategories;
    QJsonObject m_bundleObj;

    bool m_isEmpty = true;
    bool m_bundleExists = false;

    Version m_quick3dVersion;
    Utils::FilePath m_bundlePath;
    QString m_baseUrl;
};

} // namespace QmlDesigner
