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

#include "qmakestep.h"

#include "qmakemakestep.h"
#include "qmakebuildconfiguration.h"
#include "qmakekitinformation.h"
#include "qmakenodes.h"
#include "qmakeparser.h"
#include "qmakeproject.h"
#include "qmakeprojectmanagerconstants.h"
#include "qmakesettings.h"

#include <projectexplorer/buildmanager.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/gnumakeparser.h>
#include <projectexplorer/processparameters.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>

#include <coreplugin/icore.h>
#include <coreplugin/icontext.h>
#include <coreplugin/variablechooser.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtversionmanager.h>
#include <qtsupport/qtsupportconstants.h>
#include <utils/algorithm.h>
#include <utils/hostosinfo.h>
#include <utils/qtcprocess.h>
#include <utils/utilsicons.h>

#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPlainTextEdit>

using namespace QmakeProjectManager;
using namespace QmakeProjectManager::Internal;
using namespace QtSupport;
using namespace ProjectExplorer;
using namespace Utils;

namespace {
const char QMAKE_ARGUMENTS_KEY[] = "QtProjectManager.QMakeBuildStep.QMakeArguments";
const char QMAKE_FORCED_KEY[] = "QtProjectManager.QMakeBuildStep.QMakeForced";
const char QMAKE_SELECTED_ABIS_KEY[] = "QtProjectManager.QMakeBuildStep.SelectedAbis";
}

QMakeStep::QMakeStep(BuildStepList *bsl, Utils::Id id)
    : AbstractProcessStep(bsl, id)
{
    //: QMakeStep default display name
    setDefaultDisplayName(tr("qmake"));
    setLowPriority();
}

QmakeBuildConfiguration *QMakeStep::qmakeBuildConfiguration() const
{
    return qobject_cast<QmakeBuildConfiguration *>(buildConfiguration());
}

QmakeBuildSystem *QMakeStep::qmakeBuildSystem() const
{
    return qmakeBuildConfiguration()->qmakeBuildSystem();
}

///
/// Returns all arguments
/// That is: possbile subpath
/// spec
/// config arguemnts
/// moreArguments
/// user arguments
QString QMakeStep::allArguments(const BaseQtVersion *v, ArgumentFlags flags) const
{
    QTC_ASSERT(v, return QString());
    QmakeBuildConfiguration *bc = qmakeBuildConfiguration();
    QStringList arguments;
    if (bc->subNodeBuild())
        arguments << bc->subNodeBuild()->filePath().toUserOutput();
    else if (flags & ArgumentFlag::OmitProjectPath)
        arguments << project()->projectFilePath().fileName();
    else
        arguments << project()->projectFilePath().toUserOutput();

    if (v->qtVersion() < QtVersionNumber(5, 0, 0))
        arguments << "-r";
    bool userProvidedMkspec = false;
    for (QtcProcess::ConstArgIterator ait(m_userArgs); ait.next(); ) {
        if (ait.value() == "-spec") {
            if (ait.next()) {
                userProvidedMkspec = true;
                break;
            }
        }
    }
    const QString specArg = mkspec();
    if (!userProvidedMkspec && !specArg.isEmpty())
        arguments << "-spec" << QDir::toNativeSeparators(specArg);

    // Find out what flags we pass on to qmake
    arguments << bc->configCommandLineArguments();

    arguments << deducedArguments().toArguments();

    QString args = QtcProcess::joinArgs(arguments);
    // User arguments
    QtcProcess::addArgs(&args, m_userArgs);
    foreach (QString arg, m_extraArgs)
        QtcProcess::addArgs(&args, arg);
    return (flags & ArgumentFlag::Expand) ? bc->macroExpander()->expand(args) : args;
}

QMakeStepConfig QMakeStep::deducedArguments() const
{
    Kit *kit = target()->kit();
    QMakeStepConfig config;
    Abi targetAbi;
    if (ToolChain *tc = ToolChainKitAspect::cxxToolChain(kit)) {
        targetAbi = tc->targetAbi();
        if (HostOsInfo::isWindowsHost()
            && tc->typeId() == ProjectExplorer::Constants::CLANG_TOOLCHAIN_TYPEID) {
            config.sysRoot = SysRootKitAspect::sysRoot(kit).toString();
            config.targetTriple = tc->originalTargetTriple();
        }
    }

    BaseQtVersion *version = QtKitAspect::qtVersion(target()->kit());

    config.archConfig = QMakeStepConfig::targetArchFor(targetAbi, version);
    config.osType = QMakeStepConfig::osTypeFor(targetAbi, version);
    config.separateDebugInfo = qmakeBuildConfiguration()->separateDebugInfo();
    config.linkQmlDebuggingQQ2 = qmakeBuildConfiguration()->qmlDebugging();
    config.useQtQuickCompiler = qmakeBuildConfiguration()->useQtQuickCompiler();

    return config;
}

bool QMakeStep::init()
{
    m_wasSuccess = true;
    QmakeBuildConfiguration *qmakeBc = qmakeBuildConfiguration();
    const BaseQtVersion *qtVersion = QtKitAspect::qtVersion(target()->kit());

    if (!qtVersion) {
        emit addOutput(tr("No Qt version configured."), BuildStep::OutputFormat::ErrorMessage);
        return false;
    }

    FilePath workingDirectory;

    if (qmakeBc->subNodeBuild())
        workingDirectory = qmakeBc->qmakeBuildSystem()->buildDir(qmakeBc->subNodeBuild()->filePath());
    else
        workingDirectory = qmakeBc->buildDirectory();

    m_qmakeCommand = CommandLine{qtVersion->qmakeCommand(), allArguments(qtVersion), CommandLine::Raw};
    m_runMakeQmake = (qtVersion->qtVersion() >= QtVersionNumber(5, 0 ,0));

    QString makefile = workingDirectory.toString() + '/';

    if (qmakeBc->subNodeBuild()) {
        QmakeProFileNode *pro = qmakeBc->subNodeBuild();
        if (pro && !pro->makefile().isEmpty())
            makefile.append(pro->makefile());
        else
            makefile.append("Makefile");
    } else if (!qmakeBc->makefile().isEmpty()) {
        makefile.append(qmakeBc->makefile());
    } else {
        makefile.append("Makefile");
    }

    if (m_runMakeQmake) {
        const FilePath make = makeCommand();
        if (make.isEmpty()) {
            emit addOutput(tr("Could not determine which \"make\" command to run. "
                              "Check the \"make\" step in the build configuration."),
                           BuildStep::OutputFormat::ErrorMessage);
            return false;
        }
        m_makeCommand = CommandLine{make, makeArguments(makefile), CommandLine::Raw};
    } else {
        m_makeCommand = {};
    }

    // Check whether we need to run qmake
    if (m_forced || QmakeSettings::alwaysRunQmake()
            || qmakeBc->compareToImportFrom(makefile) != QmakeBuildConfiguration::MakefileMatches) {
        m_needToRunQMake = true;
    }
    m_forced = false;

    ProcessParameters *pp = processParameters();
    pp->setMacroExpander(qmakeBc->macroExpander());
    pp->setWorkingDirectory(workingDirectory);
    pp->setEnvironment(qmakeBc->environment());

    QmakeProFileNode *node = static_cast<QmakeProFileNode *>(qmakeBc->project()->rootProjectNode());
    if (qmakeBc->subNodeBuild())
        node = qmakeBc->subNodeBuild();
    QTC_ASSERT(node, return false);
    QString proFile = node->filePath().toString();

    Tasks tasks = qtVersion->reportIssues(proFile, workingDirectory.toString());
    Utils::sort(tasks);

    if (!tasks.isEmpty()) {
        bool canContinue = true;
        foreach (const ProjectExplorer::Task &t, tasks) {
            emit addTask(t);
            if (t.type == Task::Error)
                canContinue = false;
        }
        if (!canContinue) {
            emitFaultyConfigurationMessage();
            return false;
        }
    }

    m_scriptTemplate = node->projectType() == ProjectType::ScriptTemplate;

    return AbstractProcessStep::init();
}

void QMakeStep::setupOutputFormatter(OutputFormatter *formatter)
{
    formatter->addLineParser(new QMakeParser);
    m_outputFormatter = formatter;
    AbstractProcessStep::setupOutputFormatter(formatter);
}

void QMakeStep::doRun()
{
    if (m_scriptTemplate) {
        emit finished(true);
        return;
    }

    if (!m_needToRunQMake) {
        emit addOutput(tr("Configuration unchanged, skipping qmake step."), BuildStep::OutputFormat::NormalMessage);
        emit finished(true);
        return;
    }

    m_needToRunQMake = false;

    m_nextState = State::RUN_QMAKE;
    runNextCommand();
}

void QMakeStep::setForced(bool b)
{
    m_forced = b;
}

ProjectExplorer::BuildStepConfigWidget *QMakeStep::createConfigWidget()
{
    return new QMakeStepConfigWidget(this);
}

void QMakeStep::processStartupFailed()
{
    m_needToRunQMake = true;
    AbstractProcessStep::processStartupFailed();
}

bool QMakeStep::processSucceeded(int exitCode, QProcess::ExitStatus status)
{
    bool result = AbstractProcessStep::processSucceeded(exitCode, status);
    if (!result)
        m_needToRunQMake = true;
    emit buildConfiguration()->buildDirectoryChanged();
    return result;
}

void QMakeStep::doCancel()
{
    AbstractProcessStep::doCancel();
}

void QMakeStep::finish(bool success)
{
    m_wasSuccess = success;
    runNextCommand();
}

void QMakeStep::startOneCommand(const CommandLine &command)
{
    ProcessParameters *pp = processParameters();
    pp->setCommandLine(command);

    AbstractProcessStep::doRun();
}

void QMakeStep::runNextCommand()
{
    if (isCanceled())
        m_wasSuccess = false;

    if (!m_wasSuccess)
        m_nextState = State::POST_PROCESS;

    emit progress(static_cast<int>(m_nextState) * 100 / static_cast<int>(State::POST_PROCESS),
                  QString());

    switch (m_nextState) {
    case State::IDLE:
        return;
    case State::RUN_QMAKE:
        m_outputFormatter->setLineParsers({new QMakeParser});
        m_nextState = (m_runMakeQmake ? State::RUN_MAKE_QMAKE_ALL : State::POST_PROCESS);
        startOneCommand(m_qmakeCommand);
        return;
    case State::RUN_MAKE_QMAKE_ALL:
        {
            auto *parser = new GnuMakeParser;
            parser->addSearchDir(processParameters()->workingDirectory());
            m_outputFormatter->setLineParsers({parser});
            m_nextState = State::POST_PROCESS;
            startOneCommand(m_makeCommand);
        }
        return;
    case State::POST_PROCESS:
        m_nextState = State::IDLE;
        emit finished(m_wasSuccess);
        return;
    }
}

QStringList QMakeStep::selectedAbis() const
{
    return m_selectedAbis;
}

void QMakeStep::setSelectedAbis(const QStringList &selectedAbis)
{
    m_selectedAbis = selectedAbis;
}

void QMakeStep::setUserArguments(const QString &arguments)
{
    if (m_userArgs == arguments)
        return;
    m_userArgs = arguments;

    emit userArgumentsChanged();

    emit qmakeBuildConfiguration()->qmakeBuildConfigurationChanged();
    qmakeBuildSystem()->scheduleUpdateAllNowOrLater();
}

QStringList QMakeStep::extraArguments() const
{
    return m_extraArgs;
}

void QMakeStep::setExtraArguments(const QStringList &args)
{
    if (m_extraArgs != args) {
        m_extraArgs = args;
        emit extraArgumentsChanged();
        emit qmakeBuildConfiguration()->qmakeBuildConfigurationChanged();
        qmakeBuildSystem()->scheduleUpdateAllNowOrLater();
    }
}

QStringList QMakeStep::extraParserArguments() const
{
    return m_extraParserArgs;
}

void QMakeStep::setExtraParserArguments(const QStringList &args)
{
    m_extraParserArgs = args;
}

FilePath QMakeStep::makeCommand() const
{
    if (auto ms = stepList()->firstOfType<MakeStep>())
        return ms->makeExecutable();
    return FilePath();
}

QString QMakeStep::makeArguments(const QString &makefile) const
{
    QString args;
    if (!makefile.isEmpty()) {
        Utils::QtcProcess::addArg(&args, "-f");
        Utils::QtcProcess::addArg(&args, makefile);
    }
    Utils::QtcProcess::addArg(&args, "qmake_all");
    return args;
}

QString QMakeStep::effectiveQMakeCall() const
{
    BaseQtVersion *qtVersion = QtKitAspect::qtVersion(target()->kit());
    QString qmake = qtVersion ? qtVersion->qmakeCommand().toUserOutput() : QString();
    if (qmake.isEmpty())
        qmake = tr("<no Qt version>");
    QString make = makeCommand().toString();
    if (make.isEmpty())
        make = tr("<no Make step found>");

    QString result = qmake;
    if (qtVersion) {
        QmakeBuildConfiguration *qmakeBc = qmakeBuildConfiguration();
        const QString makefile = qmakeBc ? qmakeBc->makefile() : QString();
        result += ' ' + allArguments(qtVersion, ArgumentFlag::Expand);
        if (qtVersion->qtVersion() >= QtVersionNumber(5, 0, 0))
            result.append(QString::fromLatin1(" && %1 %2").arg(make).arg(makeArguments(makefile)));
    }
    return result;
}

QStringList QMakeStep::parserArguments()
{
    // NOTE: extra parser args placed before the other args intentionally
    QStringList result = m_extraParserArgs;
    BaseQtVersion *qt = QtKitAspect::qtVersion(target()->kit());
    QTC_ASSERT(qt, return QStringList());
    for (QtcProcess::ConstArgIterator ait(allArguments(qt, ArgumentFlag::Expand)); ait.next(); ) {
        if (ait.isSimple())
            result << ait.value();
    }
    return result;
}

QString QMakeStep::userArguments()
{
    return m_userArgs;
}

QString QMakeStep::mkspec() const
{
    QString additionalArguments = m_userArgs;
    QtcProcess::addArgs(&additionalArguments, m_extraArgs);
    for (QtcProcess::ArgIterator ait(&additionalArguments); ait.next(); ) {
        if (ait.value() == "-spec") {
            if (ait.next())
                return FilePath::fromUserInput(ait.value()).toString();
        }
    }

    return QmakeKitAspect::effectiveMkspec(target()->kit());
}

QVariantMap QMakeStep::toMap() const
{
    QVariantMap map(AbstractProcessStep::toMap());
    map.insert(QMAKE_ARGUMENTS_KEY, m_userArgs);
    map.insert(QMAKE_FORCED_KEY, m_forced);
    map.insert(QMAKE_SELECTED_ABIS_KEY, m_selectedAbis);
    return map;
}

bool QMakeStep::fromMap(const QVariantMap &map)
{
    m_userArgs = map.value(QMAKE_ARGUMENTS_KEY).toString();
    m_forced = map.value(QMAKE_FORCED_KEY, false).toBool();
    m_selectedAbis = map.value(QMAKE_SELECTED_ABIS_KEY).toStringList();

    // Backwards compatibility with < Creator 4.12.
    const QVariant separateDebugInfo
            = map.value("QtProjectManager.QMakeBuildStep.SeparateDebugInfo");
    if (separateDebugInfo.isValid())
        qmakeBuildConfiguration()->forceSeparateDebugInfo(separateDebugInfo.toBool());
    const QVariant qmlDebugging
            = map.value("QtProjectManager.QMakeBuildStep.LinkQmlDebuggingLibrary");
    if (qmlDebugging.isValid())
        qmakeBuildConfiguration()->forceQmlDebugging(qmlDebugging.toBool());
    const QVariant useQtQuickCompiler
            = map.value("QtProjectManager.QMakeBuildStep.UseQtQuickCompiler");
    if (useQtQuickCompiler.isValid())
        qmakeBuildConfiguration()->forceQtQuickCompiler(useQtQuickCompiler.toBool());

    return BuildStep::fromMap(map);
}

////
// QMakeStepConfigWidget
////

QMakeStepConfigWidget::QMakeStepConfigWidget(QMakeStep *step)
    : BuildStepConfigWidget(step), m_step(step)
{
    auto label_0 = new QLabel(tr("qmake build configuration:"), this);

    auto buildConfigurationWidget = new QWidget(this);

    buildConfigurationComboBox = new QComboBox(buildConfigurationWidget);
    buildConfigurationComboBox->addItem(tr("Debug"));
    buildConfigurationComboBox->addItem(tr("Release"));

    QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    sizePolicy.setHeightForWidth(buildConfigurationComboBox->sizePolicy().hasHeightForWidth());
    buildConfigurationComboBox->setSizePolicy(sizePolicy);

    auto horizontalLayout_0 = new QHBoxLayout(buildConfigurationWidget);
    horizontalLayout_0->setContentsMargins(0, 0, 0, 0);
    horizontalLayout_0->addWidget(buildConfigurationComboBox);
    horizontalLayout_0->addItem(new QSpacerItem(71, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));

    auto qmakeArgsLabel = new QLabel(tr("Additional arguments:"), this);

    qmakeAdditonalArgumentsLineEdit = new QLineEdit(this);

    auto label = new QLabel(tr("Effective qmake call:"), this);
    label->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignTop);

    qmakeArgumentsEdit = new QPlainTextEdit(this);
    qmakeArgumentsEdit->setEnabled(true);
    qmakeArgumentsEdit->setMaximumSize(QSize(16777215, 120));
    qmakeArgumentsEdit->setTextInteractionFlags(Qt::TextSelectableByKeyboard|Qt::TextSelectableByMouse);

    abisLabel = new QLabel(tr("ABIs:"), this);
    abisLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignTop);

    abisListWidget = new QListWidget(this);
    qmakeAdditonalArgumentsLineEdit->setText(m_step->userArguments());

    auto formLayout = new QFormLayout(this);
    formLayout->addRow(label_0, buildConfigurationWidget);
    formLayout->addRow(qmakeArgsLabel, qmakeAdditonalArgumentsLineEdit);
    formLayout->addRow(label, qmakeArgumentsEdit);
    formLayout->addRow(abisLabel, abisListWidget);

    qmakeBuildConfigChanged();

    updateSummaryLabel();
    updateEffectiveQMakeCall();

    connect(qmakeAdditonalArgumentsLineEdit, &QLineEdit::textEdited,
            this, &QMakeStepConfigWidget::qmakeArgumentsLineEdited);
    connect(buildConfigurationComboBox,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &QMakeStepConfigWidget::buildConfigurationSelected);
    connect(step, &QMakeStep::userArgumentsChanged,
            this, &QMakeStepConfigWidget::userArgumentsChanged);
    connect(step->qmakeBuildConfiguration(), &QmakeBuildConfiguration::qmlDebuggingChanged,
            this, [this] {
        linkQmlDebuggingLibraryChanged();
        askForRebuild(tr("QML Debugging"));
    });
    connect(step->project(), &Project::projectLanguagesUpdated,
            this, &QMakeStepConfigWidget::linkQmlDebuggingLibraryChanged);
    connect(step->target(), &Target::parsingFinished,
            this, &QMakeStepConfigWidget::updateEffectiveQMakeCall);
    connect(step->qmakeBuildConfiguration(), &QmakeBuildConfiguration::useQtQuickCompilerChanged,
            this, &QMakeStepConfigWidget::useQtQuickCompilerChanged);
    connect(step->qmakeBuildConfiguration(), &QmakeBuildConfiguration::separateDebugInfoChanged,
            this, &QMakeStepConfigWidget::separateDebugInfoChanged);
    connect(step->qmakeBuildConfiguration(), &QmakeBuildConfiguration::qmakeBuildConfigurationChanged,
            this, &QMakeStepConfigWidget::qmakeBuildConfigChanged);
    connect(step->target(), &Target::kitChanged, this, &QMakeStepConfigWidget::qtVersionChanged);
    connect(abisListWidget, &QListWidget::itemChanged, this, [this]{
        abisChanged();
        if (QmakeBuildConfiguration *bc = m_step->qmakeBuildConfiguration())
            BuildManager::buildLists({bc->cleanSteps()});
    });
    auto chooser = new Core::VariableChooser(qmakeAdditonalArgumentsLineEdit);
    chooser->addMacroExpanderProvider([step] { return step->macroExpander(); });
    chooser->addSupportedWidget(qmakeAdditonalArgumentsLineEdit);
}

QMakeStepConfigWidget::~QMakeStepConfigWidget()
{
}

void QMakeStepConfigWidget::qtVersionChanged()
{
    updateSummaryLabel();
    updateEffectiveQMakeCall();
}

void QMakeStepConfigWidget::qmakeBuildConfigChanged()
{
    QmakeBuildConfiguration *bc = m_step->qmakeBuildConfiguration();
    bool debug = bc->qmakeBuildConfiguration() & BaseQtVersion::DebugBuild;
    m_ignoreChange = true;
    buildConfigurationComboBox->setCurrentIndex(debug? 0 : 1);
    m_ignoreChange = false;
    updateSummaryLabel();
    updateEffectiveQMakeCall();
}

void QMakeStepConfigWidget::userArgumentsChanged()
{
    if (m_ignoreChange)
        return;
    qmakeAdditonalArgumentsLineEdit->setText(m_step->userArguments());
    updateSummaryLabel();
    updateEffectiveQMakeCall();
}

void QMakeStepConfigWidget::linkQmlDebuggingLibraryChanged()
{
    updateSummaryLabel();
    updateEffectiveQMakeCall();
}

void QMakeStepConfigWidget::useQtQuickCompilerChanged()
{
    updateSummaryLabel();
    updateEffectiveQMakeCall();
    askForRebuild(tr("Qt Quick Compiler"));
}

void QMakeStepConfigWidget::separateDebugInfoChanged()
{
    updateSummaryLabel();
    updateEffectiveQMakeCall();
    askForRebuild(tr("Separate Debug Information"));
}

void QMakeStepConfigWidget::abisChanged()
{
    QStringList abis;
    for (int i = 0; i < abisListWidget->count(); ++i) {
        auto item = abisListWidget->item(i);
        if (item->checkState() == Qt::CheckState::Checked)
            abis << item->text();
    }
    m_step->setSelectedAbis(abis);

    if (isAndroidKit()) {
        const QString prefix = "ANDROID_ABIS=";
        QStringList args = m_step->extraArguments();
        for (auto it = args.begin(); it != args.end(); ++it) {
            if (it->startsWith(prefix)) {
                args.erase(it);
                break;
            }
        }
        args << prefix + '"' + abis.join(' ') + '"';
        m_step->setExtraArguments(args);
    }

    updateSummaryLabel();
    updateEffectiveQMakeCall();
}

void QMakeStepConfigWidget::qmakeArgumentsLineEdited()
{
    m_ignoreChange = true;
    m_step->setUserArguments(qmakeAdditonalArgumentsLineEdit->text());
    m_ignoreChange = false;

    updateSummaryLabel();
    updateEffectiveQMakeCall();
}

void QMakeStepConfigWidget::buildConfigurationSelected()
{
    if (m_ignoreChange)
        return;
    QmakeBuildConfiguration *bc = m_step->qmakeBuildConfiguration();
    BaseQtVersion::QmakeBuildConfigs buildConfiguration = bc->qmakeBuildConfiguration();
    if (buildConfigurationComboBox->currentIndex() == 0) { // debug
        buildConfiguration = buildConfiguration | BaseQtVersion::DebugBuild;
    } else {
        buildConfiguration = buildConfiguration & ~BaseQtVersion::DebugBuild;
    }
    m_ignoreChange = true;
    bc->setQMakeBuildConfiguration(buildConfiguration);
    m_ignoreChange = false;

    updateSummaryLabel();
    updateEffectiveQMakeCall();
}

void QMakeStepConfigWidget::askForRebuild(const QString &title)
{
    auto *question = new QMessageBox(Core::ICore::dialogParent());
    question->setWindowTitle(title);
    question->setText(tr("The option will only take effect if the project is recompiled. Do you want to recompile now?"));
    question->setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    question->setModal(true);
    connect(question, &QDialog::finished, this, &QMakeStepConfigWidget::recompileMessageBoxFinished);
    question->show();
}

bool QMakeStepConfigWidget::isAndroidKit() const
{
    BaseQtVersion *qtVersion = QtKitAspect::qtVersion(m_step->target()->kit());
    if (!qtVersion)
        return false;

    const Abis abis = qtVersion->qtAbis();
    return Utils::anyOf(abis, [](const Abi &abi) {
        return abi.osFlavor() == Abi::OSFlavor::AndroidLinuxFlavor;
    });
}

void QMakeStepConfigWidget::updateSummaryLabel()
{
    BaseQtVersion *qtVersion = QtKitAspect::qtVersion(m_step->target()->kit());
    if (!qtVersion) {
        setSummaryText(tr("<b>qmake:</b> No Qt version set. Cannot run qmake."));
        return;
    }
    const Abis abis = qtVersion->qtAbis();
    const bool enableAbisSelect = abis.size() > 1;
    abisLabel->setVisible(enableAbisSelect);
    abisListWidget->setVisible(enableAbisSelect);

    if (enableAbisSelect && abisListWidget->count() != abis.size()) {
        abisListWidget->clear();
        QStringList selectedAbis = m_step->selectedAbis();

        if (selectedAbis.isEmpty() && isAndroidKit()) {
            // Prefer ARM for Android, prefer 32bit.
            for (const Abi &abi : abis) {
                if (abi.param() == "armeabi-v7a")
                    selectedAbis.append(abi.param());
            }
            if (selectedAbis.isEmpty()) {
                for (const Abi &abi : abis) {
                    if (abi.param() == "arm64-v8a")
                        selectedAbis.append(abi.param());
                }
            }
        }

        for (const Abi &abi : abis) {
            const QString param = abi.param();
            auto item = new QListWidgetItem{param, abisListWidget};
            item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
            item->setCheckState(selectedAbis.contains(param) ? Qt::Checked : Qt::Unchecked);
        }
        abisChanged();
    }

    const QString program = qtVersion->qmakeCommand().fileName();
    setSummaryText(tr("<b>qmake:</b> %1 %2").arg(program,
                                                 m_step->project()->projectFilePath().fileName()));
}

void QMakeStepConfigWidget::updateEffectiveQMakeCall()
{
    qmakeArgumentsEdit->setPlainText(m_step->effectiveQMakeCall());
}

void QMakeStepConfigWidget::recompileMessageBoxFinished(int button)
{
    if (button == QMessageBox::Yes) {
        if (BuildConfiguration *bc = m_step->buildConfiguration())
            BuildManager::buildLists({bc->cleanSteps(), bc->buildSteps()});
    }
}

////
// QMakeStepFactory
////

QMakeStepFactory::QMakeStepFactory()
{
    registerStep<QMakeStep>(Constants::QMAKE_BS_ID);
    setSupportedConfiguration(Constants::QMAKE_BC_ID);
    setSupportedStepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
    setDisplayName(QMakeStep::tr("qmake"));
    setFlags(BuildStepInfo::UniqueStep);
}

QMakeStepConfig::TargetArchConfig QMakeStepConfig::targetArchFor(const Abi &targetAbi, const BaseQtVersion *version)
{
    QMakeStepConfig::TargetArchConfig arch = QMakeStepConfig::NoArch;
    if (!version || version->type() != QtSupport::Constants::DESKTOPQT)
        return arch;
    if ((targetAbi.os() == ProjectExplorer::Abi::DarwinOS)
            && (targetAbi.binaryFormat() == ProjectExplorer::Abi::MachOFormat)) {
        if (targetAbi.architecture() == ProjectExplorer::Abi::X86Architecture) {
            if (targetAbi.wordWidth() == 32)
                arch = QMakeStepConfig::X86;
            else if (targetAbi.wordWidth() == 64)
                arch = QMakeStepConfig::X86_64;
        } else if (targetAbi.architecture() == ProjectExplorer::Abi::PowerPCArchitecture) {
            if (targetAbi.wordWidth() == 32)
                arch = QMakeStepConfig::PowerPC;
            else if (targetAbi.wordWidth() == 64)
                arch = QMakeStepConfig::PowerPC64;
        }
    }
    return arch;
}

QMakeStepConfig::OsType QMakeStepConfig::osTypeFor(const ProjectExplorer::Abi &targetAbi, const BaseQtVersion *version)
{
    QMakeStepConfig::OsType os = QMakeStepConfig::NoOsType;
    const char IOSQT[] = "Qt4ProjectManager.QtVersion.Ios";
    if (!version || version->type() != IOSQT)
        return os;
    if ((targetAbi.os() == ProjectExplorer::Abi::DarwinOS)
            && (targetAbi.binaryFormat() == ProjectExplorer::Abi::MachOFormat)) {
        if (targetAbi.architecture() == ProjectExplorer::Abi::X86Architecture) {
            os = QMakeStepConfig::IphoneSimulator;
        } else if (targetAbi.architecture() == ProjectExplorer::Abi::ArmArchitecture) {
            os = QMakeStepConfig::IphoneOS;
        }
    }
    return os;
}

QStringList QMakeStepConfig::toArguments() const
{
    QStringList arguments;
    if (archConfig == X86)
        arguments << "CONFIG+=x86";
    else if (archConfig == X86_64)
        arguments << "CONFIG+=x86_64";
    else if (archConfig == PowerPC)
        arguments << "CONFIG+=ppc";
    else if (archConfig == PowerPC64)
        arguments << "CONFIG+=ppc64";

    // TODO: make that depend on the actual Qt version that is used
    if (osType == IphoneSimulator)
        arguments << "CONFIG+=iphonesimulator" << "CONFIG+=simulator" /*since Qt 5.7*/;
    else if (osType == IphoneOS)
        arguments << "CONFIG+=iphoneos" << "CONFIG+=device" /*since Qt 5.7*/;

    if (linkQmlDebuggingQQ2 == TriState::Enabled)
        arguments << "CONFIG+=qml_debug";
    else if (linkQmlDebuggingQQ2 == TriState::Disabled)
        arguments << "CONFIG-=qml_debug";

    if (useQtQuickCompiler == TriState::Enabled)
        arguments << "CONFIG+=qtquickcompiler";
    else if (useQtQuickCompiler == TriState::Disabled)
        arguments << "CONFIG-=qtquickcompiler";

    if (separateDebugInfo == TriState::Enabled)
        arguments << "CONFIG+=force_debug_info" << "CONFIG+=separate_debug_info";
    else if (separateDebugInfo == TriState::Disabled)
        arguments << "CONFIG-=separate_debug_info";

    if (!sysRoot.isEmpty()) {
        arguments << ("QMAKE_CFLAGS+=--sysroot=\"" + sysRoot + "\"");
        arguments << ("QMAKE_CXXFLAGS+=--sysroot=\"" + sysRoot + "\"");
        arguments << ("QMAKE_LFLAGS+=--sysroot=\"" + sysRoot + "\"");
        if (!targetTriple.isEmpty()) {
            arguments << ("QMAKE_CFLAGS+=--target=" + targetTriple);
            arguments << ("QMAKE_CXXFLAGS+=--target=" + targetTriple);
            arguments << ("QMAKE_LFLAGS+=--target=" + targetTriple);
        }
    }

    return arguments;
}
