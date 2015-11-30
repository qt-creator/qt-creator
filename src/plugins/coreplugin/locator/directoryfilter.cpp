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

#include "directoryfilter.h"

#include <coreplugin/coreconstants.h>
#include <utils/filesearch.h>

#include <QFileDialog>
#include <QTimer>

using namespace Core;
using namespace Core::Internal;

DirectoryFilter::DirectoryFilter(Id id)
    : m_dialog(0)
{
    setId(id);
    setIncludedByDefault(true);
    setDisplayName(tr("Generic Directory Filter"));

    m_filters.append(QLatin1String("*.h"));
    m_filters.append(QLatin1String("*.cpp"));
    m_filters.append(QLatin1String("*.ui"));
    m_filters.append(QLatin1String("*.qrc"));
}

QByteArray DirectoryFilter::saveState() const
{
    QMutexLocker locker(&m_lock);
    QByteArray value;
    QDataStream out(&value, QIODevice::WriteOnly);
    out << displayName();
    out << m_directories;
    out << m_filters;
    out << shortcutString();
    out << isIncludedByDefault();
    out << m_files;
    return value;
}

bool DirectoryFilter::restoreState(const QByteArray &state)
{
    QMutexLocker locker(&m_lock);

    QString name;
    QString shortcut;
    bool defaultFilter;

    QDataStream in(state);
    in >> name;
    in >> m_directories;
    in >> m_filters;
    in >> shortcut;
    in >> defaultFilter;
    in >> m_files;

    setDisplayName(name);
    setShortcutString(shortcut);
    setIncludedByDefault(defaultFilter);

    updateFileIterator();
    return true;
}

bool DirectoryFilter::openConfigDialog(QWidget *parent, bool &needsRefresh)
{
    bool success = false;
    QDialog dialog(parent);
    m_dialog = &dialog;
    m_ui.setupUi(&dialog);
    dialog.setWindowTitle(ILocatorFilter::msgConfigureDialogTitle());
    m_ui.prefixLabel->setText(ILocatorFilter::msgPrefixLabel());
    m_ui.prefixLabel->setToolTip(ILocatorFilter::msgPrefixToolTip());
    m_ui.defaultFlag->setText(ILocatorFilter::msgIncludeByDefault());
    m_ui.defaultFlag->setToolTip(ILocatorFilter::msgIncludeByDefaultToolTip());
    connect(m_ui.addButton, &QPushButton::clicked,
            this, &DirectoryFilter::addDirectory, Qt::DirectConnection);
    connect(m_ui.editButton, &QPushButton::clicked,
            this, &DirectoryFilter::editDirectory, Qt::DirectConnection);
    connect(m_ui.removeButton, &QPushButton::clicked,
            this, &DirectoryFilter::removeDirectory, Qt::DirectConnection);
    connect(m_ui.directoryList, &QListWidget::itemSelectionChanged,
            this, &DirectoryFilter::updateOptionButtons, Qt::DirectConnection);
    m_ui.nameEdit->setText(displayName());
    m_ui.nameEdit->selectAll();
    m_ui.directoryList->clear();
    m_ui.directoryList->addItems(m_directories);
    m_ui.fileTypeEdit->setText(m_filters.join(QLatin1Char(',')));
    m_ui.shortcutEdit->setText(shortcutString());
    m_ui.defaultFlag->setChecked(isIncludedByDefault());
    updateOptionButtons();
    if (dialog.exec() == QDialog::Accepted) {
        QMutexLocker locker(&m_lock);
        bool directoriesChanged = false;
        QStringList oldDirectories = m_directories;
        QStringList oldFilters = m_filters;
        setDisplayName(m_ui.nameEdit->text().trimmed());
        m_directories.clear();
        int oldCount = oldDirectories.count();
        int newCount = m_ui.directoryList->count();
        if (oldCount != newCount)
            directoriesChanged = true;
        for (int i = 0; i < newCount; ++i) {
            m_directories.append(m_ui.directoryList->item(i)->text());
            if (!directoriesChanged && m_directories.at(i) != oldDirectories.at(i))
                directoriesChanged = true;
        }
        m_filters = m_ui.fileTypeEdit->text().trimmed().split(QLatin1Char(','));
        setShortcutString(m_ui.shortcutEdit->text().trimmed());
        setIncludedByDefault(m_ui.defaultFlag->isChecked());
        if (directoriesChanged || oldFilters != m_filters)
            needsRefresh = true;
        success = true;
    }
    return success;
}

void DirectoryFilter::addDirectory()
{
    QString dir = QFileDialog::getExistingDirectory(m_dialog, tr("Select Directory"));
    if (!dir.isEmpty())
        m_ui.directoryList->addItem(dir);
}

void DirectoryFilter::editDirectory()
{
    if (m_ui.directoryList->selectedItems().count() < 1)
        return;
    QListWidgetItem *currentItem = m_ui.directoryList->selectedItems().at(0);
    QString dir = QFileDialog::getExistingDirectory(m_dialog, tr("Select Directory"),
                                                    currentItem->text());
    if (!dir.isEmpty())
        currentItem->setText(dir);
}

void DirectoryFilter::removeDirectory()
{
    if (m_ui.directoryList->selectedItems().count() < 1)
        return;
    QListWidgetItem *currentItem = m_ui.directoryList->selectedItems().at(0);
    delete m_ui.directoryList->takeItem(m_ui.directoryList->row(currentItem));
}

void DirectoryFilter::updateOptionButtons()
{
    bool haveSelectedItem = (m_ui.directoryList->selectedItems().count() > 0);
    m_ui.editButton->setEnabled(haveSelectedItem);
    m_ui.removeButton->setEnabled(haveSelectedItem);
}

void DirectoryFilter::updateFileIterator()
{
    setFileIterator(new BaseFileFilter::ListIterator(m_files));
}

void DirectoryFilter::refresh(QFutureInterface<void> &future)
{
    QStringList directories;
    {
        QMutexLocker locker(&m_lock);
        if (m_directories.count() < 1) {
            m_files.clear();
            QTimer::singleShot(0, this, SLOT(updateFileIterator()));
            future.setProgressRange(0, 1);
            future.setProgressValueAndText(1, tr("%1 filter update: 0 files").arg(displayName()));
            return;
        }
        directories = m_directories;
    }
    Utils::SubDirFileIterator subDirIterator(directories, m_filters);
    future.setProgressRange(0, subDirIterator.maxProgress());
    QStringList filesFound;
    auto end = subDirIterator.end();
    for (auto it = subDirIterator.begin(); it != end; ++it) {
        if (future.isCanceled())
            break;
        filesFound << (*it).filePath;
        if (future.isProgressUpdateNeeded()
                || future.progressValue() == 0 /*workaround for regression in Qt*/) {
            future.setProgressValueAndText(subDirIterator.currentProgress(),
                                           tr("%1 filter update: %n files", 0, filesFound.size()).arg(displayName()));
        }
    }

    if (!future.isCanceled()) {
        QMutexLocker locker(&m_lock);
        m_files = filesFound;
        QTimer::singleShot(0, this, SLOT(updateFileIterator()));
        future.setProgressValue(subDirIterator.maxProgress());
    } else {
        future.setProgressValueAndText(subDirIterator.currentProgress(), tr("%1 filter update: canceled").arg(displayName()));
    }
}
