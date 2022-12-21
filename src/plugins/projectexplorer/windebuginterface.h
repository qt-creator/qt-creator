// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QMap>
#include <QThread>

#include <map>
#include <mutex>
#include <vector>

namespace ProjectExplorer {
namespace Internal {

class WinDebugInterface : public QThread
{
    Q_OBJECT

public:
    explicit WinDebugInterface(QObject *parent = nullptr);
    ~WinDebugInterface() override;

    static WinDebugInterface *instance();

    static bool stop();
    static void startIfNeeded();

signals:
    void debugOutput(qint64 pid, const QString &message);
    void cannotRetrieveDebugOutput();
    void _q_debugOutputReady();

private:
    enum Handles { DataReadyEventHandle, TerminateEventHandle, HandleCount };

    void run() override;
    bool runLoop();
    void emitReadySignal();
    void dispatchDebugOutput();

    static WinDebugInterface *m_instance;

    qint64 m_creatorPid = -1;
    Qt::HANDLE m_waitHandles[HandleCount] = {};
    Qt::HANDLE m_bufferReadyEvent = nullptr;
    Qt::HANDLE m_sharedFile = nullptr;
    void *m_sharedMem = nullptr;
    std::mutex m_outputMutex;
    bool m_readySignalEmitted = false;
    std::map<qint64, std::vector<QString>> m_debugOutput;
};

} // namespace Internal
} // namespace ProjectExplorer
