/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
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
**************************************************************************/
#include "maemoinstalltosysrootstep.h"

#include "maemodeployables.h"
#include "maemoglobal.h"
#include "maemopackagecreationstep.h"
#include "maemotoolchain.h"
#include "qt4maemodeployconfiguration.h"

#include <qt4projectmanager/qt4buildconfiguration.h>
#include <qt4projectmanager/qt4target.h>

#include <QtCore/QLatin1Char>
#include <QtCore/QProcess>
#include <QtCore/QWeakPointer>

using namespace ProjectExplorer;

namespace Qt4ProjectManager {
namespace Internal {

class AbstractMaemoInstallPackageToSysrootWidget : public BuildStepConfigWidget
{
    Q_OBJECT
public:
    AbstractMaemoInstallPackageToSysrootWidget(AbstractMaemoInstallPackageToSysrootStep *step)
        : m_step(step) {}

    virtual void init()
    {
        BuildStepList * const list
             = qobject_cast<BuildStepList *>(m_step->parent());
        connect(list, SIGNAL(stepInserted(int)), SIGNAL(updateSummary()));
        connect(list, SIGNAL(stepMoved(int,int)), SIGNAL(updateSummary()));
        connect(list, SIGNAL(aboutToRemoveStep(int)), SLOT(handleStepToBeRemoved(int)));
        connect(list, SIGNAL(stepRemoved(int)), SIGNAL(updateSummary()));
    }

    virtual QString summaryText() const
    {
        if (!MaemoGlobal::earlierBuildStep<AbstractMaemoPackageCreationStep>(m_step->deployConfiguration(), m_step)) {
            return QLatin1String("<font color=\"red\">")
                + tr("Cannot deploy to sysroot: No packaging step found.")
                + QLatin1String("</font>");
        }
        return QLatin1String("<b>") + displayName() + QLatin1String("</b>");
    }

private:
    Q_SLOT void handleStepToBeRemoved(int step)
    {
        BuildStepList * const list
            = qobject_cast<BuildStepList *>(m_step->parent());
        if (list->steps().at(step) == m_step)
            disconnect(list, 0, this, 0);
    }

    const AbstractMaemoInstallPackageToSysrootStep * const m_step;
};


class MaemoInstallDebianPackageToSysrootWidget : public AbstractMaemoInstallPackageToSysrootWidget
{
    Q_OBJECT
public:
    MaemoInstallDebianPackageToSysrootWidget(AbstractMaemoInstallPackageToSysrootStep *step)
        : AbstractMaemoInstallPackageToSysrootWidget(step) {}

    virtual QString displayName() const { return MaemoInstallDebianPackageToSysrootStep::DisplayName; }
};

class MaemoInstallRpmPackageToSysrootWidget : public AbstractMaemoInstallPackageToSysrootWidget
{
    Q_OBJECT
public:
    MaemoInstallRpmPackageToSysrootWidget(AbstractMaemoInstallPackageToSysrootStep *step)
        : AbstractMaemoInstallPackageToSysrootWidget(step) {}

    virtual QString displayName() const { return MaemoInstallRpmPackageToSysrootStep::DisplayName; }
};

class MaemoCopyFilesToSysrootWidget : public BuildStepConfigWidget
{
    Q_OBJECT
public:
    MaemoCopyFilesToSysrootWidget(const BuildStep *buildStep)
        : m_buildStep(buildStep) {}

    virtual void init()
    {
        if (m_buildStep) {
            connect(m_buildStep.data(), SIGNAL(displayNameChanged()),
                SIGNAL(updateSummary()));
        }
    }
    virtual QString summaryText() const {
        return QLatin1String("<b>") + displayName() + QLatin1String("</b>"); }
    virtual QString displayName() const {
        return m_buildStep ? m_buildStep.data()->displayName() : QString();
    }
private:
    const QWeakPointer<const BuildStep> m_buildStep;
};


AbstractMaemoInstallPackageToSysrootStep::AbstractMaemoInstallPackageToSysrootStep(BuildStepList *bsl,
    const QString &id)
        : BuildStep(bsl, id)
{
}

AbstractMaemoInstallPackageToSysrootStep::AbstractMaemoInstallPackageToSysrootStep(BuildStepList *bsl,
    AbstractMaemoInstallPackageToSysrootStep *other)
        : BuildStep(bsl, other)
{
}

void AbstractMaemoInstallPackageToSysrootStep::run(QFutureInterface<bool> &fi)
{
    const Qt4BuildConfiguration * const bc
        = qobject_cast<Qt4BaseTarget *>(target())->activeBuildConfiguration();
    if (!bc) {
        addOutput(tr("Can't install to sysroot without build configuration."),
            ErrorMessageOutput);
        fi.reportResult(false);
        return;
    }

    const AbstractMaemoPackageCreationStep * const pStep
        = MaemoGlobal::earlierBuildStep<AbstractMaemoPackageCreationStep>(deployConfiguration(), this);
    if (!pStep) {
        addOutput(tr("Can't install package to sysroot without packaging step."),
            ErrorMessageOutput);
        fi.reportResult(false);
        return;
    }

    m_installerProcess = new QProcess;
    connect(m_installerProcess, SIGNAL(readyReadStandardOutput()),
        SLOT(handleInstallerStdout()));
    connect(m_installerProcess, SIGNAL(readyReadStandardError()),
        SLOT(handleInstallerStderr()));

    emit addOutput(tr("Installing package to sysroot ..."), MessageOutput);
    const QtVersion * const qtVersion = bc->qtVersion();
    const QString packageFilePath = pStep->packageFilePath();
    const int packageFileSize = QFileInfo(packageFilePath).size() / (1024*1024);
    const QStringList args = madArguments() << packageFilePath;
    MaemoGlobal::callMadAdmin(*m_installerProcess, args, qtVersion, true);
    if (!m_installerProcess->waitForFinished((2*packageFileSize + 10)*1000)
            || m_installerProcess->exitStatus() != QProcess::NormalExit
            || m_installerProcess->exitCode() != 0) {
        emit addOutput(tr("Installation to sysroot failed, continuing anyway."),
            ErrorMessageOutput);
        if (m_installerProcess->state() != QProcess::NotRunning) {
            m_installerProcess->terminate();
            m_installerProcess->waitForFinished();
            m_installerProcess->kill();
        }
        fi.reportResult(true);
        return;
    }

    fi.reportResult(true);
    m_installerProcess->deleteLater();
    m_installerProcess = 0;
}

void AbstractMaemoInstallPackageToSysrootStep::handleInstallerStdout()
{
    if (m_installerProcess)
        emit addOutput(QString::fromLocal8Bit(m_installerProcess->readAllStandardOutput()), NormalOutput);
}

void AbstractMaemoInstallPackageToSysrootStep::handleInstallerStderr()
{
    if (m_installerProcess)
        emit addOutput(QString::fromLocal8Bit(m_installerProcess->readAllStandardError()), ErrorOutput);
}


MaemoInstallDebianPackageToSysrootStep::MaemoInstallDebianPackageToSysrootStep(BuildStepList *bsl)
    : AbstractMaemoInstallPackageToSysrootStep(bsl, Id)
{
    setDisplayName(DisplayName);
}

MaemoInstallDebianPackageToSysrootStep::MaemoInstallDebianPackageToSysrootStep(BuildStepList *bsl,
    MaemoInstallDebianPackageToSysrootStep *other)
        : AbstractMaemoInstallPackageToSysrootStep(bsl, other)
{
    setDisplayName(DisplayName);
}

BuildStepConfigWidget *MaemoInstallDebianPackageToSysrootStep::createConfigWidget()
{
    return new MaemoInstallDebianPackageToSysrootWidget(this);
}


QStringList MaemoInstallDebianPackageToSysrootStep::madArguments() const
{
    return QStringList() << QLatin1String("xdpkg") << QLatin1String("-i")
        << QLatin1String("--no-force-downgrade");
}

const QString MaemoInstallDebianPackageToSysrootStep::Id
    = QLatin1String("MaemoInstallDebianPackageToSysrootStep");
const QString MaemoInstallDebianPackageToSysrootStep::DisplayName
    = tr("Install Debian package to sysroot");

MaemoInstallRpmPackageToSysrootStep::MaemoInstallRpmPackageToSysrootStep(BuildStepList *bsl)
    : AbstractMaemoInstallPackageToSysrootStep(bsl, Id)
{
    setDisplayName(DisplayName);
}

MaemoInstallRpmPackageToSysrootStep::MaemoInstallRpmPackageToSysrootStep(BuildStepList *bsl,
    MaemoInstallRpmPackageToSysrootStep *other)
        : AbstractMaemoInstallPackageToSysrootStep(bsl, other)
{
    setDisplayName(DisplayName);
}

BuildStepConfigWidget *MaemoInstallRpmPackageToSysrootStep::createConfigWidget()
{
    return new MaemoInstallRpmPackageToSysrootWidget(this);
}

QStringList MaemoInstallRpmPackageToSysrootStep::madArguments() const
{
    return QStringList() << QLatin1String("xrpm") << QLatin1String("-i");
}

const QString MaemoInstallRpmPackageToSysrootStep::Id
    = QLatin1String("MaemoInstallRpmPackageToSysrootStep");
const QString MaemoInstallRpmPackageToSysrootStep::DisplayName
    = tr("Install RPM package to sysroot");


MaemoCopyToSysrootStep::MaemoCopyToSysrootStep(BuildStepList *bsl)
    : BuildStep(bsl, Id)
{
    setDisplayName(DisplayName);
}

MaemoCopyToSysrootStep::MaemoCopyToSysrootStep(BuildStepList *bsl,
    MaemoCopyToSysrootStep *other)
        : BuildStep(bsl, other)
{
    setDisplayName(DisplayName);
}

void MaemoCopyToSysrootStep::run(QFutureInterface<bool> &fi)
{
    const Qt4BuildConfiguration * const bc
        = qobject_cast<Qt4BaseTarget *>(target())->activeBuildConfiguration();
    if (!bc) {
        addOutput(tr("Can't copy to sysroot without build configuration."),
            ErrorMessageOutput);
        fi.reportResult(false);
        return;
    }

    const MaemoToolChain * const tc
        = dynamic_cast<MaemoToolChain *>(bc->toolChain());
    if (!tc) {
        addOutput(tr("Can't copy to sysroot without toolchain."),
            ErrorMessageOutput);
        fi.reportResult(false);
        return;
    }

    emit addOutput(tr("Copying files to sysroot ..."), MessageOutput);
    QDir sysRootDir(tc->sysroot());
    const QSharedPointer<MaemoDeployables> deployables
        = qobject_cast<Qt4MaemoDeployConfiguration *>(deployConfiguration())->deployables();
    const QChar sep = QLatin1Char('/');
    for (int i = 0; i < deployables->deployableCount(); ++i) {
        const MaemoDeployable &deployable = deployables->deployableAt(i);
        const QFileInfo localFileInfo(deployable.localFilePath);
        const QString targetFilePath = tc->sysroot() + sep
            + deployable.remoteDir + sep + localFileInfo.fileName();
        sysRootDir.mkpath(deployable.remoteDir.mid(1));
        QString errorMsg;
        MaemoGlobal::removeRecursively(targetFilePath, errorMsg);
        if (!MaemoGlobal::copyRecursively(deployable.localFilePath,
                targetFilePath, &errorMsg)) {
            emit addOutput(tr("Sysroot installation failed: %1\n"
                " Continuing anyway.").arg(errorMsg), ErrorMessageOutput);
        }
        QCoreApplication::processEvents();
        if (fi.isCanceled()) {
            fi.reportResult(false);
            return;
        }
    }
    fi.reportResult(true);
}

BuildStepConfigWidget *MaemoCopyToSysrootStep::createConfigWidget()
{
    return new MaemoCopyFilesToSysrootWidget(this);
}

const QString MaemoCopyToSysrootStep::Id
    = QLatin1String("MaemoCopyToSysrootStep");
const QString MaemoCopyToSysrootStep::DisplayName
    = tr("Copy files to sysroot");


MaemoMakeInstallToSysrootStep::MaemoMakeInstallToSysrootStep(BuildStepList *bsl)
    : AbstractProcessStep(bsl, Id)
{
    setDefaultDisplayName(DisplayName);
}

MaemoMakeInstallToSysrootStep::MaemoMakeInstallToSysrootStep(BuildStepList *bsl,
        MaemoMakeInstallToSysrootStep *other)
    : AbstractProcessStep(bsl, other)
{
    setDefaultDisplayName(DisplayName);
}

bool MaemoMakeInstallToSysrootStep::init()
{
    const Qt4BuildConfiguration * const bc
        = qobject_cast<Qt4BuildConfiguration *>(target()->activeBuildConfiguration());
    if (!bc) {
        addOutput("Can't deploy: No active build dconfiguration.",
            ErrorMessageOutput);
        return false;
    }
    const QtVersion * const qtVersion = bc->qtVersion();
    if (!qtVersion) {
        addOutput("Can't deploy: Unusable build configuration.",
            ErrorMessageOutput);
        return false;

    }
    processParameters()->setCommand(MaemoGlobal::madCommand(qtVersion));
    const QStringList args = QStringList() << QLatin1String("-t")
        << MaemoGlobal::targetName(qtVersion) << QLatin1String("make")
        << QLatin1String("install")
        << (QLatin1String("INSTALL_ROOT=") + qtVersion->systemRoot());
    processParameters()->setArguments(args.join(QLatin1String(" ")));
    processParameters()->setEnvironment(bc->environment());
    processParameters()->setWorkingDirectory(bc->buildDirectory());
    return true;
}

BuildStepConfigWidget *MaemoMakeInstallToSysrootStep::createConfigWidget()
{
    return new MaemoCopyFilesToSysrootWidget(this);
}

const QString MaemoMakeInstallToSysrootStep::Id
    = QLatin1String("MaemoMakeInstallToSysrootStep");
const QString MaemoMakeInstallToSysrootStep::DisplayName
    = tr("Copy files to sysroot");

} // namespace Internal
} // namespace Qt4ProjectManager

#include "maemoinstalltosysrootstep.moc"
