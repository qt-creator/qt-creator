// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectstorageids.h"
#include "projectstoragetracing.h"

#include <QDir>
#include <QTimer>

#include <utils/algorithm.h>

#include <functional>

namespace QmlDesigner {

template<typename Timer>
class DirectoryPathCompressor final
{
public:
    DirectoryPathCompressor()
    {
        NanotraceHR::Tracer tracer{"directory path compressor constructor",
                                   ProjectStorageTracing::category()};

        m_timer.setSingleShot(true);
    }

    ~DirectoryPathCompressor() = default;

    void addDirectoryPathId(DirectoryPathId directoryPathId)
    {
        NanotraceHR::Tracer tracer{"directory path compressor add directory path id",
                                   ProjectStorageTracing::category()};

        auto found = std::ranges::lower_bound(m_directoryPathIds, directoryPathId);

        if (found == m_directoryPathIds.end() || *found != directoryPathId)
            m_directoryPathIds.insert(found, directoryPathId);

        restartTimer();
    }

    const DirectoryPathIds &directoryPathIds()
    {
        NanotraceHR::Tracer tracer{"directory path compressor get directory path ids",
                                   ProjectStorageTracing::category()};

        return m_directoryPathIds;
    }

    virtual void setCallback(std::function<void(const QmlDesigner::DirectoryPathIds &)> &&callback)
    {
        NanotraceHR::Tracer tracer{"directory path compressor set callback",
                                   ProjectStorageTracing::category()};

        if (connection)
            QObject::disconnect(connection);
        connection = QObject::connect(&m_timer, &Timer::timeout, [this, callback = std::move(callback)] {
            NanotraceHR::Tracer tracer{"directory path compressor call callback",
                                       ProjectStorageTracing::category()};

            try {
                callback(m_directoryPathIds);
                m_directoryPathIds.clear();
            } catch (const std::exception &) {
            }
        });
    }

    virtual void restartTimer()
    {
        NanotraceHR::Tracer tracer{"directory path compressor restart timer",
                                   ProjectStorageTracing::category()};

        m_timer.start(20);
    }

    Timer &timer()
    {
        NanotraceHR::Tracer tracer{"directory path compressor get timer",
                                   ProjectStorageTracing::category()};

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
