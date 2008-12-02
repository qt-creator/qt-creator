/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/
#ifndef GITPLUGIN_H
#define GITPLUGIN_H

#include "gitoutputwindow.h"
#include "settingspage.h"

#include <coreplugin/editormanager/ieditorfactory.h>
#include <coreplugin/icorelistener.h>
#include <projectexplorer/ProjectExplorerInterfaces>
#include <extensionsystem/iplugin.h>

#include <QtCore/QObject>
#include <QtCore/QProcess>
#include <QtCore/QList>

QT_BEGIN_NAMESPACE
class QFile;
class QAction;
class QTemporaryFile;
QT_END_NAMESPACE

namespace Core {
    class IEditorFactory;
    class ICore;
}

namespace Git {
namespace Internal {

    class GitPlugin;
    class GitClient;
    class ChangeSelectionDialog;
    class GitSubmitEditor;
    struct CommitData;

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

    bool                        vcsOpen(const QString &fileName);

    bool                        initialize(const QStringList &arguments
                                           , QString *error_message);
    void                        extensionsInitialized();

    QString                     getWorkingDirectory();

public slots:
    void                        updateActions();
    bool                        editorAboutToClose(Core::IEditor *editor);

private slots:
    void                        diffCurrentFile();
    void                        diffCurrentProject();
    void                        submitEditorDiff(const QStringList &);
    void                        submitCurrentLog();
    void                        statusFile();
    void                        statusProject();
    void                        logFile();
    void                        blameFile();
    void                        logProject();
    void                        undoFileChanges();
    void                        undoProjectChanges();
    void                        addFile();

    void                        showCommit();
    void                        startCommit();
    void                        pull();
    void                        push();

private:
    friend class GitClient;
    QFileInfo                   currentFile();
    Core::IEditor               *openSubmitEditor(const QString &fileName, const CommitData &cd);
    void                        cleanChangeTmpFile();

    static GitPlugin            *m_instance;
    Core::ICore                 *m_core;
    QAction                     *m_diffAction;
    QAction                     *m_diffProjectAction;
    QAction                     *m_statusAction;
    QAction                     *m_statusProjectAction;
    QAction                     *m_logAction;
    QAction                     *m_blameAction;
    QAction                     *m_logProjectAction;
    QAction                     *m_undoFileAction;
    QAction                     *m_undoProjectAction;
    QAction                     *m_showAction;
    QAction                     *m_addAction;
    QAction                     *m_commitAction;
    QAction                     *m_pullAction;
    QAction                     *m_pushAction;

    QAction                     *m_submitCurrentAction;
    QAction                     *m_diffSelectedFilesAction;
    QAction                     *m_undoAction;
    QAction                     *m_redoAction;

    ProjectExplorer::ProjectExplorerPlugin *m_projectExplorer;
    GitClient                   *m_gitClient;
    GitOutputWindow             *m_outputWindow;
    ChangeSelectionDialog       *m_changeSelectionDialog;
    SettingsPage                *m_settingsPage;
    QList<Core::IEditorFactory*> m_editorFactories;
    CoreListener                *m_coreListener;
    Core::IEditorFactory        *m_submitEditorFactory;
    QString                     m_submitRepository;
    QTemporaryFile              *m_changeTmpFile;
};

} // namespace Git
} // namespace Internal

#endif
