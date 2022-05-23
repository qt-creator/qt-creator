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
    bool isCancelOnWait() const; // TODO: The original contained cancelOnWait, what suggests action, not a getter

    void flushFinishedFutures();

private:

    QList<QFuture<void>> m_futures;
    bool m_cancelOnWait = false; // TODO: True default makes more sense...
};

} // namespace Utils
