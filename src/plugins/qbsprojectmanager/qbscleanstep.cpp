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

#include "qbscleanstep.h"

#include "qbsbuildconfiguration.h"
#include "qbsproject.h"
#include "qbsprojectmanagerconstants.h"

#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/buildmanager.h>
#include <projectexplorer/target.h>
#include <utils/qtcassert.h>

using namespace ProjectExplorer;

namespace QbsProjectManager {
namespace Internal {

// --------------------------------------------------------------------
// QbsCleanStep:
// --------------------------------------------------------------------

QbsCleanStep::QbsCleanStep(ProjectExplorer::BuildStepList *bsl) :
    ProjectExplorer::BuildStep(bsl, Constants::QBS_CLEANSTEP_ID)
{
    setDisplayName(tr("Qbs Clean"));

    m_dryRunAspect = addAspect<BaseBoolAspect>();
    m_dryRunAspect->setSettingsKey("Qbs.DryRun");
    m_dryRunAspect->setLabel(tr("Dry run"));

    m_keepGoingAspect = addAspect<BaseBoolAspect>();
    m_keepGoingAspect->setSettingsKey("Qbs.DryKeepGoing");
    m_keepGoingAspect->setLabel(tr("Keep going"));

    m_effectiveCommandAspect = addAspect<BaseStringAspect>();
    m_effectiveCommandAspect->setDisplayStyle(BaseStringAspect::TextEditDisplay);
    m_effectiveCommandAspect->setLabelText(tr("Equivalent command line:"));

    updateState();

    connect(this, &ProjectExplorer::ProjectConfiguration::displayNameChanged,
            this, &QbsCleanStep::updateState);
    connect(m_dryRunAspect, &BaseBoolAspect::changed,
            this, &QbsCleanStep::updateState);
    connect(m_keepGoingAspect, &BaseBoolAspect::changed,
            this, &QbsCleanStep::updateState);
}

QbsCleanStep::~QbsCleanStep()
{
    doCancel();
    if (m_job) {
        m_job->deleteLater();
        m_job = nullptr;
    }
}

bool QbsCleanStep::init()
{
    if (project()->isParsing() || m_job)
        return false;

    auto bc = static_cast<QbsBuildConfiguration *>(buildConfiguration());

    if (!bc)
        return false;

    m_products = bc->products();
    return true;
}

void QbsCleanStep::doRun()
{
    auto pro = static_cast<QbsProject *>(project());
    qbs::CleanOptions options;
    options.setDryRun(m_dryRunAspect->value());
    options.setKeepGoing(m_keepGoingAspect->value());

    QString error;
    m_job = pro->clean(options, m_products, error);
    if (!m_job) {
        emit addOutput(error, OutputFormat::ErrorMessage);
        emit finished(false);
        return;
    }

    m_maxProgress = 0;

    connect(m_job, &qbs::AbstractJob::finished, this, &QbsCleanStep::cleaningDone);
    connect(m_job, &qbs::AbstractJob::taskStarted,
            this, &QbsCleanStep::handleTaskStarted);
    connect(m_job, &qbs::AbstractJob::taskProgress,
            this, &QbsCleanStep::handleProgress);
}

ProjectExplorer::BuildStepConfigWidget *QbsCleanStep::createConfigWidget()
{
    auto w = BuildStep::createConfigWidget();
    connect(this, &QbsCleanStep::stateChanged, w, [this, w] {
        w->setSummaryText(tr("<b>Qbs:</b> %1").arg(m_effectiveCommandAspect->value()));
    });
    return w;
}

void QbsCleanStep::doCancel()
{
    if (m_job)
        m_job->cancel();
}

void QbsCleanStep::cleaningDone(bool success)
{
    // Report errors:
    foreach (const qbs::ErrorItem &item, m_job->error().items()) {
        createTaskAndOutput(ProjectExplorer::Task::Error, item.description(),
                            item.codeLocation().filePath(), item.codeLocation().line());
    }

    emit finished(success);
    m_job->deleteLater();
    m_job = nullptr;
}

void QbsCleanStep::handleTaskStarted(const QString &desciption, int max)
{
    Q_UNUSED(desciption);
    m_maxProgress = max;
}

void QbsCleanStep::handleProgress(int value)
{
    if (m_maxProgress > 0)
        emit progress(value * 100 / m_maxProgress, m_description);
}

void QbsCleanStep::updateState()
{
    QString command = static_cast<QbsBuildConfiguration *>(buildConfiguration())
            ->equivalentCommandLine(this);
    m_effectiveCommandAspect->setValue(command);
    emit stateChanged();
}

void QbsCleanStep::createTaskAndOutput(ProjectExplorer::Task::TaskType type, const QString &message, const QString &file, int line)
{
    ProjectExplorer::Task task = ProjectExplorer::Task(type, message,
                                                       Utils::FileName::fromString(file), line,
                                                       ProjectExplorer::Constants::TASK_CATEGORY_COMPILE);
    emit addTask(task, 1);
    emit addOutput(message, OutputFormat::Stdout);
}

// --------------------------------------------------------------------
// QbsCleanStepFactory:
// --------------------------------------------------------------------

QbsCleanStepFactory::QbsCleanStepFactory()
{
    registerStep<QbsCleanStep>(Constants::QBS_CLEANSTEP_ID);
    setSupportedStepList(ProjectExplorer::Constants::BUILDSTEPS_CLEAN);
    setSupportedConfiguration(Constants::QBS_BC_ID);
    setDisplayName(QbsCleanStep::tr("Qbs Clean"));
}

} // namespace Internal
} // namespace QbsProjectManager
