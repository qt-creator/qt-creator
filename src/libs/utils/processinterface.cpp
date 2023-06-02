// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "processinterface.h"

#include "qtcassert.h"

namespace Utils {

namespace Pty {

void Data::resize(const QSize &size)
{
    m_size = size;
    if (m_data->m_handler)
        m_data->m_handler(size);
}

} // namespace Pty

/*!
 * \brief controlSignalToInt
 * \param controlSignal
 * \return Converts the ControlSignal enum to the corresponding unix signal
 */
int ProcessInterface::controlSignalToInt(ControlSignal controlSignal)
{
    switch (controlSignal) {
    case ControlSignal::Terminate: return 15;
    case ControlSignal::Kill:      return 9;
    case ControlSignal::Interrupt: return 2;
    case ControlSignal::KickOff:   return 19;
    case ControlSignal::CloseWriteChannel:
        QTC_CHECK(false);
        return 0;
    }
    return 0;
}

} // Utils
