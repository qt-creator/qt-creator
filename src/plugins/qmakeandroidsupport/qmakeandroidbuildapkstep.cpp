/**************************************************************************
**
** Copyright (c) 2014 BogDan Vatra <bog_dan_ro@yahoo.com>
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qmakeandroidbuildapkstep.h"
#include "qmakeandroidbuildapkwidget.h"

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
const QLatin1String ProFilePathForInputFile("ProFilePathForInputFile");


//////////////////
// QmakeAndroidBuildApkStepFactory
/////////////////

QmakeAndroidBuildApkStepFactory::QmakeAndroidBuildApkStepFactory(QObject *parent)
    : IBuildStepFactory(parent)
{
}

QList<Core::Id> QmakeAndroidBuildApkStepFactory::availableCreationIds(ProjectExplorer::BuildStepList *parent) const
{
    if (parent->id() != ProjectExplorer::Constants::BUILDSTEPS_BUILD
            || !canHandle(parent->target())
            || parent->contains(ANDROID_BUILD_APK_ID)) {
        return QList<Core::Id>();
    }

    return QList<Core::Id>() << ANDROID_BUILD_APK_ID;
}

QString QmakeAndroidBuildApkStepFactory::displayNameForId(const Core::Id id) const
{
    if (id == ANDROID_BUILD_APK_ID)
        return tr("Build Android APK");
    return QString();
}

bool QmakeAndroidBuildApkStepFactory::canCreate(ProjectExplorer::BuildStepList *parent, const Core::Id id) const
{
    return availableCreationIds(parent).contains(id);
}

ProjectExplorer::BuildStep *QmakeAndroidBuildApkStepFactory::create(ProjectExplorer::BuildStepList *parent, const Core::Id id)
{
    Q_ASSERT(canCreate(parent, id));
    Q_UNUSED(id);
    return new QmakeAndroidBuildApkStep(parent);
}

bool QmakeAndroidBuildApkStepFactory::canRestore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map) const
{
    return canCreate(parent, ProjectExplorer::idFromMap(map));
}

ProjectExplorer::BuildStep *QmakeAndroidBuildApkStepFactory::restore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map)
{
    Q_ASSERT(canRestore(parent, map));
    QmakeAndroidBuildApkStep * const step = new QmakeAndroidBuildApkStep(parent);
    if (!step->fromMap(map)) {
        delete step;
        return 0;
    }
    return step;
}

bool QmakeAndroidBuildApkStepFactory::canClone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *product) const
{
    return canCreate(parent, product->id());
}

ProjectExplorer::BuildStep *QmakeAndroidBuildApkStepFactory::clone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *product)
{
    Q_ASSERT(canClone(parent, product));
    return new QmakeAndroidBuildApkStep(parent, static_cast<QmakeAndroidBuildApkStep *>(product));
}

bool QmakeAndroidBuildApkStepFactory::canHandle(ProjectExplorer::Target *t) const
{
    return t->project()->supportsKit(t->kit())
            && AndroidManager::supportsAndroid(t)
            && qobject_cast<QmakeProject *>(t->project());
}


QmakeAndroidBuildApkStep::QmakeAndroidBuildApkStep(ProjectExplorer::BuildStepList *bc)
    :Android::AndroidBuildApkStep(bc, ANDROID_BUILD_APK_ID)
{
    ctor();
}

QString QmakeAndroidBuildApkStep::proFilePathForInputFile() const
{
    return m_proFilePathForInputFile;
}

void QmakeAndroidBuildApkStep::setProFilePathForInputFile(const QString &path)
{
    m_proFilePathForInputFile = path;
}

QmakeAndroidBuildApkStep::QmakeAndroidBuildApkStep(ProjectExplorer::BuildStepList *bc, QmakeAndroidBuildApkStep *other)
    : Android::AndroidBuildApkStep(bc, other),
      m_proFilePathForInputFile(other->m_proFilePathForInputFile)
{
    ctor();
}

Utils::FileName QmakeAndroidBuildApkStep::androidPackageSourceDir() const
{
    QmakeProjectManager::QmakeProject *pro = static_cast<QmakeProjectManager::QmakeProject *>(project());
    const QmakeProjectManager::QmakeProFileNode *node
            = pro->rootQmakeProjectNode()->findProFileFor(m_proFilePathForInputFile);
    if (!node)
        return Utils::FileName();
    return Utils::FileName::fromString(node->singleVariableValue(QmakeProjectManager::AndroidPackageSourceDir));
}

void QmakeAndroidBuildApkStep::ctor()
{
    connect(project(), SIGNAL(proFilesEvaluated()),
           this, SLOT(updateInputFile()));
}

bool QmakeAndroidBuildApkStep::init()
{
    if (AndroidManager::checkForQt51Files(project()->projectDirectory()))
        emit addOutput(tr("Found old folder \"android\" in source directory. Qt 5.2 does not use that folder by default."), ErrorOutput);

    if (!Android::AndroidBuildApkStep::init())
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
    const QmakeProjectManager::QmakeProFileNode *node = pro->rootQmakeProjectNode()->findProFileFor(m_proFilePathForInputFile);
    if (!node) { // should never happen
        emit addOutput(tr("Internal Error: Could not find .pro file."), BuildStep::ErrorMessageOutput);
        return false;
    }

    QString inputFile = node->singleVariableValue(QmakeProjectManager::AndroidDeploySettingsFile);
    if (inputFile.isEmpty()) { // should never happen
        emit addOutput(tr("Internal Error: Unknown Android deployment JSON file location."), BuildStep::ErrorMessageOutput);
        return false;
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

    if (buildConfiguration()->buildType() == ProjectExplorer::BuildConfiguration::Release)
        arguments << QLatin1String("--release");

    QStringList argumentsPasswordConcealed = arguments;

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
                   BuildStep::MessageOutput);
}

ProjectExplorer::BuildStepConfigWidget *QmakeAndroidBuildApkStep::createConfigWidget()
{
    return new QmakeAndroidBuildApkWidget(this);
}

bool QmakeAndroidBuildApkStep::fromMap(const QVariantMap &map)
{
    m_proFilePathForInputFile = map.value(ProFilePathForInputFile).toString();
    return Android::AndroidBuildApkStep::fromMap(map);
}

QVariantMap QmakeAndroidBuildApkStep::toMap() const
{
    QVariantMap map = Android::AndroidBuildApkStep::toMap();
    map.insert(ProFilePathForInputFile, m_proFilePathForInputFile);
    return map;
}

void QmakeAndroidBuildApkStep::updateInputFile()
{
    QmakeProject *pro = static_cast<QmakeProject *>(project());
    QList<QmakeProFileNode *> nodes = pro->applicationProFiles();

    const QmakeProFileNode *node = pro->rootQmakeProjectNode()->findProFileFor(m_proFilePathForInputFile);
    if (!nodes.contains(const_cast<QmakeProFileNode *>(node))) {
        if (!nodes.isEmpty())
            m_proFilePathForInputFile = nodes.first()->path();
        else
            m_proFilePathForInputFile.clear();
    }

    emit inputFileChanged();
}

} // namespace Internal
} // namespace QmakeProjectManager
