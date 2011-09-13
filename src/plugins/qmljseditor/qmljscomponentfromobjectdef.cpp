/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "qmljscomponentfromobjectdef.h"
#include "qmljscomponentnamedialog.h"
#include "qmljsquickfixassist.h"

#include <coreplugin/ifile.h>

#include <qmljs/parser/qmljsast_p.h>
#include <qmljs/qmljsdocument.h>
#include <qmljstools/qmljsrefactoringchanges.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>

using namespace QmlJS::AST;
using namespace QmlJSEditor;
using namespace QmlJSEditor::Internal;
using namespace QmlJSTools;

namespace {

static QString toString(Statement *statement)
{
    ExpressionStatement *expStmt = cast<ExpressionStatement *>(statement);
    if (!expStmt)
        return QString();
    if (IdentifierExpression *idExp = cast<IdentifierExpression *>(expStmt->expression)) {
        return idExp->name.toString();
    } else if (StringLiteral *strExp = cast<StringLiteral *>(expStmt->expression)) {
        return strExp->value.toString();
    }
    return QString();
}

static QString getIdProperty(UiObjectDefinition *def)
{
    QString objectName;

    if (def && def->initializer) {
        for (UiObjectMemberList *iter = def->initializer->members; iter; iter = iter->next) {
            if (UiScriptBinding *script = cast<UiScriptBinding*>(iter->member)) {
                if (!script->qualifiedId)
                    continue;
                if (script->qualifiedId->next)
                    continue;
                if (!script->qualifiedId->name.isEmpty()) {
                    if (script->qualifiedId->name == QLatin1String("id"))
                        return toString(script->statement);
                    if (script->qualifiedId->name == QLatin1String("objectName"))
                        objectName = toString(script->statement);
                }
            }
        }
    }

    return objectName;
}

class Operation: public QmlJSQuickFixOperation
{
    UiObjectDefinition *m_objDef;
    QString m_idName, m_componentName;

public:
    Operation(const QSharedPointer<const QmlJSQuickFixAssistInterface> &interface,
              UiObjectDefinition *objDef)
        : QmlJSQuickFixOperation(interface, 0)
        , m_objDef(objDef)
    {
        Q_ASSERT(m_objDef != 0);

        m_idName = getIdProperty(m_objDef);

        if (m_idName.isEmpty()) {
            setDescription(QCoreApplication::translate("QmlJSEditor::ComponentFromObjectDef",
                                                       "Move Component into separate file"));
        } else {
            m_componentName = m_idName;
            m_componentName[0] = m_componentName.at(0).toUpper();
            setDescription(QCoreApplication::translate("QmlJSEditor::ComponentFromObjectDef",
                                                       "Move Component into '%1.qml'").arg(m_componentName));
        }
    }

    virtual void performChanges(QmlJSRefactoringFilePtr currentFile,
                                const QmlJSRefactoringChanges &refactoring)
    {
        QString componentName = m_componentName;
        QString path = QFileInfo(fileName()).path();
        if (componentName.isEmpty()) {
            ComponentNameDialog::go(&componentName, &path, assistInterface()->widget());
        }

        if (componentName.isEmpty() || path.isEmpty())
            return;

        const QString newFileName = path + QDir::separator() + componentName
                + QLatin1String(".qml");

        QString imports;
        UiProgram *prog = currentFile->qmljsDocument()->qmlProgram();
        if (prog && prog->imports) {
            const int start = currentFile->startOf(prog->imports->firstSourceLocation());
            const int end = currentFile->startOf(prog->members->member->firstSourceLocation());
            imports = currentFile->textOf(start, end);
        }

        const int start = currentFile->startOf(m_objDef->firstSourceLocation());
        const int end = currentFile->startOf(m_objDef->lastSourceLocation());
        const QString txt = imports + currentFile->textOf(start, end)
                + QLatin1String("}\n");

        // stop if we can't create the new file
        if (!refactoring.createFile(newFileName, txt))
            return;

        QString replacement = componentName + QLatin1String(" {\n");
        if (!m_idName.isEmpty())
                replacement += QLatin1String("id: ") + m_idName
                        + QLatin1Char('\n');

        Utils::ChangeSet changes;
        changes.replace(start, end, replacement);
        currentFile->setChangeSet(changes);
        currentFile->appendIndentRange(Range(start, end + 1));
        currentFile->apply();
    }
};

} // end of anonymous namespace


QList<QmlJSQuickFixOperation::Ptr> ComponentFromObjectDef::match(
    const QSharedPointer<const QmlJSQuickFixAssistInterface> &interface)
{
    const int pos = interface->currentFile()->cursor().position();

    QList<Node *> path = interface->semanticInfo().rangePath(pos);
    for (int i = path.size() - 1; i >= 0; --i) {
        Node *node = path.at(i);
        if (UiObjectDefinition *objDef = cast<UiObjectDefinition *>(node)) {
            if (!interface->currentFile()->isCursorOn(objDef->qualifiedTypeNameId))
                return noResult();
             // check that the node is not the root node
            if (i > 0 && !cast<UiProgram*>(path.at(i - 1))) {
                return singleResult(new Operation(interface, objDef));
            }
        }
    }

    return noResult();
}
