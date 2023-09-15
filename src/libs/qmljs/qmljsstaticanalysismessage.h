// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmljs_global.h"
#include "qmljsconstants.h"
#include "parser/qmljsastfwd_p.h"

#include <QString>
#include <QList>

QT_FORWARD_DECLARE_CLASS(QRegularExpression)

namespace QmlJS {
class DiagnosticMessage;

namespace StaticAnalysis {

enum Type {
    // Changing the numbers can break user code.
    // When adding a new check, also add it to the documentation, currently
    // in creator-code-syntax.qdoc.
    UnknownType = 0,
    ErrInvalidEnumValue = 1,
    ErrEnumValueMustBeStringOrNumber = 2,
    ErrNumberValueExpected = 3,
    ErrBooleanValueExpected = 4,
    ErrStringValueExpected = 5,
    ErrInvalidUrl = 6,
    WarnFileOrDirectoryDoesNotExist = 7,
    ErrInvalidColor = 8,
    ErrAnchorLineExpected = 9,
    ErrPropertiesCanOnlyHaveOneBinding = 10,
    ErrIdExpected = 11,
    ErrInvalidId = 14,
    ErrDuplicateId = 15,
    ErrInvalidPropertyName = 16,
    ErrDoesNotHaveMembers = 17,
    ErrInvalidMember = 18,
    WarnAssignmentInCondition = 19,
    WarnCaseWithoutFlowControl = 20,
    WarnEval = 23,
    WarnUnreachable = 28,
    WarnWith = 29,
    WarnComma = 30,
    WarnUnnecessaryMessageSuppression = 31,
    WarnAlreadyFormalParameter = 103,
    WarnAlreadyFunction = 104,
    WarnVarUsedBeforeDeclaration = 105,
    WarnAlreadyVar = 106,
    WarnDuplicateDeclaration = 107,
    WarnFunctionUsedBeforeDeclaration = 108,
    WarnBooleanConstructor = 109,
    WarnStringConstructor = 110,
    WarnObjectConstructor = 111,
    WarnArrayConstructor = 112,
    WarnFunctionConstructor = 113,
    HintAnonymousFunctionSpacing = 114,
    WarnBlock = 115,
    WarnVoid = 116,
    WarnConfusingPluses = 117,
    WarnConfusingMinuses = 119,
    HintDeclareVarsInOneLine = 121,
    HintExtraParentheses = 123,
    MaybeWarnEqualityTypeCoercion = 126,
    WarnConfusingExpressionStatement = 127,
    StateCannotHaveChildItem = 128,
    ErrTypeIsInstantiatedRecursively = 129,
    HintDeclarationsShouldBeAtStartOfFunction = 201,
    HintOneStatementPerLine = 202,
    WarnImperativeCodeNotEditableInVisualDesigner = 203,
    WarnUnsupportedTypeInVisualDesigner = 204,
    WarnReferenceToParentItemNotSupportedByVisualDesigner = 205,
    WarnUndefinedValueForVisualDesigner = 206,
    WarnStatesOnlyInRootItemForVisualDesigner = 207,
    ErrUnsupportedRootTypeInVisualDesigner = 208,
    ErrInvalidIdeInVisualDesigner = 209,
    ErrUnsupportedRootTypeInQmlUi = 220,
    ErrUnsupportedTypeInQmlUi = 221,
    ErrFunctionsNotSupportedInQmlUi = 222,
    ErrBlocksNotSupportedInQmlUi = 223,
    ErrBehavioursNotSupportedInQmlUi = 224,
    ErrStatesOnlyInRootItemInQmlUi = 225,
    ErrReferenceToParentItemNotSupportedInQmlUi = 226,
    WarnDoNotMixTranslationFunctionsInQmlUi = 227,
    ErrUnknownComponent = 300,
    ErrCouldNotResolvePrototypeOf = 301,
    ErrCouldNotResolvePrototype = 302,
    ErrPrototypeCycle = 303,
    ErrInvalidPropertyType = 304,
    WarnEqualityTypeCoercion = 305,
    WarnExpectedNewWithUppercaseFunction = 306,
    WarnNewWithLowercaseFunction = 307,
    WarnNumberConstructor = 308,
    HintBinaryOperatorSpacing = 309,
    WarnUnintentinalEmptyBlock = 310,
    HintPreferNonVarPropertyType = 311,
    ErrMissingRequiredProperty = 312,
    ErrObjectValueExpected = 313,
    ErrArrayValueExpected = 314,
    ErrDifferentValueExpected = 315,
    ErrSmallerNumberValueExpected = 316,
    ErrLargerNumberValueExpected = 317,
    ErrMaximumNumberValueIsExclusive = 318,
    ErrMinimumNumberValueIsExclusive = 319,
    ErrInvalidStringValuePattern = 320,
    ErrLongerStringValueExpected = 321,
    ErrShorterStringValueExpected = 322,
    ErrInvalidArrayValueLength = 323,
    ErrHitMaximumRecursion = 324,
    ErrToManyComponentChildren = 326,
    WarnComponentRequiresChildren = 327,
    WarnDuplicateImport = 400,
    ErrAliasReferRoot = 401,
    WarnAliasReferRootHierarchy = 402
};

class QMLJS_EXPORT PrototypeMessageData {
public:
    Type type;
    Severity::Enum severity;
    QString message;
    int placeholders;
};

class QMLJS_EXPORT Message
{
public:
    Message();
    Message(Type type, SourceLocation location,
            const QString &arg1 = QString(),
            const QString &arg2 = QString(),
            bool appendTypeId = true);

    static QList<Type> allMessageTypes();

    bool isValid() const;
    DiagnosticMessage toDiagnosticMessage() const;

    QString suppressionString() const;
    static QRegularExpression suppressionPattern();

    SourceLocation location;
    QString message;
    Type type;
    Severity::Enum severity = Severity::Enum::Hint;

    static const PrototypeMessageData prototypeForMessageType(Type type);
};

} // namespace StaticAnalysis
} // namespace QmlJS
