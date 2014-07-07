/****************************************************************************
**
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

#include "winrtpackagedeploymentstep.h"
#include "winrtpackagedeploymentstepwidget.h"
#include "winrtconstants.h"

#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildtargetinfo.h>
#include <projectexplorer/deployablefile.h>
#include <projectexplorer/deploymentdata.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <qtsupport/qtkitinformation.h>
#include <utils/qtcprocess.h>

#include <QRegularExpression>

using namespace ProjectExplorer;
using Utils::QtcProcess;

namespace WinRt {
namespace Internal {

WinRtPackageDeploymentStep::WinRtPackageDeploymentStep(BuildStepList *bsl)
    : AbstractProcessStep(bsl, Constants::WINRT_BUILD_STEP_DEPLOY)
    , m_createMappingFile(false)
{
    setDisplayName(tr("Run windeployqt"));
    m_args = defaultWinDeployQtArguments();
}

bool WinRtPackageDeploymentStep::init()
{
    Utils::FileName proFile = project()->projectFilePath();
    const QString targetPath
            = target()->applicationTargets().targetForProject(proFile).toString()
                + QLatin1String(".exe");
    QString targetDir = targetPath.left(targetPath.lastIndexOf(QLatin1Char('/')) + 1);
    // ### Actually, targetForProject is supposed to return the file path including the file
    // extension. Whenever this will eventually work, we have to remove the .exe suffix here.

    const QtSupport::BaseQtVersion *qt = QtSupport::QtKitInformation::qtVersion(target()->kit());
    if (!qt)
        return false;

    QString args = QtcProcess::quoteArg(QDir::toNativeSeparators(targetPath));
    args += QLatin1Char(' ') + m_args;

    m_manifestFileName = QStringLiteral("AppxManifest");

    if (qt->type() == QLatin1String(Constants::WINRT_WINPHONEQT)
            && qt->mkspec().toString().contains(QLatin1String("msvc2012"))) {
        m_createMappingFile = true;
        m_manifestFileName = QStringLiteral("WMAppManifest");
    }

    if (m_createMappingFile) {
        args += QLatin1String(" -list mapping");
        m_mappingFileContent = QLatin1String("[Files]\n\"") + QDir::toNativeSeparators(targetDir)
                + m_manifestFileName + QLatin1String(".xml\" \"") + m_manifestFileName + QLatin1String(".xml\"\n");

        QDir assetDirectory(targetDir + QLatin1String("assets"));
        if (assetDirectory.exists()) {
            QStringList iconsToDeploy;
            const QString fullManifestPath = targetDir + m_manifestFileName + QLatin1String(".xml");
            if (!parseIconsAndExecutableFromManifest(fullManifestPath, &iconsToDeploy,
                                                     &m_executablePathInManifest)) {
                raiseError(tr("Cannot parse manifest file %1.").arg(fullManifestPath));
                return false;
            }
            foreach (QString icon, iconsToDeploy) {
                m_mappingFileContent += QLatin1Char('"')
                        + QDir::toNativeSeparators(targetDir + icon) + QLatin1String("\" \"")
                        + QDir::toNativeSeparators(icon) + QLatin1String("\"\n");
            }
        }
    }

    ProcessParameters *params = processParameters();
    params->setCommand(QLatin1String("windeployqt.exe"));
    params->setArguments(args);
    params->setEnvironment(target()->activeBuildConfiguration()->environment());

    return AbstractProcessStep::init();
}

bool WinRtPackageDeploymentStep::processSucceeded(int exitCode, QProcess::ExitStatus status)
{
    if (m_createMappingFile) {
        Utils::FileName proFile = project()->projectFilePath();
        QString targetPath
                = target()->applicationTargets().targetForProject(proFile).toString();
        QString targetDir = targetPath.left(targetPath.lastIndexOf(QLatin1Char('/')) + 1);
        QString targetInstallationPath;
        // The list holds the local file paths and the "remote" file paths
        QList<QPair<QString, QString> > installableFilesList;
        foreach (DeployableFile file, target()->deploymentData().allFiles()) {
            QString remoteFilePath = file.remoteFilePath();
            QString localFilePath = file.localFilePath().toString();
            if (localFilePath == targetPath) {
                if (!targetPath.endsWith(QLatin1String(".exe"))) {
                    remoteFilePath += QLatin1String(".exe");
                    localFilePath += QLatin1String(".exe");
                }
                targetInstallationPath = remoteFilePath;
            }
            installableFilesList.append(QPair<QString, QString>(localFilePath, remoteFilePath));
        }

        // if there are no INSTALLS set we just deploy the files from windeployqt, the manifest
        // and the icons referenced in there and the actual build target
        QString baseDir;
        if (targetInstallationPath.isEmpty()) {
            targetPath += QLatin1String(".exe");
            m_mappingFileContent
                    += QLatin1Char('"') + QDir::toNativeSeparators(targetPath) + QLatin1String("\" \"")
                    + QDir::toNativeSeparators(m_executablePathInManifest) + QLatin1String("\"\n");
            baseDir = targetDir;
        } else {
            baseDir = targetInstallationPath.left(targetInstallationPath.lastIndexOf(QLatin1Char('/')) + 1);
        }

        typedef QPair<QString, QString> QStringPair;
        foreach (const QStringPair &pair, installableFilesList) {
            // For the mapping file we need the remote paths relative to the application's executable
            QString relativeRemotePath;
            if (QDir(pair.second).isRelative())
                relativeRemotePath = pair.second;
            else
                relativeRemotePath = QDir(baseDir).relativeFilePath(pair.second);
            m_mappingFileContent += QLatin1Char('"') + QDir::toNativeSeparators(pair.first)
                    + QLatin1String("\" \"") + QDir::toNativeSeparators(relativeRemotePath)
                    + QLatin1String("\"\n");
        }

        const QString mappingFilePath = targetDir + m_manifestFileName + QLatin1String(".map");
        QFile mappingFile(mappingFilePath);
        if (!mappingFile.open(QFile::WriteOnly | QFile::Text)) {
            raiseError(tr("Cannot open mapping file %1 for writing.").arg(mappingFilePath));
            return false;
        }
        mappingFile.write(m_mappingFileContent.toUtf8());
    }

    return AbstractProcessStep::processSucceeded(exitCode, status);
}

void WinRtPackageDeploymentStep::stdOutput(const QString &line)
{
    if (m_createMappingFile)
        m_mappingFileContent += line;
    AbstractProcessStep::stdOutput(line);
}

BuildStepConfigWidget *WinRtPackageDeploymentStep::createConfigWidget()
{
    return new WinRtPackageDeploymentStepWidget(this);
}

void WinRtPackageDeploymentStep::setWinDeployQtArguments(const QString &args)
{
    m_args = args;
}

QString WinRtPackageDeploymentStep::winDeployQtArguments() const
{
    return m_args;
}

QString WinRtPackageDeploymentStep::defaultWinDeployQtArguments() const
{
    QString args;
    QtcProcess::addArg(&args, QStringLiteral("--qmldir"));
    QtcProcess::addArg(&args, project()->projectDirectory().toUserOutput());
    return args;
}

void WinRtPackageDeploymentStep::raiseError(const QString &errorMessage)
{
    emit addOutput(errorMessage, BuildStep::ErrorMessageOutput);
    emit addTask(ProjectExplorer::Task(ProjectExplorer::Task::Error, errorMessage, Utils::FileName(), -1,
                                       ProjectExplorer::Constants::TASK_CATEGORY_DEPLOYMENT));
}

bool WinRtPackageDeploymentStep::fromMap(const QVariantMap &map)
{
    if (!AbstractProcessStep::fromMap(map))
        return false;
    QVariant v = map.value(QLatin1String(Constants::WINRT_BUILD_STEP_DEPLOY_ARGUMENTS));
    if (v.isValid())
        m_args = v.toString();
    return true;
}

QVariantMap WinRtPackageDeploymentStep::toMap() const
{
    QVariantMap map = AbstractProcessStep::toMap();
    map.insert(QLatin1String(Constants::WINRT_BUILD_STEP_DEPLOY_ARGUMENTS), m_args);
    return map;
}

bool WinRtPackageDeploymentStep::parseIconsAndExecutableFromManifest(QString manifestFileName, QStringList *icons, QString *executable)
{
    if (!icons->isEmpty())
        icons->clear();
    QFile manifestFile(manifestFileName);
    if (!manifestFile.open(QFile::ReadOnly))
        return false;
    const QString contents = QString::fromUtf8(manifestFile.readAll());

    QRegularExpression iconPattern(QStringLiteral("[\\\\/a-zA-Z0-9_\\-\\!]*\\.(png|jpg|jpeg)"));
    QRegularExpressionMatchIterator iterator = iconPattern.globalMatch(contents);
    while (iterator.hasNext()) {
        QRegularExpressionMatch match = iterator.next();
        const QString icon = match.captured(0);
        icons->append(icon);
    }

    QRegularExpression executablePattern(QStringLiteral("ImagePath=\"([a-zA-Z0-9_-]*\\.exe)\""));
    QRegularExpressionMatch match = executablePattern.match(contents);
    if (!match.hasMatch())
        return false;
    *executable = match.captured(1);

    return true;
}

} // namespace Internal
} // namespace WinRt
