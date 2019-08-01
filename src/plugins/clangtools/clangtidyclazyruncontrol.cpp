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

#include "clangtidyclazyruncontrol.h"

#include "clangtidyclazyrunner.h"
#include "clangtidyclazytool.h"

using namespace ProjectExplorer;

namespace ClangTools {
namespace Internal {

ClangTidyClazyRunWorker::ClangTidyClazyRunWorker(RunControl *runControl,
        const CppTools::ClangDiagnosticConfig &diagnosticConfig,
        const FileInfos &fileInfos)
    : ClangToolRunWorker(runControl, fileInfos)
    , m_diagnosticConfig(diagnosticConfig)
{
    setId("ClangTidyClazyRunner");
    init();
}

QList<RunnerCreator> ClangTidyClazyRunWorker::runnerCreators()
{
    QList<RunnerCreator> creators;

    if (!m_diagnosticConfig.clazyChecks().isEmpty())
        creators << [this]() { return createRunner<ClazyRunner>(); };
    if (m_diagnosticConfig.clangTidyMode() != CppTools::ClangDiagnosticConfig::TidyMode::Disabled)
        creators << [this]() { return createRunner<ClangTidyRunner>(); };

    return creators;
}

template <class T>
ClangToolRunner *ClangTidyClazyRunWorker::createRunner()
{
    auto runner = new T(m_diagnosticConfig, this);
    runner->init(m_clangExecutable, m_temporaryDir.path(), m_environment);
    connect(runner, &ClangToolRunner::finishedWithSuccess,
            this, &ClangTidyClazyRunWorker::onRunnerFinishedWithSuccess);
    connect(runner, &ClangToolRunner::finishedWithFailure,
            this, &ClangTidyClazyRunWorker::onRunnerFinishedWithFailure);
    return runner;
}

ClangTool *ClangTidyClazyRunWorker::tool()
{
    return ClangTidyClazyTool::instance();
}

} // namespace Internal
} // namespace ClangTools

