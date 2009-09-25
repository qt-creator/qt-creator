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

#include <coreplugin/editormanager/ieditorfactory.h>
#include <coreplugin/icorelistener.h>
#include <extensionsystem/iplugin.h>

#include <QtCore/QObject>
#include <QtCore/QProcess>
#include <QtCore/QStringList>

QT_BEGIN_NAMESPACE
class QFile;
class QAction;
class QFileInfo;
QT_END_NAMESPACE

namespace Core {
class IEditorFactory;
class ICore;
class IVersionControl;
namespace Utils {
class ParameterAction;
}
} // namespace Core

namespace Git {
namespace Internal {

class GitPlugin;
class GitClient;
class ChangeSelectionDialog;
class GitSubmitEditor;
struct CommitData;
struct GitSettings;

// Just a proxy for GitPlugin
class CoreListener : public Core::ICoreListener
{
    Q_OBJECT
public:
    CoreListener(GitPlugin *plugin) : m_plugin(plugin) { }
    bool editorAboutToClose(Core::IEditor *editor);
    bool coreAboutToClose() { return true; }
private:
    GitPlugin *m_plugin;
};

class GitPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT

public:
    GitPlugin();
    ~GitPlugin();

    static GitPlugin *instance();

    virtual bool initialize(const QStringList &arguments, QString *error_message);
    virtual void extensionsInitialized();

    QString getWorkingDirectory();

    GitSettings settings() const;
    void setSettings(const GitSettings &s);

    GitClient *gitClient() const;

public slots:
    void updateActions();
    bool editorAboutToClose(Core::IEditor *editor);

private slots:
    void diffCurrentFile();
    void diffCurrentProject();
    void submitEditorDiff(const QStringList &unstaged, const QStringList &staged);
    void submitCurrentLog();
    void statusFile();
    void statusProject();
    void logFile();
    void blameFile();
    void logProject();
    void undoFileChanges();
    void undoProjectChanges();
    void stageFile();
    void unstageFile();

    void showCommit();
    void startCommit();
    void stash();
    void stashPop();
    void branchList();
    void stashList();
    void pull();
    void push();

private:
    bool isCommitEditorOpen() const;
    QFileInfo currentFile() const;
    Core::IEditor *openSubmitEditor(const QString &fileName, const CommitData &cd);
    void cleanCommitMessageFile();

    static GitPlugin *m_instance;
    Core::ICore *m_core;
    Core::Utils::ParameterAction *m_diffAction;
    Core::Utils::ParameterAction *m_diffProjectAction;
    Core::Utils::ParameterAction *m_statusAction;
    Core::Utils::ParameterAction *m_statusProjectAction;
    Core::Utils::ParameterAction *m_logAction;
    Core::Utils::ParameterAction *m_blameAction;
    Core::Utils::ParameterAction *m_logProjectAction;
    Core::Utils::ParameterAction *m_undoFileAction;
    QAction *m_undoProjectAction;
    QAction *m_showAction;
    Core::Utils::ParameterAction *m_stageAction;
    Core::Utils::ParameterAction *m_unstageAction;
    Core::Utils::ParameterAction *m_revertAction;
    QAction *m_commitAction;
    QAction *m_pullAction;
    QAction *m_pushAction;

    QAction *m_submitCurrentAction;
    QAction *m_diffSelectedFilesAction;
    QAction *m_undoAction;
    QAction *m_redoAction;
    QAction *m_stashAction;
    QAction *m_stashPopAction;
    QAction *m_stashListAction;
    QAction *m_branchListAction;

    GitClient                   *m_gitClient;
    ChangeSelectionDialog       *m_changeSelectionDialog;
    QString                     m_submitRepository;
    QStringList                 m_submitOrigCommitFiles;
    QStringList                 m_submitOrigDeleteFiles;
    QString                     m_commitMessageFileName;
    bool                        m_submitActionTriggered;
};

} // namespace Git
} // namespace Internal

#endif // GITPLUGIN_H
