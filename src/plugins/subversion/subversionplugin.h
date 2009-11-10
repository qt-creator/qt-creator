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

#ifndef SUBVERSIONPLUGIN_H
#define SUBVERSIONPLUGIN_H

#include "subversionsettings.h"

#include <coreplugin/icorelistener.h>
#include <extensionsystem/iplugin.h>

QT_BEGIN_NAMESPACE
class QDir;
class QAction;
class QTextCodec;
QT_END_NAMESPACE

namespace Core {
    class IVersionControl;
}
namespace Utils {
    class ParameterAction;
}

namespace ProjectExplorer {
    class ProjectExplorerPlugin;
}

namespace Subversion {
namespace Internal {

class SubversionSubmitEditor;

struct SubversionResponse
{
    SubversionResponse() : error(false) {}
    bool error;
    QString stdOut;
    QString stdErr;
    QString message;
};

class SubversionPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT

public:
    SubversionPlugin();
    ~SubversionPlugin();

    bool initialize(const QStringList &arguments, QString *error_message);
    void extensionsInitialized();
    bool editorAboutToClose(Core::IEditor *editor);

    void svnDiff(const QStringList &files, QString diffname = QString());

    SubversionSubmitEditor *openSubversionSubmitEditor(const QString &fileName);

    SubversionSettings settings() const;
    void setSettings(const SubversionSettings &s);

    // IVersionControl
    bool vcsAdd(const QString &fileName);
    bool vcsDelete(const QString &fileName);
    bool managesDirectory(const QString &directory) const;
    QString findTopLevelForDirectory(const QString &directory) const;

    static SubversionPlugin *subversionPluginInstance();

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
    void describe(const QString &source, const QString &changeNr);
    void slotDescribe();
    void updateProject();
    void submitCurrentLog();
    void diffFiles(const QStringList &);

private:
    inline bool isCommitEditorOpen() const;
    QString currentFileName() const;
    Core::IEditor * showOutputInEditor(const QString& title, const QString &output,
                                       int editorType, const QString &source,
                                       QTextCodec *codec);
    SubversionResponse runSvn(const QStringList &arguments, int timeOut,
                              bool showStdOutInOutputWindow, QTextCodec *outputCodec = 0);
    void annotate(const QString &file);
    void filelog(const QString &file);
    bool managesDirectory(const QDir &directory) const;
    QString findTopLevelForDirectoryI(const QString &directory) const;
    QStringList currentProjectsTopLevels(QString *name = 0) const;
    void startCommit(const QStringList &files);
    bool commit(const QString &messageFile, const QStringList &subVersionFileList);
    void cleanCommitMessageFile();

    const QStringList m_svnDirectories;

    SubversionSettings m_settings;
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
    QAction *m_describeAction;

    QAction *m_submitCurrentLogAction;
    QAction *m_submitDiffAction;
    QAction *m_submitUndoAction;
    QAction *m_submitRedoAction;
    bool    m_submitActionTriggered;

    static SubversionPlugin *m_subversionPluginInstance;
};

// Just a proxy for SubversionPlugin
class CoreListener : public Core::ICoreListener
{
    Q_OBJECT
public:
    CoreListener(SubversionPlugin *plugin) : m_plugin(plugin) { }

    // Start commit when submit editor closes
    bool editorAboutToClose(Core::IEditor *editor) {
        return m_plugin->editorAboutToClose(editor);
    }

    // TODO: how to handle that ???
    bool coreAboutToClose() {
        return true;
    }

private:
    SubversionPlugin *m_plugin;
};

} // namespace Subversion
} // namespace Internal

#endif // SUBVERSIONPLUGIN_H
