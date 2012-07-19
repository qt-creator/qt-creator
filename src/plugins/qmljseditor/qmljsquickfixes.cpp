/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#include "qmljsquickfix.h"
#include "qmljscomponentfromobjectdef.h"
#include "qmljswrapinloader.h"
#include "qmljseditor.h"
#include "qmljsquickfixassist.h"

#include <extensionsystem/iplugin.h>
#include <extensionsystem/pluginmanager.h>
#include <qmljs/parser/qmljsast_p.h>
#include <qmljstools/qmljsrefactoringchanges.h>

#include <QApplication>

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
    virtual QList<QmlJSQuickFixOperation::Ptr> match(
        const QSharedPointer<const QmlJSQuickFixAssistInterface> &interface)
    {
        UiObjectInitializer *objectInitializer = 0;

        const int pos = interface->currentFile()->cursor().position();

        if (QmlJS::AST::Node *member = interface->semanticInfo().rangeAt(pos)) {
            if (QmlJS::AST::UiObjectBinding *b = QmlJS::AST::cast<QmlJS::AST::UiObjectBinding *>(member)) {
                if (b->initializer->lbraceToken.startLine == b->initializer->rbraceToken.startLine)
                    objectInitializer = b->initializer;

            } else if (QmlJS::AST::UiObjectDefinition *b = QmlJS::AST::cast<QmlJS::AST::UiObjectDefinition *>(member)) {
                if (b->initializer->lbraceToken.startLine == b->initializer->rbraceToken.startLine)
                    objectInitializer = b->initializer;
            }
        }

        if (objectInitializer)
            return singleResult(new Operation(interface, objectInitializer));
        else
            return noResult();
    }

private:
    class Operation: public QmlJSQuickFixOperation
    {
        UiObjectInitializer *_objectInitializer;

    public:
        Operation(const QSharedPointer<const QmlJSQuickFixAssistInterface> &interface,
                  UiObjectInitializer *objectInitializer)
            : QmlJSQuickFixOperation(interface, 0)
            , _objectInitializer(objectInitializer)
        {
            setDescription(QApplication::translate("QmlJSEditor::QuickFix",
                                                   "Split Initializer"));
        }

        virtual void performChanges(QmlJSRefactoringFilePtr currentFile,
                                    const QmlJSRefactoringChanges &)
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

            currentFile->setChangeSet(changes);
            currentFile->appendIndentRange(Range(currentFile->startOf(_objectInitializer->lbraceToken),
                                      currentFile->startOf(_objectInitializer->rbraceToken)));
            currentFile->apply();
        }
    };
};

/*
  Adds a comment to suppress a static analysis message
*/
class AddAnalysisMessageSuppressionComment: public QmlJSQuickFixFactory
{
    Q_DECLARE_TR_FUNCTIONS(QmlJSEditor::AddAnalysisMessageSuppressionComment)
public:
    virtual QList<QmlJSQuickFixOperation::Ptr> match(
        const QSharedPointer<const QmlJSQuickFixAssistInterface> &interface)
    {
        const QList<StaticAnalysis::Message> &messages = interface->semanticInfo().staticAnalysisMessages;

        foreach (const StaticAnalysis::Message &message, messages) {
            if (interface->currentFile()->isCursorOn(message.location)) {
                return singleResult(new Operation(interface, message));
            }
        }

        return noResult();
    }

private:
    class Operation: public QmlJSQuickFixOperation
    {
        StaticAnalysis::Message _message;

    public:
        Operation(const QSharedPointer<const QmlJSQuickFixAssistInterface> &interface,
                  const StaticAnalysis::Message &message)
            : QmlJSQuickFixOperation(interface, 0)
            , _message(message)
        {
            setDescription(AddAnalysisMessageSuppressionComment::tr("Add a Comment to Suppress This Message"));
        }

        virtual void performChanges(QmlJSRefactoringFilePtr currentFile,
                                    const QmlJSRefactoringChanges &)
        {
            Utils::ChangeSet changes;
            const int insertLoc = _message.location.begin() - _message.location.startColumn + 1;
            changes.insert(insertLoc, QString("// %1\n").arg(_message.suppressionString()));
            currentFile->setChangeSet(changes);
            currentFile->appendIndentRange(Range(insertLoc, insertLoc + 1));
            currentFile->apply();
        }
    };
};

} // end of anonymous namespace

void registerQuickFixes(ExtensionSystem::IPlugin *plugIn)
{
    plugIn->addAutoReleasedObject(new SplitInitializerOp);
    plugIn->addAutoReleasedObject(new ComponentFromObjectDef);
    plugIn->addAutoReleasedObject(new WrapInLoader);
    plugIn->addAutoReleasedObject(new AddAnalysisMessageSuppressionComment);
}
