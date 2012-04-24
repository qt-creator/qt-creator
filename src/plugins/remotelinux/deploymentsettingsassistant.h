/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
#ifndef DEPLOYMENTSETTINGSASSISTANT_H
#define DEPLOYMENTSETTINGSASSISTANT_H

#include "remotelinux_export.h"

#include <QObject>
#include <QStringList>

namespace ProjectExplorer { class Project; }

namespace RemoteLinux {
class DeployableFile;
class DeployableFilesPerProFile;
class DeploymentInfo;

namespace Internal { class DeploymentSettingsAssistantInternal; }

class REMOTELINUX_EXPORT DeploymentSettingsAssistant : public QObject
{
    Q_OBJECT

public:
    DeploymentSettingsAssistant(DeploymentInfo *deploymentInfo, ProjectExplorer::Project *parent);
    ~DeploymentSettingsAssistant();

    bool addDeployableToProFile(const QString &qmakeScope,
                                const DeployableFilesPerProFile *proFileInfo,
                                const QString &variableName, const DeployableFile &deployable);

private slots:
    void handleDeploymentInfoUpdated();

private:
    bool addLinesToProFile(const QString &qmakeScope, const DeployableFilesPerProFile *proFileInfo, const QStringList &lines);

    Internal::DeploymentSettingsAssistantInternal * const d;
};

} // namespace RemoteLinux

#endif // DEPLOYMENTSETTINGSASSISTANT_H
