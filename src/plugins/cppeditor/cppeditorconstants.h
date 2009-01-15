/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#ifndef CPPPLUGIN_GLOBAL_H
#define CPPPLUGIN_GLOBAL_H

namespace CppEditor {
namespace Constants {

const char * const FORMATCODE   = "CppEditor.FormatCode";
const char * const M_CONTEXT    = "CppEditor.ContextMenu";
const char * const C_CPPEDITOR  = "C++ Editor";
const char * const CPPEDITOR_KIND = "C++ Editor";
const char * const SWITCH_DECLARATION_DEFINITION = "CppEditor.SwitchDeclarationDefinition";
const char * const JUMP_TO_DEFINITION = "CppEditor.JumpToDefinition";

const char * const HEADER_FILE_TYPE = "CppHeaderFiles";
const char * const SOURCE_FILE_TYPE = "CppSourceFiles";

const char * const MOVE_TO_PREVIOUS_TOKEN = "CppEditor.MoveToPreviousToken";
const char * const MOVE_TO_NEXT_TOKEN = "CppEditor.MoveToNextToken";

const char * const C_SOURCE_MIMETYPE = "text/x-csrc";
const char * const C_HEADER_MIMETYPE = "text/x-chdr";
const char * const CPP_SOURCE_MIMETYPE = "text/x-c++src";
const char * const CPP_HEADER_MIMETYPE = "text/x-c++hdr";

} // namespace Constants
} // namespace CppEditor

#endif // CPPPLUGIN_GLOBAL_H
