// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "statuslabel.h"

#include <QTimer>

/*!
    \class Utils::StatusLabel
    \inmodule QtCreator

    \brief The StatusLabel class displays messages for a while with a timeout.
*/

namespace Utils {

StatusLabel::StatusLabel(QWidget *parent) : QLabel(parent)
{
    // A manual size let's us shrink below minimum text width which is what
    // we want in [fake] status bars.
    setMinimumSize(QSize(30, 10));
}

void StatusLabel::stopTimer()
{
    if (m_timer && m_timer->isActive())
        m_timer->stop();
}

void StatusLabel::showStatusMessage(const QString &message, int timeoutMS)
{
    setText(message);
    if (timeoutMS > 0) {
        if (!m_timer) {
            m_timer = new QTimer(this);
            m_timer->setSingleShot(true);
            connect(m_timer, &QTimer::timeout, this, &StatusLabel::slotTimeout);
        }
        m_timer->start(timeoutMS);
    } else {
        m_lastPermanentStatusMessage = message;
        stopTimer();
    }
}

void StatusLabel::slotTimeout()
{
    setText(m_lastPermanentStatusMessage);
}

void StatusLabel::clearStatusMessage()
{
    stopTimer();
    m_lastPermanentStatusMessage.clear();
    clear();
}

} // namespace Utils
