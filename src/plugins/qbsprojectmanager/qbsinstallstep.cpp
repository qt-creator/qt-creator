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

#include "qbsinstallstep.h"

#include "qbsbuildconfiguration.h"
#include "qbsbuildstep.h"
#include "qbsproject.h"
#include "qbsprojectmanagerconstants.h"

#include "ui_qbsinstallstepconfigwidget.h"

#include <coreplugin/icore.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <utils/qtcassert.h>

#include <QFileInfo>

// --------------------------------------------------------------------
// Constants:
// --------------------------------------------------------------------

static const char QBS_REMOVE_FIRST[] = "Qbs.RemoveFirst";
static const char QBS_DRY_RUN[] = "Qbs.DryRun";
static const char QBS_KEEP_GOING[] = "Qbs.DryKeepGoing";

namespace QbsProjectManager {
namespace Internal {

// --------------------------------------------------------------------
// QbsInstallStep:
// --------------------------------------------------------------------

QbsInstallStep::QbsInstallStep(ProjectExplorer::BuildStepList *bsl) :
    ProjectExplorer::BuildStep(bsl, Core::Id(Constants::QBS_INSTALLSTEP_ID)),
    m_job(0), m_showCompilerOutput(true), m_parser(0)
{
    setDisplayName(tr("Qbs Install"));
    ctor();
}

QbsInstallStep::QbsInstallStep(ProjectExplorer::BuildStepList *bsl, const QbsInstallStep *other) :
    ProjectExplorer::BuildStep(bsl, Core::Id(Constants::QBS_INSTALLSTEP_ID)),
    m_qbsInstallOptions(other->m_qbsInstallOptions), m_job(0),
    m_showCompilerOutput(other->m_showCompilerOutput), m_parser(0)
{
    ctor();
}

QbsInstallStep::~QbsInstallStep()
{
    cancel();
    if (m_job)
        m_job->deleteLater();
    m_job = 0;
}

bool QbsInstallStep::init(QList<const BuildStep *> &earlierSteps)
{
    Q_UNUSED(earlierSteps);
    QTC_ASSERT(!static_cast<QbsProject *>(project())->isParsing() && !m_job, return false);
    return true;
}

void QbsInstallStep::run(QFutureInterface<bool> &fi)
{
    m_fi = &fi;

    QbsProject *pro = static_cast<QbsProject *>(project());
    m_job = pro->install(m_qbsInstallOptions);

    if (!m_job) {
        reportRunResult(*m_fi, false);
        return;
    }

    m_progressBase = 0;

    connect(m_job, &qbs::AbstractJob::finished, this, &QbsInstallStep::installDone);
    connect(m_job, &qbs::AbstractJob::taskStarted,
            this, &QbsInstallStep::handleTaskStarted);
    connect(m_job, &qbs::AbstractJob::taskProgress,
            this, &QbsInstallStep::handleProgress);
}

ProjectExplorer::BuildStepConfigWidget *QbsInstallStep::createConfigWidget()
{
    return new QbsInstallStepConfigWidget(this);
}

bool QbsInstallStep::runInGuiThread() const
{
    return true;
}

void QbsInstallStep::cancel()
{
    if (m_job)
        m_job->cancel();
}

QString QbsInstallStep::installRoot() const
{
    const QbsBuildStep * const bs = buildConfig()->qbsStep();
    return bs ? bs->installRoot().toString() : QString();
}

bool QbsInstallStep::removeFirst() const
{
    return m_qbsInstallOptions.removeExistingInstallation();
}

bool QbsInstallStep::dryRun() const
{
    return m_qbsInstallOptions.dryRun();
}

bool QbsInstallStep::keepGoing() const
{
    return m_qbsInstallOptions.keepGoing();
}

void QbsInstallStep::ctor()
{
    const QbsBuildConfiguration * const bc = buildConfig();
    connect(bc, &QbsBuildConfiguration::qbsConfigurationChanged,
            this, &QbsInstallStep::handleBuildConfigChanged);
    if (bc->qbsStep()) {
        connect(bc->qbsStep(), &QbsBuildStep::qbsBuildOptionsChanged,
                this, &QbsInstallStep::handleBuildConfigChanged);
    }
}

const QbsBuildConfiguration *QbsInstallStep::buildConfig() const
{
    return static_cast<QbsBuildConfiguration *>(
                deployConfiguration()->target()->activeBuildConfiguration());
}

bool QbsInstallStep::fromMap(const QVariantMap &map)
{
    if (!ProjectExplorer::BuildStep::fromMap(map))
        return false;

    m_qbsInstallOptions.setInstallRoot(installRoot());
    m_qbsInstallOptions.setRemoveExistingInstallation(
                map.value(QLatin1String(QBS_REMOVE_FIRST), false).toBool());
    m_qbsInstallOptions.setDryRun(map.value(QLatin1String(QBS_DRY_RUN), false).toBool());
    m_qbsInstallOptions.setKeepGoing(map.value(QLatin1String(QBS_KEEP_GOING), false).toBool());

    return true;
}

QVariantMap QbsInstallStep::toMap() const
{
    QVariantMap map = ProjectExplorer::BuildStep::toMap();
    map.insert(QLatin1String(QBS_REMOVE_FIRST), m_qbsInstallOptions.removeExistingInstallation());
    map.insert(QLatin1String(QBS_DRY_RUN), m_qbsInstallOptions.dryRun());
    map.insert(QLatin1String(QBS_KEEP_GOING), m_qbsInstallOptions.keepGoing());
    return map;
}

qbs::InstallOptions QbsInstallStep::installOptions() const
{
    return m_qbsInstallOptions;
}

void QbsInstallStep::installDone(bool success)
{
    // Report errors:
    foreach (const qbs::ErrorItem &item, m_job->error().items()) {
        createTaskAndOutput(ProjectExplorer::Task::Error, item.description(),
                            item.codeLocation().filePath(), item.codeLocation().line());
    }

    QTC_ASSERT(m_fi, return);
    reportRunResult(*m_fi, success);
    m_fi = 0; // do not delete, it is not ours
    m_job->deleteLater();
    m_job = 0;
}

void QbsInstallStep::handleTaskStarted(const QString &desciption, int max)
{
    Q_UNUSED(desciption);
    QTC_ASSERT(m_fi, return);
    m_progressBase = m_fi->progressValue();
    m_fi->setProgressRange(0, m_progressBase + max);
}

void QbsInstallStep::handleProgress(int value)
{
    QTC_ASSERT(m_fi, return);
    m_fi->setProgressValue(m_progressBase + value);
}

void QbsInstallStep::createTaskAndOutput(ProjectExplorer::Task::TaskType type,
                                         const QString &message, const QString &file, int line)
{
    ProjectExplorer::Task task = ProjectExplorer::Task(type, message,
                                                       Utils::FileName::fromString(file), line,
                                                       ProjectExplorer::Constants::TASK_CATEGORY_COMPILE);
    emit addTask(task, 1);
    emit addOutput(message, OutputFormat::Stdout);
}

void QbsInstallStep::setRemoveFirst(bool rf)
{
    if (m_qbsInstallOptions.removeExistingInstallation() == rf)
        return;
    m_qbsInstallOptions.setRemoveExistingInstallation(rf);
    emit changed();
}

void QbsInstallStep::setDryRun(bool dr)
{
    if (m_qbsInstallOptions.dryRun() == dr)
        return;
    m_qbsInstallOptions.setDryRun(dr);
    emit changed();
}

void QbsInstallStep::setKeepGoing(bool kg)
{
    if (m_qbsInstallOptions.keepGoing() == kg)
        return;
    m_qbsInstallOptions.setKeepGoing(kg);
    emit changed();
}

void QbsInstallStep::handleBuildConfigChanged()
{
    m_qbsInstallOptions.setInstallRoot(installRoot());
    emit changed();
}

// --------------------------------------------------------------------
// QbsInstallStepConfigWidget:
// --------------------------------------------------------------------

QbsInstallStepConfigWidget::QbsInstallStepConfigWidget(QbsInstallStep *step) :
    m_step(step), m_ignoreChange(false)
{
    connect(m_step, &ProjectExplorer::ProjectConfiguration::displayNameChanged,
            this, &QbsInstallStepConfigWidget::updateState);
    connect(m_step, &QbsInstallStep::changed,
            this, &QbsInstallStepConfigWidget::updateState);

    setContentsMargins(0, 0, 0, 0);

    QbsProject *project = static_cast<QbsProject *>(m_step->project());

    m_ui = new Ui::QbsInstallStepConfigWidget;
    m_ui->setupUi(this);

    connect(m_ui->removeFirstCheckBox, &QAbstractButton::toggled,
            this, &QbsInstallStepConfigWidget::changeRemoveFirst);
    connect(m_ui->dryRunCheckBox, &QAbstractButton::toggled,
            this, &QbsInstallStepConfigWidget::changeDryRun);
    connect(m_ui->keepGoingCheckBox, &QAbstractButton::toggled,
            this, &QbsInstallStepConfigWidget::changeKeepGoing);

    connect(project, &ProjectExplorer::Project::parsingFinished,
            this, &QbsInstallStepConfigWidget::updateState);

    updateState();
}

QbsInstallStepConfigWidget::~QbsInstallStepConfigWidget()
{
    delete m_ui;
}

QString QbsInstallStepConfigWidget::summaryText() const
{
    return m_summary;
}

QString QbsInstallStepConfigWidget::displayName() const
{
    return m_step->displayName();
}

void QbsInstallStepConfigWidget::updateState()
{
    if (!m_ignoreChange) {
        m_ui->installRootValueLabel->setText(m_step->installRoot());
        m_ui->removeFirstCheckBox->setChecked(m_step->removeFirst());
        m_ui->dryRunCheckBox->setChecked(m_step->dryRun());
        m_ui->keepGoingCheckBox->setChecked(m_step->keepGoing());
    }

    QString command = m_step->buildConfig()->equivalentCommandLine(m_step);

    m_ui->commandLineTextEdit->setPlainText(command);

    QString summary = tr("<b>Qbs:</b> %1").arg(command);
    if (m_summary != summary) {
        m_summary = summary;
        emit updateSummary();
    }
}

void QbsInstallStepConfigWidget::changeRemoveFirst(bool rf)
{
    m_step->setRemoveFirst(rf);
}

void QbsInstallStepConfigWidget::changeDryRun(bool dr)
{
    m_step->setDryRun(dr);
}

void QbsInstallStepConfigWidget::changeKeepGoing(bool kg)
{
    m_step->setKeepGoing(kg);
}

// --------------------------------------------------------------------
// QbsInstallStepFactory:
// --------------------------------------------------------------------

QbsInstallStepFactory::QbsInstallStepFactory(QObject *parent) :
    ProjectExplorer::IBuildStepFactory(parent)
{ }

QList<ProjectExplorer::BuildStepInfo> QbsInstallStepFactory::availableSteps(ProjectExplorer::BuildStepList *parent) const
{
    if (parent->id() == ProjectExplorer::Constants::BUILDSTEPS_DEPLOY
            && qobject_cast<ProjectExplorer::DeployConfiguration *>(parent->parent())
            && qobject_cast<QbsProject *>(parent->target()->project()))
        return {{ Constants::QBS_INSTALLSTEP_ID, tr("Qbs Install") }};
    return {};
}

ProjectExplorer::BuildStep *QbsInstallStepFactory::create(ProjectExplorer::BuildStepList *parent,
                                                          const Core::Id id)
{
    Q_UNUSED(id);
    return new QbsInstallStep(parent);
}

ProjectExplorer::BuildStep *QbsInstallStepFactory::clone(ProjectExplorer::BuildStepList *parent,
                                                         ProjectExplorer::BuildStep *product)
{
    return new QbsInstallStep(parent, static_cast<QbsInstallStep *>(product));
}

} // namespace Internal
} // namespace QbsProjectManager
