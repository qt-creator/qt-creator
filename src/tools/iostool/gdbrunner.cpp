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

#include "gdbrunner.h"

#include "iostool.h"
#include "mobiledevicelib.h"

#ifdef Q_OS_UNIX
#include <unistd.h>
#include <fcntl.h>
#endif

namespace Ios {

GdbRunner::GdbRunner(IosTool *iosTool, ServiceConnRef conn) :
    QObject(nullptr),
    m_iosTool(iosTool),
    m_conn(conn)
{
}

void GdbRunner::run()
{
    {
        QMutexLocker l(&m_iosTool->m_xmlMutex);
        if (!m_iosTool->splitAppOutput) {
            m_iosTool->out.writeStartElement(QLatin1String("app_output"));
            m_iosTool->inAppOutput = true;
        }
        m_iosTool->outFile.flush();
    }
    Ios::IosDeviceManager::instance()->processGdbServer(m_conn);
    {
        QMutexLocker l(&m_iosTool->m_xmlMutex);
        if (!m_iosTool->splitAppOutput) {
            m_iosTool->inAppOutput = false;
            m_iosTool->out.writeEndElement();
        }
        m_iosTool->outFile.flush();
    }
    MobileDeviceLib::instance().serviceConnectionInvalidate(m_conn);
    m_conn = nullptr;
    m_iosTool->doExit();
    emit finished();
}

void GdbRunner::stop(int phase)
{
    Ios::IosDeviceManager::instance()->stopGdbServer(m_conn, phase);
}

}
