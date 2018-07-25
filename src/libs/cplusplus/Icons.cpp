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

#include "Icons.h"

#include <cplusplus/FullySpecifiedType.h>
#include <cplusplus/Scope.h>
#include <cplusplus/Symbols.h>
#include <cplusplus/Type.h>

using namespace CPlusPlus;
using CPlusPlus::Icons;

QIcon Icons::iconForSymbol(const Symbol *symbol)
{
    return iconForType(iconTypeForSymbol(symbol));
}

QIcon Icons::keywordIcon()
{
    return iconForType(Utils::CodeModelIcon::Keyword);
}

QIcon Icons::macroIcon()
{
    return iconForType(Utils::CodeModelIcon::Macro);
}

Utils::CodeModelIcon::Type Icons::iconTypeForSymbol(const Symbol *symbol)
{
    using namespace Utils::CodeModelIcon;
    if (const Template *templ = symbol->asTemplate()) {
        if (Symbol *decl = templ->declaration())
            return iconTypeForSymbol(decl);
    }

    FullySpecifiedType symbolType = symbol->type();
    if (symbol->isFunction() || (symbol->isDeclaration() && symbolType &&
                                 symbolType->isFunctionType()))
    {
        const Function *function = symbol->asFunction();
        if (!function)
            function = symbol->type()->asFunctionType();

        if (function->isSlot()) {
            if (function->isPublic())
                return SlotPublic;
            else if (function->isProtected())
                return SlotProtected;
            else if (function->isPrivate())
                return SlotPrivate;
        } else if (function->isSignal()) {
            return Signal;
        } else if (symbol->isPublic()) {
            return symbol->isStatic() ? FuncPublicStatic : FuncPublic;
        } else if (symbol->isProtected()) {
            return symbol->isStatic() ? FuncProtectedStatic : FuncProtected;
        } else if (symbol->isPrivate()) {
            return symbol->isStatic() ? FuncPrivateStatic : FuncPrivate;
        }
    } else if (symbol->enclosingScope() && symbol->enclosingScope()->isEnum()) {
        return Enumerator;
    } else if (symbol->isDeclaration() || symbol->isArgument()) {
        if (symbol->isPublic()) {
            return symbol->isStatic() ? VarPublicStatic : VarPublic;
        } else if (symbol->isProtected()) {
            return symbol->isStatic() ? VarProtectedStatic : VarProtected;
        } else if (symbol->isPrivate()) {
            return symbol->isStatic() ? VarPrivateStatic : VarPrivate;
        }
    } else if (symbol->isEnum()) {
        return Utils::CodeModelIcon::Enum;
    } else if (symbol->isForwardClassDeclaration()) {
        return Utils::CodeModelIcon::Class; // TODO: Store class key in ForwardClassDeclaration
    } else if (const Class *klass = symbol->asClass()) {
        return klass->isStruct() ? Struct : Utils::CodeModelIcon::Class;
    } else if (symbol->isObjCClass() || symbol->isObjCForwardClassDeclaration()) {
        return Utils::CodeModelIcon::Class;
    } else if (symbol->isObjCProtocol() || symbol->isObjCForwardProtocolDeclaration()) {
        return Utils::CodeModelIcon::Class;
    } else if (symbol->isObjCMethod()) {
        return FuncPublic;
    } else if (symbol->isNamespace()) {
        return Utils::CodeModelIcon::Namespace;
    } else if (symbol->isTypenameArgument()) {
        return Utils::CodeModelIcon::Class;
    } else if (symbol->isQtPropertyDeclaration() || symbol->isObjCPropertyDeclaration()) {
        return Property;
    } else if (symbol->isUsingNamespaceDirective() ||
               symbol->isUsingDeclaration()) {
        // TODO: Might be nice to have a different icons for these things
        return Utils::CodeModelIcon::Namespace;
    }

    return Unknown;
}
