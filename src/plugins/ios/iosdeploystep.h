/****************************************************************************
**
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
#ifndef IOSDEPLOYSTEP_H
#define IOSDEPLOYSTEP_H

#include "iosconfigurations.h"
#include "iosdevice.h"
#include "iossimulator.h"

#include <projectexplorer/buildstep.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <qtsupport/baseqtversion.h>

#include <QFutureInterface>
#include <QProcess>

QT_BEGIN_NAMESPACE
class QEventLoop;
class QTimer;
QT_END_NAMESPACE

namespace Ios {
class IosToolHandler;
namespace Internal {
class IosDeviceConfigListModel;
class IosPackageCreationStep;

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

class IosDeployStep : public ProjectExplorer::BuildStep
{
    Q_OBJECT
public:
    enum TransferStatus {
        NoTransfer,
        TransferInProgress,
        TransferOk,
        TransferFailed
    };

    friend class IosDeployStepFactory;
    IosDeployStep(ProjectExplorer::BuildStepList *bc);

    virtual ~IosDeployStep();

    bool fromMap(const QVariantMap &map);
    QVariantMap toMap() const;

    virtual void run(QFutureInterface<bool> &fi);
    void cleanup();
    ProjectExplorer::IDevice::ConstPtr device() const;
    IosDevice::ConstPtr iosdevice() const;
    IosSimulator::ConstPtr iossimulator() const;
    void cancel();
signals:
    //void done();
    //void error();

private slots:
    void handleIsTransferringApp(Ios::IosToolHandler *handler, const QString &bundlePath,
                           const QString &deviceId, int progress, int maxProgress,
                           const QString &info);
    void handleDidTransferApp(Ios::IosToolHandler *handler, const QString &bundlePath, const QString &deviceId,
                        Ios::IosToolHandler::OpStatus status);
    void handleFinished(Ios::IosToolHandler *handler);
    void handleErrorMsg(Ios::IosToolHandler *handler, const QString &msg);

private:
    IosDeployStep(ProjectExplorer::BuildStepList *bc,
        IosDeployStep *other);
    bool init();
    ProjectExplorer::BuildStepConfigWidget *createConfigWidget();
    bool immutable() const { return true; }
    bool runInGuiThread() const { return true; }

    void ctor();
    QString deviceId() const;
    QString appBundle() const;
    void raiseError(const QString &error);
    void writeOutput(const QString &text, OutputFormat = MessageOutput);
private:
    TransferStatus m_transferStatus;
    IosToolHandler *m_toolHandler;
    QFutureInterface<bool> m_futureInterface;
    ProjectExplorer::IDevice::ConstPtr m_device;
    static const Core::Id Id;
};

} // namespace Internal
} // namespace Ios

#endif // IOSDEPLOYSTEP_H
