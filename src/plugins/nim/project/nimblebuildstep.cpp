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
#include "nimblebuildstepwidget.h"
#include "nimbletaskstepwidget.h"
#include "nimconstants.h"
#include "nimbleproject.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/ioutputparser.h>
#include <projectexplorer/processparameters.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <QRegularExpression>
#include <QStandardPaths>

using namespace Nim;
using namespace ProjectExplorer;
using namespace Utils;

namespace {

class NimParser : public IOutputParser
{
public:
    void stdOutput(const QString &line) final
    {
        parseLine(line.trimmed());
        IOutputParser::stdOutput(line);
    }

    void stdError(const QString &line) final
    {
        parseLine(line.trimmed());
        IOutputParser::stdError(line);
    }

private:
    void parseLine(const QString &line)
    {
        static QRegularExpression regex("(.+.nim)\\((\\d+), (\\d+)\\) (.+)",
                                        QRegularExpression::OptimizeOnFirstUsageOption);
        static QRegularExpression warning("(Warning):(.*)",
                                          QRegularExpression::OptimizeOnFirstUsageOption);
        static QRegularExpression error("(Error):(.*)",
                                        QRegularExpression::OptimizeOnFirstUsageOption);

        QRegularExpressionMatch match = regex.match(line);
        if (!match.hasMatch())
            return;
        const QString filename = match.captured(1);
        bool lineOk = false;
        const int lineNumber = match.captured(2).toInt(&lineOk);
        const QString message = match.captured(4);
        if (!lineOk)
            return;

        Task::TaskType type = Task::Unknown;

        if (warning.match(message).hasMatch())
            type = Task::Warning;
        else if (error.match(message).hasMatch())
            type = Task::Error;
        else
            return;

        emit addTask(CompileTask(type, message, FilePath::fromUserInput(filename), lineNumber));
    }
};

}

NimbleBuildStep::NimbleBuildStep(BuildStepList *parentList, Core::Id id)
    : AbstractProcessStep(parentList, id)
{
    setDefaultDisplayName(tr(Constants::C_NIMBLEBUILDSTEP_DISPLAY));
    setDisplayName(tr(Constants::C_NIMBLEBUILDSTEP_DISPLAY));
    QTC_ASSERT(buildConfiguration(), return);
    QObject::connect(buildConfiguration(), &BuildConfiguration::buildTypeChanged, this, &NimbleBuildStep::resetArguments);
    QObject::connect(this, &NimbleBuildStep::argumentsChanged, this, &NimbleBuildStep::onArgumentsChanged);
    resetArguments();
}

bool NimbleBuildStep::init()
{
    auto parser = new NimParser();
    parser->setWorkingDirectory(project()->projectDirectory());
    setOutputParser(parser);

    ProcessParameters* params = processParameters();
    params->setEnvironment(buildConfiguration()->environment());
    params->setMacroExpander(buildConfiguration()->macroExpander());
    params->setWorkingDirectory(project()->projectDirectory());
    params->setCommandLine({QStandardPaths::findExecutable("nimble"), {"build", m_arguments}});
    return AbstractProcessStep::init();
}

BuildStepConfigWidget *NimbleBuildStep::createConfigWidget()
{
    return new NimbleBuildStepWidget(this);
}

QString NimbleBuildStep::arguments() const
{
    return m_arguments;
}

void NimbleBuildStep::setArguments(const QString &args)
{
    if (m_arguments == args)
        return;
    m_arguments = args;
    emit argumentsChanged(args);
}

void NimbleBuildStep::resetArguments()
{
    setArguments(defaultArguments());
}

bool NimbleBuildStep::fromMap(const QVariantMap &map)
{
    m_arguments = map.value(Constants::C_NIMBLEBUILDSTEP_ARGUMENTS, defaultArguments()).toString();
    return AbstractProcessStep::fromMap(map);
}

QVariantMap NimbleBuildStep::toMap() const
{
    auto map = AbstractProcessStep::toMap();
    map[Constants::C_NIMBLEBUILDSTEP_ARGUMENTS] = m_arguments;
    return map;
}

QString NimbleBuildStep::defaultArguments() const
{
    QTC_ASSERT(buildConfiguration(), return {}; );
    switch (buildConfiguration()->buildType()) {
    case ProjectExplorer::BuildConfiguration::Debug:
        return {"--debugger:native"};
    case ProjectExplorer::BuildConfiguration::Unknown:
    case ProjectExplorer::BuildConfiguration::Profile:
    case ProjectExplorer::BuildConfiguration::Release:
    default:
        return {};
    }
}

void NimbleBuildStep::onArgumentsChanged()
{
    ProcessParameters* params = processParameters();
    params->setCommandLine({QStandardPaths::findExecutable("nimble"), {"build", m_arguments}});
}

NimbleBuildStepFactory::NimbleBuildStepFactory()
{
    registerStep<NimbleBuildStep>(Constants::C_NIMBLEBUILDSTEP_ID);
    setDisplayName(NimbleBuildStep::tr("Nimble Build"));
    setSupportedStepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
    setSupportedConfiguration(Constants::C_NIMBLEBUILDCONFIGURATION_ID);
    setRepeatable(true);
}
