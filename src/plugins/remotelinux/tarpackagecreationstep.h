// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

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
