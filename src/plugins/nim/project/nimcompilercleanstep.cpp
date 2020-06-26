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

#include "nimcompilercleanstep.h"
#include "nimbuildconfiguration.h"

#include "../nimconstants.h"

#include <projectexplorer/projectconfigurationaspects.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <utils/qtcassert.h>

#include <QDir>
#include <QDateTime>

using namespace ProjectExplorer;
using namespace Utils;

namespace Nim {

class NimCompilerCleanStep final : public BuildStep
{
    Q_DECLARE_TR_FUNCTIONS(Nim::NimCompilerCleanStep)

public:
    NimCompilerCleanStep(BuildStepList *parentList, Utils::Id id);

private:
    bool init() final;
    void doRun() final;
    void doCancel() final {}  // Can be left empty. The run() function hardly does anything.

    bool removeCacheDirectory();
    bool removeOutFilePath();

    Utils::FilePath m_buildDir;
};

NimCompilerCleanStep::NimCompilerCleanStep(BuildStepList *parentList, Utils::Id id)
    : BuildStep(parentList, id)
{
    setDefaultDisplayName(tr("Nim Clean Step"));
    setDisplayName(tr("Nim Clean Step"));

    auto workingDirectory = addAspect<BaseStringAspect>();
    workingDirectory->setLabelText(tr("Working directory:"));
    workingDirectory->setDisplayStyle(BaseStringAspect::LineEditDisplay);

    setSummaryUpdater([this, workingDirectory] {
        workingDirectory->setFilePath(buildDirectory());
        return displayName();
    });
}

bool NimCompilerCleanStep::init()
{
    FilePath buildDir = buildDirectory();
    bool result = buildDir.exists();
    if (result)
        m_buildDir = buildDir;
    return result;
}

void NimCompilerCleanStep::doRun()
{
    if (!m_buildDir.exists()) {
        emit addOutput(tr("Build directory \"%1\" does not exist.").arg(m_buildDir.toUserOutput()), OutputFormat::ErrorMessage);
        emit finished(false);
        return;
    }

    if (!removeCacheDirectory()) {
        emit addOutput(tr("Failed to delete the cache directory."), OutputFormat::ErrorMessage);
        emit finished(false);
        return;
    }

    if (!removeOutFilePath()) {
        emit addOutput(tr("Failed to delete the out file."), OutputFormat::ErrorMessage);
        emit finished(false);
        return;
    }

    emit addOutput(tr("Clean step completed successfully."), OutputFormat::NormalMessage);
    emit finished(true);
}

bool NimCompilerCleanStep::removeCacheDirectory()
{
    auto bc = qobject_cast<NimBuildConfiguration*>(buildConfiguration());
    QTC_ASSERT(bc, return false);
    if (!bc->cacheDirectory().exists())
        return true;
    QDir dir = QDir::fromNativeSeparators(bc->cacheDirectory().toString());
    const QString dirName = dir.dirName();
    if (!dir.cdUp())
        return false;
    const QString newName = QStringLiteral("%1.bkp.%2").arg(dirName, QString::number(QDateTime::currentMSecsSinceEpoch()));
    return dir.rename(dirName, newName);
}

bool NimCompilerCleanStep::removeOutFilePath()
{
    auto bc = qobject_cast<NimBuildConfiguration*>(buildConfiguration());
    QTC_ASSERT(bc, return false);
    if (!bc->outFilePath().exists())
        return true;
    return QFile(bc->outFilePath().toFileInfo().absoluteFilePath()).remove();
}

// NimCompilerCleanStepFactory

NimCompilerCleanStepFactory::NimCompilerCleanStepFactory()
{
    registerStep<NimCompilerCleanStep>(Constants::C_NIMCOMPILERCLEANSTEP_ID);
    setFlags(BuildStepInfo::Unclonable);
    setSupportedStepList(ProjectExplorer::Constants::BUILDSTEPS_CLEAN);
    setSupportedConfiguration(Constants::C_NIMBUILDCONFIGURATION_ID);
    setRepeatable(false);
    setDisplayName(NimCompilerCleanStep::tr("Nim Compiler Clean Step"));
}

} // Nim
