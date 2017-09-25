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

#include "nodeinstanceserverinterface.h"

#include <QPointer>
#include <QProcess>
#include <QFile>
#include <QTime>
#include <QTimer>

QT_BEGIN_NAMESPACE
class QLocalServer;
class QLocalSocket;
class QProcess;
QT_END_NAMESPACE

namespace ProjectExplorer {
class Kit;
class Project;
}

namespace QmlDesigner {

class NodeInstanceClientInterface;
class NodeInstanceView;
class NodeInstanceClientProxy;

class NodeInstanceServerProxy : public NodeInstanceServerInterface
{
    Q_OBJECT

public:
    enum PuppetStreamType {
        FirstPuppetStream,
        SecondPuppetStream,
        ThirdPuppetStream,
    };

    explicit NodeInstanceServerProxy(NodeInstanceView *nodeInstanceView,
                                     RunModus runModus,
                                     ProjectExplorer::Kit *kit,
                                     ProjectExplorer::Project *project);
    ~NodeInstanceServerProxy();
    void createInstances(const CreateInstancesCommand &command) override;
    void changeFileUrl(const ChangeFileUrlCommand &command) override;
    void createScene(const CreateSceneCommand &command) override;
    void clearScene(const ClearSceneCommand &command) override;
    void removeInstances(const RemoveInstancesCommand &command) override;
    void removeProperties(const RemovePropertiesCommand &command) override;
    void changePropertyBindings(const ChangeBindingsCommand &command) override;
    void changePropertyValues(const ChangeValuesCommand &command) override;
    void changeAuxiliaryValues(const ChangeAuxiliaryCommand &command) override;
    void reparentInstances(const ReparentInstancesCommand &command) override;
    void changeIds(const ChangeIdsCommand &command) override;
    void changeState(const ChangeStateCommand &command) override;
    void completeComponent(const CompleteComponentCommand &command) override;
    void changeNodeSource(const ChangeNodeSourceCommand &command) override;
    void token(const TokenCommand &command) override;
    void removeSharedMemory(const RemoveSharedMemoryCommand &command) override;
    void benchmark(const QString &message) override;

protected:
    void writeCommand(const QVariant &command);
    void dispatchCommand(const QVariant &command, PuppetStreamType puppetStreamType);
    NodeInstanceClientInterface *nodeInstanceClient() const;
    void puppetAlive(PuppetStreamType puppetStreamType);
    QString qrcMappingString() const;

signals:
    void processCrashed();

private slots:
    void processFinished();
    void processFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void readFirstDataStream();
    void readSecondDataStream();
    void readThirdDataStream();

    void printEditorProcessOutput();
    void printPreviewProcessOutput();
    void printRenderProcessOutput();
    void showCannotConnectToPuppetWarningAndSwitchToEditMode();
private:
    QFile m_captureFileForTest;
    QTimer m_firstTimer;
    QTimer m_secondTimer;
    QTimer m_thirdTimer;
    QPointer<QLocalServer> m_localServer;
    QPointer<QLocalSocket> m_firstSocket;
    QPointer<QLocalSocket> m_secondSocket;
    QPointer<QLocalSocket> m_thirdSocket;
    QPointer<NodeInstanceView> m_nodeInstanceView;
    QPointer<QProcess> m_qmlPuppetEditorProcess;
    QPointer<QProcess> m_qmlPuppetPreviewProcess;
    QPointer<QProcess> m_qmlPuppetRenderProcess;
    quint32 m_firstBlockSize = 0;
    quint32 m_secondBlockSize = 0;
    quint32 m_thirdBlockSize = 0;
    quint32 m_writeCommandCounter = 0;
    quint32 m_firstLastReadCommandCounter = 0;
    quint32 m_secondLastReadCommandCounter = 0;
    quint32 m_thirdLastReadCommandCounter = 0;
    RunModus m_runModus;
    int m_synchronizeId = -1;
    QTime m_benchmarkTimer;
};

} // namespace QmlDesigner
