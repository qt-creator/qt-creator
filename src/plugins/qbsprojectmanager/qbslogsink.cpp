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

QbsLogSink::QbsLogSink(QObject *parent) : QObject(parent)
{
}

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
        mm->printToOutputPane(msg);
}

void QbsLogSink::doPrintMessage(qbs::LoggerLevel level, const QString &message, const QString &tag)
{
    Q_UNUSED(tag);
    const QString fullMessage = QString::fromLocal8Bit(qbs::logLevelTag(level)) + message;

    {
        QMutexLocker l(&m_mutex);
        m_messages.append(fullMessage);
    }
    QMetaObject::invokeMethod(this, "sendMessages", Qt::QueuedConnection);
}

} // namespace Internal
} // namespace QbsProjectManager
