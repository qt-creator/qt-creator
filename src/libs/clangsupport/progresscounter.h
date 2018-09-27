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

#include <functional>

namespace ClangBackEnd {

class ProgressCounter
{
public:
    using SetProgressCallback = std::function<void(int, int)>;

    ProgressCounter(SetProgressCallback &&progressCallback)
        : m_progressCallback(std::move(progressCallback))
    {}

    void addTotal(int total)
    {
        m_total += total;

        m_progressCallback(m_progress, m_total);
    }

    void removeTotal(int total)
    {
        m_total -= total;

        sendProgress();
    }

    void addProgress(int progress)
    {
        m_progress += progress;

        sendProgress();
    }

    void sendProgress()
    {
        m_progressCallback(m_progress, m_total);

        if (m_progress >= m_total) {
            m_progress = 0;
            m_total = 0;
        }
    }

    int total() const
    {
        return m_total;
    }

    int progress() const
    {
        return m_total;
    }

private:
    std::function<void(int, int)> m_progressCallback;
    int m_progress = 0;
    int m_total = 0;
};

}
