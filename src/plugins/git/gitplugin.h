/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef GITPLUGIN_H
#define GITPLUGIN_H

#include "settingspage.h"
#include "vcsbase/vcsbaseplugin.h"

#include <coreplugin/editormanager/ieditorfactory.h>
#include <extensionsystem/iplugin.h>

#include <QtCore/QObject>
#include <QtCore/QProcess>
#include <QtCore/QStringList>
#include <QtCore/QPointer>

QT_BEGIN_NAMESPACE
class QFile;
class QAction;
class QFileInfo;
QT_END_NAMESPACE

namespace Core {
class IEditorFactory;
class ICore;
}
namespace Utils {
class ParameterAction;
}

namespace Git {
namespace Internal {

class GitPlugin;
class GitVersionControl;
class GitClient;
class ChangeSelectionDialog;
class GitSubmitEditor;
struct CommitData;
struct GitSettings;
class StashDialog;

class GitPlugin : public VCSBase::VCSBasePlugin
{
    Q_OBJECT

public:
    GitPlugin();
    ~GitPlugin();

    static GitPlugin *instance();

    virtual bool initialize(const QStringList &arguments, QString *error_message);
    virtual void extensionsInitialized();

    GitVersionControl *gitVersionControl() const;

    GitSettings settings() const;
    void setSettings(const GitSettings &s);

    GitClient *gitClient() const;  

private slots:
    void diffCurrentFile();
    void diffCurrentProject();
    void diffRepository();
    void submitEditorDiff(const QStringList &unstaged, const QStringList &staged);
    void submitCurrentLog();
    void statusRepository();
    void logFile();
    void blameFile();
    void logProject();
    void undoFileChanges();
    void logRepository();
    void undoRepositoryChanges();
    void stageFile();
    void unstageFile();

    void showCommit();
    void startCommit();
    void stash();
    void stashSnapshot();
    void stashPop();
    void branchList();
    void stashList();
    void pull();
    void push();

protected:
    virtual void updateActions(VCSBase::VCSBasePlugin::ActionState);
    virtual bool submitEditorAboutToClose(VCSBase::VCSBaseSubmitEditor *submitEditor);

private:
    bool isCommitEditorOpen() const;
    Core::IEditor *openSubmitEditor(const QString &fileName, const CommitData &cd);
    void cleanCommitMessageFile();

    static GitPlugin *m_instance;
    Core::ICore *m_core;
    Utils::ParameterAction *m_diffAction;
    Utils::ParameterAction *m_diffProjectAction;
    QAction *m_diffRepositoryAction;
    QAction *m_statusRepositoryAction;
    Utils::ParameterAction *m_logAction;
    Utils::ParameterAction *m_blameAction;
    Utils::ParameterAction *m_logProjectAction;
    Utils::ParameterAction *m_undoFileAction;
    QAction *m_logRepositoryAction;
    QAction *m_undoRepositoryAction;
    QAction *m_createRepositoryAction;

    QAction *m_showAction;
    Utils::ParameterAction *m_stageAction;
    Utils::ParameterAction *m_unstageAction;
    QAction *m_commitAction;
    QAction *m_pullAction;
    QAction *m_pushAction;

    QAction *m_submitCurrentAction;
    QAction *m_diffSelectedFilesAction;
    QAction *m_undoAction;
    QAction *m_redoAction;
    QAction *m_stashAction;
    QAction *m_stashSnapshotAction;
    QAction *m_stashPopAction;
    QAction *m_stashListAction;
    QAction *m_branchListAction;
    QAction *m_menuAction;

    GitClient                   *m_gitClient;
    ChangeSelectionDialog       *m_changeSelectionDialog;
    QPointer<StashDialog>       m_stashDialog;
    QString                     m_submitRepository;
    QStringList                 m_submitOrigCommitFiles;
    QStringList                 m_submitOrigDeleteFiles;
    QString                     m_commitMessageFileName;
    bool                        m_submitActionTriggered;

};

} // namespace Git
} // namespace Internal

#endif // GITPLUGIN_H
