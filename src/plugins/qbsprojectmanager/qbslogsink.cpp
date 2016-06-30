/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "qbslogsink.h"

#include <qbs.h>

#include <coreplugin/messagemanager.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/taskhub.h>
#include <utils/fileutils.h>

#include <QCoreApplication>
#include <QMutexLocker>
#include <QTimer>

using namespace ProjectExplorer;

namespace QbsProjectManager {
namespace Internal {

// --------------------------------------------------------------------
// QbsLogSink:
// --------------------------------------------------------------------

QbsLogSink::QbsLogSink(QObject *parent) : QObject(parent)
{
    connect(this, &QbsLogSink::newTask,
            TaskHub::instance(),
            [](const Task &task) { TaskHub::addTask(task); }, Qt::QueuedConnection);
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
        Core::MessageManager::write(msg, Core::MessageManager::Silent);
}

void QbsLogSink::doPrintWarning(const qbs::ErrorInfo &warning)
{
    foreach (const qbs::ErrorItem &item, warning.items())
        emit newTask(Task(Task::Warning,
                          item.description(),
                          Utils::FileName::fromString(item.codeLocation().filePath()),
                          item.codeLocation().line(),
                          Constants::TASK_CATEGORY_BUILDSYSTEM));
}

void QbsLogSink::doPrintMessage(qbs::LoggerLevel level, const QString &message, const QString &tag)
{
    Q_UNUSED(tag);

    {
        QMutexLocker l(&m_mutex);
        if (level <= qbs::LoggerWarning) {
            doPrintWarning(qbs::ErrorInfo(message));
            return;
        }
        m_messages.append(qbs::logLevelTag(level) + message);
    }
    QMetaObject::invokeMethod(this, "sendMessages", Qt::QueuedConnection);
}

} // namespace Internal
} // namespace QbsProjectManager
