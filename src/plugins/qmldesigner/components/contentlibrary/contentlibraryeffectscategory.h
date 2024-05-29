// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "contentlibraryitem.h"

#include <QObject>

namespace QmlDesigner {

class ContentLibraryEffectsCategory : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString bundleCategoryName MEMBER m_name CONSTANT)
    Q_PROPERTY(bool bundleCategoryVisible MEMBER m_visible NOTIFY categoryVisibleChanged)
    Q_PROPERTY(bool bundleCategoryExpanded MEMBER m_expanded NOTIFY categoryExpandChanged)
    Q_PROPERTY(QList<ContentLibraryItem *> bundleCategoryItems MEMBER m_categoryItems
               NOTIFY categoryItemsChanged)

public:
    ContentLibraryEffectsCategory(QObject *parent, const QString &name);

    void addBundleItem(ContentLibraryItem *bundleItem);
    bool updateImportedState(const QStringList &importedMats);
    bool filter(const QString &searchText);

    QString name() const;
    bool visible() const;
    bool expanded() const;
    QList<ContentLibraryItem *> categoryItems() const;

signals:
    void categoryVisibleChanged();
    void categoryExpandChanged();
    void categoryItemsChanged();

private:
    QString m_name;
    bool m_visible = true;
    bool m_expanded = true;

    QList<ContentLibraryItem *> m_categoryItems;
};

} // namespace QmlDesigner
