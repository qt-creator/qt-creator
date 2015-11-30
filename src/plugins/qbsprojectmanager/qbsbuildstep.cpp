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

#include "qbsbuildstep.h"

#include "qbsbuildconfiguration.h"
#include "qbsparser.h"
#include "qbsproject.h"
#include "qbsprojectmanagerconstants.h"

#include "ui_qbsbuildstepconfigwidget.h"

#include <coreplugin/icore.h>
#include <coreplugin/coreicons.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <qtsupport/debugginghelperbuildtask.h>
#include <qtsupport/qtversionmanager.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <qbs.h>

static const char QBS_CONFIG[] = "Qbs.Configuration";
static const char QBS_DRY_RUN[] = "Qbs.DryRun";
static const char QBS_KEEP_GOING[] = "Qbs.DryKeepGoing";
static const char QBS_MAXJOBCOUNT[] = "Qbs.MaxJobs";
static const char QBS_SHOWCOMMANDLINES[] = "Qbs.ShowCommandLines";
static const char QBS_INSTALL[] = "Qbs.Install";
static const char QBS_CLEAN_INSTALL_ROOT[] = "Qbs.CleanInstallRoot";

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
    m_job(0), m_parser(0), m_parsingProject(false)
{
    setDisplayName(tr("Qbs Build"));
    setQbsConfiguration(QVariantMap());
}

QbsBuildStep::QbsBuildStep(ProjectExplorer::BuildStepList *bsl, const QbsBuildStep *other) :
    ProjectExplorer::BuildStep(bsl, Core::Id(Constants::QBS_BUILDSTEP_ID)),
    m_qbsBuildOptions(other->m_qbsBuildOptions),  m_job(0), m_parser(0), m_parsingProject(false)
{
    setQbsConfiguration(other->qbsConfiguration());
}

QbsBuildStep::~QbsBuildStep()
{
    cancel();
    if (m_job) {
        m_job->deleteLater();
        m_job = 0;
    }
    delete m_parser;
}

bool QbsBuildStep::init(QList<const BuildStep *> &earlierSteps)
{
    Q_UNUSED(earlierSteps);
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
    m_activeFileTags = bc->activeFileTags();
    m_products = bc->products();

    connect(m_parser, SIGNAL(addOutput(QString,ProjectExplorer::BuildStep::OutputFormat)),
            this, SIGNAL(addOutput(QString,ProjectExplorer::BuildStep::OutputFormat)));
    connect(m_parser, SIGNAL(addTask(ProjectExplorer::Task)),
            this, SIGNAL(addTask(ProjectExplorer::Task)));

    return true;
}

void QbsBuildStep::run(QFutureInterface<bool> &fi)
{
    m_fi = &fi;

    // We need a pre-build parsing step in order not to lose project file changes done
    // right before building (but before the delay has elapsed).
    parseProject();
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
    if (m_parsingProject)
        qbsProject()->cancelParsing();
    else if (m_job)
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
    QbsBuildConfiguration *bc = static_cast<QbsBuildConfiguration *>(buildConfiguration());
    if (bc)
        bc->emitBuildTypeChanged();
    emit qbsConfigurationChanged();
}

bool QbsBuildStep::dryRun() const
{
    return m_qbsBuildOptions.dryRun();
}

bool QbsBuildStep::keepGoing() const
{
    return m_qbsBuildOptions.keepGoing();
}

bool QbsBuildStep::showCommandLines() const
{
    return m_qbsBuildOptions.echoMode() == qbs::CommandEchoModeCommandLine;
}

bool QbsBuildStep::install() const
{
    return m_qbsBuildOptions.install();
}

bool QbsBuildStep::cleanInstallRoot() const
{
    return m_qbsBuildOptions.removeExistingInstallation();
}

int QbsBuildStep::maxJobs() const
{
    if (m_qbsBuildOptions.maxJobCount() > 0)
        return m_qbsBuildOptions.maxJobCount();
    return qbs::BuildOptions::defaultMaxJobCount();
}

bool QbsBuildStep::fromMap(const QVariantMap &map)
{
    if (!ProjectExplorer::BuildStep::fromMap(map))
        return false;

    setQbsConfiguration(map.value(QLatin1String(QBS_CONFIG)).toMap());
    m_qbsBuildOptions.setDryRun(map.value(QLatin1String(QBS_DRY_RUN)).toBool());
    m_qbsBuildOptions.setKeepGoing(map.value(QLatin1String(QBS_KEEP_GOING)).toBool());
    m_qbsBuildOptions.setMaxJobCount(map.value(QLatin1String(QBS_MAXJOBCOUNT)).toInt());
    const bool showCommandLines = map.value(QLatin1String(QBS_SHOWCOMMANDLINES)).toBool();
    m_qbsBuildOptions.setEchoMode(showCommandLines ? qbs::CommandEchoModeCommandLine
                                                   : qbs::CommandEchoModeSummary);
    m_qbsBuildOptions.setInstall(map.value(QLatin1String(QBS_INSTALL), true).toBool());
    m_qbsBuildOptions.setRemoveExistingInstallation(map.value(QLatin1String(QBS_CLEAN_INSTALL_ROOT))
                                                    .toBool());
    return true;
}

QVariantMap QbsBuildStep::toMap() const
{
    QVariantMap map = ProjectExplorer::BuildStep::toMap();
    map.insert(QLatin1String(QBS_CONFIG), m_qbsConfiguration);
    map.insert(QLatin1String(QBS_DRY_RUN), m_qbsBuildOptions.dryRun());
    map.insert(QLatin1String(QBS_KEEP_GOING), m_qbsBuildOptions.keepGoing());
    map.insert(QLatin1String(QBS_MAXJOBCOUNT), m_qbsBuildOptions.maxJobCount());
    map.insert(QLatin1String(QBS_SHOWCOMMANDLINES),
               m_qbsBuildOptions.echoMode() == qbs::CommandEchoModeCommandLine);
    map.insert(QLatin1String(QBS_INSTALL), m_qbsBuildOptions.install());
    map.insert(QLatin1String(QBS_CLEAN_INSTALL_ROOT),
               m_qbsBuildOptions.removeExistingInstallation());
    return map;
}

void QbsBuildStep::buildingDone(bool success)
{
    m_lastWasSuccess = success;
    // Report errors:
    foreach (const qbs::ErrorItem &item, m_job->error().items())
        createTaskAndOutput(ProjectExplorer::Task::Error, item.description(),
                            item.codeLocation().filePath(), item.codeLocation().line());

    QbsProject *pro = static_cast<QbsProject *>(project());

    // Building can uncover additional target artifacts.
    pro->updateAfterBuild();

    // The reparsing, if it is necessary, has to be done before finished() is emitted, as
    // otherwise a potential additional build step could conflict with the parsing step.
    if (pro->parsingScheduled())
        parseProject();
    else
        finish();
}

void QbsBuildStep::reparsingDone(bool success)
{
    disconnect(qbsProject(), SIGNAL(projectParsingDone(bool)), this, SLOT(reparsingDone(bool)));
    m_parsingProject = false;
    if (m_job) { // This was a scheduled reparsing after building.
        finish();
    } else if (!success) {
        m_lastWasSuccess = false;
        finish();
    } else {
        build();
    }
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

void QbsBuildStep::handleCommandDescriptionReport(const QString &highlight, const QString &message)
{
    Q_UNUSED(highlight);
    emit addOutput(message, NormalOutput);
}

void QbsBuildStep::handleProcessResultReport(const qbs::ProcessResult &result)
{
    bool hasOutput = !result.stdOut().isEmpty() || !result.stdErr().isEmpty();

    if (result.success() && !hasOutput)
        return;

    m_parser->setWorkingDirectory(result.workingDirectory());

    QString commandline = result.executableFilePath() + QLatin1Char(' ')
            + Utils::QtcProcess::joinArgs(result.arguments());
    addOutput(commandline, NormalOutput);

    foreach (const QString &line, result.stdErr()) {
        m_parser->stdError(line);
        addOutput(line, ErrorOutput);
    }
    foreach (const QString &line, result.stdOut()) {
        m_parser->stdOutput(line);
        addOutput(line, NormalOutput);
    }
    m_parser->flush();
}

void QbsBuildStep::createTaskAndOutput(ProjectExplorer::Task::TaskType type, const QString &message,
                                       const QString &file, int line)
{
    ProjectExplorer::Task task = ProjectExplorer::Task(type, message,
                                                       Utils::FileName::fromString(file), line,
                                                       ProjectExplorer::Constants::TASK_CATEGORY_COMPILE);
    emit addTask(task, 1);
    emit addOutput(message, NormalOutput);
}

QString QbsBuildStep::buildVariant() const
{
    return qbsConfiguration().value(QLatin1String(Constants::QBS_CONFIG_VARIANT_KEY)).toString();
}

bool QbsBuildStep::isQmlDebuggingEnabled() const
{
    QVariantMap data = qbsConfiguration();
    return data.value(QLatin1String(Constants::QBS_CONFIG_DECLARATIVE_DEBUG_KEY), false).toBool()
            || data.value(QLatin1String(Constants::QBS_CONFIG_QUICK_DEBUG_KEY), false).toBool();
}

void QbsBuildStep::setBuildVariant(const QString &variant)
{
    if (m_qbsConfiguration.value(QLatin1String(Constants::QBS_CONFIG_VARIANT_KEY)).toString() == variant)
        return;
    m_qbsConfiguration.insert(QLatin1String(Constants::QBS_CONFIG_VARIANT_KEY), variant);
    emit qbsConfigurationChanged();
    QbsBuildConfiguration *bc = static_cast<QbsBuildConfiguration *>(buildConfiguration());
    if (bc)
        bc->emitBuildTypeChanged();
}

QString QbsBuildStep::profile() const
{
    return qbsConfiguration().value(QLatin1String(Constants::QBS_CONFIG_PROFILE_KEY)).toString();
}

void QbsBuildStep::setDryRun(bool dr)
{
    if (m_qbsBuildOptions.dryRun() == dr)
        return;
    m_qbsBuildOptions.setDryRun(dr);
    emit qbsBuildOptionsChanged();
}

void QbsBuildStep::setKeepGoing(bool kg)
{
    if (m_qbsBuildOptions.keepGoing() == kg)
        return;
    m_qbsBuildOptions.setKeepGoing(kg);
    emit qbsBuildOptionsChanged();
}

void QbsBuildStep::setMaxJobs(int jobcount)
{
    if (m_qbsBuildOptions.maxJobCount() == jobcount)
        return;
    m_qbsBuildOptions.setMaxJobCount(jobcount);
    emit qbsBuildOptionsChanged();
}

void QbsBuildStep::setShowCommandLines(bool show)
{
    if (showCommandLines() == show)
        return;
    m_qbsBuildOptions.setEchoMode(show ? qbs::CommandEchoModeCommandLine
                                       : qbs::CommandEchoModeSummary);
    emit qbsBuildOptionsChanged();
}

void QbsBuildStep::setInstall(bool install)
{
    if (m_qbsBuildOptions.install() == install)
        return;
    m_qbsBuildOptions.setInstall(install);
    emit qbsBuildOptionsChanged();
}

void QbsBuildStep::setCleanInstallRoot(bool clean)
{
    if (m_qbsBuildOptions.removeExistingInstallation() == clean)
        return;
    m_qbsBuildOptions.setRemoveExistingInstallation(clean);
    emit qbsBuildOptionsChanged();
}

void QbsBuildStep::parseProject()
{
    m_parsingProject = true;
    connect(qbsProject(), SIGNAL(projectParsingDone(bool)), SLOT(reparsingDone(bool)));
    qbsProject()->parseCurrentBuildConfiguration();
}

void QbsBuildStep::build()
{
    qbs::BuildOptions options(m_qbsBuildOptions);
    options.setChangedFiles(m_changedFiles);
    options.setFilesToConsider(m_changedFiles);
    options.setActiveFileTags(m_activeFileTags);

    QString error;
    m_job = qbsProject()->build(options, m_products, error);
    if (!m_job) {
        emit addOutput(error, ErrorMessageOutput);
        m_fi->reportResult(false);
        emit finished();
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
    connect(m_job, SIGNAL(reportProcessResult(qbs::ProcessResult)),
            this, SLOT(handleProcessResultReport(qbs::ProcessResult)));

}

void QbsBuildStep::finish()
{
    QTC_ASSERT(m_fi, return);
    m_fi->reportResult(m_lastWasSuccess);
    m_fi = 0; // do not delete, it is not ours
    if (m_job) {
        m_job->deleteLater();
        m_job = 0;
    }

    emit finished();
}

QbsProject *QbsBuildStep::qbsProject() const
{
    return static_cast<QbsProject *>(project());
}

// --------------------------------------------------------------------
// QbsBuildStepConfigWidget:
// --------------------------------------------------------------------

QbsBuildStepConfigWidget::QbsBuildStepConfigWidget(QbsBuildStep *step) :
    m_step(step),
    m_ignoreChange(false)
{
    connect(m_step, SIGNAL(displayNameChanged()), this, SLOT(updateState()));
    connect(m_step, SIGNAL(qbsConfigurationChanged()), this, SLOT(updateState()));
    connect(m_step, SIGNAL(qbsBuildOptionsChanged()), this, SLOT(updateState()));

    setContentsMargins(0, 0, 0, 0);

    m_ui = new Ui::QbsBuildStepConfigWidget;
    m_ui->setupUi(this);

    m_ui->propertyEdit->setValidationFunction([this](Utils::FancyLineEdit *edit,
                                                     QString *errorMessage) {
        return validateProperties(edit, errorMessage);
    });
    m_ui->qmlDebuggingWarningText->setPixmap(Core::Icons::WARNING.pixmap());

    connect(m_ui->buildVariantComboBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(changeBuildVariant(int)));
    connect(m_ui->dryRunCheckBox, SIGNAL(toggled(bool)), this, SLOT(changeDryRun(bool)));
    connect(m_ui->keepGoingCheckBox, SIGNAL(toggled(bool)), this, SLOT(changeKeepGoing(bool)));
    connect(m_ui->jobSpinBox, SIGNAL(valueChanged(int)), this, SLOT(changeJobCount(int)));
    connect(m_ui->showCommandLinesCheckBox, &QCheckBox::toggled, this,
            &QbsBuildStepConfigWidget::changeShowCommandLines);
    connect(m_ui->installCheckBox, &QCheckBox::toggled, this,
            &QbsBuildStepConfigWidget::changeInstall);
    connect(m_ui->cleanInstallRootCheckBox, &QCheckBox::toggled, this,
            &QbsBuildStepConfigWidget::changeCleanInstallRoot);
    connect(m_ui->qmlDebuggingLibraryCheckBox, SIGNAL(toggled(bool)),
            this, SLOT(linkQmlDebuggingLibraryChecked(bool)));
    connect(QtSupport::QtVersionManager::instance(), SIGNAL(dumpUpdatedFor(Utils::FileName)),
            this, SLOT(updateQmlDebuggingOption()));
    updateState();
}

QbsBuildStepConfigWidget::~QbsBuildStepConfigWidget()
{
    delete m_ui;
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
    if (!m_ignoreChange) {
        m_ui->dryRunCheckBox->setChecked(m_step->dryRun());
        m_ui->keepGoingCheckBox->setChecked(m_step->keepGoing());
        m_ui->jobSpinBox->setValue(m_step->maxJobs());
        m_ui->showCommandLinesCheckBox->setChecked(m_step->showCommandLines());
        m_ui->installCheckBox->setChecked(m_step->install());
        m_ui->cleanInstallRootCheckBox->setChecked(m_step->cleanInstallRoot());
        updatePropertyEdit(m_step->qbsConfiguration());
        m_ui->qmlDebuggingLibraryCheckBox->setChecked(m_step->isQmlDebuggingEnabled());
    }

    updateQmlDebuggingOption();

    const QString buildVariant = m_step->buildVariant();
    const int idx = (buildVariant == QLatin1String(Constants::QBS_VARIANT_DEBUG)) ? 0 : 1;
    m_ui->buildVariantComboBox->setCurrentIndex(idx);
    QString command = QbsBuildConfiguration::equivalentCommandLine(m_step);

    for (int i = 0; i < m_propertyCache.count(); ++i) {
        command += QLatin1Char(' ') + m_propertyCache.at(i).first
                + QLatin1Char(':') + m_propertyCache.at(i).second;
    }

    if (m_step->isQmlDebuggingEnabled())
        command += QLatin1String(" Qt.declarative.qmlDebugging:true Qt.quick.qmlDebugging:true");
    m_ui->commandLineTextEdit->setPlainText(command);

    QString summary = tr("<b>Qbs:</b> %1").arg(command);
    if (m_summary != summary) {
        m_summary = summary;
        emit updateSummary();
    }
}

void QbsBuildStepConfigWidget::updateQmlDebuggingOption()
{
    QString warningText;
    bool supported = QtSupport::BaseQtVersion::isQmlDebuggingSupported(m_step->target()->kit(),
                                                                       &warningText);
    m_ui->qmlDebuggingLibraryCheckBox->setEnabled(supported);

    if (supported && m_step->isQmlDebuggingEnabled())
        warningText = tr("Might make your application vulnerable. Only use in a safe environment.");

    m_ui->qmlDebuggingWarningText->setText(warningText);
    m_ui->qmlDebuggingWarningIcon->setVisible(!warningText.isEmpty());
}


void QbsBuildStepConfigWidget::updatePropertyEdit(const QVariantMap &data)
{
    QVariantMap editable = data;

    // remove data that is edited with special UIs:
    editable.remove(QLatin1String(Constants::QBS_CONFIG_PROFILE_KEY));
    editable.remove(QLatin1String(Constants::QBS_CONFIG_VARIANT_KEY));
    editable.remove(QLatin1String(Constants::QBS_CONFIG_DECLARATIVE_DEBUG_KEY));
    editable.remove(QLatin1String(Constants::QBS_CONFIG_QUICK_DEBUG_KEY));

    QStringList propertyList;
    for (QVariantMap::const_iterator i = editable.constBegin(); i != editable.constEnd(); ++i)
        propertyList.append(i.key() + QLatin1Char(':') + i.value().toString());

    m_ui->propertyEdit->setText(Utils::QtcProcess::joinArgs(propertyList));
}

void QbsBuildStepConfigWidget::changeBuildVariant(int idx)
{
    QString variant;
    if (idx == 1)
        variant = QLatin1String(Constants::QBS_VARIANT_RELEASE);
    else
        variant = QLatin1String(Constants::QBS_VARIANT_DEBUG);
    m_ignoreChange = true;
    m_step->setBuildVariant(variant);
    m_ignoreChange = false;
}

void QbsBuildStepConfigWidget::changeDryRun(bool dr)
{
    m_ignoreChange = true;
    m_step->setDryRun(dr);
    m_ignoreChange = false;
}

void QbsBuildStepConfigWidget::changeShowCommandLines(bool show)
{
    m_ignoreChange = true;
    m_step->setShowCommandLines(show);
    m_ignoreChange = false;
}

void QbsBuildStepConfigWidget::changeKeepGoing(bool kg)
{
    m_ignoreChange = true;
    m_step->setKeepGoing(kg);
    m_ignoreChange = false;
}

void QbsBuildStepConfigWidget::changeJobCount(int count)
{
    m_ignoreChange = true;
    m_step->setMaxJobs(count);
    m_ignoreChange = false;
}

void QbsBuildStepConfigWidget::changeInstall(bool install)
{
    m_ignoreChange = true;
    m_step->setInstall(install);
    m_ignoreChange = false;
}

void QbsBuildStepConfigWidget::changeCleanInstallRoot(bool clean)
{
    m_ignoreChange = true;
    m_step->setCleanInstallRoot(clean);
    m_ignoreChange = false;
}

void QbsBuildStepConfigWidget::applyCachedProperties()
{
    QVariantMap data;
    QVariantMap tmp = m_step->qbsConfiguration();

    // Insert values set up with special UIs:
    data.insert(QLatin1String(Constants::QBS_CONFIG_PROFILE_KEY),
                tmp.value(QLatin1String(Constants::QBS_CONFIG_PROFILE_KEY)));
    data.insert(QLatin1String(Constants::QBS_CONFIG_VARIANT_KEY),
                tmp.value(QLatin1String(Constants::QBS_CONFIG_VARIANT_KEY)));
    if (tmp.contains(QLatin1String(Constants::QBS_CONFIG_DECLARATIVE_DEBUG_KEY)))
        data.insert(QLatin1String(Constants::QBS_CONFIG_DECLARATIVE_DEBUG_KEY),
                    tmp.value(QLatin1String(Constants::QBS_CONFIG_DECLARATIVE_DEBUG_KEY)));
    if (tmp.contains(QLatin1String(Constants::QBS_CONFIG_QUICK_DEBUG_KEY)))
        data.insert(QLatin1String(Constants::QBS_CONFIG_QUICK_DEBUG_KEY),
                    tmp.value(QLatin1String(Constants::QBS_CONFIG_QUICK_DEBUG_KEY)));

    for (int i = 0; i < m_propertyCache.count(); ++i)
        data.insert(m_propertyCache.at(i).first, m_propertyCache.at(i).second);

    m_ignoreChange = true;
    m_step->setQbsConfiguration(data);
    m_ignoreChange = false;
}

void QbsBuildStepConfigWidget::linkQmlDebuggingLibraryChecked(bool checked)
{
    QVariantMap data = m_step->qbsConfiguration();
    if (checked) {
        data.insert(QLatin1String(Constants::QBS_CONFIG_DECLARATIVE_DEBUG_KEY), checked);
        data.insert(QLatin1String(Constants::QBS_CONFIG_QUICK_DEBUG_KEY), checked);
    } else {
        data.remove(QLatin1String(Constants::QBS_CONFIG_DECLARATIVE_DEBUG_KEY));
        data.remove(QLatin1String(Constants::QBS_CONFIG_QUICK_DEBUG_KEY));
    }

    m_ignoreChange = true;
    m_step->setQbsConfiguration(data);
    m_ignoreChange = false;
}

bool QbsBuildStepConfigWidget::validateProperties(Utils::FancyLineEdit *edit, QString *errorMessage)
{
    Utils::QtcProcess::SplitError err;
    QStringList argList = Utils::QtcProcess::splitArgs(edit->text(), Utils::HostOsInfo::hostOs(),
                                                       false, &err);
    if (err != Utils::QtcProcess::SplitOk) {
        if (errorMessage)
            *errorMessage = tr("Could not split properties.");
        return false;
    }

    QList<QPair<QString, QString> > properties;
    foreach (const QString &arg, argList) {
        int pos = arg.indexOf(QLatin1Char(':'));
        QString key;
        QString value;
        if (pos > 0) {
            key = arg.left(pos);
            value = arg.mid(pos + 1);
            properties.append(qMakePair(key, value));
        } else {
            if (errorMessage)
                *errorMessage = tr("No \":\" found in property definition.");
            return false;
        }
    }

    if (m_propertyCache != properties) {
        m_propertyCache = properties;
        applyCachedProperties();
    }
    return true;
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
            && qobject_cast<QbsBuildConfiguration *>(parent->parent())
            && qobject_cast<QbsProject *>(parent->target()->project()))
        return QList<Core::Id>() << Core::Id(Constants::QBS_BUILDSTEP_ID);
    return QList<Core::Id>();
}

QString QbsBuildStepFactory::displayNameForId(Core::Id id) const
{
    if (id == Core::Id(Constants::QBS_BUILDSTEP_ID))
        return tr("Qbs Build");
    return QString();
}

bool QbsBuildStepFactory::canCreate(ProjectExplorer::BuildStepList *parent, Core::Id id) const
{
    if (parent->id() != Core::Id(ProjectExplorer::Constants::BUILDSTEPS_BUILD)
            || !qobject_cast<QbsBuildConfiguration *>(parent->parent())
            || !qobject_cast<QbsProject *>(parent->target()->project()))
        return false;
    return id == Core::Id(Constants::QBS_BUILDSTEP_ID);
}

ProjectExplorer::BuildStep *QbsBuildStepFactory::create(ProjectExplorer::BuildStepList *parent, Core::Id id)
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
