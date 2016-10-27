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

#include "qmljsstaticanalysismessage.h"
#include "qmljsconstants.h"
#include "parser/qmljsengine_p.h"

#include <utils/qtcassert.h>

#include <QCoreApplication>
#include <QRegExp>

using namespace QmlJS;
using namespace QmlJS::StaticAnalysis;
using namespace QmlJS::Severity;

namespace {

class StaticAnalysisMessages
{
    Q_DECLARE_TR_FUNCTIONS(QmlJS::StaticAnalysisMessages)

public:
    void newMsg(Type type, Enum severity, const QString &message, int placeholders = 0)
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
    return StaticAnalysisMessages::tr("Do not use \"%1\" as a constructor."
                                      "\n\nFor more information, see the"
                                      " \"Checking Code Syntax\" documentation.")
            .arg(QLatin1String(what));
}

StaticAnalysisMessages::StaticAnalysisMessages()
{
    // When changing a message or severity, update the documentation, currently
    // in creator-editors.qdoc, accordingly.
    newMsg(ErrInvalidEnumValue, Error,
           tr("Invalid value for enum."));
    newMsg(ErrEnumValueMustBeStringOrNumber, Error,
           tr("Enum value must be a string or a number."));
    newMsg(ErrNumberValueExpected, Error,
           tr("Number value expected."));
    newMsg(ErrBooleanValueExpected, Error,
           tr("Boolean value expected."));
    newMsg(ErrStringValueExpected, Error,
           tr("String value expected."));
    newMsg(ErrInvalidUrl, Error,
           tr("Invalid URL."));
    newMsg(WarnFileOrDirectoryDoesNotExist, Warning,
           tr("File or directory does not exist."));
    newMsg(ErrInvalidColor, Error,
           tr("Invalid color."));
    newMsg(ErrAnchorLineExpected, Error,
           tr("Anchor line expected."));
    newMsg(ErrPropertiesCanOnlyHaveOneBinding, Error,
           tr("Duplicate property binding."));
    newMsg(ErrIdExpected, Error,
           tr("Id expected."));
    newMsg(ErrInvalidId, Error,
           tr("Invalid id."));
    newMsg(ErrDuplicateId, Error,
           tr("Duplicate id."));
    newMsg(ErrInvalidPropertyName, Error,
           tr("Invalid property name \"%1\"."), 1);
    newMsg(ErrDoesNotHaveMembers, Error,
           tr("\"%1\" does not have members."), 1);
    newMsg(ErrInvalidMember, Error,
           tr("\"%1\" is not a member of \"%2\"."), 2);
    newMsg(WarnAssignmentInCondition, Warning,
           tr("Assignment in condition."));
    newMsg(WarnCaseWithoutFlowControl, Warning,
           tr("Unterminated non-empty case block."));
    newMsg(WarnEval, Warning,
           tr("Do not use 'eval'."));
    newMsg(WarnUnreachable, Warning,
           tr("Unreachable."));
    newMsg(WarnWith, Warning,
           tr("Do not use 'with'."));
    newMsg(WarnComma, Warning,
           tr("Do not use comma expressions."));
    newMsg(WarnAlreadyFormalParameter, Warning,
           tr("\"%1\" already is a formal parameter."), 1);
    newMsg(WarnUnnecessaryMessageSuppression, Warning,
           tr("Unnecessary message suppression."));
    newMsg(WarnAlreadyFunction, Warning,
           tr("\"%1\" already is a function."), 1);
    newMsg(WarnVarUsedBeforeDeclaration, Warning,
           tr("var \"%1\" is used before its declaration."), 1);
    newMsg(WarnAlreadyVar, Warning,
           tr("\"%1\" already is a var."), 1);
    newMsg(WarnDuplicateDeclaration, Warning,
           tr("\"%1\" is declared more than once."), 1);
    newMsg(WarnFunctionUsedBeforeDeclaration, Warning,
           tr("Function \"%1\" is used before its declaration."), 1);
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
           tr("The 'function' keyword and the opening parenthesis should be separated by a single space."));
    newMsg(WarnBlock, Warning,
           tr("Do not use stand-alone blocks."));
    newMsg(WarnVoid, Warning,
           tr("Do not use void expressions."));
    newMsg(WarnConfusingPluses, Warning,
           tr("Confusing pluses."));
    newMsg(WarnConfusingMinuses, Warning,
           tr("Confusing minuses."));
    newMsg(HintDeclareVarsInOneLine, Hint,
           tr("Declare all function vars on a single line."));
    newMsg(HintExtraParentheses, Hint,
           tr("Unnecessary parentheses."));
    newMsg(MaybeWarnEqualityTypeCoercion, MaybeWarning,
           tr("== and != may perform type coercion, use === or !== to avoid it."));
    newMsg(WarnConfusingExpressionStatement, Error,
           tr("Expression statements should be assignments, calls or delete expressions only."));
    newMsg(HintDeclarationsShouldBeAtStartOfFunction, Hint,
           tr("Place var declarations at the start of a function."));
    newMsg(HintOneStatementPerLine, Hint,
           tr("Use only one statement per line."));
    newMsg(ErrUnknownComponent, Error,
           tr("Unknown component."));
    newMsg(ErrCouldNotResolvePrototypeOf, Error,
           tr("Could not resolve the prototype \"%1\" of \"%2\"."), 2);
    newMsg(ErrCouldNotResolvePrototype, Error,
           tr("Could not resolve the prototype \"%1\"."), 1);
    newMsg(ErrPrototypeCycle, Error,
           tr("Prototype cycle, the last non-repeated component is \"%1\"."), 1);
    newMsg(ErrInvalidPropertyType, Error,
           tr("Invalid property type \"%1\"."), 1);
    newMsg(WarnEqualityTypeCoercion, Error,
           tr("== and != perform type coercion, use === or !== to avoid it."));
    newMsg(WarnExpectedNewWithUppercaseFunction, Error,
           tr("Calls of functions that start with an uppercase letter should use 'new'."));
    newMsg(WarnNewWithLowercaseFunction, Error,
           tr("Use 'new' only with functions that start with an uppercase letter."));
    newMsg(WarnNumberConstructor, Error,
           msgInvalidConstructor("Function"));
    newMsg(HintBinaryOperatorSpacing, Hint,
           tr("Use spaces around binary operators."));
    newMsg(WarnUnintentinalEmptyBlock, Error,
           tr("Unintentional empty block, use ({}) for empty object literal."));
    newMsg(HintPreferNonVarPropertyType, Hint,
           tr("Use %1 instead of 'var' or 'variant' to improve performance."), 1);
    newMsg(ErrMissingRequiredProperty, Error,
           tr("Missing property \"%1\"."), 1);
    newMsg(ErrObjectValueExpected, Error,
           tr("Object value expected."));
    newMsg(ErrArrayValueExpected, Error,
           tr("Array value expected."));
    newMsg(ErrDifferentValueExpected, Error,
           tr("%1 value expected."), 1);
    newMsg(ErrSmallerNumberValueExpected, Error,
           tr("Maximum number value is %1."), 1);
    newMsg(ErrLargerNumberValueExpected, Error,
           tr("Minimum number value is %1."), 1);
    newMsg(ErrMaximumNumberValueIsExclusive, Error,
           tr("Maximum number value is exclusive."));
    newMsg(ErrMinimumNumberValueIsExclusive, Error,
           tr("Minimum number value is exclusive."));
    newMsg(ErrInvalidStringValuePattern, Error,
           tr("String value does not match required pattern."));
    newMsg(ErrLongerStringValueExpected, Error,
           tr("Minimum string value length is %1."), 1);
    newMsg(ErrShorterStringValueExpected, Error,
           tr("Maximum string value length is %1."), 1);
    newMsg(ErrInvalidArrayValueLength, Error,
           tr("%1 elements expected in array value."), 1);
    newMsg(WarnImperativeCodeNotEditableInVisualDesigner, Warning,
            tr("Imperative code is not supported in the Qt Quick Designer."));
    newMsg(WarnUnsupportedTypeInVisualDesigner, Warning,
            tr("This type (%1) is not supported in the Qt Quick Designer."), 1);
    newMsg(WarnReferenceToParentItemNotSupportedByVisualDesigner, Warning,
            tr("Reference to parent item cannot be resolved correctly by the Qt Quick Designer."));
    newMsg(WarnUndefinedValueForVisualDesigner, Warning,
            tr("This visual property binding cannot be evaluated in the local context "
               "and might not show up in Qt Quick Designer as expected."));
    newMsg(WarnStatesOnlyInRootItemForVisualDesigner, Warning,
            tr("Qt Quick Designer only supports states in the root item."));
    newMsg(ErrInvalidIdeInVisualDesigner, Error,
           tr("This id might be ambiguous and is not supported in the Qt Quick Designer."));
    newMsg(WarnAboutQtQuick1InsteadQtQuick2, Warning,
            tr("Using Qt Quick 1 code model instead of Qt Quick 2."));
    newMsg(ErrUnsupportedRootTypeInVisualDesigner, Error,
           tr("This type (%1) is not supported as a root element by Qt Quick Designer."), 1);
    newMsg(ErrUnsupportedRootTypeInQmlUi, Error,
           tr("This type (%1) is not supported as a root element of a Qt Quick UI form."), 1);
    newMsg(ErrUnsupportedTypeInQmlUi, Error,
            tr("This type (%1) is not supported in a Qt Quick UI form."), 1);
    newMsg(ErrFunctionsNotSupportedInQmlUi, Error,
            tr("Functions are not supported in a Qt Quick UI form."));
    newMsg(ErrBlocksNotSupportedInQmlUi, Error,
            tr("JavaScript blocks are not supported in a Qt Quick UI form."));
    newMsg(ErrBehavioursNotSupportedInQmlUi, Error,
            tr("Behavior type is not supported in a Qt Quick UI form."));
    newMsg(ErrStatesOnlyInRootItemInQmlUi, Error,
            tr("States are only supported in the root item in a Qt Quick UI form."));
    newMsg(ErrReferenceToParentItemNotSupportedInQmlUi, Error,
            tr("Referencing the parent of the root item is not supported in a Qt Quick UI form."));
    newMsg(StateCannotHaveChildItem, Error,
            tr("A State cannot have a child item (%1)."), 1);
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
    const PrototypeMessageData &prototype = prototypeForMessageType(type);
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
        diagnostic.kind = Warning;
        break;
    default:
        diagnostic.kind = Error;
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

const PrototypeMessageData Message::prototypeForMessageType(Type type)
{
     QTC_CHECK(messages()->messages.contains(type));
     const PrototypeMessageData &prototype = messages()->messages.value(type);

     return prototype;
}
