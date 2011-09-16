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
#ifndef DIRECTUPLOADSTEP_H
#define DIRECTUPLOADSTEP_H

#include "abstractremotelinuxdeploystep.h"
#include "genericdirectuploadservice.h"
#include "remotelinux_export.h"

namespace RemoteLinux {
namespace Internal {
class GenericDirectUploadStepPrivate;
}

class REMOTELINUX_EXPORT GenericDirectUploadStep : public AbstractRemoteLinuxDeployStep
{
    Q_OBJECT

public:
    GenericDirectUploadStep(ProjectExplorer::BuildStepList *bsl, const QString &id);
    GenericDirectUploadStep(ProjectExplorer::BuildStepList *bsl, GenericDirectUploadStep *other);
    ~GenericDirectUploadStep();

    ProjectExplorer::BuildStepConfigWidget *createConfigWidget();
    bool isDeploymentPossible(QString *whyNot = 0) const;

    void setIncrementalDeployment(bool incremental);
    bool incrementalDeployment() const;

    static QString stepId();
    static QString displayName();

private:
    GenericDirectUploadService *deployService() const;
    bool fromMap(const QVariantMap &map);
    QVariantMap toMap() const;

    void ctor();

    Internal::GenericDirectUploadStepPrivate *d;
};

} //namespace RemoteLinux

#endif // DIRECTUPLOADSTEP_H
