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

#include <QDebug>

static const bool debug = ! qgetenv("QTC_LOOKUPCONTEXT_DEBUG").isEmpty();

namespace CPlusPlus {

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
        if (LookupScope *scopeCon = _context.lookupType(scope)) {
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
    }
    return results;
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
    bool foundTypedef = false;
    foreach (const LookupItem &it, namedTypeItems) {
        Symbol *declaration = it.declaration();
        if (!declaration || !declaration->isTypedef())
            continue;
        if (visited.contains(declaration))
            break;
        visited.insert(declaration);

        // continue working with the typedefed type and scope
        if (type->type()->isPointerType()) {
            *type = FullySpecifiedType(
                        _context.bindings()->control()->pointerType(declaration->type()));
        } else if (type->type()->isReferenceType()) {
            *type = FullySpecifiedType(
                        _context.bindings()->control()->referenceType(
                            declaration->type(),
                            declaration->type()->asReferenceType()->isRvalueReference()));
        } else {
            *type = declaration->type();
        }

        *scope = it.scope();
        _binding = it.binding();
        foundTypedef = true;
        break;
    }

    return foundTypedef;
}

} // namespace CPlusPlus
