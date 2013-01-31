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

#include "qrceditor.h"
#include "undocommands_p.h"

#include <QDebug>
#include <QScopedPointer>
#include <QMenu>
#include <QFileDialog>
#include <QMessageBox>

using namespace ResourceEditor;
using namespace ResourceEditor::Internal;

QrcEditor::QrcEditor(QWidget *parent)
  : QWidget(parent),
    m_treeview(new ResourceView(&m_history)),
    m_addFileAction(0)
{
    m_ui.setupUi(this);
    QHBoxLayout *layout = new QHBoxLayout;
    layout->setSpacing(0);
    layout->setMargin(0);
    m_ui.centralWidget->setLayout(layout);
    m_treeview->setFrameStyle(QFrame::NoFrame);;
    layout->addWidget(m_treeview);

    connect(m_ui.removeButton, SIGNAL(clicked()), this, SLOT(onRemove()));

    // 'Add' button with menu
    QMenu *addMenu = new QMenu(this);
    m_addFileAction = addMenu->addAction(tr("Add Files"), this, SLOT(onAddFiles()));
    addMenu->addAction(tr("Add Prefix"), this, SLOT(onAddPrefix()));
    m_ui.addButton->setMenu(addMenu);

    connect(m_treeview, SIGNAL(removeItem()), this, SLOT(onRemove()));
    connect(m_treeview->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
            this, SLOT(updateCurrent()));
    connect(m_treeview, SIGNAL(dirtyChanged(bool)), this, SIGNAL(dirtyChanged(bool)));
    connect(m_treeview, SIGNAL(itemActivated(QString)),
            this, SIGNAL(itemActivated(QString)));
    connect(m_treeview, SIGNAL(showContextMenu(QPoint,QString)),
            this, SIGNAL(showContextMenu(QPoint,QString)));
    m_treeview->setFocus();

    connect(m_ui.aliasText, SIGNAL(textEdited(QString)),
            this, SLOT(onAliasChanged(QString)));
    connect(m_ui.prefixText, SIGNAL(textEdited(QString)),
            this, SLOT(onPrefixChanged(QString)));
    connect(m_ui.languageText, SIGNAL(textEdited(QString)),
            this, SLOT(onLanguageChanged(QString)));

    // Prevent undo command merging after a switch of focus:
    // (0) The initial text is "Green".
    // (1) The user appends " is a color." --> text is "Green is a color."
    // (2) The user clicks into some other line edit --> loss of focus
    // (3) The user gives focuse again and substitutes "Green" with "Red"
    //     --> text now is "Red is a color."
    // (4) The user hits undo --> text now is "Green is a color."
    //     Without calling advanceMergeId() it would have been "Green", instead.
    connect(m_ui.aliasText, SIGNAL(editingFinished()),
            m_treeview, SLOT(advanceMergeId()));
    connect(m_ui.prefixText, SIGNAL(editingFinished()),
            m_treeview, SLOT(advanceMergeId()));
    connect(m_ui.languageText, SIGNAL(editingFinished()),
            m_treeview, SLOT(advanceMergeId()));

    connect(&m_history, SIGNAL(canRedoChanged(bool)), this, SLOT(updateHistoryControls()));
    connect(&m_history, SIGNAL(canUndoChanged(bool)), this, SLOT(updateHistoryControls()));
    updateHistoryControls();
    updateCurrent();
}

QrcEditor::~QrcEditor()
{
}

QString QrcEditor::fileName() const
{
    return m_treeview->fileName();
}

void QrcEditor::setFileName(const QString &fileName)
{
    m_treeview->setFileName(fileName);
}

bool QrcEditor::load(const QString &fileName)
{
    const bool success = m_treeview->load(fileName);
    if (success) {
        // Set "focus"
        m_treeview->setCurrentIndex(m_treeview->model()->index(0,0));

        // Expand prefix nodes
        m_treeview->expandAll();
    }
    return success;
}

void QrcEditor::refresh()
{
    m_treeview->refresh();
}

bool QrcEditor::save()
{
    return m_treeview->save();
}

bool QrcEditor::isDirty()
{
    return m_treeview->isDirty();
}

void QrcEditor::setDirty(bool dirty)
{
    m_treeview->setDirty(dirty);
}

// Propagates a change of selection in the tree
// to the alias/prefix/language edit controls
void QrcEditor::updateCurrent()
{
    const bool isValid = m_treeview->currentIndex().isValid();
    const bool isPrefix = m_treeview->isPrefix(m_treeview->currentIndex()) && isValid;
    const bool isFile = !isPrefix && isValid;

    m_ui.aliasLabel->setEnabled(isFile);
    m_ui.aliasText->setEnabled(isFile);
    m_currentAlias = m_treeview->currentAlias();
    m_ui.aliasText->setText(m_currentAlias);

    m_ui.prefixLabel->setEnabled(isPrefix);
    m_ui.prefixText->setEnabled(isPrefix);
    m_currentPrefix = m_treeview->currentPrefix();
    m_ui.prefixText->setText(m_currentPrefix);

    m_ui.languageLabel->setEnabled(isPrefix);
    m_ui.languageText->setEnabled(isPrefix);
    m_currentLanguage = m_treeview->currentLanguage();
    m_ui.languageText->setText(m_currentLanguage);

    m_ui.addButton->setEnabled(true);
    m_addFileAction->setEnabled(isValid);
    m_ui.removeButton->setEnabled(isValid);
}

void QrcEditor::updateHistoryControls()
{
    emit undoStackChanged(m_history.canUndo(), m_history.canRedo());
}

// Helper for resolveLocationIssues():
// For code clarity, a context with convenience functions to execute
// the dialogs required for checking the image file paths
// (and keep them around for file dialog execution speed).
// Basically, resolveLocationIssues() checks the paths of the images
// and asks the user to copy them into the resource file location.
// When the user does a multiselection of files, this requires popping
// up the dialog several times in a row.
struct ResolveLocationContext {
    ResolveLocationContext() : copyButton(0), skipButton(0), abortButton(0) {}

    QAbstractButton *execLocationMessageBox(QWidget *parent,
                                        const QString &file,
                                        bool wantSkipButton);
    QString execCopyFileDialog(QWidget *parent,
                               const QDir &dir,
                               const QString &targetPath);

    QScopedPointer<QMessageBox> messageBox;
    QScopedPointer<QFileDialog> copyFileDialog;

    QPushButton *copyButton;
    QPushButton *skipButton;
    QPushButton *abortButton;
};

QAbstractButton *ResolveLocationContext::execLocationMessageBox(QWidget *parent,
                                                            const QString &file,
                                                            bool wantSkipButton)
{
    if (messageBox.isNull()) {
        messageBox.reset(new QMessageBox(QMessageBox::Warning,
                                         QrcEditor::tr("Invalid file location"),
                                         QString(), QMessageBox::NoButton, parent));
        copyButton = messageBox->addButton(QrcEditor::tr("Copy"), QMessageBox::ActionRole);
        abortButton = messageBox->addButton(QrcEditor::tr("Abort"), QMessageBox::RejectRole);
        messageBox->setDefaultButton(copyButton);
    }
    if (wantSkipButton && !skipButton) {
        skipButton = messageBox->addButton(QrcEditor::tr("Skip"), QMessageBox::DestructiveRole);
        messageBox->setEscapeButton(skipButton);
    }
    messageBox->setText(QrcEditor::tr("The file %1 is not in a subdirectory of the resource file. You now have the option to copy this file to a valid location.")
                    .arg(QDir::toNativeSeparators(file)));
    messageBox->exec();
    return messageBox->clickedButton();
}

QString ResolveLocationContext::execCopyFileDialog(QWidget *parent, const QDir &dir, const QString &targetPath)
{
    // Delayed creation of file dialog.
    if (copyFileDialog.isNull()) {
        copyFileDialog.reset(new QFileDialog(parent, QrcEditor::tr("Choose Copy Location")));
        copyFileDialog->setFileMode(QFileDialog::AnyFile);
        copyFileDialog->setAcceptMode(QFileDialog::AcceptSave);
    }
    copyFileDialog->selectFile(targetPath);
    // Repeat until the path entered is no longer above 'dir'
    // (relative is not "../").
    while (true) {
        if (copyFileDialog->exec() != QDialog::Accepted)
            return QString();
        const QStringList files = copyFileDialog->selectedFiles();
        if (files.isEmpty())
            return QString();
        const QString relPath = dir.relativeFilePath(files.front());
        if (!relPath.startsWith(QLatin1String("../")))
            return files.front();
    }
    return QString();
}

// Helper to copy a file with message boxes
static inline bool copyFile(const QString &file, const QString &copyName, QWidget *parent)
{
    if (QFile::exists(copyName)) {
        if (!QFile::remove(copyName)) {
            QMessageBox::critical(parent, QrcEditor::tr("Overwriting Failed"),
                                  QrcEditor::tr("Could not overwrite file %1.")
                                  .arg(QDir::toNativeSeparators(copyName)));
            return false;
        }
    }
    if (!QFile::copy(file, copyName)) {
        QMessageBox::critical(parent, QrcEditor::tr("Copying Failed"),
                              QrcEditor::tr("Could not copy the file to %1.")
                              .arg(QDir::toNativeSeparators(copyName)));
        return false;
    }
    return true;
}

void QrcEditor::resolveLocationIssues(QStringList &files)
{
    const QDir dir = QFileInfo(m_treeview->fileName()).absoluteDir();
    const QString dotdotSlash = QLatin1String("../");
    int i = 0;
    const int count = files.count();
    const int initialCount = files.count();

    // Find first troublesome file
    for (; i < count; i++) {
        QString const &file = files.at(i);
        const QString relativePath = dir.relativeFilePath(file);
        if (relativePath.startsWith(dotdotSlash))
            break;
    }

    // All paths fine -> no interaction needed
    if (i == count)
        return;

    // Interact with user from now on
    ResolveLocationContext context;
    bool abort = false;
    for (QStringList::iterator it = files.begin(); it != files.end(); ) {
        // Path fine -> skip file
        QString const &file = *it;
        QString const relativePath = dir.relativeFilePath(file);
        if (!relativePath.startsWith(dotdotSlash))
            continue;
        // Path troublesome and aborted -> remove file
        bool ok = false;
        if (!abort) {
            // Path troublesome -> query user "Do you want copy/abort/skip".
            QAbstractButton *clickedButton = context.execLocationMessageBox(this, file, initialCount > 1);
            if (clickedButton == context.copyButton) {
                const QFileInfo fi(file);
                QFileInfo suggestion;
                QDir tmpTarget(dir.path() + QDir::separator() + QLatin1String("Resources"));
                if (tmpTarget.exists())
                    suggestion.setFile(tmpTarget, fi.fileName());
                else
                    suggestion.setFile(dir, fi.fileName());
                // Prompt for copy location, copy and replace name.
                const QString copyName = context.execCopyFileDialog(this, dir, suggestion.absoluteFilePath());
                ok = !copyName.isEmpty() && copyFile(file, copyName, this);
                if (ok)
                    *it = copyName;
            } else if (clickedButton == context.abortButton) {
                abort = true;
            }
        } // !abort
        if (ok) { // Remove files where user canceled or failures occurred.
            ++it;
        } else {
            it = files.erase(it);
        }
    } // for files
}

void QrcEditor::setResourceDragEnabled(bool e)
{
    m_treeview->setResourceDragEnabled(e);
}

bool QrcEditor::resourceDragEnabled() const
{
    return m_treeview->resourceDragEnabled();
}

void QrcEditor::addFile(const QString &prefix, const QString &file)
{
    // TODO: make this function UNDO / REDO aware
    m_treeview->addFile(prefix, file);
}

void QrcEditor::editCurrentItem()
{
    if (m_treeview->selectionModel()->currentIndex().isValid())
        m_treeview->edit(m_treeview->selectionModel()->currentIndex());
}

QString QrcEditor::currentResourcePath() const
{
    return m_treeview->currentResourcePath();
}

// Slot for change of line edit content 'alias'
void QrcEditor::onAliasChanged(const QString &alias)
{
    const QString &before = m_currentAlias;
    const QString &after = alias;
    m_treeview->setCurrentAlias(before, after);
    m_currentAlias = alias;
    updateHistoryControls();
}

// Slot for change of line edit content 'prefix'
void QrcEditor::onPrefixChanged(const QString &prefix)
{
    const QString &before = m_currentPrefix;
    const QString &after = prefix;
    m_treeview->setCurrentPrefix(before, after);
    m_currentPrefix = prefix;
    updateHistoryControls();
}

// Slot for change of line edit content 'language'
void QrcEditor::onLanguageChanged(const QString &language)
{
    const QString &before = m_currentLanguage;
    const QString &after = language;
    m_treeview->setCurrentLanguage(before, after);
    m_currentLanguage = language;
    updateHistoryControls();
}

// Slot for 'Remove' button
void QrcEditor::onRemove()
{
    // Find current item, push and execute command
    const QModelIndex current = m_treeview->currentIndex();
    int afterDeletionArrayIndex = current.row();
    QModelIndex afterDeletionParent = current.parent();
    m_treeview->findSamePlacePostDeletionModelIndex(afterDeletionArrayIndex, afterDeletionParent);
    QUndoCommand * const removeCommand = new RemoveEntryCommand(m_treeview, current);
    m_history.push(removeCommand);
    const QModelIndex afterDeletionModelIndex
            = m_treeview->model()->index(afterDeletionArrayIndex, 0, afterDeletionParent);
    m_treeview->setCurrentIndex(afterDeletionModelIndex);
    updateHistoryControls();
}

// Slot for 'Add File' button
void QrcEditor::onAddFiles()
{
    QModelIndex const current = m_treeview->currentIndex();
    int const currentIsPrefixNode = m_treeview->isPrefix(current);
    int const prefixArrayIndex = currentIsPrefixNode ? current.row()
            : m_treeview->model()->parent(current).row();
    int const cursorFileArrayIndex = currentIsPrefixNode ? 0 : current.row();
    QStringList fileNames = m_treeview->fileNamesToAdd();
    fileNames = m_treeview->existingFilesSubtracted(prefixArrayIndex, fileNames);
    resolveLocationIssues(fileNames);
    if (fileNames.isEmpty())
        return;
    QUndoCommand * const addFilesCommand = new AddFilesCommand(
            m_treeview, prefixArrayIndex, cursorFileArrayIndex, fileNames);
    m_history.push(addFilesCommand);
    updateHistoryControls();
}

// Slot for 'Add Prefix' button
void QrcEditor::onAddPrefix()
{
    QUndoCommand * const addEmptyPrefixCommand = new AddEmptyPrefixCommand(m_treeview);
    m_history.push(addEmptyPrefixCommand);
    updateHistoryControls();
}

// Slot for 'Undo' button
void QrcEditor::onUndo()
{
    m_history.undo();
    updateCurrent();
    updateHistoryControls();
}

// Slot for 'Redo' button
void QrcEditor::onRedo()
{
    m_history.redo();
    updateCurrent();
    updateHistoryControls();
}
