/****************************************************************************
**
** Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
** Contact: http://www.qt.io/licensing
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

#include "nimblebuildstep.h"
#include "nimbletaskstepwidget.h"
#include "nimconstants.h"
#include "nimbleproject.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/ioutputparser.h>
#include <projectexplorer/processparameters.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runconfigurationaspects.h>

#include <QRegularExpression>
#include <QStandardPaths>

using namespace ProjectExplorer;
using namespace Utils;

namespace Nim {

class NimParser : public OutputTaskParser
{
    Result handleLine(const QString &lne, Utils::OutputFormat) override
    {
        const QString line = lne.trimmed();
        static const QRegularExpression regex("(.+.nim)\\((\\d+), (\\d+)\\) (.+)");
        static const QRegularExpression warning("(Warning):(.*)");
        static const QRegularExpression error("(Error):(.*)");

        const QRegularExpressionMatch match = regex.match(line);
        if (!match.hasMatch())
            return Status::NotHandled;
        const QString filename = match.captured(1);
        bool lineOk = false;
        const int lineNumber = match.captured(2).toInt(&lineOk);
        const QString message = match.captured(4);
        if (!lineOk)
            return Status::NotHandled;

        Task::TaskType type = Task::Unknown;

        if (warning.match(message).hasMatch())
            type = Task::Warning;
        else if (error.match(message).hasMatch())
            type = Task::Error;
        else
            return Status::NotHandled;

        const CompileTask t(type, message, absoluteFilePath(FilePath::fromUserInput(filename)),
                            lineNumber);
        LinkSpecs linkSpecs;
        addLinkSpecForAbsoluteFilePath(linkSpecs, t.file, t.line, match, 1);
        scheduleTask(t, 1);
        return {Status::Done, linkSpecs};
    }
};

class NimbleBuildStep : public AbstractProcessStep
{
public:
    NimbleBuildStep(BuildStepList *parentList, Id id);

    bool init() final;
    void setupOutputFormatter(OutputFormatter *formatter) final;

private:
    QString defaultArguments() const;
    void onArgumentsChanged();

    ArgumentsAspect *m_arguments;
};

NimbleBuildStep::NimbleBuildStep(BuildStepList *parentList, Id id)
    : AbstractProcessStep(parentList, id)
{
    setDefaultDisplayName(tr(Constants::C_NIMBLEBUILDSTEP_DISPLAY));
    setDisplayName(tr(Constants::C_NIMBLEBUILDSTEP_DISPLAY));

    m_arguments = addAspect<ArgumentsAspect>();
    m_arguments->setSettingsKey(Constants::C_NIMBLEBUILDSTEP_ARGUMENTS);
    m_arguments->setResetter([this] { return defaultArguments(); });

    QTC_ASSERT(buildConfiguration(), return);
    QObject::connect(buildConfiguration(), &BuildConfiguration::buildTypeChanged,
                     m_arguments, &ArgumentsAspect::resetArguments);
    QObject::connect(m_arguments, &ArgumentsAspect::argumentsChanged,
                     this, &NimbleBuildStep::onArgumentsChanged);
}

bool NimbleBuildStep::init()
{
    m_arguments->setArguments(defaultArguments());
    ProcessParameters *params = processParameters();
    params->setEnvironment(buildEnvironment());
    params->setMacroExpander(macroExpander());
    params->setWorkingDirectory(project()->projectDirectory());
    params->setCommandLine({QStandardPaths::findExecutable("nimble"),
                            {"build", m_arguments->arguments(macroExpander())}});
    return AbstractProcessStep::init();
}

void NimbleBuildStep::setupOutputFormatter(OutputFormatter *formatter)
{
    const auto parser = new NimParser();
    parser->addSearchDir(project()->projectDirectory());
    formatter->addLineParser(parser);
    AbstractProcessStep::setupOutputFormatter(formatter);
}

QString NimbleBuildStep::defaultArguments() const
{
    switch (buildType()) {
    case BuildConfiguration::Debug:
        return {"--debugger:native"};
    case BuildConfiguration::Unknown:
    case BuildConfiguration::Profile:
    case BuildConfiguration::Release:
    default:
        return {};
    }
}

void NimbleBuildStep::onArgumentsChanged()
{
    ProcessParameters *params = processParameters();
    params->setCommandLine({QStandardPaths::findExecutable("nimble"),
                            {"build", m_arguments->arguments(macroExpander())}});
}

NimbleBuildStepFactory::NimbleBuildStepFactory()
{
    registerStep<NimbleBuildStep>(Constants::C_NIMBLEBUILDSTEP_ID);
    setDisplayName(NimbleBuildStep::tr("Nimble Build"));
    setSupportedStepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
    setSupportedConfiguration(Constants::C_NIMBLEBUILDCONFIGURATION_ID);
    setRepeatable(true);
}

} // Nim
