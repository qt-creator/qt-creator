// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "processinterface.h"

#include "qtcassert.h"

namespace Utils {

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
    case ControlSignal::KickOff:   QTC_CHECK(false); return 0;
    }
    return 0;
}

} // Utils
