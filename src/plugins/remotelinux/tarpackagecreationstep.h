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

#include "abstractpackagingstep.h"
#include "deploymenttimeinfo.h"
#include "remotelinux_export.h"

#include <projectexplorer/deployablefile.h>

QT_BEGIN_NAMESPACE
class QFile;
class QFileInfo;
QT_END_NAMESPACE

namespace RemoteLinux {

class REMOTELINUX_EXPORT TarPackageCreationStep : public AbstractPackagingStep
{
    Q_OBJECT
public:
    TarPackageCreationStep(ProjectExplorer::BuildStepList *bsl);
    TarPackageCreationStep(ProjectExplorer::BuildStepList *bsl, TarPackageCreationStep *other);

    static Core::Id stepId();
    static QString displayName();

    bool init(QList<const BuildStep *> &earlierSteps) override;
    void run(QFutureInterface<bool> &fi) override;

    void setIgnoreMissingFiles(bool ignoreMissingFiles);
    bool ignoreMissingFiles() const;

    void setIncrementalDeployment(bool incrementalDeployment);
    bool isIncrementalDeployment() const;

private:
    void deployFinished(bool success);

    void addNeededDeploymentFiles(const ProjectExplorer::DeployableFile &deployable,
                                  const ProjectExplorer::Kit *kit);

    ProjectExplorer::BuildStepConfigWidget *createConfigWidget() override;
    bool fromMap(const QVariantMap &map) override;
    QVariantMap toMap() const override;

    QString packageFileName() const override;

    void ctor();
    bool doPackage(QFutureInterface<bool> &fi);
    bool appendFile(QFile &tarFile, const QFileInfo &fileInfo,
        const QString &remoteFilePath, const QFutureInterface<bool> &fi);
    bool writeHeader(QFile &tarFile, const QFileInfo &fileInfo,
        const QString &remoteFilePath);

    DeploymentTimeInfo m_deployTimes;

    bool m_incrementalDeployment;
    bool m_ignoreMissingFiles;
    bool m_packagingNeeded;
    QList<ProjectExplorer::DeployableFile> m_files;
};

} // namespace RemoteLinux
