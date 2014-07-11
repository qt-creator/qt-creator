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

#ifndef ANDROIDDEPLOYQTSTEP_H
#define ANDROIDDEPLOYQTSTEP_H

#include "androidconfigurations.h"

#include <projectexplorer/abstractprocessstep.h>
#include <qtsupport/baseqtversion.h>

QT_BEGIN_NAMESPACE
class QAbstractItemModel;
QT_END_NAMESPACE

namespace Android {
namespace Internal {

class AndroidDeployQtStepFactory : public ProjectExplorer::IBuildStepFactory
{
    Q_OBJECT
public:
    explicit AndroidDeployQtStepFactory(QObject *parent = 0);

    QList<Core::Id> availableCreationIds(ProjectExplorer::BuildStepList *parent) const;
    QString displayNameForId(Core::Id id) const;

    bool canCreate(ProjectExplorer::BuildStepList *parent, Core::Id id) const;
    ProjectExplorer::BuildStep *create(ProjectExplorer::BuildStepList *parent, Core::Id id);

    bool canRestore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map) const;
    ProjectExplorer::BuildStep *restore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map);

    bool canClone(ProjectExplorer::BuildStepList *parent,
                  ProjectExplorer::BuildStep *product) const;
    ProjectExplorer::BuildStep *clone(ProjectExplorer::BuildStepList *parent,
                                      ProjectExplorer::BuildStep *product);
};

class AndroidDeployQtStep : public ProjectExplorer::AbstractProcessStep
{
    Q_OBJECT
    friend class AndroidDeployQtStepFactory;
public:
    AndroidDeployQtStep(ProjectExplorer::BuildStepList *bc);

    enum AndroidDeployQtAction
    {
        MinistroDeployment, // use ministro
        DebugDeployment,
        BundleLibrariesDeployment
    };

    bool fromMap(const QVariantMap &map);
    QVariantMap toMap() const;

    AndroidDeployQtStep::AndroidDeployQtAction deployAction() const;
    QString deviceSerialNumber();

    void setBuildTargetSdk(const QString &sdk);
    QString buildTargetSdk() const;

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

    QString proFilePathForInputFile() const;
    void setProFilePathForInputFile(const QString &path);

    bool runInGuiThread() const;

signals:
    // also on purpose emitted if the possible values of this changed
    void inputFileChanged();

public slots:
    void setDeployAction(AndroidDeployQtAction deploy); // slot?

private slots:
    void showInGraphicalShell();

    void updateInputFile();
private:
    AndroidDeployQtStep(ProjectExplorer::BuildStepList *bc,
        AndroidDeployQtStep *other);
    void ctor();
    bool keystorePassword();
    bool certificatePassword();
    void runCommand(const QString &program, const QStringList &arguments);

    bool init();
    void run(QFutureInterface<bool> &fi);
    ProjectExplorer::BuildStepConfigWidget *createConfigWidget();
    bool immutable() const { return true; }
    void processFinished(int exitCode, QProcess::ExitStatus status);

    QString m_buildTargetSdk;
    QString m_serialNumber;
    AndroidDeployQtAction m_deployAction;
    bool m_signPackage;
    bool m_verbose;
    bool m_openPackageLocation;
    bool m_openPackageLocationForRun;
    QString m_buildDirectory;

    Utils::FileName m_keystorePath;
    QString m_keystorePasswd;
    QString m_certificateAlias;
    QString m_certificatePasswd;
    QString m_avdName;
    QString m_apkPath;
    QString m_targetArch;
    QString m_proFilePathForInputFile;
    int m_deviceAPILevel;

    static const Core::Id Id;
};

}
}

#endif // ANDROIDDEPLOYQTSTEP_H
