// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmljsstaticanalysismessage.h"

#include "qmljsconstants.h"
#include "qmljstr.h"

#include "parser/qmljsengine_p.h"
#include "parser/qmljsdiagnosticmessage_p.h"

#include <utils/qtcassert.h>

#include <QCoreApplication>
#include <QRegularExpression>

using namespace QmlJS;
using namespace QmlJS::StaticAnalysis;
using namespace QmlJS::Severity;

namespace {

class StaticAnalysisMessages
{
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
    return Tr::tr("Do not use \"%1\" as a constructor."
                  "\n\nFor more information, see the"
                  " \"Checking Code Syntax\" documentation.")
            .arg(QLatin1String(what));
}

StaticAnalysisMessages::StaticAnalysisMessages()
{
    // When changing a message or severity, update the documentation, currently
    // in creator-code-syntax.qdoc, accordingly.
    newMsg(ErrInvalidEnumValue, Error,
           Tr::tr("Invalid value for enum."));
    newMsg(ErrEnumValueMustBeStringOrNumber, Error,
           Tr::tr("Enum value must be a string or a number."));
    newMsg(ErrNumberValueExpected, Error,
           Tr::tr("Number value expected."));
    newMsg(ErrBooleanValueExpected, Error,
           Tr::tr("Boolean value expected."));
    newMsg(ErrStringValueExpected, Error,
           Tr::tr("String value expected."));
    newMsg(ErrInvalidUrl, Error,
           Tr::tr("Invalid URL."));
    newMsg(WarnFileOrDirectoryDoesNotExist, Warning,
           Tr::tr("File or directory does not exist."));
    newMsg(ErrInvalidColor, Error,
           Tr::tr("Invalid color."));
    newMsg(ErrAnchorLineExpected, Error,
           Tr::tr("Anchor line expected."));
    newMsg(ErrPropertiesCanOnlyHaveOneBinding, Error,
           Tr::tr("Duplicate property binding."));
    newMsg(ErrIdExpected, Error,
           Tr::tr("Id expected."));
    newMsg(ErrInvalidId, Error,
           Tr::tr("Invalid id."));
    newMsg(ErrDuplicateId, Error,
           Tr::tr("Duplicate id."));
    newMsg(ErrInvalidPropertyName, Error,
           Tr::tr("Invalid property name \"%1\"."), 1);
    newMsg(ErrDoesNotHaveMembers, Error,
           Tr::tr("\"%1\" does not have members."), 1);
    newMsg(ErrInvalidMember, Error,
           Tr::tr("\"%1\" is not a member of \"%2\"."), 2);
    newMsg(WarnAssignmentInCondition, Warning,
           Tr::tr("Assignment in condition."));
    newMsg(WarnCaseWithoutFlowControl, Warning,
           Tr::tr("Unterminated non-empty case block."));
    newMsg(WarnEval, Warning,
           Tr::tr("Do not use 'eval'."));
    newMsg(WarnUnreachable, Warning,
           Tr::tr("Unreachable."));
    newMsg(WarnWith, Warning,
           Tr::tr("Do not use 'with'."));
    newMsg(WarnComma, Warning,
           Tr::tr("Do not use comma expressions."));
    newMsg(WarnAlreadyFormalParameter, Warning,
           Tr::tr("\"%1\" already is a formal parameter."), 1);
    newMsg(WarnUnnecessaryMessageSuppression, Warning,
           Tr::tr("Unnecessary message suppression."));
    newMsg(WarnAlreadyFunction, Warning,
           Tr::tr("\"%1\" already is a function."), 1);
    newMsg(WarnVarUsedBeforeDeclaration, Warning,
           Tr::tr("var \"%1\" is used before its declaration."), 1);
    newMsg(WarnAlreadyVar, Warning,
           Tr::tr("\"%1\" already is a var."), 1);
    newMsg(WarnDuplicateDeclaration, Warning,
           Tr::tr("\"%1\" is declared more than once."), 1);
    newMsg(WarnFunctionUsedBeforeDeclaration, Warning,
           Tr::tr("Function \"%1\" is used before its declaration."), 1);
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
           Tr::tr("The 'function' keyword and the opening parenthesis should be separated by a single space."));
    newMsg(WarnBlock, Warning,
           Tr::tr("Do not use stand-alone blocks."));
    newMsg(WarnVoid, Warning,
           Tr::tr("Do not use void expressions."));
    newMsg(WarnConfusingPluses, Warning,
           Tr::tr("Confusing pluses."));
    newMsg(WarnConfusingMinuses, Warning,
           Tr::tr("Confusing minuses."));
    newMsg(HintDeclareVarsInOneLine, Hint,
           Tr::tr("Declare all function vars on a single line."));
    newMsg(HintExtraParentheses, Hint,
           Tr::tr("Unnecessary parentheses."));
    newMsg(MaybeWarnEqualityTypeCoercion, MaybeWarning,
           Tr::tr("== and != may perform type coercion, use === or !== to avoid it."));
    newMsg(WarnConfusingExpressionStatement, Warning,
           Tr::tr("Expression statements should be assignments, calls or delete expressions only."));
    newMsg(HintDeclarationsShouldBeAtStartOfFunction, Hint,
           Tr::tr("Place var declarations at the start of a function."));
    newMsg(HintOneStatementPerLine, Hint,
           Tr::tr("Use only one statement per line."));
    newMsg(ErrUnknownComponent, Error,
           Tr::tr("Unknown component."));
    newMsg(ErrCouldNotResolvePrototypeOf, Error,
           Tr::tr("Could not resolve the prototype \"%1\" of \"%2\"."), 2);
    newMsg(ErrCouldNotResolvePrototype, Error,
           Tr::tr("Could not resolve the prototype \"%1\"."), 1);
    newMsg(ErrPrototypeCycle, Error,
           Tr::tr("Prototype cycle, the last non-repeated component is \"%1\"."), 1);
    newMsg(ErrInvalidPropertyType, Error,
           Tr::tr("Invalid property type \"%1\"."), 1);
    newMsg(WarnEqualityTypeCoercion, Error,
           Tr::tr("== and != perform type coercion, use === or !== to avoid it."));
    newMsg(WarnExpectedNewWithUppercaseFunction, Error,
           Tr::tr("Calls of functions that start with an uppercase letter should use 'new'."));
    newMsg(WarnNewWithLowercaseFunction, Error,
           Tr::tr("Use 'new' only with functions that start with an uppercase letter."));
    newMsg(WarnNumberConstructor, Error,
           msgInvalidConstructor("Function"));
    newMsg(HintBinaryOperatorSpacing, Hint,
           Tr::tr("Use spaces around binary operators."));
    newMsg(WarnUnintentinalEmptyBlock, Error,
           Tr::tr("Unintentional empty block, use ({}) for empty object literal."));
    newMsg(HintPreferNonVarPropertyType, Hint,
           Tr::tr("Use %1 instead of 'var' or 'variant' to improve performance."), 1);
    newMsg(ErrMissingRequiredProperty, Error,
           Tr::tr("Missing property \"%1\"."), 1);
    newMsg(ErrObjectValueExpected, Error,
           Tr::tr("Object value expected."));
    newMsg(ErrArrayValueExpected, Error,
           Tr::tr("Array value expected."));
    newMsg(ErrDifferentValueExpected, Error,
           Tr::tr("%1 value expected."), 1);
    newMsg(ErrSmallerNumberValueExpected, Error,
           Tr::tr("Maximum number value is %1."), 1);
    newMsg(ErrLargerNumberValueExpected, Error,
           Tr::tr("Minimum number value is %1."), 1);
    newMsg(ErrMaximumNumberValueIsExclusive, Error,
           Tr::tr("Maximum number value is exclusive."));
    newMsg(ErrMinimumNumberValueIsExclusive, Error,
           Tr::tr("Minimum number value is exclusive."));
    newMsg(ErrInvalidStringValuePattern, Error,
           Tr::tr("String value does not match required pattern."));
    newMsg(ErrLongerStringValueExpected, Error,
           Tr::tr("Minimum string value length is %1."), 1);
    newMsg(ErrShorterStringValueExpected, Error,
           Tr::tr("Maximum string value length is %1."), 1);
    newMsg(ErrInvalidArrayValueLength, Error,
           Tr::tr("%1 elements expected in array value."), 1);
    newMsg(WarnImperativeCodeNotEditableInVisualDesigner, Warning,
           Tr::tr("Imperative code is not supported in Qt Design Studio."));
    newMsg(WarnUnsupportedTypeInVisualDesigner, Warning,
           Tr::tr("This type (%1) is not supported in Qt Design Studio."), 1);
    newMsg(WarnReferenceToParentItemNotSupportedByVisualDesigner, Warning,
           Tr::tr("Reference to parent item cannot be resolved correctly by Qt Design Studio."));
    newMsg(WarnUndefinedValueForVisualDesigner, Warning,
           Tr::tr("This visual property binding cannot be evaluated in the local context "
                  "and might not show up in Qt Design Studio as expected."));
    newMsg(WarnStatesOnlyInRootItemForVisualDesigner, Warning,
           Tr::tr("Qt Design Studio only supports states in the root item."));
    newMsg(ErrInvalidIdeInVisualDesigner, Error,
           Tr::tr("This id might be ambiguous and is not supported in Qt Design Studio."));
    newMsg(ErrUnsupportedRootTypeInVisualDesigner, Error,
           Tr::tr("This type (%1) is not supported as a root element by Qt Design Studio."), 1);
    newMsg(ErrUnsupportedRootTypeInQmlUi, Error,
           Tr::tr("This type (%1) is not supported as a root element of a UI file (.ui.qml)."), 1);
    newMsg(ErrUnsupportedTypeInQmlUi, Error,
           Tr::tr("This type (%1) is not supported in a UI file (.ui.qml)."), 1);
    newMsg(ErrFunctionsNotSupportedInQmlUi, Error,
           Tr::tr("Functions are not supported in a UI file (.ui.qml)."));
    newMsg(ErrBlocksNotSupportedInQmlUi, Error,
           Tr::tr("JavaScript blocks are not supported in a UI file (.ui.qml)."));
    newMsg(ErrBehavioursNotSupportedInQmlUi, Error,
           Tr::tr("Behavior type is not supported in a UI file (.ui.qml)."));
    newMsg(ErrStatesOnlyInRootItemInQmlUi, Error,
           Tr::tr("States are only supported in the root item in a UI file (.ui.qml)."));
    newMsg(ErrReferenceToParentItemNotSupportedInQmlUi, Error,
           Tr::tr("Referencing the parent of the root item is not supported in a UI file (.ui.qml)."));
    newMsg(WarnDoNotMixTranslationFunctionsInQmlUi,
           Warning,
           Tr::tr("Do not mix translation functions in a UI file (.ui.qml)."));
    newMsg(StateCannotHaveChildItem, Error,
           Tr::tr("A State cannot have a child item (%1)."), 1);
    newMsg(WarnDuplicateImport, Warning,
           Tr::tr("Duplicate import (%1)."), 1);
    newMsg(ErrHitMaximumRecursion, Error,
           Tr::tr("Hit maximum recursion limit when visiting AST."));
    newMsg(ErrTypeIsInstantiatedRecursively, Error,
           Tr::tr("Type cannot be instantiated recursively (%1)."), 1);
    newMsg(ErrToManyComponentChildren, Error,
           Tr::tr("Components are only allowed to have a single child element."));
    newMsg(WarnComponentRequiresChildren, Warning,
           Tr::tr("Components require a child element."));
    newMsg(ErrAliasReferRoot, Error,
           Tr::tr("Do not reference the root item as alias."));
    newMsg(WarnAliasReferRootHierarchy, Warning,
           Tr::tr("Avoid referencing the root item in a hierarchy."));
}

} // anonymous namespace

Q_GLOBAL_STATIC(StaticAnalysisMessages, messages)

QList<Type> Message::allMessageTypes()
{
    return messages()->messages.keys();
}

Message::Message()
    : type(UnknownType)
{}

Message::Message(Type type,
                 SourceLocation location,
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

QRegularExpression Message::suppressionPattern()
{
    return QRegularExpression("@disable-check M(\\d+)");
}

const PrototypeMessageData Message::prototypeForMessageType(Type type)
{
     QTC_CHECK(messages()->messages.contains(type));
     const PrototypeMessageData &prototype = messages()->messages.value(type);

     return prototype;
}
