/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

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
    ~GenericDirectUploadStep() override;

    ProjectExplorer::BuildStepConfigWidget *createConfigWidget() override;
    bool initInternal(QString *error = 0) override;

    void setIncrementalDeployment(bool incremental);
    bool incrementalDeployment() const;

    void setIgnoreMissingFiles(bool ignoreMissingFiles);
    bool ignoreMissingFiles() const;

    static Core::Id stepId();
    static QString displayName();

private:
    GenericDirectUploadService *deployService() const override;
    bool fromMap(const QVariantMap &map) override;
    QVariantMap toMap() const override;

    void ctor();

    Internal::GenericDirectUploadStepPrivate *d;
};

} //namespace RemoteLinux
