/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#include "settingspage.h"

#include "quickopenplugin.h"
#include "iquickopenfilter.h"
#include "directoryfilter.h"

#include <qtconcurrent/QtConcurrentTools>
#include <utils/qtcassert.h>

#include <QtGui/QMessageBox>

Q_DECLARE_METATYPE(QuickOpen::IQuickOpenFilter*)

using namespace QuickOpen;
using namespace QuickOpen::Internal;

SettingsPage::SettingsPage(Core::ICore *core, QuickOpenPlugin *plugin)
    : m_core(core), m_plugin(plugin), m_page(0)
{
}

QWidget *SettingsPage::createPage(QWidget *parent)
{
    if (!m_page) {
        m_page = new QWidget(parent);
        m_ui.setupUi(m_page);
        connect(m_ui.filterList, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
                this, SLOT(updateButtonStates()));
        connect(m_ui.filterList, SIGNAL(itemActivated(QListWidgetItem *)),
                this, SLOT(configureFilter(QListWidgetItem *)));
        connect(m_ui.editButton, SIGNAL(clicked()),
                this, SLOT(configureFilter()));
        connect(m_ui.addButton, SIGNAL(clicked()),
                this, SLOT(addCustomFilter()));
        connect(m_ui.removeButton, SIGNAL(clicked()),
                this, SLOT(removeCustomFilter()));
    }
    m_ui.refreshInterval->setValue(m_plugin->refreshInterval());
    m_filters = m_plugin->filter();
    m_customFilters = m_plugin->customFilter();
    saveFilterStates();
    updateFilterList();
    return m_page;
}

void SettingsPage::finished(bool accepted)
{
    if (!accepted) {
        restoreFilterStates();
        foreach (IQuickOpenFilter *filter, m_addedFilters)
            delete filter;
    } else {
        foreach (IQuickOpenFilter *filter, m_removedFilters)
            delete filter;
        m_plugin->setFilter(m_filters);
        m_plugin->setCustomFilter(m_customFilters);
        m_plugin->setRefreshInterval(m_ui.refreshInterval->value());
        requestRefresh();
        m_plugin->saveSettings();
    }
    m_addedFilters.clear();
    m_removedFilters.clear();
    m_filters.clear();
    m_customFilters.clear();
    m_refreshFilters.clear();
}

void SettingsPage::requestRefresh()
{
    if (!m_refreshFilters.isEmpty())
        m_plugin->refresh(m_refreshFilters);
}

void SettingsPage::saveFilterStates()
{
    m_filterStates.clear();
    foreach (IQuickOpenFilter *filter, m_filters)
        m_filterStates.insert(filter, filter->saveState());
}

void SettingsPage::restoreFilterStates()
{
    foreach (IQuickOpenFilter *filter, m_filterStates.keys())
        filter->restoreState(m_filterStates.value(filter));
}

void SettingsPage::updateFilterList()
{
    m_ui.filterList->clear();
    foreach (IQuickOpenFilter *filter, m_filters) {
        QString title;
        if (filter->defaultActiveState())
            title = filter->trName();
        else
            title = tr("%1 (Prefix: %2)").arg(filter->trName()).arg(filter->shortcutString());
        QListWidgetItem *item = new QListWidgetItem(title);
        item->setData(Qt::UserRole, qVariantFromValue(filter));
        m_ui.filterList->addItem(item);
    }
    if (m_ui.filterList->count() > 0)
        m_ui.filterList->setCurrentRow(0);
}

void SettingsPage::updateButtonStates()
{
    QListWidgetItem *item = m_ui.filterList->currentItem();
    IQuickOpenFilter *filter = (item ? item->data(Qt::UserRole).value<IQuickOpenFilter *>() : 0);
    m_ui.editButton->setEnabled(filter && filter->isConfigurable());
    m_ui.removeButton->setEnabled(filter && m_customFilters.contains(filter));
}

void SettingsPage::configureFilter(QListWidgetItem *item)
{
    if (!item)
        item = m_ui.filterList->currentItem();
    QTC_ASSERT(item, return);
    IQuickOpenFilter *filter = item->data(Qt::UserRole).value<IQuickOpenFilter *>();
    QTC_ASSERT(filter, return);

    if (!filter->isConfigurable())
        return;
    bool needsRefresh = false;
    filter->openConfigDialog(m_page, needsRefresh);
    if (needsRefresh && !m_refreshFilters.contains(filter))
        m_refreshFilters.append(filter);
    updateFilterList();
}

void SettingsPage::addCustomFilter()
{
    IQuickOpenFilter *filter = new DirectoryFilter(m_core);
    bool needsRefresh = false;
    if (filter->openConfigDialog(m_page, needsRefresh)) {
        m_filters.append(filter);
        m_addedFilters.append(filter);
        m_customFilters.append(filter);
        m_refreshFilters.append(filter);
        updateFilterList();
    }
}

void SettingsPage::removeCustomFilter()
{
    QListWidgetItem *item = m_ui.filterList->currentItem();
    QTC_ASSERT(item, return);
    IQuickOpenFilter *filter = item->data(Qt::UserRole).value<IQuickOpenFilter *>();
    QTC_ASSERT(m_customFilters.contains(filter), return);
    m_filters.removeAll(filter);
    m_customFilters.removeAll(filter);
    m_refreshFilters.removeAll(filter);
    if (m_addedFilters.contains(filter)) {
        m_addedFilters.removeAll(filter);
        delete filter;
    } else {
        m_removedFilters.append(filter);
    }
    updateFilterList();
}
