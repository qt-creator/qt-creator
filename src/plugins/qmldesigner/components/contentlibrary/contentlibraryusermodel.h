// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "usercategory.h"

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

public:
    ContentLibraryUserModel(ContentLibraryWidget *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setSearchText(const QString &searchText);
    void updateImportedState(const QStringList &importedItems, const QString &bundleId);

    bool jsonPropertyExists(const QString &propName, const QString &propValue,
                            const QString &bundleId) const;

    void setQuick3DImportVersion(int major, int minor);

    bool hasRequiredQuick3DImport() const;

    void updateIsEmpty();

    void resetModel();

    void addItem(const QString &bundleId, const QString &name, const QString &qml,const QUrl &icon,
                 const QStringList &files);
    void refreshSection(const QString &bundleId);
    void addTextures(const Utils::FilePaths &paths);
    void removeTextures(const QStringList &fileNames);

    void removeItemByName(const QString &qmlFileName, const QString &bundleId);

    void setBundleObj(const QJsonObject &newBundleObj);
    QJsonObject &bundleObjectRef(const QString &bundleId);

    void loadBundles(bool force = false);

    Q_INVOKABLE void applyToSelected(QmlDesigner::ContentLibraryItem *mat, bool add = false);
    Q_INVOKABLE void addToProject(ContentLibraryItem *item);
    Q_INVOKABLE void removeFromProject(QObject *item);
    Q_INVOKABLE void removeTexture(QmlDesigner::ContentLibraryTexture *tex, bool refresh = true);
    Q_INVOKABLE void removeFromContentLib(QObject *item);

signals:
    void hasRequiredQuick3DImportChanged();
    void isEmptyChanged();
    void applyToSelectedTriggered(QmlDesigner::ContentLibraryItem *mat, bool add = false);

private:
    // section indices must match the order in initModel()
    enum SectionIndex { MaterialsSectionIdx = 0,
                        TexturesSectionIdx,
                        Items3DSectionIdx,
                        EffectsSectionIdx };

    void createCategories();
    void loadMaterialBundle();
    void load3DBundle();
    void loadTextureBundle();
    void removeItem(ContentLibraryItem *item);
    SectionIndex bundleIdToSectionIndex(const QString &bundleId) const;

    ContentLibraryWidget *m_widget = nullptr;
    QString m_searchText;

    QList<UserCategory *> m_userCategories;

    bool m_isEmpty = true;

    int m_quick3dMajorVersion = -1;
    int m_quick3dMinorVersion = -1;

    enum Roles { TitleRole = Qt::UserRole + 1, ItemsRole, EmptyRole, NoMatchRole };
};

} // namespace QmlDesigner
