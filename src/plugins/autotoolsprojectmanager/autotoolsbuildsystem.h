// Copyright (C) 2016 Openismus GmbH.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/buildsystem.h>

#include <memory>

namespace ProjectExplorer { class ProjectUpdater; }

namespace AutotoolsProjectManager::Internal {

class MakefileParserThread;

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
    void makefileParsingFinished();

    /// Return value for AutotoolsProject::files()
    QStringList m_files;

    /// Responsible for parsing the makefiles asynchronously in a thread
    std::unique_ptr<MakefileParserThread> m_makefileParserThread;

    ProjectExplorer::ProjectUpdater *m_cppCodeModelUpdater = nullptr;
};

} // AutotoolsProjectManager::Internal
