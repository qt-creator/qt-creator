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
#ifndef DIRECTUPLOADSTEP_H
#define DIRECTUPLOADSTEP_H

#include "abstractremotelinuxdeploystep.h"
#include "genericdirectuploadservice.h"
#include "remotelinux_export.h"

namespace RemoteLinux {
namespace Internal { class GenericDirectUploadStepPrivate; }

class REMOTELINUX_EXPORT GenericDirectUploadStep : public AbstractRemoteLinuxDeployStep
{
    Q_OBJECT

public:
    GenericDirectUploadStep(ProjectExplorer::BuildStepList *bsl, Core::Id id);
    GenericDirectUploadStep(ProjectExplorer::BuildStepList *bsl, GenericDirectUploadStep *other);
    ~GenericDirectUploadStep();

    ProjectExplorer::BuildStepConfigWidget *createConfigWidget();
    bool initInternal(QString *error = 0);

    void setIncrementalDeployment(bool incremental);
    bool incrementalDeployment() const;

    void setIgnoreMissingFiles(bool ignoreMissingFiles);
    bool ignoreMissingFiles() const;

    static Core::Id stepId();
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
