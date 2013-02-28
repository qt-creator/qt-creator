/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qbsstep.h"

#include "qbsbuildconfiguration.h"
#include "qbsparser.h"
#include "qbsproject.h"
#include "qbsprojectmanagerconstants.h"

#include "ui_qbsstepconfigwidget.h"

#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <utils/qtcassert.h>

#include <qbs.h>

#include <QTimer>

// --------------------------------------------------------------------
// Constants:
// --------------------------------------------------------------------

static const char QBS_DRY_RUN[] = "Qbs.DryRun";
static const char QBS_KEEP_GOING[] = "Qbs.DryKeepGoing";
static const char QBS_MAXJOBCOUNT[] = "Qbs.MaxJobs";

namespace QbsProjectManager {
namespace Internal {

// --------------------------------------------------------------------
// QbsStep:
// --------------------------------------------------------------------

QbsStep::QbsStep(ProjectExplorer::BuildStepList *bsl, Core::Id id) :
    ProjectExplorer::BuildStep(bsl, id),
    m_job(0)
{
    m_qbsBuildOptions.maxJobCount = QbsManager::preferences()->jobs();
}

QbsStep::QbsStep(ProjectExplorer::BuildStepList *bsl, const QbsStep *other) :
    ProjectExplorer::BuildStep(bsl, Core::Id(Constants::QBS_BUILDSTEP_ID)),
    m_qbsBuildOptions(other->m_qbsBuildOptions), m_job(0)
{ }

QbsBuildConfiguration *QbsStep::currentBuildConfiguration() const
{
    QbsBuildConfiguration *bc = static_cast<QbsBuildConfiguration *>(buildConfiguration());
    if (!bc)
        bc = static_cast<QbsBuildConfiguration *>(target()->activeBuildConfiguration());
    return bc;
}

QbsStep::~QbsStep()
{
    cancel();
    m_job->deleteLater();
    m_job = 0;
}

bool QbsStep::init()
{
    if (static_cast<QbsProject *>(project())->isParsing() || m_job)
        return false;

    if (!currentBuildConfiguration())
        return false;

    return true;
}

void QbsStep::run(QFutureInterface<bool> &fi)
{
    m_fi = &fi;

    m_job = createJob();

    if (!m_job) {
        jobDone(false);
        return;
    }

    m_progressBase = 0;

    connect(m_job, SIGNAL(finished(bool,qbs::AbstractJob*)), this, SLOT(jobDone(bool)));
    connect(m_job, SIGNAL(taskStarted(QString,int,qbs::AbstractJob*)),
            this, SLOT(handleTaskStarted(QString,int)));
    connect(m_job, SIGNAL(taskProgress(int,qbs::AbstractJob*)),
            this, SLOT(handleProgress(int)));
}

QFutureInterface<bool> *QbsStep::future() const
{
    return m_fi;
}

bool QbsStep::runInGuiThread() const
{
    return true;
}

void QbsStep::cancel()
{
    if (m_job)
        m_job->cancel();
}

bool QbsStep::dryRun() const
{
    return m_qbsBuildOptions.dryRun;
}

bool QbsStep::keepGoing() const
{
    return m_qbsBuildOptions.keepGoing;
}

int QbsStep::maxJobs() const
{
    return m_qbsBuildOptions.maxJobCount;
}

bool QbsStep::fromMap(const QVariantMap &map)
{
    if (!ProjectExplorer::BuildStep::fromMap(map))
        return false;

    m_qbsBuildOptions.dryRun = map.value(QLatin1String(QBS_DRY_RUN)).toBool();
    m_qbsBuildOptions.keepGoing = map.value(QLatin1String(QBS_KEEP_GOING)).toBool();
    m_qbsBuildOptions.maxJobCount = map.value(QLatin1String(QBS_MAXJOBCOUNT)).toInt();

    if (m_qbsBuildOptions.maxJobCount <= 0)
        m_qbsBuildOptions.maxJobCount = QbsManager::preferences()->jobs();

    return true;
}

QVariantMap QbsStep::toMap() const
{
    QVariantMap map = ProjectExplorer::BuildStep::toMap();
    map.insert(QLatin1String(QBS_DRY_RUN), m_qbsBuildOptions.dryRun);
    map.insert(QLatin1String(QBS_KEEP_GOING), m_qbsBuildOptions.keepGoing);
    map.insert(QLatin1String(QBS_MAXJOBCOUNT), m_qbsBuildOptions.maxJobCount);
    return map;
}

void QbsStep::jobDone(bool success)
{
    // Report errors:
    if (m_job) {
        foreach (const qbs::ErrorData &data, m_job->error().entries())
            createTaskAndOutput(ProjectExplorer::Task::Error, data.description(),
                                data.codeLocation().fileName, data.codeLocation().line);
        m_job->deleteLater();
        m_job = 0;
    }

    QTC_ASSERT(m_fi, return);
    m_fi->reportResult(success);
    m_fi = 0; // do not delete, it is not ours

    emit finished();
}

void QbsStep::handleTaskStarted(const QString &desciption, int max)
{
    Q_UNUSED(desciption);
    QTC_ASSERT(m_fi, return);

    m_progressBase = m_fi->progressValue();
    m_fi->setProgressRange(0, m_progressBase + max);
}

void QbsStep::handleProgress(int value)
{
    QTC_ASSERT(m_fi, return);
    m_fi->setProgressValue(m_progressBase + value);
}

void QbsStep::createTaskAndOutput(ProjectExplorer::Task::TaskType type, const QString &message,
                                       const QString &file, int line)
{
    emit addTask(ProjectExplorer::Task(type, message,
                                       Utils::FileName::fromString(file), line,
                                       ProjectExplorer::Constants::TASK_CATEGORY_COMPILE));
    emit addOutput(message, NormalOutput);
}

void QbsStep::setDryRun(bool dr)
{
    if (m_qbsBuildOptions.dryRun == dr)
        return;
    m_qbsBuildOptions.dryRun = dr;
    emit qbsBuildOptionsChanged();
}

void QbsStep::setKeepGoing(bool kg)
{
    if (m_qbsBuildOptions.keepGoing == kg)
        return;
    m_qbsBuildOptions.keepGoing = kg;
    emit qbsBuildOptionsChanged();
}

void QbsStep::setMaxJobs(int jobcount)
{
    if (m_qbsBuildOptions.maxJobCount == jobcount)
        return;
    m_qbsBuildOptions.maxJobCount = jobcount;
    emit qbsBuildOptionsChanged();
}

// --------------------------------------------------------------------
// QbsStepConfigWidget:
// --------------------------------------------------------------------

QbsStepConfigWidget::QbsStepConfigWidget(QbsStep *step) :
    m_step(step)
{
    connect(m_step, SIGNAL(displayNameChanged()), this, SLOT(updateState()));
    connect(m_step, SIGNAL(qbsBuildOptionsChanged()), this, SLOT(updateState()));

    setContentsMargins(0, 0, 0, 0);

    m_ui = new Ui::QbsStepConfigWidget;
    m_ui->setupUi(this);

    connect(m_ui->dryRunCheckBox, SIGNAL(toggled(bool)), this, SLOT(changeDryRun(bool)));
    connect(m_ui->keepGoingCheckBox, SIGNAL(toggled(bool)), this, SLOT(changeKeepGoing(bool)));
    connect(m_ui->jobSpinBox, SIGNAL(valueChanged(int)), this, SLOT(changeJobCount(int)));

    QTimer::singleShot(0, this, SLOT(updateState()));
}

QString QbsStepConfigWidget::summaryText() const
{
    return m_summary;
}

QString QbsStepConfigWidget::displayName() const
{
    return m_step->displayName();
}

void QbsStepConfigWidget::updateState()
{
    m_ui->dryRunCheckBox->setChecked(m_step->dryRun());
    m_ui->keepGoingCheckBox->setChecked(m_step->keepGoing());
    m_ui->jobSpinBox->setValue(m_step->maxJobs());

    QString command = QLatin1String("qbs");

    const QString qbsCmd = qbsCommand();
    if (!qbsCmd.isEmpty()) {
        command += QLatin1String(" ");
        command += qbsCmd;
    }

    const QString buildDir = m_step->target()->activeBuildConfiguration()->buildDirectory();
    const QString sourceDir = m_step->project()->projectDirectory();
    if (buildDir != sourceDir)
        command += QString::fromLatin1(" -f \"%1\"").arg(QDir(buildDir).relativeFilePath(sourceDir));

    if (m_step->dryRun())
        command += QLatin1String(" --dry-run");
    if (m_step->keepGoing())
        command += QLatin1String(" --keep-going");
    if (m_step->maxJobs() != QbsManager::preferences()->jobs())
        command += QString::fromLatin1(" --jobs %1").arg(m_step->maxJobs());


    const QString args = additionalQbsArguments();
    if (!args.isEmpty()) {
        command += QLatin1String(" ");
        command += args;
    }

    QString summary = tr("<b>Qbs:</b> %1").arg(command);
    if (m_summary !=  summary) {
        m_summary = summary;
        emit updateSummary();
    }
}

void QbsStepConfigWidget::changeDryRun(bool dr)
{
    m_step->setDryRun(dr);
}

void QbsStepConfigWidget::changeKeepGoing(bool kg)
{
    m_step->setKeepGoing(kg);
}

void QbsStepConfigWidget::changeJobCount(int count)
{
    m_step->setMaxJobs(count);
}

void QbsStepConfigWidget::addWidget(QWidget *widget)
{
    if (widget)
        static_cast<QVBoxLayout *>(layout())->insertWidget(0, widget);
    updateState();
}

void QbsStepConfigWidget::setJobCountUiVisible(bool show)
{
    m_ui->jobSpinBox->setVisible(show);
    m_ui->jobLabel->setVisible(show);
}

} // namespace Internal
} // namespace QbsProjectManager
