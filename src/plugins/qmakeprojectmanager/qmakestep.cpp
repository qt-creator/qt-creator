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

#include <android/androidconstants.h>

#include <projectexplorer/buildmanager.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/gnumakeparser.h>
#include <projectexplorer/processparameters.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>

#include <coreplugin/icore.h>
#include <coreplugin/icontext.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtversionmanager.h>
#include <qtsupport/qtsupportconstants.h>

#include <ios/iosconstants.h>

#include <utils/algorithm.h>
#include <utils/hostosinfo.h>
#include <utils/layoutbuilder.h>
#include <utils/qtcprocess.h>
#include <utils/utilsicons.h>
#include <utils/variablechooser.h>

#include <QDir>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPlainTextEdit>

using namespace QtSupport;
using namespace ProjectExplorer;
using namespace Utils;

using namespace QmakeProjectManager::Internal;

namespace QmakeProjectManager {

const char QMAKE_ARGUMENTS_KEY[] = "QtProjectManager.QMakeBuildStep.QMakeArguments";
const char QMAKE_FORCED_KEY[] = "QtProjectManager.QMakeBuildStep.QMakeForced";
const char QMAKE_SELECTED_ABIS_KEY[] = "QtProjectManager.QMakeBuildStep.SelectedAbis";

QMakeStep::QMakeStep(BuildStepList *bsl, Id id)
    : AbstractProcessStep(bsl, id)
{
    setLowPriority();

    m_buildType = addAspect<SelectionAspect>();
    m_buildType->setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);
    m_buildType->setDisplayName(tr("qmake build configuration:"));
    m_buildType->addOption(tr("Debug"));
    m_buildType->addOption(tr("Release"));

    m_userArgs = addAspect<ArgumentsAspect>();
    m_userArgs->setSettingsKey(QMAKE_ARGUMENTS_KEY);
    m_userArgs->setLabelText(tr("Additional arguments:"));

    m_effectiveCall = addAspect<StringAspect>();
    m_effectiveCall->setDisplayStyle(StringAspect::TextEditDisplay);
    m_effectiveCall->setLabelText(tr("Effective qmake call:"));
    m_effectiveCall->setReadOnly(true);
    m_effectiveCall->setUndoRedoEnabled(false);
    m_effectiveCall->setEnabled(true);

    auto updateSummary = [this] {
        BaseQtVersion *qtVersion = QtKitAspect::qtVersion(target()->kit());
        if (!qtVersion)
            return tr("<b>qmake:</b> No Qt version set. Cannot run qmake.");
        const QString program = qtVersion->qmakeFilePath().fileName();
        return tr("<b>qmake:</b> %1 %2").arg(program, project()->projectFilePath().fileName());
    };
    setSummaryUpdater(updateSummary);

    connect(target(), &Target::kitChanged, this, updateSummary);
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
    for (ProcessArgs::ConstArgIterator ait(userArguments()); ait.next(); ) {
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

    QString args = ProcessArgs::joinArgs(arguments);
    // User arguments
    ProcessArgs::addArgs(&args, userArguments());
    for (QString arg : qAsConst(m_extraArgs))
        ProcessArgs::addArgs(&args, arg);
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

    BaseQtVersion *version = QtKitAspect::qtVersion(kit);

    config.archConfig = QMakeStepConfig::targetArchFor(targetAbi, version);
    config.osType = QMakeStepConfig::osTypeFor(targetAbi, version);
    config.separateDebugInfo = qmakeBuildConfiguration()->separateDebugInfo();
    config.linkQmlDebuggingQQ2 = qmakeBuildConfiguration()->qmlDebugging();
    config.useQtQuickCompiler = qmakeBuildConfiguration()->useQtQuickCompiler();

    return config;
}

bool QMakeStep::init()
{
    if (!AbstractProcessStep::init())
        return false;

    m_wasSuccess = true;
    QmakeBuildConfiguration *qmakeBc = qmakeBuildConfiguration();
    const BaseQtVersion *qtVersion = QtKitAspect::qtVersion(kit());

    if (!qtVersion) {
        emit addOutput(tr("No Qt version configured."), BuildStep::OutputFormat::ErrorMessage);
        return false;
    }

    FilePath workingDirectory;

    if (qmakeBc->subNodeBuild())
        workingDirectory = qmakeBc->qmakeBuildSystem()->buildDir(qmakeBc->subNodeBuild()->filePath());
    else
        workingDirectory = qmakeBc->buildDirectory();

    m_qmakeCommand = CommandLine{qtVersion->qmakeFilePath(), allArguments(qtVersion), CommandLine::Raw};
    m_runMakeQmake = (qtVersion->qtVersion() >= QtVersionNumber(5, 0 ,0));

    // The Makefile is used by qmake and make on the build device, from that
    // perspective it is local.
    QString makefile = workingDirectory.path() + '/';

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

    processParameters()->setWorkingDirectory(workingDirectory);

    QmakeProFileNode *node = static_cast<QmakeProFileNode *>(qmakeBc->project()->rootProjectNode());
    if (qmakeBc->subNodeBuild())
        node = qmakeBc->subNodeBuild();
    QTC_ASSERT(node, return false);
    QString proFile = node->filePath().toString();

    Tasks tasks = qtVersion->reportIssues(proFile, workingDirectory.toString());
    Utils::sort(tasks);

    if (!tasks.isEmpty()) {
        bool canContinue = true;
        for (const Task &t : qAsConst(tasks)) {
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

    return true;
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

void QMakeStep::setUserArguments(const QString &arguments)
{
    m_userArgs->setArguments(arguments);
}

QStringList QMakeStep::extraArguments() const
{
    return m_extraArgs;
}

void QMakeStep::setExtraArguments(const QStringList &args)
{
    if (m_extraArgs != args) {
        m_extraArgs = args;
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
        ProcessArgs::addArg(&args, "-f");
        ProcessArgs::addArg(&args, makefile);
    }
    ProcessArgs::addArg(&args, "qmake_all");
    return args;
}

QString QMakeStep::effectiveQMakeCall() const
{
    BaseQtVersion *qtVersion = QtKitAspect::qtVersion(kit());
    QString qmake = qtVersion ? qtVersion->qmakeFilePath().toUserOutput() : QString();
    if (qmake.isEmpty())
        qmake = tr("<no Qt version>");
    QString make = makeCommand().toUserOutput();
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
    BaseQtVersion *qt = QtKitAspect::qtVersion(kit());
    QTC_ASSERT(qt, return QStringList());
    for (ProcessArgs::ConstArgIterator ait(allArguments(qt, ArgumentFlag::Expand)); ait.next(); ) {
        if (ait.isSimple())
            result << ait.value();
    }
    return result;
}

QString QMakeStep::userArguments() const
{
    return m_userArgs->arguments(macroExpander());
}

QString QMakeStep::mkspec() const
{
    QString additionalArguments = userArguments();
    ProcessArgs::addArgs(&additionalArguments, m_extraArgs);
    for (ProcessArgs::ArgIterator ait(&additionalArguments); ait.next(); ) {
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
    map.insert(QMAKE_FORCED_KEY, m_forced);
    map.insert(QMAKE_SELECTED_ABIS_KEY, m_selectedAbis);
    return map;
}

bool QMakeStep::fromMap(const QVariantMap &map)
{
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

QWidget *QMakeStep::createConfigWidget()
{
    abisLabel = new QLabel(tr("ABIs:"));
    abisLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignTop);

    abisListWidget = new QListWidget;

    Layouting::Form builder;
    builder.addRow(m_buildType);
    builder.addRow(m_userArgs);
    builder.addRow(m_effectiveCall);
    builder.addRow({abisLabel, abisListWidget});
    auto widget = builder.emerge(false);

    qmakeBuildConfigChanged();

    updateSummary();
    updateAbiWidgets();
    updateEffectiveQMakeCall();

    connect(m_userArgs, &BaseAspect::changed, widget, [this] {
        updateAbiWidgets();
        updateEffectiveQMakeCall();

        emit qmakeBuildConfiguration()->qmakeBuildConfigurationChanged();
        qmakeBuildSystem()->scheduleUpdateAllNowOrLater();
    });

    connect(m_buildType, &BaseAspect::changed,
            widget, [this] { buildConfigurationSelected(); });

    connect(qmakeBuildConfiguration(), &QmakeBuildConfiguration::qmlDebuggingChanged,
            widget, [this] {
        linkQmlDebuggingLibraryChanged();
        askForRebuild(tr("QML Debugging"));
    });

    connect(project(), &Project::projectLanguagesUpdated,
            widget, [this] { linkQmlDebuggingLibraryChanged(); });
    connect(target(), &Target::parsingFinished,
            widget, [this] { updateEffectiveQMakeCall(); });
    connect(qmakeBuildConfiguration(), &QmakeBuildConfiguration::useQtQuickCompilerChanged,
            widget, [this] { useQtQuickCompilerChanged(); });
    connect(qmakeBuildConfiguration(), &QmakeBuildConfiguration::separateDebugInfoChanged,
            widget, [this] { separateDebugInfoChanged(); });
    connect(qmakeBuildConfiguration(), &QmakeBuildConfiguration::qmakeBuildConfigurationChanged,
            widget, [this] { qmakeBuildConfigChanged(); });
    connect(target(), &Target::kitChanged,
            widget, [this] { qtVersionChanged(); });

    connect(abisListWidget, &QListWidget::itemChanged, this, [this] {
        abisChanged();
        if (QmakeBuildConfiguration *bc = qmakeBuildConfiguration())
            BuildManager::buildLists({bc->cleanSteps()});
    });

    VariableChooser::addSupportForChildWidgets(widget, macroExpander());

    return widget;
}

void QMakeStep::qtVersionChanged()
{
    updateAbiWidgets();
    updateEffectiveQMakeCall();
}

void QMakeStep::qmakeBuildConfigChanged()
{
    QmakeBuildConfiguration *bc = qmakeBuildConfiguration();
    bool debug = bc->qmakeBuildConfiguration() & BaseQtVersion::DebugBuild;
    m_ignoreChange = true;
    m_buildType->setValue(debug? 0 : 1);
    m_ignoreChange = false;
    updateAbiWidgets();
    updateEffectiveQMakeCall();
}

void QMakeStep::linkQmlDebuggingLibraryChanged()
{
    updateAbiWidgets();
    updateEffectiveQMakeCall();
}

void QMakeStep::useQtQuickCompilerChanged()
{
    updateAbiWidgets();
    updateEffectiveQMakeCall();
    askForRebuild(tr("Qt Quick Compiler"));
}

void QMakeStep::separateDebugInfoChanged()
{
    updateAbiWidgets();
    updateEffectiveQMakeCall();
    askForRebuild(tr("Separate Debug Information"));
}

static bool isIos(const Kit *k)
{
    const Id deviceType = DeviceTypeKitAspect::deviceTypeId(k);
    return deviceType == Ios::Constants::IOS_DEVICE_TYPE
           || deviceType == Ios::Constants::IOS_SIMULATOR_TYPE;
}

void QMakeStep::abisChanged()
{
    m_selectedAbis.clear();
    for (int i = 0; i < abisListWidget->count(); ++i) {
        auto item = abisListWidget->item(i);
        if (item->checkState() == Qt::CheckState::Checked)
            m_selectedAbis << item->text();
    }

    if (BaseQtVersion *qtVersion = QtKitAspect::qtVersion(target()->kit())) {
        if (qtVersion->hasAbi(Abi::LinuxOS, Abi::AndroidLinuxFlavor)) {
            const QString prefix = "ANDROID_ABIS=";
            QStringList args = m_extraArgs;
            for (auto it = args.begin(); it != args.end(); ++it) {
                if (it->startsWith(prefix)) {
                    args.erase(it);
                    break;
                }
            }
            if (!m_selectedAbis.isEmpty())
                args << prefix + '"' + m_selectedAbis.join(' ') + '"';
            setExtraArguments(args);

            buildSystem()->setProperty(Android::Constants::ANDROID_ABIS, m_selectedAbis);
        } else if (qtVersion->hasAbi(Abi::DarwinOS) && !isIos(target()->kit())) {
            const QString prefix = "QMAKE_APPLE_DEVICE_ARCHS=";
            QStringList args = m_extraArgs;
            for (auto it = args.begin(); it != args.end(); ++it) {
                if (it->startsWith(prefix)) {
                    args.erase(it);
                    break;
                }
            }
            QStringList archs;
            for (const QString &selectedAbi : qAsConst(m_selectedAbis)) {
                const auto abi = Abi::abiFromTargetTriplet(selectedAbi);
                if (abi.architecture() == Abi::X86Architecture)
                    archs << "x86_64";
                else if (abi.architecture() == Abi::ArmArchitecture)
                    archs << "arm64";
            }
            if (!archs.isEmpty())
                args << prefix + '"' + archs.join(' ') + '"';
            setExtraArguments(args);
        }
    }

    updateAbiWidgets();
    updateEffectiveQMakeCall();
}

void QMakeStep::buildConfigurationSelected()
{
    if (m_ignoreChange)
        return;
    QmakeBuildConfiguration *bc = qmakeBuildConfiguration();
    BaseQtVersion::QmakeBuildConfigs buildConfiguration = bc->qmakeBuildConfiguration();
    if (m_buildType->value() == 0) { // debug
        buildConfiguration = buildConfiguration | BaseQtVersion::DebugBuild;
    } else {
        buildConfiguration = buildConfiguration & ~BaseQtVersion::DebugBuild;
    }
    m_ignoreChange = true;
    bc->setQMakeBuildConfiguration(buildConfiguration);
    m_ignoreChange = false;

    updateAbiWidgets();
    updateEffectiveQMakeCall();
}

void QMakeStep::askForRebuild(const QString &title)
{
    auto *question = new QMessageBox(Core::ICore::dialogParent());
    question->setWindowTitle(title);
    question->setText(tr("The option will only take effect if the project is recompiled. Do you want to recompile now?"));
    question->setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    question->setModal(true);
    connect(question, &QDialog::finished, this, &QMakeStep::recompileMessageBoxFinished);
    question->show();
}

void QMakeStep::updateAbiWidgets()
{
    if (!abisLabel)
        return;

    BaseQtVersion *qtVersion = QtKitAspect::qtVersion(target()->kit());
    if (!qtVersion)
        return;

    const Abis abis = qtVersion->qtAbis();
    const bool enableAbisSelect = abis.size() > 1;
    abisLabel->setVisible(enableAbisSelect);
    abisListWidget->setVisible(enableAbisSelect);

    if (enableAbisSelect && abisListWidget->count() != abis.size()) {
        abisListWidget->clear();
        QStringList selectedAbis = m_selectedAbis;

        if (selectedAbis.isEmpty()) {
            if (qtVersion->hasAbi(Abi::LinuxOS, Abi::AndroidLinuxFlavor)) {
                // Prefer ARM for Android, prefer 32bit.
                for (const Abi &abi : abis) {
                    if (abi.param() == ProjectExplorer::Constants::ANDROID_ABI_ARMEABI_V7A)
                        selectedAbis.append(abi.param());
                }
                if (selectedAbis.isEmpty()) {
                    for (const Abi &abi : abis) {
                        if (abi.param() == ProjectExplorer::Constants::ANDROID_ABI_ARM64_V8A)
                            selectedAbis.append(abi.param());
                    }
                }
            } else if (qtVersion->hasAbi(Abi::DarwinOS) && !isIos(target()->kit()) && HostOsInfo::isRunningUnderRosetta()) {
                // Automatically select arm64 when running under Rosetta
                for (const Abi &abi : abis) {
                    if (abi.architecture() == Abi::ArmArchitecture)
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
}

void QMakeStep::updateEffectiveQMakeCall()
{
    m_effectiveCall->setValue(effectiveQMakeCall());
}

void QMakeStep::recompileMessageBoxFinished(int button)
{
    if (button == QMessageBox::Yes) {
        if (BuildConfiguration *bc = buildConfiguration())
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
    //: QMakeStep default display name
    setDisplayName(::QmakeProjectManager::QMakeStep::tr("qmake"));
    setFlags(BuildStepInfo::UniqueStep);
}

QMakeStepConfig::TargetArchConfig QMakeStepConfig::targetArchFor(const Abi &targetAbi, const BaseQtVersion *version)
{
    TargetArchConfig arch = NoArch;
    if (!version || version->type() != QtSupport::Constants::DESKTOPQT)
        return arch;
    if (targetAbi.os() == Abi::DarwinOS && targetAbi.binaryFormat() == Abi::MachOFormat) {
        if (targetAbi.architecture() == Abi::X86Architecture) {
            if (targetAbi.wordWidth() == 32)
                arch = X86;
            else if (targetAbi.wordWidth() == 64)
                arch = X86_64;
        } else if (targetAbi.architecture() == Abi::PowerPCArchitecture) {
            if (targetAbi.wordWidth() == 32)
                arch = PowerPC;
            else if (targetAbi.wordWidth() == 64)
                arch = PowerPC64;
        }
    }
    return arch;
}

QMakeStepConfig::OsType QMakeStepConfig::osTypeFor(const Abi &targetAbi, const BaseQtVersion *version)
{
    OsType os = NoOsType;
    const char IOSQT[] = "Qt4ProjectManager.QtVersion.Ios";
    if (!version || version->type() != IOSQT)
        return os;
    if (targetAbi.os() == Abi::DarwinOS && targetAbi.binaryFormat() == Abi::MachOFormat) {
        if (targetAbi.architecture() == Abi::X86Architecture)
            os = IphoneSimulator;
        else if (targetAbi.architecture() == Abi::ArmArchitecture)
            os = IphoneOS;
    }
    return os;
}

QStringList QMakeStepConfig::toArguments() const
{
    QStringList arguments;

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

} // QmakeProjectManager
