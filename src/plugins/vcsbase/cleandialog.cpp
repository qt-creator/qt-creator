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

#include "cleandialog.h"
#include "ui_cleandialog.h"
#include "vcsbaseoutputwindow.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/progressmanager.h>

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
#include <QFuture>
#include <QtConcurrentRun>

namespace VcsBase {
namespace Internal {

enum { nameColumn, columnCount };
enum { fileNameRole = Qt::UserRole, isDirectoryRole = Qt::UserRole + 1 };

// Helper for recursively removing files.
static void removeFileRecursion(const QFileInfo &f, QString *errorMessage)
{
    // The version control system might list files/directory in arbitrary
    // order, causing files to be removed from parent directories.
    if (!f.exists())
        return;
    if (f.isDir()) {
        const QDir dir(f.absoluteFilePath());
        foreach (const QFileInfo &fi, dir.entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden))
            removeFileRecursion(fi, errorMessage);
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

// A QFuture task for cleaning files in the background.
// Emits error signal if not all files can be deleted.
class CleanFilesTask : public QObject
{
    Q_OBJECT

public:
    explicit CleanFilesTask(const QString &repository, const QStringList &files);

    void run();

signals:
    void error(const QString &e);

private:
    const QString m_repository;
    const QStringList m_files;

    QString m_errorMessage;
};

CleanFilesTask::CleanFilesTask(const QString &repository, const QStringList &files) :
    m_repository(repository), m_files(files)
{
}

void CleanFilesTask::run()
{
    foreach (const QString &name, m_files)
        removeFileRecursion(QFileInfo(name), &m_errorMessage);
    if (!m_errorMessage.isEmpty()) {
        // Format and emit error.
        const QString msg = CleanDialog::tr("There were errors when cleaning the repository %1:").
                            arg(QDir::toNativeSeparators(m_repository));
        m_errorMessage.insert(0, QLatin1Char('\n'));
        m_errorMessage.insert(0, msg);
        emit error(m_errorMessage);
    }
    // Run in background, need to delete ourselves
    this->deleteLater();
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

    \brief File selector dialog for files not under version control.

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
    connect(d->ui.filesTreeView, SIGNAL(doubleClicked(QModelIndex)),
            this, SLOT(slotDoubleClicked(QModelIndex)));
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
    QStandardItem *nameItem = new QStandardItem(QDir::toNativeSeparators(fileName));
    nameItem->setFlags(Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
    nameItem->setIcon(isDir ? folderIcon : fileIcon);
    nameItem->setCheckable(true);
    nameItem->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
    nameItem->setData(QVariant(fi.absoluteFilePath()), Internal::fileNameRole);
    nameItem->setData(QVariant(isDir), Internal::isDirectoryRole);
    // Tooltip with size information
    if (fi.isFile()) {
        const QString lastModified = fi.lastModified().toString(Qt::DefaultLocaleShortDate);
        nameItem->setToolTip(tr("%n bytes, last modified %1", 0, fi.size()).arg(lastModified));
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
    Internal::CleanFilesTask *cleanTask = new Internal::CleanFilesTask(d->m_workingDirectory, selectedFiles);
    connect(cleanTask, SIGNAL(error(QString)),
            VcsBase::VcsBaseOutputWindow::instance(), SLOT(appendSilently(QString)),
            Qt::QueuedConnection);

    QFuture<void> task = QtConcurrent::run(cleanTask, &Internal::CleanFilesTask::run);
    const QString taskName = tr("Cleaning %1").
                             arg(QDir::toNativeSeparators(d->m_workingDirectory));
    Core::ICore::progressManager()->addTask(task, taskName,
                                                        QLatin1String("VcsBase.cleanRepository"));
    return true;
}

void CleanDialog::slotDoubleClicked(const QModelIndex &index)
{
    // Open file on doubleclick
    if (const QStandardItem *item = d->m_filesModel->itemFromIndex(index))
        if (!item->data(Internal::isDirectoryRole).toBool()) {
            const QString fname = item->data(Internal::fileNameRole).toString();
            Core::EditorManager::openEditor(fname, Core::Id(), Core::EditorManager::ModeSwitch);
    }
}

void CleanDialog::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        d->ui.retranslateUi(this);
        break;
    default:
        break;
    }
}

} // namespace VcsBase

#include "cleandialog.moc"
