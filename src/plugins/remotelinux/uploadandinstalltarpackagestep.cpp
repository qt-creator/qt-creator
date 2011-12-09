/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/
#include "uploadandinstalltarpackagestep.h"

#include "remotelinuxdeployconfiguration.h"
#include "remotelinuxdeploystepwidget.h"
#include "remotelinuxpackageinstaller.h"
#include "tarpackagecreationstep.h"

#include <projectexplorer/buildsteplist.h>

using namespace ProjectExplorer;

namespace RemoteLinux {
namespace Internal {
namespace {

class ConfigWidget : public BuildStepConfigWidget
{
    Q_OBJECT
public:
    ConfigWidget(UploadAndInstallTarPackageStep *step) : m_widget(step)
    {
        connect(&m_widget, SIGNAL(updateSummary()), SIGNAL(updateSummary()));
    }
    ~ConfigWidget() {}

private:
    QString summaryText() const { return m_widget.summaryText(); }
    QString displayName() const { return m_widget.displayName(); }
    bool showWidget() const { return false; }

    RemoteLinuxDeployStepWidget m_widget;
};
} // anonymous namespace

class UploadAndInstallTarPackageServicePrivate
{
public:
    RemoteLinuxTarPackageInstaller installer;
};

} // namespace Internal

using namespace Internal;

UploadAndInstallTarPackageService::UploadAndInstallTarPackageService(QObject *parent)
    : AbstractUploadAndInstallPackageService(parent),
      d(new UploadAndInstallTarPackageServicePrivate)
{
}

UploadAndInstallTarPackageService::~UploadAndInstallTarPackageService()
{
    delete d;
}

AbstractRemoteLinuxPackageInstaller *UploadAndInstallTarPackageService::packageInstaller() const
{
    return &d->installer;
}


UploadAndInstallTarPackageStep::UploadAndInstallTarPackageStep(BuildStepList *bsl)
    : AbstractRemoteLinuxDeployStep(bsl, stepId())
{
    ctor();
}

UploadAndInstallTarPackageStep::UploadAndInstallTarPackageStep(BuildStepList *bsl,
        UploadAndInstallTarPackageStep *other)
    : AbstractRemoteLinuxDeployStep(bsl, other)
{
    ctor();
}

void UploadAndInstallTarPackageStep::ctor()
{
    m_deployService = new UploadAndInstallTarPackageService(this);
    setDefaultDisplayName(displayName());
}

bool UploadAndInstallTarPackageStep::initInternal(QString *error)
{
    const TarPackageCreationStep * const pStep
        = deployConfiguration()->earlierBuildStep<TarPackageCreationStep>(this);
    if (!pStep) {
        if (error)
            *error = tr("No tarball creation step found.");
        return false;
    }
    m_deployService->setPackageFilePath(pStep->packageFilePath());
    return m_deployService->isDeploymentPossible(error);
}

BuildStepConfigWidget *UploadAndInstallTarPackageStep::createConfigWidget()
{
    return new ConfigWidget(this);
}

QString UploadAndInstallTarPackageStep::stepId()
{
    return QLatin1String("MaemoUploadAndInstallTarPackageStep");
}

QString UploadAndInstallTarPackageStep::displayName()
{
    return tr("Deploy tarball via SFTP upload");
}

} //namespace RemoteLinux

#include "uploadandinstalltarpackagestep.moc"
