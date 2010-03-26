/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "filtersettingspage.h"

#include "filternamedialog.h"
#include "helpconstants.h"
#include "helpmanager.h"

#include <QtCore/QCoreApplication>

#include <QtGui/QFileDialog>
#include <QtGui/QMessageBox>

#include <QtHelp/QHelpEngineCore>

using namespace Help::Internal;

FilterSettingsPage::FilterSettingsPage()
{
}

QString FilterSettingsPage::id() const
{
    return QLatin1String("D.Filters");
}

QString FilterSettingsPage::displayName() const
{
    return tr("Filters");
}

QString FilterSettingsPage::category() const
{
    return QLatin1String(Help::Constants::HELP_CATEGORY);
}

QString FilterSettingsPage::displayCategory() const
{
    return QCoreApplication::translate("Help", Help::Constants::HELP_TR_CATEGORY);
}

QIcon FilterSettingsPage::categoryIcon() const
{
    return QIcon(QLatin1String(Help::Constants::HELP_CATEGORY_ICON));
}

QWidget *FilterSettingsPage::createPage(QWidget *parent)
{
    QWidget *widget = new QWidget(parent);
    m_ui.setupUi(widget);

    updateFilterPage(); // does call setupData on the engine

    connect(m_ui.attributeWidget, SIGNAL(itemChanged(QTreeWidgetItem*, int)),
        this, SLOT(updateFilterMap()));
    connect(m_ui.filterWidget,
        SIGNAL(currentItemChanged(QListWidgetItem*, QListWidgetItem*)), this,
        SLOT(updateAttributes(QListWidgetItem*)));
    connect(m_ui.filterAddButton, SIGNAL(clicked()), this, SLOT(addFilter()));
    connect(m_ui.filterRemoveButton, SIGNAL(clicked()), this,
        SLOT(removeFilter()));

    if (m_searchKeywords.isEmpty()) {
        m_searchKeywords = m_ui.filterGroupBox->title() + QLatin1Char(' ')
            + m_ui.attributesGroupBox->title();
    }
    return widget;
}

void FilterSettingsPage::updateFilterPage()
{
    m_ui.filterWidget->clear();
    m_ui.attributeWidget->clear();

    m_filterMapBackup.clear();
    const QHelpEngineCore &engine = HelpManager::helpEngineCore();
    const QStringList &filters = engine.customFilters();
    foreach (const QString &filter, filters) {
        const QStringList &attributes = engine.filterAttributes(filter);
        m_filterMapBackup.insert(filter, attributes);
        if (!m_filterMap.contains(filter))
            m_filterMap.insert(filter, attributes);
    }
    m_ui.filterWidget->addItems(m_filterMap.keys());

    const QStringList &attributes = engine.filterAttributes();
    foreach (const QString &attribute, attributes)
        new QTreeWidgetItem(m_ui.attributeWidget, QStringList(attribute));

    if (m_filterMap.keys().count())
        m_ui.filterWidget->setCurrentRow(0);
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
        QHelpEngineCore *engine = &HelpManager::helpEngineCore();
        foreach (const QString &filter, m_removedFilters)
           engine->removeCustomFilter(filter);

        FilterMap::const_iterator it;
        for (it = m_filterMap.constBegin(); it != m_filterMap.constEnd(); ++it)
            engine->addCustomFilter(it.key(), it.value());

        // emit this signal to the help plugin, since we don't want
        // to force gui help engine setup if we are not in help mode
        emit filtersChanged();
    }
}

bool FilterSettingsPage::matches(const QString &s) const
{
    return m_searchKeywords.contains(s, Qt::CaseInsensitive);
}
