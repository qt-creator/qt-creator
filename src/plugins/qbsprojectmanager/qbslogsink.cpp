/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qbslogsink.h"

#include <qbs.h>

#include <coreplugin/messagemanager.h>

#include <QCoreApplication>
#include <QMutexLocker>
#include <QTimer>

namespace QbsProjectManager {
namespace Internal {

// --------------------------------------------------------------------
// QbsLogSink:
// --------------------------------------------------------------------

void QbsLogSink::sendMessages()
{
    QStringList toSend;
    {
        QMutexLocker l(&m_mutex);
        toSend = m_messages;
        m_messages.clear();
    }

    Core::MessageManager *mm = Core::MessageManager::instance();
    foreach (const QString &msg, toSend)
        mm->printToOutputPanePopup(msg);
}

void QbsLogSink::outputLogMessage(qbs::LoggerLevel level, const qbs::LogMessage &logMessage)
{
    QString msg;
    if (logMessage.printLogLevel) {
        QByteArray levelTag = qbs::Logger::logLevelTag(level);
        qbs::Internal::TextColor color = qbs::Internal::TextColorDefault;
        switch (level) {
        case qbs::LoggerError:
            color = qbs::Internal::TextColorRed;
            break;
        case qbs::LoggerWarning:
            color = qbs::Internal::TextColorYellow;
            break;
        default:
            break;
        }
        msg = colorize(color, levelTag);
    }

    msg.append(colorize(logMessage.textColor, logMessage.data));

    {
        QMutexLocker l(&m_mutex);
        m_messages.append(msg);
    }
    QMetaObject::invokeMethod(this, "sendMessages", Qt::QueuedConnection);
}

QString QbsLogSink::colorize(qbs::Internal::TextColor color, const QByteArray &text)
{
    switch (color) {
    case qbs::Internal::TextColorBlack:
        return QString::fromLatin1("<font color=\"rgb(0,0, 0)\">%1</font>").arg(QString::fromLocal8Bit(text));
    case qbs::Internal::TextColorDarkRed:
        return QString::fromLatin1("<font color=\"rgb(170,0,0)\">%1</font>").arg(QString::fromLocal8Bit(text));
    case qbs::Internal::TextColorDarkGreen:
        return QString::fromLatin1("<font color=\"rgb(0,170,0)\">%1</font>").arg(QString::fromLocal8Bit(text));
    case qbs::Internal::TextColorDarkBlue:
        return QString::fromLatin1("<font color=\"rgb(0,0,170)\">%1</font>").arg(QString::fromLocal8Bit(text));
    case qbs::Internal::TextColorDarkCyan:
        return QString::fromLatin1("<font color=\"rgb(0,170,170)\">%1</font>").arg(QString::fromLocal8Bit(text));
    case qbs::Internal::TextColorDarkMagenta:
        return QString::fromLatin1("<font color=\"rgb(170,0,170)\">%1</font>").arg(QString::fromLocal8Bit(text));
    case qbs::Internal::TextColorDarkYellow:
        return QString::fromLatin1("<font color=\"rgb(170,85,0)\">%1</font>").arg(QString::fromLocal8Bit(text));
    case qbs::Internal::TextColorGray:
        return QString::fromLatin1("<font color=\"rgb(170,170,170)\">%1</font>").arg(QString::fromLocal8Bit(text));
    case qbs::Internal::TextColorRed:
        return QString::fromLatin1("<font color=\"rgb(255,85,85)\">%1</font>").arg(QString::fromLocal8Bit(text));
    case qbs::Internal::TextColorGreen:
        return QString::fromLatin1("<font color=\"rgb(85,255,85)\">%1</font>").arg(QString::fromLocal8Bit(text));
    case qbs::Internal::TextColorBlue:
        return QString::fromLatin1("<font color=\"rgb(85,85,255)\">%1</font>").arg(QString::fromLocal8Bit(text));
    case qbs::Internal::TextColorCyan:
        return QString::fromLatin1("<font color=\"rgb(85,255,255)\">%1</font>").arg(QString::fromLocal8Bit(text));
    case qbs::Internal::TextColorMagenta:
        return QString::fromLatin1("<font color=\"rgb(255,85,255)\">%1</font>").arg(QString::fromLocal8Bit(text));
    case qbs::Internal::TextColorYellow:
        return QString::fromLatin1("<font color=\"rgb(255,255,85)\">%1</font>").arg(QString::fromLocal8Bit(text));
    case qbs::Internal::TextColorWhite:
        return QString::fromLatin1("<font color=\"rgb(255,255,255)\">%1</font>").arg(QString::fromLocal8Bit(text));
    case qbs::Internal::TextColorBright:
    case qbs::Internal::TextColorDefault:
        // fallthrough:
    default:
        return QString::fromLocal8Bit(text);
    }
}

} // namespace Internal
} // namespace QbsProjectManager
