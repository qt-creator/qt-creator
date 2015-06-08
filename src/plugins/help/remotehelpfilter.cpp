/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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

    m_remoteUrls.append(QLatin1String("https://www.bing.com/search?q=%1"));
    m_remoteUrls.append(QLatin1String("https://www.google.com/search?q=%1"));
    m_remoteUrls.append(QLatin1String("https://search.yahoo.com/search?p=%1"));
    m_remoteUrls.append(QLatin1String("https://www.cplusplus.com/reference/stl/%1"));
    m_remoteUrls.append(QLatin1String("https://en.wikipedia.org/w/index.php?search=%1"));
}

RemoteHelpFilter::~RemoteHelpFilter()
{
}

QList<Core::LocatorFilterEntry> RemoteHelpFilter::matchesFor(QFutureInterface<Core::LocatorFilterEntry> &future, const QString &pattern)
{
    QList<Core::LocatorFilterEntry> entries;
    foreach (const QString &url, remoteUrls()) {
        if (future.isCanceled())
            break;

        entries.append(Core::LocatorFilterEntry(this, url.arg(pattern), QVariant(),
            m_icon));
    }
    return entries;
}

void RemoteHelpFilter::accept(Core::LocatorFilterEntry selection) const
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
    out << m_remoteUrls.join(QLatin1Char('^'));
    out << shortcutString();
    out << isIncludedByDefault();
    return value;
}

bool RemoteHelpFilter::restoreState(const QByteArray &state)
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

    return true;
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
