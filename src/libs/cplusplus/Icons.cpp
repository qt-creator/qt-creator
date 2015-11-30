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

#include "Icons.h"

#include <cplusplus/FullySpecifiedType.h>
#include <cplusplus/Scope.h>
#include <cplusplus/Symbols.h>
#include <cplusplus/Type.h>

using namespace CPlusPlus;
using CPlusPlus::Icons;

Icons::Icons()
    : _classIcon(QLatin1String(":/codemodel/images/class.png")),
      _structIcon(QLatin1String(":/codemodel/images/struct.png")),
      _enumIcon(QLatin1String(":/codemodel/images/enum.png")),
      _enumeratorIcon(QLatin1String(":/codemodel/images/enumerator.png")),
      _funcPublicIcon(QLatin1String(":/codemodel/images/func.png")),
      _funcProtectedIcon(QLatin1String(":/codemodel/images/func_prot.png")),
      _funcPrivateIcon(QLatin1String(":/codemodel/images/func_priv.png")),
      _funcPublicStaticIcon(QLatin1String(":/codemodel/images/func_st.png")),
      _funcProtectedStaticIcon(QLatin1String(":/codemodel/images/func_prot_st.png")),
      _funcPrivateStaticIcon(QLatin1String(":/codemodel/images/func_priv_st.png")),
      _namespaceIcon(QLatin1String(":/codemodel/images/namespace.png")),
      _varPublicIcon(QLatin1String(":/codemodel/images/var.png")),
      _varProtectedIcon(QLatin1String(":/codemodel/images/var_prot.png")),
      _varPrivateIcon(QLatin1String(":/codemodel/images/var_priv.png")),
      _varPublicStaticIcon(QLatin1String(":/codemodel/images/var_st.png")),
      _varProtectedStaticIcon(QLatin1String(":/codemodel/images/var_prot_st.png")),
      _varPrivateStaticIcon(QLatin1String(":/codemodel/images/var_priv_st.png")),
      _signalIcon(QLatin1String(":/codemodel/images/signal.png")),
      _slotPublicIcon(QLatin1String(":/codemodel/images/slot.png")),
      _slotProtectedIcon(QLatin1String(":/codemodel/images/slot_prot.png")),
      _slotPrivateIcon(QLatin1String(":/codemodel/images/slot_priv.png")),
      _keywordIcon(QLatin1String(":/codemodel/images/keyword.png")),
      _macroIcon(QLatin1String(":/codemodel/images/macro.png"))
{
}

QIcon Icons::iconForSymbol(const Symbol *symbol) const
{
    return iconForType(iconTypeForSymbol(symbol));
}

QIcon Icons::keywordIcon() const
{
    return _keywordIcon;
}

QIcon Icons::macroIcon() const
{
    return _macroIcon;
}

Icons::IconType Icons::iconTypeForSymbol(const Symbol *symbol)
{
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
                return SlotPublicIconType;
            else if (function->isProtected())
                return SlotProtectedIconType;
            else if (function->isPrivate())
                return SlotPrivateIconType;
        } else if (function->isSignal()) {
            return SignalIconType;
        } else if (symbol->isPublic()) {
            return symbol->isStatic() ? FuncPublicStaticIconType : FuncPublicIconType;
        } else if (symbol->isProtected()) {
            return symbol->isStatic() ? FuncProtectedStaticIconType : FuncProtectedIconType;
        } else if (symbol->isPrivate()) {
            return symbol->isStatic() ? FuncPrivateStaticIconType : FuncPrivateIconType;
        }
    } else if (symbol->enclosingScope() && symbol->enclosingScope()->isEnum()) {
        return EnumeratorIconType;
    } else if (symbol->isDeclaration() || symbol->isArgument()) {
        if (symbol->isPublic()) {
            return symbol->isStatic() ? VarPublicStaticIconType : VarPublicIconType;
        } else if (symbol->isProtected()) {
            return symbol->isStatic() ? VarProtectedStaticIconType : VarProtectedIconType;
        } else if (symbol->isPrivate()) {
            return symbol->isStatic() ? VarPrivateStaticIconType : VarPrivateIconType;
        }
    } else if (symbol->isEnum()) {
        return EnumIconType;
    } else if (symbol->isForwardClassDeclaration()) {
        return ClassIconType; // TODO: Store class key in ForwardClassDeclaration
    } else if (const Class *klass = symbol->asClass()) {
        return klass->isStruct() ? StructIconType : ClassIconType;
    } else if (symbol->isObjCClass() || symbol->isObjCForwardClassDeclaration()) {
        return ClassIconType;
    } else if (symbol->isObjCProtocol() || symbol->isObjCForwardProtocolDeclaration()) {
        return ClassIconType;
    } else if (symbol->isObjCMethod()) {
        return FuncPublicIconType;
    } else if (symbol->isNamespace()) {
        return NamespaceIconType;
    } else if (symbol->isTypenameArgument()) {
        return ClassIconType;
    } else if (symbol->isUsingNamespaceDirective() ||
               symbol->isUsingDeclaration()) {
        // TODO: Might be nice to have a different icons for these things
        return NamespaceIconType;
    }

    return UnknownIconType;
}

QIcon Icons::iconForType(IconType type) const
{
    switch (type) {
    case ClassIconType:
        return _classIcon;
    case StructIconType:
        return _structIcon;
    case EnumIconType:
        return _enumIcon;
    case EnumeratorIconType:
        return _enumeratorIcon;
    case FuncPublicIconType:
        return _funcPublicIcon;
    case FuncProtectedIconType:
        return _funcProtectedIcon;
    case FuncPrivateIconType:
        return _funcPrivateIcon;
    case FuncPublicStaticIconType:
        return _funcPublicStaticIcon;
    case FuncProtectedStaticIconType:
        return _funcProtectedStaticIcon;
    case FuncPrivateStaticIconType:
        return _funcPrivateStaticIcon;
    case NamespaceIconType:
        return _namespaceIcon;
    case VarPublicIconType:
        return _varPublicIcon;
    case VarProtectedIconType:
        return _varProtectedIcon;
    case VarPrivateIconType:
        return _varPrivateIcon;
    case VarPublicStaticIconType:
        return _varPublicStaticIcon;
    case VarProtectedStaticIconType:
        return _varProtectedStaticIcon;
    case VarPrivateStaticIconType:
        return _varPrivateStaticIcon;
    case SignalIconType:
        return _signalIcon;
    case SlotPublicIconType:
        return _slotPublicIcon;
    case SlotProtectedIconType:
        return _slotProtectedIcon;
    case SlotPrivateIconType:
        return _slotPrivateIcon;
    case KeywordIconType:
        return _keywordIcon;
    case MacroIconType:
        return _macroIcon;
    default:
        break;
    }
    return QIcon();
}
