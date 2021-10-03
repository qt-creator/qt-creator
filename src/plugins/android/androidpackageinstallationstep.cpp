/****************************************************************************
**
** Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
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

#include "androidpackageinstallationstep.h"

#include "androidconstants.h"
#include "androidmanager.h"

#include <projectexplorer/abstractprocessstep.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/gnumakeparser.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/processparameters.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <projectexplorer/taskhub.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/buildsystem.h>

#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitinformation.h>

#include <utils/hostosinfo.h>
#include <utils/qtcprocess.h>

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
    Q_DECLARE_TR_FUNCTIONS(Android::AndroidPackageInstallationStep)

public:
    AndroidPackageInstallationStep(BuildStepList *bsl, Id id);

    QString nativeAndroidBuildPath() const;
private:
    bool init() final;
    void setupOutputFormatter(OutputFormatter *formatter) final;
    void doRun() final;

    void reportWarningOrError(const QString &message, ProjectExplorer::Task::TaskType type);

    QStringList m_androidDirsToClean;
};

AndroidPackageInstallationStep::AndroidPackageInstallationStep(BuildStepList *bsl, Id id)
    : AbstractProcessStep(bsl, id)
{
    setDisplayName(tr("Copy application data"));
    setWidgetExpandedByDefault(false);
    setImmutable(true);
    setSummaryUpdater([this] {
        return tr("<b>Make install:</b> Copy App Files to %1").arg(nativeAndroidBuildPath());
    });
    setUseEnglishOutput();
}

bool AndroidPackageInstallationStep::init()
{
    if (!AbstractProcessStep::init()) {
        reportWarningOrError(tr("\"%1\" step failed initialization.").arg(displayName()),
                             Task::TaskType::Error);
        return false;
    }

    ToolChain *tc = ToolChainKitAspect::cxxToolChain(kit());
    QTC_ASSERT(tc, reportWarningOrError(tr("\"%1\" step has an invalid C++ toolchain.")
                                        .arg(displayName()), Task::TaskType::Error);
            return false);

    QString dirPath = nativeAndroidBuildPath();
    const QString innerQuoted = ProcessArgs::quoteArg(dirPath);
    const QString outerQuoted = ProcessArgs::quoteArg("INSTALL_ROOT=" + innerQuoted);

    CommandLine cmd{tc->makeCommand(buildEnvironment())};
    cmd.addArgs(outerQuoted + " install", CommandLine::Raw);

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

void AndroidPackageInstallationStep::doRun()
{
    QString error;
    foreach (const QString &dir, m_androidDirsToClean) {
        FilePath androidDir = FilePath::fromString(dir);
        if (!dir.isEmpty() && androidDir.exists()) {
            emit addOutput(tr("Removing directory %1").arg(dir), OutputFormat::NormalMessage);
            if (!androidDir.removeRecursively(&error)) {
                reportWarningOrError(tr("Failed to clean \"%1\" from the previous build, with "
                                        "error:\n%2").arg(androidDir.toUserOutput()).arg(error),
                                     Task::TaskType::Error);
                emit finished(false);
                return;
            }
        }
    }
    AbstractProcessStep::doRun();

    // NOTE: This is a workaround for QTCREATORBUG-24155
    // Needed for Qt 5.15.0 and Qt 5.14.x versions
    if (buildType() == BuildConfiguration::BuildType::Debug) {
        QtSupport::BaseQtVersion *version = QtSupport::QtKitAspect::qtVersion(kit());
        if (version && version->qtVersion() >= QtSupport::QtVersionNumber{5, 14}
            && version->qtVersion() <= QtSupport::QtVersionNumber{5, 15, 0}) {
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
    setDisplayName(AndroidPackageInstallationStep::tr("Deploy to device"));
}

} // namespace Internal
} // namespace Android
