/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#ifndef CPPCANONICALSYMBOL_H
#define CPPCANONICALSYMBOL_H

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

#endif // CPPCANONICALSYMBOL_H
