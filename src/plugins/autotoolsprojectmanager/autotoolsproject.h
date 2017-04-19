/****************************************************************************
**
** Copyright (C) 2016 Openismus GmbH.
** Author: Peter Penz (ppenz@openismus.com)
** Author: Patricia Santana Cruz (patriciasantanacruz@gmail.com)
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <projectexplorer/project.h>

#include <utils/fileutils.h>

namespace Utils { class FileSystemWatcher; }

namespace CppTools { class CppProjectUpdater; }

namespace AutotoolsProjectManager {
namespace Internal {

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
    explicit AutotoolsProject(const Utils::FileName &fileName);
    ~AutotoolsProject() override;

    static QString defaultBuildDirectory(const QString &projectPath);
    QStringList buildTargets() const;

protected:
    RestoreResult fromMap(const QVariantMap &map, QString *errorMessage) override;

private:
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

    /**
     * This function is in charge of the code completion.
     */
    void updateCppCodeModel();

    /// Return value for AutotoolsProject::files()
    QStringList m_files;

    /// Watches project files for changes.
    Utils::FileSystemWatcher *m_fileWatcher;
    QStringList m_watchedFiles;

    /// Responsible for parsing the makefiles asynchronously in a thread
    MakefileParserThread *m_makefileParserThread = nullptr;

    CppTools::CppProjectUpdater *m_cppCodeModelUpdater = nullptr;
};

} // namespace Internal
} // namespace AutotoolsProjectManager
