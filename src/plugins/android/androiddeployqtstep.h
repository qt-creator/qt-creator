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

#ifndef ANDROIDDEPLOYQTSTEP_H
#define ANDROIDDEPLOYQTSTEP_H

#include "androidbuildapkstep.h"
#include "androidconfigurations.h"

#include <projectexplorer/abstractprocessstep.h>
#include <qtsupport/baseqtversion.h>

namespace Utils {
class QtcProcess;
}

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

    QList<Core::Id> availableCreationIds(ProjectExplorer::BuildStepList *parent) const override;
    QString displayNameForId(Core::Id id) const override;

    bool canCreate(ProjectExplorer::BuildStepList *parent, Core::Id id) const override;
    ProjectExplorer::BuildStep *create(ProjectExplorer::BuildStepList *parent, Core::Id id) override;

    bool canRestore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map) const override;
    ProjectExplorer::BuildStep *restore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map) override;

    bool canClone(ProjectExplorer::BuildStepList *parent,
                  ProjectExplorer::BuildStep *product) const override;
    ProjectExplorer::BuildStep *clone(ProjectExplorer::BuildStepList *parent,
                                      ProjectExplorer::BuildStep *product) override;
};

class AndroidDeployQtStep : public ProjectExplorer::BuildStep
{
    Q_OBJECT
    friend class AndroidDeployQtStepFactory;
public:
    enum UninstallType {
        Keep,
        Uninstall,
        ForceUnintall
    };
public:
    explicit AndroidDeployQtStep(ProjectExplorer::BuildStepList *bc);

    bool fromMap(const QVariantMap &map) override;
    QVariantMap toMap() const override;

    bool runInGuiThread() const override;

    UninstallType uninstallPreviousPackage();

    AndroidDeviceInfo deviceInfo() const;

public slots:
    void setUninstallPreviousPackage(bool uninstall);

signals:
    void askForUninstall();
    void setSerialNumber(const QString &serialNumber);

private:
    AndroidDeployQtStep(ProjectExplorer::BuildStepList *bc, AndroidDeployQtStep *other);
    void ctor();
    void runCommand(const QString &program, const QStringList &arguments);

    bool init(QList<const BuildStep *> &earlierSteps) override;
    void run(QFutureInterface<bool> &fi) override;
    enum DeployResult { Success, Failure, AskUinstall };
    DeployResult runDeploy(QFutureInterface<bool> &fi);
    void slotAskForUninstall();
    void slotSetSerialNumber(const QString &serialNumber);

    ProjectExplorer::BuildStepConfigWidget *createConfigWidget() override;
    bool immutable() const override { return true; }

    void processReadyReadStdOutput();
    void stdOutput(const QString &line);
    void processReadyReadStdError();
    void stdError(const QString &line);

    void slotProcessFinished(int, QProcess::ExitStatus);
    void processFinished(int exitCode, QProcess::ExitStatus status);

    Utils::FileName m_manifestName;
    QString m_serialNumber;
    QString m_buildDirectory;
    QString m_avdName;
    QString m_apkPath;
    QStringList m_appProcessBinaries;
    QString m_libdir;

    QString m_targetArch;
    bool m_uninstallPreviousPackage;
    bool m_uninstallPreviousPackageRun;
    bool m_installOk;
    bool m_useAndroiddeployqt;
    bool m_askForUinstall;
    static const Core::Id Id;
    QString m_androiddeployqtArgs;
    QString m_adbPath;
    QString m_command;
    QString m_workingDirectory;
    Utils::Environment m_environment;
    Utils::QtcProcess *m_process;
    AndroidDeviceInfo m_deviceInfo;
};

}
} // namespace Android

#endif // ANDROIDDEPLOYQTSTEP_H
