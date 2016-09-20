/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "locatorsettingspage.h"
#include "locatorconstants.h"

#include "locator.h"
#include "ilocatorfilter.h"
#include "directoryfilter.h"

#include <coreplugin/coreconstants.h>
#include <utils/categorysortfiltermodel.h>
#include <utils/headerviewstretcher.h>
#include <utils/qtcassert.h>
#include <utils/treemodel.h>

#include <QCoreApplication>

using namespace Utils;

static const int SortRole = Qt::UserRole + 1;

namespace Core {
namespace Internal {

enum FilterItemColumn
{
    FilterName = 0,
    FilterPrefix,
    FilterIncludedByDefault
};

class FilterItem : public TreeItem
{
public:
    FilterItem(ILocatorFilter *filter);

    QVariant data(int column, int role) const override;
    Qt::ItemFlags flags(int column) const override;
    bool setData(int column, const QVariant &data, int role) override;

    ILocatorFilter *filter() const;

private:
    ILocatorFilter *m_filter;
};

class CategoryItem : public TreeItem
{
public:
    CategoryItem(const QString &name, int order);
    QVariant data(int column, int role) const override;
    Qt::ItemFlags flags(int column) const override { Q_UNUSED(column); return Qt::ItemIsEnabled; }

private:
    QString m_name;
    int m_order;
};

} // Internal
} // Core

using namespace Core;
using namespace Core::Internal;

FilterItem::FilterItem(ILocatorFilter *filter)
    : m_filter(filter)
{
}

QVariant FilterItem::data(int column, int role) const
{
    switch (column) {
    case FilterName:
        if (role == Qt::DisplayRole || role == SortRole)
            return m_filter->displayName();
        break;
    case FilterPrefix:
        if (role == Qt::DisplayRole || role == SortRole || role == Qt::EditRole)
            return m_filter->shortcutString();
        break;
    case FilterIncludedByDefault:
        if (role == Qt::CheckStateRole || role == SortRole || role == Qt::EditRole)
            return m_filter->isIncludedByDefault() ? Qt::Checked : Qt::Unchecked;
        break;
    default:
        break;
    }
    return QVariant();
}

Qt::ItemFlags FilterItem::flags(int column) const
{
    if (column == FilterPrefix)
        return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable;
    if (column == FilterIncludedByDefault)
        return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsUserCheckable;
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

bool FilterItem::setData(int column, const QVariant &data, int role)
{
    switch (column) {
    case FilterName:
        break;
    case FilterPrefix:
        if (role == Qt::EditRole && data.canConvert<QString>()) {
            m_filter->setShortcutString(data.toString());
            return true;
        }
        break;
    case FilterIncludedByDefault:
        if (role == Qt::CheckStateRole && data.canConvert<bool>()) {
            m_filter->setIncludedByDefault(data.toBool());
            return true;
        }
    }
    return false;
}

ILocatorFilter *FilterItem::filter() const
{
    return m_filter;
}

CategoryItem::CategoryItem(const QString &name, int order)
    : m_name(name), m_order(order)
{
}

QVariant CategoryItem::data(int column, int role) const
{
    Q_UNUSED(column);
    if (role == SortRole)
        return m_order;
    if (role == Qt::DisplayRole)
        return m_name;
    return QVariant();
}

LocatorSettingsPage::LocatorSettingsPage(Locator *plugin)
    : m_plugin(plugin), m_widget(0)
{
    setId(Constants::FILTER_OPTIONS_PAGE);
    setDisplayName(QCoreApplication::translate("Locator", Constants::FILTER_OPTIONS_PAGE));
    setCategory(Constants::SETTINGS_CATEGORY_CORE);
    setDisplayCategory(QCoreApplication::translate("Core", Constants::SETTINGS_TR_CATEGORY_CORE));
    setCategoryIcon(Utils::Icon(Constants::SETTINGS_CATEGORY_CORE_ICON));
}

QWidget *LocatorSettingsPage::widget()
{
    if (!m_widget) {
        m_filters = m_plugin->filters();
        m_customFilters = m_plugin->customFilters();

        m_widget = new QWidget;
        m_ui.setupUi(m_widget);
        m_ui.refreshInterval->setToolTip(m_ui.refreshIntervalLabel->toolTip());

        m_ui.filterEdit->setFiltering(true);

        m_ui.filterList->setSelectionMode(QAbstractItemView::SingleSelection);
        m_ui.filterList->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_ui.filterList->setSortingEnabled(true);
        m_ui.filterList->setUniformRowHeights(true);
        m_ui.filterList->setActivationMode(Utils::DoubleClickActivation);

        m_model = new TreeModel<>(m_ui.filterList);
        initializeModel();
        m_proxyModel = new CategorySortFilterModel(m_ui.filterList);
        m_proxyModel->setSourceModel(m_model);
        m_proxyModel->setSortRole(SortRole);
        m_proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
        m_proxyModel->setFilterKeyColumn(-1/*all*/);
        m_ui.filterList->setModel(m_proxyModel);
        m_ui.filterList->expandAll();

        new HeaderViewStretcher(m_ui.filterList->header(), FilterName);
        m_ui.filterList->header()->setSortIndicator(FilterName, Qt::AscendingOrder);

        connect(m_ui.filterEdit, &FancyLineEdit::filterChanged,
                this, &LocatorSettingsPage::setFilter);
        connect(m_ui.filterList->selectionModel(), &QItemSelectionModel::currentChanged,
                this, &LocatorSettingsPage::updateButtonStates);
        connect(m_ui.filterList, &Utils::TreeView::activated,
                this, &LocatorSettingsPage::configureFilter);
        connect(m_ui.editButton, &QPushButton::clicked,
                this, [this]() { configureFilter(m_ui.filterList->currentIndex()); });
        connect(m_ui.addButton, &QPushButton::clicked,
                this, &LocatorSettingsPage::addCustomFilter);
        connect(m_ui.removeButton, &QPushButton::clicked,
                this, &LocatorSettingsPage::removeCustomFilter);

        m_ui.refreshInterval->setValue(m_plugin->refreshInterval());
        saveFilterStates();
    }
    return m_widget;
}

void LocatorSettingsPage::apply()
{
    // Delete removed filters and clear added filters
    qDeleteAll(m_removedFilters);
    m_removedFilters.clear();
    m_addedFilters.clear();

    // Pass the new configuration on to the plugin
    m_plugin->setFilters(m_filters);
    m_plugin->setCustomFilters(m_customFilters);
    m_plugin->setRefreshInterval(m_ui.refreshInterval->value());
    requestRefresh();
    m_plugin->saveSettings();
    saveFilterStates();
}

void LocatorSettingsPage::finish()
{
    // If settings were applied, this shouldn't change anything. Otherwise it
    // makes sure the filter states aren't changed permanently.
    restoreFilterStates();

    // Delete added filters and clear removed filters
    qDeleteAll(m_addedFilters);
    m_addedFilters.clear();
    m_removedFilters.clear();

    // Further cleanup
    m_filters.clear();
    m_customFilters.clear();
    m_refreshFilters.clear();
    delete m_widget;
}

void LocatorSettingsPage::requestRefresh()
{
    if (!m_refreshFilters.isEmpty())
        m_plugin->refresh(m_refreshFilters);
}

void LocatorSettingsPage::setFilter(const QString &text)
{
    m_proxyModel->setFilterFixedString(text);
    m_ui.filterList->expandAll();
}

void LocatorSettingsPage::saveFilterStates()
{
    m_filterStates.clear();
    foreach (ILocatorFilter *filter, m_filters)
        m_filterStates.insert(filter, filter->saveState());
}

void LocatorSettingsPage::restoreFilterStates()
{
    foreach (ILocatorFilter *filter, m_filterStates.keys())
        filter->restoreState(m_filterStates.value(filter));
}

void LocatorSettingsPage::initializeModel()
{
    m_model->setHeader(QStringList({ tr("Name"), tr("Prefix"), tr("Default") }));
    m_model->setHeaderToolTip(QStringList({
        QString(),
        ILocatorFilter::msgPrefixToolTip(),
        ILocatorFilter::msgIncludeByDefaultToolTip()
    }));
    m_model->clear();
    QSet<ILocatorFilter *> customFilterSet = m_customFilters.toSet();
    auto builtIn = new CategoryItem(tr("Built-in"), 0/*order*/);
    foreach (ILocatorFilter *filter, m_filters)
        if (!filter->isHidden() && !customFilterSet.contains(filter))
            builtIn->appendChild(new FilterItem(filter));
    m_customFilterRoot = new CategoryItem(tr("Custom"), 1/*order*/);
    foreach (ILocatorFilter *customFilter, m_customFilters)
        m_customFilterRoot->appendChild(new FilterItem(customFilter));

    m_model->rootItem()->appendChild(builtIn);
    m_model->rootItem()->appendChild(m_customFilterRoot);
}

void LocatorSettingsPage::updateButtonStates()
{
    const QModelIndex currentIndex = m_proxyModel->mapToSource(m_ui.filterList->currentIndex());
    bool selected = currentIndex.isValid();
    ILocatorFilter *filter = 0;
    if (selected) {
        auto item = dynamic_cast<FilterItem *>(m_model->itemForIndex(currentIndex));
        if (item)
            filter = item->filter();
    }
    m_ui.editButton->setEnabled(filter && filter->isConfigurable());
    m_ui.removeButton->setEnabled(filter && m_customFilters.contains(filter));
}

void LocatorSettingsPage::configureFilter(const QModelIndex &proxyIndex)
{
    const QModelIndex index = m_proxyModel->mapToSource(proxyIndex);
    QTC_ASSERT(index.isValid(), return);
    auto item = dynamic_cast<FilterItem *>(m_model->itemForIndex(index));
    QTC_ASSERT(item, return);
    ILocatorFilter *filter = item->filter();
    QTC_ASSERT(filter->isConfigurable(), return);
    bool includedByDefault = filter->isIncludedByDefault();
    QString shortcutString = filter->shortcutString();
    bool needsRefresh = false;
    filter->openConfigDialog(m_widget, needsRefresh);
    if (needsRefresh && !m_refreshFilters.contains(filter))
        m_refreshFilters.append(filter);
    if (filter->isIncludedByDefault() != includedByDefault)
        item->updateColumn(FilterIncludedByDefault);
    if (filter->shortcutString() != shortcutString)
        item->updateColumn(FilterPrefix);
}

void LocatorSettingsPage::addCustomFilter()
{
    ILocatorFilter *filter = new DirectoryFilter(
                Id(Constants::CUSTOM_FILTER_BASEID).withSuffix(m_customFilters.size() + 1));
    bool needsRefresh = false;
    if (filter->openConfigDialog(m_widget, needsRefresh)) {
        m_filters.append(filter);
        m_addedFilters.append(filter);
        m_customFilters.append(filter);
        m_refreshFilters.append(filter);
        m_customFilterRoot->appendChild(new FilterItem(filter));
    }
}

void LocatorSettingsPage::removeCustomFilter()
{
    QModelIndex currentIndex = m_proxyModel->mapToSource(m_ui.filterList->currentIndex());
    QTC_ASSERT(currentIndex.isValid(), return);
    auto item = dynamic_cast<FilterItem *>(m_model->itemForIndex(currentIndex));
    QTC_ASSERT(item, return);
    ILocatorFilter *filter = item->filter();
    QTC_ASSERT(m_customFilters.contains(filter), return);
    m_model->destroyItem(item);
    m_filters.removeAll(filter);
    m_customFilters.removeAll(filter);
    m_refreshFilters.removeAll(filter);
    if (m_addedFilters.contains(filter)) {
        m_addedFilters.removeAll(filter);
        delete filter;
    } else {
        m_removedFilters.append(filter);
    }
}
