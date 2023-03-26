// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "animationdriver.h"
#include "private/qabstractanimation_p.h"

AnimationDriver::AnimationDriver(QObject *parent)
    : QAnimationDriver(parent)
{
    setProperty("allowNegativeDelta", true);
    install();
}

AnimationDriver::~AnimationDriver()
{

}

void AnimationDriver::timerEvent([[maybe_unused]] QTimerEvent *e)
{
    Q_ASSERT(e->timerId() == m_timer.timerId());

    quint32 old = elapsed();
    // Provide same time for all users
    if (m_seekerEnabled) {
        m_seekerElapsed += (m_seekerPos * 100) / 30;
        if (m_seekerElapsed + m_elapsed - m_pauseTime < -100) // -100 to allow small negative value
            m_seekerElapsed = -(m_elapsed - m_pauseTime) - 100;
    } else {
        if (!m_elapsedTimer.isValid())
            m_elapsedTimer.restart();
        else
            m_elapsed = m_elapsedTimer.elapsed();
    }
    m_delta = elapsed() - old;
    advance();
    Q_EMIT advanced();
}

void AnimationDriver::startTimer()
{
    m_timer.start(m_interval, Qt::PreciseTimer, this);
}

void AnimationDriver::stopTimer()
{
    m_timer.stop();
}

void AnimationDriver::setSeekerPosition(int position)
{
    if (!m_seekerEnabled)
        return;

    if (!m_timer.isActive())
        startTimer();

    m_seekerPos = position;
}
