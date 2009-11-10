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

#ifndef CVSPLUGIN_H
#define CVSPLUGIN_H

#include "cvssettings.h"
#include "cvsutils.h"

#include <coreplugin/icorelistener.h>
#include <extensionsystem/iplugin.h>

QT_BEGIN_NAMESPACE
class QDir;
class QAction;
class QTextCodec;
QT_END_NAMESPACE

namespace Core {
    class IEditorFactory;
    class IVersionControl;
}

namespace Utils {
    class ParameterAction;
}

namespace ProjectExplorer {
    class ProjectExplorerPlugin;
}

namespace CVS {
namespace Internal {

class CVSSubmitEditor;

struct CVSResponse
{
    enum Result { Ok, NonNullExitCode, OtherError };
    CVSResponse() : result(Ok) {}

    Result result;
    QString stdOut;
    QString stdErr;
    QString message;
    QString workingDirectory;
};

/* This plugin differs from the other VCS plugins in that it
 * runs CVS commands from a working directory using relative
 * path specifications, which is a requirement imposed by
 * Tortoise CVS. This has to be taken into account; for example,
 * the diff editor has an additional property specifying the
 * base directory for its interaction to work. */

class CVSPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT

public:
    CVSPlugin();
    ~CVSPlugin();

    virtual bool initialize(const QStringList &arguments, QString *error_message);
    virtual void extensionsInitialized();
    virtual bool editorAboutToClose(Core::IEditor *editor);

    void cvsDiff(const QStringList &files, QString diffname = QString());

    CVSSubmitEditor *openCVSSubmitEditor(const QString &fileName);

    CVSSettings settings() const;
    void setSettings(const CVSSettings &s);

    // IVersionControl
    bool vcsAdd(const QString &fileName);
    bool vcsDelete(const QString &fileName);
    bool managesDirectory(const QString &directory) const;
    QString findTopLevelForDirectory(const QString &directory) const;

    static CVSPlugin *cvsPluginInstance();

private slots:
    void updateActions();
    void addCurrentFile();
    void deleteCurrentFile();
    void revertCurrentFile();
    void diffProject();
    void diffCurrentFile();
    void startCommitAll();
    void startCommitCurrentFile();
    void filelogCurrentFile();
    void annotateCurrentFile();
    void projectStatus();
    void slotDescribe(const QString &source, const QString &changeNr);
    void updateProject();
    void submitCurrentLog();
    void diffFiles(const QStringList &);

private:
    bool isCommitEditorOpen() const;
    QString currentFileName() const;
    Core::IEditor * showOutputInEditor(const QString& title, const QString &output,
                                       int editorType, const QString &source,
                                       QTextCodec *codec);
    CVSResponse runCVS(const QStringList &arguments,
                       QStringList fileArguments,
                       int timeOut,
                       bool showStdOutInOutputWindow, QTextCodec *outputCodec = 0,
                       bool mergeStderr = false);

    CVSResponse runCVS(const QString &workingDirectory,
                       const QStringList &arguments,
                       int timeOut,
                       bool showStdOutInOutputWindow, QTextCodec *outputCodec = 0,
                       bool mergeStderr = false);

    void annotate(const QString &file);
    bool describe(const QString &source, const QString &changeNr, QString *errorMessage);
    bool describe(const QString &repository, QList<CVS_LogEntry> entries, QString *errorMessage);
    void filelog(const QString &file);
    bool managesDirectory(const QDir &directory) const;
    QString findTopLevelForDirectoryI(const QString &directory) const;
    QStringList currentProjectsTopLevels(QString *name = 0) const;
    void startCommit(const QString &file);
    bool commit(const QString &messageFile, const QStringList &subVersionFileList);
    void cleanCommitMessageFile();

    CVSSettings m_settings;
    Core::IVersionControl *m_versionControl;
    QString m_commitMessageFileName;

    ProjectExplorer::ProjectExplorerPlugin *m_projectExplorer;

    Utils::ParameterAction *m_addAction;
    Utils::ParameterAction *m_deleteAction;
    Utils::ParameterAction *m_revertAction;
    QAction *m_diffProjectAction;
    Utils::ParameterAction *m_diffCurrentAction;
    QAction *m_commitAllAction;
    Utils::ParameterAction *m_commitCurrentAction;
    Utils::ParameterAction *m_filelogCurrentAction;
    Utils::ParameterAction *m_annotateCurrentAction;
    QAction *m_statusAction;
    QAction *m_updateProjectAction;

    QAction *m_submitCurrentLogAction;
    QAction *m_submitDiffAction;
    QAction *m_submitUndoAction;
    QAction *m_submitRedoAction;
    bool    m_submitActionTriggered;

    static CVSPlugin *m_cvsPluginInstance;
};

// Just a proxy for CVSPlugin
class CoreListener : public Core::ICoreListener
{
    Q_OBJECT
public:
    CoreListener(CVSPlugin *plugin) : m_plugin(plugin) { }

    // Start commit when submit editor closes
    bool editorAboutToClose(Core::IEditor *editor) {
        return m_plugin->editorAboutToClose(editor);
    }

    // TODO: how to handle that ???
    bool coreAboutToClose() {
        return true;
    }

private:
    CVSPlugin *m_plugin;
};

} // namespace CVS
} // namespace Internal

#endif // CVSPLUGIN_H
