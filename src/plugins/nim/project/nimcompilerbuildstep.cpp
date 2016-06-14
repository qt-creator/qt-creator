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
#include "nimcompilerbuildstepconfigwidget.h"

#include "../nimconstants.h"

#include <projectexplorer/buildconfiguration.h>
#include <utils/qtcassert.h>

#include <QDir>

using namespace ProjectExplorer;
using namespace Utils;

namespace Nim {

NimCompilerBuildStep::NimCompilerBuildStep(BuildStepList *parentList)
    : AbstractProcessStep(parentList, Constants::C_NIMCOMPILERBUILDSTEP_ID)
{
    setDefaultDisplayName(tr(Constants::C_NIMCOMPILERBUILDSTEP_DISPLAY));
    setDisplayName(tr(Constants::C_NIMCOMPILERBUILDSTEP_DISPLAY));

    auto bc = qobject_cast<NimBuildConfiguration *>(buildConfiguration());
    connect(bc, &NimBuildConfiguration::buildDirectoryChanged,
            this, &NimCompilerBuildStep::updateProcessParameters);
    connect(this, &NimCompilerBuildStep::outFilePathChanged,
            bc, &NimBuildConfiguration::outFilePathChanged);
    updateProcessParameters();
}

BuildStepConfigWidget *NimCompilerBuildStep::createConfigWidget()
{
    return new NimCompilerBuildStepConfigWidget(this);
}

bool NimCompilerBuildStep::fromMap(const QVariantMap &map)
{
    AbstractProcessStep::fromMap(map);
    m_userCompilerOptions = map[Constants::C_NIMCOMPILERBUILDSTEP_USERCOMPILEROPTIONS].toString().split(QLatin1Char('|'));
    m_defaultOptions = static_cast<DefaultBuildOptions>(map[Constants::C_NIMCOMPILERBUILDSTEP_DEFAULTBUILDOPTIONS].toInt());
    m_targetNimFile = FileName::fromString(map[Constants::C_NIMCOMPILERBUILDSTEP_TARGETNIMFILE].toString());
    updateProcessParameters();
    return true;
}

QVariantMap NimCompilerBuildStep::toMap() const
{
    QVariantMap result = AbstractProcessStep::toMap();
    result[Constants::C_NIMCOMPILERBUILDSTEP_USERCOMPILEROPTIONS] = m_userCompilerOptions.join(QLatin1Char('|'));
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

FileName NimCompilerBuildStep::targetNimFile() const
{
    return m_targetNimFile;
}

void NimCompilerBuildStep::setTargetNimFile(const FileName &targetNimFile)
{
    if (targetNimFile == m_targetNimFile)
        return;
    m_targetNimFile = targetNimFile;
    emit targetNimFileChanged(targetNimFile);
    updateProcessParameters();
}

FileName NimCompilerBuildStep::outFilePath() const
{
    return m_outFilePath;
}

void NimCompilerBuildStep::setOutFilePath(const FileName &outFilePath)
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
    updateArguments();
    updateWorkingDirectory();
    updateEnvironment();
    emit processParametersChanged();
}

void NimCompilerBuildStep::updateOutFilePath()
{
    auto bc = qobject_cast<NimBuildConfiguration *>(buildConfiguration());
    QTC_ASSERT(bc, return);
    const QString targetName = Utils::HostOsInfo::withExecutableSuffix(m_targetNimFile.toFileInfo().baseName());
    FileName outFilePath = bc->buildDirectory().appendPath(targetName);
    setOutFilePath(outFilePath);
}

void NimCompilerBuildStep::updateCommand()
{
    processParameters()->setCommand(QStringLiteral("nim"));
}

void NimCompilerBuildStep::updateWorkingDirectory()
{
    auto bc = qobject_cast<NimBuildConfiguration *>(buildConfiguration());
    QTC_ASSERT(bc, return);
    processParameters()->setWorkingDirectory(bc->buildDirectory().toString());
}

void NimCompilerBuildStep::updateArguments()
{
    auto bc = qobject_cast<NimBuildConfiguration *>(buildConfiguration());
    QTC_ASSERT(bc, return);

    QStringList arguments;
    arguments << QStringLiteral("c");

    switch (m_defaultOptions) {
    case Release:
        arguments << QStringLiteral("-d:release");
        break;
    case Debug:
        arguments << QStringLiteral("--debugInfo")
                  << QStringLiteral("--lineDir:on");
        break;
    default:
        break;
    }

    arguments << QStringLiteral("--out:%1").arg(m_outFilePath.toString());
    arguments << QStringLiteral("--nimCache:%1").arg(bc->cacheDirectory().toString());

    arguments << m_userCompilerOptions;
    arguments << m_targetNimFile.toString();

    // Remove empty args
    auto predicate = [](const QString &str) { return str.isEmpty(); };
    auto it = std::remove_if(arguments.begin(), arguments.end(), predicate);
    arguments.erase(it, arguments.end());

    processParameters()->setArguments(arguments.join(QChar::Space));
}

void NimCompilerBuildStep::updateEnvironment()
{
    auto bc = qobject_cast<NimBuildConfiguration *>(buildConfiguration());
    QTC_ASSERT(bc, return);
    processParameters()->setEnvironment(bc->environment());
}

}

