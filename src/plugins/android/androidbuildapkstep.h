/**************************************************************************
**
** Copyright (c) 2014 BogDan Vatra <bog_dan_ro@yahoo.com>
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef ANDROIDBUILDAPKSTEP_H
#define ANDROIDBUILDAPKSTEP_H

#include "android_global.h"
#include <projectexplorer/abstractprocessstep.h>
#include <qtsupport/baseqtversion.h>

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

    bool runInGuiThread() const;

    QString buildTargetSdk() const;
    void setBuildTargetSdk(const QString &sdk);
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
    virtual Utils::FileName androidPackageSourceDir() const = 0;

protected:
    AndroidDeployAction m_deployAction;
    bool m_signPackage;
    bool m_verbose;
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
