// Copyright (C) 2016 Openismus GmbH.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/buildsystem.h>

#include <solutions/tasking/tasktreerunner.h>

namespace ProjectExplorer { class ProjectUpdater; }

namespace AutotoolsProjectManager::Internal {

class MakefileParserOutputData;

class AutotoolsBuildSystem final : public ProjectExplorer::BuildSystem
{
public:
    explicit AutotoolsBuildSystem(ProjectExplorer::Target *target);
    ~AutotoolsBuildSystem() final;

private:
    void triggerParsing() final;
    QString name() const final { return QLatin1String("autotools"); }

    /**
     * Is invoked when the makefile parsing by m_makefileParserThread has
     * been finished. Adds all sources and files into the project tree and
     * takes care listen to file changes for Makefile.am and configure.ac
     * files.
     */
    void makefileParsingFinished(const MakefileParserOutputData &outputData);

    /// Return value for AutotoolsProject::files()
    QStringList m_files;

    /// Responsible for parsing the makefiles asynchronously in a thread
    Tasking::TaskTreeRunner m_parserRunner;

    std::unique_ptr<ProjectExplorer::ProjectUpdater> m_cppCodeModelUpdater;
};

} // AutotoolsProjectManager::Internal
