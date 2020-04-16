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
#include <projectexplorer/toolchain.h>

#include <utils/hostosinfo.h>
#include <utils/qtcprocess.h>

#include <QDir>

using namespace ProjectExplorer;
using namespace Utils;

namespace Android {
namespace Internal {

class AndroidPackageInstallationStep final : public AbstractProcessStep
{
    Q_DECLARE_TR_FUNCTIONS(Android::AndroidPackageInstallationStep)

public:
    AndroidPackageInstallationStep(BuildStepList *bsl, Core::Id id);

    BuildStepConfigWidget *createConfigWidget() final;

private:
    bool init() final;
    void setupOutputFormatter(Utils::OutputFormatter *formatter) override;
    void doRun() final;

    QStringList m_androidDirsToClean;
};

class AndroidPackageInstallationStepWidget final : public BuildStepConfigWidget
{
    Q_DECLARE_TR_FUNCTIONS(Android::AndroidPackageInstallationStepWidget)

public:
    AndroidPackageInstallationStepWidget(BuildStep *step)
        : BuildStepConfigWidget(step)
    {
        setDisplayName(tr("Make install"));
        setSummaryText("<b>" + tr("Make install") + "</b>");
    }
};

AndroidPackageInstallationStep::AndroidPackageInstallationStep(BuildStepList *bsl, Core::Id id)
    : AbstractProcessStep(bsl, id)
{
    const QString name = tr("Copy application data");
    setDefaultDisplayName(name);
    setDisplayName(name);
    setWidgetExpandedByDefault(false);
    setImmutable(true);
}

bool AndroidPackageInstallationStep::init()
{
    QString dirPath = buildDirectory().pathAppended(Constants::ANDROID_BUILDDIRECTORY).toString();
    if (HostOsInfo::isWindowsHost())
        if (buildEnvironment().searchInPath("sh.exe").isEmpty())
            dirPath = QDir::toNativeSeparators(dirPath);

    ToolChain *tc = ToolChainKitAspect::cxxToolChain(target()->kit());
    QTC_ASSERT(tc, return false);

    CommandLine cmd{tc->makeCommand(buildEnvironment())};
    const QString innerQuoted = QtcProcess::quoteArg(dirPath);
    const QString outerQuoted = QtcProcess::quoteArg("INSTALL_ROOT=" + innerQuoted);
    cmd.addArgs(outerQuoted + " install", CommandLine::Raw);

    ProcessParameters *pp = processParameters();
    pp->setMacroExpander(macroExpander());
    pp->setWorkingDirectory(buildDirectory());
    Environment env = buildEnvironment();
    Environment::setupEnglishOutput(&env);
    pp->setEnvironment(env);
    pp->setCommandLine(cmd);

    m_androidDirsToClean.clear();
    // don't remove gradle's cache, it takes ages to rebuild it.
    m_androidDirsToClean << dirPath + "/assets";
    m_androidDirsToClean << dirPath + "/libs";

    return AbstractProcessStep::init();
}

void AndroidPackageInstallationStep::setupOutputFormatter(OutputFormatter *formatter)
{
    formatter->addLineParser(new GnuMakeParser);
    formatter->addLineParsers(target()->kit()->createOutputParsers());
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
            if (!FileUtils::removeRecursively(androidDir, &error)) {
                emit addOutput(error, OutputFormat::Stderr);
                emit finished(false);
                return;
            }
        }
    }
    AbstractProcessStep::doRun();
}

BuildStepConfigWidget *AndroidPackageInstallationStep::createConfigWidget()
{
    return new AndroidPackageInstallationStepWidget(this);
}

//
// AndroidPackageInstallationStepFactory
//

AndroidPackageInstallationFactory::AndroidPackageInstallationFactory()
{
    registerStep<AndroidPackageInstallationStep>(Constants::ANDROID_PACKAGE_INSTALLATION_STEP_ID);
    setSupportedStepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
    setSupportedDeviceType(Android::Constants::ANDROID_DEVICE_TYPE);
    setRepeatable(false);
    setDisplayName(AndroidPackageInstallationStep::tr("Deploy to device"));
}

} // namespace Internal
} // namespace Android
