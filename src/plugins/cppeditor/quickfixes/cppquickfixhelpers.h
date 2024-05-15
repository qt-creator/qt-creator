// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../cpprefactoringchanges.h"

#include <QStringList>

namespace CppEditor::Internal {
class CppQuickFixInterface;

// These are generated functions that should not be offered in quickfixes.
const QStringList magicQObjectFunctions();

// Given include is e.g. "afile.h" or <afile.h> (quotes/angle brackets included!).
void insertNewIncludeDirective(
    const QString &include,
    CppRefactoringFilePtr file,
    const CPlusPlus::Document::Ptr &cppDocument,
    Utils::ChangeSet &changes);

// Returns a non-null value if and only if the cursor is on the name of a (proper) class
// declaration or at some place inside the body of a class declaration that does not
// correspond to an AST of its own, i.e. on "empty space".
CPlusPlus::ClassSpecifierAST *astForClassOperations(const CppQuickFixInterface &interface);

bool nameIncludesOperatorName(const CPlusPlus::Name *name);

QString inlinePrefix(const Utils::FilePath &targetFile,
                     const std::function<bool()> &extraCondition = {});

CPlusPlus::Class *isMemberFunction(
    const CPlusPlus::LookupContext &context, CPlusPlus::Function *function);

CPlusPlus::Namespace *isNamespaceFunction(
    const CPlusPlus::LookupContext &context, CPlusPlus::Function *function);

} // namespace CppEditor::Internal
