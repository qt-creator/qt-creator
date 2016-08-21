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

#include "TypePrettyPrinter.h"

#include "Overview.h"

#include <cplusplus/FullySpecifiedType.h>
#include <cplusplus/Literals.h>
#include <cplusplus/CoreTypes.h>
#include <cplusplus/Symbols.h>
#include <cplusplus/Scope.h>

#include <QStringList>
#include <QDebug>

using namespace CPlusPlus;

/*!
   \class TypePrettyPrinter

   \brief The TypePrettyPrinter class is a helper class for the Overview class.
    This class does the main type conversion work.

    Do not use this class directly, use Overview instead.
 */

TypePrettyPrinter::TypePrettyPrinter(const Overview *overview)
    : _overview(overview)
    , _needsParens(false)
    , _isIndirectionType(false)
    , _isIndirectionToArrayOrFunction(false)
{ }

TypePrettyPrinter::~TypePrettyPrinter()
{ }

const Overview *TypePrettyPrinter::overview() const
{ return _overview; }

QString TypePrettyPrinter::operator()(const FullySpecifiedType &ty)
{
    QString previousName = switchText();
    bool previousNeedsParens = switchNeedsParens(false);
    acceptType(ty);
    switchNeedsParens(previousNeedsParens);
    return switchText(previousName);
}

QString TypePrettyPrinter::operator()(const FullySpecifiedType &type, const QString &name)
{
    const QString previousName = switchName(name);
    QString text = operator()(type);
    if (! _name.isEmpty() && ! text.isEmpty()) {
        const QChar ch = text.at(text.size() - 1);
        if (ch.isLetterOrNumber() || ch == QLatin1Char('_') || ch == QLatin1Char('>'))
            text += QLatin1Char(' ');
        text += _name;
    } else if (text.isEmpty()) {
        text = name;
    }
    (void) switchName(previousName);
    return text;
}


void TypePrettyPrinter::acceptType(const FullySpecifiedType &ty)
{
    const FullySpecifiedType previousFullySpecifiedType = _fullySpecifiedType;
    _fullySpecifiedType = ty;
    accept(ty.type());
    _fullySpecifiedType = previousFullySpecifiedType;
}

QString TypePrettyPrinter::switchName(const QString &name)
{
    const QString previousName = _name;
    _name = name;
    return previousName;

}

bool TypePrettyPrinter::switchIsIndirectionType(bool isIndirectionType)
{
    bool previousIsIndirectionType = _isIndirectionType;
    _isIndirectionType = isIndirectionType;
    return previousIsIndirectionType;
}

bool TypePrettyPrinter::switchIsIndirectionToArrayOrFunction(bool isIndirectionToArrayOrFunction)
{
    bool previousIsIndirectionToArrayOrFunction = _isIndirectionToArrayOrFunction;
    _isIndirectionToArrayOrFunction = isIndirectionToArrayOrFunction;
    return previousIsIndirectionToArrayOrFunction;
}

QString TypePrettyPrinter::switchText(const QString &text)
{
    QString previousText = _text;
    _text = text;
    return previousText;
}

bool TypePrettyPrinter::switchNeedsParens(bool needsParens)
{
    bool previousNeedsParens = _needsParens;
    _needsParens = needsParens;
    return previousNeedsParens;
}

void TypePrettyPrinter::visit(UndefinedType *)
{
    if (_fullySpecifiedType.isSigned() || _fullySpecifiedType.isUnsigned()) {
        prependSpaceUnlessBracket();
        if (_fullySpecifiedType.isSigned())
            _text.prepend(QLatin1String("signed"));
        else if (_fullySpecifiedType.isUnsigned())
            _text.prepend(QLatin1String("unsigned"));
    }

    prependCv(_fullySpecifiedType);
}

void TypePrettyPrinter::visit(VoidType *)
{
    prependSpaceUnlessBracket();
    _text.prepend(QLatin1String("void"));
    prependCv(_fullySpecifiedType);
}

void TypePrettyPrinter::visit(NamedType *type)
{
    prependSpaceUnlessBracket();
    _text.prepend(overview()->prettyName(type->name()));
    prependCv(_fullySpecifiedType);
}

void TypePrettyPrinter::visit(Namespace *type)
{
    _text.prepend(overview()->prettyName(type->name()));
    prependCv(_fullySpecifiedType);
}

void TypePrettyPrinter::visit(Template *type)
{
    if (Symbol *d = type->declaration()) {
        const Overview &oo = *overview();
        if (oo.showTemplateParameters && ! _name.isEmpty()) {
            _name += QLatin1Char('<');
            for (unsigned index = 0; index < type->templateParameterCount(); ++index) {
                if (index)
                    _name += QLatin1String(", ");
                QString arg = oo.prettyName(type->templateParameterAt(index)->name());
                if (arg.isEmpty()) {
                    arg += QLatin1Char('T');
                    arg += QString::number(index + 1);
                }
                _name += arg;
            }
            _name += QLatin1Char('>');
        }
        acceptType(d->type());
    }
    prependCv(_fullySpecifiedType);
}

void TypePrettyPrinter::visit(Class *classTy)
{
    _text.prepend(overview()->prettyName(classTy->name()));
    prependCv(_fullySpecifiedType);
}


void TypePrettyPrinter::visit(Enum *type)
{
    _text.prepend(overview()->prettyName(type->name()));
    prependCv(_fullySpecifiedType);
}

void TypePrettyPrinter::visitIndirectionType(
    const TypePrettyPrinter::IndirectionType indirectionType,
    const FullySpecifiedType &elementType,
    bool isIndirectionToArrayOrFunction)
{
    QLatin1Char indirectionSign = indirectionType == aPointerType
        ? QLatin1Char('*') : QLatin1Char('&');

    const bool prevIsIndirectionType = switchIsIndirectionType(true);
    const bool hasName = ! _name.isEmpty();
    if (hasName) {
        _text.prepend(_name);
        _name.clear();
    }
    prependCv(_fullySpecifiedType);

    if (_text.startsWith(QLatin1Char('&')) && indirectionType != aPointerType)
        _text.prepend(QLatin1Char(' '));

    const bool prevIsIndirectionToArrayOrFunction
        = switchIsIndirectionToArrayOrFunction(isIndirectionToArrayOrFunction);

    // Space after indirectionSign?
    prependSpaceAfterIndirection(hasName);

    // Write indirectionSign or reference
    if (indirectionType == aRvalueReferenceType)
        _text.prepend(QLatin1String("&&"));
    else
        _text.prepend(indirectionSign);

    // Space before indirectionSign?
    prependSpaceBeforeIndirection(elementType);

    _needsParens = true;
    acceptType(elementType);
    switchIsIndirectionToArrayOrFunction(prevIsIndirectionToArrayOrFunction);
    switchIsIndirectionType(prevIsIndirectionType);
}

void TypePrettyPrinter::visit(IntegerType *type)
{
    prependSpaceUnlessBracket();
    switch (type->kind()) {
    case IntegerType::Char:
        _text.prepend(QLatin1String("char"));
        break;
    case IntegerType::Char16:
        _text.prepend(QLatin1String("char16_t"));
        break;
    case IntegerType::Char32:
        _text.prepend(QLatin1String("char32_t"));
        break;
    case IntegerType::WideChar:
        _text.prepend(QLatin1String("wchar_t"));
        break;
    case IntegerType::Bool:
        _text.prepend(QLatin1String("bool"));
        break;
    case IntegerType::Short:
        _text.prepend(QLatin1String("short"));
        break;
    case IntegerType::Int:
        _text.prepend(QLatin1String("int"));
        break;
    case IntegerType::Long:
        _text.prepend(QLatin1String("long"));
        break;
    case IntegerType::LongLong:
        _text.prepend(QLatin1String("long long"));
        break;
    }

    if (_fullySpecifiedType.isSigned() || _fullySpecifiedType.isUnsigned()) {
        prependWordSeparatorSpace();
        if (_fullySpecifiedType.isSigned())
            _text.prepend(QLatin1String("signed"));
        else if (_fullySpecifiedType.isUnsigned())
            _text.prepend(QLatin1String("unsigned"));
    }

    prependCv(_fullySpecifiedType);
}

void TypePrettyPrinter::visit(FloatType *type)
{
    prependSpaceUnlessBracket();
    switch (type->kind()) {
    case FloatType::Float:
        _text.prepend(QLatin1String("float"));
        break;
    case FloatType::Double:
        _text.prepend(QLatin1String("double"));
        break;
    case FloatType::LongDouble:
        _text.prepend(QLatin1String("long double"));
        break;
    }

    prependCv(_fullySpecifiedType);
}

void TypePrettyPrinter::visit(PointerToMemberType *type)
{
    prependCv(_fullySpecifiedType);
    _text.prepend(QLatin1String("::*"));
    _text.prepend(_overview->prettyName(type->memberName()));
    _needsParens = true;
    acceptType(type->elementType());
}

void TypePrettyPrinter::prependSpaceBeforeIndirection(const FullySpecifiedType &type)
{
    const bool elementTypeIsPointerOrReference = type.type()->isPointerType()
        || type.type()->isReferenceType();
    const bool elementIsConstPointerOrReference = elementTypeIsPointerOrReference && type.isConst();
    const bool shouldBindToLeftSpecifier = _overview->starBindFlags & Overview::BindToLeftSpecifier;
    if (elementIsConstPointerOrReference && ! shouldBindToLeftSpecifier)
        _text.prepend(QLatin1Char(' '));
}

void TypePrettyPrinter::prependSpaceAfterIndirection(bool hasName)
{
    const bool hasCvSpecifier = _fullySpecifiedType.isConst() || _fullySpecifiedType.isVolatile();
    const bool shouldBindToIdentifier = _overview->starBindFlags & Overview::BindToIdentifier;
    const bool shouldBindToRightSpecifier =
        _overview->starBindFlags & Overview::BindToRightSpecifier;

    const bool spaceBeforeNameNeeded = hasName && ! shouldBindToIdentifier
        && ! _isIndirectionToArrayOrFunction;
    const bool spaceBeforeSpecifierNeeded = hasCvSpecifier && ! shouldBindToRightSpecifier;

    const bool case1 = hasCvSpecifier && spaceBeforeSpecifierNeeded;
    const bool case2 = ! hasCvSpecifier && spaceBeforeNameNeeded;
    // case 3: In "char *argv[]", put a space between '*' and "argv" when requested
    const bool case3 = ! hasCvSpecifier && ! shouldBindToIdentifier
        && ! _isIndirectionToArrayOrFunction && _text.size() && _text.at(0).isLetter();
    if (case1 || case2 || case3)
        _text.prepend(QLatin1Char(' '));
}

void TypePrettyPrinter::visit(PointerType *type)
{
    const bool isIndirectionToFunction = type->elementType().type()->isFunctionType();
    const bool isIndirectionToArray = type->elementType().type()->isArrayType();

    visitIndirectionType(aPointerType, type->elementType(),
        isIndirectionToFunction || isIndirectionToArray);
}

void TypePrettyPrinter::visit(ReferenceType *type)
{
    const bool isIndirectionToFunction = type->elementType().type()->isFunctionType();
    const bool isIndirectionToArray = type->elementType().type()->isArrayType();
    const IndirectionType indirectionType = type->isRvalueReference()
        ? aRvalueReferenceType : aReferenceType;

    visitIndirectionType(indirectionType, type->elementType(),
        isIndirectionToFunction || isIndirectionToArray);
}

void TypePrettyPrinter::visit(ArrayType *type)
{
    if (_needsParens) {
        _text.prepend(QLatin1Char('('));
        _text.append(QLatin1Char(')'));
        _needsParens = false;
    } else if (! _name.isEmpty()) {
        _text.prepend(_name);
        _name.clear();
    }
    _text.append(QLatin1String("[]"));

    acceptType(type->elementType());
}

static bool endsWithPtrOrRef(const QString &type)
{
    return type.endsWith(QLatin1Char('*'))
            || type.endsWith(QLatin1Char('&'));
}

void TypePrettyPrinter::visit(Function *type)
{
    if (_needsParens) {
        _text.prepend(QLatin1Char('('));
        if (! _name.isEmpty()) {
            appendSpace();
            _text.append(_name);
            _name.clear();
        }
        _text.append(QLatin1Char(')'));
        _needsParens = false;
    } else if (! _name.isEmpty() && _overview->showFunctionSignatures) {
        appendSpace();
        _text.append(_name);
        _name.clear();
    }

    if (_overview->showReturnTypes) {
        const QString returnType = _overview->prettyType(type->returnType());
        if (!returnType.isEmpty()) {
            if (!endsWithPtrOrRef(returnType))
                _text.prepend(QLatin1Char(' '));
            _text.prepend(returnType);
        }
    }

    if (_overview->showEnclosingTemplate) {
        if (Template *templ = type->enclosingTemplate()) {
            QString templateScope = "template<";
            for (unsigned i = 0, total = templ->templateParameterCount(); i < total; ++i) {
                if (Symbol *param = templ->templateParameterAt(i)) {
                    if (i > 0)
                        templateScope.append(", ");
                    if (TypenameArgument *typenameArg = param->asTypenameArgument()) {
                        templateScope.append(QLatin1String(typenameArg->isClassDeclarator()
                                                           ? "class " : "typename "));
                        templateScope.append(_overview->prettyName(typenameArg->name()));
                    } else if (Argument *arg = param->asArgument()) {
                        templateScope.append(operator()(arg->type(),
                                                        _overview->prettyName(arg->name())));
                    }
                }
            }
            _text.prepend(templateScope + ">\n");
        }
    }

    if (_overview->showFunctionSignatures) {
        Overview argumentText;
        argumentText.starBindFlags = _overview->starBindFlags;
        argumentText.showReturnTypes = true;
        argumentText.showArgumentNames = false;
        argumentText.showFunctionSignatures = true;

        _text += QLatin1Char('(');

        for (unsigned index = 0, argc = type->argumentCount(); index < argc; ++index) {
            if (index != 0)
                _text += QLatin1String(", ");

            if (Argument *arg = type->argumentAt(index)->asArgument()) {
                if (index + 1 == _overview->markedArgument)
                    const_cast<Overview*>(_overview)->markedArgumentBegin = _text.length();

                const Name *name = 0;

                if (_overview->showArgumentNames)
                    name = arg->name();

                _text += argumentText.prettyType(arg->type(), name);

                if (_overview->showDefaultArguments) {
                    if (const StringLiteral *initializer = arg->initializer()) {
                        _text += QLatin1String(" = ");
                        _text += QString::fromUtf8(initializer->chars(), initializer->size());
                    }
                }

                if (index + 1 == _overview->markedArgument)
                    const_cast<Overview*>(_overview)->markedArgumentEnd = _text.length();
            }
        }

        if (type->isVariadic())
            _text += QLatin1String("...");

        _text += QLatin1Char(')');
        if (type->isConst()) {
            appendSpace();
            _text += QLatin1String("const");
        }
        if (type->isVolatile()) {
            appendSpace();
            _text += QLatin1String("volatile");
        }

        // add ref-qualifier
        if (type->refQualifier() != Function::NoRefQualifier) {
            if (!_overview->starBindFlags.testFlag(Overview::BindToLeftSpecifier)
                    || (!type->isConst() && !type->isVolatile())) {
                appendSpace();
            }
            _text += type->refQualifier() == Function::LvalueRefQualifier
                        ? QLatin1String("&")
                        : QLatin1String("&&");
        }
    }
}

void TypePrettyPrinter::appendSpace()
{
    if (_text.isEmpty())
        return;

    const QChar ch = _text.at(_text.length() - 1);

    if (ch.isLetterOrNumber() || ch == QLatin1Char('_') || ch == QLatin1Char(')')
            || ch == QLatin1Char('>'))
        _text += QLatin1Char(' ');
}

void TypePrettyPrinter::prependSpaceUnlessBracket()
{
    if (_text.isEmpty())
        return;

    const QChar ch = _text.at(0);

    if (ch != QLatin1Char('[')) {
        const bool shouldBindToTypeNam = _overview->starBindFlags & Overview::BindToTypeName;
        const bool caseNoIndirection = ! _isIndirectionType;
        const bool caseIndirectionToArrayOrFunction = _isIndirectionType
            && _isIndirectionToArrayOrFunction;
        const bool casePointerNoBind = _isIndirectionType && ! _isIndirectionToArrayOrFunction
            && ! shouldBindToTypeNam;
        if (caseNoIndirection || caseIndirectionToArrayOrFunction || casePointerNoBind)
            _text.prepend(QLatin1Char(' '));
    }
}

void TypePrettyPrinter::prependWordSeparatorSpace()
{
    if (_text.isEmpty())
        return;

    const QChar ch = _text.at(0);

    if (ch.isLetterOrNumber() || ch == QLatin1Char('_'))
        _text.prepend(QLatin1Char(' '));
}

void TypePrettyPrinter::prependCv(const FullySpecifiedType &ty)
{
    if (ty.isVolatile()) {
        prependWordSeparatorSpace();
        _text.prepend(QLatin1String("volatile"));
    }

    if (ty.isConst()) {
        prependWordSeparatorSpace();
        _text.prepend(QLatin1String("const"));
    }
}

