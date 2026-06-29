// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace EmacsKeys {
namespace Constants {

inline constexpr char DELETE_CHARACTER[]         = "EmacsKeys.DeleteCharacter";
inline constexpr char KILL_WORD[]                = "EmacsKeys.KillWord";
inline constexpr char KILL_LINE[]                = "EmacsKeys.KillLine";
inline constexpr char INSERT_LINE_AND_INDENT[]   = "EmacsKeys.InsertLineAndIndent";

inline constexpr char GOTO_FILE_START[]          = "EmacsKeys.GotoFileStart";
inline constexpr char GOTO_FILE_END[]            = "EmacsKeys.GotoFileEnd";
inline constexpr char GOTO_LINE_START[]          = "EmacsKeys.GotoLineStart";
inline constexpr char GOTO_LINE_END[]            = "EmacsKeys.GotoLineEnd";
inline constexpr char GOTO_NEXT_LINE[]           = "EmacsKeys.GotoNextLine";
inline constexpr char GOTO_PREVIOUS_LINE[]       = "EmacsKeys.GotoPreviousLine";
inline constexpr char GOTO_NEXT_CHARACTER[]      = "EmacsKeys.GotoNextCharacter";
inline constexpr char GOTO_PREVIOUS_CHARACTER[]  = "EmacsKeys.GotoPreviousCharacter";
inline constexpr char GOTO_NEXT_WORD[]           = "EmacsKeys.GotoNextWord";
inline constexpr char GOTO_PREVIOUS_WORD[]       = "EmacsKeys.GotoPreviousWord";

inline constexpr char MARK[]                     = "EmacsKeys.Mark";
inline constexpr char EXCHANGE_CURSOR_AND_MARK[] = "EmacsKeys.ExchangeCursorAndMark";
inline constexpr char COPY[]                     = "EmacsKeys.Copy";
inline constexpr char CUT[]                      = "EmacsKeys.Cut";
inline constexpr char YANK[]                     = "EmacsKeys.Yank";

inline constexpr char SCROLL_HALF_DOWN[]         = "EmacsKeys.ScrollHalfDown";
inline constexpr char SCROLL_HALF_UP[]           = "EmacsKeys.ScrollHalfUp";

} // namespace EmacsKeys
} // namespace Constants
