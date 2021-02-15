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
#include "nimoutputtaskparser.h"
#include "nimtoolchain.h"

#include <projectexplorer/processparameters.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/ioutputparser.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QComboBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QRegularExpression>
#include <QTextEdit>

using namespace ProjectExplorer;
using namespace Utils;

namespace Nim {

NimCompilerBuildStep::NimCompilerBuildStep(BuildStepList *parentList, Utils::Id id)
    : AbstractProcessStep(parentList, id)
{
    setCommandLineProvider([this] { return commandLine(); });

    connect(project(), &ProjectExplorer::Project::fileListChanged,
            this, &NimCompilerBuildStep::updateTargetNimFile);
}

void NimCompilerBuildStep::setupOutputFormatter(OutputFormatter *formatter)
{
    formatter->addLineParser(new NimParser);
    formatter->addLineParsers(kit()->createOutputParsers());
    formatter->addSearchDir(buildDirectory());
    AbstractProcessStep::setupOutputFormatter(formatter);
}

QWidget *NimCompilerBuildStep::createConfigWidget()
{
    auto widget = new QWidget;

    setDisplayName(tr("Nim build step"));
    setSummaryText(tr("Nim build step"));

    auto targetComboBox = new QComboBox(widget);

    auto additionalArgumentsLineEdit = new QLineEdit(widget);

    auto commandTextEdit = new QTextEdit(widget);
    commandTextEdit->setEnabled(false);
    commandTextEdit->setMinimumSize(QSize(0, 0));

    auto defaultArgumentsComboBox = new QComboBox(widget);
    defaultArgumentsComboBox->addItem(tr("None"));
    defaultArgumentsComboBox->addItem(tr("Debug"));
    defaultArgumentsComboBox->addItem(tr("Release"));

    auto formLayout = new QFormLayout(widget);
    formLayout->addRow(tr("Target:"), targetComboBox);
    formLayout->addRow(tr("Default arguments:"), defaultArgumentsComboBox);
    formLayout->addRow(tr("Extra arguments:"),  additionalArgumentsLineEdit);
    formLayout->addRow(tr("Command:"), commandTextEdit);

    auto updateUi = [=] {
        const CommandLine cmd = commandLine();
        const QStringList parts = QtcProcess::splitArgs(cmd.toUserOutput());

        commandTextEdit->setText(parts.join(QChar::LineFeed));

        // Re enter the files
        targetComboBox->clear();
        const FilePaths files = project()->files(Project::AllFiles);
        for (const FilePath &file : files) {
            if (file.endsWith(".nim"))
                targetComboBox->addItem(file.fileName(), file.toString());
        }

        const int index = targetComboBox->findData(m_targetNimFile.toString());
        targetComboBox->setCurrentIndex(index);

        const QString text = m_userCompilerOptions.join(QChar::Space);
        additionalArgumentsLineEdit->setText(text);

        defaultArgumentsComboBox->setCurrentIndex(m_defaultOptions);
    };

    connect(project(), &Project::fileListChanged, this, updateUi);

    connect(targetComboBox, QOverload<int>::of(&QComboBox::activated),
            this, [this, targetComboBox, updateUi] {
        const QVariant data = targetComboBox->currentData();
        m_targetNimFile = FilePath::fromString(data.toString());
        updateUi();
    });

    connect(additionalArgumentsLineEdit, &QLineEdit::textEdited,
            this, [this, updateUi](const QString &text) {
        m_userCompilerOptions = text.split(QChar::Space);
        updateUi();
    });

    connect(defaultArgumentsComboBox, QOverload<int>::of(&QComboBox::activated),
            this, [this, updateUi](int index) {
        m_defaultOptions = static_cast<DefaultBuildOptions>(index);
        updateUi();
    });

    updateUi();

    return widget;
}

bool NimCompilerBuildStep::fromMap(const QVariantMap &map)
{
    AbstractProcessStep::fromMap(map);
    m_userCompilerOptions = map[Constants::C_NIMCOMPILERBUILDSTEP_USERCOMPILEROPTIONS].toString().split('|');
    m_defaultOptions = static_cast<DefaultBuildOptions>(map[Constants::C_NIMCOMPILERBUILDSTEP_DEFAULTBUILDOPTIONS].toInt());
    m_targetNimFile = FilePath::fromString(map[Constants::C_NIMCOMPILERBUILDSTEP_TARGETNIMFILE].toString());
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

void NimCompilerBuildStep::setBuildType(BuildConfiguration::BuildType buildType)
{
    switch (buildType) {
    case BuildConfiguration::Release:
        m_defaultOptions = DefaultBuildOptions::Release;
        break;
    case BuildConfiguration::Debug:
        m_defaultOptions = DefaultBuildOptions::Debug;
        break;
    default:
        m_defaultOptions = DefaultBuildOptions::Empty;
        break;
    }

    updateTargetNimFile();
}

CommandLine NimCompilerBuildStep::commandLine()
{
    auto bc = qobject_cast<NimBuildConfiguration *>(buildConfiguration());
    QTC_ASSERT(bc, return {});

    auto tc = ToolChainKitAspect::toolChain(kit(), Constants::C_NIMLANGUAGE_ID);
    QTC_ASSERT(tc, return {});

    CommandLine cmd{tc->compilerCommand()};

    cmd.addArg("c");

    if (m_defaultOptions == Release)
        cmd.addArg("-d:release");
    else if (m_defaultOptions == Debug)
        cmd.addArgs({"--debugInfo", "--lineDir:on"});

    cmd.addArg("--out:" + outFilePath().toString());
    cmd.addArg("--nimCache:" + bc->cacheDirectory().toString());

    for (const QString &arg : qAsConst(m_userCompilerOptions)) {
        if (!arg.isEmpty())
            cmd.addArg(arg);
    }

    if (!m_targetNimFile.isEmpty())
        cmd.addArg(m_targetNimFile.toString());

    return cmd;
}

FilePath NimCompilerBuildStep::outFilePath() const
{
    const QString targetName = m_targetNimFile.toFileInfo().baseName();
    return buildDirectory().pathAppended(HostOsInfo::withExecutableSuffix(targetName));
}

void NimCompilerBuildStep::updateTargetNimFile()
{
    if (!m_targetNimFile.isEmpty())
        return;

    const FilePaths files = project()->files(Project::AllFiles);
    for (const FilePath &file : files) {
        if (file.endsWith(".nim")) {
            m_targetNimFile = file;
            break;
        }
    }
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
