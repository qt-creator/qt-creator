// Copyright (C) 2018 Benjamin Balga
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

namespace SerialTerminal {
namespace Constants {

const char OUTPUT_PANE_TITLE[] = QT_TRANSLATE_NOOP("QtC::SerialTerminal", "Serial Terminal");

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
