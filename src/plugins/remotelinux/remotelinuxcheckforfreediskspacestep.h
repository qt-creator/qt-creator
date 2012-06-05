/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
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
#ifndef REMOTELINUXCHECKFORFREEDISKSPACESTEP_H
#define REMOTELINUXCHECKFORFREEDISKSPACESTEP_H

#include "abstractremotelinuxdeploystep.h"

namespace RemoteLinux {
namespace Internal { class RemoteLinuxCheckForFreeDiskSpaceStepPrivate; }

class REMOTELINUX_EXPORT RemoteLinuxCheckForFreeDiskSpaceStep : public AbstractRemoteLinuxDeployStep
{
    Q_OBJECT
public:
    explicit RemoteLinuxCheckForFreeDiskSpaceStep(ProjectExplorer::BuildStepList *bsl,
            Core::Id id = stepId());
    RemoteLinuxCheckForFreeDiskSpaceStep(ProjectExplorer::BuildStepList *bsl,
            RemoteLinuxCheckForFreeDiskSpaceStep *other);
    ~RemoteLinuxCheckForFreeDiskSpaceStep();

    void setPathToCheck(const QString &path);
    QString pathToCheck() const;

    void setRequiredSpaceInBytes(quint64 space);
    quint64 requiredSpaceInBytes() const;

    static Core::Id stepId();
    static QString stepDisplayName();

protected:
    bool fromMap(const QVariantMap &map);
    QVariantMap toMap() const;
    ProjectExplorer::BuildStepConfigWidget *createConfigWidget();

    bool initInternal(QString *error);
    AbstractRemoteLinuxDeployService *deployService() const;

private:
    void ctor();

    Internal::RemoteLinuxCheckForFreeDiskSpaceStepPrivate *d;
};

} // namespace RemoteLinux

#endif // REMOTELINUXCHECKFORFREEDISKSPACESTEP_H
