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

#include <qabstractanimation.h>
#include <QtCore/qbasictimer.h>
#include <QtCore/qelapsedtimer.h>
#include <QtCore/qmath.h>

class AnimationDriver : public QAnimationDriver
{
    Q_OBJECT
public:
    AnimationDriver(QObject *parent = nullptr);
    ~AnimationDriver();
    void timerEvent(QTimerEvent *e) override;
    void setInterval(int interval)
    {
        m_interval = qBound(1, interval, 60);
    }
    int interval() const
    {
        return m_interval;
    }
    void reset()
    {
        m_elapsedTimer.invalidate();
        m_pauseBegin = 0;
        m_pauseTime = 0;
        m_elapsed = 0;
        m_seekerElapsed = 0;
        stopTimer();
    }
    void restart()
    {
        m_pauseTime = 0;
        m_elapsed = 0;
        m_seekerElapsed = 0;
        startTimer();
    }
    void pause()
    {
        m_pauseBegin = m_elapsedTimer.elapsed();
        stopTimer();
    }
    void play()
    {
        if (m_elapsedTimer.isValid())
            m_pauseTime += m_elapsedTimer.elapsed() - m_pauseBegin;
        startTimer();
    }
    qint64 elapsed() const override
    {
        return m_elapsed + m_seekerElapsed - m_pauseTime;
    }
    void setSeekerPosition(int position);
    void setSeekerEnabled(bool enable)
    {
        m_seekerEnabled = enable;
    }
    bool isSeekerEnabled() const
    {
        return m_seekerEnabled;
    }
    bool isAnimating() const
    {
        return (!m_seekerEnabled && m_timer.isActive()) || (m_seekerEnabled && m_delta && m_seekerPos);
    }
Q_SIGNALS:
    void advanced();

private:
    Q_SLOT void startTimer();
    Q_SLOT void stopTimer();

    QBasicTimer m_timer;
    QElapsedTimer m_elapsedTimer;
    int m_interval = 16;
    int m_seekerPos = 0;
    bool m_seekerEnabled = false;
    qint64 m_elapsed = 0;
    qint64 m_seekerElapsed = 0;
    qint64 m_delta = 0;
    qint64 m_pauseTime = 0;
    qint64 m_pauseBegin = 0;
};
