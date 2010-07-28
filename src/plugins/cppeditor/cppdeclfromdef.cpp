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

#include "cppdeclfromdef.h"

#include <Literals.h> //### remove
#include <QDebug> //###remove

#include <AST.h>
#include <ASTVisitor.h>
#include <CoreTypes.h>
#include <Names.h>
#include <Symbols.h>
#include <TranslationUnit.h>
#include <cplusplus/ASTPath.h>
#include <cplusplus/InsertionPointLocator.h>
#include <cplusplus/LookupContext.h>
#include <cplusplus/Overview.h>

#include <QtCore/QCoreApplication>

using namespace CPlusPlus;
using namespace CppEditor;
using namespace CppEditor::Internal;
using namespace CppTools;

using CppEditor::CppRefactoringChanges;

namespace {

//! \todo This method should probably be changed when Overview#prettyTypeWithName is removed.
QString prettyMinimalType(const FullySpecifiedType &ty,
                          const LookupContext &context,
                          Scope *source,
                          ClassOrNamespace *target)
{
    Overview oo;

    if (const NamedType *namedTy = ty->asNamedType())
        return oo.prettyTypeWithName(ty, context.minimalName(namedTy->name(),
                                                             source,
                                                             target),
                                     context.control().data());
    else
        return oo(ty);
}

class Operation: public CppQuickFixOperation
{
public:
    Operation(const CppQuickFixState &state, int priority,
              const QString &targetFileName, const Class *targetSymbol,
              const QString &decl)
        : CppQuickFixOperation(state, priority)
        , m_targetFileName(targetFileName)
        , m_targetSymbol(targetSymbol)
        , m_decl(decl)
    {
        setDescription(QCoreApplication::tr("Create Declaration from Definition",
                                            "CppEditor::DeclFromDef"));
    }

    void createChanges()
    {
        CppRefactoringChanges *changes = refactoringChanges();

        Document::Ptr targetDoc = changes->document(m_targetFileName);
        InsertionPointLocator locator(targetDoc);
        const InsertionLocation loc = locator.methodDeclarationInClass(m_targetSymbol, InsertionPointLocator::Public);

        int targetPosition1 = changes->positionInFile(m_targetFileName, loc.line() - 1, loc.column() - 1);
        int targetPosition2 = changes->positionInFile(m_targetFileName, loc.line(), 0) - 1;

        Utils::ChangeSet target;
        target.insert(targetPosition1, loc.prefix() + m_decl);
        changes->changeFile(m_targetFileName, target);

        changes->reindent(m_targetFileName,
                          Utils::ChangeSet::Range(targetPosition1, targetPosition2));

        changes->openEditor(m_targetFileName, loc.line() - 1, loc.column() - 1);
    }

private:
    QString m_targetFileName;
    const Class *m_targetSymbol;
    QString m_decl;
};

} // anonymous namespace

QList<CppQuickFixOperation::Ptr> DeclFromDef::match(const CppQuickFixState &state)
{
    const QList<AST *> &path = state.path();

    FunctionDefinitionAST *funDef = 0;
    int idx = 0;
    for (; idx < path.size(); ++idx) {
        AST *node = path.at(idx);
        if (FunctionDefinitionAST *candidate = node->asFunctionDefinition()) {
            if (!funDef && state.isCursorOn(candidate) && !state.isCursorOn(candidate->function_body))
                funDef = candidate;
        } else if (node->asClassSpecifier()) {
            return noResult();
        }
    }

    if (!funDef || !funDef->symbol)
        return noResult();

    Function *method = funDef->symbol;

    if (ClassOrNamespace *targetBinding = state.context().lookupParent(method)) {
        foreach (Symbol *s, targetBinding->symbols()) {
            if (Class *clazz = s->asClass()) {
                return singleResult(new Operation(state, idx,
                                                  QLatin1String(clazz->fileName()),
                                                  clazz,
                                                  generateDeclaration(state,
                                                                      method,
                                                                      targetBinding)));
            } //! \todo support insertion into namespaces
        }
    }

    return noResult();
}

QString DeclFromDef::generateDeclaration(const CppQuickFixState &state,
                                         Function *method,
                                         ClassOrNamespace *targetBinding)
{
    Overview oo;
    QString decl;

    decl.append(prettyMinimalType(method->returnType(),
                                  state.context(),
                                  method->scope(),
                                  targetBinding));

    decl.append(QLatin1Char(' '));
    decl.append(QLatin1String(method->name()->identifier()->chars()));
    decl.append(QLatin1Char('('));
    for (unsigned argIdx = 0; argIdx < method->argumentCount(); ++argIdx) {
        if (argIdx > 0)
            decl.append(QLatin1String(", "));
        Argument *arg = method->argumentAt(argIdx)->asArgument();
        decl.append(prettyMinimalType(arg->type(),
                                      state.context(),
                                      method->members(),
                                      targetBinding));
        decl.append(QLatin1Char(' '));
        decl.append(oo(arg->name()));
    }
    decl.append(QLatin1String(");\n\n"));

    return decl;
}
