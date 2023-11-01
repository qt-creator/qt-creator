// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmakestep.h"

#include "qmakebuildconfiguration.h"
#include "qmakekitaspect.h"
#include "qmakenodes.h"
#include "qmakeparser.h"
#include "qmakeproject.h"
#include "qmakeprojectmanagerconstants.h"
#include "qmakeprojectmanagertr.h"
#include "qmakesettings.h"

#include <android/androidconstants.h>

#include <projectexplorer/buildmanager.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/gnumakeparser.h>
#include <projectexplorer/makestep.h>
#include <projectexplorer/processparameters.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>

#include <coreplugin/icore.h>
#include <coreplugin/icontext.h>
#include <qtsupport/qtkitaspect.h>
#include <qtsupport/qtversionmanager.h>
#include <qtsupport/qtsupportconstants.h>

#include <ios/iosconstants.h>

#include <utils/algorithm.h>
#include <utils/hostosinfo.h>
#include <utils/layoutbuilder.h>
#include <utils/process.h>
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

    buildType.setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);
    buildType.setDisplayName(Tr::tr("qmake build configuration:"));
    buildType.addOption(Tr::tr("Debug"));
    buildType.addOption(Tr::tr("Release"));

    userArguments.setMacroExpander(macroExpander());
    userArguments.setSettingsKey(QMAKE_ARGUMENTS_KEY);
    userArguments.setLabelText(Tr::tr("Additional arguments:"));

    effectiveCall.setDisplayStyle(StringAspect::TextEditDisplay);
    effectiveCall.setLabelText(Tr::tr("Effective qmake call:"));
    effectiveCall.setReadOnly(true);
    effectiveCall.setEnabled(true);

    auto updateSummary = [this] {
        QtVersion *qtVersion = QtKitAspect::qtVersion(target()->kit());
        if (!qtVersion)
            return Tr::tr("<b>qmake:</b> No Qt version set. Cannot run qmake.");
        const QString program = qtVersion->qmakeFilePath().fileName();
        return Tr::tr("<b>qmake:</b> %1 %2").arg(program, project()->projectFilePath().fileName());
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
QString QMakeStep::allArguments(const QtVersion *v, ArgumentFlags flags) const
{
    QTC_ASSERT(v, return QString());
    QmakeBuildConfiguration *bc = qmakeBuildConfiguration();
    QStringList arguments;
    if (bc->subNodeBuild())
        arguments << bc->subNodeBuild()->filePath().nativePath();
    else
        arguments << project()->projectFilePath().nativePath();

    if (v->qtVersion() < QVersionNumber(5, 0, 0))
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
    for (QString arg : std::as_const(m_extraArgs))
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

    QtVersion *version = QtKitAspect::qtVersion(kit);

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

    QmakeBuildConfiguration *qmakeBc = qmakeBuildConfiguration();
    const QtVersion *qtVersion = QtKitAspect::qtVersion(kit());

    if (!qtVersion) {
        emit addOutput(Tr::tr("No Qt version configured."), OutputFormat::ErrorMessage);
        return false;
    }

    FilePath workingDirectory;

    if (qmakeBc->subNodeBuild())
        workingDirectory = qmakeBc->qmakeBuildSystem()->buildDir(qmakeBc->subNodeBuild()->filePath());
    else
        workingDirectory = qmakeBc->buildDirectory();

    m_qmakeCommand = CommandLine{qtVersion->qmakeFilePath(), allArguments(qtVersion), CommandLine::Raw};
    m_runMakeQmake = (qtVersion->qtVersion() >= QVersionNumber(5, 0 ,0));

    // The Makefile is used by qmake and make on the build device, from that
    // perspective it is local.

    QString make;
    if (QmakeProFileNode *pro = qmakeBc->subNodeBuild()) {
        if (!pro->makefile().isEmpty())
            make = pro->makefile();
        else
            make = "Makefile";
    } else if (!qmakeBc->makefile().isEmpty()) {
        make = qmakeBc->makefile().path();
    } else {
        make = "Makefile";
    }

    FilePath makeFile = workingDirectory / make;

    if (m_runMakeQmake) {
        const FilePath make = makeCommand();
        if (make.isEmpty()) {
            emit addOutput(Tr::tr("Could not determine which \"make\" command to run. "
                                  "Check the \"make\" step in the build configuration."),
                           OutputFormat::ErrorMessage);
            return false;
        }
        m_makeCommand = CommandLine{make, makeArguments(makeFile.path()), CommandLine::Raw};
    } else {
        m_makeCommand = {};
    }

    // Check whether we need to run qmake
    if (m_forced || settings().alwaysRunQmake()
            || qmakeBc->compareToImportFrom(makeFile) != QmakeBuildConfiguration::MakefileMatches) {
        m_needToRunQMake = true;
    }
    m_forced = false;

    processParameters()->setWorkingDirectory(workingDirectory);

    QmakeProFileNode *node = static_cast<QmakeProFileNode *>(qmakeBc->project()->rootProjectNode());
    if (qmakeBc->subNodeBuild())
        node = qmakeBc->subNodeBuild();
    QTC_ASSERT(node, return false);

    const Tasks tasks = Utils::sorted(qtVersion->reportIssues(node->filePath(), workingDirectory));
    if (!tasks.isEmpty()) {
        bool canContinue = true;
        for (const Task &t : tasks) {
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

Tasking::GroupItem QMakeStep::runRecipe()
{
    using namespace Tasking;

    const auto onSetup = [this] {
        if (m_scriptTemplate)
            return SetupResult::StopWithDone;
        if (m_needToRunQMake)
            return SetupResult::Continue;
        emit addOutput(Tr::tr("Configuration unchanged, skipping qmake step."),
                       OutputFormat::NormalMessage);
        return SetupResult::StopWithDone;
    };

    const auto setupQMake = [this](Process &process) {
        m_outputFormatter->setLineParsers({new QMakeParser});
        ProcessParameters *pp = processParameters();
        pp->setCommandLine(m_qmakeCommand);
        return setupProcess(process) ? SetupResult::Continue : SetupResult::StopWithError;
    };

    const auto setupMakeQMake = [this](Process &process) {
        auto *parser = new GnuMakeParser;
        parser->addSearchDir(processParameters()->workingDirectory());
        m_outputFormatter->setLineParsers({parser});
        ProcessParameters *pp = processParameters();
        pp->setCommandLine(m_makeCommand);
        return setupProcess(process) ? SetupResult::Continue : SetupResult::StopWithError;
    };

    const auto onProcessDone = [this](const Process &process) { handleProcessDone(process); };

    const auto onDone = [this] {
        emit buildConfiguration()->buildDirectoryInitialized();
        m_needToRunQMake = false;
    };

    QList<GroupItem> processList = {onGroupSetup(onSetup),
                                    onGroupDone(onDone),
                                    ProcessTask(setupQMake, onProcessDone, onProcessDone)};
    if (m_runMakeQmake)
        processList << ProcessTask(setupMakeQMake, onProcessDone, onProcessDone);

    return Group(processList);
}

void QMakeStep::setForced(bool b)
{
    m_forced = b;
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
    return {};
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
    QtVersion *qtVersion = QtKitAspect::qtVersion(kit());
    FilePath qmake = qtVersion ? qtVersion->qmakeFilePath() : FilePath();
    if (qmake.isEmpty())
        qmake = FilePath::fromPathPart(Tr::tr("<no Qt version>"));
    FilePath make = makeCommand();
    if (make.isEmpty())
        make = FilePath::fromPathPart(Tr::tr("<no Make step found>"));

    QString result = qmake.toString();
    if (qtVersion) {
        QmakeBuildConfiguration *qmakeBc = qmakeBuildConfiguration();
        const FilePath makefile = qmakeBc ? qmakeBc->makefile() : FilePath();
        result += ' ' + allArguments(qtVersion, ArgumentFlag::Expand);
        if (qtVersion->qtVersion() >= QVersionNumber(5, 0, 0))
            result.append(QString(" && %1 %2").arg(make.path()).arg(makeArguments(makefile.path())));
    }
    return result;
}

QStringList QMakeStep::parserArguments()
{
    // NOTE: extra parser args placed before the other args intentionally
    QStringList result = m_extraParserArgs;
    QtVersion *qt = QtKitAspect::qtVersion(kit());
    QTC_ASSERT(qt, return {});
    for (ProcessArgs::ConstArgIterator ait(allArguments(qt, ArgumentFlag::Expand)); ait.next(); ) {
        if (ait.isSimple())
            result << ait.value();
    }
    return result;
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

void QMakeStep::toMap(Store &map) const
{
    AbstractProcessStep::toMap(map);
    map.insert(QMAKE_FORCED_KEY, m_forced);
    map.insert(QMAKE_SELECTED_ABIS_KEY, m_selectedAbis);
}

void QMakeStep::fromMap(const Store &map)
{
    m_forced = map.value(QMAKE_FORCED_KEY, false).toBool();
    m_selectedAbis = map.value(QMAKE_SELECTED_ABIS_KEY).toStringList();
    BuildStep::fromMap(map);
}

QWidget *QMakeStep::createConfigWidget()
{
    abisLabel = new QLabel(Tr::tr("ABIs:"));
    abisLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignTop);

    abisListWidget = new QListWidget;

    Layouting::Form builder;
    builder.addRow({buildType});
    builder.addRow({userArguments});
    builder.addRow({effectiveCall});
    builder.addRow({abisLabel, abisListWidget});
    builder.addItem(Layouting::noMargin);
    auto widget = builder.emerge();

    qmakeBuildConfigChanged();

    emit updateSummary();
    updateAbiWidgets();
    updateEffectiveQMakeCall();

    connect(&userArguments, &BaseAspect::changed, widget, [this] {
        updateAbiWidgets();
        updateEffectiveQMakeCall();

        emit qmakeBuildConfiguration()->qmakeBuildConfigurationChanged();
        qmakeBuildSystem()->scheduleUpdateAllNowOrLater();
    });

    connect(&buildType, &BaseAspect::changed,
            widget, [this] { buildConfigurationSelected(); });

    connect(qmakeBuildConfiguration(), &QmakeBuildConfiguration::qmlDebuggingChanged,
            widget, [this] {
        linkQmlDebuggingLibraryChanged();
        askForRebuild(Tr::tr("QML Debugging"));
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
        if (m_ignoreChanges.isLocked())
            return;
        abisChanged();
        if (QmakeBuildConfiguration *bc = qmakeBuildConfiguration())
            BuildManager::buildLists({bc->cleanSteps()});
    });

    connect(widget, &QObject::destroyed, this, [this] {
        abisLabel = nullptr;
        abisListWidget = nullptr;
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
    const bool debug = bc->qmakeBuildConfiguration() & QtVersion::DebugBuild;
    {
        const GuardLocker locker(m_ignoreChanges);
        buildType.setValue(debug ? 0 : 1);
    }
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
    askForRebuild(Tr::tr("Qt Quick Compiler"));
}

void QMakeStep::separateDebugInfoChanged()
{
    updateAbiWidgets();
    updateEffectiveQMakeCall();
    askForRebuild(Tr::tr("Separate Debug Information"));
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

    if (QtVersion *qtVersion = QtKitAspect::qtVersion(target()->kit())) {
        if (qtVersion->hasAbi(Abi::LinuxOS, Abi::AndroidLinuxFlavor)) {
            const QString prefix = QString("%1=").arg(Android::Constants::ANDROID_ABIS);
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
            buildSystem()->setProperty(Android::Constants::AndroidAbis, m_selectedAbis);
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
            for (const QString &selectedAbi : std::as_const(m_selectedAbis)) {
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
    if (m_ignoreChanges.isLocked())
        return;
    QmakeBuildConfiguration *bc = qmakeBuildConfiguration();
    QtVersion::QmakeBuildConfigs buildConfiguration = bc->qmakeBuildConfiguration();
    if (buildType() == 0) { // debug
        buildConfiguration = buildConfiguration | QtVersion::DebugBuild;
    } else {
        buildConfiguration = buildConfiguration & ~QtVersion::DebugBuild;
    }

    {
        const GuardLocker locker(m_ignoreChanges);
        bc->setQMakeBuildConfiguration(buildConfiguration);
    }

    updateAbiWidgets();
    updateEffectiveQMakeCall();
}

void QMakeStep::askForRebuild(const QString &title)
{
    auto *question = new QMessageBox(Core::ICore::dialogParent());
    question->setWindowTitle(title);
    question->setText(Tr::tr("The option will only take effect if the project is recompiled. Do you want to recompile now?"));
    question->setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    question->setModal(true);
    connect(question, &QDialog::finished, this, &QMakeStep::recompileMessageBoxFinished);
    question->show();
}

void QMakeStep::updateAbiWidgets()
{
    const GuardLocker locker(m_ignoreChanges);

    if (!abisLabel)
        return;

    QtVersion *qtVersion = QtKitAspect::qtVersion(target()->kit());
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
                // Prefer ARM/X86_64 for Android, prefer 64bit.
                for (const Abi &abi : abis) {
                    if (abi.param() == ProjectExplorer::Constants::ANDROID_ABI_ARM64_V8A) {
                        selectedAbis.append(abi.param());
                        break;
                    }
                }
                if (selectedAbis.isEmpty()) {
                    for (const Abi &abi : abis) {
                        if (abi.param() == ProjectExplorer::Constants::ANDROID_ABI_X86_64) {
                            selectedAbis.append(abi.param());
                            break;
                        }
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
    effectiveCall.setValue(effectiveQMakeCall());
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
    setDisplayName(::QmakeProjectManager::Tr::tr("qmake")); // Fully qualifying for lupdate
    setFlags(BuildStep::UniqueStep);
}

QMakeStepConfig::OsType QMakeStepConfig::osTypeFor(const Abi &targetAbi, const QtVersion *version)
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
