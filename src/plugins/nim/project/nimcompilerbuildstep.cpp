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

#include "nimcompilerbuildstep.h"
#include "nimbuildconfiguration.h"
#include "nimbuildsystem.h"
#include "nimcompilerbuildstepconfigwidget.h"
#include "nimconstants.h"
#include "nimtoolchain.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/ioutputparser.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/processparameters.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <utils/qtcassert.h>

#include <QDir>
#include <QRegularExpression>

using namespace ProjectExplorer;
using namespace Utils;

namespace Nim {

class NimParser : public ProjectExplorer::OutputTaskParser
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

NimCompilerBuildStep::NimCompilerBuildStep(BuildStepList *parentList, Utils::Id id)
    : AbstractProcessStep(parentList, id)
{
    setDefaultDisplayName(tr(Constants::C_NIMCOMPILERBUILDSTEP_DISPLAY));
    setDisplayName(tr(Constants::C_NIMCOMPILERBUILDSTEP_DISPLAY));

    auto bc = qobject_cast<NimBuildConfiguration *>(buildConfiguration());
    connect(bc, &NimBuildConfiguration::buildDirectoryChanged,
            this, &NimCompilerBuildStep::updateProcessParameters);
    connect(bc, &BuildConfiguration::environmentChanged,
            this, &NimCompilerBuildStep::updateProcessParameters);
    connect(this, &NimCompilerBuildStep::outFilePathChanged,
            bc, &NimBuildConfiguration::outFilePathChanged);
    connect(project(), &ProjectExplorer::Project::fileListChanged,
            this, &NimCompilerBuildStep::updateTargetNimFile);
    updateProcessParameters();
}

void NimCompilerBuildStep::setupOutputFormatter(OutputFormatter *formatter)
{
    formatter->addLineParser(new NimParser);
    formatter->addLineParsers(target()->kit()->createOutputParsers());
    formatter->addSearchDir(processParameters()->effectiveWorkingDirectory());
    AbstractProcessStep::setupOutputFormatter(formatter);
}

BuildStepConfigWidget *NimCompilerBuildStep::createConfigWidget()
{
    return new NimCompilerBuildStepConfigWidget(this);
}

bool NimCompilerBuildStep::fromMap(const QVariantMap &map)
{
    AbstractProcessStep::fromMap(map);
    m_userCompilerOptions = map[Constants::C_NIMCOMPILERBUILDSTEP_USERCOMPILEROPTIONS].toString().split('|');
    m_defaultOptions = static_cast<DefaultBuildOptions>(map[Constants::C_NIMCOMPILERBUILDSTEP_DEFAULTBUILDOPTIONS].toInt());
    m_targetNimFile = FilePath::fromString(map[Constants::C_NIMCOMPILERBUILDSTEP_TARGETNIMFILE].toString());
    updateProcessParameters();
    return true;
}

QVariantMap NimCompilerBuildStep::toMap() const
{
    QVariantMap result = AbstractProcessStep::toMap();
    result[Constants::C_NIMCOMPILERBUILDSTEP_USERCOMPILEROPTIONS] = m_userCompilerOptions.join('|');
    result[Constants::C_NIMCOMPILERBUILDSTEP_DEFAULTBUILDOPTIONS] = m_defaultOptions;
    result[Constants::C_NIMCOMPILERBUILDSTEP_TARGETNIMFILE] = m_targetNimFile.toString();
    return result;
}

QStringList NimCompilerBuildStep::userCompilerOptions() const
{
    return m_userCompilerOptions;
}

void NimCompilerBuildStep::setUserCompilerOptions(const QStringList &options)
{
    m_userCompilerOptions = options;
    emit userCompilerOptionsChanged(options);
    updateProcessParameters();
}

NimCompilerBuildStep::DefaultBuildOptions NimCompilerBuildStep::defaultCompilerOptions() const
{
    return m_defaultOptions;
}

void NimCompilerBuildStep::setDefaultCompilerOptions(NimCompilerBuildStep::DefaultBuildOptions options)
{
    if (m_defaultOptions == options)
        return;
    m_defaultOptions = options;
    emit defaultCompilerOptionsChanged(options);
    updateProcessParameters();
}

FilePath NimCompilerBuildStep::targetNimFile() const
{
    return m_targetNimFile;
}

void NimCompilerBuildStep::setTargetNimFile(const FilePath &targetNimFile)
{
    if (targetNimFile == m_targetNimFile)
        return;
    m_targetNimFile = targetNimFile;
    emit targetNimFileChanged(targetNimFile);
    updateProcessParameters();
}

FilePath NimCompilerBuildStep::outFilePath() const
{
    return m_outFilePath;
}

void NimCompilerBuildStep::setOutFilePath(const FilePath &outFilePath)
{
    if (outFilePath == m_outFilePath)
        return;
    m_outFilePath = outFilePath;
    emit outFilePathChanged(outFilePath);
}

void NimCompilerBuildStep::updateProcessParameters()
{
    updateOutFilePath();
    updateCommand();
    updateWorkingDirectory();
    updateEnvironment();
    emit processParametersChanged();
}

void NimCompilerBuildStep::updateOutFilePath()
{
    const QString targetName = Utils::HostOsInfo::withExecutableSuffix(m_targetNimFile.toFileInfo().baseName());
    setOutFilePath(buildDirectory().pathAppended(targetName));
}

void NimCompilerBuildStep::updateWorkingDirectory()
{
    processParameters()->setWorkingDirectory(buildDirectory());
}

void NimCompilerBuildStep::updateCommand()
{
    auto bc = qobject_cast<NimBuildConfiguration *>(buildConfiguration());
    QTC_ASSERT(bc, return);

    QTC_ASSERT(target(), return);
    QTC_ASSERT(target()->kit(), return);
    Kit *kit = target()->kit();
    auto tc = dynamic_cast<NimToolChain*>(ToolChainKitAspect::toolChain(kit, Constants::C_NIMLANGUAGE_ID));
    QTC_ASSERT(tc, return);

    CommandLine cmd{tc->compilerCommand()};

    cmd.addArg("c");

    if (m_defaultOptions == Release)
        cmd.addArg("-d:release");
    else if (m_defaultOptions == Debug)
        cmd.addArgs({"--debugInfo", "--lineDir:on"});

    cmd.addArg("--out:" + m_outFilePath.toString());
    cmd.addArg("--nimCache:" + bc->cacheDirectory().toString());

    for (const QString &arg : m_userCompilerOptions) {
        if (!arg.isEmpty())
            cmd.addArg(arg);
    }

    if (!m_targetNimFile.isEmpty())
        cmd.addArg(m_targetNimFile.toString());

    processParameters()->setCommandLine(cmd);
}

void NimCompilerBuildStep::updateEnvironment()
{
    processParameters()->setEnvironment(buildEnvironment());
}

void NimCompilerBuildStep::updateTargetNimFile()
{
    if (!m_targetNimFile.isEmpty())
        return;
    const Utils::FilePaths nimFiles = project()->files([](const Node *n) {
        return Project::AllFiles(n) && n->path().endsWith(".nim");
    });
    if (!nimFiles.isEmpty())
        setTargetNimFile(nimFiles.at(0));
}

// NimCompilerBuildStepFactory

NimCompilerBuildStepFactory::NimCompilerBuildStepFactory()
{
    registerStep<NimCompilerBuildStep>(Constants::C_NIMCOMPILERBUILDSTEP_ID);
    setDisplayName(NimCompilerBuildStep::tr("Nim Compiler Build Step"));
    setSupportedStepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
    setSupportedConfiguration(Constants::C_NIMBUILDCONFIGURATION_ID);
    setRepeatable(false);
}

} // namespace Nim

#ifdef WITH_TESTS

#include "nimplugin.h"

#include <projectexplorer/outputparser_test.h>

#include <QTest>

namespace Nim {

void NimPlugin::testNimParser_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<OutputParserTester::Channel>("inputChannel");
    QTest::addColumn<QString>("childStdOutLines");
    QTest::addColumn<QString>("childStdErrLines");
    QTest::addColumn<Tasks >("tasks");
    QTest::addColumn<QString>("outputLines");

    // negative tests
    QTest::newRow("pass-through stdout")
            << "Sometext" << OutputParserTester::STDOUT
            << "Sometext\n" << QString()
            << Tasks()
            << QString();
    QTest::newRow("pass-through stderr")
            << "Sometext" << OutputParserTester::STDERR
            << QString() << "Sometext\n"
            << Tasks()
            << QString();

    // positive tests
    QTest::newRow("Parse error string")
            << QString::fromLatin1("main.nim(23, 1) Error: undeclared identifier: 'x'")
            << OutputParserTester::STDERR
            << QString() << QString()
            << Tasks({CompileTask(Task::Error,
                                  "Error: undeclared identifier: 'x'",
                                  FilePath::fromUserInput("main.nim"), 23)})
            << QString();

    QTest::newRow("Parse warning string")
            << QString::fromLatin1("lib/pure/parseopt.nim(56, 34) Warning: quoteIfContainsWhite is deprecated [Deprecated]")
            << OutputParserTester::STDERR
            << QString() << QString()
            << Tasks({CompileTask(Task::Warning,
                                  "Warning: quoteIfContainsWhite is deprecated [Deprecated]",
                                   FilePath::fromUserInput("lib/pure/parseopt.nim"), 56)})
            << QString();
}

void NimPlugin::testNimParser()
{
    OutputParserTester testbench;
    testbench.addLineParser(new NimParser);
    QFETCH(QString, input);
    QFETCH(OutputParserTester::Channel, inputChannel);
    QFETCH(Tasks, tasks);
    QFETCH(QString, childStdOutLines);
    QFETCH(QString, childStdErrLines);
    QFETCH(QString, outputLines);

    testbench.testParsing(input, inputChannel,
                          tasks, childStdOutLines, childStdErrLines,
                          outputLines);
}

}
#endif
