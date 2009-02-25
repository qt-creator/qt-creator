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

#include "qrceditor.h"
#include "undocommands_p.h"

#include <QtCore/QDebug>
#include <QtGui/QMenu>
#include <QtGui/QFileDialog>
#include <QtGui/QMessageBox>

using namespace SharedTools;

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

    m_treeview->enableContextMenu(false);
    layout->addWidget(m_treeview);
    connect(m_ui.removeButton, SIGNAL(clicked()), this, SLOT(onRemove()));

    // 'Add' button with menu
    QMenu *addMenu = new QMenu(this);
    m_addFileAction = addMenu->addAction(tr("Add Files"), this, SLOT(onAddFiles()));
    addMenu->addAction(tr("Add Prefix"), this, SLOT(onAddPrefix()));
    m_ui.addButton->setMenu(addMenu);

    connect(m_treeview, SIGNAL(addPrefixTriggered()), this, SLOT(onAddPrefix()));
    connect(m_treeview, SIGNAL(addFilesTriggered(QString)), this, SLOT(onAddFiles()));
    connect(m_treeview, SIGNAL(removeItem()), this, SLOT(onRemove()));
    connect(m_treeview, SIGNAL(currentIndexChanged()), this, SLOT(updateCurrent()));
    connect(m_treeview, SIGNAL(dirtyChanged(bool)), this, SIGNAL(dirtyChanged(bool)));
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

    connect(m_treeview, SIGNAL(addFilesTriggered(const QString&)),
        this, SIGNAL(addFilesTriggered(const QString&)));

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

void QrcEditor::resolveLocationIssues(QStringList &files)
{
    const QDir dir = QFileInfo(m_treeview->fileName()).absoluteDir();
    const QString dotdotSlash = QLatin1String("../");
    int i = 0;
    int count = files.count();

    // Find first troublesome file
    for (; i < count; i++) {
        QString const &file = files.at(i);
        const QString relativePath = dir.relativeFilePath(file);
        if (relativePath.startsWith(dotdotSlash))
            break;
    }

    // All paths fine -> no interaction needed
    if (i == count) {
        return;
    }

    // Interact with user from now on
    bool abort = false;
    for (; i < count; i++) {
        // Path fine -> skip file
        QString const &file = files.at(i);
        QString const relativePath = dir.relativeFilePath(file);
        if (!relativePath.startsWith(dotdotSlash)) {
            continue;
        }

        // Path troublesome and aborted -> remove file
        if (abort) {
            files.removeAt(i);
            count--;
            i--;
            continue;
        } else {
            // Path troublesome -> query user
            QMessageBox message(this);
            message.setWindowTitle(tr("Invalid file"));
            message.setIcon(QMessageBox::Warning);
            QPushButton * const copyButton = message.addButton(tr("Copy"), QMessageBox::ActionRole);
            QPushButton * const skipButton = message.addButton(tr("Don't add"), QMessageBox::DestructiveRole);
            QPushButton * const abortButton = message.addButton(tr("Abort"), QMessageBox::RejectRole);
            message.setDefaultButton(copyButton);
            message.setEscapeButton(skipButton);
            message.setText(tr("The file %1 is not in a subdirectory of the resource file. Continuing will result in an invalid resource file.")
                            .arg(QDir::toNativeSeparators(file)));
            message.exec();
            if (message.clickedButton() == skipButton) {
                files.removeAt(i);
                count--;
                i--; // Compensate i++
            } else if (message.clickedButton() == copyButton) {
                const QFileInfo fi(file);
                const QFileInfo suggestion(dir, fi.fileName());
                const QString copyName = QFileDialog::getSaveFileName(this, tr("Choose copy location"),
                                                                suggestion.absoluteFilePath());
                if (!copyName.isEmpty()) {
                    QString relPath = dir.relativeFilePath(copyName);
                    if (relPath.startsWith(dotdotSlash)) {   // directory is still invalid
                        i--; // Compensate i++ and try again
                        continue;
                    }
                    if (QFile::exists(copyName)) {
                        if (!QFile::remove(copyName)) {
                            QMessageBox::critical(this, tr("Overwrite failed"),
                                                  tr("Could not overwrite file %1.")
                                                  .arg(QDir::toNativeSeparators(copyName)));
                            // Remove file
                            files.removeAt(i);
                            count--;
                            i--; // Compensate i++
                            continue;
                        }
                    }
                    if (!QFile::copy(file, copyName)) {
                        QMessageBox::critical(this, tr("Copying failed"),
                                              tr("Could not copy the file to %1.")
                                              .arg(QDir::toNativeSeparators(copyName)));
                        // Remove file
                        files.removeAt(i);
                        count--;
                        i--; // Compensate i++
                        continue;
                    }
                    files[i] = copyName;
                } else {
                    // Remove file
                    files.removeAt(i);
                    count--;
                    i--; // Compensate i++
                }
            } else if (message.clickedButton() == abortButton) {
                abort = true;

                files.removeAt(i);
                count--;
                i--; // Compensate i++
            }
        }
    }
}

void QrcEditor::setResourceDragEnabled(bool e)
{
    m_treeview->setResourceDragEnabled(e);
}

bool QrcEditor::resourceDragEnabled() const
{
    return m_treeview->resourceDragEnabled();
}

void QrcEditor::setDefaultAddFileEnabled(bool enable)
{
    m_treeview->setDefaultAddFileEnabled(enable);
}

bool QrcEditor::defaultAddFileEnabled() const
{
    return m_treeview->defaultAddFileEnabled();
}

void QrcEditor::addFile(const QString &prefix, const QString &file)
{
    // TODO: make this function UNDO / REDO aware
    m_treeview->addFile(prefix, file);
}

/*
void QrcEditor::removeFile(const QString &prefix, const QString &file)
{
    m_treeview->removeFile(prefix, file);
}
*/
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
