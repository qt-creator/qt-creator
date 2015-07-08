/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef PROJECTEXPLORER_H
#define PROJECTEXPLORER_H

#include "projectexplorer_export.h"
#include "projectexplorerconstants.h"
#include "runconfiguration.h"

#include <extensionsystem/iplugin.h>

#include <QPair>

QT_BEGIN_NAMESPACE
class QPoint;
class QAction;
QT_END_NAMESPACE

namespace Core {
class IMode;
class Id;
}

namespace ProjectExplorer {
class RunControl;
class RunConfiguration;
class IRunControlFactory;
class Project;
class Node;
class FolderNode;
class FileNode;

namespace Internal { class ProjectExplorerSettings; }

class PROJECTEXPLORER_EXPORT ProjectExplorerPlugin
    : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "ProjectExplorer.json")

public:
    ProjectExplorerPlugin();
    ~ProjectExplorerPlugin();

    static ProjectExplorerPlugin *instance();

    static Project *openProject(const QString &fileName, QString *error);
    static QList<Project *> openProjects(const QStringList &fileNames, QString *error);
    Q_SLOT void openProjectWelcomePage(const QString &fileName);
    static void unloadProject(Project *project);

    static bool saveModifiedFiles();

    static void showContextMenu(QWidget *view, const QPoint &globalPos, Node *node);

    //PluginInterface
    bool initialize(const QStringList &arguments, QString *errorMessage);
    void extensionsInitialized();
    ShutdownFlag aboutToShutdown();

    static void setProjectExplorerSettings(const Internal::ProjectExplorerSettings &pes);
    static Internal::ProjectExplorerSettings projectExplorerSettings();

    static void startRunControl(RunControl *runControl, Core::Id runMode);
    static void showRunErrorMessage(const QString &errorMessage);

    // internal public for FlatModel
    static void renameFile(Node *node, const QString &newFilePath);
    static QStringList projectFilePatterns();
    static bool coreAboutToClose();
    static QList<QPair<QString, QString> > recentProjects();

    static bool canRun(Project *pro, Core::Id runMode, QString *whyNot = 0);
    static void runProject(Project *pro, Core::Id, const bool forceSkipDeploy = false);
    static void runStartupProject(Core::Id runMode, bool forceSkipDeploy = false);
    static void runRunConfiguration(RunConfiguration *rc, Core::Id runMode,
                             const bool forceSkipDeploy = false);

    static void addExistingFiles(FolderNode *projectNode, const QStringList &filePaths);
    static void addExistingFiles(const QStringList &filePaths, FolderNode *folderNode);

    static void buildProject(Project *p);
    /// Normally there's no need to call this function.
    /// This function needs to be called, only if the pages that support a project changed.
    static void requestProjectModeUpdate(Project *p);

    static void initiateInlineRenaming();

    static QString displayNameForStepId(Core::Id stepId);

    static QString directoryFor(Node *node);
    static QStringList projectFileGlobs();

    static void updateContextMenuActions();

signals:
    void runControlStarted(ProjectExplorer::RunControl *rc);
    void runControlFinished(ProjectExplorer::RunControl *rc);

    // Is emitted when a project has been added/removed,
    // or the file list of a specific project has changed.
    void fileListChanged();

    void aboutToExecuteProject(ProjectExplorer::Project *project, Core::Id runMode);
    void recentProjectsChanged();

    void settingsChanged();

    void updateRunActions();

public slots:
    static void openOpenProjectDialog();

private slots:
    void restoreSession2();
    void showRenameFileError();

#ifdef WITH_TESTS
    void testAnsiFilterOutputParser_data();
    void testAnsiFilterOutputParser();

    void testGccOutputParsers_data();
    void testGccOutputParsers();

    void testCustomOutputParsers_data();
    void testCustomOutputParsers();

    void testClangOutputParser_data();
    void testClangOutputParser();

    void testLinuxIccOutputParsers_data();
    void testLinuxIccOutputParsers();

    void testGnuMakeParserParsing_data();
    void testGnuMakeParserParsing();
    void testGnuMakeParserTaskMangling_data();
    void testGnuMakeParserTaskMangling();

    void testXcodebuildParserParsing_data();
    void testXcodebuildParserParsing();

    void testMsvcOutputParsers_data();
    void testMsvcOutputParsers();

    void testGccAbiGuessing_data();
    void testGccAbiGuessing();

    void testAbiOfBinary_data();
    void testAbiOfBinary();
    void testFlavorForOs();
    void testAbiFromTargetTriplet_data();
    void testAbiFromTargetTriplet();

    void testDeviceManager();

    void testCustomWizardPreprocessor_data();
    void testCustomWizardPreprocessor();
#endif
};

} // namespace ProjectExplorer

#endif // PROJECTEXPLORER_H
