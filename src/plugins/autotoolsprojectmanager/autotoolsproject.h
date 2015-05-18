/**************************************************************************
**
** Copyright (C) 2015 Openismus GmbH.
** Authors: Peter Penz (ppenz@openismus.com)
**          Patricia Santana Cruz (patriciasantanacruz@gmail.com)
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

#ifndef AUTOTOOLSPROJECT_H
#define AUTOTOOLSPROJECT_H

#include <projectexplorer/project.h>

#include <utils/fileutils.h>

#include <QFuture>

QT_FORWARD_DECLARE_CLASS(QDir)

namespace Utils { class FileSystemWatcher; }

namespace ProjectExplorer {
class Node;
class FolderNode;
}

namespace AutotoolsProjectManager {
namespace Internal {
class AutotoolsConfigurationFactory;
class AutotoolsProjectFile;
class AutotoolsProjectNode;
class AutotoolsManager;
class MakefileParserThread;
class AutotoolsTarget;

/**
 * @brief Implementation of the ProjectExplorer::Project interface.
 *
 * Loads the autotools project and embeds it into the QtCreator project tree.
 * The class AutotoolsProject is the core of the autotools project plugin.
 * It is responsible to parse the Makefile.am files and do trigger project
 * updates if a Makefile.am file or a configure.ac file has been changed.
 */
class AutotoolsProject : public ProjectExplorer::Project
{
    Q_OBJECT

public:
    AutotoolsProject(AutotoolsManager *manager, const QString &fileName);
    ~AutotoolsProject();

    QString displayName() const;
    Core::IDocument *document() const;
    ProjectExplorer::IProjectManager *projectManager() const;
    ProjectExplorer::ProjectNode *rootProjectNode() const;
    QStringList files(FilesMode fileMode) const;
    static QString defaultBuildDirectory(const QString &projectPath);
    QStringList buildTargets() const;

protected:
    RestoreResult fromMap(const QVariantMap &map, QString *errorMessage);

private slots:
    /**
     *  Loads the project tree by parsing the makefiles.
     */
    void loadProjectTree();

    /**
     * Is invoked when the makefile parsing by m_makefileParserThread has
     * been started. Turns the mouse cursor into a busy cursor.
     */
    void makefileParsingStarted();

    /**
     * Is invoked when the makefile parsing by m_makefileParserThread has
     * been finished. Adds all sources and files into the project tree and
     * takes care listen to file changes for Makefile.am and configure.ac
     * files.
     */
    void makefileParsingFinished();

    /**
     * Is invoked, if a file of the project tree has been changed by the user.
     * If a Makefile.am or a configure.ac file has been changed, the project
     * configuration must be updated.
     */
    void onFileChanged(const QString &file);

private:
    /**
     * Creates folder-nodes and file-nodes for the project tree.
     */
    void buildFileNodeTree(const QDir &directory,
                           const QStringList &files);

    /**
     * Helper function for buildFileNodeTree(): Inserts a new folder-node for
     * the directory \p nodeDir and inserts it into \p nodes. If no parent
     * folder exists, it will be created recursively.
     */
    ProjectExplorer::FolderNode *insertFolderNode(const QDir &nodeDir,
                                                  QHash<QString, ProjectExplorer::Node *> &nodes);

    /**
     * @return All nodes (including sub-folder- and file-nodes) for the given parent folder.
     */
    QList<ProjectExplorer::Node *> nodes(ProjectExplorer::FolderNode *parent) const;

    /**
     * This function is in charge of the code completion.
     */
    void updateCppCodeModel();

private:
    /// Project manager that has been passed in the constructor
    AutotoolsManager *m_manager;

    /// File name of the makefile that has been passed in the constructor
    QString m_fileName;
    QString m_projectName;

    /// Return value for AutotoolsProject::files()
    QStringList m_files;

    /// Return value for AutotoolsProject::file()
    AutotoolsProjectFile *m_file;

    /// Return value for AutotoolsProject::rootProjectNode()
    AutotoolsProjectNode *m_rootNode;

    /// Watches project files for changes.
    Utils::FileSystemWatcher *m_fileWatcher;
    QStringList m_watchedFiles;

    /// Responsible for parsing the makefiles asynchronously in a thread
    MakefileParserThread *m_makefileParserThread;

    QFuture<void> m_codeModelFuture;
};

} // namespace Internal
} // namespace AutotoolsProjectManager

#endif // AUTOTOOLSPROJECT_H
