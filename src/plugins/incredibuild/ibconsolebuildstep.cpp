/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "ibconsolebuildstep.h"

#include "cmakecommandbuilder.h"
#include "ibconsolestepconfigwidget.h"
#include "incredibuildconstants.h"
#include "incredibuildconstants.h"
#include "makecommandbuilder.h"
#include "ui_ibconsolebuildstep.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/gnumakeparser.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/processparameters.h>
#include <projectexplorer/target.h>
#include <utils/environment.h>

using namespace ProjectExplorer;

namespace IncrediBuild {
namespace Internal {

namespace Constants {
const QLatin1String IBCONSOLE_NICE("IncrediBuild.IBConsole.Nice");
const QLatin1String IBCONSOLE_COMMANDBUILDER("IncrediBuild.IBConsole.CommandBuilder");
const QLatin1String IBCONSOLE_KEEPJOBNUM("IncrediBuild.IBConsole.KeepJobNum");
const QLatin1String IBCONSOLE_FORCEREMOTE("IncrediBuild.IBConsole.ForceRemote");
const QLatin1String IBCONSOLE_ALTERNATE("IncrediBuild.IBConsole.Alternate");
}

IBConsoleBuildStep::IBConsoleBuildStep(ProjectExplorer::BuildStepList *buildStepList, Utils::Id id)
    : ProjectExplorer::AbstractProcessStep(buildStepList, id)
    , m_earlierSteps(buildStepList)
{
    setDisplayName("IncrediBuild for Linux");
    initCommandBuilders();
}

IBConsoleBuildStep::~IBConsoleBuildStep()
{
    for (CommandBuilder* p : m_commandBuildersList) {
        delete p;
        p = nullptr;
    }
}

void IBConsoleBuildStep::tryToMigrate()
{
    // This is called when creating a fresh build step.
    // Attempt to detect build system from pre-existing steps.
    for (CommandBuilder* p : m_commandBuildersList) {
        if (p->canMigrate(m_earlierSteps)) {
            m_activeCommandBuilder = p;
            break;
        }
    }
}

void IBConsoleBuildStep::setupOutputFormatter(Utils::OutputFormatter *formatter)
{
    formatter->addLineParser(new GnuMakeParser());
    formatter->addLineParsers(target()->kit()->createOutputParsers());
    formatter->addSearchDir(processParameters()->effectiveWorkingDirectory());
    AbstractProcessStep::setupOutputFormatter(formatter);
}

bool IBConsoleBuildStep::fromMap(const QVariantMap &map)
{
    m_loadedFromMap = true;
    m_nice = map.value(Constants::IBCONSOLE_NICE, QVariant(0)).toInt();
    m_keepJobNum = map.value(Constants::IBCONSOLE_KEEPJOBNUM, QVariant(false)).toBool();
    m_forceRemote = map.value(Constants::IBCONSOLE_FORCEREMOTE, QVariant(false)).toBool();
    m_forceRemote = map.value(Constants::IBCONSOLE_ALTERNATE, QVariant(false)).toBool();

    // Command builder. Default to the first in list, which should be the "Custom Command"
    commandBuilder(map.value(Constants::IBCONSOLE_COMMANDBUILDER, QVariant(m_commandBuildersList.front()->displayName())).toString());
    bool result = m_activeCommandBuilder->fromMap(map);

    return result && AbstractProcessStep::fromMap(map);
}

QVariantMap IBConsoleBuildStep::toMap() const
{
    QVariantMap map = AbstractProcessStep::toMap();

    map[IncrediBuild::Constants::INCREDIBUILD_BUILDSTEP_TYPE] = QVariant(IncrediBuild::Constants::IBCONSOLE_BUILDSTEP_ID);
    map[Constants::IBCONSOLE_NICE] = QVariant(m_nice);
    map[Constants::IBCONSOLE_KEEPJOBNUM] = QVariant(m_keepJobNum);
    map[Constants::IBCONSOLE_ALTERNATE] = QVariant(m_forceRemote);
    map[Constants::IBCONSOLE_FORCEREMOTE] = QVariant(m_forceRemote);
    map[Constants::IBCONSOLE_COMMANDBUILDER] = QVariant(commandBuilder()->displayName());
    m_activeCommandBuilder->toMap(&map);

    return map;
}

bool IBConsoleBuildStep::init()
{
    QStringList args;

    if (m_nice != 0)
        args.append(QString("--nice %0 ").arg(m_nice));

    if (m_alternate)
        args.append("--alternate");

    if (m_forceRemote)
        args.append("--force-remote");

    m_activeCommandBuilder->keepJobNum(m_keepJobNum);
    args.append(m_activeCommandBuilder->fullCommandFlag());

    Utils::CommandLine cmdLine("ib_console", args);
    ProcessParameters *procParams = processParameters();
    procParams->setCommandLine(cmdLine);
    procParams->setEnvironment(Utils::Environment::systemEnvironment());

    BuildConfiguration *buildConfig = buildConfiguration();
    if (buildConfig) {
        procParams->setWorkingDirectory(buildConfig->buildDirectory());
        procParams->setEnvironment(buildConfig->environment());

        Utils::MacroExpander *macroExpander = buildConfig->macroExpander();
        if (macroExpander)
            procParams->setMacroExpander(macroExpander);
    }

    return AbstractProcessStep::init();
}

BuildStepConfigWidget* IBConsoleBuildStep::createConfigWidget()
{
    return new IBConsoleStepConfigWidget(this);
}

void IBConsoleBuildStep::initCommandBuilders()
{
    if (m_commandBuildersList.empty()) {
        m_commandBuildersList.push_back(new CommandBuilder(this)); // "Custom Command"- needs to be first in the list.
        m_commandBuildersList.push_back(new MakeCommandBuilder(this));
        m_commandBuildersList.push_back(new CMakeCommandBuilder(this));
    }

    // Default to "Custom Command".
    if (!m_activeCommandBuilder)
        m_activeCommandBuilder = m_commandBuildersList.front();
}

const QStringList& IBConsoleBuildStep::supportedCommandBuilders()
{
    static QStringList list;
    if (list.empty()) {
        initCommandBuilders();
        for (CommandBuilder* p : m_commandBuildersList)
            list.push_back(p->displayName());
    }

    return list;
}

void IBConsoleBuildStep::commandBuilder(const QString &commandBuilder)
{
    for (CommandBuilder* p : m_commandBuildersList) {
        if (p->displayName().compare(commandBuilder) == 0) {
            m_activeCommandBuilder = p;
            break;
        }
    }
}

} // namespace Internal
} // namespace IncrediBuild
