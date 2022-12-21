// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
        if (!m_iosTool->m_splitAppOutput) {
            m_iosTool->m_xmlWriter.writeStartElement(QLatin1String("app_output"));
            m_iosTool->m_inAppOutput = true;
        }
        m_iosTool->m_outputFile.flush();
    }
    Ios::IosDeviceManager::instance()->processGdbServer(m_conn);
    {
        QMutexLocker l(&m_iosTool->m_xmlMutex);
        if (!m_iosTool->m_splitAppOutput) {
            m_iosTool->m_inAppOutput = false;
            m_iosTool->m_xmlWriter.writeEndElement();
        }
        m_iosTool->m_outputFile.flush();
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
