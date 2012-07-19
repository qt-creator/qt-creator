/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 BogDan Vatra <bog_dan_ro@yahoo.com>
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef ANDROIDDEPLOYSTEP_H
#define ANDROIDDEPLOYSTEP_H

#include "androidconfigurations.h"

#include <projectexplorer/buildstep.h>
#include <qtsupport/baseqtversion.h>

#include <QProcess>

QT_BEGIN_NAMESPACE
class QEventLoop;
class QTimer;
QT_END_NAMESPACE

namespace Android {
namespace Internal {
class AndroidDeviceConfigListModel;
class AndroidPackageCreationStep;

class AndroidDeployStep : public ProjectExplorer::BuildStep
{
    Q_OBJECT
    friend class AndroidDeployStepFactory;

public:
    enum AndroidDeployAction
    {
        NoDeploy,
        DeployLocal,
        InstallQASI
    };

public:
    AndroidDeployStep(ProjectExplorer::BuildStepList *bc);

    virtual ~AndroidDeployStep();

    QString deviceSerialNumber();
    int deviceAPILevel();
    Utils::FileName localLibsRulesFilePath();

    AndroidDeployAction deployAction();
    bool useLocalQtLibs();

    bool fromMap(const QVariantMap &map);
    QVariantMap toMap() const;

public slots:
    void setDeployAction(AndroidDeployAction deploy);
    void setDeployQASIPackagePath(const QString &package);
    void setUseLocalQtLibs(bool useLocal);

signals:
    void done();
    void error();
    void resetDelopyAction();

private slots:
    bool deployPackage();
    void handleBuildOutput();
    void handleBuildError();

private:
    AndroidDeployStep(ProjectExplorer::BuildStepList *bc,
        AndroidDeployStep *other);
    virtual bool init();
    virtual void run(QFutureInterface<bool> &fi);
    virtual ProjectExplorer::BuildStepConfigWidget *createConfigWidget();
    virtual bool immutable() const { return true; }

    void copyLibs(const QString &srcPath, const QString &destPath, QStringList &copiedLibs, const QStringList &filter = QStringList());
    void ctor();
    void raiseError(const QString &error);
    void writeOutput(const QString &text, OutputFormat = MessageOutput);
    bool runCommand(QProcess *buildProc, const QString &program, const QStringList &arguments);

private:
    QString m_deviceSerialNumber;
    int m_deviceAPILevel;

    QString m_QASIPackagePath;
    AndroidDeployAction m_deployAction;
    bool m_useLocalQtLibs;

    // members to transfer data from init() to run
    QString m_packageName;
    QString m_qtVersionSourcePath;
    QtSupport::BaseQtVersion::QmakeBuildConfigs m_qtVersionQMakeBuildConfig;
    Utils::FileName m_androidDirPath;
    QString m_apkPathDebug;
    QString m_apkPathRelease;
    QString m_buildDirectory;
    QString m_runQASIPackagePath;
    AndroidDeployAction m_runDeployAction;

    static const Core::Id Id;
};

} // namespace Internal
} // namespace Android

#endif // ANDROIDDEPLOYSTEP_H
