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

QT_BEGIN_NAMESPACE
class QFile;
class QFileInfo;
QT_END_NAMESPACE

namespace ProjectExplorer { class DeployableFile; }

namespace RemoteLinux {

namespace Internal { class TarPackageCreationStepPrivate; }

class REMOTELINUX_EXPORT TarPackageCreationStep : public ProjectExplorer::BuildStep
{
    Q_OBJECT

public:
    explicit TarPackageCreationStep(ProjectExplorer::BuildStepList *bsl, Utils::Id id);
    ~TarPackageCreationStep() override;

    static Utils::Id stepId();
    static QString displayName();

    Utils::FilePath packageFilePath() const;

private:
    bool init() override;
    void doRun() override;
    bool fromMap(const QVariantMap &map) override;
    QVariantMap toMap() const override;

    void raiseError(const QString &errorMessage);
    void raiseWarning(const QString &warningMessage);
    bool isPackagingNeeded() const;
    void deployFinished(bool success);
    void addNeededDeploymentFiles(const ProjectExplorer::DeployableFile &deployable,
                                  const ProjectExplorer::Kit *kit);
    bool runImpl();
    bool doPackage();
    bool appendFile(QFile &tarFile, const QFileInfo &fileInfo, const QString &remoteFilePath);

    std::unique_ptr<Internal::TarPackageCreationStepPrivate> d;
};

} // namespace RemoteLinux
