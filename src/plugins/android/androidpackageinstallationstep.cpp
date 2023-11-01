// Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "androidconstants.h"
#include "androidmanager.h"
#include "androidpackageinstallationstep.h"
#include "androidtr.h"

#include <projectexplorer/abstractprocessstep.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/buildsystem.h>
#include <projectexplorer/gnumakeparser.h>
#include <projectexplorer/kitaspects.h>
#include <projectexplorer/processparameters.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <projectexplorer/taskhub.h>
#include <projectexplorer/toolchain.h>

#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitaspect.h>

#include <utils/hostosinfo.h>
#include <utils/process.h>

#include <QDir>
#include <QLoggingCategory>

using namespace ProjectExplorer;
using namespace Utils;

namespace {
static Q_LOGGING_CATEGORY(packageInstallationStepLog, "qtc.android.packageinstallationstep", QtWarningMsg)
}

namespace Android {
namespace Internal {

class AndroidPackageInstallationStep final : public AbstractProcessStep
{
public:
    AndroidPackageInstallationStep(BuildStepList *bsl, Id id);

    QString nativeAndroidBuildPath() const;
private:
    bool init() final;
    void setupOutputFormatter(OutputFormatter *formatter) final;
    Tasking::GroupItem runRecipe() final;

    void reportWarningOrError(const QString &message, ProjectExplorer::Task::TaskType type);

    QStringList m_androidDirsToClean;
};

AndroidPackageInstallationStep::AndroidPackageInstallationStep(BuildStepList *bsl, Id id)
    : AbstractProcessStep(bsl, id)
{
    setDisplayName(Tr::tr("Copy application data"));
    setWidgetExpandedByDefault(false);
    setImmutable(true);
    setSummaryUpdater([this] {
        return Tr::tr("<b>Make install:</b> Copy App Files to \"%1\"")
            .arg(QDir::toNativeSeparators(nativeAndroidBuildPath()));
    });
    setUseEnglishOutput();
}

bool AndroidPackageInstallationStep::init()
{
    if (!AbstractProcessStep::init()) {
        reportWarningOrError(Tr::tr("\"%1\" step failed initialization.").arg(displayName()),
                             Task::TaskType::Error);
        return false;
    }

    ToolChain *tc = ToolChainKitAspect::cxxToolChain(kit());
    QTC_ASSERT(tc, reportWarningOrError(Tr::tr("\"%1\" step has an invalid C++ toolchain.")
                                        .arg(displayName()), Task::TaskType::Error);
            return false);

    QString dirPath = nativeAndroidBuildPath();
    const QString innerQuoted = ProcessArgs::quoteArg(dirPath);
    const QString outerQuoted = ProcessArgs::quoteArg("INSTALL_ROOT=" + innerQuoted);

    const FilePath makeCommand = tc->makeCommand(buildEnvironment());
    CommandLine cmd{makeCommand};
    // Run install on both the target and the whole project as a workaround for QTCREATORBUG-26550.
    cmd.addArgs(QString("%1 install && cd %2 && %3 %1 install")
                .arg(outerQuoted).arg(ProcessArgs::quoteArg(buildDirectory().toUserOutput()))
                .arg(ProcessArgs::quoteArg(makeCommand.toUserOutput())), CommandLine::Raw);

    processParameters()->setCommandLine(cmd);
    // This is useful when running an example target from a Qt module project.
    processParameters()->setWorkingDirectory(AndroidManager::buildDirectory(target()));

    m_androidDirsToClean.clear();
    // don't remove gradle's cache, it takes ages to rebuild it.
    m_androidDirsToClean << dirPath + "/assets";
    m_androidDirsToClean << dirPath + "/libs";

    return true;
}

QString AndroidPackageInstallationStep::nativeAndroidBuildPath() const
{
    QString buildPath = AndroidManager::androidBuildDirectory(target()).toString();
    if (HostOsInfo::isWindowsHost())
        if (buildEnvironment().searchInPath("sh.exe").isEmpty())
            buildPath = QDir::toNativeSeparators(buildPath);

    return buildPath;
}

void AndroidPackageInstallationStep::setupOutputFormatter(OutputFormatter *formatter)
{
    formatter->addLineParser(new GnuMakeParser);
    formatter->addLineParsers(kit()->createOutputParsers());
    formatter->addSearchDir(processParameters()->effectiveWorkingDirectory());
    AbstractProcessStep::setupOutputFormatter(formatter);
}

Tasking::GroupItem AndroidPackageInstallationStep::runRecipe()
{
    using namespace Tasking;

    const auto onSetup = [this] {
        if (AndroidManager::skipInstallationAndPackageSteps(target())) {
            reportWarningOrError(Tr::tr("Product type is not an application, not running the "
                                        "Make install step."), Task::Warning);
            return SetupResult::StopWithDone;
        }

        for (const QString &dir : std::as_const(m_androidDirsToClean)) {
            const FilePath androidDir = FilePath::fromString(dir);
            if (!dir.isEmpty() && androidDir.exists()) {
                emit addOutput(Tr::tr("Removing directory %1").arg(dir), OutputFormat::NormalMessage);
                QString error;
                if (!androidDir.removeRecursively(&error)) {
                    reportWarningOrError(Tr::tr("Failed to clean \"%1\" from the previous build, "
                                         "with error:\n%2").arg(androidDir.toUserOutput()).arg(error),
                                         Task::TaskType::Error);
                    return SetupResult::StopWithError;
                }
            }
        }

        // NOTE: This is a workaround for QTCREATORBUG-24155
        // Needed for Qt 5.15.0 and Qt 5.14.x versions
        if (buildType() == BuildConfiguration::BuildType::Debug) {
            QtSupport::QtVersion *version = QtSupport::QtKitAspect::qtVersion(kit());
            if (version && version->qtVersion() >= QVersionNumber(5, 14)
                && version->qtVersion() <= QVersionNumber(5, 15, 0)) {
                const QString assetsDebugDir = nativeAndroidBuildPath().append(
                    "/assets/--Added-by-androiddeployqt--/");
                QDir dir;
                if (!dir.exists(assetsDebugDir))
                    dir.mkpath(assetsDebugDir);

                QFile file(assetsDebugDir + "debugger.command");
                if (file.open(QIODevice::WriteOnly)) {
                    qCDebug(packageInstallationStepLog, "Successful added %s to the package.",
                            qPrintable(file.fileName()));
                } else {
                    qCDebug(packageInstallationStepLog,
                            "Cannot add %s to the package. The QML debugger might not work properly.",
                            qPrintable(file.fileName()));
                }
            }
        }
        return SetupResult::Continue;
    };

    return Group { onGroupSetup(onSetup), defaultProcessTask() };
}

void AndroidPackageInstallationStep::reportWarningOrError(const QString &message,
                                                          Task::TaskType type)
{
    qCDebug(packageInstallationStepLog) << message;
    emit addOutput(message, OutputFormat::ErrorMessage);
    TaskHub::addTask(BuildSystemTask(type, message));
}

//
// AndroidPackageInstallationStepFactory
//

AndroidPackageInstallationFactory::AndroidPackageInstallationFactory()
{
    registerStep<AndroidPackageInstallationStep>(Constants::ANDROID_PACKAGE_INSTALL_STEP_ID);
    setSupportedStepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
    setSupportedDeviceType(Android::Constants::ANDROID_DEVICE_TYPE);
    setRepeatable(false);
    setDisplayName(Tr::tr("Deploy to device"));
}

} // namespace Internal
} // namespace Android
