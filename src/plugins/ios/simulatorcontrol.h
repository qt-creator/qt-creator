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
#ifndef SIMULATORCONTROL_H
#define SIMULATORCONTROL_H

#include <QHash>
#include "utils/fileutils.h"

QT_BEGIN_NAMESPACE
class QProcess;
QT_END_NAMESPACE

namespace Ios {
namespace Internal {

class IosDeviceType;
class SimulatorControlPrivate;

class SimulatorControl
{
    explicit SimulatorControl();

public:
    static QList<IosDeviceType> availableSimulators();
    static void updateAvailableSimulators();

    static bool startSimulator(const QString &simUdid);
    static bool isSimulatorRunning(const QString &simUdid);

    static bool installApp(const QString &simUdid, const Utils::FileName &bundlePath, QByteArray &commandOutput);
    static QProcess* spawnAppProcess(const QString &simUdid, const Utils::FileName &bundlePath, qint64 &pId,
                                     bool waitForDebugger, const QStringList &extraArgs);

    static qint64 launchApp(const QString &simUdid, const QString &bundleIdentifier, QByteArray *commandOutput = nullptr);
    static QString bundleIdentifier(const Utils::FileName &bundlePath);
    static QString bundleExecutable(const Utils::FileName &bundlePath);
    static bool waitForProcessSpawn(qint64 processPId);

private:
    static SimulatorControlPrivate *d;
};
} // namespace Internal
} // namespace Ios
#endif // SIMULATORCONTROL_H
