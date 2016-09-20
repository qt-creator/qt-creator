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

#include "filtersettingspage.h"

#include "helpconstants.h"

#include <filternamedialog.h>

#include <coreplugin/helpmanager.h>

#include <QCoreApplication>
#include <QFileDialog>
#include <QMessageBox>

using namespace Help::Internal;
using namespace Core;

FilterSettingsPage::FilterSettingsPage()
{
    setId("D.Filters");
    setDisplayName(tr("Filters"));
    setCategory(Help::Constants::HELP_CATEGORY);
    setDisplayCategory(QCoreApplication::translate("Help", Help::Constants::HELP_TR_CATEGORY));
    setCategoryIcon(Utils::Icon(Help::Constants::HELP_CATEGORY_ICON));
}

QWidget *FilterSettingsPage::widget()
{
    if (!m_widget) {
        m_widget = new QWidget;
        m_ui.setupUi(m_widget);

        updateFilterPage();

        connect(m_ui.attributeWidget, &QTreeWidget::itemChanged,
                this, &FilterSettingsPage::updateFilterMap);
        connect(m_ui.filterWidget, &QListWidget::currentItemChanged,
                this, &FilterSettingsPage::updateAttributes);
        connect(m_ui.filterAddButton, &QPushButton::clicked,
                this, &FilterSettingsPage::addFilter);
        connect(m_ui.filterRemoveButton, &QPushButton::clicked,
                this, &FilterSettingsPage::removeFilter);
        connect(HelpManager::instance(), &HelpManager::documentationChanged,
                this, &FilterSettingsPage::updateFilterPage);
    }
    return m_widget;
}

void FilterSettingsPage::updateFilterPage()
{
    m_ui.filterWidget->clear();
    m_ui.attributeWidget->clear();

    m_filterMapBackup.clear();

    QString lastTrUnfiltered;
    const QString trUnfiltered = tr("Unfiltered");
    if (HelpManager::customValue(Help::Constants::WeAddedFilterKey).toInt() == 1) {
        lastTrUnfiltered =
            HelpManager::customValue(Help::Constants::PreviousFilterNameKey).toString();
    }

    HelpManager::Filters filters = HelpManager::userDefinedFilters();
    HelpManager::Filters::const_iterator it;
    for (it = filters.constBegin(); it != filters.constEnd(); ++it) {
        const QString &filter = it.key();
        if (filter == trUnfiltered || filter == lastTrUnfiltered)
            continue;

        m_filterMapBackup.insert(filter, it.value());
        if (!m_filterMap.contains(filter))
            m_filterMap.insert(filter, it.value());
    }
    m_ui.filterWidget->addItems(m_filterMap.keys());

    QSet<QString> attributes;
    filters = HelpManager::filters();
    for (it = filters.constBegin(); it != filters.constEnd(); ++it)
        attributes += it.value().toSet();

    foreach (const QString &attribute, attributes)
        new QTreeWidgetItem(m_ui.attributeWidget, QStringList(attribute));

    if (!m_filterMap.keys().isEmpty()) {
        m_ui.filterWidget->setCurrentRow(0);
        updateAttributes(m_ui.filterWidget->currentItem());
    }
}

void FilterSettingsPage::updateAttributes(QListWidgetItem *item)
{
    QStringList checkedList;
    if (item)
        checkedList = m_filterMap.value(item->text());

    for (int i = 0; i < m_ui.attributeWidget->topLevelItemCount(); ++i) {
        QTreeWidgetItem *itm = m_ui.attributeWidget->topLevelItem(i);
        if (checkedList.contains(itm->text(0)))
            itm->setCheckState(0, Qt::Checked);
        else
            itm->setCheckState(0, Qt::Unchecked);
    }

    updateFilterDescription(item ? item->text() : QString());
}

void FilterSettingsPage::updateFilterMap()
{
    if (!m_ui.filterWidget->currentItem())
        return;

    const QString &filter = m_ui.filterWidget->currentItem()->text();
    if (!m_filterMap.contains(filter))
        return;

    QStringList newAtts;
    for (int i = 0; i < m_ui.attributeWidget->topLevelItemCount(); ++i) {
        QTreeWidgetItem *itm = m_ui.attributeWidget->topLevelItem(i);
        if (itm->checkState(0) == Qt::Checked)
            newAtts.append(itm->text(0));
    }
    m_filterMap[filter] = newAtts;
    updateFilterDescription(filter);
}

void FilterSettingsPage::addFilter()
{
    FilterNameDialog dia(m_ui.filterWidget);
    if (dia.exec() == QDialog::Rejected)
        return;

    const QString &filterName = dia.filterName();
    if (!m_filterMap.contains(filterName)) {
        m_filterMap.insert(filterName, QStringList());
        m_ui.filterWidget->addItem(filterName);
    }

    const QList<QListWidgetItem*> &lst = m_ui.filterWidget->findItems(filterName,
        Qt::MatchCaseSensitive);
    m_ui.filterWidget->setCurrentItem(lst.first());
}

void FilterSettingsPage::removeFilter()
{
    QListWidgetItem *item =
        m_ui.filterWidget->takeItem(m_ui.filterWidget->currentRow());
    if (!item)
        return;

    m_filterMap.remove(item->text());
    m_removedFilters.append(item->text());
    delete item;
    if (m_ui.filterWidget->count())
        m_ui.filterWidget->setCurrentRow(0);

    item = m_ui.filterWidget->item(m_ui.filterWidget->currentRow());
    updateFilterDescription(item ? item->text() : QString());
}

void FilterSettingsPage::apply()
{
    bool changed = m_filterMap.count() != m_filterMapBackup.count();
    if (!changed) {
        FilterMap::const_iterator it = m_filterMapBackup.constBegin();
        for (; it != m_filterMapBackup.constEnd() && !changed; ++it) {
            if (m_filterMap.contains(it.key())) {
                const QStringList &a = it.value();
                const QStringList &b = m_filterMap.value(it.key());
                if (a.count() == b.count()) {
                    QStringList::const_iterator i = a.constBegin();
                    for (; i != a.constEnd() && !changed; ++i) {
                        if (b.contains(*i))
                            continue;
                        changed = true;
                    }
                } else {
                    changed = true;
                }
            } else {
                changed = true;
            }
        }
    }

    if (changed) {
        foreach (const QString &filter, m_removedFilters)
            HelpManager::removeUserDefinedFilter(filter);

        FilterMap::const_iterator it;
        for (it = m_filterMap.constBegin(); it != m_filterMap.constEnd(); ++it)
            HelpManager::addUserDefinedFilter(it.key(), it.value());

        // emit this signal to the help plugin, since we don't want
        // to force gui help engine setup if we are not in help mode
        emit filtersChanged();
    }
}

void FilterSettingsPage::finish()
{
    disconnect(HelpManager::instance(), &HelpManager::documentationChanged,
               this, &FilterSettingsPage::updateFilterPage);
    delete m_widget;
}

QString FilterSettingsPage::msgFilterLabel(const QString &filter) const
{
    if (m_filterMap.keys().isEmpty())
        return tr("No user defined filters available or no filter selected.");

    const QStringList &checkedList = m_filterMap.value(filter);
    if (checkedList.isEmpty())
        return tr("The filter \"%1\" will show every documentation file"
                  " available, as no attributes are specified.").arg(filter);

    if (checkedList.size() == 1)
        return tr("The filter \"%1\" will only show documentation files that"
                  " have the attribute %2 specified.").
                arg(filter, checkedList.first());

    return tr("The filter \"%1\" will only show documentation files that"
              " have the attributes %2 specified.").
            arg(filter, checkedList.join(", "));
}

void FilterSettingsPage::updateFilterDescription(const QString &filter)
{
    m_ui.label->setText(msgFilterLabel(filter));
}
