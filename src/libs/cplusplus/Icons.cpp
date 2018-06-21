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

#include <utils/icon.h>

using namespace CPlusPlus;
using CPlusPlus::Icons;

QIcon Icons::iconForSymbol(const Symbol *symbol)
{
    return iconForType(iconTypeForSymbol(symbol));
}

QIcon Icons::keywordIcon()
{
    return iconForType(KeywordIconType);
}

QIcon Icons::macroIcon()
{
    return iconForType(MacroIconType);
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
    } else if (symbol->isQtPropertyDeclaration() || symbol->isObjCPropertyDeclaration()) {
        return PropertyIconType;
    } else if (symbol->isUsingNamespaceDirective() ||
               symbol->isUsingDeclaration()) {
        // TODO: Might be nice to have a different icons for these things
        return NamespaceIconType;
    }

    return UnknownIconType;
}

QIcon Icons::iconForType(IconType type)
{
    using namespace Utils;

    static const IconMaskAndColor classRelationIcon {
        QLatin1String(":/codemodel/images/classrelation.png"), Theme::IconsCodeModelOverlayForegroundColor};
    static const IconMaskAndColor classRelationBackgroundIcon {
        QLatin1String(":/codemodel/images/classrelationbackground.png"), Theme::IconsCodeModelOverlayBackgroundColor};
    static const IconMaskAndColor classMemberFunctionIcon {
        QLatin1String(":/codemodel/images/classmemberfunction.png"), Theme::IconsCodeModelFunctionColor};
    static const IconMaskAndColor classMemberVariableIcon {
        QLatin1String(":/codemodel/images/classmembervariable.png"), Theme::IconsCodeModelVariableColor};
    static const IconMaskAndColor functionIcon {
        QLatin1String(":/codemodel/images/member.png"), Theme::IconsCodeModelFunctionColor};
    static const IconMaskAndColor variableIcon {
        QLatin1String(":/codemodel/images/member.png"), Theme::IconsCodeModelVariableColor};
    static const IconMaskAndColor signalIcon {
        QLatin1String(":/codemodel/images/signal.png"), Theme::IconsCodeModelFunctionColor};
    static const IconMaskAndColor slotIcon {
        QLatin1String(":/codemodel/images/slot.png"), Theme::IconsCodeModelFunctionColor};
    static const IconMaskAndColor propertyIcon {
        QLatin1String(":/codemodel/images/property.png"), Theme::IconsCodeModelOverlayForegroundColor};
    static const IconMaskAndColor propertyBackgroundIcon {
        QLatin1String(":/codemodel/images/propertybackground.png"), Theme::IconsCodeModelOverlayBackgroundColor};
    static const IconMaskAndColor protectedIcon {
        QLatin1String(":/codemodel/images/protected.png"), Theme::IconsCodeModelOverlayForegroundColor};
    static const IconMaskAndColor protectedBackgroundIcon {
        QLatin1String(":/codemodel/images/protectedbackground.png"), Theme::IconsCodeModelOverlayBackgroundColor};
    static const IconMaskAndColor privateIcon {
        QLatin1String(":/codemodel/images/private.png"), Theme::IconsCodeModelOverlayForegroundColor};
    static const IconMaskAndColor privateBackgroundIcon {
        QLatin1String(":/codemodel/images/privatebackground.png"), Theme::IconsCodeModelOverlayBackgroundColor};
    static const IconMaskAndColor staticIcon {
        QLatin1String(":/codemodel/images/static.png"), Theme::IconsCodeModelOverlayForegroundColor};
    static const IconMaskAndColor staticBackgroundIcon {
        QLatin1String(":/codemodel/images/staticbackground.png"), Theme::IconsCodeModelOverlayBackgroundColor};

    switch (type) {
    case ClassIconType: {
        const static QIcon icon(Icon({
            classRelationBackgroundIcon, classRelationIcon,
            {QLatin1String(":/codemodel/images/classparent.png"), Theme::IconsCodeModelClassColor},
            classMemberFunctionIcon, classMemberVariableIcon
        }, Icon::Tint).icon());
        return icon;
    }
    case StructIconType: {
        const static QIcon icon(Icon({
            classRelationBackgroundIcon, classRelationIcon,
            {QLatin1String(":/codemodel/images/classparent.png"), Theme::IconsCodeModelStructColor},
            classMemberFunctionIcon, classMemberVariableIcon
        }, Icon::Tint).icon());
        return icon;
    }
    case EnumIconType: {
        const static QIcon icon(Icon({
            {QLatin1String(":/codemodel/images/enum.png"), Theme::IconsCodeModelEnumColor}
        }, Icon::Tint).icon());
        return icon;
    }
    case EnumeratorIconType: {
        const static QIcon icon(Icon({
            {QLatin1String(":/codemodel/images/enumerator.png"), Theme::IconsCodeModelEnumColor}
        }, Icon::Tint).icon());
        return icon;
    }
    case FuncPublicIconType: {
        const static QIcon icon(Icon({
                functionIcon}, Icon::Tint).icon());
        return icon;
    }
    case FuncProtectedIconType: {
        const static QIcon icon(Icon({
                functionIcon, protectedBackgroundIcon, protectedIcon
        }, Icon::Tint).icon());
        return icon;
    }
    case FuncPrivateIconType: {
        const static QIcon icon(Icon({
            functionIcon, privateBackgroundIcon, privateIcon
        }, Icon::Tint).icon());
        return icon;
    }
    case FuncPublicStaticIconType: {
        const static QIcon icon(Icon({
            functionIcon, staticBackgroundIcon, staticIcon
        }, Icon::Tint).icon());
        return icon;
    }
    case FuncProtectedStaticIconType: {
        const static QIcon icon(Icon({
            functionIcon, staticBackgroundIcon, staticIcon, protectedBackgroundIcon, protectedIcon
        }, Icon::Tint).icon());
        return icon;
    }
    case FuncPrivateStaticIconType: {
        const static QIcon icon(Icon({
            functionIcon, staticBackgroundIcon, staticIcon, privateBackgroundIcon, privateIcon
        }, Icon::Tint).icon());
        return icon;
    }
    case NamespaceIconType: {
        const static QIcon icon(Icon({
            {QLatin1String(":/utils/images/namespace.png"), Theme::IconsCodeModelKeywordColor}
        }, Icon::Tint).icon());
        return icon;
    }
    case VarPublicIconType: {
        const static QIcon icon(Icon({
            variableIcon
        }, Icon::Tint).icon());
        return icon;
    }
    case VarProtectedIconType: {
        const static QIcon icon(Icon({
            variableIcon, protectedBackgroundIcon, protectedIcon
        }, Icon::Tint).icon());
        return icon;
    }
    case VarPrivateIconType: {
        const static QIcon icon(Icon({
            variableIcon, privateBackgroundIcon, privateIcon
        }, Icon::Tint).icon());
        return icon;
    }
    case VarPublicStaticIconType: {
        const static QIcon icon(Icon({
            variableIcon, staticBackgroundIcon, staticIcon
        }, Icon::Tint).icon());
        return icon;
    }
    case VarProtectedStaticIconType: {
        const static QIcon icon(Icon({
            variableIcon, staticBackgroundIcon, staticIcon, protectedBackgroundIcon, protectedIcon
        }, Icon::Tint).icon());
        return icon;
    }
    case VarPrivateStaticIconType: {
        const static QIcon icon(Icon({
            variableIcon, staticBackgroundIcon, staticIcon, privateBackgroundIcon, privateIcon
        }, Icon::Tint).icon());
        return icon;
    }
    case SignalIconType: {
        const static QIcon icon(Icon({
            signalIcon
        }, Icon::Tint).icon());
        return icon;
    }
    case SlotPublicIconType: {
        const static QIcon icon(Icon({
            slotIcon
        }, Icon::Tint).icon());
        return icon;
    }
    case SlotProtectedIconType: {
        const static QIcon icon(Icon({
            slotIcon, protectedBackgroundIcon, protectedIcon
        }, Icon::Tint).icon());
        return icon;
    }
    case SlotPrivateIconType: {
        const static QIcon icon(Icon({
            slotIcon, privateBackgroundIcon, privateIcon
        }, Icon::Tint).icon());
        return icon;
    }
    case KeywordIconType: {
        const static QIcon icon(Icon({
            {QLatin1String(":/codemodel/images/keyword.png"), Theme::IconsCodeModelKeywordColor}
        }, Icon::Tint).icon());
        return icon;
    }
    case MacroIconType: {
        const static QIcon icon(Icon({
            {QLatin1String(":/codemodel/images/macro.png"), Theme::IconsCodeModelMacroColor}
        }, Icon::Tint).icon());
        return icon;
    }
    case PropertyIconType: {
        const static QIcon icon(Icon({
            variableIcon, propertyBackgroundIcon, propertyIcon
        }, Icon::Tint).icon());
        return icon;
    }
    default:
        break;
    }
    return QIcon();
}
