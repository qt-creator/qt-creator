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

    void addSourceContextId(SourceContextId sourceContextId)
    {
        auto found = std::lower_bound(m_sourceContextIds.begin(),
                                      m_sourceContextIds.end(),
                                      sourceContextId);

        if (found == m_sourceContextIds.end() || *found != sourceContextId)
            m_sourceContextIds.insert(found, sourceContextId);

        restartTimer();
    }

    const SourceContextIds &sourceContextIds() { return m_sourceContextIds; }

    virtual void setCallback(std::function<void(const QmlDesigner::SourceContextIds &)> &&callback)
    {
        if (connection)
            QObject::disconnect(connection);
        connection = QObject::connect(&m_timer, &Timer::timeout, [this, callback = std::move(callback)] {
            try {
                callback(m_sourceContextIds);
                m_sourceContextIds.clear();
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
    SourceContextIds m_sourceContextIds;
    QMetaObject::Connection connection;
    ConnectionGuard connectionGuard{connection};
    Timer m_timer;
};

} // namespace QmlDesigner
