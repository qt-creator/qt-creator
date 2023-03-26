// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "Icons.h"

#include <cplusplus/FullySpecifiedType.h>
#include <cplusplus/Scope.h>
#include <cplusplus/Symbols.h>
#include <cplusplus/Type.h>

namespace CPlusPlus {
namespace Icons {

QIcon iconForSymbol(const Symbol *symbol)
{
    return iconForType(iconTypeForSymbol(symbol));
}

QIcon keywordIcon()
{
    return iconForType(Utils::CodeModelIcon::Keyword);
}

QIcon macroIcon()
{
    return iconForType(Utils::CodeModelIcon::Macro);
}

Utils::CodeModelIcon::Type iconTypeForSymbol(const Symbol *symbol)
{
    using namespace Utils::CodeModelIcon;
    if (const Template *templ = symbol->asTemplate()) {
        if (Symbol *decl = templ->declaration())
            return iconTypeForSymbol(decl);
    }

    FullySpecifiedType symbolType = symbol->type();
    if (symbol->asFunction() || (symbol->asDeclaration() && symbolType &&
                                 symbolType->asFunctionType()))
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
    } else if (symbol->enclosingScope() && symbol->enclosingScope()->asEnum()) {
        return Enumerator;
    } else if (symbol->asDeclaration() || symbol->asArgument()) {
        if (symbol->isPublic()) {
            return symbol->isStatic() ? VarPublicStatic : VarPublic;
        } else if (symbol->isProtected()) {
            return symbol->isStatic() ? VarProtectedStatic : VarProtected;
        } else if (symbol->isPrivate()) {
            return symbol->isStatic() ? VarPrivateStatic : VarPrivate;
        }
    } else if (symbol->asEnum()) {
        return Utils::CodeModelIcon::Enum;
    } else if (symbol->asForwardClassDeclaration()) {
        return Utils::CodeModelIcon::Class; // TODO: Store class key in ForwardClassDeclaration
    } else if (const Class *klass = symbol->asClass()) {
        return klass->isStruct() ? Struct : Utils::CodeModelIcon::Class;
    } else if (symbol->asObjCClass() || symbol->asObjCForwardClassDeclaration()) {
        return Utils::CodeModelIcon::Class;
    } else if (symbol->asObjCProtocol() || symbol->asObjCForwardProtocolDeclaration()) {
        return Utils::CodeModelIcon::Class;
    } else if (symbol->asObjCMethod()) {
        return FuncPublic;
    } else if (symbol->asNamespace()) {
        return Utils::CodeModelIcon::Namespace;
    } else if (symbol->asTypenameArgument()) {
        return Utils::CodeModelIcon::Class;
    } else if (symbol->asQtPropertyDeclaration() || symbol->asObjCPropertyDeclaration()) {
        return Property;
    } else if (symbol->asUsingNamespaceDirective() ||
               symbol->asUsingDeclaration()) {
        // TODO: Might be nice to have a different icons for these things
        return Utils::CodeModelIcon::Namespace;
    }

    return Unknown;
}

} // Icons
} // CPlusPlus
