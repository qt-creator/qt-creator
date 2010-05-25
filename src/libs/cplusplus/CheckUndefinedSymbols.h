/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef CHECKUNDEFINEDSYMBOLS_H
#define CHECKUNDEFINEDSYMBOLS_H

#include "CppDocument.h"
#include "LookupContext.h"
#include <ASTVisitor.h>

namespace CPlusPlus {

class CPLUSPLUS_EXPORT CheckUndefinedSymbols: protected ASTVisitor
{
public:
    CheckUndefinedSymbols(TranslationUnit *unit, const LookupContext &context);
    virtual ~CheckUndefinedSymbols();

    QList<Document::DiagnosticMessage> operator()(AST *ast);

protected:
    using ASTVisitor::visit;

    bool warning(unsigned line, unsigned column, const QString &text, unsigned length = 0);
    bool warning(AST *ast, const QString &text);

    void checkNamespace(NameAST *name);

    virtual bool visit(UsingDirectiveAST *);
    virtual bool visit(SimpleDeclarationAST *);
    virtual bool visit(NamedTypeSpecifierAST *);

private:
    LookupContext _context;
    QString _fileName;
    QList<Document::DiagnosticMessage> _diagnosticMessages;
};

} // end of namespace CPlusPlus

#endif // CHECKUNDEFINEDSYMBOLS_H
