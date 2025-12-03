// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "changeimportsvisitor.h"

#include <qmljs/parser/qmljsast_p.h>

using namespace QmlDesigner;
using namespace QmlDesigner::Internal;

ChangeImportsVisitor::ChangeImportsVisitor(TextModifier &textModifier,
                                           const QString &source):
        QMLRewriter(textModifier), m_source(source)
{}

bool ChangeImportsVisitor::add(QmlJS::AST::UiProgram *ast, const Import &import)
{
    setDidRewriting(false);
    if (!ast)
        return false;

    if (ast->headers && ast->headers->headerItem) {
        int insertionPoint = 0;
        if (ast->members && ast->members->member)
            insertionPoint = ast->members->member->firstSourceLocation().begin();
        else
            insertionPoint = m_source.size();
        while (insertionPoint > 0) {
            --insertionPoint;
            const QChar c = m_source.at(insertionPoint);
            if (!c.isSpace() && c != QLatin1Char(';'))
                break;
        }
        replace(insertionPoint+1, 0, QStringLiteral("\n") + import.toImportString());
    } else {
        replace(0, 0, import.toImportString() + QStringLiteral("\n\n"));
    }

    setDidRewriting(true);

    return true;
}

bool ChangeImportsVisitor::remove(QmlJS::AST::UiProgram *ast, const Import &import)
{
    setDidRewriting(false);
    if (!ast)
        return false;

    for (QmlJS::AST::UiHeaderItemList *iter = ast->headers; iter; iter = iter->next) {
        auto iterImport = QmlJS::AST::cast<QmlJS::AST::UiImport *>(iter->headerItem);
        if (equals(iterImport, import)) {
            int start = iterImport->firstSourceLocation().begin();
            int end = iterImport->lastSourceLocation().end();
            includeSurroundingWhitespace(start, end);
            replace(start, end - start, QString());
            setDidRewriting(true);
        }
    }

    return didRewriting();
}

namespace {
bool isEqual(QmlJS::AST::UiImport *ast, const Import &import)
{
    if (import.isLibraryImport())
        return toString(ast->importUri) == import.url();
    else if (import.isFileImport())
        return ast->fileName == import.file();

    return false;
}
} // namespace

bool ChangeImportsVisitor::equals(QmlJS::AST::UiImport *ast, const Import &import)
{
    if (isEqual(ast, import)) {
        if (ast->version) {
            return std::array{ast->version->majorVersion, ast->version->minorVersion}
                   == import.versions();
        }
        return true;
    }

    return false;
}
