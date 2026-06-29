// Copyright (C) 2018 Benjamin Balga
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

namespace SerialTerminal {
namespace Constants {

inline constexpr char OUTPUT_PANE_TITLE[] = QT_TRANSLATE_NOOP("QtC::SerialTerminal", "Serial Terminal");

// Settings entries
inline constexpr char SETTINGS_GROUP[] = "SerialTerminalPlugin";
inline constexpr char SETTINGS_BAUDRATE[] = "BaudRate";
inline constexpr char SETTINGS_DATABITS[] = "DataBits";
inline constexpr char SETTINGS_PARITY[] = "Parity";
inline constexpr char SETTINGS_STOPBITS[] = "StopBits";
inline constexpr char SETTINGS_FLOWCONTROL[] = "FlowControl";
inline constexpr char SETTINGS_PORTNAME[] = "PortName";
inline constexpr char SETTINGS_INITIAL_DTR_STATE[] = "InitialDtr";
inline constexpr char SETTINGS_INITIAL_RTS_STATE[] = "InitialRts";
inline constexpr char SETTINGS_LINE_ENDINGS[] = "LineEndings";
inline constexpr char SETTINGS_LINE_ENDING_NAME[] = "LineEndingName";
inline constexpr char SETTINGS_LINE_ENDING_VALUE[] = "LineEndingValue";
inline constexpr char SETTINGS_DEFAULT_LINE_ENDING_INDEX[] = "DefaultLineEndingIndex";
inline constexpr char SETTINGS_CLEAR_INPUT_ON_SEND[] = "ClearInputOnSend";


inline constexpr int RECONNECT_DELAY = 1500; // milliseconds
inline constexpr int RESET_DELAY = 100; // milliseconds
inline constexpr int DEFAULT_MAX_ENTRIES = 20; // Max entries in the console line edit

// Context
inline constexpr char C_SERIAL_OUTPUT[]         = "SerialTerminal.SerialOutput";

} // namespace SerialTerminal
} // namespace Constants
