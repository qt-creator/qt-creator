/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "qmljspropertyreader.h"
#include "qmljsdocument.h"
#include <qmljs/parser/qmljsast_p.h>

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

static inline QString textAt(const Document* doc,
                                  const SourceLocation &from,
                                  const SourceLocation &to)
{
    return doc->source().mid(from.offset, to.end() - from.begin());
}

static inline int propertyType(const QString &typeName)
{
    if (typeName == QLatin1String("bool"))
        return QMetaType::type("bool");
    else if (typeName == QLatin1String("color"))
        return QMetaType::type("QColor");
    else if (typeName == QLatin1String("date"))
        return QMetaType::type("QDate");
    else if (typeName == QLatin1String("int"))
        return QMetaType::type("int");
    else if (typeName == QLatin1String("real"))
        return QMetaType::type("double");
    else if (typeName == QLatin1String("double"))
        return QMetaType::type("double");
    else if (typeName == QLatin1String("string"))
        return QMetaType::type("QString");
    else if (typeName == QLatin1String("url"))
        return QMetaType::type("QUrl");
    else if (typeName == QLatin1String("variant"))
        return QMetaType::type("QVariant");
    else
        return -1;
}

static inline QString flatten(UiQualifiedId *qualifiedId)
{
    QString result;

    for (UiQualifiedId *iter = qualifiedId; iter; iter = iter->next) {
        if (!iter->name)
            continue;

        if (!result.isEmpty())
            result.append(QLatin1Char('.'));

        result.append(iter->name->asString());
    }
    return result;
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

bool isEnum(AST::ExpressionStatement *ast)
{
    if (!ast)
        return false;

    if (FieldMemberExpression *memberExpr = cast<AST::FieldMemberExpression*>(ast->expression))
        return isEnum(memberExpr->base);
    else if (cast<IdentifierExpression*>(ast->expression))
        return true;
    else
        return false;
}

static bool isEnum(AST::Statement *ast)
{
    if (!ast)
        return false;

    if (ExpressionStatement *exprStmt = cast<ExpressionStatement*>(ast)) {
        return isEnum(exprStmt->expression);
    }
    return false;
}

} // anonymous namespace

PropertyReader::PropertyReader(Document *doc, AST::UiObjectInitializer *ast)
{
    for (UiObjectMemberList *members = ast->members; members; members = members->next) {
        UiObjectMember *member = members->member;

        if (UiScriptBinding *property = AST::cast<UiScriptBinding *>(member)) {
            if (!property->qualifiedId)
                continue; // better safe than sorry.
            const QString propertyName = flatten(property->qualifiedId);
            const QString astValue = textAt(doc,
                              property->statement->firstSourceLocation(),
                              property->statement->lastSourceLocation());
            if (isLiteralValue(property)) {
                m_properties.insert(propertyName, QVariant(deEscape(stripQuotes(astValue))));
            } else if (isEnum(property->statement)) { //enum
                 m_properties.insert(propertyName, QVariant(astValue));
            }
        } else if (UiObjectDefinition *objectDefinition = cast<UiObjectDefinition *>(member)) { //font { bold: true }
            const QString propertyName = objectDefinition->qualifiedTypeNameId->name->asString();
            if (!propertyName.isEmpty() && !propertyName.at(0).isUpper()) {
                for (UiObjectMemberList *iter = objectDefinition->initializer->members; iter; iter = iter->next) {
                    UiObjectMember *objectMember = iter->member;
                    if (UiScriptBinding *property = cast<UiScriptBinding *>(objectMember)) {
                        const QString propertyNamePart2 = flatten(property->qualifiedId);
                        const QString astValue = textAt(doc,
                            property->statement->firstSourceLocation(),
                            property->statement->lastSourceLocation());
                        if (isLiteralValue(property)) {
                            m_properties.insert(propertyName + '.' + propertyNamePart2, QVariant(deEscape(stripQuotes(astValue))));
                        } else if (isEnum(property->statement)) { //enum
                            m_properties.insert(propertyName + '.' + propertyNamePart2, QVariant(astValue));
                        }
                    }
                }
            }
        }
    }
}
} //QmlJS
