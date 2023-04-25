// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include <QFuture>
#include <QList>
#include <QtGlobal>

namespace Utils {

class QTCREATOR_UTILS_EXPORT FutureSynchronizer final
{
public:
    FutureSynchronizer() = default;
    ~FutureSynchronizer();

    template <typename T>
    void addFuture(const QFuture<T> &future)
    {
        m_futures.append(QFuture<void>(future));
        flushFinishedFutures();
    }

    bool isEmpty() const;

    void waitForFinished();
    void cancelAllFutures();
    void clearFutures();

    void setCancelOnWait(bool enabled);
    // Note: The QFutureSynchronizer contains cancelOnWait(), what suggests action, not a getter.
    bool isCancelOnWait() const;

    void flushFinishedFutures();

private:
    QList<QFuture<void>> m_futures;
    // Note: This default value is different than QFutureSynchronizer's one. True makes more sense.
    bool m_cancelOnWait = true;
};

} // namespace Utils
