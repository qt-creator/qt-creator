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
#include "nimconstants.h"
#include "nimtoolchain.h"

#include <projectexplorer/processparameters.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/ioutputparser.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QApplication>
#include <QComboBox>
#include <QDir>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QRegularExpression>
#include <QTextEdit>

using namespace ProjectExplorer;
using namespace Utils;

namespace Nim {

class NimCompilerBuildStepConfigWidget : public ProjectExplorer::BuildStepConfigWidget
{
    Q_DECLARE_TR_FUNCTIONS(Nim::NimCompilerBuildStep)

public:
    explicit NimCompilerBuildStepConfigWidget(NimCompilerBuildStep *buildStep);

private:
    void updateUi();
    void updateCommandLineText();
    void updateTargetComboBox();
    void updateAdditionalArgumentsLineEdit();
    void updateDefaultArgumentsComboBox();

    void onAdditionalArgumentsTextEdited(const QString &text);
    void onTargetChanged(int index);
    void onDefaultArgumentsComboBoxIndexChanged(int index);

    NimCompilerBuildStep *m_buildStep;
    QTextEdit *m_commandTextEdit;
    QComboBox *m_defaultArgumentsComboBox;
    QComboBox *m_targetComboBox;
    QLineEdit *m_additionalArgumentsLineEdit;
};

// NimParser

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

NimCompilerBuildStepConfigWidget::NimCompilerBuildStepConfigWidget(NimCompilerBuildStep *buildStep)
    : BuildStepConfigWidget(buildStep)
    , m_buildStep(buildStep)
{
    setDisplayName(tr(Constants::C_NIMCOMPILERBUILDSTEPWIDGET_DISPLAY));
    setSummaryText(tr(Constants::C_NIMCOMPILERBUILDSTEPWIDGET_SUMMARY));

    m_targetComboBox = new QComboBox(this);

    m_additionalArgumentsLineEdit = new QLineEdit(this);

    m_commandTextEdit = new QTextEdit(this);
    m_commandTextEdit->setEnabled(false);
    m_commandTextEdit->setMinimumSize(QSize(0, 0));

    m_defaultArgumentsComboBox = new QComboBox(this);
    m_defaultArgumentsComboBox->addItem(tr("None"));
    m_defaultArgumentsComboBox->addItem(tr("Debug"));
    m_defaultArgumentsComboBox->addItem(tr("Release"));

    auto formLayout = new QFormLayout(this);
    formLayout->addRow(tr("Target:"), m_targetComboBox);
    formLayout->addRow(tr("Default arguments:"), m_defaultArgumentsComboBox);
    formLayout->addRow(tr("Extra arguments:"),  m_additionalArgumentsLineEdit);
    formLayout->addRow(tr("Command:"), m_commandTextEdit);

    // Connect the project signals
    connect(m_buildStep->project(),
            &Project::fileListChanged,
            this,
            &NimCompilerBuildStepConfigWidget::updateUi);

    // Connect build step signals
    connect(m_buildStep, &NimCompilerBuildStep::processParametersChanged,
            this, &NimCompilerBuildStepConfigWidget::updateUi);

    // Connect UI signals
    connect(m_targetComboBox, QOverload<int>::of(&QComboBox::activated),
            this, &NimCompilerBuildStepConfigWidget::onTargetChanged);
    connect(m_additionalArgumentsLineEdit, &QLineEdit::textEdited,
            this, &NimCompilerBuildStepConfigWidget::onAdditionalArgumentsTextEdited);
    connect(m_defaultArgumentsComboBox, QOverload<int>::of(&QComboBox::activated),
            this, &NimCompilerBuildStepConfigWidget::onDefaultArgumentsComboBoxIndexChanged);

    updateUi();
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

void NimCompilerBuildStepConfigWidget::onTargetChanged(int index)
{
    Q_UNUSED(index)
    auto data = m_targetComboBox->currentData();
    FilePath path = FilePath::fromString(data.toString());
    m_buildStep->setTargetNimFile(path);
}

void NimCompilerBuildStepConfigWidget::onDefaultArgumentsComboBoxIndexChanged(int index)
{
    auto options = static_cast<NimCompilerBuildStep::DefaultBuildOptions>(index);
    m_buildStep->setDefaultCompilerOptions(options);
}

void NimCompilerBuildStepConfigWidget::updateUi()
{
    updateCommandLineText();
    updateTargetComboBox();
    updateAdditionalArgumentsLineEdit();
    updateDefaultArgumentsComboBox();
}

void NimCompilerBuildStepConfigWidget::onAdditionalArgumentsTextEdited(const QString &text)
{
    m_buildStep->setUserCompilerOptions(text.split(QChar::Space));
}

void NimCompilerBuildStepConfigWidget::updateCommandLineText()
{
    ProcessParameters *parameters = m_buildStep->processParameters();

    const CommandLine cmd = parameters->command();
    const QStringList parts = QtcProcess::splitArgs(cmd.toUserOutput());

    m_commandTextEdit->setText(parts.join(QChar::LineFeed));
}

void NimCompilerBuildStepConfigWidget::updateTargetComboBox()
{
    QTC_ASSERT(m_buildStep, return );

    // Re enter the files
    m_targetComboBox->clear();

    const FilePaths nimFiles = m_buildStep->project()->files([](const Node *n) {
        return Project::AllFiles(n) && n->path().endsWith(".nim");
    });

    for (const FilePath &file : nimFiles)
        m_targetComboBox->addItem(file.fileName(), file.toString());

    const int index = m_targetComboBox->findData(m_buildStep->targetNimFile().toString());
    m_targetComboBox->setCurrentIndex(index);
}

void NimCompilerBuildStepConfigWidget::updateAdditionalArgumentsLineEdit()
{
    const QString text = m_buildStep->userCompilerOptions().join(QChar::Space);
    m_additionalArgumentsLineEdit->setText(text);
}

void NimCompilerBuildStepConfigWidget::updateDefaultArgumentsComboBox()
{
    const int index = m_buildStep->defaultCompilerOptions();
    m_defaultArgumentsComboBox->setCurrentIndex(index);
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
