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

#include "qmljsutils.h"

#include "parser/qmljsast_p.h"

using namespace QmlJS;
using namespace QmlJS::AST;

/*!
  \namespace QmlJS
  QML and JavaScript language support library
*/

namespace {
class SharedData
{
public:
    SharedData()
    {
        validBuiltinPropertyNames.insert(QLatin1String("action"));
        validBuiltinPropertyNames.insert(QLatin1String("bool"));
        validBuiltinPropertyNames.insert(QLatin1String("color"));
        validBuiltinPropertyNames.insert(QLatin1String("date"));
        validBuiltinPropertyNames.insert(QLatin1String("double"));
        validBuiltinPropertyNames.insert(QLatin1String("enumeration"));
        validBuiltinPropertyNames.insert(QLatin1String("font"));
        validBuiltinPropertyNames.insert(QLatin1String("int"));
        validBuiltinPropertyNames.insert(QLatin1String("list"));
        validBuiltinPropertyNames.insert(QLatin1String("point"));
        validBuiltinPropertyNames.insert(QLatin1String("real"));
        validBuiltinPropertyNames.insert(QLatin1String("rect"));
        validBuiltinPropertyNames.insert(QLatin1String("size"));
        validBuiltinPropertyNames.insert(QLatin1String("string"));
        validBuiltinPropertyNames.insert(QLatin1String("time"));
        validBuiltinPropertyNames.insert(QLatin1String("url"));
        validBuiltinPropertyNames.insert(QLatin1String("var"));
        validBuiltinPropertyNames.insert(QLatin1String("variant")); // obsolete in Qt 5
        validBuiltinPropertyNames.insert(QLatin1String("vector3d"));
        validBuiltinPropertyNames.insert(QLatin1String("alias"));
    }

    QSet<QString> validBuiltinPropertyNames;
};
} // anonymous namespace
Q_GLOBAL_STATIC(SharedData, sharedData)

QColor QmlJS::toQColor(const QString &qmlColorString)
{
    QColor color;
    if (qmlColorString.size() == 9 && qmlColorString.at(0) == QLatin1Char('#')) {
        bool ok;
        const int alpha = qmlColorString.mid(1, 2).toInt(&ok, 16);
        if (ok) {
            QString name(qmlColorString.at(0));
            name.append(qmlColorString.right(6));
            if (QColor::isValidColor(name)) {
                color.setNamedColor(name);
                color.setAlpha(alpha);
            }
        }
    } else {
        if (QColor::isValidColor(qmlColorString))
            color.setNamedColor(qmlColorString);
    }
    return color;
}

QString QmlJS::toString(UiQualifiedId *qualifiedId, QChar delimiter)
{
    QString result;

    for (UiQualifiedId *iter = qualifiedId; iter; iter = iter->next) {
        if (iter != qualifiedId)
            result += delimiter;

        result += iter->name;
    }

    return result;
}


SourceLocation QmlJS::locationFromRange(const SourceLocation &start,
                                        const SourceLocation &end)
{
    return SourceLocation(start.offset,
                          end.end() - start.begin(),
                          start.startLine,
                          start.startColumn);
}

SourceLocation QmlJS::fullLocationForQualifiedId(AST::UiQualifiedId *qualifiedId)
{
    SourceLocation start = qualifiedId->identifierToken;
    SourceLocation end = qualifiedId->identifierToken;

    for (UiQualifiedId *iter = qualifiedId; iter; iter = iter->next) {
        if (iter->identifierToken.isValid())
            end = iter->identifierToken;
    }

    return locationFromRange(start, end);
}

/*!
    \returns the value of the 'id:' binding in \a object
    \param idBinding optional out parameter to get the UiScriptBinding for the id binding
*/
QString QmlJS::idOfObject(Node *object, UiScriptBinding **idBinding)
{
    if (idBinding)
        *idBinding = 0;

    UiObjectInitializer *initializer = initializerOfObject(object);
    if (!initializer) {
        initializer = cast<UiObjectInitializer *>(object);
        if (!initializer)
            return QString();
    }

    for (UiObjectMemberList *iter = initializer->members; iter; iter = iter->next) {
        if (UiScriptBinding *script = cast<UiScriptBinding*>(iter->member)) {
            if (!script->qualifiedId)
                continue;
            if (script->qualifiedId->next)
                continue;
            if (script->qualifiedId->name != QLatin1String("id"))
                continue;
            if (ExpressionStatement *expstmt = cast<ExpressionStatement *>(script->statement)) {
                if (IdentifierExpression *idexp = cast<IdentifierExpression *>(expstmt->expression)) {
                    if (idBinding)
                        *idBinding = script;
                    return idexp->name.toString();
                }
            }
        }
    }

    return QString();
}

/*!
    \returns the UiObjectInitializer if \a object is a UiObjectDefinition or UiObjectBinding, otherwise 0
*/
UiObjectInitializer *QmlJS::initializerOfObject(Node *object)
{
    if (UiObjectDefinition *definition = cast<UiObjectDefinition *>(object))
        return definition->initializer;
    if (UiObjectBinding *binding = cast<UiObjectBinding *>(object))
        return binding->initializer;
    return 0;
}

UiQualifiedId *QmlJS::qualifiedTypeNameId(Node *node)
{
    if (UiObjectBinding *binding = AST::cast<UiObjectBinding *>(node))
        return binding->qualifiedTypeNameId;
    else if (UiObjectDefinition *binding = AST::cast<UiObjectDefinition *>(node))
        return binding->qualifiedTypeNameId;
    return 0;
}

DiagnosticMessage QmlJS::errorMessage(const AST::SourceLocation &loc, const QString &message)
{
    return DiagnosticMessage(DiagnosticMessage::Error, loc, message);
}

bool QmlJS::isValidBuiltinPropertyType(const QString &name)
{
    return sharedData()->validBuiltinPropertyNames.contains(name);
}

