// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QByteArray>
#include <QMap>
#include <QObject>

#include <functional>

namespace ProjectExplorer {

class ProjectExplorerPluginPrivate;

class JournaldWatcher : public QObject
{
    Q_OBJECT

public:
    using LogEntry = QMap<QByteArray, QByteArray>;
    using Subscription = std::function<void(const LogEntry&)>;

    ~JournaldWatcher() override;

    static JournaldWatcher *instance();

    static const QByteArray &machineId();

    static bool subscribe(QObject *subscriber, const Subscription &subscription);
    static void unsubscribe(QObject *subscriber);

private:
    JournaldWatcher();

    void handleEntry();

    static JournaldWatcher *m_instance;

    friend class ProjectExplorerPluginPrivate;
};

} // namespace ProjectExplorer
