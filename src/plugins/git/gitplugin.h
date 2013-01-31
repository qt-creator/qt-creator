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

#ifndef GITPLUGIN_H
#define GITPLUGIN_H

#include "gitsettings.h"

#include "vcsbase/vcsbaseplugin.h"

#include <coreplugin/editormanager/ieditorfactory.h>
#include <extensionsystem/iplugin.h>

#include <QObject>
#include <QProcess>
#include <QStringList>
#include <QPointer>
#include <QPair>
#include <QVector>

QT_BEGIN_NAMESPACE
class QFile;
class QAction;
class QFileInfo;
QT_END_NAMESPACE

namespace Core {
class IEditorFactory;
class Command;
class Context;
class ActionManager;
class ActionContainer;
}
namespace Utils {
class ParameterAction;
}
namespace Locator {
    class CommandLocator;
}
namespace Git {
namespace Internal {

class GitVersionControl;
class GitClient;
class ChangeSelectionDialog;
class GitSubmitEditor;
class CommitData;
class StashDialog;
class BranchDialog;
class RemoteDialog;

typedef void (GitClient::*GitClientMemberFunc)(const QString &);

typedef QPair<QAction *, Core::Command* > ActionCommandPair;
typedef QPair<Utils::ParameterAction *, Core::Command* > ParameterActionCommandPair;

class GitPlugin : public VcsBase::VcsBasePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Git.json")

public:
    GitPlugin();
    ~GitPlugin();

    static GitPlugin *instance();

    bool initialize(const QStringList &arguments, QString *errorMessage);

    GitVersionControl *gitVersionControl() const;

    const GitSettings &settings() const;
    void setSettings(const GitSettings &s);

    GitClient *gitClient() const;

public slots:
    void startCommit();

private slots:
    void diffCurrentFile();
    void diffCurrentProject();
    void diffRepository();
    void submitEditorDiff(const QStringList &unstaged, const QStringList &staged);
    void submitEditorMerge(const QStringList &unmerged);
    void submitCurrentLog();
    void logFile();
    void blameFile();
    void logProject();
    void undoFileChanges(bool revertStaging = true);
    void undoUnstagedFileChanges();
    void resetRepository();
    void startRevertCommit();
    void startCherryPickCommit();
    void stageFile();
    void unstageFile();
    void gitkForCurrentFile();
    void gitkForCurrentFolder();
    void cleanProject();
    void cleanRepository();
    void applyCurrentFilePatch();
    void promptApplyPatch();
    void gitClientMemberFuncRepositoryAction();

    void showCommit();
    void startAmendCommit();
    void stash();
    void stashSnapshot();
    void branchList();
    void remoteList();
    void stashList();
    void fetch();
    void pull();
    void push();
    void startMergeTool();

#ifdef WITH_TESTS
    void testStatusParsing_data();
    void testStatusParsing();
    void testDiffFileResolving_data();
    void testDiffFileResolving();
    void testLogResolving();
#endif
protected:
    void updateActions(VcsBase::VcsBasePlugin::ActionState);
    bool submitEditorAboutToClose(VcsBase::VcsBaseSubmitEditor *submitEditor);

private:
    inline ParameterActionCommandPair
            createParameterAction(Core::ActionContainer *ac,
                                  const QString &defaultText, const QString &parameterText,
                                  const Core::Id &id, const Core::Context &context, bool addToLocator);

    inline ParameterActionCommandPair
            createFileAction(Core::ActionContainer *ac,
                             const QString &defaultText, const QString &parameterText,
                             const Core::Id &id, const Core::Context &context, bool addToLocator,
                             const char *pluginSlot);

    inline ParameterActionCommandPair
            createProjectAction(Core::ActionContainer *ac,
                                const QString &defaultText, const QString &parameterText,
                                const Core::Id &id, const Core::Context &context, bool addToLocator);

    inline ParameterActionCommandPair
                createProjectAction(Core::ActionContainer *ac,
                                    const QString &defaultText, const QString &parameterText,
                                    const Core::Id &id, const Core::Context &context, bool addToLocator,
                                    const char *pluginSlot);


    inline ActionCommandPair createRepositoryAction(Core::ActionContainer *ac,
                                           const QString &text, const Core::Id &id,
                                           const Core::Context &context, bool addToLocator);
    inline ActionCommandPair createRepositoryAction(Core::ActionContainer *ac,
                                           const QString &text, const Core::Id &id,
                                           const Core::Context &context,
                                           bool addToLocator, const char *pluginSlot);
    inline ActionCommandPair createRepositoryAction(Core::ActionContainer *ac,
                                           const QString &text, const Core::Id &id,
                                           const Core::Context &context,
                                           bool addToLocator, GitClientMemberFunc);

    void updateRepositoryBrowserAction();
    bool isCommitEditorOpen() const;
    Core::IEditor *openSubmitEditor(const QString &fileName, const CommitData &cd, bool amend);
    void cleanCommitMessageFile();
    void cleanRepository(const QString &directory);
    void applyPatch(const QString &workingDirectory, QString file = QString());
    void startCommit(bool amend);

    static GitPlugin *m_instance;
    Locator::CommandLocator *m_commandLocator;
    QAction *m_createRepositoryAction;

    QAction *m_showAction;

    QAction *m_submitCurrentAction;
    QAction *m_diffSelectedFilesAction;
    QAction *m_undoAction;
    QAction *m_redoAction;
    QAction *m_menuAction;
    QAction *m_repositoryBrowserAction;

    QVector<Utils::ParameterAction *> m_fileActions;
    QVector<Utils::ParameterAction *> m_projectActions;
    QVector<QAction *> m_repositoryActions;
    Utils::ParameterAction *m_applyCurrentFilePatchAction;

    GitClient                   *m_gitClient;
    ChangeSelectionDialog       *m_changeSelectionDialog;
    QPointer<StashDialog>       m_stashDialog;
    QPointer<BranchDialog>      m_branchDialog;
    QPointer<RemoteDialog>      m_remoteDialog;
    QString                     m_submitRepository;
    QString                     m_commitMessageFileName;
    QString                     m_commitAmendSHA1;
    bool                        m_submitActionTriggered;

    GitSettings m_settings;
};

} // namespace Git
} // namespace Internal

#endif // GITPLUGIN_H
