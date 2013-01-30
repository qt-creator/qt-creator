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

#include "qbscleanstep.h"

#include "qbsbuildconfiguration.h"
#include "qbsproject.h"
#include "qbsprojectmanagerconstants.h"

#include "ui_qbscleanstepconfigwidget.h"

#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <utils/qtcassert.h>

static const char QBS_CLEAN_ALL[] = "Qbs.CleanAll";
static const char QBS_DRY_RUN[] = "Qbs.DryRun";
static const char QBS_KEEP_GOING[] = "Qbs.DryKeepGoing";
static const char QBS_MAXJOBCOUNT[] = "Qbs.MaxJobs";

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
    m_cleanAll(false), m_job(0), m_showCompilerOutput(true), m_parser(0)
{
    setDisplayName(tr("qbs clean"));
}

QbsCleanStep::QbsCleanStep(ProjectExplorer::BuildStepList *bsl, const QbsCleanStep *other) :
    ProjectExplorer::BuildStep(bsl, Core::Id(Constants::QBS_CLEANSTEP_ID)),
    m_qbsBuildOptions(other->m_qbsBuildOptions), m_cleanAll(other->m_cleanAll), m_job(0),
    m_showCompilerOutput(other->m_showCompilerOutput), m_parser(0)
{ }

QbsCleanStep::~QbsCleanStep()
{
    cancel();
    m_job->deleteLater();
    m_job = 0;
}

bool QbsCleanStep::init()
{
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
    qbs::BuildOptions options(m_qbsBuildOptions);

    m_job = pro->clean(options, m_cleanAll);

    if (!m_job) {
        m_fi->reportResult(false);
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
    return m_qbsBuildOptions.dryRun;
}

bool QbsCleanStep::keepGoing() const
{
    return m_qbsBuildOptions.keepGoing;
}

int QbsCleanStep::maxJobs() const
{
    return m_qbsBuildOptions.maxJobCount;
}

bool QbsCleanStep::cleanAll() const
{
    return m_cleanAll;
}

bool QbsCleanStep::fromMap(const QVariantMap &map)
{
    if (!ProjectExplorer::BuildStep::fromMap(map))
        return false;

    m_qbsBuildOptions.dryRun = map.value(QLatin1String(QBS_DRY_RUN)).toBool();
    m_qbsBuildOptions.keepGoing = map.value(QLatin1String(QBS_KEEP_GOING)).toBool();
    m_qbsBuildOptions.maxJobCount = map.value(QLatin1String(QBS_MAXJOBCOUNT)).toInt();
    m_cleanAll = map.value(QLatin1String(QBS_CLEAN_ALL)).toBool();

    return true;
}

QVariantMap QbsCleanStep::toMap() const
{
    QVariantMap map = ProjectExplorer::BuildStep::toMap();
    map.insert(QLatin1String(QBS_DRY_RUN), m_qbsBuildOptions.dryRun);
    map.insert(QLatin1String(QBS_KEEP_GOING), m_qbsBuildOptions.keepGoing);
    map.insert(QLatin1String(QBS_MAXJOBCOUNT), m_qbsBuildOptions.maxJobCount);
    map.insert(QLatin1String(QBS_CLEAN_ALL), m_cleanAll);

    return map;
}

void QbsCleanStep::cleaningDone(bool success)
{
    // Report errors:
    foreach (const qbs::ErrorData &data, m_job->error().entries()) {
        createTaskAndOutput(ProjectExplorer::Task::Error, data.description(),
                            data.codeLocation().fileName, data.codeLocation().line);
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
    emit addTask(ProjectExplorer::Task(type, message,
                                       Utils::FileName::fromString(file), line,
                                       ProjectExplorer::Constants::TASK_CATEGORY_COMPILE));
    emit addOutput(message, NormalOutput);
}

void QbsCleanStep::setDryRun(bool dr)
{
    if (m_qbsBuildOptions.dryRun == dr)
        return;
    m_qbsBuildOptions.dryRun = dr;
    emit changed();
}

void QbsCleanStep::setKeepGoing(bool kg)
{
    if (m_qbsBuildOptions.keepGoing == kg)
        return;
    m_qbsBuildOptions.keepGoing = kg;
    emit changed();
}

void QbsCleanStep::setMaxJobs(int jobcount)
{
    if (m_qbsBuildOptions.maxJobCount == jobcount)
        return;
    m_qbsBuildOptions.maxJobCount = jobcount;
    emit changed();
}

void QbsCleanStep::setCleanAll(bool ca)
{
    if (m_cleanAll == ca)
        return;
    m_cleanAll = ca;
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

    connect(m_ui->cleanAllCheckBox, SIGNAL(toggled(bool)),
            this, SLOT(changeCleanAll(bool)));
    connect(m_ui->dryRunCheckBox, SIGNAL(toggled(bool)), this, SLOT(changeDryRun(bool)));
    connect(m_ui->keepGoingCheckBox, SIGNAL(toggled(bool)), this, SLOT(changeKeepGoing(bool)));
    connect(m_ui->jobSpinBox, SIGNAL(valueChanged(int)), this, SLOT(changeJobCount(int)));

    updateState();
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
    m_ui->cleanAllCheckBox->setChecked(m_step->cleanAll());
    m_ui->dryRunCheckBox->setChecked(m_step->dryRun());
    m_ui->keepGoingCheckBox->setChecked(m_step->keepGoing());
    m_ui->jobSpinBox->setValue(m_step->maxJobs());

    qbs::BuildOptions defaultOptions;

    QString command = QLatin1String("qbs clean ");
    if (m_step->dryRun())
        command += QLatin1String("--dryRun ");
    if (m_step->keepGoing())
        command += QLatin1String("--keepGoing ");
    if (m_step->maxJobs() != defaultOptions.maxJobCount)
        command += QString::fromLatin1("--jobs %1 ").arg(m_step->maxJobs());
    if (m_step->cleanAll())
        command += QLatin1String(" --all-artifacts");

    QString summary = tr("<b>Qbs:</b> %1").arg(command);
    if (m_summary !=  summary) {
        m_summary = summary;
        emit updateSummary();
    }
}

void QbsCleanStepConfigWidget::changeCleanAll(bool ca)
{
    m_step->setCleanAll(ca);
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

QString QbsCleanStepFactory::displayNameForId(const Core::Id id) const
{
    if (id == Core::Id(Constants::QBS_CLEANSTEP_ID))
        return tr("Qbs");
    return QString();
}

bool QbsCleanStepFactory::canCreate(ProjectExplorer::BuildStepList *parent, const Core::Id id) const
{
    if (parent->id() != Core::Id(ProjectExplorer::Constants::BUILDSTEPS_CLEAN)
            || !qobject_cast<QbsBuildConfiguration *>(parent->parent()))
        return false;
    return id == Core::Id(Constants::QBS_CLEANSTEP_ID);
}

ProjectExplorer::BuildStep *QbsCleanStepFactory::create(ProjectExplorer::BuildStepList *parent, const Core::Id id)
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
