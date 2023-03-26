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
class DirectoryPathCompressor
{
public:
    DirectoryPathCompressor() { m_timer.setSingleShot(true); }

    virtual ~DirectoryPathCompressor() = default;

    void addSourceContextId(SourceContextId sourceContextId)
    {
        auto found = std::lower_bound(m_sourceContextIds.begin(),
                                      m_sourceContextIds.end(),
                                      sourceContextId);

        if (found == m_sourceContextIds.end() || *found != sourceContextId)
            m_sourceContextIds.insert(found, sourceContextId);

        restartTimer();
    }

    SourceContextIds takeSourceContextIds() { return std::move(m_sourceContextIds); }

    virtual void setCallback(std::function<void(QmlDesigner::SourceContextIds &&)> &&callback)
    {
        QObject::connect(&m_timer, &Timer::timeout, [this, callback = std::move(callback)] {
            callback(takeSourceContextIds());
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
    SourceContextIds m_sourceContextIds;
    Timer m_timer;
};

} // namespace QmlDesigner
