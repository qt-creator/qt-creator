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

#include "directoryfilter.h"

#include <QtCore/QDir>
#include <QtCore/QStack>
#include <QtGui/QCompleter>
#include <QtGui/QFileDialog>
#include <QtGui/QMessageBox>

#include <qtconcurrent/QtConcurrentTools>

using namespace QuickOpen;
using namespace QuickOpen::Internal;

DirectoryFilter::DirectoryFilter()
  : m_name(tr("Generic Directory Filter")),
    m_filters(QStringList() << "*.h" << "*.cpp" << "*.ui" << "*.qrc")
{
    setIncludedByDefault(true);
}

QByteArray DirectoryFilter::saveState() const
{
    QMutexLocker locker(&m_lock);
    QByteArray value;
    QDataStream out(&value, QIODevice::WriteOnly);
    out << m_name;
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

    QStringList dirEntries;
    QString shortcut;
    bool defaultFilter;

    QDataStream in(state);
    in >> m_name;
    in >> dirEntries;
    in >> m_filters;
    in >> shortcut;
    in >> defaultFilter;
    in >> m_files;

    setShortcutString(shortcut);
    setIncludedByDefault(defaultFilter);

    // ### TODO throw that out again:
    // clear the list of directories of empty entries (which might at least happen at a format change)
    m_directories.clear();
    foreach (const QString &dir, dirEntries) {
        if (dir.isEmpty())
            continue;
        m_directories.append(dir);
    }

    generateFileNames();
    return true;
}

bool DirectoryFilter::openConfigDialog(QWidget *parent, bool &needsRefresh)
{
    bool success = false;
    QDialog dialog(parent);
    m_dialog = &dialog;
    m_ui.setupUi(&dialog);
    dialog.setWindowTitle(tr("Filter Configuration"));
    connect(m_ui.addButton, SIGNAL(clicked()),
            this, SLOT(addDirectory()), Qt::DirectConnection);
    connect(m_ui.editButton, SIGNAL(clicked()),
            this, SLOT(editDirectory()), Qt::DirectConnection);
    connect(m_ui.removeButton, SIGNAL(clicked()),
            this, SLOT(removeDirectory()), Qt::DirectConnection);
    connect(m_ui.directoryList, SIGNAL(itemSelectionChanged()),
            this, SLOT(updateOptionButtons()), Qt::DirectConnection);
    m_ui.nameEdit->setText(m_name);
    m_ui.nameEdit->selectAll();
    m_ui.directoryList->clear();
    m_ui.directoryList->addItems(m_directories);
    m_ui.fileTypeEdit->setText(m_filters.join(tr(",")));
    m_ui.shortcutEdit->setText(shortcutString());
    m_ui.defaultFlag->setChecked(!isIncludedByDefault());
    updateOptionButtons();
    if (dialog.exec() == QDialog::Accepted) {
        QMutexLocker locker(&m_lock);
        bool directoriesChanged = false;
        QStringList oldDirectories = m_directories;
        QStringList oldFilters = m_filters;
        m_name = m_ui.nameEdit->text().trimmed();
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
        m_filters = m_ui.fileTypeEdit->text().trimmed().split(tr(","));
        setShortcutString(m_ui.shortcutEdit->text().trimmed());
        setIncludedByDefault(!m_ui.defaultFlag->isChecked());
        if (directoriesChanged || oldFilters != m_filters)
            needsRefresh = true;
        success = true;
    }
    return success;
}

void DirectoryFilter::addDirectory()
{
    QString dir = QFileDialog::getExistingDirectory(m_dialog, tr("Choose a directory to add"));
    if (!dir.isEmpty()) {
        m_ui.directoryList->addItem(dir);
    }
}

void DirectoryFilter::editDirectory()
{
    if (m_ui.directoryList->selectedItems().count() < 1)
        return;
    QListWidgetItem *currentItem = m_ui.directoryList->selectedItems().at(0);
    QString dir = QFileDialog::getExistingDirectory(m_dialog, tr("Choose a directory to add"),
                                                    currentItem->text());
    if (!dir.isEmpty()) {
        currentItem->setText(dir);
    }
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

void DirectoryFilter::refresh(QFutureInterface<void> &future)
{
    const int MAX = 360;
    future.setProgressRange(0, MAX);
    if (m_directories.count() < 1) {
        QMutexLocker locker(&m_lock);
        m_files.clear();
        generateFileNames();
        future.setProgressValueAndText(MAX, tr("%1 filter update: 0 files").arg(m_name));
        return;
    }
    int progress = 0;
    int MAX_PER = MAX;
    QStringList files;
    QStack<QDir> dirs;
    QStack<int> progressValues;
    QStack<bool> processedValues;
    { // initialize
        QMutexLocker locker(&m_lock);
        MAX_PER = MAX/m_directories.count();
        foreach (const QString &directoryEntry, m_directories) {
            if (!directoryEntry.isEmpty()) {
                dirs.push(QDir(directoryEntry));
                progressValues.push(MAX_PER);
                processedValues.push(false);
            }
        }
    }
    while (!dirs.isEmpty() && !future.isCanceled()) {
        if (future.isProgressUpdateNeeded()) {
            future.setProgressValueAndText(progress, tr("%1 filter update: %2 files").arg(m_name).arg(files.size()));
        }
        QDir dir = dirs.pop();
        int dirProgressMax = progressValues.pop();
        bool processed = processedValues.pop();
        if (dir.exists()) {
            QStringList subDirs;
            if (!processed) {
                subDirs = dir.entryList(QDir::Dirs|QDir::Hidden|QDir::NoDotAndDotDot,
                    QDir::Name|QDir::IgnoreCase|QDir::LocaleAware);
            }
            if (subDirs.isEmpty()) {
                QStringList fileEntries = dir.entryList(m_filters,
                    QDir::Files|QDir::Hidden,
                    QDir::Name|QDir::IgnoreCase|QDir::LocaleAware);
                foreach (const QString &file, fileEntries)
                    files.append(dir.path()+"/"+file);
                progress += dirProgressMax;
            } else {
                int subProgress = dirProgressMax/(subDirs.size()+1);
                int selfProgress = subProgress + dirProgressMax%(subDirs.size()+1);
                dirs.push(dir);
                progressValues.push(selfProgress);
                processedValues.push(true);
                foreach (const QString &directory, subDirs) {
                    dirs.push(QDir(dir.path()+"/"+directory));
                    progressValues.push(subProgress);
                    processedValues.push(false);
                }
            }
        } else {
            progress += dirProgressMax;
        }
    }

    if (!future.isCanceled()) {
        QMutexLocker locker(&m_lock);
        m_files = files;
        generateFileNames();
        future.setProgressValue(MAX);
    } else {
        future.setProgressValueAndText(progress, tr("%1 filter update: canceled").arg(m_name));
    }
}
