/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/
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
