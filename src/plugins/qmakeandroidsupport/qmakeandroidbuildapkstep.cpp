/****************************************************************************
**
** Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
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

#include "qmakeandroidbuildapkstep.h"
#include "qmakeandroidbuildapkwidget.h"
#include "qmakeandroidrunconfiguration.h"

#include <android/androidconfigurations.h>
#include <android/androidconstants.h>
#include <android/androidmanager.h>
#include <android/androidqtsupport.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/target.h>
#include <qtsupport/qtkitinformation.h>
#include <qmakeprojectmanager/qmakenodes.h>
#include <qmakeprojectmanager/qmakeproject.h>

#include <utils/qtcprocess.h>

#include <QHBoxLayout>

using namespace Android;
using QmakeProjectManager::QmakeProject;
using QmakeProjectManager::QmakeProFileNode;

namespace QmakeAndroidSupport {
namespace Internal {

const Core::Id ANDROID_BUILD_APK_ID("QmakeProjectManager.AndroidBuildApkStep");

//////////////////
// QmakeAndroidBuildApkStepFactory
/////////////////

QmakeAndroidBuildApkStepFactory::QmakeAndroidBuildApkStepFactory(QObject *parent)
    : IBuildStepFactory(parent)
{
}

QList<ProjectExplorer::BuildStepInfo> QmakeAndroidBuildApkStepFactory::availableSteps(ProjectExplorer::BuildStepList *parent) const
{
    ProjectExplorer::Target *target = parent->target();
    if (parent->id() != ProjectExplorer::Constants::BUILDSTEPS_BUILD
            || !target->project()->supportsKit(target->kit())
            || !AndroidManager::supportsAndroid(target)
            || !qobject_cast<QmakeProject *>(target->project())
            || parent->contains(ANDROID_BUILD_APK_ID))
        return {};

    return {{ ANDROID_BUILD_APK_ID, tr("Build Android APK") }};
}

ProjectExplorer::BuildStep *QmakeAndroidBuildApkStepFactory::create(ProjectExplorer::BuildStepList *parent, const Core::Id id)
{
    Q_UNUSED(id);
    return new QmakeAndroidBuildApkStep(parent);
}

ProjectExplorer::BuildStep *QmakeAndroidBuildApkStepFactory::clone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *product)
{
    return new QmakeAndroidBuildApkStep(parent, static_cast<QmakeAndroidBuildApkStep *>(product));
}

QmakeAndroidBuildApkStep::QmakeAndroidBuildApkStep(ProjectExplorer::BuildStepList *bc)
    :AndroidBuildApkStep(bc, ANDROID_BUILD_APK_ID)
{ }

Utils::FileName QmakeAndroidBuildApkStep::proFilePathForInputFile() const
{
    ProjectExplorer::RunConfiguration *rc = target()->activeRunConfiguration();
    if (auto *arc = qobject_cast<QmakeAndroidRunConfiguration *>(rc))
        return arc->proFilePath();
    return Utils::FileName();
}

QmakeAndroidBuildApkStep::QmakeAndroidBuildApkStep(ProjectExplorer::BuildStepList *bc, QmakeAndroidBuildApkStep *other)
    : AndroidBuildApkStep(bc, other)
{ }

Utils::FileName QmakeAndroidBuildApkStep::androidPackageSourceDir() const
{
    QmakeProjectManager::QmakeProject *pro = static_cast<QmakeProjectManager::QmakeProject *>(project());
    const QmakeProjectManager::QmakeProFileNode *node
            = pro->rootProjectNode()->findProFileFor(proFilePathForInputFile());
    if (!node)
        return Utils::FileName();

    QFileInfo sourceDirInfo(node->singleVariableValue(QmakeProjectManager::Variable::AndroidPackageSourceDir));
    return Utils::FileName::fromString(sourceDirInfo.canonicalFilePath());
}

bool QmakeAndroidBuildApkStep::init(QList<const BuildStep *> &earlierSteps)
{
    if (AndroidManager::checkForQt51Files(project()->projectDirectory()))
        emit addOutput(tr("Found old folder \"android\" in source directory. Qt 5.2 does not use that folder by default."), OutputFormat::Stderr);

    if (!AndroidBuildApkStep::init(earlierSteps))
        return false;

    QtSupport::BaseQtVersion *version = QtSupport::QtKitInformation::qtVersion(target()->kit());
    if (!version)
        return false;

    QString command = version->qmakeProperty("QT_HOST_BINS");
    if (!command.endsWith(QLatin1Char('/')))
        command += QLatin1Char('/');
    command += QLatin1String("androiddeployqt");
    if (Utils::HostOsInfo::isWindowsHost())
        command += QLatin1String(".exe");

    QString deploymentMethod;
    if (m_deployAction == MinistroDeployment)
        deploymentMethod = QLatin1String("ministro");
    else if (m_deployAction == DebugDeployment)
        deploymentMethod = QLatin1String("debug");
    else if (m_deployAction == BundleLibrariesDeployment)
        deploymentMethod = QLatin1String("bundled");

    ProjectExplorer::BuildConfiguration *bc = buildConfiguration();
    QString outputDir = bc->buildDirectory().appendPath(QLatin1String(Constants::ANDROID_BUILDDIRECTORY)).toString();

    const auto *pro = static_cast<QmakeProjectManager::QmakeProject *>(project());
    const QmakeProjectManager::QmakeProFileNode *node = pro->rootProjectNode()->findProFileFor(proFilePathForInputFile());
    m_skipBuilding = !node;
    if (m_skipBuilding)
        return true;

    QString inputFile = node->singleVariableValue(QmakeProjectManager::Variable::AndroidDeploySettingsFile);
    if (inputFile.isEmpty()) {
        m_skipBuilding = true;
        return true;
    }

    QStringList arguments;
    arguments << QLatin1String("--input")
              << inputFile
              << QLatin1String("--output")
              << outputDir
              << QLatin1String("--deployment")
              << deploymentMethod
              << QLatin1String("--android-platform")
              << AndroidManager::buildTargetSDK(target())
              << QLatin1String("--jdk")
              << AndroidConfigurations::currentConfig().openJDKLocation().toString();

    if (m_verbose)
        arguments << QLatin1String("--verbose");

    if (m_useGradle)
        arguments << QLatin1String("--gradle");
    else
        arguments << QLatin1String("--ant")
                  << AndroidConfigurations::currentConfig().antToolPath().toString();


    QStringList argumentsPasswordConcealed = arguments;

    if (version->qtVersion() >= QtSupport::QtVersionNumber(5, 6, 0)) {
        if (bc->buildType() == ProjectExplorer::BuildConfiguration::Debug)
            arguments << QLatin1String("--gdbserver");
        else
            arguments << QLatin1String("--no-gdbserver");
    }

    if (m_signPackage) {
        arguments << QLatin1String("--sign")
                  << m_keystorePath.toString()
                  << m_certificateAlias
                  << QLatin1String("--storepass")
                  << m_keystorePasswd;
        argumentsPasswordConcealed << QLatin1String("--sign") << QLatin1String("******")
                                   << QLatin1String("--storepass") << QLatin1String("******");
        if (!m_certificatePasswd.isEmpty()) {
            arguments << QLatin1String("--keypass")
                      << m_certificatePasswd;
            argumentsPasswordConcealed << QLatin1String("--keypass")
                      << QLatin1String("******");
        }

    }

    ProjectExplorer::ProcessParameters *pp = processParameters();
    setupProcessParameters(pp, bc, arguments, command);

    // Generate arguments with keystore password concealed
    ProjectExplorer::ProcessParameters pp2;
    setupProcessParameters(&pp2, bc, argumentsPasswordConcealed, command);
    m_command = pp2.effectiveCommand();
    m_argumentsPasswordConcealed = pp2.prettyArguments();

    return true;
}

void QmakeAndroidBuildApkStep::run(QFutureInterface<bool> &fi)
{
    if (m_skipBuilding) {
        emit addOutput(tr("No application .pro file found, not building an APK."), BuildStep::OutputFormat::ErrorMessage);
        reportRunResult(fi, true);
        return;
    }
    AndroidBuildApkStep::run(fi);
}

void QmakeAndroidBuildApkStep::setupProcessParameters(ProjectExplorer::ProcessParameters *pp,
                                                      ProjectExplorer::BuildConfiguration *bc,
                                                      const QStringList &arguments,
                                                      const QString &command)
{
    pp->setMacroExpander(bc->macroExpander());
    pp->setWorkingDirectory(bc->buildDirectory().toString());
    Utils::Environment env = bc->environment();
    pp->setEnvironment(env);
    pp->setCommand(command);
    pp->setArguments(Utils::QtcProcess::joinArgs(arguments));
    pp->resolveAll();
}

void QmakeAndroidBuildApkStep::processStarted()
{
    emit addOutput(tr("Starting: \"%1\" %2")
                   .arg(QDir::toNativeSeparators(m_command),
                        m_argumentsPasswordConcealed),
                   BuildStep::OutputFormat::NormalMessage);
}

ProjectExplorer::BuildStepConfigWidget *QmakeAndroidBuildApkStep::createConfigWidget()
{
    return new QmakeAndroidBuildApkWidget(this);
}

bool QmakeAndroidBuildApkStep::fromMap(const QVariantMap &map)
{
    return AndroidBuildApkStep::fromMap(map);
}

QVariantMap QmakeAndroidBuildApkStep::toMap() const
{
    QVariantMap map = AndroidBuildApkStep::toMap();
    return map;
}

} // namespace Internal
} // namespace QmakeAndroidSupport
