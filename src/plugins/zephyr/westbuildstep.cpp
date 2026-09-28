// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "westbuildstep.h"

#include "zephyrconstants.h"
#include "zephyrsettings.h"
#include "zephyrtr.h"

#include <projectexplorer/abstractprocessstep.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/processparameters.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/target.h>
#include <projectexplorer/ioutputparser.h>
#include <projectexplorer/task.h>

#include <qtsupport/qtkitaspect.h>
#include <qtsupport/baseqtversion.h>

#include <utils/algorithm.h>
#include <utils/aspects.h>
#include <utils/layoutbuilder.h>
#include <utils/outputformatter.h>
#include <utils/pathchooser.h>
#include <utils/result.h>
#include <utils/terminalhooks.h>

#include <QDesktopServices>
#include <QDir>
#include <QSettings>

#ifdef WITH_TESTS
#include <QTemporaryDir>
#include <QTest>
#endif

using namespace ProjectExplorer;
using namespace QtTaskTree;
using namespace Utils;

namespace Zephyr::Internal {

static const char SDK_INSTALL_SCHEME[] = "zephyr-sdk-install";

class WestSdkInstaller : public QObject
{
    Q_OBJECT

public:
    WestSdkInstaller()
    {
        QDesktopServices::setUrlHandler(SDK_INSTALL_SCHEME, this, "install");
    }

    ~WestSdkInstaller()
    {
        QDesktopServices::unsetUrlHandler(SDK_INSTALL_SCHEME);
    }

public slots:
    void install(const QUrl &)
    {
        const FilePath ws = settings().workspaceDir();
        const FilePath west = settings().westFilePath();
        CommandLine cmd;
        if (ws.osType() == OsTypeWindows) {
            cmd = {ws.findCmdExe(), {"/k", west.nativePath() + " sdk install"}};
        } else {
            cmd = {ws.withNewPath("/bin/bash"),
                   {"-c", west.nativePath() + " sdk install; exec $SHELL"}};
        }
        Terminal::Hooks::instance().openTerminal({cmd, ws, std::nullopt});
    }
};

class WestOutputParser final : public OutputTaskParser
{
public:
    explicit WestOutputParser(const FilePath &workspaceDir)
        : m_workspaceDir(workspaceDir) {}

private:
    Result handleLine(const QString &line, OutputFormat format) final
    {
        if (format != StdErrFormat)
            return Status::NotHandled;
        if (line.contains("Zephyr-sdk") && line.contains("Could not find")) {
            BuildSystemTask task(Task::Error,
                Tr::tr("Zephyr SDK (toolchain) not installed."));
            const QString url = QString(SDK_INSTALL_SCHEME) + ":install";
            task.addLinkDetail(url,
                Tr::tr("Run \"west sdk install\" in \"%1\"")
                    .arg(m_workspaceDir.toUserOutput()));
            scheduleTask(task, 1, 0);
            return Status::Done;
        }
        return Status::NotHandled;
    }

    FilePath m_workspaceDir;
};

static QString boardFromWestConfig(const FilePath &workspaceDir)
{
    if (workspaceDir.isEmpty())
        return {};

    const FilePath configFile = workspaceDir.pathAppended(".west/config");
    if (!configFile.isReadableFile())
        return {};

    QSettings cfg(configFile.toFSPathString(), QSettings::IniFormat);
    QVariant board = cfg.value("build/board");
    return board.toString();
}

// West does not notice when a configured build directory changes between a single image
// and sysbuild, and a plain reconfigure fails on the other CMake source directory.
// Without an explicit choice west takes the mode from its own configuration.
static bool sysbuildSwitched(const FilePath &buildDir, const QString &sysbuildChoice)
{
    if (sysbuildChoice.isEmpty() || !(buildDir / "CMakeCache.txt").exists())
        return false;
    return (buildDir / "domains.yaml").exists() != (sysbuildChoice == "--sysbuild");
}

// West re-runs CMake only when it is given options that affect it, and Zephyr keeps
// command line Kconfig values in its cache. So when these options change, force a
// reconfigure and clear what the previous ones left behind.
static QStringList reconfigureArgs(const QStringList &configureArgs,
                                   const QStringList &lastConfigureArgs)
{
    if (configureArgs == lastConfigureArgs)
        return {};
    QStringList args{"--cmake", "--cmake-opt=-UCLI_CONFIG_*_OPTIMIZATIONS"};
    const auto hasArg = [&configureArgs](const QString &prefix) {
        return Utils::anyOf(configureArgs, [&prefix](const QString &arg) {
            return arg.startsWith(prefix);
        });
    };
    if (!hasArg("--extra-conf="))
        args.append("--cmake-opt=-DEXTRA_CONF_FILE=");
    if (!hasArg("--extra-dtc-overlay="))
        args.append("--cmake-opt=-DEXTRA_DTC_OVERLAY_FILE=");
    return args;
}

class WestBuildStep final : public AbstractProcessStep
{
public:
    WestBuildStep(BuildStepList *bsl, Id id)
        : AbstractProcessStep(bsl, id)
    {
        m_board.setSettingsKey("Zephyr.WestBuildStep.Board");
        m_board.setLabelText(Tr::tr("Board:"));
        m_board.setDisplayStyle(StringAspect::LineEditDisplay);
        m_board.setPlaceHolderText(Tr::tr("For example, qemu_x86"));
        m_board.setDefaultValue(boardFromWestConfig(settings().workspaceDir()));

        m_sysbuild.setSettingsKey("Zephyr.WestBuildStep.Sysbuild");
        m_sysbuild.setLabelText(Tr::tr("Sysbuild:"));
        m_sysbuild.setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);
        m_sysbuild.setUseDataAsSavedValue();
        m_sysbuild.addOption({Tr::tr("Workspace Default"), {}, QString()});
        m_sysbuild.addOption({Tr::tr("Enabled"), {}, QString("--sysbuild")});
        m_sysbuild.addOption({Tr::tr("Disabled"), {}, QString("--no-sysbuild")});
        m_sysbuild.setToolTip(Tr::tr("Build the application together with its bootloader "
                                     "and other images. The workspace default is the "
                                     "build.sysbuild setting of west."));

        m_optimization.setSettingsKey("Zephyr.WestBuildStep.Optimization");
        m_optimization.setLabelText(Tr::tr("Optimization:"));
        m_optimization.setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);
        m_optimization.setUseDataAsSavedValue();
        m_optimization.addOption({Tr::tr("Project Default"), {}, QString()});
        m_optimization.addOption({Tr::tr("Debug"), {}, QString("CONFIG_DEBUG_OPTIMIZATIONS")});
        m_optimization.addOption({Tr::tr("Size"), {}, QString("CONFIG_SIZE_OPTIMIZATIONS")});
        m_optimization.addOption({Tr::tr("Speed"), {}, QString("CONFIG_SPEED_OPTIMIZATIONS")});
        m_optimization.addOption({Tr::tr("None"), {}, QString("CONFIG_NO_OPTIMIZATIONS")});

        m_extraConfFiles.setSettingsKey("Zephyr.WestBuildStep.ExtraConfFiles");
        m_extraConfFiles.setLabelText(Tr::tr("Extra Kconfig fragments:"));

        m_extraOverlays.setSettingsKey("Zephyr.WestBuildStep.ExtraOverlays");
        m_extraOverlays.setLabelText(Tr::tr("Extra devicetree overlays:"));

        m_extraArgs.setSettingsKey("Zephyr.WestBuildStep.ExtraArgs");
        m_extraArgs.setLabelText(Tr::tr("Extra arguments:"));
        m_extraArgs.setDisplayStyle(StringAspect::LineEditDisplay);

        setCommandLineProvider([this] { return westCommand(); });
        setWorkingDirectoryProvider([this]() -> FilePath {
            const FilePath ws = settings().workspaceDir();
            return ws.isEmpty() ? project()->projectDirectory() : ws;
        });
        setUseEnglishOutput();
    }

    QWidget *createConfigWidget() final
    {
        auto updateDetails = [this] {
            ProcessParameters param;
            setupProcessParameters(&param);
            setSummaryText(param.summary(displayName()));
        };

        setDisplayName(Tr::tr("West Build"));

        updateDetails();

        m_board.addOnChanged(this, updateDetails);
        m_sysbuild.addOnChanged(this, updateDetails);
        m_optimization.addOnChanged(this, updateDetails);
        m_extraConfFiles.addOnChanged(this, updateDetails);
        m_extraOverlays.addOnChanged(this, updateDetails);
        m_extraArgs.addOnChanged(this, updateDetails);

        using namespace Layouting;
        return Form {
            noMargin,
            m_board, br,
            m_sysbuild, br,
            m_optimization, br,
            m_extraConfFiles, br,
            m_extraOverlays, br,
            m_extraArgs
        }.emerge();
    }

    void setupOutputFormatter(OutputFormatter *formatter) final
    {
        formatter->addLineParser(new WestOutputParser(settings().workspaceDir()));
        AbstractProcessStep::setupOutputFormatter(formatter);
    }

private:
    bool init() final
    {
        m_appliedConfigureArgs = configureArgs();
        m_appliedStampFile = stampFile();
        m_reconfigures = needsPristine()
            || !reconfigureArgs(m_appliedConfigureArgs, lastConfigureArgs()).isEmpty();
        return AbstractProcessStep::init();
    }

    GroupItem runRecipe() final
    {
        const auto writeStamp = [this](const QStringList &applied) {
            if (!m_appliedStampFile.parentDir().exists())
                return;
            const Result<qint64> res
                = m_appliedStampFile.writeFileContents(applied.join('\n').toUtf8());
            if (!res)
                emit addOutput(res.error(), OutputFormat::ErrorMessage);
        };
        // Until the build succeeds, the CMake cache may hold the new arguments or the old.
        const QStringList incomplete = m_appliedConfigureArgs + QStringList{"<incomplete>"};
        const auto onSetup = [this, writeStamp, incomplete] {
            if (m_reconfigures)
                writeStamp(incomplete);
        };
        const auto onDone = [this, writeStamp, incomplete](DoneWith result) {
            if (result == DoneWith::Success)
                writeStamp(m_appliedConfigureArgs);
            else if (m_reconfigures)
                writeStamp(incomplete); // A pristine build removed the earlier one.
        };
        return Group { onGroupSetup(onSetup), onGroupDone(onDone), defaultProcessTask() };
    }

    FilePath stampFile() const
    {
        return buildConfiguration()->buildDirectory() / ".qtc_west_configure";
    }

    bool needsPristine() const
    {
        return sysbuildSwitched(buildConfiguration()->buildDirectory(),
                                m_sysbuild.itemValue().toString());
    }

    QStringList lastConfigureArgs() const
    {
        const QByteArray stamp = stampFile().fileContents().value_or(QByteArray());
        return QString::fromUtf8(stamp).split('\n', Qt::SkipEmptyParts);
    }

    QStringList configureArgs() const
    {
        QStringList args;
        for (const FilePath &file : m_extraConfFiles())
            args.append("--extra-conf=" + file.nativePath());
        for (const FilePath &file : m_extraOverlays())
            args.append("--extra-dtc-overlay=" + file.nativePath());
        const QString optimization = m_optimization.itemValue().toString();
        if (!optimization.isEmpty())
            args.append("--cmake-opt=-D" + optimization + "=y");
        return args;
    }

    CommandLine westCommand() const
    {
        const FilePath projectDir = project()->projectDirectory();
        const FilePath ws = settings().workspaceDir();

        CommandLine cmd{settings().westFilePath()};
        cmd.addArg("build");
        if (!m_board().isEmpty()) {
            cmd.addArg("-b");
            cmd.addArg(m_board());
        }
        cmd.addArg("-d");
        cmd.addArg(buildConfiguration()->buildDirectory().nativePath());
        if (!ws.isEmpty() && ws != projectDir) {
            cmd.addArg("--source-dir");
            cmd.addArg(projectDir.nativePath());
        }
        if (const QString sysbuild = m_sysbuild.itemValue().toString(); !sysbuild.isEmpty())
            cmd.addArg(sysbuild);
        const QStringList configure = configureArgs();
        if (needsPristine())
            cmd.addArg("--pristine=always");
        else
            cmd.addArgs(reconfigureArgs(configure, lastConfigureArgs()));
        cmd.addArgs(configure);
        cmd.addArgs(m_extraArgs(), CommandLine::Raw);
        return cmd;
    }

    StringAspect m_board{this};
    SelectionAspect m_sysbuild{this};
    SelectionAspect m_optimization{this};
    FilePathListAspect m_extraConfFiles{this};
    FilePathListAspect m_extraOverlays{this};
    StringAspect m_extraArgs{this};

    QStringList m_appliedConfigureArgs;
    FilePath m_appliedStampFile;
    bool m_reconfigures = false;
};

class WestBuildStepFactory final : public BuildStepFactory
{
public:
    WestBuildStepFactory()
    {
        registerStep<WestBuildStep>(Constants::WEST_BUILD_STEP_ID);
        setDisplayName(Tr::tr("West Build"));
        setSupportedStepLists({ProjectExplorer::Constants::BUILDSTEPS_BUILD});
    }
};

static FilePath qmlProjectExporterFromKit(const Kit *kit)
{
    const QtSupport::QtVersion *qt = QtSupport::QtKitAspect::qtVersion(kit);
    if (!qt)
        return {};
    const FilePath candidate = qt->hostBinPath()
                                   .pathAppended("qmlprojectexporter")
                                   .withExecutableSuffix();
    return candidate.isExecutableFile() ? candidate : FilePath{};
}

class WestExportStep final : public AbstractProcessStep
{
public:
    WestExportStep(BuildStepList *bsl, Id id)
        : AbstractProcessStep(bsl, id)
    {
        m_qmlProjectFile.setSettingsKey("Zephyr.WestExportStep.QmlProjectFile");
        m_qmlProjectFile.setLabelText(Tr::tr("QML project file:"));
        m_qmlProjectFile.setExpectedKind(PathChooserKind::File);
        m_qmlProjectFile.setPromptDialogFilter("*.qmlproject");

        // Auto-detect a single .qmlproject at project root on first creation.
        if (m_qmlProjectFile().isEmpty()) {
            const FilePaths found = project()->projectDirectory()
                                        .dirEntries({{"*.qmlproject"}, DirFilterFlag::Files});
            if (found.size() == 1)
                m_qmlProjectFile.setValue(found.first());
        }

        setCommandLineProvider([this] { return exporterCommand(); });
        setWorkingDirectoryProvider([this]() -> FilePath {
            return project()->projectDirectory();
        });
        setUseEnglishOutput();
    }

    bool init() override
    {
        setEnabled(!exporterFilePath().isEmpty() && !m_qmlProjectFile().isEmpty());
        return AbstractProcessStep::init();
    }

    QWidget *createConfigWidget() final
    {
        auto updateDetails = [this] {
            ProcessParameters param;
            setupProcessParameters(&param);
            setSummaryText(param.summary(displayName()));
        };

        setDisplayName(Tr::tr("QML Project Export"));
        updateDetails();
        m_qmlProjectFile.addOnChanged(this, updateDetails);

        using namespace Layouting;
        return Form { noMargin, m_qmlProjectFile }.emerge();
    }

private:
    FilePath exporterFilePath() const
    {
        const FilePath fromSettings = settings().qmlProjectExporterFilePath();
        return fromSettings.isEmpty()
                   ? qmlProjectExporterFromKit(buildConfiguration()->kit())
                   : fromSettings;
    }

    CommandLine exporterCommand() const
    {
        CommandLine cmd{exporterFilePath()};
        cmd.addArg("--project-type=zephyr");
        cmd.addArg("--outdir");
        cmd.addArg(project()->projectDirectory().nativePath());
        cmd.addArg(m_qmlProjectFile().nativePath());
        return cmd;
    }

    FilePathAspect m_qmlProjectFile{this};
};

class WestExportStepFactory final : public BuildStepFactory
{
public:
    WestExportStepFactory()
    {
        registerStep<WestExportStep>(Constants::WEST_EXPORT_STEP_ID);
        setDisplayName(Tr::tr("QML Project Export"));
        setSupportedStepLists({ProjectExplorer::Constants::BUILDSTEPS_BUILD});
    }
};

class WestCleanStep final : public AbstractProcessStep
{
public:
    WestCleanStep(BuildStepList *bsl, Id id)
        : AbstractProcessStep(bsl, id)
    {
        m_pristine.setSettingsKey("Zephyr.WestCleanStep.Pristine");
        m_pristine.setLabelText(Tr::tr("Pristine:"));
        m_pristine.setLabelPlacement(BoolAspect::LabelPlacement::InExtraLabel);
        m_pristine.setToolTip(Tr::tr("Remove the whole build directory contents, including "
                                     "the CMake cache, instead of only the build products."));

        setCommandLineProvider([this] {
            CommandLine cmd{settings().westFilePath()};
            cmd.addArg("build");
            cmd.addArg("-d");
            cmd.addArg(buildConfiguration()->buildDirectory().nativePath());
            cmd.addArg("-t");
            cmd.addArg(m_pristine() ? QString("pristine") : QString("clean"));
            return cmd;
        });
        setWorkingDirectoryProvider([this]() -> FilePath {
            const FilePath ws = settings().workspaceDir();
            return ws.isEmpty() ? project()->projectDirectory() : ws;
        });
        setUseEnglishOutput();
    }

    QWidget *createConfigWidget() final
    {
        auto updateDetails = [this] {
            ProcessParameters param;
            setupProcessParameters(&param);
            setSummaryText(param.summary(displayName()));
        };

        setDisplayName(Tr::tr("West Clean"));
        updateDetails();
        m_pristine.addOnChanged(this, updateDetails);

        using namespace Layouting;
        return Form { noMargin, m_pristine }.emerge();
    }

private:
    BoolAspect m_pristine{this};
};

class WestCleanStepFactory final : public BuildStepFactory
{
public:
    WestCleanStepFactory()
    {
        registerStep<WestCleanStep>(Constants::WEST_CLEAN_STEP_ID);
        setDisplayName(Tr::tr("West Clean"));
        setSupportedStepLists({ProjectExplorer::Constants::BUILDSTEPS_CLEAN});
    }
};

static bool isZephyrProject(Project *project)
{
    return project->projectDirectory().pathAppended("west.yml").isFile();
}

static void connectTarget(Target *target)
{
    auto addStepsIfMissing = [](BuildConfiguration *bc) {
        BuildStepList *bsl = bc->buildSteps();

        if (!bsl->contains(Constants::WEST_BUILD_STEP_ID))
            bsl->appendStep(Constants::WEST_BUILD_STEP_ID);

        BuildStepList *cleanBsl = bc->cleanSteps();
        if (!cleanBsl->contains(Constants::WEST_CLEAN_STEP_ID))
            cleanBsl->appendStep(Constants::WEST_CLEAN_STEP_ID);
    };

    for (BuildConfiguration *bc : target->buildConfigurations())
        addStepsIfMissing(bc);

    QObject::connect(target, &Target::addedBuildConfiguration,
                     target, addStepsIfMissing);
}

static void connectProject(Project *project)
{
    if (!isZephyrProject(project))
        return;

    for (Target *target : project->targets())
        connectTarget(target);

    QObject::connect(project, &Project::addedTarget, project, connectTarget);
}

class WestFlashStep final : public AbstractProcessStep
{
public:
    WestFlashStep(BuildStepList *bsl, Id id)
        : AbstractProcessStep(bsl, id)
    {
        m_extraArgs.setSettingsKey("Zephyr.WestFlashStep.ExtraArgs");
        m_extraArgs.setLabelText(Tr::tr("Extra arguments:"));
        m_extraArgs.setDisplayStyle(StringAspect::LineEditDisplay);

        setCommandLineProvider([this] {
            CommandLine cmd{settings().westFilePath()};
            cmd.addArg("flash");
            cmd.addArg("-d");
            cmd.addArg(buildConfiguration()->buildDirectory().nativePath());
            cmd.addArgs(m_extraArgs(), CommandLine::Raw);
            return cmd;
        });
        setWorkingDirectoryProvider([this]() -> FilePath {
            const FilePath ws = settings().workspaceDir();
            return ws.isEmpty() ? project()->projectDirectory() : ws;
        });
        setUseEnglishOutput();
    }

    QWidget *createConfigWidget() final
    {
        auto updateDetails = [this] {
            ProcessParameters param;
            setupProcessParameters(&param);
            setSummaryText(param.summary(displayName()));
        };

        setDisplayName(Tr::tr("West Flash"));
        updateDetails();
        m_extraArgs.addOnChanged(this, updateDetails);

        using namespace Layouting;
        return Form { noMargin, m_extraArgs }.emerge();
    }

    StringAspect m_extraArgs{this};
};

class WestFlashStepFactory final : public BuildStepFactory
{
public:
    WestFlashStepFactory()
    {
        registerStep<WestFlashStep>(Constants::WEST_FLASH_STEP_ID);
        setDisplayName(Tr::tr("West Flash"));
        setSupportedStepLists({ProjectExplorer::Constants::BUILDSTEPS_DEPLOY});
    }
};

class ZephyrDeployConfigurationFactory final : public DeployConfigurationFactory
{
public:
    ZephyrDeployConfigurationFactory()
    {
        setConfigBaseId(Constants::ZEPHYR_DEPLOY_CONFIG_ID);
        setSupportedProjectType(Constants::ZEPHYR_PROJECT_ID);
        setDefaultDisplayName(Tr::tr("Flash with West"));
        addInitialStep(Constants::WEST_FLASH_STEP_ID);
    }
};

void setupWestBuildSteps()
{
    static WestExportStepFactory theExportStepFactory;
    static WestBuildStepFactory theBuildStepFactory;
    static WestCleanStepFactory theCleanStepFactory;
    static WestFlashStepFactory theFlashStepFactory;
    static ZephyrDeployConfigurationFactory theDeployConfigFactory;
    static WestSdkInstaller theSdkInstaller;

    for (Project *project : ProjectManager::projects())
        connectProject(project);

    QObject::connect(ProjectManager::instance(), &ProjectManager::projectAdded,
                     ProjectManager::instance(), connectProject);
}

#ifdef WITH_TESTS

class WestBuildStepTest final : public QObject
{
    Q_OBJECT

private slots:
    void testReconfigureArgs_data();
    void testReconfigureArgs();
    void testSysbuildSwitched();
};

void WestBuildStepTest::testReconfigureArgs_data()
{
    QTest::addColumn<QStringList>("configureArgs");
    QTest::addColumn<QStringList>("lastConfigureArgs");
    QTest::addColumn<QStringList>("expected");

    const QString force = "--cmake";
    const QString clearOptimization = "--cmake-opt=-UCLI_CONFIG_*_OPTIMIZATIONS";
    const QString clearConf = "--cmake-opt=-DEXTRA_CONF_FILE=";
    const QString clearOverlay = "--cmake-opt=-DEXTRA_DTC_OVERLAY_FILE=";
    const QString debug = "--cmake-opt=-DCONFIG_DEBUG_OPTIMIZATIONS=y";
    const QString size = "--cmake-opt=-DCONFIG_SIZE_OPTIMIZATIONS=y";
    const QString conf = "--extra-conf=/a.conf";
    const QString overlay = "--extra-dtc-overlay=/a.overlay";

    QTest::newRow("unchanged default") << QStringList() << QStringList() << QStringList();
    QTest::newRow("unchanged") << QStringList{conf, debug} << QStringList{conf, debug}
                               << QStringList();
    QTest::newRow("back to default") << QStringList() << QStringList{debug}
                                     << QStringList{force, clearOptimization, clearConf,
                                                    clearOverlay};
    QTest::newRow("other optimization") << QStringList{size} << QStringList{debug}
                                        << QStringList{force, clearOptimization, clearConf,
                                                       clearOverlay};
    QTest::newRow("last fragment removed") << QStringList{overlay} << QStringList{conf, overlay}
                                           << QStringList{force, clearOptimization, clearConf};
    QTest::newRow("last overlay removed") << QStringList{conf} << QStringList{conf, overlay}
                                          << QStringList{force, clearOptimization,
                                                         clearOverlay};
}

void WestBuildStepTest::testReconfigureArgs()
{
    QFETCH(QStringList, configureArgs);
    QFETCH(QStringList, lastConfigureArgs);
    QFETCH(QStringList, expected);

    QCOMPARE(reconfigureArgs(configureArgs, lastConfigureArgs), expected);
}

void WestBuildStepTest::testSysbuildSwitched()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const FilePath buildDir = FilePath::fromString(tmp.path());
    const QString workspaceDefault;
    const QString on = "--sysbuild";
    const QString off = "--no-sysbuild";
    QVERIFY(!sysbuildSwitched(buildDir, off));
    QVERIFY(!sysbuildSwitched(buildDir, on));

    QVERIFY_RESULT((buildDir / "CMakeCache.txt").writeFileContents({}));
    QVERIFY(!sysbuildSwitched(buildDir, off));
    QVERIFY(sysbuildSwitched(buildDir, on));
    QVERIFY(!sysbuildSwitched(buildDir, workspaceDefault));

    QVERIFY_RESULT((buildDir / "domains.yaml").writeFileContents({}));
    QVERIFY(sysbuildSwitched(buildDir, off));
    QVERIFY(!sysbuildSwitched(buildDir, on));
    QVERIFY(!sysbuildSwitched(buildDir, workspaceDefault));
}

QObject *createWestBuildStepTest()
{
    return new WestBuildStepTest;
}

#endif // WITH_TESTS

} // namespace Zephyr::Internal

#include "westbuildstep.moc"
