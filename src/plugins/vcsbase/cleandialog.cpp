// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cleandialog.h"

#include "vcsbasetr.h"
#include "vcsoutputwindow.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/progressmanager/progressmanager.h>

#include <utils/async.h>
#include <utils/layoutbuilder.h>

#include <QApplication>
#include <QCheckBox>
#include <QDateTime>
#include <QDebug>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGroupBox>
#include <QHeaderView>
#include <QIcon>
#include <QMessageBox>
#include <QStandardItemModel>
#include <QStyle>
#include <QTimer>
#include <QTreeView>

using namespace Utils;

namespace VcsBase {
namespace Internal {

enum { nameColumn, columnCount };
enum { fileNameRole = Qt::UserRole, isDirectoryRole = Qt::UserRole + 1 };

// Helper for recursively removing files.
static void removeFileRecursion(QPromise<void> &promise, const QFileInfo &f,
                                QString *errorMessage)
{
    if (promise.isCanceled())
        return;
    // The version control system might list files/directory in arbitrary
    // order, causing files to be removed from parent directories.
    if (!f.exists())
        return;
    if (f.isDir()) {
        const QDir dir(f.absoluteFilePath());
        const QList<QFileInfo> infos = dir.entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden);
        for (const QFileInfo &fi : infos)
            removeFileRecursion(promise, fi, errorMessage);
        QDir parent = f.absoluteDir();
        if (!parent.rmdir(f.fileName()))
            errorMessage->append(Tr::tr("The directory %1 could not be deleted.")
                                     .arg(QDir::toNativeSeparators(f.absoluteFilePath())));
        return;
    }
    if (!QFile::remove(f.absoluteFilePath())) {
        if (!errorMessage->isEmpty())
            errorMessage->append(QLatin1Char('\n'));
        errorMessage->append(Tr::tr("The file %1 could not be deleted.")
                                 .arg(QDir::toNativeSeparators(f.absoluteFilePath())));
    }
}

// Cleaning files in the background
static void runCleanFiles(QPromise<void> &promise, const FilePath &repository,
                          const QStringList &files,
                          const std::function<void(const QString&)> &errorHandler)
{
    QString errorMessage;
    promise.setProgressRange(0, files.size());
    promise.setProgressValue(0);
    int fileIndex = 0;
    for (const QString &name : files) {
        removeFileRecursion(promise, QFileInfo(name), &errorMessage);
        if (promise.isCanceled())
            break;
        promise.setProgressValue(++fileIndex);
    }
    if (!errorMessage.isEmpty()) {
        // Format and emit error.
        const QString msg = Tr::tr("There were errors when cleaning the repository %1:")
                                .arg(repository.toUserOutput());
        errorMessage.insert(0, QLatin1Char('\n'));
        errorMessage.insert(0, msg);
        errorHandler(errorMessage);
    }
}

static void handleError(const QString &errorMessage)
{
    QTimer::singleShot(0, VcsOutputWindow::instance(), [errorMessage] {
        VcsOutputWindow::instance()->appendSilently(errorMessage);
    });
}

// ---------------- CleanDialogPrivate ----------------

class CleanDialogPrivate
{
public:
    CleanDialogPrivate() :
        m_filesModel(new QStandardItemModel(0, columnCount))
    {}

    QGroupBox *m_groupBox;
    QCheckBox *m_selectAllCheckBox;
    QTreeView *m_filesTreeView;

    QStandardItemModel *m_filesModel;
    FilePath m_workingDirectory;
};


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
    resize(682, 659);
    setWindowTitle(Tr::tr("Clean Repository"));

    d->m_groupBox = new QGroupBox(this);

    d->m_selectAllCheckBox = new QCheckBox(Tr::tr("Select All"));

    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel);
    buttonBox->addButton(Tr::tr("Delete..."), QDialogButtonBox::AcceptRole);

    d->m_filesModel->setHorizontalHeaderLabels(QStringList(Tr::tr("Name")));

    d->m_filesTreeView = new QTreeView;
    d->m_filesTreeView->setModel(d->m_filesModel);
    d->m_filesTreeView->setUniformRowHeights(true);
    d->m_filesTreeView->setSelectionMode(QAbstractItemView::NoSelection);
    d->m_filesTreeView->setAllColumnsShowFocus(true);
    d->m_filesTreeView->setRootIsDecorated(false);

    using namespace Layouting;

    Column {
        d->m_selectAllCheckBox,
        d->m_filesTreeView
    }.attachTo(d->m_groupBox);

    Column {
        d->m_groupBox,
        buttonBox
    }.attachTo(this);

    connect(d->m_filesTreeView, &QAbstractItemView::doubleClicked,
            this, &CleanDialog::slotDoubleClicked);
    connect(d->m_selectAllCheckBox, &QAbstractButton::clicked,
            this, &CleanDialog::selectAllItems);
    connect(d->m_filesTreeView, &QAbstractItemView::clicked,
            this, &CleanDialog::updateSelectAllCheckBox);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

CleanDialog::~CleanDialog()
{
    delete d;
}

void CleanDialog::setFileList(const FilePath &workingDirectory, const QStringList &files,
                              const QStringList &ignoredFiles)
{
    d->m_workingDirectory = workingDirectory;
    d->m_groupBox->setTitle(Tr::tr("Repository: %1").arg(workingDirectory.toUserOutput()));
    if (const int oldRowCount = d->m_filesModel->rowCount())
        d->m_filesModel->removeRows(0, oldRowCount);

    for (const QString &fileName : files)
        addFile(workingDirectory, fileName, true);
    for (const QString &fileName : ignoredFiles)
        addFile(workingDirectory, fileName, false);

    for (int c = 0; c < d->m_filesModel->columnCount(); c++)
        d->m_filesTreeView->resizeColumnToContents(c);

    if (ignoredFiles.isEmpty())
        d->m_selectAllCheckBox->setChecked(true);
}

void CleanDialog::addFile(const FilePath &workingDirectory, const QString &fileName, bool checked)
{
    QStyle *style = QApplication::style();
    const QIcon folderIcon = style->standardIcon(QStyle::SP_DirIcon);
    const QIcon fileIcon = style->standardIcon(QStyle::SP_FileIcon);
    const FilePath fullPath = workingDirectory.pathAppended(fileName);
    const bool isDir = fullPath.isDir();
    const bool isChecked = checked && !isDir;
    auto nameItem = new QStandardItem(QDir::toNativeSeparators(fileName));
    nameItem->setFlags(Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
    nameItem->setIcon(isDir ? folderIcon : fileIcon);
    nameItem->setCheckable(true);
    nameItem->setCheckState(isChecked ? Qt::Checked : Qt::Unchecked);
    nameItem->setData(fullPath.absoluteFilePath().toVariant(), Internal::fileNameRole);
    nameItem->setData(QVariant(isDir), Internal::isDirectoryRole);
    // Tooltip with size information
    if (fullPath.isFile()) {
        const QString lastModified = QLocale::system().toString(fullPath.lastModified(),
                                                                QLocale::ShortFormat);
        nameItem->setToolTip(Tr::tr("%n bytes, last modified %1.", nullptr,
                                    fullPath.fileSize()).arg(lastModified));
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

    if (QMessageBox::question(this, Tr::tr("Delete"),
                              Tr::tr("Do you want to delete %n files?", nullptr, selectedFiles.size()),
                              QMessageBox::Yes|QMessageBox::No, QMessageBox::Yes) != QMessageBox::Yes)
        return false;

    // Remove in background
    QFuture<void> task = Utils::asyncRun(Internal::runCleanFiles, d->m_workingDirectory,
                                         selectedFiles, Internal::handleError);

    const QString taskName = Tr::tr("Cleaning \"%1\"").arg(d->m_workingDirectory.toUserOutput());
    Core::ProgressManager::addTask(task, taskName, "VcsBase.cleanRepository");
    return true;
}

void CleanDialog::slotDoubleClicked(const QModelIndex &index)
{
    // Open file on doubleclick
    if (const QStandardItem *item = d->m_filesModel->itemFromIndex(index))
        if (!item->data(Internal::isDirectoryRole).toBool()) {
            const auto fname = FilePath::fromVariant(item->data(Internal::fileNameRole));
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
        d->m_selectAllCheckBox->setChecked(checked);
    }
}

} // namespace VcsBase
