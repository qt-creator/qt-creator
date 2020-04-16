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
#include "qbsproject.h"
#include "qbsprojectmanagerconstants.h"
#include "qbssession.h"
#include "qbssettings.h"

#include <coreplugin/icore.h>
#include <coreplugin/variablechooser.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <qtsupport/qtversionmanager.h>
#include <utils/macroexpander.h>
#include <utils/outputformatter.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QBoxLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QPlainTextEdit>
#include <QSpinBox>
#include <QThread>

// --------------------------------------------------------------------
// Constants:
// --------------------------------------------------------------------

const char QBS_CONFIG[] = "Qbs.Configuration";
const char QBS_KEEP_GOING[] = "Qbs.DryKeepGoing";
const char QBS_MAXJOBCOUNT[] = "Qbs.MaxJobs";
const char QBS_SHOWCOMMANDLINES[] = "Qbs.ShowCommandLines";
const char QBS_INSTALL[] = "Qbs.Install";
const char QBS_CLEAN_INSTALL_ROOT[] = "Qbs.CleanInstallRoot";

using namespace ProjectExplorer;
using namespace Utils;

namespace QbsProjectManager {
namespace Internal {

class QbsBuildStepConfigWidget : public ProjectExplorer::BuildStepConfigWidget
{
    Q_OBJECT
public:
    QbsBuildStepConfigWidget(QbsBuildStep *step);

private:
    void updateState();
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

    QbsBuildStep *qbsStep() const;

    bool validateProperties(Utils::FancyLineEdit *edit, QString *errorMessage);

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
    bool m_ignoreChange = false;

    QComboBox *buildVariantComboBox;
    QSpinBox *jobSpinBox;
    FancyLineEdit *propertyEdit;
    PathChooser *installDirChooser;
    QCheckBox *keepGoingCheckBox;
    QCheckBox *showCommandLinesCheckBox;
    QCheckBox *forceProbesCheckBox;
    QCheckBox *installCheckBox;
    QCheckBox *cleanInstallRootCheckBox;
    QCheckBox *defaultInstallDirCheckBox;
    QPlainTextEdit *commandLineTextEdit;
};

// --------------------------------------------------------------------
// QbsBuildStep:
// --------------------------------------------------------------------

QbsBuildStep::QbsBuildStep(BuildStepList *bsl, Core::Id id) :
    BuildStep(bsl, id)
{
    setDisplayName(tr("Qbs Build"));
    setQbsConfiguration(QVariantMap());

    auto qbsBuildConfig = qobject_cast<QbsBuildConfiguration *>(buildConfiguration());
    QTC_CHECK(qbsBuildConfig);
    connect(this, &QbsBuildStep::qbsConfigurationChanged,
            qbsBuildConfig, &QbsBuildConfiguration::qbsConfigurationChanged);

//    setQbsConfiguration(other->qbsConfiguration(PreserveVariables));
}

QbsBuildStep::~QbsBuildStep()
{
    doCancel();
    if (m_session)
        m_session->disconnect(this);
}

bool QbsBuildStep::init()
{
    if (qbsBuildSystem()->isParsing() || m_session)
        return false;

    auto bc = static_cast<QbsBuildConfiguration *>(buildConfiguration());

    if (!bc)
        return false;

    m_changedFiles = bc->changedFiles();
    m_activeFileTags = bc->activeFileTags();
    m_products = bc->products();

    return true;
}

void QbsBuildStep::setupOutputFormatter(OutputFormatter *formatter)
{
    formatter->addLineParsers(target()->kit()->createOutputParsers());
    BuildStep::setupOutputFormatter(formatter);
}

void QbsBuildStep::doRun()
{
    // We need a pre-build parsing step in order not to lose project file changes done
    // right before building (but before the delay has elapsed).
    m_parsingAfterBuild = false;
    parseProject();
}

ProjectExplorer::BuildStepConfigWidget *QbsBuildStep::createConfigWidget()
{
    return new QbsBuildStepConfigWidget(this);
}

void QbsBuildStep::doCancel()
{
    if (m_parsingProject)
        qbsBuildSystem()->cancelParsing();
    else if (m_session)
        m_session->cancelCurrentJob();
}

QVariantMap QbsBuildStep::qbsConfiguration(VariableHandling variableHandling) const
{
    QVariantMap config = m_qbsConfiguration;
    const auto qbsBuildConfig = static_cast<QbsBuildConfiguration *>(buildConfiguration());
    config.insert(Constants::QBS_FORCE_PROBES_KEY, m_forceProbes);

    const auto store = [&config](TriState ts, const QString &key) {
        if (ts == TriState::Enabled)
            config.insert(key, true);
        else if (ts == TriState::Disabled)
            config.insert(key, false);
        else
            config.remove(key);
    };

    store(qbsBuildConfig->separateDebugInfoSetting(),
          Constants::QBS_CONFIG_SEPARATE_DEBUG_INFO_KEY);

    store(qbsBuildConfig->qmlDebuggingSetting(),
          Constants::QBS_CONFIG_QUICK_DEBUG_KEY);

    store(qbsBuildConfig->qtQuickCompilerSetting(),
          Constants::QBS_CONFIG_QUICK_COMPILER_KEY);

    if (variableHandling == ExpandVariables) {
        const MacroExpander * const expander = macroExpander();
        for (auto it = config.begin(), end = config.end(); it != end; ++it) {
            const QString rawString = it.value().toString();
            const QString expandedString = expander->expand(rawString);
            it.value() = expandedString;
        }
    }
    return config;
}

void QbsBuildStep::setQbsConfiguration(const QVariantMap &config)
{
    QVariantMap tmp = config;
    tmp.insert(Constants::QBS_CONFIG_PROFILE_KEY, qbsBuildSystem()->profile());
    if (!tmp.contains(Constants::QBS_CONFIG_VARIANT_KEY))
        tmp.insert(Constants::QBS_CONFIG_VARIANT_KEY,
                   QString::fromLatin1(Constants::QBS_VARIANT_DEBUG));

    if (tmp == m_qbsConfiguration)
        return;
    m_qbsConfiguration = tmp;
    if (ProjectExplorer::BuildConfiguration *bc = buildConfiguration())
        emit bc->buildTypeChanged();
    emit qbsConfigurationChanged();
}

bool QbsBuildStep::hasCustomInstallRoot() const
{
    return m_qbsConfiguration.contains(Constants::QBS_INSTALL_ROOT_KEY);
}

Utils::FilePath QbsBuildStep::installRoot(VariableHandling variableHandling) const
{
    const QString root =
            qbsConfiguration(variableHandling).value(Constants::QBS_INSTALL_ROOT_KEY).toString();
    if (!root.isNull())
        return Utils::FilePath::fromString(root);
    QString defaultInstallDir = QbsSettings::defaultInstallDirTemplate();
    if (variableHandling == VariableHandling::ExpandVariables)
        defaultInstallDir = macroExpander()->expand(defaultInstallDir);
    return FilePath::fromString(defaultInstallDir);
}

int QbsBuildStep::maxJobs() const
{
    if (m_maxJobCount > 0)
        return m_maxJobCount;
    return QThread::idealThreadCount();
}

static QString forceProbesKey() { return QLatin1String("Qbs.forceProbesKey"); }

bool QbsBuildStep::fromMap(const QVariantMap &map)
{
    if (!ProjectExplorer::BuildStep::fromMap(map))
        return false;

    setQbsConfiguration(map.value(QBS_CONFIG).toMap());
    m_keepGoing = map.value(QBS_KEEP_GOING).toBool();
    m_maxJobCount = map.value(QBS_MAXJOBCOUNT).toInt();
    m_showCommandLines = map.value(QBS_SHOWCOMMANDLINES).toBool();
    m_install = map.value(QBS_INSTALL, true).toBool();
    m_cleanInstallDir = map.value(QBS_CLEAN_INSTALL_ROOT).toBool();
    m_forceProbes = map.value(forceProbesKey()).toBool();
    return true;
}

QVariantMap QbsBuildStep::toMap() const
{
    QVariantMap map = ProjectExplorer::BuildStep::toMap();
    map.insert(QBS_CONFIG, m_qbsConfiguration);
    map.insert(QBS_KEEP_GOING, m_keepGoing);
    map.insert(QBS_MAXJOBCOUNT, m_maxJobCount);
    map.insert(QBS_SHOWCOMMANDLINES, m_showCommandLines);
    map.insert(QBS_INSTALL, m_install);
    map.insert(QBS_CLEAN_INSTALL_ROOT, m_cleanInstallDir);
    map.insert(forceProbesKey(), m_forceProbes);
    return map;
}

void QbsBuildStep::buildingDone(const ErrorInfo &error)
{
    m_session->disconnect(this);
    m_session = nullptr;
    m_lastWasSuccess = !error.hasError();
    for (const ErrorInfoItem &item : qAsConst(error.items)) {
        createTaskAndOutput(
                    ProjectExplorer::Task::Error,
                    item.description,
                    item.filePath.toString(),
                    item.line);
    }

    // Building can uncover additional target artifacts.
    qbsBuildSystem()->updateAfterBuild();

    // The reparsing, if it is necessary, has to be done before finished() is emitted, as
    // otherwise a potential additional build step could conflict with the parsing step.
    if (qbsBuildSystem()->parsingScheduled()) {
        m_parsingAfterBuild = true;
        parseProject();
    } else {
        finish();
    }
}

void QbsBuildStep::reparsingDone(bool success)
{
    disconnect(target(), &Target::parsingFinished, this, &QbsBuildStep::reparsingDone);
    m_parsingProject = false;
    if (m_parsingAfterBuild) {
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
    m_currentTask = desciption;
    m_maxProgress = max;
}

void QbsBuildStep::handleProgress(int value)
{
    if (m_maxProgress > 0)
        emit progress(value * 100 / m_maxProgress, m_currentTask);
}

void QbsBuildStep::handleCommandDescription(const QString &message)
{
    emit addOutput(message, OutputFormat::Stdout);
}

void QbsBuildStep::handleProcessResult(
        const FilePath &executable,
        const QStringList &arguments,
        const FilePath &workingDir,
        const QStringList &stdOut,
        const QStringList &stdErr,
        bool success)
{
    Q_UNUSED(workingDir);
    const bool hasOutput = !stdOut.isEmpty() || !stdErr.isEmpty();
    if (success && !hasOutput)
        return;

    emit addOutput(executable.toUserOutput() + ' '  + QtcProcess::joinArgs(arguments),
                   OutputFormat::Stdout);
    for (const QString &line : stdErr)
        emit addOutput(line, OutputFormat::Stderr);
    for (const QString &line : stdOut)
        emit addOutput(line, OutputFormat::Stdout);
}

void QbsBuildStep::createTaskAndOutput(ProjectExplorer::Task::TaskType type, const QString &message,
                                       const QString &file, int line)
{
    emit addOutput(message, OutputFormat::Stdout);
    emit addTask(CompileTask(type, message, FilePath::fromString(file), line), 1);
}

QString QbsBuildStep::buildVariant() const
{
    return qbsConfiguration(PreserveVariables).value(Constants::QBS_CONFIG_VARIANT_KEY).toString();
}

QbsBuildSystem *QbsBuildStep::qbsBuildSystem() const
{
    return static_cast<QbsBuildSystem *>(buildSystem());
}

void QbsBuildStep::setBuildVariant(const QString &variant)
{
    if (m_qbsConfiguration.value(Constants::QBS_CONFIG_VARIANT_KEY).toString() == variant)
        return;
    m_qbsConfiguration.insert(Constants::QBS_CONFIG_VARIANT_KEY, variant);
    emit qbsConfigurationChanged();
    if (ProjectExplorer::BuildConfiguration *bc = buildConfiguration())
        emit bc->buildTypeChanged();
}

QString QbsBuildStep::profile() const
{
    return qbsConfiguration(PreserveVariables).value(Constants::QBS_CONFIG_PROFILE_KEY).toString();
}

void QbsBuildStep::setKeepGoing(bool kg)
{
    if (m_keepGoing == kg)
        return;
    m_keepGoing = kg;
    emit qbsBuildOptionsChanged();
}

void QbsBuildStep::setMaxJobs(int jobcount)
{
    if (m_maxJobCount == jobcount)
        return;
    m_maxJobCount = jobcount;
    emit qbsBuildOptionsChanged();
}

void QbsBuildStep::setShowCommandLines(bool show)
{
    if (showCommandLines() == show)
        return;
    m_showCommandLines = show;
    emit qbsBuildOptionsChanged();
}

void QbsBuildStep::setInstall(bool install)
{
    if (m_install == install)
        return;
    m_install = install;
    emit qbsBuildOptionsChanged();
}

void QbsBuildStep::setCleanInstallRoot(bool clean)
{
    if (m_cleanInstallDir == clean)
        return;
    m_cleanInstallDir = clean;
    emit qbsBuildOptionsChanged();
}

void QbsBuildStep::parseProject()
{
    m_parsingProject = true;
    connect(target(), &Target::parsingFinished, this, &QbsBuildStep::reparsingDone);
    qbsBuildSystem()->parseCurrentBuildConfiguration();
}

void QbsBuildStep::build()
{
    m_session = qbsBuildSystem()->session();
    if (!m_session) {
        emit addOutput(tr("No qbs session exists for this target."), OutputFormat::ErrorMessage);
        emit finished(false);
        return;
    }

    QJsonObject request;
    request.insert("type", "build-project");
    request.insert("max-job-count", maxJobs());
    request.insert("keep-going", keepGoing());
    request.insert("command-echo-mode", showCommandLines() ? "command-line" : "summary");
    request.insert("install", install());
    QbsSession::insertRequestedModuleProperties(request);
    request.insert("clean-install-root", cleanInstallRoot());
    if (!m_products.isEmpty())
        request.insert("products", QJsonArray::fromStringList(m_products));
    if (!m_changedFiles.isEmpty()) {
        const auto changedFilesArray = QJsonArray::fromStringList(m_changedFiles);
        request.insert("changed-files", changedFilesArray);
        request.insert("files-to-consider", changedFilesArray);
    }
    if (!m_activeFileTags.isEmpty())
        request.insert("active-file-tags", QJsonArray::fromStringList(m_activeFileTags));
    request.insert("data-mode", "only-if-changed");

    m_session->sendRequest(request);
    m_maxProgress = 0;
    connect(m_session, &QbsSession::projectBuilt, this, &QbsBuildStep::buildingDone);
    connect(m_session, &QbsSession::taskStarted, this, &QbsBuildStep::handleTaskStarted);
    connect(m_session, &QbsSession::taskProgress, this, &QbsBuildStep::handleProgress);
    connect(m_session, &QbsSession::commandDescription,
            this, &QbsBuildStep::handleCommandDescription);
    connect(m_session, &QbsSession::processResult, this, &QbsBuildStep::handleProcessResult);
    connect(m_session, &QbsSession::errorOccurred, this, [this] {
        buildingDone(ErrorInfo(tr("Build canceled: Qbs session failed.")));
    });
}

void QbsBuildStep::finish()
{
    m_session = nullptr;
    emit finished(m_lastWasSuccess);
}

QbsBuildStepData QbsBuildStep::stepData() const
{
    QbsBuildStepData data;
    data.command = "build";
    data.dryRun = false;
    data.keepGoing = m_keepGoing;
    data.forceProbeExecution = m_forceProbes;
    data.showCommandLines = m_showCommandLines;
    data.noInstall = !m_install;
    data.noBuild = false;
    data.cleanInstallRoot = m_cleanInstallDir;
    data.jobCount = maxJobs();
    data.installRoot = installRoot();
    return data;
}

void QbsBuildStep::dropSession()
{
    if (m_session) {
        doCancel();
        m_session->disconnect(this);
        m_session = nullptr;
    }
}


// --------------------------------------------------------------------
// QbsBuildStepConfigWidget:
// --------------------------------------------------------------------

QbsBuildStepConfigWidget::QbsBuildStepConfigWidget(QbsBuildStep *step) :
    BuildStepConfigWidget(step),
    m_ignoreChange(false)
{
    connect(step, &ProjectConfiguration::displayNameChanged,
            this, &QbsBuildStepConfigWidget::updateState);
    connect(static_cast<QbsBuildConfiguration *>(step->buildConfiguration()),
            &QbsBuildConfiguration::qbsConfigurationChanged,
            this, &QbsBuildStepConfigWidget::updateState);
    connect(step, &QbsBuildStep::qbsBuildOptionsChanged,
            this, &QbsBuildStepConfigWidget::updateState);
    connect(&QbsSettings::instance(), &QbsSettings::settingsChanged,
            this, &QbsBuildStepConfigWidget::updateState);
    connect(step->buildConfiguration(), &BuildConfiguration::buildDirectoryChanged,
            this, &QbsBuildStepConfigWidget::updateState);

    setContentsMargins(0, 0, 0, 0);

    buildVariantComboBox = new QComboBox(this);
    buildVariantComboBox->addItem(tr("Debug"));
    buildVariantComboBox->addItem(tr("Release"));

    QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    sizePolicy.setHeightForWidth(buildVariantComboBox->sizePolicy().hasHeightForWidth());
    buildVariantComboBox->setSizePolicy(sizePolicy);

    auto horizontalLayout_5 = new QHBoxLayout();
    horizontalLayout_5->addWidget(buildVariantComboBox);
    horizontalLayout_5->addItem(new QSpacerItem(70, 13, QSizePolicy::Expanding, QSizePolicy::Minimum));

    jobSpinBox = new QSpinBox(this);

    auto horizontalLayout_6 = new QHBoxLayout();
    horizontalLayout_6->addWidget(jobSpinBox);
    horizontalLayout_6->addItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));

    propertyEdit = new FancyLineEdit(this);
    keepGoingCheckBox = new QCheckBox(this);
    showCommandLinesCheckBox = new QCheckBox(this);
    forceProbesCheckBox = new QCheckBox(this);

    auto flagsLayout = new QHBoxLayout();
    flagsLayout->addWidget(keepGoingCheckBox);
    flagsLayout->addWidget(showCommandLinesCheckBox);
    flagsLayout->addWidget(forceProbesCheckBox);
    flagsLayout->addItem(new QSpacerItem(40, 13, QSizePolicy::Expanding, QSizePolicy::Minimum));

    installCheckBox = new QCheckBox(this);

    cleanInstallRootCheckBox = new QCheckBox(this);

    defaultInstallDirCheckBox = new QCheckBox(this);

    auto installLayout = new QHBoxLayout();
    installLayout->addWidget(installCheckBox);
    installLayout->addWidget(cleanInstallRootCheckBox);
    installLayout->addWidget(defaultInstallDirCheckBox);
    installLayout->addItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));

    installDirChooser = new PathChooser(this);
    installDirChooser->setExpectedKind(PathChooser::Directory);

    commandLineTextEdit = new QPlainTextEdit(this);
    commandLineTextEdit->setUndoRedoEnabled(false);
    commandLineTextEdit->setReadOnly(true);
    commandLineTextEdit->setTextInteractionFlags(Qt::TextSelectableByKeyboard|Qt::TextSelectableByMouse);

    auto formLayout = new QFormLayout(this);
    formLayout->addRow(tr("Build variant:"), horizontalLayout_5);
    formLayout->addRow(tr("Parallel jobs:"), horizontalLayout_6);
    formLayout->addRow(tr("Properties:"), propertyEdit);
    formLayout->addRow(tr("Flags:"), flagsLayout);
    formLayout->addRow(tr("Installation flags:"), installLayout);
    formLayout->addRow(tr("Installation directory:"), installDirChooser);
    formLayout->addRow(tr("Equivalent command line:"), commandLineTextEdit);

    QWidget::setTabOrder(buildVariantComboBox, jobSpinBox);
    QWidget::setTabOrder(jobSpinBox, propertyEdit);
    QWidget::setTabOrder(propertyEdit, keepGoingCheckBox);
    QWidget::setTabOrder(keepGoingCheckBox, showCommandLinesCheckBox);
    QWidget::setTabOrder(showCommandLinesCheckBox, forceProbesCheckBox);
    QWidget::setTabOrder(forceProbesCheckBox, installCheckBox);
    QWidget::setTabOrder(installCheckBox, cleanInstallRootCheckBox);
    QWidget::setTabOrder(cleanInstallRootCheckBox, commandLineTextEdit);

    jobSpinBox->setToolTip(tr("Number of concurrent build jobs."));
    propertyEdit->setToolTip(tr("Properties to pass to the project."));
    keepGoingCheckBox->setToolTip(tr("Keep going when errors occur (if at all possible)."));
    keepGoingCheckBox->setText(tr("Keep going"));
    showCommandLinesCheckBox->setText(tr("Show command lines"));
    forceProbesCheckBox->setText(tr("Force probes"));
    installCheckBox->setText(tr("Install"));
    cleanInstallRootCheckBox->setText(tr("Clean install root"));
    defaultInstallDirCheckBox->setText(tr("Use default location"));

    auto chooser = new Core::VariableChooser(this);
    chooser->addSupportedWidget(propertyEdit);
    chooser->addSupportedWidget(installDirChooser->lineEdit());
    chooser->addMacroExpanderProvider([step] { return step->macroExpander(); });
    propertyEdit->setValidationFunction([this](FancyLineEdit *edit, QString *errorMessage) {
        return validateProperties(edit, errorMessage);
    });

    connect(buildVariantComboBox,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &QbsBuildStepConfigWidget::changeBuildVariant);
    connect(keepGoingCheckBox, &QAbstractButton::toggled,
            this, &QbsBuildStepConfigWidget::changeKeepGoing);
    connect(jobSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &QbsBuildStepConfigWidget::changeJobCount);
    connect(showCommandLinesCheckBox, &QCheckBox::toggled, this,
            &QbsBuildStepConfigWidget::changeShowCommandLines);
    connect(installCheckBox, &QCheckBox::toggled, this,
            &QbsBuildStepConfigWidget::changeInstall);
    connect(cleanInstallRootCheckBox, &QCheckBox::toggled, this,
            &QbsBuildStepConfigWidget::changeCleanInstallRoot);
    connect(defaultInstallDirCheckBox, &QCheckBox::toggled, this,
            &QbsBuildStepConfigWidget::changeUseDefaultInstallDir);
    connect(installDirChooser, &Utils::PathChooser::rawPathChanged, this,
            &QbsBuildStepConfigWidget::changeInstallDir);
    connect(forceProbesCheckBox, &QCheckBox::toggled, this,
            &QbsBuildStepConfigWidget::changeForceProbes);
    updateState();
    setSummaryText(tr("<b>Qbs:</b> %1").arg("build"));
}

void QbsBuildStepConfigWidget::updateState()
{
    if (!m_ignoreChange) {
        keepGoingCheckBox->setChecked(qbsStep()->keepGoing());
        jobSpinBox->setValue(qbsStep()->maxJobs());
        showCommandLinesCheckBox->setChecked(qbsStep()->showCommandLines());
        installCheckBox->setChecked(qbsStep()->install());
        cleanInstallRootCheckBox->setChecked(qbsStep()->cleanInstallRoot());
        forceProbesCheckBox->setChecked(qbsStep()->forceProbes());
        updatePropertyEdit(qbsStep()->qbsConfiguration(QbsBuildStep::PreserveVariables));
        installDirChooser->setFilePath(qbsStep()->installRoot(QbsBuildStep::PreserveVariables));
        defaultInstallDirCheckBox->setChecked(!qbsStep()->hasCustomInstallRoot());
    }

    const QString buildVariant = qbsStep()->buildVariant();
    const int idx = (buildVariant == Constants::QBS_VARIANT_DEBUG) ? 0 : 1;
    buildVariantComboBox->setCurrentIndex(idx);
    const auto qbsBuildConfig = static_cast<QbsBuildConfiguration *>(step()->buildConfiguration());

    QString command = qbsBuildConfig->equivalentCommandLine(qbsStep()->stepData());

    for (int i = 0; i < m_propertyCache.count(); ++i) {
        command += ' ' + m_propertyCache.at(i).name + ':' + m_propertyCache.at(i).effectiveValue;
    }

    const auto addToCommand = [&command](TriState ts, const QString &key) {
        if (ts == TriState::Enabled)
            command.append(' ').append(key).append(":true");
        else if (ts == TriState::Disabled)
            command.append(' ').append(key).append(":false");
        // Do nothing for TriState::Default
    };

    addToCommand(qbsBuildConfig->separateDebugInfoSetting(),
                 Constants::QBS_CONFIG_SEPARATE_DEBUG_INFO_KEY);

    addToCommand(qbsBuildConfig->qmlDebuggingSetting(),
                 Constants::QBS_CONFIG_QUICK_DEBUG_KEY);

    addToCommand(qbsBuildConfig->qtQuickCompilerSetting(),
                 Constants::QBS_CONFIG_QUICK_COMPILER_KEY);

    commandLineTextEdit->setPlainText(command);
}


void QbsBuildStepConfigWidget::updatePropertyEdit(const QVariantMap &data)
{
    QVariantMap editable = data;

    // remove data that is edited with special UIs:
    editable.remove(Constants::QBS_CONFIG_PROFILE_KEY);
    editable.remove(Constants::QBS_CONFIG_VARIANT_KEY);
    editable.remove(Constants::QBS_CONFIG_DECLARATIVE_DEBUG_KEY); // For existing .user files
    editable.remove(Constants::QBS_CONFIG_SEPARATE_DEBUG_INFO_KEY);
    editable.remove(Constants::QBS_CONFIG_QUICK_DEBUG_KEY);
    editable.remove(Constants::QBS_CONFIG_QUICK_COMPILER_KEY);
    editable.remove(Constants::QBS_FORCE_PROBES_KEY);
    editable.remove(Constants::QBS_INSTALL_ROOT_KEY);

    QStringList propertyList;
    for (QVariantMap::const_iterator i = editable.constBegin(); i != editable.constEnd(); ++i)
        propertyList.append(i.key() + ':' + i.value().toString());

    propertyEdit->setText(QtcProcess::joinArgs(propertyList));
}

void QbsBuildStepConfigWidget::changeBuildVariant(int idx)
{
    QString variant;
    if (idx == 1)
        variant = Constants::QBS_VARIANT_RELEASE;
    else
        variant = Constants::QBS_VARIANT_DEBUG;
    m_ignoreChange = true;
    qbsStep()->setBuildVariant(variant);
    m_ignoreChange = false;
}

void QbsBuildStepConfigWidget::changeShowCommandLines(bool show)
{
    m_ignoreChange = true;
    qbsStep()->setShowCommandLines(show);
    m_ignoreChange = false;
}

void QbsBuildStepConfigWidget::changeKeepGoing(bool kg)
{
    m_ignoreChange = true;
    qbsStep()->setKeepGoing(kg);
    m_ignoreChange = false;
}

void QbsBuildStepConfigWidget::changeJobCount(int count)
{
    m_ignoreChange = true;
    qbsStep()->setMaxJobs(count);
    m_ignoreChange = false;
}

void QbsBuildStepConfigWidget::changeInstall(bool install)
{
    m_ignoreChange = true;
    qbsStep()->setInstall(install);
    m_ignoreChange = false;
}

void QbsBuildStepConfigWidget::changeCleanInstallRoot(bool clean)
{
    m_ignoreChange = true;
    qbsStep()->setCleanInstallRoot(clean);
    m_ignoreChange = false;
}

void QbsBuildStepConfigWidget::changeUseDefaultInstallDir(bool useDefault)
{
    m_ignoreChange = true;
    QVariantMap config = qbsStep()->qbsConfiguration(QbsBuildStep::PreserveVariables);
    installDirChooser->setEnabled(!useDefault);
    if (useDefault)
        config.remove(Constants::QBS_INSTALL_ROOT_KEY);
    else
        config.insert(Constants::QBS_INSTALL_ROOT_KEY, installDirChooser->rawPath());
    qbsStep()->setQbsConfiguration(config);
    m_ignoreChange = false;
}

void QbsBuildStepConfigWidget::changeInstallDir(const QString &dir)
{
    if (!qbsStep()->hasCustomInstallRoot())
        return;
    m_ignoreChange = true;
    QVariantMap config = qbsStep()->qbsConfiguration(QbsBuildStep::PreserveVariables);
    config.insert(Constants::QBS_INSTALL_ROOT_KEY, dir);
    qbsStep()->setQbsConfiguration(config);
    m_ignoreChange = false;
}

void QbsBuildStepConfigWidget::changeForceProbes(bool forceProbes)
{
    m_ignoreChange = true;
    qbsStep()->setForceProbes(forceProbes);
    m_ignoreChange = false;
}

void QbsBuildStepConfigWidget::applyCachedProperties()
{
    QVariantMap data;
    const QVariantMap tmp = qbsStep()->qbsConfiguration(QbsBuildStep::PreserveVariables);

    // Insert values set up with special UIs:
    data.insert(Constants::QBS_CONFIG_PROFILE_KEY,
                tmp.value(Constants::QBS_CONFIG_PROFILE_KEY));
    data.insert(Constants::QBS_CONFIG_VARIANT_KEY,
                tmp.value(Constants::QBS_CONFIG_VARIANT_KEY));
    const QStringList additionalSpecialKeys({Constants::QBS_CONFIG_DECLARATIVE_DEBUG_KEY,
                                             Constants::QBS_CONFIG_QUICK_DEBUG_KEY,
                                             Constants::QBS_CONFIG_QUICK_COMPILER_KEY,
                                             Constants::QBS_CONFIG_SEPARATE_DEBUG_INFO_KEY,
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
    qbsStep()->setQbsConfiguration(data);
    m_ignoreChange = false;
}

QbsBuildStep *QbsBuildStepConfigWidget::qbsStep() const
{
    return static_cast<QbsBuildStep *>(step());
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
    const MacroExpander * const expander = step()->macroExpander();
    foreach (const QString &rawArg, argList) {
        int pos = rawArg.indexOf(':');
        if (pos > 0) {
            const QString propertyName = rawArg.left(pos);
            static const QStringList specialProperties{
                Constants::QBS_CONFIG_PROFILE_KEY, Constants::QBS_CONFIG_VARIANT_KEY,
                Constants::QBS_CONFIG_QUICK_DEBUG_KEY, Constants::QBS_CONFIG_QUICK_COMPILER_KEY,
                Constants::QBS_INSTALL_ROOT_KEY, Constants::QBS_CONFIG_SEPARATE_DEBUG_INFO_KEY,
            };
            if (specialProperties.contains(propertyName)) {
                if (errorMessage) {
                    *errorMessage = tr("Property \"%1\" cannot be set here. "
                                       "Please use the dedicated UI element.").arg(propertyName);
                }
                return false;
            }
            const QString rawValue = rawArg.mid(pos + 1);
            Property property(propertyName, rawValue, expander->expand(rawValue));
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

QbsBuildStepFactory::QbsBuildStepFactory()
{
    registerStep<QbsBuildStep>(Constants::QBS_BUILDSTEP_ID);
    setDisplayName(QbsBuildStep::tr("Qbs Build"));
    setSupportedStepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
    setSupportedConfiguration(Constants::QBS_BC_ID);
    setSupportedProjectType(Constants::PROJECT_ID);
}

} // namespace Internal
} // namespace QbsProjectManager

#include "qbsbuildstep.moc"
