/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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
#include <projectexplorer/treescanner.h>

#include <QTimer>

namespace CppTools {
class CppProjectUpdater;
} // namespace CppTools

namespace ProjectExplorer {
class BuildConfiguration;
class ExtraCompiler;
} // namespace ProjectExplorer

namespace CMakeProjectManager {

class CMakeProject;

namespace Internal {
class CMakeBuildConfiguration;
} // namespace Internal

// --------------------------------------------------------------------
// BuildSystem:
// --------------------------------------------------------------------

class BuildSystem : public QObject
{
    Q_OBJECT

public:
    const static int PARAM_CUSTOM_OFFSET = 3;
    enum Parameters : int {
        PARAM_DEFAULT = 0, // use defaults

        PARAM_IGNORE = (1 << (PARAM_CUSTOM_OFFSET - 3)), // Ignore this request without raising a fuss
        PARAM_ERROR = (1 << (PARAM_CUSTOM_OFFSET - 2)), // Ignore this request and warn

        PARAM_URGENT = (1 << (PARAM_CUSTOM_OFFSET - 1)), // Do not wait for more requests, start ASAP
    };

    explicit BuildSystem(ProjectExplorer::Project *project);
    ~BuildSystem() override;

    BuildSystem(const BuildSystem &other) = delete;

    ProjectExplorer::Project *project() const;

    bool isWaitingForParse() const;
    void requestParse(int reparseParameters); // request a (delayed!) parser run.

protected:
    class ParsingContext
    {
    public:
        ParsingContext() = default;

        ParsingContext(const ParsingContext &other) = delete;
        ParsingContext &operator=(const ParsingContext &other) = delete;
        ParsingContext(ParsingContext &&other) = default;
        ParsingContext &operator=(ParsingContext &&other) = default;

        ProjectExplorer::Project::ParseGuard guard;

        int parameters = PARAM_DEFAULT;
        ProjectExplorer::Project *project = nullptr;
        ProjectExplorer::BuildConfiguration *buildConfiguration = nullptr;

    private:
        ParsingContext(ProjectExplorer::Project::ParseGuard &&g,
                       int params,
                       ProjectExplorer::Project *p,
                       ProjectExplorer::BuildConfiguration *bc)
            : guard(std::move(g))
            , parameters(params)
            , project(p)
            , buildConfiguration(bc)
        {}

        friend class BuildSystem;
    };

    virtual bool validateParsingContext(const ParsingContext &ctx)
    {
        Q_UNUSED(ctx)
        return true;
    }

    virtual void parseProject(ParsingContext &&ctx) = 0; // actual code to parse project

private:
    void triggerParsing();

    ProjectExplorer::Project *m_project;

    QTimer m_delayedParsingTimer;
    int m_delayedParsingParameters = PARAM_DEFAULT;
};

// --------------------------------------------------------------------
// CMakeBuildSystem:
// --------------------------------------------------------------------

class CMakeBuildSystem : public BuildSystem
{
    Q_OBJECT

public:
    explicit CMakeBuildSystem(CMakeProject *project);
    ~CMakeBuildSystem() final;

protected:
    bool validateParsingContext(const ParsingContext &ctx) final;
    void parseProject(ParsingContext &&ctx) final;

private:
    // Treescanner states:
    void handleTreeScanningFinished();

    // Parser states:
    void handleParsingSuccess(Internal::CMakeBuildConfiguration *bc);
    void handleParsingError(Internal::CMakeBuildConfiguration *bc);

    // Combining Treescanner and Parser states:
    void combineScanAndParse();

    void updateProjectData(CMakeProject *p, Internal::CMakeBuildConfiguration *bc);
    QList<ProjectExplorer::ExtraCompiler *> findExtraCompilers(CMakeProject *p);
    void updateQmlJSCodeModel(CMakeProject *p, Internal::CMakeBuildConfiguration *bc);

    ProjectExplorer::TreeScanner m_treeScanner;
    QHash<QString, bool> m_mimeBinaryCache;
    QList<const ProjectExplorer::FileNode *> m_allFiles;

    bool m_waitingForScan = false;
    bool m_waitingForParse = false;
    bool m_combinedScanAndParseResult = false;

    ParsingContext m_currentContext;

    CppTools::CppProjectUpdater *m_cppCodeModelUpdater = nullptr;
    QList<ProjectExplorer::ExtraCompiler *> m_extraCompilers;

    friend class Internal::CMakeBuildConfiguration; // For handleParsing* callbacks
};

} // namespace CMakeProjectManager
