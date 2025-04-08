// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectstorageids.h"

#include <QDir>
#include <QTimer>

#include <utils/algorithm.h>

#include <functional>

namespace QmlDesigner {

template<typename Timer>
class DirectoryPathCompressor final
{
public:
    DirectoryPathCompressor() { m_timer.setSingleShot(true); }

    ~DirectoryPathCompressor() = default;

    void addDirectoryPathId(DirectoryPathId directoryPathId)
    {
        auto found = std::ranges::lower_bound(m_directoryPathIds, directoryPathId);

        if (found == m_directoryPathIds.end() || *found != directoryPathId)
            m_directoryPathIds.insert(found, directoryPathId);

        restartTimer();
    }

    const DirectoryPathIds &directoryPathIds() { return m_directoryPathIds; }

    virtual void setCallback(std::function<void(const QmlDesigner::DirectoryPathIds &)> &&callback)
    {
        if (connection)
            QObject::disconnect(connection);
        connection = QObject::connect(&m_timer, &Timer::timeout, [this, callback = std::move(callback)] {
            try {
                callback(m_directoryPathIds);
                m_directoryPathIds.clear();
            } catch (const std::exception &) {
            }
        });
    }

    virtual void restartTimer()
    {
        m_timer.start(20);
    }

    Timer &timer()
    {
        return m_timer;
    }

private:
    struct ConnectionGuard
    {
        ~ConnectionGuard()
        {
            if (connection)
                QObject::disconnect(connection);
        }

        QMetaObject::Connection &connection;
    };

private:
    DirectoryPathIds m_directoryPathIds;
    QMetaObject::Connection connection;
    ConnectionGuard connectionGuard{connection};
    Timer m_timer;
};

} // namespace QmlDesigner
