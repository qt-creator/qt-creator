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

#include "processutils.h"
#include <QProcess>

#ifdef Q_OS_WIN
#ifdef QTCREATOR_PCH_H
#define CALLBACK WINAPI
#endif
#include <qt_windows.h>
#endif

namespace Utils {

QIODevice::OpenMode ProcessStartHandler::openMode() const
{
    if (m_processMode == ProcessMode::Writer)
        return QIODevice::ReadWrite; // some writers also read data
    if (m_writeData.isEmpty())
        return QIODevice::ReadOnly; // only reading
    return QIODevice::ReadWrite; // initial write and then reading (close the write channel)
}

void ProcessStartHandler::handleProcessStart(QProcess *process)
{
    if (m_processMode == ProcessMode::Writer)
        return;
    if (m_writeData.isEmpty())
        process->closeWriteChannel();
}

void ProcessStartHandler::handleProcessStarted(QProcess *process)
{
    if (!m_writeData.isEmpty()) {
        process->write(m_writeData);
        m_writeData = {};
        if (m_processMode == ProcessMode::Reader)
            process->closeWriteChannel();
    }
}


void ProcessStartHandler::setBelowNormalPriority(QProcess *process)
{
#ifdef Q_OS_WIN
    process->setCreateProcessArgumentsModifier(
        [](QProcess::CreateProcessArguments *args) {
            args->flags |= BELOW_NORMAL_PRIORITY_CLASS;
    });
#else
    Q_UNUSED(process)
#endif // Q_OS_WIN
}

void ProcessStartHandler::setNativeArguments(QProcess *process, const QString &arguments)
{
#ifdef Q_OS_WIN
    if (!arguments.isEmpty())
        process->setNativeArguments(arguments);
#else
    Q_UNUSED(process)
    Q_UNUSED(arguments)
#endif // Q_OS_WIN
}

} // namespace Utils
