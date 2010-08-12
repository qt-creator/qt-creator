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

using namespace QmlJS::AST;
using namespace QmlJSEditor;
using namespace QmlJSEditor::Internal;

namespace {

static QString getIdProperty(UiObjectDefinition *def)
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

class Operation: public QmlJSQuickFixOperation
{
    UiObjectDefinition *m_objDef;
    QString m_componentName;

public:
    Operation(const QmlJSQuickFixState &state, UiObjectDefinition *objDef)
        : QmlJSQuickFixOperation(state, 0)
        , m_objDef(objDef)
    {
        Q_ASSERT(m_objDef != 0);

        m_componentName = getIdProperty(m_objDef);
        m_componentName[0] = m_componentName.at(0).toUpper();

        setDescription(QCoreApplication::translate("QmlJSEditor::ComponentFromObjectDef",
                                                   "Move Component into '%1.qml'").arg(m_componentName));
    }

    virtual void performChanges(TextEditor::RefactoringFile *currentFile, QmlJSRefactoringChanges *refactoring)
    {
        const QString newFileName = QFileInfo(fileName()).path()
                + QDir::separator() + m_componentName + QLatin1String(".qml");

        QString imports;
        UiProgram *prog = state().semanticInfo().document->qmlProgram();
        if (prog && prog->imports) {
            const int start = startPosition(prog->imports->firstSourceLocation());
            const int end = startPosition(prog->members->member->firstSourceLocation());
            imports = state().textOf(start, end);
        }

        const int start = startPosition(m_objDef->firstSourceLocation());
        const int end = startPosition(m_objDef->lastSourceLocation());
        const QString txt = imports + state().textOf(start, end)
                + QLatin1String("}\n");

        // stop if we can't create the new file
        if (!refactoring->createFile(newFileName, txt))
            return;

        Utils::ChangeSet changes;
        changes.replace(start, end, m_componentName + QLatin1String(" {\n"));
        currentFile->change(changes);
        currentFile->indent(range(start, end + 1));
    }
};

} // end of anonymous namespace

QList<QmlJSQuickFixOperation::Ptr> ComponentFromObjectDef::match(const QmlJSQuickFixState &state)
{
    QList<QmlJSQuickFixOperation::Ptr> result;
    const int pos = state.textCursor().position();

    QList<Node *> path = state.semanticInfo().astPath(pos);
    for (int i = path.size() - 1; i >= 0; --i) {
        Node *node = path.at(i);
        if (UiObjectDefinition *objDef = cast<UiObjectDefinition *>(node)) {
             // check that the node is not the root node
            if (i > 0 && !cast<UiProgram*>(path.at(i - 1))) {
                if (!getIdProperty(objDef).isEmpty()) {
                    result.append(QmlJSQuickFixOperation::Ptr(new Operation(state, objDef)));
                    return result;
                }
            }
        }
    }

    return result;
}
