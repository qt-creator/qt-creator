// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../projectexplorer_export.h"
#include "idevicefwd.h"

#include <QAbstractItemModel>
#include <QList>

#include <memory>

namespace Utils { class ProcessInfo; }

namespace ProjectExplorer {

namespace Internal { class DeviceProcessListPrivate; }

class PROJECTEXPLORER_EXPORT ProcessList : public QObject
{
    Q_OBJECT

public:
    ProcessList(const IDeviceConstPtr &device, QObject *parent = nullptr);
    ~ProcessList() override;

    void update();
    void killProcess(int row);

    Utils::ProcessInfo at(int row) const;
    QAbstractItemModel *model() const;

signals:
    void processListUpdated();
    void error(const QString &errorMsg);
    void processKilled();

private:
    void handleUpdate();
    void reportDelayedKillStatus(const QString &errorMessage);

    void setFinished();

    const std::unique_ptr<Internal::DeviceProcessListPrivate> d;
};

} // namespace ProjectExplorer
