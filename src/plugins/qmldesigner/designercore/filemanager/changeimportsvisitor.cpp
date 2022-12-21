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
            insertionPoint = m_source.length();
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

bool ChangeImportsVisitor::equals(QmlJS::AST::UiImport *ast, const Import &import)
{
    bool equal = false;
    if (import.isLibraryImport())
        equal = toString(ast->importUri) == import.url();
    else if (import.isFileImport())
        equal = ast->fileName == import.file();

    if (equal && ast->version) {
        const QStringList versions = import.version().split('.');
        if (versions.size() >= 1 && versions[0].toInt() == ast->version->majorVersion) {
            if (versions.size() >= 2)
                equal = versions[1].toInt() == ast->version->minorVersion;
            else
                equal = ast->version->minorVersion == 0;
        }
    }

    return equal;
}
