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

#include "qmljscomponentfromobjectdef.h"
#include "qmljsrefactoringchanges.h"

#include <coreplugin/ifile.h>

#include <qmljs/parser/qmljsast_p.h>
#include <qmljs/qmljsdocument.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>

#include <QDebug>

using namespace QmlJS::AST;
using namespace QmlJSEditor::Internal;

ComponentFromObjectDef::ComponentFromObjectDef(TextEditor::BaseTextEditor *editor)
    : QmlJSQuickFixOperation(editor), _objDef(0)
{}

static QString getId(UiObjectDefinition *def)
{
    if (def && def->initializer) {
        for (UiObjectMemberList *iter = def->initializer->members; iter; iter = iter->next) {
            if (UiScriptBinding *script = cast<UiScriptBinding*>(iter->member)) {
                if (!script->qualifiedId)
                    continue;
                if (script->qualifiedId->next)
                    continue;
                if (script->qualifiedId->name) {
                    if (script->qualifiedId->name->asString() == QLatin1String("id")) {
                        ExpressionStatement *expStmt = cast<ExpressionStatement *>(script->statement);
                        if (!expStmt)
                            continue;
                        if (IdentifierExpression *idExp = cast<IdentifierExpression *>(expStmt->expression)) {
                            return idExp->name->asString();
                        } else if (StringLiteral *strExp = cast<StringLiteral *>(expStmt->expression)) {
                            return strExp->value->asString();
                        }
                    }
                }
            }
        }
    }

    return QString();
}

QString ComponentFromObjectDef::description() const
{
    return QCoreApplication::translate("QmlJSEditor::ComponentFromObjectDef",
                                       "Extract Component");
}

void ComponentFromObjectDef::createChangeSet()
{
    Q_ASSERT(_objDef != 0);

    QString componentName = getId(_objDef);
    componentName[0] = componentName.at(0).toUpper();

    const QString path = editor()->file()->fileName();
    const QString fileName = QFileInfo(path).path() + QDir::separator() + componentName + QLatin1String(".qml");

    QString imports;
    UiProgram *prog = semanticInfo().document->qmlProgram();
    if (prog && prog->imports) {
        const int start = position(prog->imports->firstSourceLocation());
        const int end = position(prog->members->member->firstSourceLocation());
        imports = textOf(start, end);
    }

    const int start = position(_objDef->firstSourceLocation());
    const int end = position(_objDef->lastSourceLocation());
    const QString txt = imports + textOf(start, end) + QLatin1String("}\n");

    replace(start, end, componentName + QLatin1String(" {\n"));
    reindent(range(start, end + 1));

    qmljsRefactoringChanges()->createFile(fileName, txt);
    qmljsRefactoringChanges()->reindent(fileName, range(0, txt.length() - 1));
}

int ComponentFromObjectDef::check()
{
    _objDef = 0;
    const int pos = textCursor().position();

    QList<Node *> path = semanticInfo().astPath(pos);
    for (int i = path.size() - 1; i >= 0; --i) {
        Node *node = path.at(i);
        qDebug() << "path component" << i << typeid(*node).name();
        if (UiObjectDefinition *objDef = cast<UiObjectDefinition *>(node)) {
            if (i > 0 && !cast<UiProgram*>(path.at(i - 1))) { // node is not the root node
                if (!getId(objDef).isEmpty()) {
                    _objDef = objDef;
                    return 0;
                }
            }
        }
    }

    return -1;
}
