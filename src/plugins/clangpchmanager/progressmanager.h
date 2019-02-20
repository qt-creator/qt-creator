/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "progressmanagerinterface.h"

#include <QFutureInterface>
#include <QCoreApplication>
#include <QString>

#include <functional>
#include <memory>

namespace ClangPchManager {

class ProgressManager : public ProgressManagerInterface
{
public:
    using Promise = QFutureInterface<void>;
    using Callback = std::function<void(Promise &)>;

    ProgressManager(Callback &&callback)
        : m_callback(std::move(callback))
    {}


    void setProgress(int currentProgress,  int maximumProgress)
    {
        if (!m_promise)
            initialize();

        if (m_promise->progressMaximum() != maximumProgress)
            m_promise->setProgressRange(0, maximumProgress);
        m_promise->setProgressValue(currentProgress);

        if (currentProgress >= maximumProgress)
            finish();
    }

    Promise *promise()
    {
        return m_promise.get();
    }
private:
    void initialize()
    {
        m_promise = std::make_unique<Promise>();
        m_callback(*m_promise);
    }

    void finish()
    {
        m_promise->reportFinished();
        m_promise.reset();
    }

private:
    Callback m_callback;
    std::unique_ptr<Promise> m_promise;
};

} // namespace ClangPchManager
