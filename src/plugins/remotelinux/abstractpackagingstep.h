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

#include "remotelinux_export.h"

#include <projectexplorer/buildstep.h>

namespace RemoteLinux {
class RemoteLinuxDeployConfiguration;

namespace Internal { class AbstractPackagingStepPrivate; }

class REMOTELINUX_EXPORT AbstractPackagingStep : public ProjectExplorer::BuildStep
{
    Q_OBJECT

public:
    AbstractPackagingStep(ProjectExplorer::BuildStepList *bsl, Core::Id id);
    AbstractPackagingStep(ProjectExplorer::BuildStepList *bsl, AbstractPackagingStep *other);
    ~AbstractPackagingStep() override;

    QString packageFilePath() const;
    QString cachedPackageFilePath() const;
    bool init(QList<const BuildStep *> &earlierSteps) override;

signals:
    void packageFilePathChanged();
    void unmodifyDeploymentData();

protected:
    void setPackagingStarted();
    void setPackagingFinished(bool success);

    void raiseError(const QString &errorMessage);
    void raiseWarning(const QString &warningMessage);
    QString cachedPackageDirectory() const;
    QString packageDirectory() const;

    virtual bool isPackagingNeeded() const;

private:
    void handleBuildConfigurationChanged();
    void setDeploymentDataUnmodified();
    void setDeploymentDataModified();

    virtual QString packageFileName() const = 0;

    void ctor();

    Internal::AbstractPackagingStepPrivate *d;
};

} // namespace RemoteLinux
