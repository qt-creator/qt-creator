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
#include <cplusplus/LookupContext.h>
#include <cplusplus/Overview.h>

#include <QtCore/QCoreApplication>

using namespace CPlusPlus;
using namespace CppEditor::Internal;
using namespace CppTools;

using CppEditor::CppRefactoringChanges;

namespace {

class InsertionPointFinder: public ASTVisitor
{
public:
    InsertionPointFinder(Document::Ptr doc, const QString &className)
        : ASTVisitor(doc->translationUnit())
        , _doc(doc)
        , _className(className)
    {}

    void operator()(int *line, int *column)
    {
        if (!line && !column)
            return;
        _line = 0;
        _column = 0;

        AST *ast = translationUnit()->ast();
        accept(ast);

        if (line)
            *line = _line - 1;
        if (column)
            *column = _column - 1;
    }

protected:
    using ASTVisitor::visit;

    bool visit(ClassSpecifierAST *ast)
    {
        if (!ast->symbol || _className != QLatin1String(ast->symbol->identifier()->chars()))
            return true;

        unsigned currentVisibility = (tokenKind(ast->classkey_token) == T_CLASS) ? T_PUBLIC : T_PRIVATE;
        unsigned insertBefore = 0;

        for (DeclarationListAST *iter = ast->member_specifier_list; iter; iter = iter->next) {
            DeclarationAST *decl = iter->value;
            if (AccessDeclarationAST *xsDecl = decl->asAccessDeclaration()) {
                const unsigned token = xsDecl->access_specifier_token;
                const int kind = tokenKind(token);
                if (kind == T_PUBLIC) {
                    currentVisibility = T_PUBLIC;
                } else if (kind == T_PROTECTED) {
                    if (currentVisibility == T_PUBLIC) {
                        insertBefore = token;
                        break;
                    } else {
                        currentVisibility = T_PROTECTED;
                    }
                } else if (kind == T_PRIVATE) {
                    if (currentVisibility == T_PUBLIC
                            || currentVisibility == T_PROTECTED) {
                        insertBefore = token;
                        break;
                    } else {
                        currentVisibility = T_PRIVATE;
                    }
                }
            }
        }

        if (!insertBefore)
            insertBefore = ast->rbrace_token;

        getTokenStartPosition(insertBefore, &_line, &_column);

        return false;
    }

private:
    Document::Ptr _doc;
    QString _className;

    unsigned _line;
    unsigned _column;
};

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

} // anonymous namespace

DeclFromDef::DeclFromDef(TextEditor::BaseTextEditor *editor)
    : CppQuickFixOperation(editor)
{}

QString DeclFromDef::description() const
{
    return QCoreApplication::tr("Create Declaration from Definition", "DeclFromDef");
}

int DeclFromDef::match(const QList<CPlusPlus::AST *> &path)
{
    m_targetFileName.clear();
    m_targetSymbolName.clear();
    m_decl.clear();

    FunctionDefinitionAST *funDef = 0;
    int idx = 0;
    for (; idx < path.size(); ++idx) {
        AST *node = path.at(idx);
        if (FunctionDefinitionAST *candidate = node->asFunctionDefinition())
            if (!funDef)
                funDef = candidate;
        else if (node->asClassSpecifier())
            return -1;
    }

    if (!funDef || !funDef->symbol)
        return -1;

    Function *method = funDef->symbol;
    LookupContext context(document(), snapshot());

    if (ClassOrNamespace *targetBinding = context.lookupParent(method)) {
        foreach (Symbol *s, targetBinding->symbols()) {
            if (Class *clazz = s->asClass()) {
                m_targetFileName = QLatin1String(clazz->fileName());
                m_targetSymbolName = QLatin1String(clazz->identifier()->chars());

                m_decl = generateDeclaration(method, targetBinding);

                return idx;
            } // ### TODO: support insertion into namespaces
        }
    }

    return -1;
}

void DeclFromDef::createChanges()
{
    CppRefactoringChanges *changes = refactoringChanges();

    Document::Ptr targetDoc = changes->document(m_targetFileName);
    InsertionPointFinder findInsertionPoint(targetDoc, m_targetSymbolName);
    int line = 0, column = 0;
    findInsertionPoint(&line, &column);

    int targetPosition1 = changes->positionInFile(m_targetFileName, line, column);
    int targetPosition2 = changes->positionInFile(m_targetFileName, line + 1, 0) - 1;

    Utils::ChangeSet target;
    target.insert(targetPosition1, m_decl);
    changes->changeFile(m_targetFileName, target);

    changes->reindent(m_targetFileName,
                      Utils::ChangeSet::Range(targetPosition1, targetPosition2));

    changes->openEditor(m_targetFileName, line, column);
}

QString DeclFromDef::generateDeclaration(Function *method, ClassOrNamespace *targetBinding)
{
    LookupContext context(document(), snapshot());
    Overview oo;
    QString decl;

    decl.append(prettyMinimalType(method->returnType(),
                                  context,
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
                                      context,
                                      method->members(),
                                      targetBinding));
        decl.append(QLatin1Char(' '));
        decl.append(oo(arg->name()));
    }
    decl.append(QLatin1String(");\n\n"));

    return decl;
}
