/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

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
