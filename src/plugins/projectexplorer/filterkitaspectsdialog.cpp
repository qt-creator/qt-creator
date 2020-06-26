/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "filterkitaspectsdialog.h"

#include "kitmanager.h"

#include <utils/itemviews.h>
#include <utils/qtcassert.h>
#include <utils/treemodel.h>

#include <QDialogButtonBox>
#include <QHeaderView>
#include <QString>
#include <QVBoxLayout>

using namespace Utils;

namespace ProjectExplorer {
namespace Internal {

class FilterTreeItem : public TreeItem
{
public:
    FilterTreeItem(const KitAspect *aspect, bool enabled) : m_aspect(aspect), m_enabled(enabled)
    { }

    QString displayName() const { return m_aspect->displayName(); }
    Utils::Id id() const { return m_aspect->id(); }
    bool enabled() const { return m_enabled; }

private:
    QVariant data(int column, int role) const override
    {
        QTC_ASSERT(column < 2, return QVariant());
        if (column == 0 && role == Qt::DisplayRole)
            return displayName();
        if (column == 1 && role == Qt::CheckStateRole)
            return m_enabled ? Qt::Checked : Qt::Unchecked;
        return QVariant();
    }

    bool setData(int column, const QVariant &data, int role) override
    {
        QTC_ASSERT(column == 1 && !m_aspect->isEssential(), return false);
        if (role == Qt::CheckStateRole) {
            m_enabled = data.toInt() == Qt::Checked;
            return true;
        }
        return false;
    }

    Qt::ItemFlags flags(int column) const override
    {
        QTC_ASSERT(column < 2, return Qt::ItemFlags());
        Qt::ItemFlags flags = Qt::ItemIsSelectable;
        if (column == 0 || !m_aspect->isEssential())
            flags |= Qt::ItemIsEnabled;
        if (column == 1 && !m_aspect->isEssential())
            flags |= Qt::ItemIsUserCheckable;
        return flags;
    }

    const KitAspect * const m_aspect;
    bool m_enabled;
};

class FilterKitAspectsModel : public TreeModel<TreeItem, FilterTreeItem>
{
public:
    FilterKitAspectsModel(const Kit *kit, QObject *parent) : TreeModel(parent)
    {
        setHeader({FilterKitAspectsDialog::tr("Setting"), FilterKitAspectsDialog::tr("Visible")});
        for (const KitAspect * const aspect : KitManager::kitAspects()) {
            if (kit && !aspect->isApplicableToKit(kit))
                continue;
            const QSet<Utils::Id> irrelevantAspects = kit ? kit->irrelevantAspects()
                                                         : KitManager::irrelevantAspects();
            auto * const item = new FilterTreeItem(aspect,
                                                   !irrelevantAspects.contains(aspect->id()));
            rootItem()->appendChild(item);
        }
        static const auto cmp = [](const TreeItem *item1, const TreeItem *item2) {
            return static_cast<const FilterTreeItem *>(item1)->displayName()
                    < static_cast<const FilterTreeItem *>(item2)->displayName();
        };
        rootItem()->sortChildren(cmp);
    }

    QSet<Utils::Id> disabledItems() const
    {
        QSet<Utils::Id> ids;
        for (int i = 0; i < rootItem()->childCount(); ++i) {
            const FilterTreeItem * const item
                    = static_cast<FilterTreeItem *>(rootItem()->childAt(i));
            if (!item->enabled())
                ids << item->id();
        }
        return ids;
    }
};

class FilterTreeView : public TreeView
{
public:
    FilterTreeView(QWidget *parent) : TreeView(parent)
    {
        setUniformRowHeights(true);
    }

private:
    QSize sizeHint() const override
    {
        const int width = columnWidth(0) + columnWidth(1);
        const int height = model()->rowCount() * rowHeight(model()->index(0, 0))
                + header()->sizeHint().height();
        return {width, height};
    }
};

FilterKitAspectsDialog::FilterKitAspectsDialog(const Kit *kit, QWidget *parent)
    : QDialog(parent), m_model(new FilterKitAspectsModel(kit, this))
{
    auto * const layout = new QVBoxLayout(this);
    auto * const view = new FilterTreeView(this);
    view->setModel(m_model);
    view->resizeColumnToContents(0);
    layout->addWidget(view);
    auto * const buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addWidget(buttonBox);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

QSet<Utils::Id> FilterKitAspectsDialog::irrelevantAspects() const
{
    return static_cast<FilterKitAspectsModel *>(m_model)->disabledItems();
}

} // namespace Internal
} // namespace ProjectExplorer
