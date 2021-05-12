/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "futuresynchronizer.h"

/*! \class Utils::FutureSynchronizer

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
    clearFutures();
}

void FutureSynchronizer::cancelAllFutures()
{
    for (QFuture<void> &future : m_futures)
        future.cancel();
}

void FutureSynchronizer::clearFutures()
{
    m_futures.clear();
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
    for (const QFuture<void> &future : qAsConst(m_futures)) {
        if (!future.isFinished())
            newFutures.append(future);
    }
    m_futures = newFutures;
}

} // namespace Utils
