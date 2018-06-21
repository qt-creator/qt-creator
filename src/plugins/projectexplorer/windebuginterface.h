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

#include <QThread>

namespace ProjectExplorer {
namespace Internal {

class WinDebugInterface : public QThread
{
    Q_OBJECT

public:
    explicit WinDebugInterface(QObject *parent = nullptr);
    ~WinDebugInterface() override;

    static WinDebugInterface *instance();

    bool stop();

signals:
    void debugOutput(qint64 pid, const QString &message);
    void cannotRetrieveDebugOutput();

private:
    enum Handles { DataReadyEventHandle, TerminateEventHandle, HandleCount };

    void run() override;
    bool runLoop();

    static WinDebugInterface *m_instance;

    qint64 m_creatorPid = -1;
    Qt::HANDLE m_waitHandles[HandleCount] = {};
    Qt::HANDLE m_bufferReadyEvent = nullptr;
    Qt::HANDLE m_sharedFile = nullptr;
    void *m_sharedMem = nullptr;
};

} // namespace Internal
} // namespace ProjectExplorer
