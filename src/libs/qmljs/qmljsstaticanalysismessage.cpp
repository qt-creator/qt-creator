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

#include "qmljsstaticanalysismessage.h"

#include <utils/qtcassert.h>

#include <QCoreApplication>

using namespace QmlJS;
using namespace QmlJS::StaticAnalysis;

namespace {

class StaticAnalysisMessages
{
    Q_DECLARE_TR_FUNCTIONS(QmlJS::StaticAnalysisMessages)

public:
    class PrototypeMessageData {
    public:
        Type type;
        Severity severity;
        QString message;
        int placeholders;
    };

    void newMsg(Type type, Severity severity, const QString &message, int placeholders = 0)
    {
        PrototypeMessageData prototype;
        prototype.type = type;
        prototype.severity = severity;
        prototype.message = message;
        prototype.placeholders = placeholders;
        QTC_CHECK(placeholders <= 2);
        QTC_ASSERT(!messages.contains(type), return);
        messages[type] = prototype;
    }

    StaticAnalysisMessages();
    QHash<Type, PrototypeMessageData> messages;
};

static inline QString msgInvalidConstructor(const char *what)
{
    return StaticAnalysisMessages::tr("do not use '%1' as a constructor").arg(QLatin1String(what));
}

StaticAnalysisMessages::StaticAnalysisMessages()
{
    // When changing a message or severity, update the documentation, currently
    // in creator-editors.qdoc, accordingly.
    newMsg(ErrInvalidEnumValue, Error,
           tr("invalid value for enum"));
    newMsg(ErrEnumValueMustBeStringOrNumber, Error,
           tr("enum value must be a string or a number"));
    newMsg(ErrNumberValueExpected, Error,
           tr("number value expected"));
    newMsg(ErrBooleanValueExpected, Error,
           tr("boolean value expected"));
    newMsg(ErrStringValueExpected, Error,
           tr("string value expected"));
    newMsg(ErrInvalidUrl, Error,
           tr("invalid URL"));
    newMsg(WarnFileOrDirectoryDoesNotExist, Warning,
           tr("file or directory does not exist"));
    newMsg(ErrInvalidColor, Error,
           tr("invalid color"));
    newMsg(ErrAnchorLineExpected, Error,
           tr("anchor line expected"));
    newMsg(ErrPropertiesCanOnlyHaveOneBinding, Error,
           tr("duplicate property binding"));
    newMsg(ErrIdExpected, Error,
           tr("id expected"));
    newMsg(ErrInvalidId, Error,
           tr("invalid id"));
    newMsg(ErrDuplicateId, Error,
           tr("duplicate id"));
    newMsg(ErrInvalidPropertyName, Error,
           tr("invalid property name '%1'"), 1);
    newMsg(ErrDoesNotHaveMembers, Error,
           tr("'%1' does not have members"), 1);
    newMsg(ErrInvalidMember, Error,
           tr("'%1' is not a member of '%2'"), 2);
    newMsg(WarnAssignmentInCondition, Warning,
           tr("assignment in condition"));
    newMsg(WarnCaseWithoutFlowControl, Warning,
           tr("unterminated non-empty case block"));
    newMsg(WarnEval, Warning,
           tr("do not use 'eval'"));
    newMsg(WarnUnreachable, Warning,
           tr("unreachable"));
    newMsg(WarnWith, Warning,
           tr("do not use 'with'"));
    newMsg(WarnComma, Warning,
           tr("do not use comma expressions"));
    newMsg(WarnAlreadyFormalParameter, Warning,
           tr("'%1' is already a formal parameter"), 1);
    newMsg(WarnUnnecessaryMessageSuppression, Warning,
           tr("unnecessary message suppression"));
    newMsg(WarnAlreadyFunction, Warning,
           tr("'%1' is already a function"), 1);
    newMsg(WarnVarUsedBeforeDeclaration, Warning,
           tr("var '%1' is used before its declaration"), 1);
    newMsg(WarnAlreadyVar, Warning,
           tr("'%1' is already a var"), 1);
    newMsg(WarnDuplicateDeclaration, Warning,
           tr("'%1' is declared more than once"), 1);
    newMsg(WarnFunctionUsedBeforeDeclaration, Warning,
           tr("function '%1' is used before its declaration"), 1);
    newMsg(WarnBooleanConstructor, Warning,
           msgInvalidConstructor("Boolean"));
    newMsg(WarnStringConstructor, Warning,
           msgInvalidConstructor("String"));
    newMsg(WarnObjectConstructor, Warning,
           msgInvalidConstructor("Object"));
    newMsg(WarnArrayConstructor, Warning,
           msgInvalidConstructor("Array"));
    newMsg(WarnFunctionConstructor, Warning,
           msgInvalidConstructor("Function"));
    newMsg(HintAnonymousFunctionSpacing, Hint,
           tr("the 'function' keyword and the opening parenthesis should be separated by a single space"));
    newMsg(WarnBlock, Warning,
           tr("do not use stand-alone blocks"));
    newMsg(WarnVoid, Warning,
           tr("do not use void expressions"));
    newMsg(WarnConfusingPluses, Warning,
           tr("confusing pluses"));
    newMsg(WarnConfusingMinuses, Warning,
           tr("confusing minuses"));
    newMsg(HintDeclareVarsInOneLine, Hint,
           tr("declare all function vars on a single line"));
    newMsg(HintExtraParentheses, Hint,
           tr("unnecessary parentheses"));
    newMsg(MaybeWarnEqualityTypeCoercion, MaybeWarning,
           tr("== and != may perform type coercion, use === or !== to avoid"));
    newMsg(WarnConfusingExpressionStatement, Warning,
           tr("expression statements should be assignments, calls or delete expressions only"));
    newMsg(HintDeclarationsShouldBeAtStartOfFunction, Hint,
           tr("var declarations should be at the start of a function"));
    newMsg(HintOneStatementPerLine, Hint,
           tr("only use one statement per line"));
    newMsg(ErrUnknownComponent, Error,
           tr("unknown component"));
    newMsg(ErrCouldNotResolvePrototypeOf, Error,
           tr("could not resolve the prototype '%1' of '%2'"), 2);
    newMsg(ErrCouldNotResolvePrototype, Error,
           tr("could not resolve the prototype '%1'"), 1);
    newMsg(ErrPrototypeCycle, Error,
           tr("prototype cycle, the last non-repeated component is '%1'"), 1);
    newMsg(ErrInvalidPropertyType, Error,
           tr("invalid property type '%1'"), 1);
    newMsg(WarnEqualityTypeCoercion, Warning,
           tr("== and != perform type coercion, use === or !== to avoid"));
    newMsg(WarnExpectedNewWithUppercaseFunction, Warning,
           tr("calls of functions that start with an uppercase letter should use 'new'"));
    newMsg(WarnNewWithLowercaseFunction, Warning,
           tr("'new' should only be used with functions that start with an uppercase letter"));
    newMsg(WarnNumberConstructor, Warning,
           msgInvalidConstructor("Function"));
    newMsg(HintBinaryOperatorSpacing, Hint,
           tr("use spaces around binary operators"));
    newMsg(WarnUnintentinalEmptyBlock, Warning,
           tr("unintentional empty block, use ({}) for empty object literal"));
    newMsg(HintPreferNonVarPropertyType, Hint,
           tr("use %1 instead of 'var' or 'variant' to improve performance"), 1);
    newMsg(ErrMissingRequiredProperty, Error,
           tr("missing property '%1'"), 1);
    newMsg(ErrObjectValueExpected, Error,
           tr("object value expected"));
    newMsg(ErrArrayValueExpected, Error,
           tr("array value expected"));
    newMsg(ErrDifferentValueExpected, Error,
           tr("%1 value expected"), 1);
    newMsg(ErrSmallerNumberValueExpected, Error,
           tr("maximum number value is %1"), 1);
    newMsg(ErrLargerNumberValueExpected, Error,
           tr("minimum number value is %1"), 1);
    newMsg(ErrMaximumNumberValueIsExclusive, Error,
           tr("maximum number value is exclusive"));
    newMsg(ErrMinimumNumberValueIsExclusive, Error,
           tr("minimum number value is exclusive"));
    newMsg(ErrInvalidStringValuePattern, Error,
           tr("string value does not match required pattern"));
    newMsg(ErrLongerStringValueExpected, Error,
           tr("minimum string value length is %1"), 1);
    newMsg(ErrShorterStringValueExpected, Error,
           tr("maximum string value length is %1"), 1);
    newMsg(ErrInvalidArrayValueLength, Error,
           tr("%1 elements expected in array value"), 1);
    newMsg(WarnImperativeCodeNotEditableInVisualDesigner, Warning,
            tr("Imperative code is not supported in the Qt Quick Designer"));
    newMsg(WarnUnsupportedTypeInVisualDesigner, Warning,
            tr("This type is not supported in the Qt Quick Designer"));
    newMsg(WarnReferenceToParentItemNotSupportedByVisualDesigner, Warning,
            tr("Reference to parent item cannot be resolved correctly by the Qt Quick Designer"));
    newMsg(WarnUndefinedValueForVisualDesigner, Warning,
            tr("This visual property binding cannot be evaluated in the local context "
               "and might not show up in Qt Quick Designer as expected"));
    newMsg(WarnStatesOnlyInRootItemForVisualDesigner, Warning,
            tr("Qt Quick Designer only supports states in the root item "));
}

} // anonymous namespace

Q_GLOBAL_STATIC(StaticAnalysisMessages, messages)

QList<Type> Message::allMessageTypes()
{
    return messages()->messages.keys();
}

Message::Message()
    : type(UnknownType), severity(Hint)
{}

Message::Message(Type type,
                 AST::SourceLocation location,
                 const QString &arg1,
                 const QString &arg2,
                 bool appendTypeId)
    : location(location), type(type)
{
    QTC_ASSERT(messages()->messages.contains(type), return);
    const StaticAnalysisMessages::PrototypeMessageData &prototype = messages()->messages.value(type);
    severity = prototype.severity;
    message = prototype.message;
    if (prototype.placeholders == 0) {
        if (!arg1.isEmpty() || !arg2.isEmpty())
            qWarning() << "StaticAnalysis message" << type << "expects no arguments";
    } else if (prototype.placeholders == 1) {
        if (arg1.isEmpty() || !arg2.isEmpty())
            qWarning() << "StaticAnalysis message" << type << "expects exactly one argument";
        message = message.arg(arg1);
    } else if (prototype.placeholders == 2) {
        if (arg1.isEmpty() || arg2.isEmpty())
            qWarning() << "StaticAnalysis message" << type << "expects exactly two arguments";
        message = message.arg(arg1, arg2);
    }
    if (appendTypeId)
        message.append(QString::fromLatin1(" (M%1)").arg(QString::number(prototype.type)));
}

bool Message::isValid() const
{
    return type != UnknownType && location.isValid() && !message.isEmpty();
}

DiagnosticMessage Message::toDiagnosticMessage() const
{
    DiagnosticMessage diagnostic;
    switch (severity) {
    case Hint:
    case MaybeWarning:
    case Warning:
        diagnostic.kind = DiagnosticMessage::Warning;
        break;
    default:
        diagnostic.kind = DiagnosticMessage::Error;
        break;
    }
    diagnostic.loc = location;
    diagnostic.message = message;
    return diagnostic;
}

QString Message::suppressionString() const
{
    return QString::fromLatin1("@disable-check M%1").arg(QString::number(type));
}

QRegExp Message::suppressionPattern()
{
    return QRegExp(QLatin1String("@disable-check M(\\d+)"));
}
