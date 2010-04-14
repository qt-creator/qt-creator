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
#include "maemotoolchain.h"

#include <qt4buildconfiguration.h>
#include <qt4project.h>
#include <qt4target.h>

#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QProcess>
#include <QtCore/QProcessEnvironment>
#include <QtCore/QStringBuilder>
#include <QtGui/QWidget>

using ProjectExplorer::BuildConfiguration;
using ProjectExplorer::BuildStepConfigWidget;

namespace Qt4ProjectManager {
namespace Internal {

MaemoPackageCreationStep::MaemoPackageCreationStep(BuildConfiguration *buildConfig)
    : ProjectExplorer::BuildStep(buildConfig, CreatePackageId)
{
}

MaemoPackageCreationStep::MaemoPackageCreationStep(BuildConfiguration *buildConfig,
    MaemoPackageCreationStep *other)
    : BuildStep(buildConfig, other)
{

}

bool MaemoPackageCreationStep::init()
{
    return true;
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
    qDebug("%s", Q_FUNC_INFO);
    if (!packagingNeeded())
        return true;

    QFile configFile(targetRoot() % QLatin1String("/config.sh"));
    if (!configFile.open(QIODevice::ReadOnly)) {
        qDebug("Cannot open config file '%s'", qPrintable(configFile.fileName()));
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

    const QString projectDir = QFileInfo(executable()).absolutePath();
    env.insert(QLatin1String("PWD"), projectDir);

    const QRegExp envPattern(QLatin1String("([^=]+)=[\"']?([^;\"']+)[\"']? ;.*"));
    QByteArray line;
    do {
        line = configFile.readLine(200);
        if (envPattern.exactMatch(line))
            env.insert(envPattern.cap(1), envPattern.cap(2));
    } while (!line.isEmpty());
    
    qDebug("Process environment: %s", qPrintable(env.toStringList().join("\n")));
    qDebug("sysroot: '%s'", qPrintable(env.value(QLatin1String("SYSROOT_DIR"))));
    
    QProcess buildProc;
    buildProc.setProcessEnvironment(env);
    buildProc.setWorkingDirectory(projectDir);

    if (!QFileInfo(projectDir + QLatin1String("/debian")).exists()) {
        const QString command = QLatin1String("dh_make -s -n -p ")
            % executableFileName() % QLatin1String("_0.1");
        if (!runCommand(buildProc, command))
            return false;
        QFile rulesFile(projectDir + QLatin1String("/debian/rules"));
        if (!rulesFile.open(QIODevice::ReadWrite)) {
            qDebug("Error: Could not open debian/rules.");
            return false;
        }

        QByteArray rulesContents = rulesFile.readAll();
        rulesContents.replace("DESTDIR", "INSTALL_ROOT");
        rulesFile.resize(0);
        rulesFile.write(rulesContents);
        if (rulesFile.error() != QFile::NoError) {
            qDebug("Error: could not access debian/rules");
            return false;
        }
    }

    if (!runCommand(buildProc, QLatin1String("dh_installdirs")))
        return false;
    
    const QString targetFile(projectDir % QLatin1String("/debian/")
        % executableFileName().toLower() % QLatin1String("/usr/bin/")
        % executableFileName());
    if (QFile::exists(targetFile)) {
        if (!QFile::remove(targetFile)) {
            qDebug("Error: Could not remove '%s'", qPrintable(targetFile));
            return false;
        }
    }
    if (!QFile::copy(executable(), targetFile)) {
        qDebug("Error: Could not copy '%s' to '%s'",
               qPrintable(executable()), qPrintable(targetFile));
        return false;
    }

    const QStringList commands = QStringList() << QLatin1String("dh_link")
        << QLatin1String("dh_fixperms") << QLatin1String("dh_installdeb")
        << QLatin1String("dh_shlibdeps") << QLatin1String("dh_gencontrol")
        << QLatin1String("dh_md5sums") << QLatin1String("dh_builddeb --destdir=.");
    foreach (const QString &command, commands) {
        if (!runCommand(buildProc, command))
            return false;
    }

    return true;
}

bool MaemoPackageCreationStep::runCommand(QProcess &proc, const QString &command)
{
    qDebug("Running command '%s'", qPrintable(command));
    QString perl;
#ifdef Q_OS_WIN
    perl = maddeRoot() + QLatin1String("/bin/perl.exe ");
#endif
    proc.start(perl + maddeRoot() % QLatin1String("/madbin/") % command);
    proc.write("\n"); // For dh_make
    if (!proc.waitForFinished(100000) && proc.error() == QProcess::Timedout) {
        qDebug("command '%s' hangs", qPrintable(command));
        return false;
    }
    if (proc.exitCode() != 0) {
        qDebug("command '%s' failed with return value %d and output '%s'",
            qPrintable(command),  proc.exitCode(), (proc.readAllStandardOutput()
            + "\n" + proc.readAllStandardError()).data());
        return false;
    }
    qDebug("Command finished, output was '%s'", (proc.readAllStandardOutput()
        + "\n" + proc.readAllStandardError()).data());
    return true;
}

const Qt4BuildConfiguration *MaemoPackageCreationStep::qt4BuildConfiguration() const
{
    return static_cast<Qt4BuildConfiguration *>(buildConfiguration());
}

QString MaemoPackageCreationStep::executable() const
{
    const TargetInformation &ti = qt4BuildConfiguration()->qt4Target()
        ->qt4Project()->rootProjectNode()->targetInformation();
    if (!ti.valid)
        return QString();

    return QDir::toNativeSeparators(QDir::cleanPath(ti.workingDir
        + QLatin1Char('/') + ti.target));
}

QString MaemoPackageCreationStep::executableFileName() const
{
    return QFileInfo(executable()).fileName();
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
#if 0
    QFileInfo execInfo(executable());
    const QString packageFile = execInfo.absolutePath() % QLatin1Char('/')
        % executableFileName() % QLatin1String("_0.1_armel.deb");
    QFileInfo packageInfo(packageFile);
    return !packageInfo.exists()
        || packageInfo.lastModified() <= execInfo.lastModified();
#else
    return false;
#endif
}

const QLatin1String MaemoPackageCreationStep::CreatePackageId("Qt4ProjectManager.MaemoPackageCreationStep");

} // namespace Internal
} // namespace Qt4ProjectManager
