/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "Icons.h"

#include <cplusplus/FullySpecifiedType.h>
#include <cplusplus/Scope.h>
#include <cplusplus/Symbols.h>
#include <cplusplus/Type.h>

using namespace CPlusPlus;
using CPlusPlus::Icons;

Icons::Icons()
    : _classIcon(QLatin1String(":/codemodel/images/class.png")),
      _enumIcon(QLatin1String(":/codemodel/images/enum.png")),
      _enumeratorIcon(QLatin1String(":/codemodel/images/enumerator.png")),
      _funcPublicIcon(QLatin1String(":/codemodel/images/func.png")),
      _funcProtectedIcon(QLatin1String(":/codemodel/images/func_prot.png")),
      _funcPrivateIcon(QLatin1String(":/codemodel/images/func_priv.png")),
      _namespaceIcon(QLatin1String(":/codemodel/images/namespace.png")),
      _varPublicIcon(QLatin1String(":/codemodel/images/var.png")),
      _varProtectedIcon(QLatin1String(":/codemodel/images/var_prot.png")),
      _varPrivateIcon(QLatin1String(":/codemodel/images/var_priv.png")),
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
            return FuncPublicIconType;
        } else if (symbol->isProtected()) {
            return FuncProtectedIconType;
        } else if (symbol->isPrivate()) {
            return FuncPrivateIconType;
        }
    } else if (symbol->enclosingScope() && symbol->enclosingScope()->isEnum()) {
        return EnumeratorIconType;
    } else if (symbol->isDeclaration() || symbol->isArgument()) {
        if (symbol->isPublic())
            return VarPublicIconType;
        else if (symbol->isProtected())
            return VarProtectedIconType;
        else if (symbol->isPrivate())
            return VarPrivateIconType;
    } else if (symbol->isEnum()) {
        return EnumIconType;
    } else if (symbol->isClass() || symbol->isForwardClassDeclaration()) {
        return ClassIconType;
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
    case NamespaceIconType:
        return _namespaceIcon;
    case VarPublicIconType:
        return _varPublicIcon;
    case VarProtectedIconType:
        return _varProtectedIcon;
    case VarPrivateIconType:
        return _varPrivateIcon;
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
