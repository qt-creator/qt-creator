/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef CPPPLUGIN_GLOBAL_H
#define CPPPLUGIN_GLOBAL_H

namespace CppEditor {
namespace Constants {

const char M_CONTEXT[] = "CppEditor.ContextMenu";
const char C_CPPEDITOR[] = "CppPlugin.C++Editor";
const char CPPEDITOR_ID[] = "CppPlugin.C++Editor";
const char CPPEDITOR_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("OpenWith::Editors", "C++ Editor");
const char SWITCH_DECLARATION_DEFINITION[] = "CppEditor.SwitchDeclarationDefinition";
const char RENAME_SYMBOL_UNDER_CURSOR[] = "CppEditor.RenameSymbolUnderCursor";
const char FIND_USAGES[] = "CppEditor.FindUsages";
const char M_REFACTORING_MENU_INSERTION_POINT[] = "CppEditor.RefactorGroup";
const char UPDATE_CODEMODEL[] = "CppEditor.UpdateCodeModel";

const int TYPE_HIERARCHY_PRIORITY = 700;
const char TYPE_HIERARCHY_ID[] = "CppEditor.TypeHierarchy";
const char OPEN_TYPE_HIERARCHY[] = "CppEditor.OpenTypeHierarchy";

const char C_SOURCE_MIMETYPE[] = "text/x-csrc";
const char C_HEADER_MIMETYPE[] = "text/x-chdr";
const char CPP_SOURCE_MIMETYPE[] = "text/x-c++src";
const char CPP_HEADER_MIMETYPE[] = "text/x-c++hdr";

const char WIZARD_CATEGORY[] = "O.C++";
const char WIZARD_TR_CATEGORY[] = QT_TRANSLATE_NOOP("CppEditor", "C++");

const char CPP_SNIPPETS_GROUP_ID[] = "C++";

} // namespace Constants
} // namespace CppEditor

#endif // CPPPLUGIN_GLOBAL_H
