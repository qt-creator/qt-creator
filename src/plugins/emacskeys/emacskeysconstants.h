/****************************************************************************
**
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

namespace EmacsKeys {
namespace Constants {

const char DELETE_CHARACTER[]         = "EmacsKeys.DeleteCharacter";
const char KILL_WORD[]                = "EmacsKeys.KillWord";
const char KILL_LINE[]                = "EmacsKeys.KillLine";
const char INSERT_LINE_AND_INDENT[]   = "EmacsKeys.InsertLineAndIndent";

const char GOTO_FILE_START[]          = "EmacsKeys.GotoFileStart";
const char GOTO_FILE_END[]            = "EmacsKeys.GotoFileEnd";
const char GOTO_LINE_START[]          = "EmacsKeys.GotoLineStart";
const char GOTO_LINE_END[]            = "EmacsKeys.GotoLineEnd";
const char GOTO_NEXT_LINE[]           = "EmacsKeys.GotoNextLine";
const char GOTO_PREVIOUS_LINE[]       = "EmacsKeys.GotoPreviousLine";
const char GOTO_NEXT_CHARACTER[]      = "EmacsKeys.GotoNextCharacter";
const char GOTO_PREVIOUS_CHARACTER[]  = "EmacsKeys.GotoPreviousCharacter";
const char GOTO_NEXT_WORD[]           = "EmacsKeys.GotoNextWord";
const char GOTO_PREVIOUS_WORD[]       = "EmacsKeys.GotoPreviousWord";

const char MARK[]                     = "EmacsKeys.Mark";
const char EXCHANGE_CURSOR_AND_MARK[] = "EmacsKeys.ExchangeCursorAndMark";
const char COPY[]                     = "EmacsKeys.Copy";
const char CUT[]                      = "EmacsKeys.Cut";
const char YANK[]                     = "EmacsKeys.Yank";

const char SCROLL_HALF_DOWN[]         = "EmacsKeys.ScrollHalfDown";
const char SCROLL_HALF_UP[]           = "EmacsKeys.ScrollHalfUp";

} // namespace EmacsKeys
} // namespace Constants
