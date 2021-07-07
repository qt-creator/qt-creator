/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "utils_global.h"

#include <QtCore/qobject.h>

QT_BEGIN_NAMESPACE
class QLocalServer;
QT_END_NAMESPACE

namespace Utils {
namespace Internal {
class LauncherProcess;
class LauncherSocket;
}

class QTCREATOR_UTILS_EXPORT LauncherInterface : public QObject
{
    Q_OBJECT
public:
    static LauncherInterface &instance();
    ~LauncherInterface() override;

    static void startLauncher() { instance().doStart(); }
    static void stopLauncher() { instance().doStop(); }
    static Internal::LauncherSocket *socket() { return instance().m_socket; }

signals:
    void errorOccurred(const QString &error);

private:
    LauncherInterface();

    void doStart();
    void doStop();
    void handleNewConnection();
    void handleProcessError();
    void handleProcessFinished();
    void handleProcessStderr();

    QLocalServer * const m_server;
    Internal::LauncherSocket *const m_socket;
    Internal::LauncherProcess *m_process = nullptr;
    int m_startRequests = 0;
};

} // namespace Utils
