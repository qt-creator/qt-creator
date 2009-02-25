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

#ifndef GITPLUGIN_H
#define GITPLUGIN_H

#include "gitoutputwindow.h"
#include "settingspage.h"

#include <coreplugin/editormanager/ieditorfactory.h>
#include <coreplugin/icorelistener.h>
#include <extensionsystem/iplugin.h>
#include <projectexplorer/projectexplorer.h>

#include <QtCore/QObject>
#include <QtCore/QProcess>
#include <QtCore/QStringList>

QT_BEGIN_NAMESPACE
class QFile;
class QAction;
class QTemporaryFile;
QT_END_NAMESPACE

namespace Core {
class IEditorFactory;
class ICore;
class IVersionControl;
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

    bool initialize(const QStringList &arguments, QString *error_message);
    void extensionsInitialized();

    QString getWorkingDirectory();

    GitOutputWindow *outputWindow() const;


    GitSettings settings() const;
    void setSettings(const GitSettings &s);

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
    void revertFile();

    void showCommit();
    void startCommit();
    void stash();
    void stashPop();
    void branchList();
    void stashList();
    void pull();
    void push();

private:
    QFileInfo currentFile() const;
    Core::IEditor *openSubmitEditor(const QString &fileName, const CommitData &cd);
    void cleanChangeTmpFile();

    static GitPlugin *m_instance;
    Core::ICore *m_core;
    QAction *m_diffAction;
    QAction *m_diffProjectAction;
    QAction *m_statusAction;
    QAction *m_statusProjectAction;
    QAction *m_logAction;
    QAction *m_blameAction;
    QAction *m_logProjectAction;
    QAction *m_undoFileAction;
    QAction *m_undoProjectAction;
    QAction *m_showAction;
    QAction *m_stageAction;
    QAction *m_unstageAction;
    QAction *m_revertAction;
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

    ProjectExplorer::ProjectExplorerPlugin *m_projectExplorer;
    GitClient                   *m_gitClient;
    GitOutputWindow             *m_outputWindow;
    ChangeSelectionDialog       *m_changeSelectionDialog;
    SettingsPage                *m_settingsPage;
    QList<Core::IEditorFactory*> m_editorFactories;
    CoreListener                *m_coreListener;
    Core::IEditorFactory        *m_submitEditorFactory;
    Core::IVersionControl       *m_versionControl;
    QString                     m_submitRepository;
    QStringList                 m_submitOrigCommitFiles;
    QTemporaryFile              *m_changeTmpFile;
};

} // namespace Git
} // namespace Internal

#endif // GITPLUGIN_H
