// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "modelfwd.h"

#include <QAbstractListModel>
#include <QJsonObject>

QT_FORWARD_DECLARE_CLASS(QUrl)

namespace QmlDesigner {

class ContentLibraryEffect;
class ContentLibraryMaterial;
class ContentLibraryTexture;
class ContentLibraryWidget;
class NodeMetaInfo;

namespace Internal {
class ContentLibraryBundleImporter;
}

class ContentLibraryUserModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(bool matBundleExists READ matBundleExists NOTIFY matBundleExistsChanged)
    Q_PROPERTY(bool isEmpty MEMBER m_isEmpty NOTIFY isEmptyChanged)
    Q_PROPERTY(bool hasRequiredQuick3DImport READ hasRequiredQuick3DImport NOTIFY hasRequiredQuick3DImportChanged)
    Q_PROPERTY(bool hasModelSelection READ hasModelSelection NOTIFY hasModelSelectionChanged)
    Q_PROPERTY(bool importerRunning MEMBER m_importerRunning NOTIFY importerRunningChanged)
    Q_PROPERTY(QList<ContentLibraryMaterial *> userMaterials MEMBER m_userMaterials NOTIFY userMaterialsChanged)
    Q_PROPERTY(QList<ContentLibraryTexture *> userTextures MEMBER m_userTextures NOTIFY userTexturesChanged)
    Q_PROPERTY(QList<ContentLibraryEffect *> user3DItems MEMBER m_user3DItems NOTIFY user3DItemsChanged)
    Q_PROPERTY(QList<ContentLibraryEffect *> userEffects MEMBER m_userEffects NOTIFY userEffectsChanged)

public:
    ContentLibraryUserModel(ContentLibraryWidget *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setSearchText(const QString &searchText);
    void updateImportedState(const QStringList &importedMats);

    QPair<QString, QString> getUniqueLibMaterialNameAndQml(const QString &matName) const;
    TypeName qmlToModule(const QString &qmlName) const;

    void setQuick3DImportVersion(int major, int minor);

    bool hasRequiredQuick3DImport() const;

    bool matBundleExists() const;

    bool hasModelSelection() const;
    void setHasModelSelection(bool b);

    void resetModel();
    void updateIsEmpty();

    void addMaterial(const QString &name, const QString &qml, const QUrl &icon, const QStringList &files);
    void addTextures(const QStringList &paths);

    void setBundleObj(const QJsonObject &newBundleObj);
    QJsonObject &bundleJsonObjectRef();

    Internal::ContentLibraryBundleImporter *bundleImporter() const;

    Q_INVOKABLE void applyToSelected(QmlDesigner::ContentLibraryMaterial *mat, bool add = false);
    Q_INVOKABLE void addToProject(QmlDesigner::ContentLibraryMaterial *mat);
    Q_INVOKABLE void removeFromProject(QmlDesigner::ContentLibraryMaterial *mat);

signals:
    void isEmptyChanged();
    void hasRequiredQuick3DImportChanged();
    void hasModelSelectionChanged();
    void userMaterialsChanged();
    void userTexturesChanged();
    void user3DItemsChanged();
    void userEffectsChanged();

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
    void loadMaterialBundle();
    void loadTextureBundle();
    bool isValidIndex(int idx) const;
    void createImporter(const QString &bundlePath, const QString &bundleId,
                        const QStringList &sharedFiles);

    ContentLibraryWidget *m_widget = nullptr;
    QString m_searchText;
    QString m_bundleId;

    QList<ContentLibraryMaterial *> m_userMaterials;
    QList<ContentLibraryTexture *> m_userTextures;
    QList<ContentLibraryEffect *> m_userEffects;
    QList<ContentLibraryEffect *> m_user3DItems;
    QStringList m_userCategories;

    QJsonObject m_bundleObj;
    Internal::ContentLibraryBundleImporter *m_importer = nullptr;

    bool m_isEmpty = true;
    bool m_matBundleExists = false;
    bool m_hasModelSelection = false;
    bool m_importerRunning = false;

    int m_quick3dMajorVersion = -1;
    int m_quick3dMinorVersion = -1;

    QString m_importerBundlePath;
    QString m_importerBundleId;
    QStringList m_importerSharedFiles;

    enum Roles { NameRole = Qt::UserRole + 1, VisibleRole, ItemsRole };
};

} // namespace QmlDesigner
