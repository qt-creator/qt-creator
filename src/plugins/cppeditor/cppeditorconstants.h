/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef CPPEDITORCONSTANTS_H
#define CPPEDITORCONSTANTS_H

namespace CppEditor {
namespace Constants {

const char M_CONTEXT[] = "CppEditor.ContextMenu";
const char CPPEDITOR_ID[] = "CppEditor.C++Editor";
const char CPPEDITOR_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("OpenWith::Editors", "C++ Editor");
const char SWITCH_DECLARATION_DEFINITION[] = "CppEditor.SwitchDeclarationDefinition";
const char OPEN_DECLARATION_DEFINITION_IN_NEXT_SPLIT[] = "CppEditor.OpenDeclarationDefinitionInNextSplit";
const char RENAME_SYMBOL_UNDER_CURSOR[] = "CppEditor.RenameSymbolUnderCursor";
const char FIND_USAGES[] = "CppEditor.FindUsages";
const char OPEN_PREPROCESSOR_DIALOG[] = "CppEditor.OpenPreprocessorDialog";
const char M_REFACTORING_MENU_INSERTION_POINT[] = "CppEditor.RefactorGroup";
const char UPDATE_CODEMODEL[] = "CppEditor.UpdateCodeModel";
const char INSPECT_CPP_CODEMODEL[] = "CppEditor.InspectCppCodeModel";

const char TYPE_HIERARCHY_ID[] = "CppEditor.TypeHierarchy";
const char OPEN_TYPE_HIERARCHY[] = "CppEditor.OpenTypeHierarchy";

const char INCLUDE_HIERARCHY_ID[] = "CppEditor.IncludeHierarchy";
const char OPEN_INCLUDE_HIERARCHY[] = "CppEditor.OpenIncludeHierarchy";

const char C_SOURCE_MIMETYPE[] = "text/x-csrc";
const char C_HEADER_MIMETYPE[] = "text/x-chdr";
const char CPP_SOURCE_MIMETYPE[] = "text/x-c++src";
const char CPP_HEADER_MIMETYPE[] = "text/x-c++hdr";

const char CPP_SNIPPETS_GROUP_ID[] = "C++";

const char CPP_PREPROCESSOR_PROJECT_PREFIX[] = "CppPreprocessorProject-";

} // namespace Constants
} // namespace CppEditor

#endif // CPPEDITORCONSTANTS_H
