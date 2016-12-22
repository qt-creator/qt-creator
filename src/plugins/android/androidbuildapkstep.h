/****************************************************************************
**
** Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
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

#include "android_global.h"
#include <projectexplorer/abstractprocessstep.h>

QT_BEGIN_NAMESPACE
class QAbstractItemModel;
QT_END_NAMESPACE

namespace Android {

class ANDROID_EXPORT AndroidBuildApkStep : public ProjectExplorer::AbstractProcessStep
{
    Q_OBJECT
public:
    AndroidBuildApkStep(ProjectExplorer::BuildStepList *bc, const Core::Id id);

    enum AndroidDeployAction
    {
        MinistroDeployment, // use ministro
        DebugDeployment,
        BundleLibrariesDeployment
    };

    bool fromMap(const QVariantMap &map) override;
    QVariantMap toMap() const override;

    AndroidDeployAction deployAction() const;

    // signing
    Utils::FileName keystorePath();
    void setKeystorePath(const Utils::FileName &path);
    void setKeystorePassword(const QString &pwd);
    void setCertificateAlias(const QString &alias);
    void setCertificatePassword(const QString &pwd);

    QAbstractItemModel *keystoreCertificates();
    bool signPackage() const;
    void setSignPackage(bool b);

    bool openPackageLocation() const;
    void setOpenPackageLocation(bool open);

    bool verboseOutput() const;
    void setVerboseOutput(bool verbose);

    bool useGradle() const;
    void setUseGradle(bool b);

    QString buildTargetSdk() const;
    void setBuildTargetSdk(const QString &sdk);

    virtual Utils::FileName androidPackageSourceDir() const = 0;
    void setDeployAction(AndroidDeployAction deploy);

signals:
    void useGradleChanged();

protected:
    Q_INVOKABLE void showInGraphicalShell();

    AndroidBuildApkStep(ProjectExplorer::BuildStepList *bc,
        AndroidBuildApkStep *other);

    bool init(QList<const BuildStep *> &earlierSteps) override;
    ProjectExplorer::BuildStepConfigWidget *createConfigWidget() override;
    bool immutable() const override { return true; }
    void processFinished(int exitCode, QProcess::ExitStatus status) override;
    bool verifyKeystorePassword();
    bool verifyCertificatePassword();

protected:
    AndroidDeployAction m_deployAction = BundleLibrariesDeployment;
    bool m_signPackage = false;
    bool m_verbose = false;
    bool m_useGradle = false;
    bool m_openPackageLocation = false;
    bool m_openPackageLocationForRun = false;
    QString m_buildTargetSdk;

    Utils::FileName m_keystorePath;
    QString m_keystorePasswd;
    QString m_certificateAlias;
    QString m_certificatePasswd;
    QString m_apkPath;
};

} // namespace Android
