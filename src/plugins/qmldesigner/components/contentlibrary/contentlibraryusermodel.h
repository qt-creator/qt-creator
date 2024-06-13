// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>

#include <QAbstractListModel>
#include <QJsonObject>

QT_FORWARD_DECLARE_CLASS(QUrl)

namespace QmlDesigner {

class ContentLibraryItem;
class ContentLibraryTexture;
class ContentLibraryWidget;
class NodeMetaInfo;

class ContentLibraryUserModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(bool hasRequiredQuick3DImport READ hasRequiredQuick3DImport NOTIFY hasRequiredQuick3DImportChanged)
    Q_PROPERTY(bool isEmpty MEMBER m_isEmpty NOTIFY isEmptyChanged)
    Q_PROPERTY(QList<ContentLibraryItem *> userMaterials MEMBER m_userMaterials NOTIFY userMaterialsChanged)
    Q_PROPERTY(QList<ContentLibraryTexture *> userTextures MEMBER m_userTextures NOTIFY userTexturesChanged)
    Q_PROPERTY(QList<ContentLibraryItem *> user3DItems MEMBER m_user3DItems NOTIFY user3DItemsChanged)
    Q_PROPERTY(QList<ContentLibraryItem *> userEffects MEMBER m_userEffects NOTIFY userEffectsChanged)

public:
    enum SectionIndex { MaterialsSectionIdx = 0,
                        TexturesSectionIdx,
                        Items3DSectionIdx,
                        EffectsSectionIdx };

    ContentLibraryUserModel(ContentLibraryWidget *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setSearchText(const QString &searchText);
    void updateMaterialsImportedState(const QStringList &importedItems);
    void update3DImportedState(const QStringList &importedItems);

    QPair<QString, QString> getUniqueLibMaterialNames(const QString &defaultName = "Material") const;
    QPair<QString, QString> getUniqueLib3DNames(const QString &defaultName = "Item") const;
    QPair<QString, QString> getUniqueLibItemNames(const QString &defaultName = "Item",
                                                  const QJsonObject &bundleObj = {}) const;

    void setQuick3DImportVersion(int major, int minor);

    bool hasRequiredQuick3DImport() const;

    void updateIsEmpty();

    void resetModel();
    void updateNoMatchMaterials();
    void updateNoMatchTextures();
    void updateNoMatch3D();

    void addMaterial(const QString &name, const QString &qml, const QUrl &icon, const QStringList &files);
    void add3DItem(const QString &name, const QString &qml, const QUrl &icon, const QStringList &files);
    void refreshSection(SectionIndex sectionIndex);
    void addTextures(const QStringList &paths);

    void add3DInstance(ContentLibraryItem *bundleItem);

    void remove3DFromContentLibByName(const QString &qmlFileName);
    void removeMaterialFromContentLibByName(const QString &qmlFileName);

    void setBundleObj(const QJsonObject &newBundleObj);
    QJsonObject &bundleJsonMaterialObjectRef();
    QJsonObject &bundleJson3DObjectRef();

    void loadBundles();

    Q_INVOKABLE void applyToSelected(QmlDesigner::ContentLibraryItem *mat, bool add = false);
    Q_INVOKABLE void addToProject(QObject *item);
    Q_INVOKABLE void removeFromProject(QObject *item);
    Q_INVOKABLE void removeTexture(QmlDesigner::ContentLibraryTexture *tex);
    Q_INVOKABLE void removeFromContentLib(QObject *item);

signals:
    void hasRequiredQuick3DImportChanged();
    void isEmptyChanged();
    void userMaterialsChanged();
    void userTexturesChanged();
    void user3DItemsChanged();
    void userEffectsChanged();
    void applyToSelectedTriggered(QmlDesigner::ContentLibraryItem *mat, bool add = false);

private:
    void loadMaterialBundle();
    void load3DBundle();
    void loadTextureBundle();
    void removeMaterialFromContentLib(ContentLibraryItem *mat);
    void remove3DFromContentLib(ContentLibraryItem *item);

    ContentLibraryWidget *m_widget = nullptr;
    QString m_searchText;
    QString m_bundleIdMaterial;
    QString m_bundleId3D;
    QStringList m_bundleMaterialSharedFiles;
    QStringList m_bundle3DSharedFiles;
    Utils::FilePath m_bundlePathMaterial;
    Utils::FilePath m_bundlePath3D;

    QList<ContentLibraryItem *> m_userMaterials;
    QList<ContentLibraryTexture *> m_userTextures;
    QList<ContentLibraryItem *> m_userEffects;
    QList<ContentLibraryItem *> m_user3DItems;
    const QStringList m_userCategories = {tr("Materials"), tr("Textures"), tr("3D"),
                                          /*tr("Effects"), tr("2D components")*/}; // TODO;

    QJsonObject m_bundleObjMaterial;
    QJsonObject m_bundleObj3D;

    bool m_noMatchMaterials = true;
    bool m_noMatchTextures = true;
    bool m_noMatch3D = true;
    bool m_noMatchEffects = true;
    bool m_matBundleLoaded = false;
    bool m_bundle3DLoaded = false;
    bool m_isEmpty = true;

    int m_quick3dMajorVersion = -1;
    int m_quick3dMinorVersion = -1;

    enum Roles { NameRole = Qt::UserRole + 1, VisibleRole, ItemsRole, NoMatchRole };
};

} // namespace QmlDesigner
