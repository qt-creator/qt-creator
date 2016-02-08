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

#include "cleandialog.h"
#include "ui_cleandialog.h"
#include "vcsoutputwindow.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <utils/runextensions.h>

#include <QStandardItemModel>
#include <QMessageBox>
#include <QApplication>
#include <QStyle>
#include <QIcon>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include <QDateTime>
#include <QTimer>

namespace VcsBase {
namespace Internal {

enum { nameColumn, columnCount };
enum { fileNameRole = Qt::UserRole, isDirectoryRole = Qt::UserRole + 1 };

// Helper for recursively removing files.
static void removeFileRecursion(QFutureInterface<void> &futureInterface,
                                const QFileInfo &f, QString *errorMessage)
{
    if (futureInterface.isCanceled())
        return;
    // The version control system might list files/directory in arbitrary
    // order, causing files to be removed from parent directories.
    if (!f.exists())
        return;
    if (f.isDir()) {
        const QDir dir(f.absoluteFilePath());
        foreach (const QFileInfo &fi, dir.entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden))
            removeFileRecursion(futureInterface, fi, errorMessage);
        QDir parent = f.absoluteDir();
        if (!parent.rmdir(f.fileName()))
            errorMessage->append(VcsBase::CleanDialog::tr("The directory %1 could not be deleted.").
                                 arg(QDir::toNativeSeparators(f.absoluteFilePath())));
        return;
    }
    if (!QFile::remove(f.absoluteFilePath())) {
        if (!errorMessage->isEmpty())
            errorMessage->append(QLatin1Char('\n'));
        errorMessage->append(VcsBase::CleanDialog::tr("The file %1 could not be deleted.").
                             arg(QDir::toNativeSeparators(f.absoluteFilePath())));
    }
}

// Cleaning files in the background
static void runCleanFiles(QFutureInterface<void> &futureInterface,
                          const QString &repository, const QStringList &files,
                          const std::function<void(const QString&)> &errorHandler)
{
    QString errorMessage;
    futureInterface.setProgressRange(0, files.size());
    futureInterface.setProgressValue(0);
    foreach (const QString &name, files) {
        removeFileRecursion(futureInterface, QFileInfo(name), &errorMessage);
        if (futureInterface.isCanceled())
            break;
        futureInterface.setProgressValue(futureInterface.progressValue() + 1);
    }
    if (!errorMessage.isEmpty()) {
        // Format and emit error.
        const QString msg = CleanDialog::tr("There were errors when cleaning the repository %1:").
                            arg(QDir::toNativeSeparators(repository));
        errorMessage.insert(0, QLatin1Char('\n'));
        errorMessage.insert(0, msg);
        errorHandler(errorMessage);
    }
}

static void handleError(const QString &errorMessage)
{
    QTimer::singleShot(0, VcsOutputWindow::instance(), [errorMessage]() {
        VcsOutputWindow::instance()->appendSilently(errorMessage);
    });
}

// ---------------- CleanDialogPrivate ----------------

class CleanDialogPrivate
{
public:
    CleanDialogPrivate();

    Internal::Ui::CleanDialog ui;
    QStandardItemModel *m_filesModel;
    QString m_workingDirectory;
};

CleanDialogPrivate::CleanDialogPrivate() :
    m_filesModel(new QStandardItemModel(0, columnCount))
{
}

} // namespace Internal

/*!
    \class VcsBase::CleanDialog

    \brief The CleanDialog class provides a file selector dialog for files not
    under version control.

    Completely clean a directory under version control
    from all files that are not under version control based on a list
    generated from the version control system. Presents the user with
    a checkable list of files and/or directories. Double click opens a file.
*/

CleanDialog::CleanDialog(QWidget *parent) :
    QDialog(parent),
    d(new Internal::CleanDialogPrivate)
{
    setModal(true);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    d->ui.setupUi(this);
    d->ui.buttonBox->addButton(tr("Delete..."), QDialogButtonBox::AcceptRole);

    d->m_filesModel->setHorizontalHeaderLabels(QStringList(tr("Name")));
    d->ui.filesTreeView->setModel(d->m_filesModel);
    d->ui.filesTreeView->setUniformRowHeights(true);
    d->ui.filesTreeView->setSelectionMode(QAbstractItemView::NoSelection);
    d->ui.filesTreeView->setAllColumnsShowFocus(true);
    d->ui.filesTreeView->setRootIsDecorated(false);
    connect(d->ui.filesTreeView, &QAbstractItemView::doubleClicked,
            this, &CleanDialog::slotDoubleClicked);
    connect(d->ui.selectAllCheckBox, &QAbstractButton::clicked,
            this, &CleanDialog::selectAllItems);
    connect(d->ui.filesTreeView, &QAbstractItemView::clicked,
            this, &CleanDialog::updateSelectAllCheckBox);
}

CleanDialog::~CleanDialog()
{
    delete d;
}

void CleanDialog::setFileList(const QString &workingDirectory, const QStringList &files,
                              const QStringList &ignoredFiles)
{
    d->m_workingDirectory = workingDirectory;
    d->ui.groupBox->setTitle(tr("Repository: %1").
                             arg(QDir::toNativeSeparators(workingDirectory)));
    if (const int oldRowCount = d->m_filesModel->rowCount())
        d->m_filesModel->removeRows(0, oldRowCount);

    foreach (const QString &fileName, files)
        addFile(workingDirectory, fileName, true);
    foreach (const QString &fileName, ignoredFiles)
        addFile(workingDirectory, fileName, false);

    for (int c = 0; c < d->m_filesModel->columnCount(); c++)
        d->ui.filesTreeView->resizeColumnToContents(c);

    if (ignoredFiles.isEmpty())
        d->ui.selectAllCheckBox->setChecked(true);
}

void CleanDialog::addFile(const QString &workingDirectory, QString fileName, bool checked)
{
    QStyle *style = QApplication::style();
    const QIcon folderIcon = style->standardIcon(QStyle::SP_DirIcon);
    const QIcon fileIcon = style->standardIcon(QStyle::SP_FileIcon);
    const QChar slash = QLatin1Char('/');
    // Clean the trailing slash of directories
    if (fileName.endsWith(slash))
        fileName.chop(1);
    QFileInfo fi(workingDirectory + slash + fileName);
    bool isDir = fi.isDir();
    if (isDir)
        checked = false;
    auto nameItem = new QStandardItem(QDir::toNativeSeparators(fileName));
    nameItem->setFlags(Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
    nameItem->setIcon(isDir ? folderIcon : fileIcon);
    nameItem->setCheckable(true);
    nameItem->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
    nameItem->setData(QVariant(fi.absoluteFilePath()), Internal::fileNameRole);
    nameItem->setData(QVariant(isDir), Internal::isDirectoryRole);
    // Tooltip with size information
    if (fi.isFile()) {
        const QString lastModified = fi.lastModified().toString(Qt::DefaultLocaleShortDate);
        nameItem->setToolTip(tr("%n bytes, last modified %1.", 0, fi.size()).arg(lastModified));
    }
    d->m_filesModel->appendRow(nameItem);
}

QStringList CleanDialog::checkedFiles() const
{
    QStringList rc;
    if (const int rowCount = d->m_filesModel->rowCount()) {
        for (int r = 0; r < rowCount; r++) {
            const QStandardItem *item = d->m_filesModel->item(r, 0);
            if (item->checkState() == Qt::Checked)
                rc.push_back(item->data(Internal::fileNameRole).toString());
        }
    }
    return rc;
}

void CleanDialog::accept()
{
    if (promptToDelete())
        QDialog::accept();
}

bool CleanDialog::promptToDelete()
{
    // Prompt the user and delete files
    const QStringList selectedFiles = checkedFiles();
    if (selectedFiles.isEmpty())
        return true;

    if (QMessageBox::question(this, tr("Delete"),
                              tr("Do you want to delete %n files?", 0, selectedFiles.size()),
                              QMessageBox::Yes|QMessageBox::No, QMessageBox::Yes) != QMessageBox::Yes)
        return false;

    // Remove in background
    QFuture<void> task = Utils::runAsync(Internal::runCleanFiles, d->m_workingDirectory,
                                         selectedFiles, Internal::handleError);

    const QString taskName = tr("Cleaning \"%1\"").
                             arg(QDir::toNativeSeparators(d->m_workingDirectory));
    Core::ProgressManager::addTask(task, taskName, "VcsBase.cleanRepository");
    return true;
}

void CleanDialog::slotDoubleClicked(const QModelIndex &index)
{
    // Open file on doubleclick
    if (const QStandardItem *item = d->m_filesModel->itemFromIndex(index))
        if (!item->data(Internal::isDirectoryRole).toBool()) {
            const QString fname = item->data(Internal::fileNameRole).toString();
            Core::EditorManager::openEditor(fname);
    }
}

void CleanDialog::selectAllItems(bool checked)
{
    if (const int rowCount = d->m_filesModel->rowCount()) {
        for (int r = 0; r < rowCount; ++r) {
            QStandardItem *item = d->m_filesModel->item(r, 0);
            item->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
        }
    }
}

void CleanDialog::updateSelectAllCheckBox()
{
    bool checked = true;
    if (const int rowCount = d->m_filesModel->rowCount()) {
        for (int r = 0; r < rowCount; ++r) {
            const QStandardItem *item = d->m_filesModel->item(r, 0);
            if (item->checkState() == Qt::Unchecked) {
                checked = false;
                break;
            }
        }
        d->ui.selectAllCheckBox->setChecked(checked);
    }
}

} // namespace VcsBase
