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

#include "deviceprocesslist.h"

#include <QString>

QT_BEGIN_NAMESPACE
class QProcess;
QT_END_NAMESPACE

namespace ProjectExplorer {
namespace Internal {

class LocalProcessList : public DeviceProcessList
{
    Q_OBJECT

public:
    explicit LocalProcessList(const IDevice::ConstPtr &device, QObject *parent = 0);
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    static QList<DeviceProcessItem> getLocalProcesses();

private:
    void doUpdate() override;
    void doKillProcess(const DeviceProcessItem &process) override;

private:
    void handleUpdate();
    void reportDelayedKillStatus(const QString &errorMessage);

    const qint64 m_myPid;
};

} // namespace Internal
} // namespace ProjectExplorer
