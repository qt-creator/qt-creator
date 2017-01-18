/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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
const char ERRORS_IN_HEADER_FILES[] = "CppEditor.ErrorsInHeaderFiles";
const char MULTIPLE_PARSE_CONTEXTS_AVAILABLE[] = "CppEditor.MultipleParseContextsAvailable";
const char NO_PROJECT_CONFIGURATION[] = "CppEditor.NoProjectConfiguration";
const char M_REFACTORING_MENU_INSERTION_POINT[] = "CppEditor.RefactorGroup";
const char UPDATE_CODEMODEL[] = "CppEditor.UpdateCodeModel";
const char INSPECT_CPP_CODEMODEL[] = "CppEditor.InspectCppCodeModel";

const char TYPE_HIERARCHY_ID[] = "CppEditor.TypeHierarchy";
const char OPEN_TYPE_HIERARCHY[] = "CppEditor.OpenTypeHierarchy";

const char INCLUDE_HIERARCHY_ID[] = "CppEditor.IncludeHierarchy";
const char OPEN_INCLUDE_HIERARCHY[] = "CppEditor.OpenIncludeHierarchy";

const char CPP_SNIPPETS_GROUP_ID[] = "C++";

const char EXTRA_PREPROCESSOR_DIRECTIVES[] = "CppEditor.ExtraPreprocessorDirectives-";
const char PREFERRED_PARSE_CONTEXT[] = "CppEditor.PreferredParseContext-";

} // namespace Constants
} // namespace CppEditor
