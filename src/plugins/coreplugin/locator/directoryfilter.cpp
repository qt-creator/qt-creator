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

#include "directoryfilter.h"

#include <coreplugin/coreconstants.h>
#include <utils/algorithm.h>
#include <utils/filesearch.h>

#include <QFileDialog>
#include <QTimer>

using namespace Core;
using namespace Core::Internal;

DirectoryFilter::DirectoryFilter(Id id)
    : m_filters({"*.h", "*.cpp", "*.ui", "*.qrc"}),
      m_exclusionFilters({"*/.git/*", "*/.cvs/*", "*/.svn/*"})
{
    setId(id);
    setIncludedByDefault(true);
    setDisplayName(tr("Generic Directory Filter"));
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
    out << m_exclusionFilters;
    return value;
}

void DirectoryFilter::restoreState(const QByteArray &state)
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
    if (!in.atEnd()) // Qt Creator 4.3 and later
        in >> m_exclusionFilters;
    else
        m_exclusionFilters.clear();

    setDisplayName(name);
    setShortcutString(shortcut);
    setIncludedByDefault(defaultFilter);

    updateFileIterator();
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
    m_ui.filePatternLabel->setText(Utils::msgFilePatternLabel());
    m_ui.filePatternLabel->setBuddy(m_ui.filePattern);
    m_ui.filePattern->setToolTip(Utils::msgFilePatternToolTip());
    m_ui.filePattern->setText(Utils::transform(m_filters, &QDir::toNativeSeparators)
                              .join(QLatin1Char(',')));
    m_ui.exclusionPatternLabel->setText(Utils::msgExclusionPatternLabel());
    m_ui.exclusionPatternLabel->setBuddy(m_ui.exclusionPattern);
    m_ui.exclusionPattern->setToolTip(Utils::msgFilePatternToolTip());
    m_ui.exclusionPattern->setText(Utils::transform(m_exclusionFilters, &QDir::toNativeSeparators)
                                   .join(QLatin1Char(',')));
    m_ui.shortcutEdit->setText(shortcutString());
    m_ui.defaultFlag->setChecked(isIncludedByDefault());
    updateOptionButtons();
    if (dialog.exec() == QDialog::Accepted) {
        QMutexLocker locker(&m_lock);
        bool directoriesChanged = false;
        const QStringList oldDirectories = m_directories;
        const QStringList oldFilters = m_filters;
        const QStringList oldExclusionFilters = m_exclusionFilters;
        setDisplayName(m_ui.nameEdit->text().trimmed());
        m_directories.clear();
        const int oldCount = oldDirectories.count();
        const int newCount = m_ui.directoryList->count();
        if (oldCount != newCount)
            directoriesChanged = true;
        for (int i = 0; i < newCount; ++i) {
            m_directories.append(m_ui.directoryList->item(i)->text());
            if (!directoriesChanged && m_directories.at(i) != oldDirectories.at(i))
                directoriesChanged = true;
        }
        m_filters = Utils::splitFilterUiText(m_ui.filePattern->text());
        m_exclusionFilters = Utils::splitFilterUiText(m_ui.exclusionPattern->text());
        setShortcutString(m_ui.shortcutEdit->text().trimmed());
        setIncludedByDefault(m_ui.defaultFlag->isChecked());
        needsRefresh = directoriesChanged || oldFilters != m_filters
                || oldExclusionFilters != m_exclusionFilters;
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
            QTimer::singleShot(0, this, &DirectoryFilter::updateFileIterator);
            future.setProgressRange(0, 1);
            future.setProgressValueAndText(1, tr("%1 filter update: 0 files").arg(displayName()));
            return;
        }
        directories = m_directories;
    }
    Utils::SubDirFileIterator subDirIterator(directories, m_filters, m_exclusionFilters);
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
        QTimer::singleShot(0, this, &DirectoryFilter::updateFileIterator);
        future.setProgressValue(subDirIterator.maxProgress());
    } else {
        future.setProgressValueAndText(subDirIterator.currentProgress(), tr("%1 filter update: canceled").arg(displayName()));
    }
}
