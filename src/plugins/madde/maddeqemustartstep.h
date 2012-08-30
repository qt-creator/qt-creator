/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/
#ifndef MAEMOQEMUCHECKSTEP_H
#define MAEMOQEMUCHECKSTEP_H

#include <remotelinux/abstractremotelinuxdeploystep.h>

namespace Madde {
namespace Internal {
class MaddeQemuStartService;

class MaddeQemuStartStep : public RemoteLinux::AbstractRemoteLinuxDeployStep
{
    Q_OBJECT
public:
    MaddeQemuStartStep(ProjectExplorer::BuildStepList *bsl);
    MaddeQemuStartStep(ProjectExplorer::BuildStepList *bsl, MaddeQemuStartStep *other);

    static Core::Id stepId();
    static QString stepDisplayName();

private:
    void ctor();

    RemoteLinux::AbstractRemoteLinuxDeployService *deployService() const;
    bool initInternal(QString *error = 0);

    MaddeQemuStartService *m_service;
};

} // namespace Internal
} // namespace Madde

#endif // MAEMOQEMUCHECKSTEP_H
