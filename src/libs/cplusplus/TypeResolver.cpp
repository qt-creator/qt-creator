/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "TypeResolver.h"
#include "Overview.h"
#include "TypeOfExpression.h"

#include <QDebug>

static const bool debug = ! qgetenv("QTC_LOOKUPCONTEXT_DEBUG").isEmpty();

namespace CPlusPlus {

namespace {

class DeduceAutoCheck : public ASTVisitor
{
public:
    DeduceAutoCheck(const Identifier *id, TranslationUnit *tu)
        : ASTVisitor(tu), _id(id), _block(false)
    {
        accept(tu->ast());
    }

    virtual bool preVisit(AST *)
    {
        if (_block)
            return false;

        return true;
    }

    virtual bool visit(SimpleNameAST *ast)
    {
        if (ast->name
                && ast->name->identifier()
                && strcmp(ast->name->identifier()->chars(), _id->chars()) == 0) {
            _block = true;
        }

        return false;
    }

    virtual bool visit(MemberAccessAST *ast)
    {
        accept(ast->base_expression);
        return false;
    }

    const Identifier *_id;
    bool _block;
};

} // namespace anonymous

void TypeResolver::resolve(FullySpecifiedType *type, Scope **scope, LookupScope *binding)
{
    QSet<Symbol *> visited;
    _binding = binding;
    // Use a hard limit when trying to resolve typedefs. Typedefs in templates can refer to
    // each other, each time enhancing the template argument and thus making it impossible to
    // use an "alreadyResolved" container. FIXME: We might overcome this by resolving the
    // template parameters.
    unsigned maxDepth = 15;
    Overview oo;
    if (Q_UNLIKELY(debug))
        qDebug() << "- before typedef resolving we have:" << oo(*type);
    for (NamedType *namedTy = 0; maxDepth && (namedTy = getNamedType(*type)); --maxDepth) {
        QList<LookupItem> namedTypeItems = getNamedTypeItems(namedTy->name(), *scope, _binding);

        if (Q_UNLIKELY(debug))
            qDebug() << "-- we have" << namedTypeItems.size() << "candidates";

        if (!findTypedef(namedTypeItems, type, scope, visited))
            break;
    }
    if (Q_UNLIKELY(debug))
        qDebug() << "-  after typedef resolving:" << oo(*type);
}

NamedType *TypeResolver::getNamedType(FullySpecifiedType &type) const
{
    NamedType *namedTy = type->asNamedType();
    if (! namedTy) {
        if (PointerType *pointerTy = type->asPointerType())
            namedTy = pointerTy->elementType()->asNamedType();
    }
    return namedTy;
}

QList<LookupItem> TypeResolver::getNamedTypeItems(const Name *name, Scope *scope,
                                                  LookupScope *binding) const
{
    QList<LookupItem> namedTypeItems = typedefsFromScopeUpToFunctionScope(name, scope);
    if (namedTypeItems.isEmpty()) {
        if (binding)
            namedTypeItems = binding->lookup(name);
        if (LookupScope *scopeCon = _factory.lookupType(scope)) {
            if (scopeCon != binding)
                namedTypeItems += scopeCon->lookup(name);
        }
    }

    return namedTypeItems;
}

/// Return all typedefs with given name from given scope up to function scope.
QList<LookupItem> TypeResolver::typedefsFromScopeUpToFunctionScope(const Name *name, Scope *scope)
{
    QList<LookupItem> results;
    if (!scope)
        return results;
    Scope *enclosingBlockScope = 0;
    for (Block *block = scope->asBlock(); block;
         block = enclosingBlockScope ? enclosingBlockScope->asBlock() : 0) {
        const unsigned memberCount = block->memberCount();
        for (unsigned i = 0; i < memberCount; ++i) {
            Symbol *symbol = block->memberAt(i);
            if (Declaration *declaration = symbol->asDeclaration()) {
                if (isTypedefWithName(declaration, name)) {
                    LookupItem item;
                    item.setDeclaration(declaration);
                    item.setScope(block);
                    item.setType(declaration->type());
                    results.append(item);
                }
            }
        }
        enclosingBlockScope = block->enclosingScope();
        if (enclosingBlockScope) {
            // For lambda, step beyond the function to its enclosing block
            if (Function *enclosingFunction = enclosingBlockScope->asFunction()) {
                if (!enclosingFunction->name())
                    enclosingBlockScope = enclosingBlockScope->enclosingScope();
            }
        }
    }
    return results;
}

// Resolves auto and decltype initializer string
QList<LookupItem> TypeResolver::resolveDeclInitializer(
        CreateBindings &factory, const Declaration *decl,
        const QSet<const Declaration* > &declarationsBeingResolved,
        const Identifier *id)
{
    const StringLiteral *initializationString = decl->getInitializer();
    if (initializationString == 0)
        return QList<LookupItem>();

    const QByteArray &initializer =
            QByteArray::fromRawData(initializationString->chars(),
                                    initializationString->size()).trimmed();

    // Skip lambda-function initializers
    if (initializer.length() > 0 && initializer[0] == '[')
        return QList<LookupItem>();

    TypeOfExpression exprTyper;
    exprTyper.setExpandTemplates(true);
    Document::Ptr doc = factory.snapshot().document(QString::fromLocal8Bit(decl->fileName()));
    exprTyper.init(doc, factory.snapshot(), factory.sharedFromThis(), declarationsBeingResolved);

    Document::Ptr exprDoc =
            documentForExpression(exprTyper.preprocessedExpression(initializer));
    factory.addExpressionDocument(exprDoc);
    exprDoc->check();

    if (id) {
        DeduceAutoCheck deduceAuto(id, exprDoc->translationUnit());
        if (deduceAuto._block)
            return QList<LookupItem>();
    }

    return exprTyper(extractExpressionAST(exprDoc), exprDoc, decl->enclosingScope());
}

bool TypeResolver::isTypedefWithName(const Declaration *declaration, const Name *name)
{
    if (declaration->isTypedef()) {
        const Identifier *identifier = declaration->name()->identifier();
        if (name->identifier()->match(identifier))
            return true;
    }
    return false;
}

bool TypeResolver::findTypedef(const QList<LookupItem> &namedTypeItems, FullySpecifiedType *type,
                               Scope **scope, QSet<Symbol *> &visited)
{
    foreach (const LookupItem &it, namedTypeItems) {
        Symbol *declaration = it.declaration();
        if (!declaration)
            continue;
        if (Template *specialization = declaration->asTemplate())
            declaration = specialization->declaration();
        if (!declaration || (!declaration->isTypedef() && !declaration->type().isDecltype()))
            continue;
        if (visited.contains(declaration))
            break;
        visited.insert(declaration);

        // continue working with the typedefed type and scope
        if (type->type()->isPointerType()) {
            *type = FullySpecifiedType(
                        _factory.control()->pointerType(declaration->type()));
        } else if (type->type()->isReferenceType()) {
            *type = FullySpecifiedType(
                        _factory.control()->referenceType(
                            declaration->type(),
                            declaration->type()->asReferenceType()->isRvalueReference()));
        } else if (declaration->type().isDecltype()) {
            Declaration *decl = declaration->asDeclaration();
            const QList<LookupItem> resolved =
                    resolveDeclInitializer(_factory, decl, QSet<const Declaration* >() << decl);
            if (!resolved.isEmpty()) {
                LookupItem item = resolved.first();
                *type = item.type();
                *scope = item.scope();
                _binding = item.binding();
                return true;
            }
        } else {
            *type = it.type();
        }

        *scope = it.scope();
        _binding = it.binding();
        return true;
    }

    return false;
}

} // namespace CPlusPlus
