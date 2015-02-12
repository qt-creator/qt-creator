/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qbslogsink.h"

#include <qbs.h>

#include <coreplugin/messagemanager.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/taskhub.h>
#include <utils/fileutils.h>

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
    connect(this, SIGNAL(newTask(ProjectExplorer::Task)),
            ProjectExplorer::TaskHub::instance(),
            SLOT(addTask(ProjectExplorer::Task)), Qt::QueuedConnection);
}

void QbsLogSink::sendMessages()
{
    QStringList toSend;
    {
        QMutexLocker l(&m_mutex);
        toSend = m_messages;
        m_messages.clear();
    }

    foreach (const QString &msg, toSend)
        Core::MessageManager::write(msg);
}

void QbsLogSink::doPrintWarning(const qbs::ErrorInfo &warning)
{
    foreach (const qbs::ErrorItem &item, warning.items())
        emit newTask(ProjectExplorer::Task(ProjectExplorer::Task::Warning,
                                           item.description(),
                                           Utils::FileName::fromString(item.codeLocation().filePath()),
                                           item.codeLocation().line(),
                                           ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));
}

void QbsLogSink::doPrintMessage(qbs::LoggerLevel level, const QString &message, const QString &tag)
{
    Q_UNUSED(tag);

    {
        QMutexLocker l(&m_mutex);
        m_messages.append(qbs::logLevelTag(level) + message);
    }
    QMetaObject::invokeMethod(this, "sendMessages", Qt::QueuedConnection);
}

} // namespace Internal
} // namespace QbsProjectManager
