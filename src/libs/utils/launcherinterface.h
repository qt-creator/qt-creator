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

#include "processutils.h"

#include <QObject>
#include <QThread>

namespace Utils {
namespace Internal {
class CallerHandle;
class LauncherHandle;
class LauncherInterfacePrivate;
class ProcessLauncherImpl;
}

class QTCREATOR_UTILS_EXPORT LauncherInterface : public QObject
{
    Q_OBJECT
public:
    static void startLauncher(const QString &pathToLauncher = {});
    static void stopLauncher();

signals:
    void errorOccurred(const QString &error);

private:
    friend class Utils::Internal::CallerHandle;
    friend class Utils::Internal::LauncherHandle;
    friend class Utils::Internal::ProcessLauncherImpl;

    static bool isStarted();
    static bool isReady();
    static void sendData(const QByteArray &data);
    static Utils::Internal::CallerHandle *registerHandle(QObject *parent, quintptr token,
                                                         ProcessMode mode);
    static void unregisterHandle(quintptr token);

    LauncherInterface();
    ~LauncherInterface() override;

    QThread m_thread;
    Internal::LauncherInterfacePrivate *m_private;
};

} // namespace Utils
