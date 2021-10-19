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

#include "animationdriver.h"
#include "private/qabstractanimation_p.h"

AnimationDriver::AnimationDriver(QObject *parent)
    : QAnimationDriver(parent)
{
    setProperty("allowNegativeDelta", true);
    install();
    connect(this, SIGNAL(started()), this, SLOT(startTimer()));
    connect(this, SIGNAL(stopped()), this, SLOT(stopTimer()));
}

AnimationDriver::~AnimationDriver()
{

}

void AnimationDriver::timerEvent(QTimerEvent *e)
{
    Q_ASSERT(e->timerId() == m_timer.timerId());
    Q_UNUSED(e);

    quint32 old = elapsed();
    // Provide same time for all users
    if (m_seekerEnabled) {
        m_seekerElapsed += (m_seekerPos * 100) / 30;
        if (m_seekerElapsed + m_elapsed < -100) // -100 to allow small negative value
            m_seekerElapsed = -m_elapsed - 100;
    } else {
        m_elapsed = QAnimationDriver::elapsed();
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
        restart();

    m_seekerPos = position;
}
