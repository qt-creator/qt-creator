// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppcanonicalsymbol.h"

#include "cpptoolsreuse.h"

#include <cplusplus/ExpressionUnderCursor.h>

#include <utils/textutils.h>

#include <QTextCursor>
#include <QTextDocument>

using namespace CPlusPlus;

namespace CppEditor::Internal {

CanonicalSymbol::CanonicalSymbol(const Document::Ptr &document,
                                 const Snapshot &snapshot)
    : m_document(document),
      m_snapshot(snapshot)
{
    m_typeOfExpression.init(document, snapshot);
    m_typeOfExpression.setExpandTemplates(true);
}

const LookupContext &CanonicalSymbol::context() const
{
    return m_typeOfExpression.context();
}

Scope *CanonicalSymbol::getScopeAndExpression(const QTextCursor &cursor, QString *code)
{
    if (!m_document)
        return nullptr;

    QTextCursor tc = cursor;
    int line, column;
    Utils::Text::convertPosition(cursor.document(), tc.position(), &line, &column);

    int pos = tc.position();
    QTextDocument *textDocument = cursor.document();
    if (!isValidIdentifierChar(textDocument->characterAt(pos)))
        if (!(pos > 0 && isValidIdentifierChar(textDocument->characterAt(pos - 1))))
            return nullptr;

    while (isValidIdentifierChar(textDocument->characterAt(pos)))
        ++pos;
    tc.setPosition(pos);

    ExpressionUnderCursor expressionUnderCursor(m_document->languageFeatures());
    *code = expressionUnderCursor(tc);
    return m_document->scopeAt(line, column);
}

Symbol *CanonicalSymbol::operator()(const QTextCursor &cursor) &
{
    QString code;

    if (Scope *scope = getScopeAndExpression(cursor, &code))
        return operator()(scope, code);

    return nullptr;
}

Symbol *CanonicalSymbol::operator()(Scope *scope, const QString &code)
{
    return canonicalSymbol(scope, code, m_typeOfExpression);
}

Symbol *CanonicalSymbol::canonicalSymbol(Scope *scope, const QString &code,
                                         TypeOfExpression &typeOfExpression)
{
    const QList<LookupItem> results =
            typeOfExpression(code.toUtf8(), scope, TypeOfExpression::Preprocess);

    for (int i = results.size() - 1; i != -1; --i) {
        const LookupItem &r = results.at(i);
        Symbol *decl = r.declaration();

        if (!(decl && decl->enclosingScope()))
            break;

        if (Class *classScope = r.declaration()->enclosingScope()->asClass()) {
            const Identifier *declId = decl->identifier();
            const Identifier *classId = classScope->identifier();

            if (classId && classId->match(declId))
                continue; // skip it, it's a ctor or a dtor.

            if (Function *funTy = r.declaration()->type()->asFunctionType()) {
                if (funTy->isVirtual())
                    return r.declaration();
            }
        }
    }

    for (const auto &r : results) {
         if (r.declaration())
            return r.declaration();
    }

    return nullptr;
}

} // namespace CppEditor::Internal
