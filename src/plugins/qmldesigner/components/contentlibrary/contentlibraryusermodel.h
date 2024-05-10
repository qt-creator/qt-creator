// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>

#include <QAbstractListModel>
#include <QJsonObject>

QT_FORWARD_DECLARE_CLASS(QUrl)

namespace QmlDesigner {

class ContentLibraryItem;
class ContentLibraryMaterial;
class ContentLibraryTexture;
class ContentLibraryWidget;
class NodeMetaInfo;

class ContentLibraryUserModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(bool matBundleExists READ matBundleExists NOTIFY matBundleExistsChanged)
    Q_PROPERTY(bool bundle3DExists MEMBER m_bundle3DExists NOTIFY bundle3DExistsChanged)
    Q_PROPERTY(bool isEmptyMaterials MEMBER m_isEmptyMaterials NOTIFY isEmptyMaterialsChanged)
    Q_PROPERTY(bool isEmpty3D MEMBER m_isEmpty3D NOTIFY isEmpty3DChanged)
    Q_PROPERTY(bool hasRequiredQuick3DImport READ hasRequiredQuick3DImport NOTIFY hasRequiredQuick3DImportChanged)
    Q_PROPERTY(QList<ContentLibraryMaterial *> userMaterials MEMBER m_userMaterials NOTIFY userMaterialsChanged)
    Q_PROPERTY(QList<ContentLibraryTexture *> userTextures MEMBER m_userTextures NOTIFY userTexturesChanged)
    Q_PROPERTY(QList<ContentLibraryItem *> user3DItems MEMBER m_user3DItems NOTIFY user3DItemsChanged)
    Q_PROPERTY(QList<ContentLibraryItem *> userEffects MEMBER m_userEffects NOTIFY userEffectsChanged)

public:
    ContentLibraryUserModel(ContentLibraryWidget *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setSearchText(const QString &searchText);
    void updateMaterialsImportedState(const QStringList &importedItems);
    void update3DImportedState(const QStringList &importedItems);

    QPair<QString, QString> getUniqueLibMaterialNameAndQml(const QString &defaultName = {}) const;
    QString getUniqueLib3DQmlName(const QString &defaultName = {}) const;

    void setQuick3DImportVersion(int major, int minor);

    bool hasRequiredQuick3DImport() const;

    bool matBundleExists() const;

    void resetModel();
    void updateIsEmptyMaterials();
    void updateIsEmpty3D();

    void addMaterial(const QString &name, const QString &qml, const QUrl &icon, const QStringList &files);
    void add3DItem(const QString &name, const QString &qml, const QUrl &icon, const QStringList &files);
    void refresh3DSection();
    void addTextures(const QStringList &paths);

    void add3DInstance(ContentLibraryItem *bundleItem);

    void setBundleObj(const QJsonObject &newBundleObj);
    QJsonObject &bundleJsonMaterialObjectRef();
    QJsonObject &bundleJson3DObjectRef();

    void loadBundles();

    Q_INVOKABLE void applyToSelected(QmlDesigner::ContentLibraryMaterial *mat, bool add = false);
    Q_INVOKABLE void addToProject(QObject *item);
    Q_INVOKABLE void removeFromProject(QObject *item);
    Q_INVOKABLE void removeTexture(QmlDesigner::ContentLibraryTexture *tex);
    Q_INVOKABLE void removeFromContentLib(QObject *item);

signals:
    void isEmptyMaterialsChanged();
    void isEmpty3DChanged();
    void hasRequiredQuick3DImportChanged();
    void userMaterialsChanged();
    void userTexturesChanged();
    void user3DItemsChanged();
    void userEffectsChanged();
    void applyToSelectedTriggered(QmlDesigner::ContentLibraryMaterial *mat, bool add = false);
    void matBundleExistsChanged();
    void bundle3DExistsChanged();

private:
    void loadMaterialBundle();
    void load3DBundle();
    void loadTextureBundle();
    bool isValidIndex(int idx) const;
    void removeMaterialFromContentLib(ContentLibraryMaterial *mat);
    void remove3DFromContentLib(ContentLibraryItem *item);

    ContentLibraryWidget *m_widget = nullptr;
    QString m_searchText;
    QString m_bundleIdMaterial;
    QString m_bundleId3D;
    QStringList m_bundleMaterialSharedFiles;
    QStringList m_bundle3DSharedFiles;
    Utils::FilePath m_bundlePath3D;

    QList<ContentLibraryMaterial *> m_userMaterials;
    QList<ContentLibraryTexture *> m_userTextures;
    QList<ContentLibraryItem *> m_userEffects;
    QList<ContentLibraryItem *> m_user3DItems;
    QStringList m_userCategories;

    QJsonObject m_bundleObjMaterial;
    QJsonObject m_bundleObj3D;

    bool m_isEmptyMaterials = true;
    bool m_isEmpty3D = true;
    bool m_matBundleExists = false;
    bool m_bundle3DExists = false;

    int m_quick3dMajorVersion = -1;
    int m_quick3dMinorVersion = -1;

    enum Roles { NameRole = Qt::UserRole + 1, VisibleRole, ItemsRole };
};

} // namespace QmlDesigner
