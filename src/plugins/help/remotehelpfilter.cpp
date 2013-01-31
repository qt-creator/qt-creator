/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "remotehelpfilter.h"

#include <QUrl>

namespace Help {
    namespace Internal {

RemoteFilterOptions::RemoteFilterOptions(RemoteHelpFilter *filter, QWidget *parent)
    : QDialog(parent)
    , m_filter(filter)
{
    m_ui.setupUi(this);
    m_ui.shortcutEdit->setText(m_filter->shortcutString());
    m_ui.limitCheck->setChecked(!m_filter->isIncludedByDefault());
    foreach (const QString &url, m_filter->remoteUrls()) {
        QListWidgetItem *item = new QListWidgetItem(url);
        m_ui.listWidget->addItem(item);
        item->setFlags(item->flags() | Qt::ItemIsEditable);
    }

    connect(m_ui.add, SIGNAL(clicked()), this, SLOT(addNewItem()));
    connect(m_ui.remove, SIGNAL(clicked()), this, SLOT(removeItem()));
    connect(m_ui.listWidget, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)), SLOT(updateRemoveButton()));
    updateRemoveButton();
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

void RemoteFilterOptions::updateRemoveButton()
{
    m_ui.remove->setEnabled(m_ui.listWidget->currentItem());
}

// -- RemoteHelpFilter

RemoteHelpFilter::RemoteHelpFilter()
{
    setId("RemoteHelpFilter");
    setDisplayName(tr("Web Search"));
    setIncludedByDefault(false);
    setShortcutString(QLatin1String("r"));

    m_remoteUrls.append(QLatin1String("http://www.bing.com/search?q=%1"));
    m_remoteUrls.append(QLatin1String("http://www.google.com/search?q=%1"));
    m_remoteUrls.append(QLatin1String("http://search.yahoo.com/search?p=%1"));
    m_remoteUrls.append(QLatin1String("http://www.cplusplus.com/reference/stl/%1"));
    m_remoteUrls.append(QLatin1String("http://en.wikipedia.org/w/index.php?search=%1"));
}

RemoteHelpFilter::~RemoteHelpFilter()
{
}

QList<Locator::FilterEntry> RemoteHelpFilter::matchesFor(QFutureInterface<Locator::FilterEntry> &future, const QString &pattern)
{
    QList<Locator::FilterEntry> entries;
    foreach (const QString &url, m_remoteUrls) {
        if (future.isCanceled())
            break;

        entries.append(Locator::FilterEntry(this, url.arg(pattern), QVariant(),
            m_icon));
    }
    return entries;
}

void RemoteHelpFilter::accept(Locator::FilterEntry selection) const
{
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
    out << m_remoteUrls.join(QLatin1String("^"));
    out << shortcutString();
    out << isIncludedByDefault();
    return value;
}

bool RemoteHelpFilter::restoreState(const QByteArray &state)
{
    QDataStream in(state);

    QString value;
    in >> value;
    m_remoteUrls = value.split(QLatin1String("^"), QString::SkipEmptyParts);

    QString shortcut;
    in >> shortcut;
    setShortcutString(shortcut);

    bool defaultFilter;
    in >> defaultFilter;
    setIncludedByDefault(defaultFilter);

    return true;
}

bool RemoteHelpFilter::openConfigDialog(QWidget *parent, bool &needsRefresh)
{
    Q_UNUSED(needsRefresh)
    RemoteFilterOptions optionsDialog(this, parent);
    if (optionsDialog.exec() == QDialog::Accepted) {
        m_remoteUrls.clear();
        setIncludedByDefault(!optionsDialog.m_ui.limitCheck->isChecked());
        setShortcutString(optionsDialog.m_ui.shortcutEdit->text().trimmed());
        for (int i = 0; i < optionsDialog.m_ui.listWidget->count(); ++i)
            m_remoteUrls.append(optionsDialog.m_ui.listWidget->item(i)->text());
        return true;
    }
    return true;
}

    } // namespace Internal
} // namespace Help
