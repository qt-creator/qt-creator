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

#include <android/androidconstants.h>
#include <android/androidmanager.h>

#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/target.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/gnumakeparser.h>
#include <utils/hostosinfo.h>
#include <utils/qtcprocess.h>

#include <QDir>

using namespace QmakeAndroidSupport::Internal;

const Core::Id AndroidPackageInstallationStep::Id = Core::Id("Qt4ProjectManager.AndroidPackageInstallationStep");

AndroidPackageInstallationStep::AndroidPackageInstallationStep(ProjectExplorer::BuildStepList *bsl)
    : AbstractProcessStep(bsl, Id)
{
    const QString name = tr("Copy application data");
    setDefaultDisplayName(name);
    setDisplayName(name);
}

AndroidPackageInstallationStep::AndroidPackageInstallationStep(ProjectExplorer::BuildStepList *bc, AndroidPackageInstallationStep *other)
    : AbstractProcessStep(bc, other)
{ }

bool AndroidPackageInstallationStep::init(QList<const BuildStep *> &earlierSteps)
{
    ProjectExplorer::BuildConfiguration *bc = buildConfiguration();
    QString dirPath = bc->buildDirectory().appendPath(QLatin1String(Android::Constants::ANDROID_BUILDDIRECTORY)).toString();
    if (Utils::HostOsInfo::isWindowsHost())
        if (bc->environment().searchInPath(QLatin1String("sh.exe")).isEmpty())
            dirPath = QDir::toNativeSeparators(dirPath);

    ProjectExplorer::ToolChain *tc
            = ProjectExplorer::ToolChainKitInformation::toolChain(target()->kit(),
                                                                  ProjectExplorer::Constants::CXX_LANGUAGE_ID);

    ProjectExplorer::ProcessParameters *pp = processParameters();
    pp->setMacroExpander(bc->macroExpander());
    pp->setWorkingDirectory(bc->buildDirectory().toString());
    pp->setCommand(tc->makeCommand(bc->environment()));
    Utils::Environment env = bc->environment();
    Utils::Environment::setupEnglishOutput(&env);
    pp->setEnvironment(env);
    const QString innerQuoted = Utils::QtcProcess::quoteArg(dirPath);
    const QString outerQuoted = Utils::QtcProcess::quoteArg(QString::fromLatin1("INSTALL_ROOT=") + innerQuoted);
    pp->setArguments(outerQuoted + QString::fromLatin1(" install"));

    pp->resolveAll();
    setOutputParser(new ProjectExplorer::GnuMakeParser());
    ProjectExplorer::IOutputParser *parser = target()->kit()->createOutputParser();
    if (parser)
        appendOutputParser(parser);
    outputParser()->setWorkingDirectory(pp->effectiveWorkingDirectory());

    m_androidDirsToClean.clear();
    // don't remove gradle's cache, it takes ages to rebuild it.
    if (!QFile::exists(dirPath + QLatin1String("/build.xml")) && Android::AndroidManager::useGradle(target())) {
        m_androidDirsToClean << dirPath + QLatin1String("/assets");
        m_androidDirsToClean << dirPath + QLatin1String("/libs");
    } else {
        m_androidDirsToClean << dirPath;
    }

    return AbstractProcessStep::init(earlierSteps);
}

void AndroidPackageInstallationStep::run(QFutureInterface<bool> &fi)
{
    QString error;
    foreach (const QString &dir, m_androidDirsToClean) {
        Utils::FileName androidDir = Utils::FileName::fromString(dir);
        if (!dir.isEmpty() && androidDir.exists()) {
            emit addOutput(tr("Removing directory %1").arg(dir), OutputFormat::NormalMessage);
            if (!Utils::FileUtils::removeRecursively(androidDir, &error)) {
                emit addOutput(error, OutputFormat::Stderr);
                reportRunResult(fi, false);
                return;
            }
        }
    }
    AbstractProcessStep::run(fi);
}

ProjectExplorer::BuildStepConfigWidget *AndroidPackageInstallationStep::createConfigWidget()
{
    return new AndroidPackageInstallationStepWidget(this);
}


bool AndroidPackageInstallationStep::immutable() const
{
    return true;
}

//
// AndroidPackageInstallationStepWidget
//

AndroidPackageInstallationStepWidget::AndroidPackageInstallationStepWidget(AndroidPackageInstallationStep *step)
    : m_step(step)
{

}

QString AndroidPackageInstallationStepWidget::summaryText() const
{
    return tr("<b>Make install</b>");
}

QString AndroidPackageInstallationStepWidget::displayName() const
{
    return tr("Make install");
}

bool AndroidPackageInstallationStepWidget::showWidget() const
{
    return false;
}
