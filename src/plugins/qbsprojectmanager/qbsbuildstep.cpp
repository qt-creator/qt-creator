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

#include "qbsbuildstep.h"

#include "qbsbuildconfiguration.h"
#include "qbsparser.h"
#include "qbsproject.h"
#include "qbsprojectmanagerconstants.h"

#include "ui_qbsbuildstepconfigwidget.h"

#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <utils/qtcassert.h>

#include <qbs.h>

static const char QBS_CONFIG[] = "Qbs.Configuration";
static const char QBS_DRY_RUN[] = "Qbs.DryRun";
static const char QBS_KEEP_GOING[] = "Qbs.DryKeepGoing";
static const char QBS_MAXJOBCOUNT[] = "Qbs.MaxJobs";

// --------------------------------------------------------------------
// Constants:
// --------------------------------------------------------------------

namespace QbsProjectManager {
namespace Internal {

// --------------------------------------------------------------------
// QbsBuildStep:
// --------------------------------------------------------------------

QbsBuildStep::QbsBuildStep(ProjectExplorer::BuildStepList *bsl) :
    ProjectExplorer::BuildStep(bsl, Core::Id(Constants::QBS_BUILDSTEP_ID)),
    m_job(0), m_showCompilerOutput(true), m_parser(0)
{
    setDisplayName(tr("qbs build"));
}

QbsBuildStep::QbsBuildStep(ProjectExplorer::BuildStepList *bsl, const QbsBuildStep *other) :
    ProjectExplorer::BuildStep(bsl, Core::Id(Constants::QBS_BUILDSTEP_ID)),
    m_qbsBuildOptions(other->m_qbsBuildOptions),  m_job(0),
    m_showCompilerOutput(other->m_showCompilerOutput), m_parser(0)
{ }

QbsBuildStep::~QbsBuildStep()
{
    cancel();
    m_job->deleteLater();
    m_job = 0;
    delete m_parser;
}

bool QbsBuildStep::init()
{
    if (static_cast<QbsProject *>(project())->isParsing() || m_job)
        return false;

    QbsBuildConfiguration *bc = static_cast<QbsBuildConfiguration *>(buildConfiguration());
    if (!bc)
        bc = static_cast<QbsBuildConfiguration *>(target()->activeBuildConfiguration());

    if (!bc)
        return false;

    delete m_parser;
    m_parser = new Internal::QbsParser;
    ProjectExplorer::IOutputParser *parser = target()->kit()->createOutputParser();
    if (parser)
        m_parser->appendOutputParser(parser);

    m_changedFiles = bc->changedFiles();

    connect(m_parser, SIGNAL(addOutput(QString,ProjectExplorer::BuildStep::OutputFormat)),
            this, SIGNAL(addOutput(QString,ProjectExplorer::BuildStep::OutputFormat)));
    connect(m_parser, SIGNAL(addTask(ProjectExplorer::Task)),
            this, SIGNAL(addTask(ProjectExplorer::Task)));

    return true;
}

void QbsBuildStep::run(QFutureInterface<bool> &fi)
{
    m_fi = &fi;

    QbsProject *pro = static_cast<QbsProject *>(project());
    qbs::BuildOptions options(m_qbsBuildOptions);
    options.changedFiles = m_changedFiles;

    m_job = pro->build(options);

    if (!m_job) {
        m_fi->reportResult(false);
        return;
    }

    m_progressBase = 0;

    connect(m_job, SIGNAL(finished(bool,qbs::AbstractJob*)), this, SLOT(buildingDone(bool)));
    connect(m_job, SIGNAL(taskStarted(QString,int,qbs::AbstractJob*)),
            this, SLOT(handleTaskStarted(QString,int)));
    connect(m_job, SIGNAL(taskProgress(int,qbs::AbstractJob*)),
            this, SLOT(handleProgress(int)));
    connect(m_job, SIGNAL(reportCommandDescription(QString,QString)),
            this, SLOT(handleCommandDescriptionReport(QString,QString)));
    connect(m_job, SIGNAL(reportWarning(qbs::Error)),
            this, SLOT(handleWarningReport(qbs::Error)));
    connect(m_job, SIGNAL(reportProcessResult(qbs::ProcessResult)),
            this, SLOT(handleProcessResultReport(qbs::ProcessResult)));
}

ProjectExplorer::BuildStepConfigWidget *QbsBuildStep::createConfigWidget()
{
    return new QbsBuildStepConfigWidget(this);
}

bool QbsBuildStep::runInGuiThread() const
{
    return true;
}

void QbsBuildStep::cancel()
{
    if (m_job)
        m_job->cancel();
}

QVariantMap QbsBuildStep::qbsConfiguration() const
{
    return m_qbsConfiguration;
}

void QbsBuildStep::setQbsConfiguration(const QVariantMap &config)
{
    QbsProject *pro = static_cast<QbsProject *>(project());

    QVariantMap tmp = config;
    tmp.insert(QLatin1String(Constants::QBS_CONFIG_PROFILE_KEY), pro->projectManager()->profileForKit(target()->kit()));
    if (!tmp.contains(QLatin1String(Constants::QBS_CONFIG_VARIANT_KEY)))
        tmp.insert(QLatin1String(Constants::QBS_CONFIG_VARIANT_KEY),
                   QString::fromLatin1(Constants::QBS_VARIANT_DEBUG));

    if (tmp == m_qbsConfiguration)
        return;
    m_qbsConfiguration = tmp;
    emit qbsConfigurationChanged();
}

bool QbsBuildStep::dryRun() const
{
    return m_qbsBuildOptions.dryRun;
}

bool QbsBuildStep::keepGoing() const
{
    return m_qbsBuildOptions.keepGoing;
}

int QbsBuildStep::maxJobs() const
{
    return m_qbsBuildOptions.maxJobCount;
}

bool QbsBuildStep::fromMap(const QVariantMap &map)
{
    if (!ProjectExplorer::BuildStep::fromMap(map))
        return false;

    setQbsConfiguration(map.value(QLatin1String(QBS_CONFIG)).toMap());
    m_qbsBuildOptions.dryRun = map.value(QLatin1String(QBS_DRY_RUN)).toBool();
    m_qbsBuildOptions.keepGoing = map.value(QLatin1String(QBS_KEEP_GOING)).toBool();
    m_qbsBuildOptions.maxJobCount = map.value(QLatin1String(QBS_MAXJOBCOUNT)).toInt();
    return true;
}

QVariantMap QbsBuildStep::toMap() const
{
    QVariantMap map = ProjectExplorer::BuildStep::toMap();
    map.insert(QLatin1String(QBS_CONFIG), m_qbsConfiguration);
    map.insert(QLatin1String(QBS_DRY_RUN), m_qbsBuildOptions.dryRun);
    map.insert(QLatin1String(QBS_KEEP_GOING), m_qbsBuildOptions.keepGoing);
    map.insert(QLatin1String(QBS_MAXJOBCOUNT), m_qbsBuildOptions.maxJobCount);
    return map;
}

void QbsBuildStep::buildingDone(bool success)
{
    // Report errors:
    foreach (const qbs::ErrorData &data, m_job->error().entries())
        createTaskAndOutput(ProjectExplorer::Task::Error, data.description(),
                            data.codeLocation().fileName, data.codeLocation().line);

    QTC_ASSERT(m_fi, return);
    m_fi->reportResult(success);
    m_fi = 0; // do not delete, it is not ours
    m_job->deleteLater();
    m_job = 0;

    emit finished();
}

void QbsBuildStep::handleTaskStarted(const QString &desciption, int max)
{
    Q_UNUSED(desciption);
    QTC_ASSERT(m_fi, return);

    m_progressBase = m_fi->progressValue();
    m_fi->setProgressRange(0, m_progressBase + max);
}

void QbsBuildStep::handleProgress(int value)
{
    QTC_ASSERT(m_fi, return);
    m_fi->setProgressValue(m_progressBase + value);
}

void QbsBuildStep::handleWarningReport(const qbs::Error &error)
{
    foreach (const qbs::ErrorData &data, error.entries()) {
        createTaskAndOutput(ProjectExplorer::Task::Warning, data.description(),
                            data.codeLocation().fileName, data.codeLocation().line);
    }
}

void QbsBuildStep::handleCommandDescriptionReport(const QString &highlight, const QString &message)
{
    Q_UNUSED(highlight);
    emit addOutput(message, NormalOutput);
}

void QbsBuildStep::handleProcessResultReport(const qbs::ProcessResult &result)
{
    bool hasOutput = !result.stdOut.isEmpty() || !result.stdErr.isEmpty();

    if (result.success && !hasOutput && !m_showCompilerOutput)
        return;

    m_parser->setWorkingDirectory(result.workingDirectory);
    foreach (const QString &line, result.stdErr) {
        m_parser->stdError(line);
        addOutput(line, ErrorOutput);
    }
    foreach (const QString &line, result.stdOut) {
        m_parser->stdOutput(line);
        addOutput(line, NormalOutput);
    }
}

void QbsBuildStep::createTaskAndOutput(ProjectExplorer::Task::TaskType type, const QString &message,
                                       const QString &file, int line)
{
    emit addTask(ProjectExplorer::Task(type, message,
                                       Utils::FileName::fromString(file), line,
                                       ProjectExplorer::Constants::TASK_CATEGORY_COMPILE));
    emit addOutput(message, NormalOutput);
}

QString QbsBuildStep::buildVariant() const
{
    return qbsConfiguration().value(QLatin1String(Constants::QBS_CONFIG_VARIANT_KEY)).toString();
}

void QbsBuildStep::setBuildVariant(const QString &variant)
{
    if (m_qbsConfiguration.value(QLatin1String(Constants::QBS_CONFIG_VARIANT_KEY)).toString() == variant)
        return;
    m_qbsConfiguration.insert(QLatin1String(Constants::QBS_CONFIG_VARIANT_KEY), variant);
    emit qbsConfigurationChanged();
}

QString QbsBuildStep::profile() const
{
    return qbsConfiguration().value(QLatin1String(Constants::QBS_CONFIG_PROFILE_KEY)).toString();
}

void QbsBuildStep::setDryRun(bool dr)
{
    if (m_qbsBuildOptions.dryRun == dr)
        return;
    m_qbsBuildOptions.dryRun = dr;
    emit qbsBuildOptionsChanged();
}

void QbsBuildStep::setKeepGoing(bool kg)
{
    if (m_qbsBuildOptions.keepGoing == kg)
        return;
    m_qbsBuildOptions.keepGoing = kg;
    emit qbsBuildOptionsChanged();
}

void QbsBuildStep::setMaxJobs(int jobcount)
{
    if (m_qbsBuildOptions.maxJobCount == jobcount)
        return;
    m_qbsBuildOptions.maxJobCount = jobcount;
    emit qbsBuildOptionsChanged();
}

// --------------------------------------------------------------------
// QbsBuildStepConfigWidget:
// --------------------------------------------------------------------

QbsBuildStepConfigWidget::QbsBuildStepConfigWidget(QbsBuildStep *step) :
    m_step(step)
{
    connect(m_step, SIGNAL(displayNameChanged()), this, SLOT(updateState()));
    connect(m_step, SIGNAL(qbsConfigurationChanged()), this, SLOT(updateState()));
    connect(m_step, SIGNAL(qbsBuildOptionsChanged()), this, SLOT(updateState()));

    setContentsMargins(0, 0, 0, 0);

    m_ui = new Ui::QbsBuildStepConfigWidget;
    m_ui->setupUi(this);

    connect(m_ui->buildVariantComboBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(changeBuildVariant(int)));
    connect(m_ui->dryRunCheckBox, SIGNAL(toggled(bool)), this, SLOT(changeDryRun(bool)));
    connect(m_ui->keepGoingCheckBox, SIGNAL(toggled(bool)), this, SLOT(changeKeepGoing(bool)));
    connect(m_ui->jobSpinBox, SIGNAL(valueChanged(int)), this, SLOT(changeJobCount(int)));

    updateState();
}

QString QbsBuildStepConfigWidget::summaryText() const
{
    return m_summary;
}

QString QbsBuildStepConfigWidget::displayName() const
{
    return m_step->displayName();
}

void QbsBuildStepConfigWidget::updateState()
{
    m_ui->dryRunCheckBox->setChecked(m_step->dryRun());
    m_ui->keepGoingCheckBox->setChecked(m_step->keepGoing());
    m_ui->jobSpinBox->setValue(m_step->maxJobs());

    const QString buildVariant = m_step->buildVariant();
    const int idx = (buildVariant == QLatin1String(Constants::QBS_VARIANT_DEBUG)) ? 0 : 1;
    m_ui->buildVariantComboBox->setCurrentIndex(idx);

    qbs::BuildOptions defaultOptions;

    QString command = QLatin1String("qbs ");
    if (m_step->dryRun())
        command += QLatin1String("--dryRun ");
    if (m_step->keepGoing())
        command += QLatin1String("--keepGoing ");
    if (m_step->maxJobs() != defaultOptions.maxJobCount)
        command += QString::fromLatin1("--jobs %1 ").arg(m_step->maxJobs());
    command += QString::fromLatin1("build profile:%1 %2").arg(m_step->profile(), buildVariant);

    QString summary = tr("<b>Qbs:</b> %1").arg(command);
    if (m_summary !=  summary) {
        m_summary = summary;
        emit updateSummary();
    }
}

void QbsBuildStepConfigWidget::changeBuildVariant(int idx)
{
    QString variant;
    if (idx == 1)
        variant = QLatin1String(Constants::QBS_VARIANT_RELEASE);
    else
        variant = QLatin1String(Constants::QBS_VARIANT_DEBUG);
    m_step->setBuildVariant(variant);
}

void QbsBuildStepConfigWidget::changeDryRun(bool dr)
{
    m_step->setDryRun(dr);
}

void QbsBuildStepConfigWidget::changeKeepGoing(bool kg)
{
    m_step->setKeepGoing(kg);
}

void QbsBuildStepConfigWidget::changeJobCount(int count)
{
    m_step->setMaxJobs(count);
}

// --------------------------------------------------------------------
// QbsBuildStepFactory:
// --------------------------------------------------------------------

QbsBuildStepFactory::QbsBuildStepFactory(QObject *parent) :
    ProjectExplorer::IBuildStepFactory(parent)
{ }

QList<Core::Id> QbsBuildStepFactory::availableCreationIds(ProjectExplorer::BuildStepList *parent) const
{
    if (parent->id() == ProjectExplorer::Constants::BUILDSTEPS_BUILD
            && qobject_cast<QbsBuildConfiguration *>(parent->parent()))
        return QList<Core::Id>() << Core::Id(Constants::QBS_BUILDSTEP_ID);
    return QList<Core::Id>();
}

QString QbsBuildStepFactory::displayNameForId(const Core::Id id) const
{
    if (id == Core::Id(Constants::QBS_BUILDSTEP_ID))
        return tr("Qbs");
    return QString();
}

bool QbsBuildStepFactory::canCreate(ProjectExplorer::BuildStepList *parent, const Core::Id id) const
{
    if (parent->id() != Core::Id(ProjectExplorer::Constants::BUILDSTEPS_BUILD)
            || !qobject_cast<QbsBuildConfiguration *>(parent->parent()))
        return false;
    return id == Core::Id(Constants::QBS_BUILDSTEP_ID);
}

ProjectExplorer::BuildStep *QbsBuildStepFactory::create(ProjectExplorer::BuildStepList *parent, const Core::Id id)
{
    if (!canCreate(parent, id))
        return 0;
    return new QbsBuildStep(parent);
}

bool QbsBuildStepFactory::canRestore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map) const
{
    return canCreate(parent, ProjectExplorer::idFromMap(map));
}

ProjectExplorer::BuildStep *QbsBuildStepFactory::restore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    QbsBuildStep *bs = new QbsBuildStep(parent);
    if (!bs->fromMap(map)) {
        delete bs;
        return 0;
    }
    return bs;
}

bool QbsBuildStepFactory::canClone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *product) const
{
    return canCreate(parent, product->id());
}

ProjectExplorer::BuildStep *QbsBuildStepFactory::clone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *product)
{
    if (!canClone(parent, product))
        return 0;
    return new QbsBuildStep(parent, static_cast<QbsBuildStep *>(product));
}

} // namespace Internal
} // namespace QbsProjectManager
