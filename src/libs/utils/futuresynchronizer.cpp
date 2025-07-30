// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "futuresynchronizer.h"

#include "qtcassert.h"
#include "threadutils.h"

/*!
  \class Utils::FutureSynchronizer
  \inmodule QtCreator

  \brief The FutureSynchronizer is an enhanced version of QFutureSynchronizer.
*/

namespace Utils {

FutureSynchronizer::~FutureSynchronizer()
{
    waitForFinished();
}

bool FutureSynchronizer::isEmpty() const
{
    return m_futures.isEmpty();
}

void FutureSynchronizer::waitForFinished()
{
    if (m_cancelOnWait)
        cancelAllFutures();
    for (QFuture<void> &future : m_futures)
        future.waitForFinished();
    m_futures.clear();
}

void FutureSynchronizer::cancelAllFutures()
{
    for (QFuture<void> &future : m_futures)
        future.cancel();
}

void FutureSynchronizer::setCancelOnWait(bool enabled)
{
    m_cancelOnWait = enabled;
}

bool FutureSynchronizer::isCancelOnWait() const
{
    return m_cancelOnWait;
}

void FutureSynchronizer::flushFinishedFutures()
{
    QList<QFuture<void>> newFutures;
    for (const QFuture<void> &future : std::as_const(m_futures)) {
        if (!future.isFinished())
            newFutures.append(future);
    }
    m_futures = newFutures;
}

void FutureSynchronizer::addFutureImpl(const QFuture<void> &future)
{
    m_futures.append(future);
    flushFinishedFutures();
}

Q_GLOBAL_STATIC(FutureSynchronizer, s_futureSynchronizer);

/*!
    Returns a global FutureSynchronizer.
    The application should cancel and wait for the tasks in this synchronizer before actually
    unloading any libraries. This is for example done by the plugin manager in Qt Creator.
    May only be accessed by the main thread.
*/
FutureSynchronizer *futureSynchronizer()
{
    QTC_ASSERT(isMainThread(), return nullptr);
    return s_futureSynchronizer;
}

} // namespace Utils
