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

#include "urllocatorfilter.h"

#include <utils/stringutils.h>

#include <QDesktopServices>
#include <QMutexLocker>
#include <QUrl>

using namespace Utils;

namespace Core {
namespace Internal {

UrlFilterOptions::UrlFilterOptions(UrlLocatorFilter *filter, QWidget *parent)
    : QDialog(parent)
    , m_filter(filter)
{
    m_ui.setupUi(this);
    setWindowTitle(ILocatorFilter::msgConfigureDialogTitle());
    m_ui.prefixLabel->setText(Core::ILocatorFilter::msgPrefixLabel());
    m_ui.prefixLabel->setToolTip(Core::ILocatorFilter::msgPrefixToolTip());
    m_ui.includeByDefault->setText(Core::ILocatorFilter::msgIncludeByDefault());
    m_ui.includeByDefault->setToolTip(Core::ILocatorFilter::msgIncludeByDefaultToolTip());
    m_ui.shortcutEdit->setText(m_filter->shortcutString());
    m_ui.includeByDefault->setChecked(m_filter->isIncludedByDefault());
    m_ui.nameEdit->setText(filter->displayName());
    m_ui.nameEdit->selectAll();
    m_ui.nameLabel->setVisible(filter->isCustomFilter());
    m_ui.nameEdit->setVisible(filter->isCustomFilter());
    m_ui.listWidget->setToolTip(
        tr("Add \"%1\" placeholder for the query string.\nDouble-click to edit item."));

    const QStringList remoteUrls = m_filter->remoteUrls();
    for (const QString &url : remoteUrls) {
        auto item = new QListWidgetItem(url);
        m_ui.listWidget->addItem(item);
        item->setFlags(item->flags() | Qt::ItemIsEditable);
    }

    connect(m_ui.add, &QPushButton::clicked, this, &UrlFilterOptions::addNewItem);
    connect(m_ui.remove, &QPushButton::clicked, this, &UrlFilterOptions::removeItem);
    connect(m_ui.moveUp, &QPushButton::clicked, this, &UrlFilterOptions::moveItemUp);
    connect(m_ui.moveDown, &QPushButton::clicked, this, &UrlFilterOptions::moveItemDown);
    connect(m_ui.listWidget,
            &QListWidget::currentItemChanged,
            this,
            &UrlFilterOptions::updateActionButtons);
    updateActionButtons();
}

void UrlFilterOptions::addNewItem()
{
    QListWidgetItem *item = new QListWidgetItem("https://www.example.com/search?query=%1");
    m_ui.listWidget->addItem(item);
    item->setSelected(true);
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    m_ui.listWidget->setCurrentItem(item);
    m_ui.listWidget->editItem(item);
}

void UrlFilterOptions::removeItem()
{
    if (QListWidgetItem *item = m_ui.listWidget->currentItem()) {
        m_ui.listWidget->removeItemWidget(item);
        delete item;
    }
}

void UrlFilterOptions::moveItemUp()
{
    const int row = m_ui.listWidget->currentRow();
    if (row > 0) {
        QListWidgetItem *item = m_ui.listWidget->takeItem(row);
        m_ui.listWidget->insertItem(row - 1, item);
        m_ui.listWidget->setCurrentRow(row - 1);
    }
}

void UrlFilterOptions::moveItemDown()
{
    const int row = m_ui.listWidget->currentRow();
    if (row >= 0 && row < m_ui.listWidget->count() - 1) {
        QListWidgetItem *item = m_ui.listWidget->takeItem(row);
        m_ui.listWidget->insertItem(row + 1, item);
        m_ui.listWidget->setCurrentRow(row + 1);
    }
}

void UrlFilterOptions::updateActionButtons()
{
    m_ui.remove->setEnabled(m_ui.listWidget->currentItem());
    const int row = m_ui.listWidget->currentRow();
    m_ui.moveUp->setEnabled(row > 0);
    m_ui.moveDown->setEnabled(row >= 0 && row < m_ui.listWidget->count() - 1);
}

} // namespace Internal

// -- UrlLocatorFilter

/*!
    \class Core::UrlLocatorFilter
    \inmodule QtCreator
    \internal
*/

UrlLocatorFilter::UrlLocatorFilter(Id id)
    : UrlLocatorFilter(tr("URL Template"), id)
{}

UrlLocatorFilter::UrlLocatorFilter(const QString &displayName, Id id)
{
    setId(id);
    setDisplayName(displayName);
    setIncludedByDefault(false);
}

UrlLocatorFilter::~UrlLocatorFilter() = default;

QList<Core::LocatorFilterEntry> UrlLocatorFilter::matchesFor(
    QFutureInterface<Core::LocatorFilterEntry> &future, const QString &entry)
{
    QList<Core::LocatorFilterEntry> entries;
    const QStringList urls = remoteUrls();
    for (const QString &url : urls) {
        if (future.isCanceled())
            break;
        const QString name = url.arg(entry);
        Core::LocatorFilterEntry filterEntry(this, name, QVariant(), m_icon);
        filterEntry.highlightInfo = {int(name.lastIndexOf(entry)), int(entry.length())};
        entries.append(filterEntry);
    }
    return entries;
}

void UrlLocatorFilter::accept(Core::LocatorFilterEntry selection,
                              QString *newText,
                              int *selectionStart,
                              int *selectionLength) const
{
    Q_UNUSED(newText)
    Q_UNUSED(selectionStart)
    Q_UNUSED(selectionLength)
    const QString &url = selection.displayName;
    if (!url.isEmpty())
        QDesktopServices::openUrl(url);
}

void UrlLocatorFilter::refresh(QFutureInterface<void> &future)
{
    Q_UNUSED(future)
    // Nothing to refresh
}

QByteArray UrlLocatorFilter::saveState() const
{
    QByteArray value;
    QDataStream out(&value, QIODevice::WriteOnly);
    out << m_remoteUrls.join('^');
    out << shortcutString();
    out << isIncludedByDefault();
    out << displayName();
    return value;
}

void UrlLocatorFilter::restoreState(const QByteArray &state)
{
    QDataStream in(state);

    QString value;
    in >> value;
    m_remoteUrls = value.split('^', Qt::SkipEmptyParts);

    QString shortcut;
    in >> shortcut;
    setShortcutString(shortcut);

    bool defaultFilter;
    in >> defaultFilter;
    setIncludedByDefault(defaultFilter);

    if (!in.atEnd()) {
        QString name;
        in >> name;
        setDisplayName(name);
    }
}

bool UrlLocatorFilter::openConfigDialog(QWidget *parent, bool &needsRefresh)
{
    Q_UNUSED(needsRefresh)
    Internal::UrlFilterOptions optionsDialog(this, parent);
    if (optionsDialog.exec() == QDialog::Accepted) {
        QMutexLocker lock(&m_mutex); Q_UNUSED(lock)
        m_remoteUrls.clear();
        setIncludedByDefault(optionsDialog.m_ui.includeByDefault->isChecked());
        setShortcutString(optionsDialog.m_ui.shortcutEdit->text().trimmed());
        for (int i = 0; i < optionsDialog.m_ui.listWidget->count(); ++i)
            m_remoteUrls.append(optionsDialog.m_ui.listWidget->item(i)->text());
        if (isCustomFilter())
            setDisplayName(optionsDialog.m_ui.nameEdit->text());
        return true;
    }
    return true;
}

void UrlLocatorFilter::addDefaultUrl(const QString &urlTemplate)
{
    m_remoteUrls.append(urlTemplate);
}

QStringList UrlLocatorFilter::remoteUrls() const
{
    QMutexLocker lock(&m_mutex);
    Q_UNUSED(lock)
    return m_remoteUrls;
}

void UrlLocatorFilter::setIsCustomFilter(bool value)
{
    m_isCustomFilter = value;
}

bool UrlLocatorFilter::isCustomFilter() const
{
    return m_isCustomFilter;
}

} // namespace Core
