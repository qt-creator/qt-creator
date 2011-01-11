/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "qmljsquickfix.h"
#include "qmljscomponentfromobjectdef.h"
#include "qmljseditor.h"

#include <extensionsystem/iplugin.h>
#include <extensionsystem/pluginmanager.h>
#include <qmljs/parser/qmljsast_p.h>
#include <qmljstools/qmljsrefactoringchanges.h>

#include <QtGui/QApplication>

using namespace QmlJS;
using namespace QmlJS::AST;
using namespace QmlJSEditor;
using namespace QmlJSEditor::Internal;
using namespace QmlJSTools;
using namespace TextEditor;
using TextEditor::RefactoringChanges;

namespace {

/*
  Reformats a one-line object into a multi-line one, i.e.
    Item { x: 10; y: 20; width: 10 }
  into
    Item {
        x: 10;
        y: 20;
        width: 10
    }
*/
class SplitInitializerOp: public QmlJSQuickFixFactory
{
public:
    virtual QList<QmlJSQuickFixOperation::Ptr> match(const QmlJSQuickFixState &state)
    {
        UiObjectInitializer *objectInitializer = 0;

        const int pos = state.currentFile().cursor().position();

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
            return singleResult(new Operation(state, objectInitializer));
        else
            return noResult();
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

        virtual void performChanges(QmlJSRefactoringFile *currentFile, QmlJSRefactoringChanges *)
        {
            Q_ASSERT(_objectInitializer != 0);

            Utils::ChangeSet changes;

            for (QmlJS::AST::UiObjectMemberList *it = _objectInitializer->members; it; it = it->next) {
                if (QmlJS::AST::UiObjectMember *member = it->member) {
                    const QmlJS::AST::SourceLocation loc = member->firstSourceLocation();

                    // insert a newline at the beginning of this binding
                    changes.insert(currentFile->startOf(loc), QLatin1String("\n"));
                }
            }

            // insert a newline before the closing brace
            changes.insert(currentFile->startOf(_objectInitializer->rbraceToken),
                           QLatin1String("\n"));

            currentFile->change(changes);
            currentFile->indent(Range(currentFile->startOf(_objectInitializer->lbraceToken),
                                      currentFile->startOf(_objectInitializer->rbraceToken)));
        }
    };
};

} // end of anonymous namespace

void QmlJSQuickFixCollector::registerQuickFixes(ExtensionSystem::IPlugin *plugIn)
{
    plugIn->addAutoReleasedObject(new SplitInitializerOp);
    plugIn->addAutoReleasedObject(new ComponentFromObjectDef);
}
