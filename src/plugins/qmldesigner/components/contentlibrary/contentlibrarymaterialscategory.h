// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

namespace QmlDesigner {

class ContentLibraryMaterial;

class ContentLibraryMaterialsCategory : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString bundleCategoryName MEMBER m_name CONSTANT)
    Q_PROPERTY(bool bundleCategoryVisible MEMBER m_visible NOTIFY categoryVisibleChanged)
    Q_PROPERTY(bool bundleCategoryExpanded MEMBER m_expanded NOTIFY categoryExpandChanged)
    Q_PROPERTY(QList<ContentLibraryMaterial *> bundleCategoryMaterials MEMBER m_categoryMaterials
               NOTIFY bundleMaterialsModelChanged)

public:
    ContentLibraryMaterialsCategory(QObject *parent, const QString &name);

    void addBundleMaterial(ContentLibraryMaterial *bundleMat);
    bool updateImportedState(const QStringList &importedMats);
    bool filter(const QString &searchText);

    QString name() const;
    bool visible() const;
    bool expanded() const;
    QList<ContentLibraryMaterial *> categoryMaterials() const;

signals:
    void categoryVisibleChanged();
    void categoryExpandChanged();
    void bundleMaterialsModelChanged();

private:
    QString m_name;
    bool m_visible = true;
    bool m_expanded = true;

    QList<ContentLibraryMaterial *> m_categoryMaterials;
};

} // namespace QmlDesigner
