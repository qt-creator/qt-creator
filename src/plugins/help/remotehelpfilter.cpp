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

#include "remotehelpfilter.h"

#include <QMutexLocker>
#include <QUrl>

namespace Help {
    namespace Internal {

RemoteFilterOptions::RemoteFilterOptions(RemoteHelpFilter *filter, QWidget *parent)
    : QDialog(parent)
    , m_filter(filter)
{
    m_ui.setupUi(this);
    setWindowTitle(Core::ILocatorFilter::msgConfigureDialogTitle());
    m_ui.prefixLabel->setText(Core::ILocatorFilter::msgPrefixLabel());
    m_ui.prefixLabel->setToolTip(Core::ILocatorFilter::msgPrefixToolTip());
    m_ui.includeByDefault->setText(Core::ILocatorFilter::msgIncludeByDefault());
    m_ui.includeByDefault->setToolTip(Core::ILocatorFilter::msgIncludeByDefaultToolTip());
    m_ui.shortcutEdit->setText(m_filter->shortcutString());
    m_ui.includeByDefault->setChecked(m_filter->isIncludedByDefault());
    foreach (const QString &url, m_filter->remoteUrls()) {
        QListWidgetItem *item = new QListWidgetItem(url);
        m_ui.listWidget->addItem(item);
        item->setFlags(item->flags() | Qt::ItemIsEditable);
    }

    connect(m_ui.add, &QPushButton::clicked,
            this, &RemoteFilterOptions::addNewItem);
    connect(m_ui.remove, &QPushButton::clicked,
            this, &RemoteFilterOptions::removeItem);
    connect(m_ui.moveUp, &QPushButton::clicked,
            this, &RemoteFilterOptions::moveItemUp);
    connect(m_ui.moveDown, &QPushButton::clicked,
            this, &RemoteFilterOptions::moveItemDown);
    connect(m_ui.listWidget, &QListWidget::currentItemChanged,
            this, &RemoteFilterOptions::updateActionButtons);
    updateActionButtons();
}

void RemoteFilterOptions::addNewItem()
{
    QListWidgetItem *item = new QListWidgetItem(tr("Double-click to edit item."));
    m_ui.listWidget->addItem(item);
    item->setSelected(true);
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    m_ui.listWidget->setCurrentItem(item);
    m_ui.listWidget->editItem(item);
}

void RemoteFilterOptions::removeItem()
{
    if (QListWidgetItem *item = m_ui.listWidget->currentItem()) {
        m_ui.listWidget->removeItemWidget(item);
        delete item;
    }
}

void RemoteFilterOptions::moveItemUp()
{
    const int row = m_ui.listWidget->currentRow();
    if (row > 0) {
        QListWidgetItem *item = m_ui.listWidget->takeItem(row);
        m_ui.listWidget->insertItem(row - 1, item);
        m_ui.listWidget->setCurrentRow(row - 1);
    }
}

void RemoteFilterOptions::moveItemDown()
{
    const int row = m_ui.listWidget->currentRow();
    if (row >= 0 && row < m_ui.listWidget->count() - 1) {
        QListWidgetItem *item = m_ui.listWidget->takeItem(row);
        m_ui.listWidget->insertItem(row + 1, item);
        m_ui.listWidget->setCurrentRow(row + 1);
    }
}

void RemoteFilterOptions::updateActionButtons()
{
    m_ui.remove->setEnabled(m_ui.listWidget->currentItem());
    const int row = m_ui.listWidget->currentRow();
    m_ui.moveUp->setEnabled(row > 0);
    m_ui.moveDown->setEnabled(row >= 0 && row < m_ui.listWidget->count() - 1);
}

// -- RemoteHelpFilter

RemoteHelpFilter::RemoteHelpFilter()
{
    setId("RemoteHelpFilter");
    setDisplayName(tr("Web Search"));
    setIncludedByDefault(false);
    setShortcutString("r");

    m_remoteUrls.append("https://www.bing.com/search?q=%1");
    m_remoteUrls.append("https://www.google.com/search?q=%1");
    m_remoteUrls.append("https://search.yahoo.com/search?p=%1");
    m_remoteUrls.append("https://stackoverflow.com/search?q=%1");
    m_remoteUrls.append("http://en.cppreference.com/mwiki/index.php?title=Special%3ASearch&search=%1");
    m_remoteUrls.append("https://en.wikipedia.org/w/index.php?search=%1");
}

RemoteHelpFilter::~RemoteHelpFilter()
{
}

QList<Core::LocatorFilterEntry> RemoteHelpFilter::matchesFor(QFutureInterface<Core::LocatorFilterEntry> &future, const QString &entry)
{
    QList<Core::LocatorFilterEntry> entries;
    foreach (const QString &url, remoteUrls()) {
        if (future.isCanceled())
            break;
        const QString name = url.arg(entry);
        Core::LocatorFilterEntry filterEntry(this, name, QVariant(), m_icon);
        filterEntry.highlightInfo = {name.lastIndexOf(entry), entry.length()};
        entries.append(filterEntry);
    }
    return entries;
}

void RemoteHelpFilter::accept(Core::LocatorFilterEntry selection,
                              QString *newText, int *selectionStart, int *selectionLength) const
{
    Q_UNUSED(newText)
    Q_UNUSED(selectionStart)
    Q_UNUSED(selectionLength)
    const QString &url = selection.displayName;
    if (!url.isEmpty())
        emit linkActivated(url);
}

void RemoteHelpFilter::refresh(QFutureInterface<void> &future)
{
    Q_UNUSED(future)
    // Nothing to refresh
}

QByteArray RemoteHelpFilter::saveState() const
{
    QByteArray value;
    QDataStream out(&value, QIODevice::WriteOnly);
    out << m_remoteUrls.join(QLatin1Char('^'));
    out << shortcutString();
    out << isIncludedByDefault();
    return value;
}

void RemoteHelpFilter::restoreState(const QByteArray &state)
{
    QDataStream in(state);

    QString value;
    in >> value;
    m_remoteUrls = value.split(QLatin1Char('^'), QString::SkipEmptyParts);

    QString shortcut;
    in >> shortcut;
    setShortcutString(shortcut);

    bool defaultFilter;
    in >> defaultFilter;
    setIncludedByDefault(defaultFilter);
}

bool RemoteHelpFilter::openConfigDialog(QWidget *parent, bool &needsRefresh)
{
    Q_UNUSED(needsRefresh)
    RemoteFilterOptions optionsDialog(this, parent);
    if (optionsDialog.exec() == QDialog::Accepted) {
        QMutexLocker lock(&m_mutex); Q_UNUSED(lock)
        m_remoteUrls.clear();
        setIncludedByDefault(optionsDialog.m_ui.includeByDefault->isChecked());
        setShortcutString(optionsDialog.m_ui.shortcutEdit->text().trimmed());
        for (int i = 0; i < optionsDialog.m_ui.listWidget->count(); ++i)
            m_remoteUrls.append(optionsDialog.m_ui.listWidget->item(i)->text());
        return true;
    }
    return true;
}

QStringList RemoteHelpFilter::remoteUrls() const
{
    QMutexLocker lock(&m_mutex); Q_UNUSED(lock)
    return m_remoteUrls;
}

    } // namespace Internal
} // namespace Help
