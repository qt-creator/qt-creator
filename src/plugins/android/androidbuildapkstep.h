/**************************************************************************
**
** Copyright (C) 2015 BogDan Vatra <bog_dan_ro@yahoo.com>
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

#ifndef ANDROIDBUILDAPKSTEP_H
#define ANDROIDBUILDAPKSTEP_H

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

    bool fromMap(const QVariantMap &map);
    QVariantMap toMap() const;

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

    bool runInGuiThread() const;

    QString buildTargetSdk() const;
    void setBuildTargetSdk(const QString &sdk);

    virtual Utils::FileName androidPackageSourceDir() const = 0;
public slots:
    void setDeployAction(AndroidDeployAction deploy);

protected slots:
    void showInGraphicalShell();

protected:
    AndroidBuildApkStep(ProjectExplorer::BuildStepList *bc,
        AndroidBuildApkStep *other);
    bool keystorePassword();
    bool certificatePassword();

    bool init();
    ProjectExplorer::BuildStepConfigWidget *createConfigWidget();
    bool immutable() const { return true; }
    void processFinished(int exitCode, QProcess::ExitStatus status);

protected:
    AndroidDeployAction m_deployAction;
    bool m_signPackage;
    bool m_verbose;
    bool m_useGradle;
    bool m_openPackageLocation;
    bool m_openPackageLocationForRun;
    QString m_buildTargetSdk;

    Utils::FileName m_keystorePath;
    QString m_keystorePasswd;
    QString m_certificateAlias;
    QString m_certificatePasswd;
    QString m_apkPath;
};

} // namespace Android

#endif // ANDROIDBUILDAPKSTEP_H
