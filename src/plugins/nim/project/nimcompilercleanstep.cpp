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
#include "nimcompilercleanstepconfigwidget.h"

#include "../nimconstants.h"

#include <utils/qtcassert.h>

#include <QDir>
#include <QDateTime>

using namespace ProjectExplorer;
using namespace Utils;

namespace Nim {

NimCompilerCleanStep::NimCompilerCleanStep(BuildStepList *parentList)
    : BuildStep(parentList, Constants::C_NIMCOMPILERCLEANSTEP_ID)
{
    setDefaultDisplayName(tr("Nim Clean Step"));
    setDisplayName(tr("Nim Clean Step"));
}

BuildStepConfigWidget *NimCompilerCleanStep::createConfigWidget()
{
    return new NimCompilerCleanStepConfigWidget(this);
}

bool NimCompilerCleanStep::init(QList<const BuildStep *> &)
{
    FileName buildDir = buildConfiguration()->buildDirectory();
    bool result = buildDir.exists();
    if (result)
        m_buildDir = buildDir;
    return result;
}

void NimCompilerCleanStep::run(QFutureInterface<bool> &fi)
{
    if (!m_buildDir.exists()) {
        emit addOutput(tr("Build directory \"%1\" does not exist.").arg(m_buildDir.toUserOutput()), BuildStep::OutputFormat::ErrorMessage);
        reportRunResult(fi, false);
        return;
    }

    if (!removeCacheDirectory()) {
        emit addOutput(tr("Failed to delete the cache directory."), BuildStep::OutputFormat::ErrorMessage);
        reportRunResult(fi, false);
        return;
    }

    if (!removeOutFilePath()) {
        emit addOutput(tr("Failed to delete the out file."), BuildStep::OutputFormat::ErrorMessage);
        reportRunResult(fi, false);
        return;
    }

    emit addOutput(tr("Clean step completed successfully."), BuildStep::OutputFormat::NormalMessage);
    reportRunResult(fi, true);
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

}

