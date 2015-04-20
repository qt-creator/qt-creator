/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qbsinstallstep.h"

#include "qbsbuildconfiguration.h"
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

static const char QBS_INSTALL_ROOT[] = "Qbs.InstallRoot";
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
}

QbsInstallStep::QbsInstallStep(ProjectExplorer::BuildStepList *bsl, const QbsInstallStep *other) :
    ProjectExplorer::BuildStep(bsl, Core::Id(Constants::QBS_INSTALLSTEP_ID)),
    m_qbsInstallOptions(other->m_qbsInstallOptions), m_job(0),
    m_showCompilerOutput(other->m_showCompilerOutput), m_parser(0)
{ }

QbsInstallStep::~QbsInstallStep()
{
    cancel();
    if (m_job)
        m_job->deleteLater();
    m_job = 0;
}

bool QbsInstallStep::init()
{
    QTC_ASSERT(!static_cast<QbsProject *>(project())->isParsing() && !m_job, return false);
    return true;
}

void QbsInstallStep::run(QFutureInterface<bool> &fi)
{
    m_fi = &fi;

    QbsProject *pro = static_cast<QbsProject *>(project());
    m_job = pro->install(m_qbsInstallOptions);

    if (!m_job) {
        m_fi->reportResult(false);
        emit finished();
        return;
    }

    m_progressBase = 0;

    connect(m_job, SIGNAL(finished(bool,qbs::AbstractJob*)), this, SLOT(installDone(bool)));
    connect(m_job, SIGNAL(taskStarted(QString,int,qbs::AbstractJob*)),
            this, SLOT(handleTaskStarted(QString,int)));
    connect(m_job, SIGNAL(taskProgress(int,qbs::AbstractJob*)),
            this, SLOT(handleProgress(int)));
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
    if (!m_qbsInstallOptions.installRoot().isEmpty())
        return m_qbsInstallOptions.installRoot();

    return qbs::InstallOptions::defaultInstallRoot();
}

QString QbsInstallStep::absoluteInstallRoot() const
{
    const qbs::ProjectData data = static_cast<QbsProject *>(project())->qbsProjectData();
    QString path = installRoot();
    if (data.isValid() && !data.buildDirectory().isEmpty() && !path.isEmpty())
        path = QDir(data.buildDirectory()).absoluteFilePath(path);
    return path;
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

bool QbsInstallStep::fromMap(const QVariantMap &map)
{
    if (!ProjectExplorer::BuildStep::fromMap(map))
        return false;

    setInstallRoot(map.value(QLatin1String(QBS_INSTALL_ROOT)).toString());
    m_qbsInstallOptions.setRemoveExistingInstallation(map.value(QLatin1String(QBS_REMOVE_FIRST), false).toBool());
    m_qbsInstallOptions.setDryRun(map.value(QLatin1String(QBS_DRY_RUN), false).toBool());
    m_qbsInstallOptions.setKeepGoing(map.value(QLatin1String(QBS_KEEP_GOING), false).toBool());

    return true;
}

QVariantMap QbsInstallStep::toMap() const
{
    QVariantMap map = ProjectExplorer::BuildStep::toMap();
    map.insert(QLatin1String(QBS_INSTALL_ROOT), m_qbsInstallOptions.installRoot());
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
    m_fi->reportResult(success);
    m_fi = 0; // do not delete, it is not ours
    m_job->deleteLater();
    m_job = 0;

    emit finished();
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
    emit addOutput(message, NormalOutput);
}

void QbsInstallStep::setInstallRoot(const QString &ir)
{
    if (m_qbsInstallOptions.installRoot() == ir)
        return;
    m_qbsInstallOptions.setInstallRoot(ir);
    emit changed();
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

// --------------------------------------------------------------------
// QbsInstallStepConfigWidget:
// --------------------------------------------------------------------

QbsInstallStepConfigWidget::QbsInstallStepConfigWidget(QbsInstallStep *step) :
    m_step(step), m_ignoreChange(false)
{
    connect(m_step, SIGNAL(displayNameChanged()), this, SLOT(updateState()));
    connect(m_step, SIGNAL(changed()), this, SLOT(updateState()));

    setContentsMargins(0, 0, 0, 0);

    QbsProject *project = static_cast<QbsProject *>(m_step->project());

    m_ui = new Ui::QbsInstallStepConfigWidget;
    m_ui->setupUi(this);

    m_ui->installRootChooser->setPromptDialogTitle(tr("Qbs Install Prefix"));
    m_ui->installRootChooser->setExpectedKind(Utils::PathChooser::Directory);
    m_ui->installRootChooser->setHistoryCompleter(QLatin1String("Qbs.InstallRoot.History"));

    connect(m_ui->installRootChooser, SIGNAL(changed(QString)), this, SLOT(changeInstallRoot()));
    connect(m_ui->removeFirstCheckBox, SIGNAL(toggled(bool)), this, SLOT(changeRemoveFirst(bool)));
    connect(m_ui->dryRunCheckBox, SIGNAL(toggled(bool)), this, SLOT(changeDryRun(bool)));
    connect(m_ui->keepGoingCheckBox, SIGNAL(toggled(bool)), this, SLOT(changeKeepGoing(bool)));

    connect(project, SIGNAL(projectParsingDone(bool)), this, SLOT(updateState()));

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
        m_ui->installRootChooser->setPath(m_step->installRoot());
        m_ui->removeFirstCheckBox->setChecked(m_step->removeFirst());
        m_ui->dryRunCheckBox->setChecked(m_step->dryRun());
        m_ui->keepGoingCheckBox->setChecked(m_step->keepGoing());
    }

    m_ui->installRootChooser->setBaseFileName(m_step->target()->activeBuildConfiguration()->buildDirectory());

    QString command = QbsBuildConfiguration::equivalentCommandLine(m_step);
    m_ui->commandLineTextEdit->setPlainText(command);

    QString summary = tr("<b>Qbs:</b> %1").arg(command);
    if (m_summary != summary) {
        m_summary = summary;
        emit updateSummary();
    }
}

void QbsInstallStepConfigWidget::changeInstallRoot()
{
    const QString path = m_ui->installRootChooser->path();
    if (m_step->installRoot() == path)
        return;

    m_ignoreChange = true;
    m_step->setInstallRoot(path);
    m_ignoreChange = false;
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

QList<Core::Id> QbsInstallStepFactory::availableCreationIds(ProjectExplorer::BuildStepList *parent) const
{
    if (parent->id() == ProjectExplorer::Constants::BUILDSTEPS_DEPLOY
            && qobject_cast<ProjectExplorer::DeployConfiguration *>(parent->parent())
            && qobject_cast<QbsProject *>(parent->target()->project()))
        return QList<Core::Id>() << Core::Id(Constants::QBS_INSTALLSTEP_ID);
    return QList<Core::Id>();
}

QString QbsInstallStepFactory::displayNameForId(Core::Id id) const
{
    if (id == Core::Id(Constants::QBS_INSTALLSTEP_ID))
        return tr("Qbs Install");
    return QString();
}

bool QbsInstallStepFactory::canCreate(ProjectExplorer::BuildStepList *parent, Core::Id id) const
{
    if (parent->id() != Core::Id(ProjectExplorer::Constants::BUILDSTEPS_DEPLOY)
            || !qobject_cast<ProjectExplorer::DeployConfiguration *>(parent->parent())
            || !qobject_cast<QbsProject *>(parent->target()->project()))
        return false;
    return id == Core::Id(Constants::QBS_INSTALLSTEP_ID);
}

ProjectExplorer::BuildStep *QbsInstallStepFactory::create(ProjectExplorer::BuildStepList *parent,
                                                          const Core::Id id)
{
    if (!canCreate(parent, id))
        return 0;
    return new QbsInstallStep(parent);
}

bool QbsInstallStepFactory::canRestore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map) const
{
    return canCreate(parent, ProjectExplorer::idFromMap(map));
}

ProjectExplorer::BuildStep *QbsInstallStepFactory::restore(ProjectExplorer::BuildStepList *parent,
                                                           const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    QbsInstallStep *bs = new QbsInstallStep(parent);
    if (!bs->fromMap(map)) {
        delete bs;
        return 0;
    }
    return bs;
}

bool QbsInstallStepFactory::canClone(ProjectExplorer::BuildStepList *parent,
                                     ProjectExplorer::BuildStep *product) const
{
    return canCreate(parent, product->id());
}

ProjectExplorer::BuildStep *QbsInstallStepFactory::clone(ProjectExplorer::BuildStepList *parent,
                                                         ProjectExplorer::BuildStep *product)
{
    if (!canClone(parent, product))
        return 0;
    return new QbsInstallStep(parent, static_cast<QbsInstallStep *>(product));
}

} // namespace Internal
} // namespace QbsProjectManager
