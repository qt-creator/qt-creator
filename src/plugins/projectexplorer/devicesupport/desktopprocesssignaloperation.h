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

#ifndef WINDOWSPROCESSSIGNALOPERATION_H
#define WINDOWSPROCESSSIGNALOPERATION_H

#include "idevice.h"

#include <projectexplorer/projectexplorer_export.h>

namespace ProjectExplorer {

class PROJECTEXPLORER_EXPORT DesktopProcessSignalOperation : public DeviceProcessSignalOperation
{
    Q_OBJECT
public:
    ~DesktopProcessSignalOperation() {}
    void killProcess(qint64 pid);
    void killProcess(const QString &filePath);
    void interruptProcess(qint64 pid);
    void interruptProcess(const QString &filePath);

private:
    void killProcessSilently(qint64 pid);
    void interruptProcessSilently(qint64 pid);

    void appendMsgCannotKill(qint64 pid, const QString &why);
    void appendMsgCannotInterrupt(qint64 pid, const QString &why);

protected:
    explicit DesktopProcessSignalOperation() {}

    friend class DesktopDevice;
};

} // namespace ProjectExplorer
#endif // WINDOWSPROCESSSIGNALOPERATION_H
