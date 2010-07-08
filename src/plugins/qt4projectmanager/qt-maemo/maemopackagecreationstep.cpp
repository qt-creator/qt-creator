/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of Qt Creator.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "maemopackagecreationstep.h"

#include "maemoconstants.h"
#include "maemopackagecreationwidget.h"
#include "maemodeployables.h"
#include "maemotoolchain.h"
#include "profilewrapper.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <qt4buildconfiguration.h>
#include <qt4project.h>
#include <qt4target.h>

#include <QtCore/QProcess>
#include <QtCore/QProcessEnvironment>
#include <QtCore/QStringBuilder>
#include <QtGui/QWidget>

namespace {
    const QLatin1String PackagingEnabledKey("Packaging Enabled");
    const QLatin1String DefaultVersionNumber("0.0.1");
    const QLatin1String VersionNumberKey("Version Number");
}

using namespace ProjectExplorer::Constants;
using ProjectExplorer::BuildConfiguration;
using ProjectExplorer::BuildStepConfigWidget;
using ProjectExplorer::Task;

namespace Qt4ProjectManager {
namespace Internal {

MaemoPackageCreationStep::MaemoPackageCreationStep(BuildConfiguration *buildConfig)
    : ProjectExplorer::BuildStep(buildConfig, CreatePackageId),
      m_deployables(new MaemoDeployables(this)),
      m_packagingEnabled(true),
      m_versionString(DefaultVersionNumber)
{
}

MaemoPackageCreationStep::MaemoPackageCreationStep(BuildConfiguration *buildConfig,
    MaemoPackageCreationStep *other)
    : BuildStep(buildConfig, other),
      m_deployables(new MaemoDeployables(this)),
      m_packagingEnabled(other->m_packagingEnabled),
      m_versionString(other->m_versionString)
{
}

MaemoPackageCreationStep::~MaemoPackageCreationStep()
{
    delete m_deployables;
}

bool MaemoPackageCreationStep::init()
{
    return true;
}

QVariantMap MaemoPackageCreationStep::toMap() const
{
    QVariantMap map(ProjectExplorer::BuildStep::toMap());
    map.insert(PackagingEnabledKey, m_packagingEnabled);
    map.insert(VersionNumberKey, m_versionString);
    return map;
}

bool MaemoPackageCreationStep::fromMap(const QVariantMap &map)
{
    m_packagingEnabled = map.value(PackagingEnabledKey, true).toBool();
    m_versionString = map.value(VersionNumberKey, DefaultVersionNumber).toString();
    return ProjectExplorer::BuildStep::fromMap(map);
}

void MaemoPackageCreationStep::run(QFutureInterface<bool> &fi)
{
    fi.reportResult(m_packagingEnabled ? createPackage() : true);
}

BuildStepConfigWidget *MaemoPackageCreationStep::createConfigWidget()
{
    return new MaemoPackageCreationWidget(this);
}

bool MaemoPackageCreationStep::createPackage()
{
    if (!packagingNeeded())
        return true;

    QTextCharFormat textCharFormat;
    emit addOutput(tr("Creating package file ..."), textCharFormat);
    QFile configFile(targetRoot() % QLatin1String("/config.sh"));
    if (!configFile.open(QIODevice::ReadOnly)) {
        raiseError(tr("Cannot open MADDE config file '%1'.")
                   .arg(nativePath(configFile)));
        return false;
    }

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    const QString &path = QDir::toNativeSeparators(maddeRoot() + QLatin1Char('/'));

    const QLatin1String key("PATH");
    QString colon = QLatin1String(":");
#ifdef Q_OS_WIN
    colon = QLatin1String(";");
    env.insert(key, path % QLatin1String("bin") % colon % env.value(key));
#endif
    env.insert(key, targetRoot() % "/bin" % colon % env.value(key));
    env.insert(key, path % QLatin1String("madbin") % colon % env.value(key));
    env.insert(QLatin1String("PERL5LIB"), path % QLatin1String("madlib/perl5"));

    const QString buildDir = buildDirectory();
    env.insert(QLatin1String("PWD"), buildDir);

    const QRegExp envPattern(QLatin1String("([^=]+)=[\"']?([^;\"']+)[\"']? ;.*"));
    QByteArray line;
    do {
        line = configFile.readLine(200);
        if (envPattern.exactMatch(line))
            env.insert(envPattern.cap(1), envPattern.cap(2));
    } while (!line.isEmpty());
    

    m_buildProc.reset(new QProcess);
    connect(m_buildProc.data(), SIGNAL(readyReadStandardOutput()), this,
        SLOT(handleBuildOutput()));
    connect(m_buildProc.data(), SIGNAL(readyReadStandardError()), this,
        SLOT(handleBuildOutput()));
    m_buildProc->setProcessEnvironment(env);
    m_buildProc->setWorkingDirectory(buildDir);
    m_buildProc->start("cd " + buildDir);
    m_buildProc->waitForFinished();

    // cache those two since we can change the version number during packaging
    // and might fail later to modify, copy, remove etc. the generated package
    const QString version = versionString();
    const QString pkgFilePath = packageFilePath();

    if (!QFileInfo(buildDir + QLatin1String("/debian")).exists()) {
        const QString command = QLatin1String("dh_make -s -n -p ")
            % projectName() % QLatin1Char('_') % versionString();
        if (!runCommand(command))
            return false;

        QFile rulesFile(buildDir + QLatin1String("/debian/rules"));
        if (!rulesFile.open(QIODevice::ReadWrite)) {
            raiseError(tr("Packaging Error: Cannot open file '%1'.")
                       .arg(nativePath(rulesFile)));
            return false;
        }

        QByteArray rulesContents = rulesFile.readAll();
        rulesContents.replace("DESTDIR", "INSTALL_ROOT");

        // Would be the right solution, but does not work (on Windows),
        // because dpkg-genchanges doesn't know about it (and can't be told).
        // rulesContents.replace("dh_builddeb", "dh_builddeb --destdir=.");

        rulesFile.resize(0);
        rulesFile.write(rulesContents);
        if (rulesFile.error() != QFile::NoError) {
            raiseError(tr("Packaging Error: Cannot write file '%1'.")
                       .arg(nativePath(rulesFile)));
            return false;
        }
    }

    {
        QFile::remove(buildDir + QLatin1String("/debian/files"));
        QFile changeLog(buildDir + QLatin1String("/debian/changelog"));
        if (changeLog.open(QIODevice::ReadWrite)) {
            QString content = QString::fromUtf8(changeLog.readAll());
            content.replace(QRegExp("\\([a-zA-Z0-9_\\.]+\\)"),
                QLatin1Char('(') % version % QLatin1Char(')'));
            changeLog.resize(0);
            changeLog.write(content.toUtf8());
        }
    }

    if (!runCommand(QLatin1String("dpkg-buildpackage -nc -uc -us")))
        return false;

    // Workaround for non-working dh_builddeb --destdir=.
    if (!QDir(buildDir).isRoot()) {
        const QString packageFileName = QFileInfo(pkgFilePath).fileName();
        const QString changesFileName = QFileInfo(packageFileName)
            .completeBaseName() + QLatin1String(".changes");
        const QString packageSourceDir = buildDir + QLatin1String("/../");
        const QString packageSourceFilePath
            = packageSourceDir + packageFileName;
        const QString changesSourceFilePath
            = packageSourceDir + changesFileName;
        const QString changesTargetFilePath
            = buildDir + QLatin1Char('/') + changesFileName;
        QFile::remove(pkgFilePath);
        QFile::remove(changesTargetFilePath);
        if (!QFile::rename(packageSourceFilePath, pkgFilePath)
            || !QFile::rename(changesSourceFilePath, changesTargetFilePath)) {
            raiseError(tr("Packaging failed."),
                tr("Could not move package files from %1 to %2.")
                .arg(packageSourceDir, buildDir));
            return false;
        }
    }

    emit addOutput(tr("Package created."), textCharFormat);
    m_deployables->setUnmodified();
    return true;
}

bool MaemoPackageCreationStep::runCommand(const QString &command)
{
    QTextCharFormat textCharFormat;
    emit addOutput(tr("Package Creation: Running command '%1'.").arg(command), textCharFormat);
    QString perl;
#ifdef Q_OS_WIN
    perl = maddeRoot() + QLatin1String("/bin/perl.exe ");
#endif
    m_buildProc->start(perl + maddeRoot() % QLatin1String("/madbin/") % command);
    if (!m_buildProc->waitForStarted()) {
        raiseError(tr("Packaging failed."),
            tr("Packaging error: Could not start command '%1'. Reason: %2")
            .arg(command).arg(m_buildProc->errorString()));
        return false;
    }
    m_buildProc->write("\n"); // For dh_make
    m_buildProc->waitForFinished(-1);
    if (m_buildProc->error() != QProcess::UnknownError || m_buildProc->exitCode() != 0) {
        QString mainMessage = tr("Packaging Error: Command '%1' failed.")
            .arg(command);
        if (m_buildProc->error() != QProcess::UnknownError)
            mainMessage += tr(" Reason: %1").arg(m_buildProc->errorString());
        else
            mainMessage += tr("Exit code: %1").arg(m_buildProc->exitCode());
        raiseError(mainMessage);
        return false;
    }
    return true;
}

void MaemoPackageCreationStep::handleBuildOutput()
{
    const QByteArray &stdOut = m_buildProc->readAllStandardOutput();
    QTextCharFormat textCharFormat;
    if (!stdOut.isEmpty())
        emit addOutput(QString::fromLocal8Bit(stdOut), textCharFormat);
    const QByteArray &errorOut = m_buildProc->readAllStandardError();
    if (!errorOut.isEmpty()) {
        textCharFormat.setForeground(QBrush(QColor("red")));
        emit addOutput(QString::fromLocal8Bit(errorOut), textCharFormat);
    }
}

const Qt4BuildConfiguration *MaemoPackageCreationStep::qt4BuildConfiguration() const
{
    return static_cast<Qt4BuildConfiguration *>(buildConfiguration());
}

QString MaemoPackageCreationStep::buildDirectory() const
{
    return qt4BuildConfiguration()->buildDirectory();
}

QString MaemoPackageCreationStep::projectName() const
{
    return qt4BuildConfiguration()->qt4Target()->qt4Project()
        ->rootProjectNode()->displayName().toLower();
}

const MaemoToolChain *MaemoPackageCreationStep::maemoToolChain() const
{
    return static_cast<MaemoToolChain *>(qt4BuildConfiguration()->toolChain());
}

QString MaemoPackageCreationStep::maddeRoot() const
{
    return maemoToolChain()->maddeRoot();
}

QString MaemoPackageCreationStep::targetRoot() const
{
    return maemoToolChain()->targetRoot();
}

bool MaemoPackageCreationStep::packagingNeeded() const
{
    QFileInfo packageInfo(packageFilePath());
    if (!packageInfo.exists() || m_deployables->isModified())
        return true;

    const int deployableCount = m_deployables->deployableCount();
    for (int i = 0; i < deployableCount; ++i) {
        if (packageInfo.lastModified()
            <= QFileInfo(m_deployables->deployableAt(i).localFilePath)
               .lastModified())
            return true;
    }

    return false;
}

QString MaemoPackageCreationStep::packageFilePath() const
{
    return buildDirectory() % '/' % projectName()
        % QLatin1Char('_') % versionString() % QLatin1String("_armel.deb");
}

QString MaemoPackageCreationStep::versionString() const
{
    return m_versionString;
}

void MaemoPackageCreationStep::setVersionString(const QString &version)
{
    m_versionString = version;
}

QString MaemoPackageCreationStep::nativePath(const QFile &file) const
{
    return QDir::toNativeSeparators(QFileInfo(file).filePath());
}

void MaemoPackageCreationStep::raiseError(const QString &shortMsg,
                                          const QString &detailedMsg)
{
    QTextCharFormat textCharFormat;
    emit addOutput(detailedMsg.isNull() ? shortMsg : detailedMsg, textCharFormat);
    emit addTask(Task(Task::Error, shortMsg, QString(), -1,
                      TASK_CATEGORY_BUILDSYSTEM));
}

const QLatin1String MaemoPackageCreationStep::CreatePackageId("Qt4ProjectManager.MaemoPackageCreationStep");

} // namespace Internal
} // namespace Qt4ProjectManager
