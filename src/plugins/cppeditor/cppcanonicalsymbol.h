// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <cplusplus/LookupContext.h>
#include <cplusplus/Symbol.h>
#include <cplusplus/TypeOfExpression.h>

QT_FORWARD_DECLARE_CLASS(QTextCursor)

namespace CppEditor::Internal {

class CanonicalSymbol
{
public:
    CanonicalSymbol(const CPlusPlus::Document::Ptr &document,
                    const CPlusPlus::Snapshot &snapshot);

    const CPlusPlus::LookupContext &context() const;

    CPlusPlus::Scope *getScopeAndExpression(const QTextCursor &cursor, QString *code);

    CPlusPlus::Symbol *operator()(const QTextCursor &cursor) &;
    CPlusPlus::Symbol *operator()(CPlusPlus::Scope *scope, const QString &code);

public:
    static CPlusPlus::Symbol *canonicalSymbol(CPlusPlus::Scope *scope,
                                              const QString &code,
                                              CPlusPlus::TypeOfExpression &typeOfExpression);

private:
    CPlusPlus::Document::Ptr m_document;
    CPlusPlus::Snapshot m_snapshot;
    CPlusPlus::TypeOfExpression m_typeOfExpression;
};

} // namespace CppEditor::Internal
