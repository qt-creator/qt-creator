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

#include "qmljspropertyreader.h"
#include "qmljsdocument.h"
#include <qmljs/parser/qmljsast_p.h>
#include <qmljs/qmljsutils.h>

#include <QLinearGradient>

namespace QmlJS {

using namespace AST;

namespace {

static inline QString deEscape(const QString &value)
{
    QString result = value;

    result.replace(QLatin1String("\\\\"), QLatin1String("\\"));
    result.replace(QLatin1String("\\\""), QLatin1String("\""));
    result.replace(QLatin1String("\\\t"), QLatin1String("\t"));
    result.replace(QLatin1String("\\\r"), QLatin1String("\r"));
    result.replace(QLatin1String("\\\n"), QLatin1String("\n"));

    return result;
}

static inline QString stripQuotes(const QString &str)
{
    if ((str.startsWith(QLatin1Char('"')) && str.endsWith(QLatin1Char('"')))
            || (str.startsWith(QLatin1Char('\'')) && str.endsWith(QLatin1Char('\''))))
        return str.mid(1, str.length() - 2);

    return str;
}

static bool isLiteralValue(ExpressionNode *expr)
{
    if (cast<NumericLiteral*>(expr))
        return true;
    else if (cast<StringLiteral*>(expr))
        return true;
    else if (UnaryPlusExpression *plusExpr = cast<UnaryPlusExpression*>(expr))
        return isLiteralValue(plusExpr->expression);
    else if (UnaryMinusExpression *minusExpr = cast<UnaryMinusExpression*>(expr))
        return isLiteralValue(minusExpr->expression);
    else if (cast<TrueLiteral*>(expr))
        return true;
    else if (cast<FalseLiteral*>(expr))
        return true;
    else
        return false;
}

static inline bool isLiteralValue(UiScriptBinding *script)
{
    if (!script || !script->statement)
        return false;

    ExpressionStatement *exprStmt = cast<ExpressionStatement *>(script->statement);
    if (exprStmt)
        return isLiteralValue(exprStmt->expression);
    else
        return false;
}

static inline QString textAt(const Document::Ptr doc,
                                  const SourceLocation &from,
                                  const SourceLocation &to)
{
    return doc->source().mid(from.offset, to.end() - from.begin());
}

static bool isEnum(AST::Statement *ast);

bool isEnum(AST::ExpressionNode *ast)
{
    if (!ast)
        return false;

    if (FieldMemberExpression *memberExpr = cast<AST::FieldMemberExpression*>(ast))
        return isEnum(memberExpr->base);
    else if (cast<IdentifierExpression*>(ast))
        return true;
    else
        return false;
}

static bool isEnum(AST::Statement *ast)
{
    if (!ast)
        return false;

    if (ExpressionStatement *exprStmt = cast<ExpressionStatement*>(ast))
        return isEnum(exprStmt->expression);
    return false;
}

static QString cleanupSemicolon(const QString &str)
{
    QString out = str;
    while (out.endsWith(QLatin1Char(';')))
        out.chop(1);
    return out;
}

} // anonymous namespace

PropertyReader::PropertyReader(Document::Ptr doc, AST::UiObjectInitializer *ast)
{
    m_ast = ast;
    m_doc = doc;

    if (!m_doc)
        return;

    for (UiObjectMemberList *members = ast->members; members; members = members->next) {
        UiObjectMember *member = members->member;

        if (UiScriptBinding *property = AST::cast<UiScriptBinding *>(member)) {
            if (!property->qualifiedId)
                continue; // better safe than sorry.
            const QString propertyName = toString(property->qualifiedId);
            const QString astValue = cleanupSemicolon(textAt(doc,
                                                             property->statement->firstSourceLocation(),
                                                             property->statement->lastSourceLocation()));
            m_astPropertyValues.insert(propertyName, astValue);
            if (isLiteralValue(property)) {
                m_properties.insert(propertyName, QVariant(deEscape(stripQuotes(astValue))));
            } else if (isEnum(property->statement)) { //enum
                m_properties.insert(propertyName, QVariant(astValue));
                m_bindingOrEnum.append(propertyName);
            }
        } else if (UiObjectDefinition *objectDefinition = cast<UiObjectDefinition *>(member)) { //font { bold: true }
            const QString propertyName = objectDefinition->qualifiedTypeNameId->name.toString();
            if (!propertyName.isEmpty() && !propertyName.at(0).isUpper()) {
                for (UiObjectMemberList *iter = objectDefinition->initializer->members; iter; iter = iter->next) {
                    UiObjectMember *objectMember = iter->member;
                    if (UiScriptBinding *property = cast<UiScriptBinding *>(objectMember)) {
                        const QString propertyNamePart2 = toString(property->qualifiedId);
                        const QString astValue = cleanupSemicolon(textAt(doc,
                                                                         property->statement->firstSourceLocation(),
                                                                         property->statement->lastSourceLocation()));
                        m_astPropertyValues.insert(propertyName, astValue);
                        if (isLiteralValue(property)) {
                            m_properties.insert(propertyName + QLatin1Char('.') + propertyNamePart2,
                                                QVariant(deEscape(stripQuotes(astValue))));
                        } else if (isEnum(property->statement)) { //enum
                            m_properties.insert(propertyName + QLatin1Char('.') + propertyNamePart2, QVariant(astValue));
                            m_bindingOrEnum.append(propertyName + QLatin1Char('.') + propertyNamePart2);
                        }
                    }
                }
            }
        } else if (UiObjectBinding* objectBinding = cast<UiObjectBinding *>(member)) {
            UiObjectInitializer *initializer = objectBinding->initializer;
            const QString astValue = cleanupSemicolon(textAt(doc,
                              initializer->lbraceToken,
                              initializer->rbraceToken));
            const QString propertyName = objectBinding->qualifiedId->name.toString();
            m_properties.insert(propertyName, QVariant(astValue));
            m_astPropertyValues.insert(propertyName, astValue);
        }
    }
}

QLinearGradient PropertyReader::parseGradient(const QString &propertyName,  bool *isBound) const
{
    if (!m_doc)
        return QLinearGradient();

    *isBound = false;

    for (UiObjectMemberList *members = m_ast->members; members; members = members->next) {
        UiObjectMember *member = members->member;

        if (UiObjectBinding* objectBinding = cast<UiObjectBinding *>(member)) {
            UiObjectInitializer *initializer = objectBinding->initializer;
            const QString objectPropertyName = objectBinding->qualifiedId->name.toString();
            const QStringRef typeName = objectBinding->qualifiedTypeNameId->name;
            if (objectPropertyName == propertyName && typeName.contains(QLatin1String("Gradient"))) {
                QLinearGradient gradient;
                QVector<QGradientStop> stops;

                for (UiObjectMemberList *members = initializer->members; members; members = members->next) {
                    UiObjectMember *member = members->member;
                    if (UiObjectDefinition *objectDefinition = cast<UiObjectDefinition *>(member)) {
                        PropertyReader localParser(m_doc, objectDefinition->initializer);
                        if (localParser.hasProperty(QLatin1String("color")) && localParser.hasProperty(QLatin1String("position"))) {
                            QColor color = QmlJS::toQColor(localParser.readProperty(QLatin1String("color")).toString());
                            qreal position = localParser.readProperty(QLatin1String("position")).toReal();
                            if (localParser.isBindingOrEnum(QLatin1String("color")) || localParser.isBindingOrEnum(QLatin1String("position")))
                                *isBound = true;
                            stops.append( QPair<qreal, QColor>(position, color));
                        }
                    }
                }

                gradient.setStops(stops);
                return gradient;
            }
        }
    }
    return QLinearGradient();
}

} //QmlJS
