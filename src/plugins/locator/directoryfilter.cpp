/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "directoryfilter.h"

#include <QDir>
#include <QStack>
#include <QCompleter>
#include <QFileDialog>
#include <QMessageBox>

#include <utils/QtConcurrentTools>
#include <utils/filesearch.h>

using namespace Locator;
using namespace Locator::Internal;

DirectoryFilter::DirectoryFilter()
  : m_name(tr("Generic Directory Filter")),
    m_filters(QStringList() << QLatin1String("*.h") << QLatin1String("*.cpp")
                            << QLatin1String("*.ui") << QLatin1String("*.qrc")),
    m_dialog(0)
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
    out << files();
    return value;
}

bool DirectoryFilter::restoreState(const QByteArray &state)
{
    QMutexLocker locker(&m_lock);

    QString shortcut;
    bool defaultFilter;

    QDataStream in(state);
    in >> m_name;
    in >> m_directories;
    in >> m_filters;
    in >> shortcut;
    in >> defaultFilter;
    in >> files();

    setShortcutString(shortcut);
    setIncludedByDefault(defaultFilter);

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
    m_ui.fileTypeEdit->setText(m_filters.join(QString(QLatin1Char(','))));
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
        m_filters = m_ui.fileTypeEdit->text().trimmed().split(QLatin1Char(','));
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
    QString dir = QFileDialog::getExistingDirectory(m_dialog, tr("Select Directory"));
    if (!dir.isEmpty()) {
        m_ui.directoryList->addItem(dir);
    }
}

void DirectoryFilter::editDirectory()
{
    if (m_ui.directoryList->selectedItems().count() < 1)
        return;
    QListWidgetItem *currentItem = m_ui.directoryList->selectedItems().at(0);
    QString dir = QFileDialog::getExistingDirectory(m_dialog, tr("Select Directory"),
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
    QStringList directories;
    {
        QMutexLocker locker(&m_lock);
        if (m_directories.count() < 1) {
            files().clear();
            generateFileNames();
            future.setProgressRange(0, 1);
            future.setProgressValueAndText(1, tr("%1 filter update: 0 files").arg(m_name));
            return;
        }
        directories = m_directories;
    }
    Utils::SubDirFileIterator it(directories, m_filters);
    future.setProgressRange(0, it.maxProgress());
    QStringList filesFound;
    while (!future.isCanceled() && it.hasNext()) {
        filesFound << it.next();
        if (future.isProgressUpdateNeeded()
                || future.progressValue() == 0 /*workaround for regression in Qt*/) {
            future.setProgressValueAndText(it.currentProgress(),
                                           tr("%1 filter update: %n files", 0, filesFound.size()).arg(m_name));
        }
    }

    if (!future.isCanceled()) {
        QMutexLocker locker(&m_lock);
        files() = filesFound;
        generateFileNames();
        future.setProgressValue(it.maxProgress());
    } else {
        future.setProgressValueAndText(it.currentProgress(), tr("%1 filter update: canceled").arg(m_name));
    }
}
