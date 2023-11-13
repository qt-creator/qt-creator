// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/theme/theme.h>

#include <QStandardItemModel>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QItemSelectionModel;
class QListView;
class QSortFilterProxyModel;
QT_END_NAMESPACE

namespace ExtensionSystem
{
class PluginSpec;
}

namespace Core {
class SearchBox;
class WelcomePageButton;
}

namespace ExtensionManager::Internal {

using PluginSpecList = QList<const ExtensionSystem::PluginSpec *>;
using Tags = QStringList;

enum ItemType {
    ItemTypePack,
    ItemTypeExtension,
};

struct ItemData {
    const QString name;
    const ItemType type = ItemTypeExtension;
    const Tags tags;
    const PluginSpecList plugins;
};

ItemData itemData(const QModelIndex &index);
void setBackgroundColor(QWidget *widget, Utils::Theme::Color colorRole);

class ExtensionsBrowser final : public QWidget
{
    Q_OBJECT

public:
    ExtensionsBrowser();

    void adjustToWidth(const int width);
    QSize sizeHint() const override;

signals:
    void itemSelected(const QModelIndex &current, const QModelIndex &previous);

private:
    int extraListViewWidth() const; // Space for scrollbar, etc.

    QScopedPointer<QStandardItemModel> m_model;
    Core::SearchBox *m_searchBox;
    Core::WelcomePageButton *m_updateButton;
    QListView *m_extensionsView;
    QItemSelectionModel *m_selectionModel = nullptr;
    QSortFilterProxyModel *m_filterProxyModel;
    int m_columnsCount = 2;
};

} // ExtensionManager::Internal
