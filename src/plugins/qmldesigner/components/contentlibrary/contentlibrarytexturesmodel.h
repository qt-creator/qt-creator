// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <QAbstractListModel>

namespace QmlDesigner {

class ContentLibraryTexturesCategory;

class ContentLibraryTexturesModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(bool matBundleExists READ matBundleExists NOTIFY matBundleExistsChanged)
    Q_PROPERTY(bool isEmpty MEMBER m_isEmpty NOTIFY isEmptyChanged)
    Q_PROPERTY(bool hasQuick3DImport READ hasQuick3DImport WRITE setHasQuick3DImport NOTIFY hasQuick3DImportChanged)
    Q_PROPERTY(bool hasModelSelection READ hasModelSelection WRITE setHasModelSelection NOTIFY hasModelSelectionChanged)
    Q_PROPERTY(bool hasMaterialRoot READ hasMaterialRoot WRITE setHasMaterialRoot NOTIFY hasMaterialRootChanged)
    Q_PROPERTY(bool hasSceneEnv READ hasSceneEnv WRITE setHasSceneEnv NOTIFY hasSceneEnvChanged)

public:
    ContentLibraryTexturesModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    QHash<int, QByteArray> roleNames() const override;

    void setSearchText(const QString &searchText);

    void setQuick3DImportVersion(int major, int minor);

    bool hasQuick3DImport() const;
    void setHasQuick3DImport(bool b);

    bool hasMaterialRoot() const;
    void setHasMaterialRoot(bool b);

    bool matBundleExists() const;

    bool hasModelSelection() const;
    void setHasModelSelection(bool b);

    bool hasSceneEnv() const;
    void setHasSceneEnv(bool b);

    void resetModel();
    void loadTextureBundle(const QString &bundlePath);

    Q_INVOKABLE void addToProject(const QString &mat);

signals:
    void isEmptyChanged();
    void hasQuick3DImportChanged();
    void hasModelSelectionChanged();
    void hasMaterialRootChanged();
    void materialVisibleChanged();
    void matBundleExistsChanged();
    void hasSceneEnvChanged();

private:
    bool isValidIndex(int idx) const;

    QString m_searchText;
    QList<ContentLibraryTexturesCategory *> m_bundleCategories;

    bool m_isEmpty = true;
    bool m_hasMaterialRoot = false;
    bool m_hasQuick3DImport = false;
    bool m_texBundleLoaded = false;
    bool m_hasModelSelection = false;
    bool m_hasSceneEnv = false;

    int m_quick3dMajorVersion = -1;
    int m_quick3dMinorVersion = -1;
};

} // namespace QmlDesigner
