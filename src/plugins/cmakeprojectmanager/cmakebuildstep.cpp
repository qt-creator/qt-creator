// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cmakebuildstep.h"

#include "cmakebuildconfiguration.h"
#include "cmakebuildsystem.h"
#include "cmakekitaspect.h"
#include "cmakeparser.h"
#include "cmakeproject.h"
#include "cmakeprojectconstants.h"
#include "cmakeprojectmanagertr.h"
#include "cmaketool.h"

#include <android/androidconstants.h>

#include <baremetal/baremetalconstants.h>

#include <ios/iosconstants.h>

#include <webassembly/webassemblyconstants.h>

#include <coreplugin/find/itemviewfind.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/environmentwidget.h>
#include <projectexplorer/gnumakeparser.h>
#include <projectexplorer/kitaspects.h>
#include <projectexplorer/processparameters.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectexplorertr.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/target.h>
#include <projectexplorer/xcodebuildparser.h>

#include <utils/algorithm.h>
#include <utils/layoutbuilder.h>

#include <QListWidget>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QTreeView>
#include <QCheckBox>

using namespace Core;
using namespace ProjectExplorer;
using namespace Tasking;
using namespace Utils;

namespace CMakeProjectManager::Internal {

const char BUILD_TARGETS_KEY[] = "CMakeProjectManager.MakeStep.BuildTargets";
const char CMAKE_ARGUMENTS_KEY[] = "CMakeProjectManager.MakeStep.CMakeArguments";
const char TOOL_ARGUMENTS_KEY[] = "CMakeProjectManager.MakeStep.AdditionalArguments";
const char USE_STAGING_KEY[] = "CMakeProjectManager.MakeStep.UseStaging";
const char STAGING_DIR_KEY[] = "CMakeProjectManager.MakeStep.StagingDir";
const char IOS_AUTOMATIC_PROVISIONG_UPDATES_ARGUMENTS_KEY[] =
        "CMakeProjectManager.MakeStep.iOSAutomaticProvisioningUpdates";
const char CLEAR_SYSTEM_ENVIRONMENT_KEY[] = "CMakeProjectManager.MakeStep.ClearSystemEnvironment";
const char USER_ENVIRONMENT_CHANGES_KEY[] = "CMakeProjectManager.MakeStep.UserEnvironmentChanges";
const char BUILD_PRESET_KEY[] = "CMakeProjectManager.MakeStep.BuildPreset";

class ProjectParserTaskAdapter : public TaskAdapter<QPointer<Target>>
{
public:
    void start() final {
        Target *target = *task();
        if (!target) {
            emit done(false);
            return;
        }
        connect(target, &Target::parsingFinished, this, &TaskInterface::done);
    }
};

using ProjectParserTask = CustomTask<ProjectParserTaskAdapter>;

class CmakeProgressParser : public Utils::OutputLineParser
{
    Q_OBJECT

signals:
    void progress(int percentage);

private:
    Result handleLine(const QString &line, Utils::OutputFormat format) override
    {
        if (format != Utils::StdOutFormat)
            return Status::NotHandled;

        static const QRegularExpression percentProgress("^\\[\\s*(\\d*)%\\]");
        static const QRegularExpression ninjaProgress("^\\[\\s*(\\d*)/\\s*(\\d*)");

        QRegularExpressionMatch match = percentProgress.match(line);
        if (match.hasMatch()) {
            bool ok = false;
            const int percent = match.captured(1).toInt(&ok);
            if (ok)
                emit progress(percent);
            return Status::Done;
        }
        match = ninjaProgress.match(line);
        if (match.hasMatch()) {
            m_useNinja = true;
            bool ok = false;
            const int done = match.captured(1).toInt(&ok);
            if (ok) {
                const int all = match.captured(2).toInt(&ok);
                if (ok && all != 0) {
                    const int percent = static_cast<int>(100.0 * done / all);
                    emit progress(percent);
                }
            }
            return Status::Done;
        }
        return Status::NotHandled;
    }
    bool hasDetectedRedirection() const override { return m_useNinja; }

    // TODO: Shouldn't we know the backend in advance? Then we could merge this class
    //       with CmakeParser.
    bool m_useNinja = false;
};


// CmakeTargetItem

CMakeTargetItem::CMakeTargetItem(const QString &target, CMakeBuildStep *step, bool special)
    : m_target(target), m_step(step), m_special(special)
{
}

QVariant CMakeTargetItem::data(int column, int role) const
{
    if (column == 0) {
        if (role == Qt::DisplayRole) {
            if (m_target.isEmpty())
                return Tr::tr("Current executable");
            return m_target;
        }

        if (role == Qt::ToolTipRole) {
            if (m_target.isEmpty()) {
                return Tr::tr("Build the executable used in the active run "
                                          "configuration. Currently: %1")
                        .arg(m_step->activeRunConfigTarget());
            }
            return Tr::tr("Target: %1").arg(m_target);
        }

        if (role == Qt::CheckStateRole)
            return m_step->buildsBuildTarget(m_target) ? Qt::Checked : Qt::Unchecked;

        if (role == Qt::FontRole) {
            if (m_special) {
                QFont italics;
                italics.setItalic(true);
                return italics;
            }
        }
    }

    return QVariant();
}

bool CMakeTargetItem::setData(int column, const QVariant &data, int role)
{
    if (column == 0 && role == Qt::CheckStateRole) {
        m_step->setBuildsBuildTarget(m_target, data.value<Qt::CheckState>() == Qt::Checked);
        return true;
    }
    return TreeItem::setData(column, data, role);
}

Qt::ItemFlags CMakeTargetItem::flags(int) const
{
    return Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

// CMakeBuildStep

static QString initialStagingDir(Kit *kit)
{
    // Avoid actual file accesses.
    auto rg = QRandomGenerator::global();
    const qulonglong rand = rg->generate64();
    char buf[sizeof(rand)];
    memcpy(&buf, &rand, sizeof(rand));
    const QByteArray ba = QByteArray(buf, sizeof(buf)).toHex();
    IDeviceConstPtr buildDevice = BuildDeviceKitAspect::device(kit);
    if (buildDevice && buildDevice->type() == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE)
        return TemporaryDirectory::masterDirectoryPath() + "/staging-" + ba;
    return QString::fromUtf8("/tmp/Qt-Creator-staging-" + ba);
}

static bool supportsStageForInstallation(const Kit *kit)
{
    IDeviceConstPtr runDevice = DeviceKitAspect::device(kit);
    IDeviceConstPtr buildDevice = BuildDeviceKitAspect::device(kit);
    QTC_ASSERT(runDevice, return false);
    QTC_ASSERT(buildDevice, return false);
    return runDevice->id() != buildDevice->id()
           && runDevice->type() != Android::Constants::ANDROID_DEVICE_TYPE
           && runDevice->type() != Ios::Constants::IOS_DEVICE_TYPE
           && runDevice->type() != Ios::Constants::IOS_SIMULATOR_TYPE
           && runDevice->type() != BareMetal::Constants::BareMetalOsType
           && runDevice->type() != WebAssembly::Constants::WEBASSEMBLY_DEVICE_TYPE;
}

CMakeBuildStep::CMakeBuildStep(BuildStepList *bsl, Id id) :
    CMakeAbstractProcessStep(bsl, id)
{
    cmakeArguments.setSettingsKey(CMAKE_ARGUMENTS_KEY);
    cmakeArguments.setLabelText(Tr::tr("CMake arguments:"));
    cmakeArguments.setDisplayStyle(StringAspect::LineEditDisplay);

    toolArguments.setSettingsKey(TOOL_ARGUMENTS_KEY);
    toolArguments.setLabelText(Tr::tr("Tool arguments:"));
    toolArguments.setDisplayStyle(StringAspect::LineEditDisplay);

    useStaging.setSettingsKey(USE_STAGING_KEY);
    useStaging.setLabel(Tr::tr("Stage for installation"), BoolAspect::LabelPlacement::AtCheckBox);
    useStaging.setDefaultValue(supportsStageForInstallation(kit()));

    stagingDir.setSettingsKey(STAGING_DIR_KEY);
    stagingDir.setLabelText(Tr::tr("Staging directory:"));
    stagingDir.setDefaultValue(initialStagingDir(kit()));

    Kit *kit = buildConfiguration()->kit();
    if (CMakeBuildConfiguration::isIos(kit)) {
        useiOSAutomaticProvisioningUpdates.setDefaultValue(true);
        useiOSAutomaticProvisioningUpdates.setSettingsKey(
                  IOS_AUTOMATIC_PROVISIONG_UPDATES_ARGUMENTS_KEY);
        useiOSAutomaticProvisioningUpdates.setLabel(
                  Tr::tr("Enable automatic provisioning updates:"));
        useiOSAutomaticProvisioningUpdates.setToolTip(
                  Tr::tr("Tells xcodebuild to create and download a provisioning profile "
                     "if a valid one does not exist."));
    } else {
        useiOSAutomaticProvisioningUpdates.setVisible(false);
    }

    m_buildTargetModel.setHeader({Tr::tr("Target")});

    setBuildTargets({defaultBuildTarget()});
    auto *bs = qobject_cast<CMakeBuildSystem *>(buildSystem());
    if (bs && !bs->buildTargets().isEmpty())
        recreateBuildTargetsModel();

    setLowPriority();

    setCommandLineProvider([this] { return cmakeCommand(); });

    setEnvironmentModifier([this](Environment &env) {
        const QString ninjaProgressString = "[%f/%t "; // ninja: [33/100
        env.setupEnglishOutput();
        if (!env.expandedValueForKey("NINJA_STATUS").startsWith(ninjaProgressString))
            env.set("NINJA_STATUS", ninjaProgressString + "%o/sec] ");
        env.modify(m_userEnvironmentChanges);

        if (useStaging())
            env.set("DESTDIR", stagingDir().path());
    });

    connect(target(), &Target::parsingFinished, this, [this](bool success) {
        if (success) // Do not change when parsing failed.
            recreateBuildTargetsModel();
    });

    connect(target(), &Target::activeRunConfigurationChanged,
            this, &CMakeBuildStep::updateBuildTargetsModel);
}

void CMakeBuildStep::toMap(Utils::Store &map) const
{
    CMakeAbstractProcessStep::toMap(map);
    map.insert(BUILD_TARGETS_KEY, m_buildTargets);
    map.insert(QLatin1String(CLEAR_SYSTEM_ENVIRONMENT_KEY), m_clearSystemEnvironment);
    map.insert(QLatin1String(USER_ENVIRONMENT_CHANGES_KEY), EnvironmentItem::toStringList(m_userEnvironmentChanges));
    map.insert(QLatin1String(BUILD_PRESET_KEY), m_buildPreset);
}

void CMakeBuildStep::fromMap(const Utils::Store &map)
{
    setBuildTargets(map.value(BUILD_TARGETS_KEY).toStringList());

    m_clearSystemEnvironment = map.value(QLatin1String(CLEAR_SYSTEM_ENVIRONMENT_KEY))
                                   .toBool();
    m_userEnvironmentChanges = EnvironmentItem::fromStringList(
        map.value(QLatin1String(USER_ENVIRONMENT_CHANGES_KEY)).toStringList());

    updateAndEmitEnvironmentChanged();

    m_buildPreset = map.value(QLatin1String(BUILD_PRESET_KEY)).toString();

    BuildStep::fromMap(map);
}

bool CMakeBuildStep::init()
{
    if (!CMakeAbstractProcessStep::init())
        return false;

    if (m_buildTargets.contains(QString())) {
        RunConfiguration *rc = target()->activeRunConfiguration();
        if (!rc || rc->buildKey().isEmpty()) {
            emit addTask(BuildSystemTask(Task::Error,
                                         ::ProjectExplorer::Tr::tr(
                                    "You asked to build the current Run Configuration's build target only, "
                                    "but it is not associated with a build target. "
                                    "Update the Make Step in your build settings.")));
            emitFaultyConfigurationMessage();
            return false;
        }
    }

    setIgnoreReturnValue(m_buildTargets == QStringList(CMakeBuildStep::cleanTarget()));

    return true;
}

void CMakeBuildStep::setupOutputFormatter(Utils::OutputFormatter *formatter)
{
    CMakeParser *cmakeParser = new CMakeParser;
    CmakeProgressParser * const progressParser = new CmakeProgressParser;
    connect(progressParser, &CmakeProgressParser::progress, this, [this](int percent) {
        emit progress(percent, {});
    });
    formatter->addLineParser(progressParser);
    cmakeParser->setSourceDirectory(project()->projectDirectory());
    formatter->addLineParsers({cmakeParser, new GnuMakeParser});
    ToolChain *tc = ToolChainKitAspect::cxxToolChain(kit());
    OutputTaskParser *xcodeBuildParser = nullptr;
    if (tc && tc->targetAbi().os() == Abi::DarwinOS) {
        xcodeBuildParser = new XcodebuildParser;
        formatter->addLineParser(xcodeBuildParser);
        progressParser->setRedirectionDetector(xcodeBuildParser);
    }
    const QList<Utils::OutputLineParser *> additionalParsers = kit()->createOutputParsers();
    for (Utils::OutputLineParser * const p : additionalParsers)
        p->setRedirectionDetector(progressParser);
    formatter->addLineParsers(additionalParsers);
    formatter->addSearchDir(processParameters()->effectiveWorkingDirectory());
    CMakeAbstractProcessStep::setupOutputFormatter(formatter);
}

GroupItem CMakeBuildStep::runRecipe()
{
    const auto onParserSetup = [this](QPointer<Target> &parseTarget) {
        // Make sure CMake state was written to disk before trying to build:
        auto bs = qobject_cast<CMakeBuildSystem *>(buildSystem());
        QTC_ASSERT(bs, return SetupResult::StopWithError);
        QString message;
        if (bs->persistCMakeState())
            message = Tr::tr("Persisting CMake state...");
        else if (bs->isWaitingForParse())
            message = Tr::tr("Running CMake in preparation to build...");
        else
            return SetupResult::StopWithDone;
        emit addOutput(message, OutputFormat::NormalMessage);
        parseTarget = target();
        return SetupResult::Continue;
    };
    const auto onParserError = [this](const QPointer<Target> &) {
        emit addOutput(Tr::tr("Project did not parse successfully, cannot build."),
                       OutputFormat::ErrorMessage);
    };
    const auto onEnd = [this] {
        updateDeploymentData();
    };
    const Group root {
        ignoreReturnValue() ? finishAllAndDone : stopOnError,
        ProjectParserTask(onParserSetup, {}, onParserError),
        defaultProcessTask(),
        onGroupDone(onEnd),
        onGroupError(onEnd)
    };
    return root;
}

QString CMakeBuildStep::defaultBuildTarget() const
{
    const BuildStepList *const bsl = stepList();
    QTC_ASSERT(bsl, return {});
    const Utils::Id parentId = bsl->id();
    if (parentId == ProjectExplorer::Constants::BUILDSTEPS_CLEAN)
        return cleanTarget();
    if (parentId == ProjectExplorer::Constants::BUILDSTEPS_DEPLOY)
        return installTarget();
    return allTarget();
}

bool CMakeBuildStep::isCleanStep() const
{
    const BuildStepList *const bsl = stepList();
    QTC_ASSERT(bsl, return false);
    const Utils::Id parentId = bsl->id();
    return parentId == ProjectExplorer::Constants::BUILDSTEPS_CLEAN;
}

QStringList CMakeBuildStep::buildTargets() const
{
    return m_buildTargets;
}

bool CMakeBuildStep::buildsBuildTarget(const QString &target) const
{
    return m_buildTargets.contains(target);
}

void CMakeBuildStep::setBuildsBuildTarget(const QString &target, bool on)
{
    QStringList targets = m_buildTargets;
    if (on && !m_buildTargets.contains(target))
        targets.append(target);
    if (!on)
        targets.removeAll(target);
    setBuildTargets(targets);
}

void CMakeBuildStep::setBuildTargets(const QStringList &buildTargets)
{
    if (buildTargets.isEmpty())
        m_buildTargets = QStringList(defaultBuildTarget());
    else
        m_buildTargets = buildTargets;
    updateBuildTargetsModel();
}

CommandLine CMakeBuildStep::cmakeCommand() const
{
    CommandLine cmd{cmakeExecutable()};

    FilePath buildDirectory = ".";
    if (buildConfiguration())
        buildDirectory = buildConfiguration()->buildDirectory();

    cmd.addArgs({"--build", buildDirectory.path()});

    cmd.addArg("--target");
    cmd.addArgs(Utils::transform(m_buildTargets, [this](const QString &s) {
        if (s.isEmpty()) {
            if (RunConfiguration *rc = target()->activeRunConfiguration())
                return rc->buildKey();
        }
        return s;
    }));
    if (useStaging())
        cmd.addArg("install");

    auto bs = qobject_cast<CMakeBuildSystem *>(buildSystem());
    if (bs && bs->isMultiConfigReader()) {
        cmd.addArg("--config");
        if (m_configuration)
            cmd.addArg(m_configuration.value());
        else
            cmd.addArg(bs->cmakeBuildType());
    }

    cmd.addArgs(cmakeArguments(), CommandLine::Raw);

    bool toolArgumentsSpecified = false;
    if (!toolArguments().isEmpty()) {
        cmd.addArg("--");
        cmd.addArgs(toolArguments(), CommandLine::Raw);
        toolArgumentsSpecified = true;
    }

    if (useiOSAutomaticProvisioningUpdates()) {
        // Only add the double dash if it wasn't added before.
        if (!toolArgumentsSpecified)
            cmd.addArg("--");
        cmd.addArgs("-allowProvisioningUpdates", CommandLine::Raw);
    }

    return cmd;
}

QString CMakeBuildStep::cleanTarget() const
{
    return QString("clean");
}

QString CMakeBuildStep::allTarget() const
{
    return m_allTarget;
}

QString CMakeBuildStep::installTarget() const
{
    return m_installTarget;
}

QStringList CMakeBuildStep::specialTargets(bool allCapsTargets)
{
    if (!allCapsTargets)
        return {"all", "clean", "install", "install/strip", "package", "test"};
    else
        return {"ALL_BUILD", "clean", "INSTALL", "PACKAGE", "RUN_TESTS"};
}

QString CMakeBuildStep::activeRunConfigTarget() const
{
    RunConfiguration *rc = target()->activeRunConfiguration();
    return rc ? rc->buildKey() : QString();
}

void CMakeBuildStep::setBuildPreset(const QString &preset)
{
    m_buildPreset = preset;
}

QWidget *CMakeBuildStep::createConfigWidget()
{
    auto updateDetails = [this] {
        ProcessParameters param;
        setupProcessParameters(&param);
        param.setCommandLine(cmakeCommand());

        QString summaryText = param.summary(displayName());

        stagingDir.setEnabled(useStaging());
        if (useStaging()) {
            //: Stage (for installation) at <staging_dir> for <installation_dir>
            summaryText.append(
                "; "
                + Tr::tr("Stage at %2 for %3").arg(stagingDir().path(), currentInstallPrefix()));
        }

        if (!m_buildPreset.isEmpty()) {
            const CMakeProject *cp = static_cast<const CMakeProject *>(project());

            const auto buildPresets = cp->presetsData().buildPresets;
            const PresetsDetails::BuildPreset preset
                = Utils::findOrDefault(buildPresets, [this](const PresetsDetails::BuildPreset &bp) {
                      return bp.name == m_buildPreset;
                  });

            const QString presetDisplayName = preset.displayName ? preset.displayName.value()
                                                                 : preset.name;
            if (!presetDisplayName.isEmpty())
                summaryText.append(QString("<br><b>Preset</b>: %1").arg(presetDisplayName));
        }

        setSummaryText(summaryText);
    };

    setDisplayName(Tr::tr("Build", "ConfigWidget display name."));

    auto buildTargetsView = new QTreeView;
    buildTargetsView->setMinimumHeight(200);
    buildTargetsView->setModel(&m_buildTargetModel);
    buildTargetsView->setRootIsDecorated(false);
    buildTargetsView->setHeaderHidden(true);

    auto frame = ItemViewFind::createSearchableWrapper(buildTargetsView,
                                                       ItemViewFind::LightColored);

    auto createAndAddEnvironmentWidgets = [this](Layouting::Form &builder) {
        auto clearBox = new QCheckBox(Tr::tr("Clear system environment"));
        clearBox->setChecked(useClearEnvironment());

        auto envWidget = new EnvironmentWidget(nullptr, EnvironmentWidget::TypeLocal, clearBox);
        envWidget->setBaseEnvironment(baseEnvironment());
        envWidget->setBaseEnvironmentText(baseEnvironmentText());
        envWidget->setUserChanges(userEnvironmentChanges());

        connect(envWidget, &EnvironmentWidget::userChangesChanged, this, [this, envWidget] {
            setUserEnvironmentChanges(envWidget->userChanges());
        });

        connect(clearBox, &QAbstractButton::toggled, this, [this, envWidget](bool checked) {
            setUseClearEnvironment(checked);
            envWidget->setBaseEnvironment(baseEnvironment());
            envWidget->setBaseEnvironmentText(baseEnvironmentText());
        });

        connect(this, &CMakeBuildStep::environmentChanged, this, [this, envWidget] {
            envWidget->setBaseEnvironment(baseEnvironment());
            envWidget->setBaseEnvironmentText(baseEnvironmentText());
        });

        builder.addRow({clearBox});
        builder.addRow({envWidget});
    };

    Layouting::Form builder;
    builder.addRow({cmakeArguments});
    builder.addRow({toolArguments});
    builder.addRow({useStaging});
    builder.addRow({stagingDir});
    builder.addRow({useiOSAutomaticProvisioningUpdates});

    builder.addRow({new QLabel(Tr::tr("Targets:")), frame});

    if (!isCleanStep() && !m_buildPreset.isEmpty())
        createAndAddEnvironmentWidgets(builder);

    builder.addItem(Layouting::noMargin);
    auto widget = builder.emerge();

    updateDetails();

    connect(&cmakeArguments, &BaseAspect::changed, this, updateDetails);
    connect(&toolArguments, &BaseAspect::changed, this, updateDetails);
    connect(&useStaging, &BaseAspect::changed, this, updateDetails);
    connect(&stagingDir, &BaseAspect::changed, this, updateDetails);
    connect(&useiOSAutomaticProvisioningUpdates, &BaseAspect::changed, this, updateDetails);

    connect(ProjectExplorerPlugin::instance(), &ProjectExplorerPlugin::settingsChanged,
            this, updateDetails);

    connect(buildConfiguration(), &BuildConfiguration::environmentChanged,
            this, updateDetails);

    connect(this, &CMakeBuildStep::buildTargetsChanged, widget, updateDetails);

    return widget;
}

void CMakeBuildStep::recreateBuildTargetsModel()
{
    auto addItem = [this](const QString &target, bool special = false) {
        auto item = new CMakeTargetItem(target, this, special);
        m_buildTargetModel.rootItem()->appendChild(item);
    };

    m_buildTargetModel.clear();

    auto bs = qobject_cast<CMakeBuildSystem *>(buildSystem());
    QStringList targetList = bs ? bs->buildTargetTitles() : QStringList();

    bool usesAllCapsTargets = bs ? bs->usesAllCapsTargets() : false;
    if (usesAllCapsTargets) {
        m_allTarget = "ALL_BUILD";
        m_installTarget = "INSTALL";

        int idx = m_buildTargets.indexOf(QString("all"));
        if (idx != -1)
            m_buildTargets[idx] = QString("ALL_BUILD");
        idx = m_buildTargets.indexOf(QString("install"));
        if (idx != -1)
            m_buildTargets[idx] = QString("INSTALL");
    }
    targetList.removeDuplicates();

    addItem(QString(), true);

    // Remove the targets that do not exist in the build system
    // This can result when selected targets get renamed
    if (!targetList.empty()) {
        Utils::erase(m_buildTargets, [targetList](const QString &bt) {
            return !bt.isEmpty() /* "current executable" */ && !targetList.contains(bt);
        });
        if (m_buildTargets.empty())
            m_buildTargets.push_back(m_allTarget);
    }

    for (const QString &buildTarget : std::as_const(targetList))
        addItem(buildTarget, specialTargets(usesAllCapsTargets).contains(buildTarget));

    updateBuildTargetsModel();
}

void CMakeBuildStep::updateBuildTargetsModel()
{
    emit m_buildTargetModel.layoutChanged();
    emit buildTargetsChanged();
}

void CMakeBuildStep::setConfiguration(const QString &configuration)
{
    m_configuration = configuration;
}

void CMakeBuildStep::setToolArguments(const QStringList &nativeToolArguments)
{
    toolArguments.setValue(nativeToolArguments.join(" "));
}

void CMakeBuildStep::setCMakeArguments(const QStringList &cmakeArgs)
{
    cmakeArguments.setValue(cmakeArgs.join(" "));
}

Environment CMakeBuildStep::environment() const
{
    return m_environment;
}

void CMakeBuildStep::setUserEnvironmentChanges(const Utils::EnvironmentItems &diff)
{
    if (m_userEnvironmentChanges == diff)
        return;
    m_userEnvironmentChanges = diff;
    updateAndEmitEnvironmentChanged();
}

EnvironmentItems CMakeBuildStep::userEnvironmentChanges() const
{
    return m_userEnvironmentChanges;
}

bool CMakeBuildStep::useClearEnvironment() const
{
    return m_clearSystemEnvironment;
}

void CMakeBuildStep::setUseClearEnvironment(bool b)
{
    if (useClearEnvironment() == b)
        return;
    m_clearSystemEnvironment = b;
    updateAndEmitEnvironmentChanged();
}

void CMakeBuildStep::updateAndEmitEnvironmentChanged()
{
    Environment env = baseEnvironment();
    env.modify(userEnvironmentChanges());
    if (env == m_environment)
        return;
    m_environment = env;
    emit environmentChanged();
}

Environment CMakeBuildStep::baseEnvironment() const
{
    Environment result;
    if (!useClearEnvironment()) {
        ProjectExplorer::IDevice::ConstPtr devicePtr = BuildDeviceKitAspect::device(kit());
        result = devicePtr ? devicePtr->systemEnvironment() : Environment::systemEnvironment();
    }
    buildConfiguration()->addToEnvironment(result);
    kit()->addToBuildEnvironment(result);
    result.modify(project()->additionalEnvironment());
    return result;
}

QString CMakeBuildStep::baseEnvironmentText() const
{
    if (useClearEnvironment())
        return Tr::tr("Clean Environment");
    else
        return Tr::tr("System Environment");
}

QString CMakeBuildStep::currentInstallPrefix() const
{
    auto bs = qobject_cast<CMakeBuildSystem *>(buildSystem());
    QTC_ASSERT(bs, return {});
    const CMakeConfig config = bs->configurationFromCMake();
    return QString::fromUtf8(config.valueOf("CMAKE_INSTALL_PREFIX"));
}

FilePath CMakeBuildStep::cmakeExecutable() const
{
    CMakeTool *tool = CMakeKitAspect::cmakeTool(kit());
    return tool ? tool->cmakeExecutable() : FilePath();
}

void CMakeBuildStep::updateDeploymentData()
{
    if (!useStaging())
        return;

    QString install = currentInstallPrefix();
    FilePath rootDir = cmakeExecutable().withNewPath(stagingDir().path());
    Q_UNUSED(install);

    DeploymentData deploymentData;
    deploymentData.setLocalInstallRoot(rootDir);

    IDeviceConstPtr device = BuildDeviceKitAspect::device(buildSystem()->kit());

    const auto appFileNames = transform<QSet<QString>>(buildSystem()->applicationTargets(),
           [](const BuildTargetInfo &appTarget) { return appTarget.targetFilePath.fileName(); });

    auto handleFile = [&appFileNames, rootDir, &deploymentData, device](const FilePath &filePath) {
        const DeployableFile::Type type = appFileNames.contains(filePath.fileName())
            ? DeployableFile::TypeExecutable
            : DeployableFile::TypeNormal;

        FilePath targetDirPath = filePath.parentDir().relativePathFrom(rootDir);

        const FilePath targetDir = device->rootPath().pathAppended(targetDirPath.path());
        deploymentData.addFile(filePath, targetDir.nativePath(), type);
        return IterationPolicy::Continue;
    };

    rootDir.iterateDirectory(handleFile,
                             {{}, QDir::Files | QDir::Hidden, QDirIterator::Subdirectories});

    buildSystem()->setDeploymentData(deploymentData);
}

// CMakeBuildStepFactory

CMakeBuildStepFactory::CMakeBuildStepFactory()
{
    registerStep<CMakeBuildStep>(Constants::CMAKE_BUILD_STEP_ID);
    setDisplayName(Tr::tr("CMake Build", "Display name for CMakeProjectManager::CMakeBuildStep id."));
    setSupportedProjectType(Constants::CMAKE_PROJECT_ID);
}

} // CMakeProjectManager::Internal

#include <cmakebuildstep.moc>
