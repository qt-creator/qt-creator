// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppquickfixhelpers.h"

#include "../includeutils.h"
#include "cppquickfixassistant.h"

using namespace CPlusPlus;
using namespace Utils;

namespace CppEditor::Internal {

void insertNewIncludeDirective(
    const QString &include,
    CppRefactoringFilePtr file,
    const Document::Ptr &cppDocument,
    ChangeSet &changes)
{
    // Find optimal position
    unsigned newLinesToPrepend = 0;
    unsigned newLinesToAppend = 0;
    const int insertLine = lineForNewIncludeDirective(
        file->filePath(),
        file->document(),
        cppDocument,
        IgnoreMocIncludes,
        AutoDetect,
        include,
        &newLinesToPrepend,
        &newLinesToAppend);
    QTC_ASSERT(insertLine >= 1, return);
    const int insertPosition = file->position(insertLine, 1);
    QTC_ASSERT(insertPosition >= 0, return);

    // Construct text to insert
    const QString includeLine = QLatin1String("#include ") + include + QLatin1Char('\n');
    QString prependedNewLines, appendedNewLines;
    while (newLinesToAppend--)
        appendedNewLines += QLatin1String("\n");
    while (newLinesToPrepend--)
        prependedNewLines += QLatin1String("\n");
    const QString textToInsert = prependedNewLines + includeLine + appendedNewLines;

    // Insert
    changes.insert(insertPosition, textToInsert);
}

ClassSpecifierAST *astForClassOperations(const CppQuickFixInterface &interface)
{
    const QList<AST *> &path = interface.path();
    if (path.isEmpty())
        return nullptr;
    if (const auto classSpec = path.last()->asClassSpecifier()) // Cursor inside class decl?
        return classSpec;

    // Cursor on a class name?
    if (path.size() < 2)
        return nullptr;
    const SimpleNameAST * const nameAST = path.at(path.size() - 1)->asSimpleName();
    if (!nameAST || !interface.isCursorOn(nameAST))
        return nullptr;
    if (const auto classSpec = path.at(path.size() - 2)->asClassSpecifier())
        return classSpec;
    return nullptr;
}

} // namespace CppEditor::Internal
