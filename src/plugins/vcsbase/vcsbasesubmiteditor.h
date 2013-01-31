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

#ifndef VCSBASE_SUBMITEDITOR_H
#define VCSBASE_SUBMITEDITOR_H

#include "vcsbase_global.h"

#include <coreplugin/editormanager/ieditor.h>

#include <QAbstractItemView>

QT_BEGIN_NAMESPACE
class QIcon;
class QAction;
QT_END_NAMESPACE

namespace VcsBase {
namespace Internal {
    class CommonVcsSettings;
}
struct VcsBaseSubmitEditorPrivate;
class SubmitEditorWidget;
class SubmitFileModel;

class VCSBASE_EXPORT VcsBaseSubmitEditorParameters
{
public:
    const char *mimeType;
    const char *id;
    const char *displayName;
    const char *context;
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

    ~VcsBaseSubmitEditor();

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
                                    bool canCommitOnFailure = true) const;

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

    // Core::IEditor
    bool createNew(const QString &contents);
    bool open(QString *errorString, const QString &fileName, const QString &realFileName);
    Core::IDocument *document();
    QString displayName() const;
    void setDisplayName(const QString &title);
    bool duplicateSupported() const;
    Core::IEditor *duplicate(QWidget *parent);
    Core::Id id() const;
    bool isTemporary() const { return true; }

    QWidget *toolBar();

    QByteArray saveState() const;
    bool restoreState(const QByteArray &state);

    QStringList checkedFiles() const;

    void setFileModel(SubmitFileModel *m, const QString &repositoryDirectory = QString());
    SubmitFileModel *fileModel() const;
    virtual void updateFileModel() { }
    QStringList rowsToFiles(const QList<int> &rows) const;

    // Utilities returning some predefined icons for actions
    static QIcon diffIcon();
    static QIcon submitIcon();

    // Utility returning all project files in case submit lists need to
    // be restricted to them
    static QStringList currentProjectFiles(bool nativeSeparators, QString *name = 0);

    // Reduce a list of untracked files reported by a VCS down to the files
    // that are actually part of the current project(s).
    static void filterUntrackedFilesOfProject(const QString &repositoryDirectory, QStringList *untrackedFiles);

    // Helper to raise an already open submit editor to prevent opening twice.
    static bool raiseSubmitEditor();

signals:
    void diffSelectedFiles(const QStringList &files);
    void diffSelectedFiles(const QList<int> &rows);

private slots:
    void slotDiffSelectedVcsFiles(const QList<int> &rawList);
    bool save(QString *errorString, const QString &fileName, bool autoSave);
    void slotDescriptionChanged();
    void slotCheckSubmitMessage();
    void slotInsertNickName();
    void slotSetFieldNickName(int);
    void slotUpdateEditorSettings(const VcsBase::Internal::CommonVcsSettings &);
    void slotRefreshCommitData();

protected:
    /* These hooks allow for modifying the contents that goes to
     * the file. The default implementation uses the text
     * of the description editor. */
    virtual QByteArray fileContents() const;
    virtual bool setFileContents(const QString &contents);

    void setDescriptionMandatory(bool v);
    bool isDescriptionMandatory() const;

private:
    void createUserFields(const QString &fieldConfigFile);
    bool checkSubmitMessage(QString *errorMessage) const;
    bool runSubmitMessageCheckScript(const QString &script, QString *errorMessage) const;
    QString promptForNickName();

    VcsBaseSubmitEditorPrivate *d;
};

} // namespace VcsBase

#endif // VCSBASE_SUBMITEDITOR_H
