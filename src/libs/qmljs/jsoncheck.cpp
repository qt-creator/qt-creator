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

#include "jsoncheck.h"

#include <qmljs/parser/qmljsast_p.h>
#include <utils/qtcassert.h>

#include <QDebug>
#include <QLatin1String>
#include <QRegExp>
#include <QMap>

#include <cmath>

using namespace QmlJS;
using namespace QmlJS::AST;
using namespace QmlJS::StaticAnalysis;
using namespace Utils;

JsonCheck::JsonCheck(Document::Ptr doc)
    : m_doc(doc)
    , m_schema(0)
{
    QTC_CHECK(m_doc->ast());
}

JsonCheck::~JsonCheck()
{}

QList<Message> JsonCheck::operator ()(JsonSchema *schema)
{
    QTC_ASSERT(schema, return QList<Message>());

    m_schema = schema;

    m_analysis.push(AnalysisData());
    processSchema(m_doc->ast());
    const AnalysisData &analysis = m_analysis.pop();

    return analysis.m_messages;
}

bool JsonCheck::preVisit(Node *ast)
{
    if (!m_firstLoc.isValid()) {
        if (ExpressionNode *expr = ast->expressionCast())
            m_firstLoc = expr->firstSourceLocation();
    }

    m_analysis.push(AnalysisData());

    return true;
}

void JsonCheck::postVisit(Node *)
{
    const AnalysisData &previous = m_analysis.pop();
    if (previous.m_messages.isEmpty())
        analysis()->m_hasMatch = true;
    else
        analysis()->m_messages.append(previous.m_messages);
    analysis()->m_ranking += previous.m_ranking;
}

bool JsonCheck::visit(ObjectLiteral *ast)
{
    if (!proceedCheck(JsonValue::Object, ast->lbraceToken))
        return false;

    analysis()->boostRanking();

    const QStringList &properties = m_schema->properties();
    if (properties.isEmpty())
        return false;

    QSet<QString> propertiesFound;
    for (PropertyNameAndValueList *it = ast->properties; it; it = it->next) {
        StringLiteralPropertyName *literalName = cast<StringLiteralPropertyName *>(it->name);
        if (literalName) {
            const QString &propertyName = literalName->id.toString();
            if (m_schema->hasPropertySchema(propertyName)) {
                analysis()->boostRanking();
                propertiesFound.insert(propertyName);
                // Sec. 5.2: "... each property definition's value MUST be a schema..."
                m_schema->enterNestedPropertySchema(propertyName);
                processSchema(it->value);
                m_schema->leaveNestedSchema();
            } else {
                analysis()->m_messages.append(Message(ErrInvalidPropertyName,
                                                      literalName->firstSourceLocation(),
                                                      propertyName, QString(),
                                                      false));
            }
        }  else {
            analysis()->m_messages.append(Message(ErrStringValueExpected,
                                                  it->name->firstSourceLocation(),
                                                  QString(), QString(),
                                                  false));
        }
    }

    QStringList missing;
    foreach (const QString &property, properties) {
        if (!propertiesFound.contains(property)) {
            m_schema->enterNestedPropertySchema(property);
            if (m_schema->required())
                missing.append(property);
            m_schema->leaveNestedSchema();
        }
    }
    if (!missing.isEmpty()) {
        analysis()->m_messages.append(Message(ErrMissingRequiredProperty,
                                              ast->firstSourceLocation(),
                                              missing.join(QLatin1String(", ")),
                                              QString(),
                                              false));
    } else {
        analysis()->boostRanking();
    }

    return false;
}

bool JsonCheck::visit(ArrayLiteral *ast)
{
    if (!proceedCheck(JsonValue::Array, ast->firstSourceLocation()))
        return false;

    analysis()->boostRanking();

    if (m_schema->hasItemSchema()) {
        // Sec. 5.5: "When this attribute value is a schema... all the items in the array MUST
        // be valid according to the schema."
        m_schema->enterNestedItemSchema();
        for (ElementList *element = ast->elements; element; element = element->next)
            processSchema(element->expression);
        m_schema->leaveNestedSchema();
    } else if (m_schema->hasItemArraySchema()) {
        // Sec. 5.5: "When this attribute value is an array of schemas... each position in the
        // instance array MUST conform to the schema in the corresponding position for this array."
        int current = 0;
        const int arraySize = m_schema->itemArraySchemaSize();
        for (ElementList *element = ast->elements; element; element = element->next, ++current) {
            if (current < arraySize) {
                if (m_schema->maybeEnterNestedArraySchema(current)) {
                    processSchema(element->expression);
                    m_schema->leaveNestedSchema();
                } else {
                    Node::accept(element->expression, this);
                }
            } else {
                // TODO: Handle additionalItems.
            }
        }
        if (current < arraySize
                || (current > arraySize && !m_schema->hasAdditionalItems())) {
            analysis()->m_messages.append(Message(ErrInvalidArrayValueLength,
                                                  ast->firstSourceLocation(),
                                                  QString::number(arraySize), QString(), false));
        }
    }

    return false;
}

bool JsonCheck::visit(NullExpression *ast)
{
    if (proceedCheck(JsonValue::Null, ast->firstSourceLocation()))
        return false;

    analysis()->boostRanking();

    return false;
}

bool JsonCheck::visit(TrueLiteral *ast)
{
    if (!proceedCheck(JsonValue::Boolean, ast->firstSourceLocation()))
        return false;

    analysis()->boostRanking();

    return false;
}

bool JsonCheck::visit(FalseLiteral *ast)
{
    if (!proceedCheck(JsonValue::Boolean, ast->firstSourceLocation()))
        return false;

    analysis()->boostRanking();

    return false;
}

bool JsonCheck::visit(NumericLiteral *ast)
{
    double dummy;
    if (std::abs(std::modf(ast->value, &dummy)) > 0.000000001) {
        if (!proceedCheck(JsonValue::Double, ast->firstSourceLocation()))
            return false;
    } else if (!proceedCheck(JsonValue::Int, ast->firstSourceLocation())) {
        return false;
    }

    analysis()->boostRanking();

    if (m_schema->hasMinimum()) {
        double minValue = m_schema->minimum();
        if (ast->value < minValue) {
            analysis()->m_messages.append(Message(ErrLargerNumberValueExpected,
                                                  ast->firstSourceLocation(),
                                                  QString::number(minValue), QString(), false));
        } else if (m_schema->hasExclusiveMinimum()
                   && std::abs(ast->value - minValue) > 0.000000001) {
            analysis()->m_messages.append(Message(ErrMinimumNumberValueIsExclusive,
                                                  ast->firstSourceLocation(),
                                                  QString(), QString(), false));
        } else {
            analysis()->boostRanking();
        }
    }

    if (m_schema->hasMaximum()) {
        double maxValue = m_schema->maximum();
        if (ast->value > maxValue) {
            analysis()->m_messages.append(Message(ErrSmallerNumberValueExpected,
                                                  ast->firstSourceLocation(),
                                                  QString::number(maxValue), QString(), false));
        } else if (m_schema->hasExclusiveMaximum()) {
            analysis()->m_messages.append(Message(ErrMaximumNumberValueIsExclusive,
                                                  ast->firstSourceLocation(),
                                                  QString(), QString(), false));
        } else {
            analysis()->boostRanking();
        }
    }

    return false;
}

bool JsonCheck::visit(StringLiteral *ast)
{
    if (!proceedCheck(JsonValue::String, ast->firstSourceLocation()))
        return false;

    analysis()->boostRanking();

    const QString &literal = ast->value.toString();

    const QString &pattern = m_schema->pattern();
    if (!pattern.isEmpty()) {
        QRegExp regExp(pattern);
        if (regExp.indexIn(literal) == -1) {
            analysis()->m_messages.append(Message(ErrInvalidStringValuePattern,
                                                  ast->firstSourceLocation(),
                                                  QString(), QString(), false));
            return false;
        }
        analysis()->boostRanking(3); // Treat string patterns with higher weight.
    }

    int expectedLength = m_schema->minimumLength();
    if (expectedLength != -1) {
        if (literal.length() < expectedLength) {
            analysis()->m_messages.append(Message(ErrLongerStringValueExpected,
                                                  ast->firstSourceLocation(),
                                                  QString::number(expectedLength), QString(),
                                                  false));
        } else {
            analysis()->boostRanking();
        }
    }

    expectedLength = m_schema->maximumLength();
    if (expectedLength != -1) {
        if (literal.length() > expectedLength) {
            analysis()->m_messages.append(Message(ErrShorterStringValueExpected,
                                                  ast->firstSourceLocation(),
                                                  QString::number(expectedLength), QString(),
                                                  false));
        } else {
            analysis()->boostRanking();
        }
    }

    return false;
}

static QString formatExpectedTypes(QStringList all)
{
    all.removeDuplicates();
    return all.join(QLatin1String(", or "));
}

void JsonCheck::processSchema(Node *ast)
{
    if (m_schema->hasTypeSchema()) {
        m_schema->enterNestedTypeSchema();
        processSchema(ast);
        m_schema->leaveNestedSchema();
    } else if (m_schema->hasUnionSchema()) {
        // Sec. 5.1: "... value is valid if it is of the same type as one of the simple
        // type definitions, or valid by one of the schemas, in the array."
        int bestRank = 0;
        QList<StaticAnalysis::Message> bestErrorGuess;
        int current = 0;
        const int unionSize = m_schema->unionSchemaSize();
        m_analysis.push(AnalysisData());
        for (; current < unionSize; ++current) {
            if (m_schema->maybeEnterNestedUnionSchema(current)) {
                processSchema(ast);
                m_schema->leaveNestedSchema();
            } else {
                Node::accept(ast, this);
            }
            if (analysis()->m_hasMatch)
                break;

            if (analysis()->m_ranking >= bestRank) {
                bestRank = analysis()->m_ranking;
                bestErrorGuess = analysis()->m_messages;
            }
            analysis()->m_ranking = 0;
            analysis()->m_messages.clear();
        }
        m_analysis.pop();

        if (current == unionSize) {
            // When we don't have a match for a union typed schema, we try to "guess" which
            // particular item from the union the user tried to represent. The one with the best
            // ranking wins.
            if (bestRank > 0) {
                analysis()->m_messages.append(bestErrorGuess);
            } else {
                analysis()->m_messages.append(Message(ErrDifferentValueExpected,
                                                      ast->firstSourceLocation(),
                                                      formatExpectedTypes(m_schema->validTypes()),
                                                      QString(),
                                                      false));
            }
        }
    } else {
        Node::accept(ast, this);
    }
}

bool JsonCheck::proceedCheck(JsonValue::Kind kind, const SourceLocation &location)
{
    if (!m_firstLoc.isValid())
        return false;

    if (!m_schema->isTypeConstrained())
        return false;

    if (!m_schema->acceptsType(JsonValue::kindToString(kind))) {
        analysis()->m_messages.append(Message(ErrDifferentValueExpected,
                                              location,
                                              formatExpectedTypes(m_schema->validTypes()),
                                              QString(),
                                              false));
        return false;
    }

    return true;
}

JsonCheck::AnalysisData *JsonCheck::analysis()
{
    return &m_analysis.top();
}
