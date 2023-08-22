// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "filterkitaspectsdialog.h"

#include "kitmanager.h"
#include "projectexplorertr.h"

#include <utils/itemviews.h>
#include <utils/qtcassert.h>
#include <utils/treemodel.h>

#include <QDialogButtonBox>
#include <QHeaderView>
#include <QString>
#include <QTextDocument>
#include <QVBoxLayout>

using namespace Utils;

namespace ProjectExplorer::Internal {

class FilterTreeItem : public TreeItem
{
public:
    FilterTreeItem(const KitAspectFactory *factory, bool enabled) : m_factory(factory), m_enabled(enabled)
    { }

    QString displayName() const {
        if (m_factory->displayName().indexOf('<') < 0)
            return m_factory->displayName();

        // removing HTML tag because KitAspect::displayName could contain html
        // e.g. "CMake <a href=\"generator\">generator</a>" (CMakeGeneratorKitAspect)
        QTextDocument html;
        html.setHtml(m_factory->displayName());
        return html.toPlainText();
    }
    Utils::Id id() const { return m_factory->id(); }
    bool enabled() const { return m_enabled; }

private:
    QVariant data(int column, int role) const override
    {
        QTC_ASSERT(column < 2, return QVariant());
        if (column == 0 && role == Qt::DisplayRole)
            return displayName();
        if (column == 1 && role == Qt::CheckStateRole)
            return m_enabled ? Qt::Checked : Qt::Unchecked;
        return {};
    }

    bool setData(int column, const QVariant &data, int role) override
    {
        QTC_ASSERT(column == 1 && !m_factory->isEssential(), return false);
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
        if (column == 0 || !m_factory->isEssential())
            flags |= Qt::ItemIsEnabled;
        if (column == 1 && !m_factory->isEssential())
            flags |= Qt::ItemIsUserCheckable;
        return flags;
    }

    const KitAspectFactory * const m_factory;
    bool m_enabled;
};

class FilterKitAspectsModel : public TreeModel<TreeItem, FilterTreeItem>
{
public:
    FilterKitAspectsModel(const Kit *kit, QObject *parent) : TreeModel(parent)
    {
        setHeader({Tr::tr("Setting"), Tr::tr("Visible")});
        for (const KitAspectFactory * const factory : KitManager::kitAspectFactories()) {
            if (kit && !factory->isApplicableToKit(kit))
                continue;
            const QSet<Utils::Id> irrelevantAspects = kit ? kit->irrelevantAspects()
                                                         : KitManager::irrelevantAspects();
            auto * const item = new FilterTreeItem(factory,
                                                   !irrelevantAspects.contains(factory->id()));
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
    FilterTreeView(QWidget *parent) : TreeView(parent) {}

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

} // ProjectExplorer::Internal
