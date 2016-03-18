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

#include <cplusplus/LookupContext.h>
#include <cplusplus/Symbol.h>
#include <cplusplus/TypeOfExpression.h>

QT_FORWARD_DECLARE_CLASS(QTextCursor)

namespace CppEditor {
namespace Internal {

class CanonicalSymbol
{
public:
    CanonicalSymbol(const CPlusPlus::Document::Ptr &document,
                    const CPlusPlus::Snapshot &snapshot);

    const CPlusPlus::LookupContext &context() const;

    CPlusPlus::Scope *getScopeAndExpression(const QTextCursor &cursor, QString *code);

    CPlusPlus::Symbol *operator()(const QTextCursor &cursor);
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

} // namespace Internal
} // namespace CppEditor
