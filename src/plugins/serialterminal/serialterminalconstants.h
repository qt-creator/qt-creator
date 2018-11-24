/****************************************************************************
**
** Copyright (C) 2018 Benjamin Balga
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

#pragma once

#include <QtGlobal>

namespace SerialTerminal {
namespace Constants {

const char OUTPUT_PANE_TITLE[] = QT_TRANSLATE_NOOP("SerialTerminal::Internal::SerialTerminalOutputPane", "Serial Terminal");

const char LOGGING_CATEGORY[] = "qtc.serialterminal.outputpane";

// Settings entries
const char SETTINGS_GROUP[] = "SerialTerminalPlugin";
const char SETTINGS_BAUDRATE[] = "BaudRate";
const char SETTINGS_DATABITS[] = "DataBits";
const char SETTINGS_PARITY[] = "Parity";
const char SETTINGS_STOPBITS[] = "StopBits";
const char SETTINGS_FLOWCONTROL[] = "FlowControl";
const char SETTINGS_PORTNAME[] = "PortName";
const char SETTINGS_INITIAL_DTR_STATE[] = "InitialDtr";
const char SETTINGS_INITIAL_RTS_STATE[] = "InitialRts";
const char SETTINGS_LINE_ENDINGS[] = "LineEndings";
const char SETTINGS_LINE_ENDING_NAME[] = "LineEndingName";
const char SETTINGS_LINE_ENDING_VALUE[] = "LineEndingValue";
const char SETTINGS_DEFAULT_LINE_ENDING_INDEX[] = "DefaultLineEndingIndex";
const char SETTINGS_CLEAR_INPUT_ON_SEND[] = "ClearInputOnSend";


const int RECONNECT_DELAY = 1500; // milliseconds
const int RESET_DELAY = 100; // milliseconds
const int DEFAULT_MAX_ENTRIES = 20; // Max entries in the console line edit

// Context
const char C_SERIAL_OUTPUT[]         = "SerialTerminal.SerialOutput";

} // namespace SerialTerminal
} // namespace Constants
