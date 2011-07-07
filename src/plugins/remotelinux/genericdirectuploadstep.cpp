/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/
#include "genericdirectuploadstep.h"

#include "deployablefile.h"
#include "deploymentinfo.h"
#include "genericdirectuploadservice.h"
#include "qt4maemodeployconfiguration.h"

#include <QtCore/QList>
#include <QtCore/QSharedPointer>

namespace RemoteLinux {

GenericDirectUploadStep::GenericDirectUploadStep(ProjectExplorer::BuildStepList *bsl, const QString &id)
    : AbstractRemoteLinuxDeployStep(bsl, id)
{
    ctor();
}

GenericDirectUploadStep::GenericDirectUploadStep(ProjectExplorer::BuildStepList *bsl, GenericDirectUploadStep *other)
    : AbstractRemoteLinuxDeployStep(bsl, other)
{
    ctor();
}

bool GenericDirectUploadStep::isDeploymentPossible(QString *whyNot) const
{
    QList<DeployableFile> deployableFiles;
    const QSharedPointer<DeploymentInfo> deploymentInfo = deployConfiguration()->deploymentInfo();
    const int deployableCount = deploymentInfo->deployableCount();
    for (int i = 0; i < deployableCount; ++i)
        deployableFiles << deploymentInfo->deployableAt(i);
    m_deployService->setDeployableFiles(deployableFiles);
    return AbstractRemoteLinuxDeployStep::isDeploymentPossible(whyNot);
}

AbstractRemoteLinuxDeployService *GenericDirectUploadStep::deployService() const
{
    return m_deployService;
}

void GenericDirectUploadStep::ctor()
{
    setDefaultDisplayName(displayName());
    m_deployService = new GenericDirectUploadService(this);
}

QString GenericDirectUploadStep::stepId()
{
    return QLatin1String("RemoteLinux.DirectUploadStep");
}

QString GenericDirectUploadStep::displayName()
{
    return tr("Upload files via SFTP");
}

} //namespace RemoteLinux
