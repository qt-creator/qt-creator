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

#pragma once

#include "vcsbase_global.h"

#include <coreplugin/editormanager/ieditor.h>

#include <QAbstractItemView>

QT_BEGIN_NAMESPACE
class QAction;
class QIcon;
QT_END_NAMESPACE

namespace ProjectExplorer { class Project; }

namespace VcsBase {
namespace Internal {
    class CommonVcsSettings;
    class SubmitEditorFile;
} // namespace Internal

class SubmitEditorWidget;
class SubmitFileModel;
class VcsBaseSubmitEditorPrivate;

class VCSBASE_EXPORT VcsBaseSubmitEditorParameters
{
public:
    const char *mimeType;
    const char *id;
    const char *displayName;
    enum DiffType { DiffRows, DiffFiles } diffType;
};

class VCSBASE_EXPORT VcsBaseSubmitEditor : public Core::IEditor
{
    Q_OBJECT
    Q_PROPERTY(QAbstractItemView::SelectionMode fileListSelectionMode READ fileListSelectionMode WRITE setFileListSelectionMode DESIGNABLE true)
    Q_PROPERTY(bool lineWrap READ lineWrap WRITE setLineWrap DESIGNABLE true)
    Q_PROPERTY(int lineWrapWidth READ lineWrapWidth WRITE setLineWrapWidth DESIGNABLE true)
    Q_PROPERTY(QString checkScriptWorkingDirectory READ checkScriptWorkingDirectory WRITE setCheckScriptWorkingDirectory DESIGNABLE true)
    Q_PROPERTY(bool emptyFileListEnabled READ isEmptyFileListEnabled WRITE setEmptyFileListEnabled DESIGNABLE true)

protected:
    explicit VcsBaseSubmitEditor(const VcsBaseSubmitEditorParameters *parameters,
                                 SubmitEditorWidget *editorWidget);

public:
    // Register the actions with the submit editor widget.
    void registerActions(QAction *editorUndoAction,  QAction *editorRedoAction,
                         QAction *submitAction = 0, QAction *diffAction = 0);
    void unregisterActions(QAction *editorUndoAction,  QAction *editorRedoAction,
                           QAction *submitAction = 0, QAction *diffAction = 0);

    ~VcsBaseSubmitEditor() override;

    // A utility routine to be called when closing a submit editor.
    // Runs checks on the message and prompts according to configuration.
    // Force prompt should be true if it is invoked by closing an editor
    // as opposed to invoking the "Submit" button.
    // 'promptSetting' points to a bool variable containing the plugin's
    // prompt setting. The user can uncheck it from the message box.
    enum PromptSubmitResult { SubmitConfirmed, SubmitCanceled, SubmitDiscarded };
    PromptSubmitResult promptSubmit(const QString &title, const QString &question,
                                    const QString &checkFailureQuestion,
                                    bool *promptSetting,
                                    bool forcePrompt = false,
                                    bool canCommitOnFailure = true);

    QAbstractItemView::SelectionMode fileListSelectionMode() const;
    void setFileListSelectionMode(QAbstractItemView::SelectionMode sm);

    // 'Commit' action enabled despite empty file list
    bool isEmptyFileListEnabled() const;
    void setEmptyFileListEnabled(bool e);

    bool lineWrap() const;
    void setLineWrap(bool);

    int lineWrapWidth() const;
    void setLineWrapWidth(int);

    QString checkScriptWorkingDirectory() const;
    void setCheckScriptWorkingDirectory(const QString &);

    Core::IDocument *document() override;

    QWidget *toolBar() override;

    QStringList checkedFiles() const;

    void setFileModel(SubmitFileModel *m);
    SubmitFileModel *fileModel() const;
    virtual void updateFileModel() { }
    QStringList rowsToFiles(const QList<int> &rows) const;

    // Utilities returning some predefined icons for actions
    static QIcon diffIcon();
    static QIcon submitIcon();

    // Reduce a list of untracked files reported by a VCS down to the files
    // that are actually part of the current project(s).
    static void filterUntrackedFilesOfProject(const QString &repositoryDirectory, QStringList *untrackedFiles);

signals:
    void diffSelectedFiles(const QStringList &files);
    void diffSelectedRows(const QList<int> &rows);
    void fileContentsChanged();

protected:
    /* These hooks allow for modifying the contents that goes to
     * the file. The default implementation uses the text
     * of the description editor. */
    virtual QByteArray fileContents() const;
    virtual bool setFileContents(const QByteArray &contents);

    QString description() const;
    void setDescription(const QString &text);

    void setDescriptionMandatory(bool v);
    bool isDescriptionMandatory() const;

private:
    void slotDiffSelectedVcsFiles(const QList<int> &rawList);
    void slotCheckSubmitMessage();
    void slotInsertNickName();
    void slotSetFieldNickName(int);
    void slotUpdateEditorSettings(const VcsBase::Internal::CommonVcsSettings &);

    void createUserFields(const QString &fieldConfigFile);
    bool checkSubmitMessage(QString *errorMessage) const;
    bool runSubmitMessageCheckScript(const QString &script, QString *errorMessage) const;
    QString promptForNickName();

    VcsBaseSubmitEditorPrivate *d;

    friend class Internal::SubmitEditorFile; // for the file contents
};

} // namespace VcsBase
