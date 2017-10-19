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

#include "qbsbuildstep.h"

#include "qbsbuildconfiguration.h"
#include "qbsparser.h"
#include "qbsproject.h"
#include "qbsprojectmanagerconstants.h"
#include "qbsprojectmanagersettings.h"

#include "ui_qbsbuildstepconfigwidget.h"

#include <coreplugin/icore.h>
#include <coreplugin/variablechooser.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <qtsupport/qtversionmanager.h>
#include <utils/macroexpander.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/utilsicons.h>

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

class QbsBuildStepConfigWidget : public ProjectExplorer::BuildStepConfigWidget
{
    Q_OBJECT
public:
    QbsBuildStepConfigWidget(QbsBuildStep *step);
    ~QbsBuildStepConfigWidget();
    QString summaryText() const;
    QString displayName() const;

private:
    void updateState();
    void updateQmlDebuggingOption();
    void updatePropertyEdit(const QVariantMap &data);

    void changeBuildVariant(int);
    void changeShowCommandLines(bool show);
    void changeKeepGoing(bool kg);
    void changeJobCount(int count);
    void changeInstall(bool install);
    void changeCleanInstallRoot(bool clean);
    void changeUseDefaultInstallDir(bool useDefault);
    void changeInstallDir(const QString &dir);
    void changeForceProbes(bool forceProbes);
    void applyCachedProperties();

    // QML debugging:
    void linkQmlDebuggingLibraryChecked(bool checked);

    bool validateProperties(Utils::FancyLineEdit *edit, QString *errorMessage);

    Ui::QbsBuildStepConfigWidget *m_ui;

    class Property
    {
    public:
        Property() = default;
        Property(const QString &n, const QString &v, const QString &e) :
            name(n), value(v), effectiveValue(e)
        {}
        bool operator==(const Property &other) const
        {
            return name == other.name
                    && value == other.value
                    && effectiveValue == other.effectiveValue;
        }

        QString name;
        QString value;
        QString effectiveValue;
    };

    QList<Property> m_propertyCache;
    QbsBuildStep *m_step;
    QString m_summary;
    bool m_ignoreChange;
};

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
    setQbsConfiguration(other->qbsConfiguration(PreserveVariables));
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

    connect(m_parser, &ProjectExplorer::IOutputParser::addOutput,
            this, [this](const QString &string, ProjectExplorer::BuildStep::OutputFormat format) {
        emit addOutput(string, format);
    });
    connect(m_parser, &ProjectExplorer::IOutputParser::addTask, this, &QbsBuildStep::addTask);

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

QVariantMap QbsBuildStep::qbsConfiguration(VariableHandling variableHandling) const
{
    QVariantMap config = m_qbsConfiguration;
    config.insert(Constants::QBS_FORCE_PROBES_KEY, m_forceProbes);
    if (variableHandling == ExpandVariables) {
        const Utils::MacroExpander *expander = Utils::globalMacroExpander();
        for (auto it = config.begin(), end = config.end(); it != end; ++it) {
            const QString rawString = it.value().toString();
            const QString expandedString = expander->expand(rawString);
            it.value() = qbs::representationToSettingsValue(expandedString);
        }
    }
    return config;
}

void QbsBuildStep::setQbsConfiguration(const QVariantMap &config)
{
    QbsProject *pro = static_cast<QbsProject *>(project());

    QVariantMap tmp = config;
    tmp.insert(Constants::QBS_CONFIG_PROFILE_KEY, pro->profileForTarget(target()));
    if (!tmp.contains(Constants::QBS_CONFIG_VARIANT_KEY))
        tmp.insert(Constants::QBS_CONFIG_VARIANT_KEY,
                   QString::fromLatin1(Constants::QBS_VARIANT_DEBUG));

    if (tmp == m_qbsConfiguration)
        return;
    m_qbsConfiguration = tmp;
    QbsBuildConfiguration *bc = static_cast<QbsBuildConfiguration *>(buildConfiguration());
    if (bc)
        bc->emitBuildTypeChanged();
    emit qbsConfigurationChanged();
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

bool QbsBuildStep::hasCustomInstallRoot() const
{
    return m_qbsConfiguration.contains(Constants::QBS_INSTALL_ROOT_KEY);
}

Utils::FileName QbsBuildStep::installRoot() const
{
    Utils::FileName root = Utils::FileName::fromString(m_qbsConfiguration
            .value(Constants::QBS_INSTALL_ROOT_KEY).toString());
    if (root.isNull()) {
        const QbsBuildConfiguration * const bc
                = static_cast<QbsBuildConfiguration *>(buildConfiguration());
        root = bc->buildDirectory().appendPath(bc->configurationName())
                .appendPath(qbs::InstallOptions::defaultInstallRoot());
    }
    return root;
}

int QbsBuildStep::maxJobs() const
{
    if (m_qbsBuildOptions.maxJobCount() > 0)
        return m_qbsBuildOptions.maxJobCount();
    return qbs::BuildOptions::defaultMaxJobCount();
}

static QString forceProbesKey() { return QLatin1String("Qbs.forceProbesKey"); }

bool QbsBuildStep::fromMap(const QVariantMap &map)
{
    if (!ProjectExplorer::BuildStep::fromMap(map))
        return false;

    setQbsConfiguration(map.value(QBS_CONFIG).toMap());
    m_qbsBuildOptions.setDryRun(map.value(QBS_DRY_RUN).toBool());
    m_qbsBuildOptions.setKeepGoing(map.value(QBS_KEEP_GOING).toBool());
    m_qbsBuildOptions.setMaxJobCount(map.value(QBS_MAXJOBCOUNT).toInt());
    const bool showCommandLines = map.value(QBS_SHOWCOMMANDLINES).toBool();
    m_qbsBuildOptions.setEchoMode(showCommandLines ? qbs::CommandEchoModeCommandLine
                                                   : qbs::CommandEchoModeSummary);
    m_qbsBuildOptions.setInstall(map.value(QBS_INSTALL, true).toBool());
    m_qbsBuildOptions.setRemoveExistingInstallation(map.value(QBS_CLEAN_INSTALL_ROOT)
                                                    .toBool());
    m_forceProbes = map.value(forceProbesKey()).toBool();
    return true;
}

QVariantMap QbsBuildStep::toMap() const
{
    QVariantMap map = ProjectExplorer::BuildStep::toMap();
    map.insert(QBS_CONFIG, m_qbsConfiguration);
    map.insert(QBS_DRY_RUN, m_qbsBuildOptions.dryRun());
    map.insert(QBS_KEEP_GOING, m_qbsBuildOptions.keepGoing());
    map.insert(QBS_MAXJOBCOUNT, m_qbsBuildOptions.maxJobCount());
    map.insert(QBS_SHOWCOMMANDLINES,
               m_qbsBuildOptions.echoMode() == qbs::CommandEchoModeCommandLine);
    map.insert(QBS_INSTALL, m_qbsBuildOptions.install());
    map.insert(QBS_CLEAN_INSTALL_ROOT,
               m_qbsBuildOptions.removeExistingInstallation());
    map.insert(forceProbesKey(), m_forceProbes);
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
    disconnect(qbsProject(), &ProjectExplorer::Project::parsingFinished,
               this, &QbsBuildStep::reparsingDone);
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
    emit addOutput(message, OutputFormat::Stdout);
}

void QbsBuildStep::handleProcessResultReport(const qbs::ProcessResult &result)
{
    bool hasOutput = !result.stdOut().isEmpty() || !result.stdErr().isEmpty();

    if (result.success() && !hasOutput)
        return;

    m_parser->setWorkingDirectory(result.workingDirectory());

    QString commandline = result.executableFilePath() + ' '
            + Utils::QtcProcess::joinArgs(result.arguments());
    emit addOutput(commandline, OutputFormat::Stdout);

    foreach (const QString &line, result.stdErr()) {
        m_parser->stdError(line);
        emit addOutput(line, OutputFormat::Stderr);
    }
    foreach (const QString &line, result.stdOut()) {
        m_parser->stdOutput(line);
        emit addOutput(line, OutputFormat::Stdout);
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
    emit addOutput(message, OutputFormat::Stdout);
}

QString QbsBuildStep::buildVariant() const
{
    return qbsConfiguration(PreserveVariables).value(Constants::QBS_CONFIG_VARIANT_KEY).toString();
}

bool QbsBuildStep::isQmlDebuggingEnabled() const
{
    QVariantMap data = qbsConfiguration(PreserveVariables);
    return data.value(Constants::QBS_CONFIG_DECLARATIVE_DEBUG_KEY, false).toBool()
            || data.value(Constants::QBS_CONFIG_QUICK_DEBUG_KEY, false).toBool();
}

void QbsBuildStep::setBuildVariant(const QString &variant)
{
    if (m_qbsConfiguration.value(Constants::QBS_CONFIG_VARIANT_KEY).toString() == variant)
        return;
    m_qbsConfiguration.insert(Constants::QBS_CONFIG_VARIANT_KEY, variant);
    emit qbsConfigurationChanged();
    QbsBuildConfiguration *bc = static_cast<QbsBuildConfiguration *>(buildConfiguration());
    if (bc)
        bc->emitBuildTypeChanged();
}

QString QbsBuildStep::profile() const
{
    return qbsConfiguration(PreserveVariables).value(Constants::QBS_CONFIG_PROFILE_KEY).toString();
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
    connect(qbsProject(), &ProjectExplorer::Project::parsingFinished,
            this, &QbsBuildStep::reparsingDone);
    qbsProject()->parseCurrentBuildConfiguration();
}

void QbsBuildStep::build()
{
    qbs::BuildOptions options(m_qbsBuildOptions);
    options.setChangedFiles(m_changedFiles);
    options.setFilesToConsider(m_changedFiles);
    options.setActiveFileTags(m_activeFileTags);
    options.setLogElapsedTime(!qEnvironmentVariableIsEmpty(Constants::QBS_PROFILING_ENV));

    QString error;
    m_job = qbsProject()->build(options, m_products, error);
    if (!m_job) {
        emit addOutput(error, OutputFormat::ErrorMessage);
        reportRunResult(*m_fi, false);
        return;
    }

    m_progressBase = 0;

    connect(m_job, &qbs::AbstractJob::finished, this, &QbsBuildStep::buildingDone);
    connect(m_job, &qbs::AbstractJob::taskStarted,
            this, &QbsBuildStep::handleTaskStarted);
    connect(m_job, &qbs::AbstractJob::taskProgress,
            this, &QbsBuildStep::handleProgress);
    connect(m_job, &qbs::BuildJob::reportCommandDescription,
            this, &QbsBuildStep::handleCommandDescriptionReport);
    connect(m_job, &qbs::BuildJob::reportProcessResult,
            this, &QbsBuildStep::handleProcessResultReport);

}

void QbsBuildStep::finish()
{
    QTC_ASSERT(m_fi, return);
    reportRunResult(*m_fi, m_lastWasSuccess);
    m_fi = 0; // do not delete, it is not ours
    if (m_job) {
        m_job->deleteLater();
        m_job = 0;
    }
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
    connect(m_step, &ProjectExplorer::ProjectConfiguration::displayNameChanged,
            this, &QbsBuildStepConfigWidget::updateState);
    connect(m_step, &QbsBuildStep::qbsConfigurationChanged,
            this, &QbsBuildStepConfigWidget::updateState);
    connect(m_step, &QbsBuildStep::qbsBuildOptionsChanged,
            this, &QbsBuildStepConfigWidget::updateState);
    connect(&QbsProjectManagerSettings::instance(), &QbsProjectManagerSettings::settingsBaseChanged,
            this, &QbsBuildStepConfigWidget::updateState);
    step->target()->subscribeSignal(&ProjectExplorer::BuildConfiguration::buildDirectoryChanged,
                                    this, [this]() {
        if (m_step->target()->activeBuildConfiguration() == sender())
            updateState();
    });

    setContentsMargins(0, 0, 0, 0);

    m_ui = new Ui::QbsBuildStepConfigWidget;
    m_ui->setupUi(this);
    m_ui->installDirChooser->setExpectedKind(Utils::PathChooser::Directory);

    auto chooser = new Core::VariableChooser(this);
    chooser->addSupportedWidget(m_ui->propertyEdit);
    m_ui->propertyEdit->setValidationFunction([this](Utils::FancyLineEdit *edit,
                                                     QString *errorMessage) {
        return validateProperties(edit, errorMessage);
    });
    m_ui->qmlDebuggingWarningText->setPixmap(Utils::Icons::WARNING.pixmap());

    connect(m_ui->buildVariantComboBox,
            static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &QbsBuildStepConfigWidget::changeBuildVariant);
    connect(m_ui->keepGoingCheckBox, &QAbstractButton::toggled,
            this, &QbsBuildStepConfigWidget::changeKeepGoing);
    connect(m_ui->jobSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &QbsBuildStepConfigWidget::changeJobCount);
    connect(m_ui->showCommandLinesCheckBox, &QCheckBox::toggled, this,
            &QbsBuildStepConfigWidget::changeShowCommandLines);
    connect(m_ui->installCheckBox, &QCheckBox::toggled, this,
            &QbsBuildStepConfigWidget::changeInstall);
    connect(m_ui->cleanInstallRootCheckBox, &QCheckBox::toggled, this,
            &QbsBuildStepConfigWidget::changeCleanInstallRoot);
    connect(m_ui->defaultInstallDirCheckBox, &QCheckBox::toggled, this,
            &QbsBuildStepConfigWidget::changeUseDefaultInstallDir);
    connect(m_ui->installDirChooser, &Utils::PathChooser::rawPathChanged, this,
            &QbsBuildStepConfigWidget::changeInstallDir);
    connect(m_ui->forceProbesCheckBox, &QCheckBox::toggled, this,
            &QbsBuildStepConfigWidget::changeForceProbes);
    connect(m_ui->qmlDebuggingLibraryCheckBox, &QAbstractButton::toggled,
            this, &QbsBuildStepConfigWidget::linkQmlDebuggingLibraryChecked);
    connect(QtSupport::QtVersionManager::instance(), &QtSupport::QtVersionManager::dumpUpdatedFor,
            this, &QbsBuildStepConfigWidget::updateQmlDebuggingOption);
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
        m_ui->keepGoingCheckBox->setChecked(m_step->keepGoing());
        m_ui->jobSpinBox->setValue(m_step->maxJobs());
        m_ui->showCommandLinesCheckBox->setChecked(m_step->showCommandLines());
        m_ui->installCheckBox->setChecked(m_step->install());
        m_ui->cleanInstallRootCheckBox->setChecked(m_step->cleanInstallRoot());
        m_ui->forceProbesCheckBox->setChecked(m_step->forceProbes());
        updatePropertyEdit(m_step->qbsConfiguration(QbsBuildStep::PreserveVariables));
        m_ui->qmlDebuggingLibraryCheckBox->setChecked(m_step->isQmlDebuggingEnabled());
        m_ui->installDirChooser->setFileName(m_step->installRoot());
        m_ui->defaultInstallDirCheckBox->setChecked(!m_step->hasCustomInstallRoot());
    }

    updateQmlDebuggingOption();

    const QString buildVariant = m_step->buildVariant();
    const int idx = (buildVariant == Constants::QBS_VARIANT_DEBUG) ? 0 : 1;
    m_ui->buildVariantComboBox->setCurrentIndex(idx);
    QString command = static_cast<QbsBuildConfiguration *>(m_step->buildConfiguration())
            ->equivalentCommandLine(m_step);

    for (int i = 0; i < m_propertyCache.count(); ++i) {
        command += ' ' + m_propertyCache.at(i).name + ':' + m_propertyCache.at(i).effectiveValue;
    }

    if (m_step->isQmlDebuggingEnabled())
        command += " Qt.declarative.qmlDebugging:true Qt.quick.qmlDebugging:true";
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
    editable.remove(Constants::QBS_CONFIG_PROFILE_KEY);
    editable.remove(Constants::QBS_CONFIG_VARIANT_KEY);
    editable.remove(Constants::QBS_CONFIG_DECLARATIVE_DEBUG_KEY);
    editable.remove(Constants::QBS_CONFIG_QUICK_DEBUG_KEY);
    editable.remove(Constants::QBS_FORCE_PROBES_KEY);
    editable.remove(Constants::QBS_INSTALL_ROOT_KEY);

    QStringList propertyList;
    for (QVariantMap::const_iterator i = editable.constBegin(); i != editable.constEnd(); ++i)
        propertyList.append(i.key() + ':' + i.value().toString());

    m_ui->propertyEdit->setText(Utils::QtcProcess::joinArgs(propertyList));
}

void QbsBuildStepConfigWidget::changeBuildVariant(int idx)
{
    QString variant;
    if (idx == 1)
        variant = Constants::QBS_VARIANT_RELEASE;
    else
        variant = Constants::QBS_VARIANT_DEBUG;
    m_ignoreChange = true;
    m_step->setBuildVariant(variant);
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

void QbsBuildStepConfigWidget::changeUseDefaultInstallDir(bool useDefault)
{
    m_ignoreChange = true;
    QVariantMap config = m_step->qbsConfiguration(QbsBuildStep::PreserveVariables);
    m_ui->installDirChooser->setEnabled(!useDefault);
    if (useDefault)
        config.remove(Constants::QBS_INSTALL_ROOT_KEY);
    else
        config.insert(Constants::QBS_INSTALL_ROOT_KEY, m_ui->installDirChooser->rawPath());
    m_step->setQbsConfiguration(config);
    m_ignoreChange = false;
}

void QbsBuildStepConfigWidget::changeInstallDir(const QString &dir)
{
    if (!m_step->hasCustomInstallRoot())
        return;
    m_ignoreChange = true;
    QVariantMap config = m_step->qbsConfiguration(QbsBuildStep::PreserveVariables);
    config.insert(Constants::QBS_INSTALL_ROOT_KEY, dir);
    m_step->setQbsConfiguration(config);
    m_ignoreChange = false;
}

void QbsBuildStepConfigWidget::changeForceProbes(bool forceProbes)
{
    m_ignoreChange = true;
    m_step->setForceProbes(forceProbes);
    m_ignoreChange = false;
}

void QbsBuildStepConfigWidget::applyCachedProperties()
{
    QVariantMap data;
    const QVariantMap tmp = m_step->qbsConfiguration(QbsBuildStep::PreserveVariables);

    // Insert values set up with special UIs:
    data.insert(Constants::QBS_CONFIG_PROFILE_KEY,
                tmp.value(Constants::QBS_CONFIG_PROFILE_KEY));
    data.insert(Constants::QBS_CONFIG_VARIANT_KEY,
                tmp.value(Constants::QBS_CONFIG_VARIANT_KEY));
    const QStringList additionalSpecialKeys({Constants::QBS_CONFIG_DECLARATIVE_DEBUG_KEY,
                                             Constants::QBS_CONFIG_QUICK_DEBUG_KEY,
                                             Constants::QBS_INSTALL_ROOT_KEY});
    for (const QString &key : additionalSpecialKeys) {
        const auto it = tmp.constFind(key);
        if (it != tmp.cend())
            data.insert(key, it.value());
    }

    for (int i = 0; i < m_propertyCache.count(); ++i) {
        const Property &property = m_propertyCache.at(i);
        data.insert(property.name, property.value);
    }

    m_ignoreChange = true;
    m_step->setQbsConfiguration(data);
    m_ignoreChange = false;
}

void QbsBuildStepConfigWidget::linkQmlDebuggingLibraryChecked(bool checked)
{
    QVariantMap data = m_step->qbsConfiguration(QbsBuildStep::PreserveVariables);
    if (checked) {
        data.insert(Constants::QBS_CONFIG_DECLARATIVE_DEBUG_KEY, checked);
        data.insert(Constants::QBS_CONFIG_QUICK_DEBUG_KEY, checked);
    } else {
        data.remove(Constants::QBS_CONFIG_DECLARATIVE_DEBUG_KEY);
        data.remove(Constants::QBS_CONFIG_QUICK_DEBUG_KEY);
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

    QList<Property> properties;
    const Utils::MacroExpander *expander = Utils::globalMacroExpander();
    foreach (const QString &rawArg, argList) {
        int pos = rawArg.indexOf(':');
        if (pos > 0) {
            const QString rawValue = rawArg.mid(pos + 1);
            Property property(rawArg.left(pos), rawValue, expander->expand(rawValue));
            properties.append(property);
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

QList<ProjectExplorer::BuildStepInfo> QbsBuildStepFactory::availableSteps(ProjectExplorer::BuildStepList *parent) const
{
    if (parent->id() == ProjectExplorer::Constants::BUILDSTEPS_BUILD
            && qobject_cast<QbsBuildConfiguration *>(parent->parent())
            && qobject_cast<QbsProject *>(parent->target()->project()))
       return {{Constants::QBS_BUILDSTEP_ID, tr("Qbs Build")}};

    return {};
}

ProjectExplorer::BuildStep *QbsBuildStepFactory::create(ProjectExplorer::BuildStepList *parent, Core::Id id)
{
    Q_UNUSED(id);
    return new QbsBuildStep(parent);
}

ProjectExplorer::BuildStep *QbsBuildStepFactory::clone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *product)
{
    return new QbsBuildStep(parent, static_cast<QbsBuildStep *>(product));
}

} // namespace Internal
} // namespace QbsProjectManager

#include "qbsbuildstep.moc"
