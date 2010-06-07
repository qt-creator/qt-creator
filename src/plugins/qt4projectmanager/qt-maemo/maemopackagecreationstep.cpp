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
#include "maemopackagecontents.h"
#include "maemotoolchain.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <qt4buildconfiguration.h>
#include <qt4project.h>
#include <qt4target.h>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QProcess>
#include <QtCore/QProcessEnvironment>
#include <QtCore/QStringBuilder>
#include <QtGui/QWidget>

using namespace ProjectExplorer::Constants;
using ProjectExplorer::BuildConfiguration;
using ProjectExplorer::BuildStepConfigWidget;
using ProjectExplorer::Task;

namespace Qt4ProjectManager {
namespace Internal {

MaemoPackageCreationStep::MaemoPackageCreationStep(BuildConfiguration *buildConfig)
    : ProjectExplorer::BuildStep(buildConfig, CreatePackageId),
      m_packageContents(new MaemoPackageContents(this))
{
}

MaemoPackageCreationStep::MaemoPackageCreationStep(BuildConfiguration *buildConfig,
    MaemoPackageCreationStep *other)
    : BuildStep(buildConfig, other),
      m_packageContents(new MaemoPackageContents(this))

{

}

bool MaemoPackageCreationStep::init()
{
    return true;
}

QVariantMap MaemoPackageCreationStep::toMap() const
{
    QVariantMap map(ProjectExplorer::BuildStep::toMap());
    return map.unite(m_packageContents->toMap());
}

bool MaemoPackageCreationStep::fromMap(const QVariantMap &map)
{
    m_packageContents->fromMap(map);
    return ProjectExplorer::BuildStep::fromMap(map);
}

void MaemoPackageCreationStep::run(QFutureInterface<bool> &fi)
{
    fi.reportResult(createPackage());
}

BuildStepConfigWidget *MaemoPackageCreationStep::createConfigWidget()
{
    return new MaemoPackageCreationWidget(this);
}

bool MaemoPackageCreationStep::createPackage()
{
    if (!packagingNeeded())
        return true;

    emit addOutput(tr("Creating package file ..."));
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
    env.insert(key, targetRoot() % "/bin" % colon % env.value(key));
    env.insert(key, path % QLatin1String("bin") % colon % env.value(key));
#endif
    env.insert(key, path % QLatin1String("madbin") % colon % env.value(key));
    env.insert(QLatin1String("PERL5LIB"), path % QLatin1String("madlib/perl5"));

    const QString buildDir = QFileInfo(localExecutableFilePath()).absolutePath();
    env.insert(QLatin1String("PWD"), buildDir);

    const QRegExp envPattern(QLatin1String("([^=]+)=[\"']?([^;\"']+)[\"']? ;.*"));
    QByteArray line;
    do {
        line = configFile.readLine(200);
        if (envPattern.exactMatch(line))
            env.insert(envPattern.cap(1), envPattern.cap(2));
    } while (!line.isEmpty());
    
    QProcess buildProc;
    buildProc.setProcessEnvironment(env);
    buildProc.setWorkingDirectory(buildDir);

    if (!QFileInfo(buildDir + QLatin1String("/debian")).exists()) {
        const QString command = QLatin1String("dh_make -s -n -p ")
            % executableFileName().toLower() % versionString();
        if (!runCommand(buildProc, command))
            return false;
        QFile rulesFile(buildDir + QLatin1String("/debian/rules"));
        if (!rulesFile.open(QIODevice::ReadWrite)) {
            raiseError(tr("Packaging Error: Cannot open file '%1'.")
                       .arg(nativePath(rulesFile)));
            return false;
        }

        QByteArray rulesContents = rulesFile.readAll();
        rulesContents.replace("DESTDIR", "INSTALL_ROOT");
        rulesFile.resize(0);
        rulesFile.write(rulesContents);
        if (rulesFile.error() != QFile::NoError) {
            raiseError(tr("Packaging Error: Cannot write file '%1'.")
                       .arg(nativePath(rulesFile)));
            return false;
        }
    }

    if (!runCommand(buildProc, QLatin1String("dh_installdirs")))
        return false;
    
    const QDir debianRoot = QDir(buildDir % QLatin1String("/debian/")
                                 % executableFileName().toLower());
    for (int i = 0; i < m_packageContents->rowCount(); ++i) {
        const MaemoPackageContents::Deployable &d
            = m_packageContents->deployableAt(i);
        const QString absTargetDir = debianRoot.path() + '/' + d.remoteDir;
        const QString targetFile
            = absTargetDir + '/' + QFileInfo(d.localFilePath).fileName();
        const QString relTargetDir = debianRoot.relativeFilePath(absTargetDir);
        if (!debianRoot.exists(relTargetDir)
            && !debianRoot.mkpath(relTargetDir)) {
            raiseError(tr("Packaging Error: Could not create directory '%1'.")
                       .arg(QDir::toNativeSeparators(absTargetDir)));
            return false;
        }
        if (QFile::exists(targetFile) && !QFile::remove(targetFile)) {
            raiseError(tr("Packaging Error: Could not replace file '%1'.")
                       .arg(QDir::toNativeSeparators(targetFile)));
            return false;
        }

        if (!QFile::copy(d.localFilePath, targetFile)) {
            raiseError(tr("Packaging Error: Could not copy '%1' to '%2'.")
                       .arg(QDir::toNativeSeparators(d.localFilePath))
                       .arg(QDir::toNativeSeparators(targetFile)));
            return false;
        }
    }

    const QStringList commands = QStringList() << QLatin1String("dh_link")
        << QLatin1String("dh_fixperms") << QLatin1String("dh_installdeb")
        << QLatin1String("dh_shlibdeps") << QLatin1String("dh_gencontrol")
        << QLatin1String("dh_md5sums") << QLatin1String("dh_builddeb --destdir=.");
    foreach (const QString &command, commands) {
        if (!runCommand(buildProc, command))
            return false;
    }

    emit addOutput(tr("Package created."));
    m_packageContents->setUnModified();
    return true;
}

bool MaemoPackageCreationStep::runCommand(QProcess &proc, const QString &command)
{
    emit addOutput(tr("Package Creation: Running command '%1'.").arg(command));
    QString perl;
#ifdef Q_OS_WIN
    perl = maddeRoot() + QLatin1String("/bin/perl.exe ");
#endif
    proc.start(perl + maddeRoot() % QLatin1String("/madbin/") % command);
    if (!proc.waitForStarted()) {
        raiseError(tr("Packaging failed."),
            tr("Packaging error: Could not start command '%1'. Reason: %2")
            .arg(command).arg(proc.errorString()));
        return false;
    }
    proc.write("\n"); // For dh_make
    if (!proc.waitForFinished(60000) && proc.error() == QProcess::Timedout) {
        raiseError(tr("Packaging failed."),
                   tr("Packaging Error: Command '%1' timed out.").arg(command));
        return false;
    }
    if (proc.error() != QProcess::UnknownError || proc.exitCode() != 0) {
        QString mainMessage = tr("Packaging Error: Command '%1' failed.")
            .arg(command);
        if (proc.error() != QProcess::UnknownError)
            mainMessage += tr(" Reason: %1").arg(proc.errorString());
        raiseError(mainMessage, mainMessage + QLatin1Char('\n')
                   + tr("Output was: ") + proc.readAllStandardError()
                   + QLatin1Char('\n') + proc.readAllStandardOutput());
        return false;
    }
    return true;
}

const Qt4BuildConfiguration *MaemoPackageCreationStep::qt4BuildConfiguration() const
{
    return static_cast<Qt4BuildConfiguration *>(buildConfiguration());
}

QString MaemoPackageCreationStep::localExecutableFilePath() const
{
    const TargetInformation &ti = qt4BuildConfiguration()->qt4Target()
        ->qt4Project()->rootProjectNode()->targetInformation();
    if (!ti.valid)
        return QString();
    return QDir::toNativeSeparators(QDir::cleanPath(ti.workingDir
        + QLatin1Char('/') + executableFileName()));
}

QString MaemoPackageCreationStep::executableFileName() const
{
    const Qt4Project * const project
        = qt4BuildConfiguration()->qt4Target()->qt4Project();
    const TargetInformation &ti
        = project->rootProjectNode()->targetInformation();
    if (!ti.valid)
        return QString();

    return project->rootProjectNode()->projectType() == LibraryTemplate
        ? QLatin1String("lib") + ti.target + QLatin1String(".so")
        : ti.target;
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
    if (!packageInfo.exists() || m_packageContents->isModified())
        return true;

    for (int i = 0; i < m_packageContents->rowCount(); ++i) {
        if (packageInfo.lastModified()
            <= QFileInfo(m_packageContents->deployableAt(i).localFilePath)
               .lastModified())
            return true;
    }

    return false;
}

QString MaemoPackageCreationStep::packageFilePath() const
{
    QFileInfo execInfo(localExecutableFilePath());
    return execInfo.path() % QDir::separator() % execInfo.fileName().toLower()
        % versionString() % QLatin1String("_armel.deb");
}

QString MaemoPackageCreationStep::versionString() const
{
    return QLatin1String("_0.1");
}

QString MaemoPackageCreationStep::nativePath(const QFile &file) const
{
    return QDir::toNativeSeparators(QFileInfo(file).filePath());
}

void MaemoPackageCreationStep::raiseError(const QString &shortMsg,
                                          const QString &detailedMsg)
{
    emit addOutput(detailedMsg.isNull() ? shortMsg : detailedMsg);
    emit addTask(Task(Task::Error, shortMsg, QString(), -1,
                      TASK_CATEGORY_BUILDSYSTEM));
}

const QLatin1String MaemoPackageCreationStep::CreatePackageId("Qt4ProjectManager.MaemoPackageCreationStep");

} // namespace Internal
} // namespace Qt4ProjectManager
