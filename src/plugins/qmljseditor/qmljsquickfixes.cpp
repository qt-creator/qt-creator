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

#include "qmljsquickfix.h"
#include "qmljscomponentfromobjectdef.h"
#include "qmljseditor.h"
#include "qmljsrefactoringchanges.h"

#include <extensionsystem/iplugin.h>
#include <extensionsystem/pluginmanager.h>
#include <qmljs/parser/qmljsast_p.h>

#include <QtGui/QApplication>

using namespace QmlJS;
using namespace QmlJS::AST;
using namespace QmlJSEditor;
using namespace QmlJSEditor::Internal;
using namespace TextEditor;
using TextEditor::RefactoringChanges;

namespace {

class SplitInitializerOp: public QmlJSQuickFixFactory
{
public:
    virtual QList<QmlJSQuickFixOperation::Ptr> match(const QmlJSQuickFixState &state)
    {
        QList<QmlJSQuickFixOperation::Ptr> result;

        UiObjectInitializer *objectInitializer = 0;

        const int pos = state.textCursor().position();

        if (QmlJS::AST::Node *member = state.semanticInfo().declaringMember(pos)) {
            if (QmlJS::AST::UiObjectBinding *b = QmlJS::AST::cast<QmlJS::AST::UiObjectBinding *>(member)) {
                if (b->initializer->lbraceToken.startLine == b->initializer->rbraceToken.startLine)
                    objectInitializer = b->initializer;

            } else if (QmlJS::AST::UiObjectDefinition *b = QmlJS::AST::cast<QmlJS::AST::UiObjectDefinition *>(member)) {
                if (b->initializer->lbraceToken.startLine == b->initializer->rbraceToken.startLine)
                    objectInitializer = b->initializer;
            }
        }

        if (objectInitializer)
            result.append(QSharedPointer<QmlJSQuickFixOperation>(new Operation(state, objectInitializer)));
        return result;
    }

private:
    class Operation: public QmlJSQuickFixOperation
    {
        UiObjectInitializer *_objectInitializer;

    public:
        Operation(const QmlJSQuickFixState &state, UiObjectInitializer *objectInitializer)
            : QmlJSQuickFixOperation(state, 0)
            , _objectInitializer(objectInitializer)
        {
            setDescription(QApplication::translate("QmlJSEditor::QuickFix",
                                                   "Split initializer"));
        }

        virtual void performChanges(TextEditor::RefactoringFile *currentFile, QmlJSRefactoringChanges *)
        {
            Q_ASSERT(_objectInitializer != 0);

            Utils::ChangeSet changes;

            for (QmlJS::AST::UiObjectMemberList *it = _objectInitializer->members; it; it = it->next) {
                if (QmlJS::AST::UiObjectMember *member = it->member) {
                    const QmlJS::AST::SourceLocation loc = member->firstSourceLocation();

                    // insert a newline at the beginning of this binding
                    changes.insert(startPosition(loc), QLatin1String("\n"));
                }
            }

            // insert a newline before the closing brace
            changes.insert(startPosition(_objectInitializer->rbraceToken),
                           QLatin1String("\n"));

            currentFile->change(changes);
            currentFile->indent(range(startPosition(_objectInitializer->lbraceToken),
                                      startPosition(_objectInitializer->rbraceToken)));
        }
    };
};

} // end of anonymous namespace

void QmlJSQuickFixCollector::registerQuickFixes(ExtensionSystem::IPlugin *plugIn)
{
    plugIn->addAutoReleasedObject(new SplitInitializerOp);
    plugIn->addAutoReleasedObject(new ComponentFromObjectDef);
}
