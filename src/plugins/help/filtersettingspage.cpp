/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "filtersettingspage.h"
#include "filternamedialog.h"

#include <QtGui/QFileDialog>
#include <QtGui/QMessageBox>
#include <QtHelp/QHelpEngine>

using namespace Help::Internal;
    
FilterSettingsPage::FilterSettingsPage(QHelpEngine *helpEngine)
{
    m_helpEngine = helpEngine;    
}

QString FilterSettingsPage::name() const
{
    return "Filters";
}

QString FilterSettingsPage::category() const
{
    return "Help";
}

QString FilterSettingsPage::trCategory() const
{
    return tr("Help");
}

QWidget *FilterSettingsPage::createPage(QWidget *parent)
{
    m_currentPage = new QWidget(parent);
    m_ui.setupUi(m_currentPage);
    m_ui.attributeWidget->header()->hide();
    m_ui.attributeWidget->setRootIsDecorated(false);
    
    connect(m_ui.attributeWidget, SIGNAL(itemChanged(QTreeWidgetItem*, int)),
        this, SLOT(updateFilterMap()));
    connect(m_ui.filterWidget,
        SIGNAL(currentItemChanged(QListWidgetItem*, QListWidgetItem*)),
        this, SLOT(updateAttributes(QListWidgetItem*)));
    connect(m_ui.filterAddButton, SIGNAL(clicked()),
        this, SLOT(addFilter()));
    connect(m_ui.filterRemoveButton, SIGNAL(clicked()),
        this, SLOT(removeFilter()));
    updateFilterPage();
            
    return m_currentPage;
}

void FilterSettingsPage::updateFilterPage()
{
    if (!m_helpEngine)
        return;

    m_ui.filterWidget->clear();
    m_ui.attributeWidget->clear();

    QHelpEngineCore help(m_helpEngine->collectionFile(), 0);
    help.setupData();    
    m_filterMapBackup.clear();
    const QStringList filters = help.customFilters();
    foreach (const QString filter, filters) {
        QStringList atts = help.filterAttributes(filter);
        m_filterMapBackup.insert(filter, atts);
        if (!m_filterMap.contains(filter))
            m_filterMap.insert(filter, atts);
    }

    m_ui.filterWidget->addItems(m_filterMap.keys());

    foreach (const QString a, help.filterAttributes())
        new QTreeWidgetItem(m_ui.attributeWidget, QStringList() << a);
    
    if (m_filterMap.keys().count())
        m_ui.filterWidget->setCurrentRow(0);
}

void FilterSettingsPage::updateAttributes(QListWidgetItem *item)
{
    QStringList checkedList;
    if (item)
        checkedList = m_filterMap.value(item->text());
    QTreeWidgetItem *itm;
    for (int i=0; i<m_ui.attributeWidget->topLevelItemCount(); ++i) {
        itm = m_ui.attributeWidget->topLevelItem(i);
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
    QString filter = m_ui.filterWidget->currentItem()->text();
    if (!m_filterMap.contains(filter))
        return;
    
    QStringList newAtts;
    QTreeWidgetItem *itm = 0;
    for (int i=0; i<m_ui.attributeWidget->topLevelItemCount(); ++i) {
        itm = m_ui.attributeWidget->topLevelItem(i);
        if (itm->checkState(0) == Qt::Checked)
            newAtts.append(itm->text(0));
    }
    m_filterMap[filter] = newAtts;    
}

void FilterSettingsPage::addFilter()
{
    FilterNameDialog dia(m_currentPage);
    if (dia.exec() == QDialog::Rejected)
        return;

    QString filterName = dia.filterName();
    if (!m_filterMap.contains(filterName)) {
        m_filterMap.insert(filterName, QStringList());
        m_ui.filterWidget->addItem(filterName);
    }

    QList<QListWidgetItem*> lst = m_ui.filterWidget
        ->findItems(filterName, Qt::MatchCaseSensitive);
    m_ui.filterWidget->setCurrentItem(lst.first());    
}

void FilterSettingsPage::removeFilter()
{
    QListWidgetItem *item = m_ui.filterWidget
        ->takeItem(m_ui.filterWidget->currentRow());
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
    // This is handled via HelpPlugin::checkForHelpChanges, which is connected
    // to DocSettingsPage::apply.
}

bool FilterSettingsPage::applyChanges()
{
    bool changed = false;
    if (m_filterMap.count() != m_filterMapBackup.count()) {
        changed = true;
    } else {
        QMapIterator<QString, QStringList> it(m_filterMapBackup);
        while (it.hasNext() && !changed) {
            it.next();
            if (!m_filterMap.contains(it.key())) {
                changed = true;
            } else {
                QStringList a = it.value();
                QStringList b = m_filterMap.value(it.key());
                if (a.count() != b.count()) {
                    changed = true;
                } else {
                    QStringList::const_iterator i(a.constBegin());
                    while (i != a.constEnd()) {
                        if (!b.contains(*i)) {
                            changed = true;
                            break;
                        }
                        ++i;
                    }
                }
            }
        }
    }
    if (changed) {
        foreach (QString filter, m_removedFilters)
            m_helpEngine->removeCustomFilter(filter);
        QMapIterator<QString, QStringList> it(m_filterMap);
        while (it.hasNext()) {
            it.next();
            m_helpEngine->addCustomFilter(it.key(), it.value());
        }
        return true;
    }
    return false;
}
