/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
#include <QtCore/QPair>
#include <QtCore/QVector>

QT_BEGIN_NAMESPACE
class QFile;
class QAction;
class QFileInfo;
QT_END_NAMESPACE

namespace Core {
class IEditorFactory;
class ICore;
class Command;
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

class GitPlugin;
class GitVersionControl;
class GitClient;
class ChangeSelectionDialog;
class GitSubmitEditor;
struct CommitData;
struct GitSettings;
class StashDialog;
class BranchDialog;

typedef void (GitClient::*GitClientMemberFunc)(const QString &);

typedef QPair<QAction *, Core::Command* > ActionCommandPair;
typedef QPair<Utils::ParameterAction *, Core::Command* > ParameterActionCommandPair;

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
    void logFile();
    void blameFile();
    void logProject();
    void undoFileChanges();
    void undoRepositoryChanges();
    void stageFile();
    void unstageFile();
    void cleanProject();
    void cleanRepository();
    void gitClientMemberFuncRepositoryAction();

    void showCommit();
    void startCommit();
    void stash();
    void stashSnapshot();
    void branchList();
    void stashList();
    void pull();
    void push();

protected:
    virtual void updateActions(VCSBase::VCSBasePlugin::ActionState);
    virtual bool submitEditorAboutToClose(VCSBase::VCSBaseSubmitEditor *submitEditor);

private:
    inline ParameterActionCommandPair
            createParameterAction(Core::ActionManager *am, Core::ActionContainer *ac,
                                  const QString &defaultText, const QString &parameterText,
                                  const QString &id, const QList<int> &context, bool addToLocator);

    inline ParameterActionCommandPair
            createFileAction(Core::ActionManager *am, Core::ActionContainer *ac,
                             const QString &defaultText, const QString &parameterText,
                             const QString &id, const QList<int> &context, bool addToLocator,
                             const char *pluginSlot);

    inline ParameterActionCommandPair
            createProjectAction(Core::ActionManager *am, Core::ActionContainer *ac,
                                const QString &defaultText, const QString &parameterText,
                                const QString &id, const QList<int> &context, bool addToLocator);

    inline ParameterActionCommandPair
                createProjectAction(Core::ActionManager *am, Core::ActionContainer *ac,
                                    const QString &defaultText, const QString &parameterText,
                                    const QString &id, const QList<int> &context, bool addToLocator,
                                    const char *pluginSlot);


    inline ActionCommandPair createRepositoryAction(Core::ActionManager *am, Core::ActionContainer *ac,
                                           const QString &text, const QString &id,
                                           const QList<int> &context, bool addToLocator);
    inline ActionCommandPair createRepositoryAction(Core::ActionManager *am, Core::ActionContainer *ac,
                                           const QString &text, const QString &id,
                                           const QList<int> &context,
                                           bool addToLocator, const char *pluginSlot);
    inline ActionCommandPair createRepositoryAction(Core::ActionManager *am, Core::ActionContainer *ac,
                                           const QString &text, const QString &id,
                                           const QList<int> &context,
                                           bool addToLocator, GitClientMemberFunc);

    bool isCommitEditorOpen() const;
    Core::IEditor *openSubmitEditor(const QString &fileName, const CommitData &cd);
    void cleanCommitMessageFile();
    void cleanRepository(const QString &directory);

    static GitPlugin *m_instance;
    Core::ICore *m_core;
    Locator::CommandLocator *m_commandLocator;
    QAction *m_createRepositoryAction;

    QAction *m_showAction;

    QAction *m_submitCurrentAction;
    QAction *m_diffSelectedFilesAction;
    QAction *m_undoAction;
    QAction *m_redoAction;
    QAction *m_menuAction;

    QVector<Utils::ParameterAction *> m_fileActions;
    QVector<Utils::ParameterAction *> m_projectActions;
    QVector<QAction *> m_repositoryActions;

    GitClient                   *m_gitClient;
    ChangeSelectionDialog       *m_changeSelectionDialog;
    QPointer<StashDialog>       m_stashDialog;
    QPointer<BranchDialog>      m_branchDialog;
    QString                     m_submitRepository;
    QStringList                 m_submitOrigCommitFiles;
    QStringList                 m_submitOrigDeleteFiles;
    QString                     m_commitMessageFileName;
    bool                        m_submitActionTriggered;

};

} // namespace Git
} // namespace Internal

#endif // GITPLUGIN_H
