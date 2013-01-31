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

#ifndef QMLJS_STATICANALYSIS_QMLJSSTATICANALYSISMESSAGE_H
#define QMLJS_STATICANALYSIS_QMLJSSTATICANALYSISMESSAGE_H

#include "qmljs_global.h"
#include "parser/qmljsengine_p.h"

#include <QRegExp>
#include <QString>
#include <QList>

namespace QmlJS {
namespace StaticAnalysis {

enum Severity
{
    Hint,         // cosmetic or convention
    MaybeWarning, // possibly a warning, insufficient information
    Warning,      // could cause unintended behavior
    MaybeError,   // possibly an error, insufficient information
    Error         // definitely an error
};

enum Type
{
    // Changing the numbers can break user code.
    // When adding a new check, also add it to the documentation, currently
    // in creator-editors.qdoc.
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
    HintDeclarationsShouldBeAtStartOfFunction = 201,
    HintOneStatementPerLine = 202,
    WarnImperativeCodeNotEditableInVisualDesigner = 203,
    WarnUnsupportedTypeInVisualDesigner = 204,
    WarnReferenceToParentItemNotSupportedByVisualDesigner = 205,
    WarnUndefinedValueForVisualDesigner = 206,
    WarnStatesOnlyInRootItemForVisualDesigner = 207,
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
    ErrInvalidArrayValueLength = 323
};

class QMLJS_EXPORT Message
{
public:
    Message();
    Message(Type type, AST::SourceLocation location,
            const QString &arg1 = QString(),
            const QString &arg2 = QString(),
            bool appendTypeId = true);

    static QList<Type> allMessageTypes();

    bool isValid() const;
    DiagnosticMessage toDiagnosticMessage() const;

    QString suppressionString() const;
    static QRegExp suppressionPattern();

    AST::SourceLocation location;
    QString message;
    Type type;
    Severity severity;
};

} // namespace StaticAnalysis
} // namespace QmlJS

#endif // QMLJS_STATICANALYSIS_QMLJSSTATICANALYSISMESSAGE_H
