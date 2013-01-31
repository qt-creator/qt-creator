/**************************************************************************
**
** Copyright (c) 2013 BogDan Vatra <bog_dan_ro@yahoo.com>
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

class DeployItem
{
public:
    DeployItem(const QString &_localFileName,
               unsigned int _localTimeStamp,
               const QString &_remoteFileName,
               bool _needsStrip)
        : localFileName(_localFileName),
          remoteFileName(_remoteFileName),
          localTimeStamp(_localTimeStamp),
          remoteTimeStamp(0),
          needsStrip(_needsStrip)
    {}
    QString localFileName;
    QString remoteFileName;
    unsigned int localTimeStamp;
    unsigned int remoteTimeStamp;
    bool needsStrip;
};

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

    void cleanLibsOnDevice();

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
    void cleanLibsFinished();

private:
    AndroidDeployStep(ProjectExplorer::BuildStepList *bc,
        AndroidDeployStep *other);
    virtual bool init();
    virtual void run(QFutureInterface<bool> &fi);
    virtual ProjectExplorer::BuildStepConfigWidget *createConfigWidget();
    virtual bool immutable() const { return true; }

    void ctor();
    void raiseError(const QString &error);
    void writeOutput(const QString &text, OutputFormat = MessageOutput);
    bool runCommand(QProcess *buildProc, const QString &program, const QStringList &arguments);
    unsigned int remoteModificationTime(const QString &fullDestination,
                               QHash<QString, unsigned int> *cache);
    void collectFiles(QList<DeployItem> *deployList, const QString &localPath,
                      const QString &remotePath, bool strip, const QStringList &filter = QStringList());
    void filterModificationTimes(QList<DeployItem> *deployList);
    void copyFilesToTemp(QList<DeployItem> *deployList, const QString &tempDirectory, const QString &sourcePrefix);
    void fetchRemoteModificationTimes(QList<DeployItem> *deployList);
    void stripFiles(const QList<DeployItem> &deployList, ProjectExplorer::Abi::Architecture architecture);
    void deployFiles(QProcess *process, const QList<DeployItem> &deployList);

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
