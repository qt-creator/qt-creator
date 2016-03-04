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

#include "ui_qbscleanstepconfigwidget.h"

#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/buildmanager.h>
#include <projectexplorer/target.h>
#include <utils/qtcassert.h>

static const char QBS_DRY_RUN[] = "Qbs.DryRun";
static const char QBS_KEEP_GOING[] = "Qbs.DryKeepGoing";

// --------------------------------------------------------------------
// Constants:
// --------------------------------------------------------------------

namespace QbsProjectManager {
namespace Internal {

// --------------------------------------------------------------------
// QbsCleanStep:
// --------------------------------------------------------------------

QbsCleanStep::QbsCleanStep(ProjectExplorer::BuildStepList *bsl) :
    ProjectExplorer::BuildStep(bsl, Core::Id(Constants::QBS_CLEANSTEP_ID)),
    m_job(0), m_showCompilerOutput(true), m_parser(0)
{
    setDisplayName(tr("Qbs Clean"));
}

QbsCleanStep::QbsCleanStep(ProjectExplorer::BuildStepList *bsl, const QbsCleanStep *other) :
    ProjectExplorer::BuildStep(bsl, Core::Id(Constants::QBS_CLEANSTEP_ID)),
    m_qbsCleanOptions(other->m_qbsCleanOptions), m_job(0),
    m_showCompilerOutput(other->m_showCompilerOutput), m_parser(0)
{ }

QbsCleanStep::~QbsCleanStep()
{
    cancel();
    if (m_job) {
        m_job->deleteLater();
        m_job = 0;
    }
}

bool QbsCleanStep::init(QList<const BuildStep *> &earlierSteps)
{
    Q_UNUSED(earlierSteps);
    if (static_cast<QbsProject *>(project())->isParsing() || m_job)
        return false;

    QbsBuildConfiguration *bc = static_cast<QbsBuildConfiguration *>(buildConfiguration());
    if (!bc)
        bc = static_cast<QbsBuildConfiguration *>(target()->activeBuildConfiguration());

    if (!bc)
        return false;

    return true;
}

void QbsCleanStep::run(QFutureInterface<bool> &fi)
{
    m_fi = &fi;

    QbsProject *pro = static_cast<QbsProject *>(project());
    qbs::CleanOptions options(m_qbsCleanOptions);

    m_job = pro->clean(options);

    if (!m_job) {
        m_fi->reportResult(false);
        emit finished();
        return;
    }

    m_progressBase = 0;

    connect(m_job, SIGNAL(finished(bool,qbs::AbstractJob*)), this, SLOT(cleaningDone(bool)));
    connect(m_job, SIGNAL(taskStarted(QString,int,qbs::AbstractJob*)),
            this, SLOT(handleTaskStarted(QString,int)));
    connect(m_job, SIGNAL(taskProgress(int,qbs::AbstractJob*)),
            this, SLOT(handleProgress(int)));
}

ProjectExplorer::BuildStepConfigWidget *QbsCleanStep::createConfigWidget()
{
    return new QbsCleanStepConfigWidget(this);
}

bool QbsCleanStep::runInGuiThread() const
{
    return true;
}

void QbsCleanStep::cancel()
{
    if (m_job)
        m_job->cancel();
}

bool QbsCleanStep::dryRun() const
{
    return m_qbsCleanOptions.dryRun();
}

bool QbsCleanStep::keepGoing() const
{
    return m_qbsCleanOptions.keepGoing();
}

int QbsCleanStep::maxJobs() const
{
    return 1;
}


bool QbsCleanStep::fromMap(const QVariantMap &map)
{
    if (!ProjectExplorer::BuildStep::fromMap(map))
        return false;

    m_qbsCleanOptions.setDryRun(map.value(QLatin1String(QBS_DRY_RUN)).toBool());
    m_qbsCleanOptions.setKeepGoing(map.value(QLatin1String(QBS_KEEP_GOING)).toBool());

    return true;
}

QVariantMap QbsCleanStep::toMap() const
{
    QVariantMap map = ProjectExplorer::BuildStep::toMap();
    map.insert(QLatin1String(QBS_DRY_RUN), m_qbsCleanOptions.dryRun());
    map.insert(QLatin1String(QBS_KEEP_GOING), m_qbsCleanOptions.keepGoing());

    return map;
}

void QbsCleanStep::cleaningDone(bool success)
{
    // Report errors:
    foreach (const qbs::ErrorItem &item, m_job->error().items()) {
        createTaskAndOutput(ProjectExplorer::Task::Error, item.description(),
                            item.codeLocation().filePath(), item.codeLocation().line());
    }

    QTC_ASSERT(m_fi, return);
    m_fi->reportResult(success);
    m_fi = 0; // do not delete, it is not ours
    m_job->deleteLater();
    m_job = 0;

    emit finished();
}

void QbsCleanStep::handleTaskStarted(const QString &desciption, int max)
{
    Q_UNUSED(desciption);
    QTC_ASSERT(m_fi, return);
    m_progressBase = m_fi->progressValue();
    m_fi->setProgressRange(0, m_progressBase + max);
}

void QbsCleanStep::handleProgress(int value)
{
    QTC_ASSERT(m_fi, return);
    m_fi->setProgressValue(m_progressBase + value);
}

void QbsCleanStep::createTaskAndOutput(ProjectExplorer::Task::TaskType type, const QString &message, const QString &file, int line)
{
    ProjectExplorer::Task task = ProjectExplorer::Task(type, message,
                                                       Utils::FileName::fromString(file), line,
                                                       ProjectExplorer::Constants::TASK_CATEGORY_COMPILE);
    emit addTask(task, 1);
    emit addOutput(message, NormalOutput);
}

void QbsCleanStep::setDryRun(bool dr)
{
    if (m_qbsCleanOptions.dryRun() == dr)
        return;
    m_qbsCleanOptions.setDryRun(dr);
    emit changed();
}

void QbsCleanStep::setKeepGoing(bool kg)
{
    if (m_qbsCleanOptions.keepGoing() == kg)
        return;
    m_qbsCleanOptions.setKeepGoing(kg);
    emit changed();
}

void QbsCleanStep::setMaxJobs(int jobcount)
{
    Q_UNUSED(jobcount); // TODO: Remove all job count-related stuff.
    emit changed();
}


// --------------------------------------------------------------------
// QbsCleanStepConfigWidget:
// --------------------------------------------------------------------

QbsCleanStepConfigWidget::QbsCleanStepConfigWidget(QbsCleanStep *step) :
    m_step(step)
{
    connect(m_step, SIGNAL(displayNameChanged()), this, SLOT(updateState()));
    connect(m_step, SIGNAL(changed()), this, SLOT(updateState()));

    setContentsMargins(0, 0, 0, 0);

    m_ui = new Ui::QbsCleanStepConfigWidget;
    m_ui->setupUi(this);

    connect(m_ui->dryRunCheckBox, SIGNAL(toggled(bool)), this, SLOT(changeDryRun(bool)));
    connect(m_ui->keepGoingCheckBox, SIGNAL(toggled(bool)), this, SLOT(changeKeepGoing(bool)));

    updateState();
}

QbsCleanStepConfigWidget::~QbsCleanStepConfigWidget()
{
    delete m_ui;
}

QString QbsCleanStepConfigWidget::summaryText() const
{
    return m_summary;
}

QString QbsCleanStepConfigWidget::displayName() const
{
    return m_step->displayName();
}

void QbsCleanStepConfigWidget::updateState()
{
    m_ui->dryRunCheckBox->setChecked(m_step->dryRun());
    m_ui->keepGoingCheckBox->setChecked(m_step->keepGoing());

    QString command = QbsBuildConfiguration::equivalentCommandLine(m_step);
    m_ui->commandLineTextEdit->setPlainText(command);

    QString summary = tr("<b>Qbs:</b> %1").arg(command);
    if (m_summary !=  summary) {
        m_summary = summary;
        emit updateSummary();
    }
}

void QbsCleanStepConfigWidget::changeDryRun(bool dr)
{
    m_step->setDryRun(dr);
}

void QbsCleanStepConfigWidget::changeKeepGoing(bool kg)
{
    m_step->setKeepGoing(kg);
}

void QbsCleanStepConfigWidget::changeJobCount(int count)
{
    m_step->setMaxJobs(count);
}

// --------------------------------------------------------------------
// QbsCleanStepFactory:
// --------------------------------------------------------------------

QbsCleanStepFactory::QbsCleanStepFactory(QObject *parent) :
    ProjectExplorer::IBuildStepFactory(parent)
{ }

QList<Core::Id> QbsCleanStepFactory::availableCreationIds(ProjectExplorer::BuildStepList *parent) const
{
    if (parent->id() == ProjectExplorer::Constants::BUILDSTEPS_CLEAN
            && qobject_cast<QbsBuildConfiguration *>(parent->parent()))
        return QList<Core::Id>() << Core::Id(Constants::QBS_CLEANSTEP_ID);
    return QList<Core::Id>();
}

QString QbsCleanStepFactory::displayNameForId(Core::Id id) const
{
    if (id == Core::Id(Constants::QBS_CLEANSTEP_ID))
        return tr("Qbs Clean");
    return QString();
}

bool QbsCleanStepFactory::canCreate(ProjectExplorer::BuildStepList *parent, Core::Id id) const
{
    if (parent->id() != Core::Id(ProjectExplorer::Constants::BUILDSTEPS_CLEAN)
            || !qobject_cast<QbsBuildConfiguration *>(parent->parent()))
        return false;
    return id == Core::Id(Constants::QBS_CLEANSTEP_ID);
}

ProjectExplorer::BuildStep *QbsCleanStepFactory::create(ProjectExplorer::BuildStepList *parent, Core::Id id)
{
    if (!canCreate(parent, id))
        return 0;
    return new QbsCleanStep(parent);
}

bool QbsCleanStepFactory::canRestore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map) const
{
    return canCreate(parent, ProjectExplorer::idFromMap(map));
}

ProjectExplorer::BuildStep *QbsCleanStepFactory::restore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    QbsCleanStep *bs = new QbsCleanStep(parent);
    if (!bs->fromMap(map)) {
        delete bs;
        return 0;
    }
    return bs;
}

bool QbsCleanStepFactory::canClone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *product) const
{
    return canCreate(parent, product->id());
}

ProjectExplorer::BuildStep *QbsCleanStepFactory::clone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *product)
{
    if (!canClone(parent, product))
        return 0;
    return new QbsCleanStep(parent, static_cast<QbsCleanStep *>(product));
}

} // namespace Internal
} // namespace QbsProjectManager
