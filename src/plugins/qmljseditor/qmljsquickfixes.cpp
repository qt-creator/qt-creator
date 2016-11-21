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
using namespace QmlJSTools;
using TextEditor::RefactoringChanges;

namespace QmlJSEditor {

using namespace Internal;

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
    void match(const QmlJSQuickFixInterface &interface, QuickFixOperations &result)
    {
        UiObjectInitializer *objectInitializer = 0;

        const int pos = interface->currentFile()->cursor().position();

        if (Node *member = interface->semanticInfo().rangeAt(pos)) {
            if (UiObjectBinding *b = AST::cast<UiObjectBinding *>(member)) {
                if (b->initializer->lbraceToken.startLine == b->initializer->rbraceToken.startLine)
                    objectInitializer = b->initializer;

            } else if (UiObjectDefinition *b = AST::cast<UiObjectDefinition *>(member)) {
                if (b->initializer->lbraceToken.startLine == b->initializer->rbraceToken.startLine)
                    objectInitializer = b->initializer;
            }
        }

        if (objectInitializer)
            result << new Operation(interface, objectInitializer);
    }

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

        void performChanges(QmlJSRefactoringFilePtr currentFile,
                                    const QmlJSRefactoringChanges &)
        {
            Q_ASSERT(_objectInitializer != 0);

            Utils::ChangeSet changes;

            for (UiObjectMemberList *it = _objectInitializer->members; it; it = it->next) {
                if (UiObjectMember *member = it->member) {
                    const SourceLocation loc = member->firstSourceLocation();

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
    void match(const QmlJSQuickFixInterface &interface, QuickFixOperations &result)
    {
        const QList<StaticAnalysis::Message> &messages = interface->semanticInfo().staticAnalysisMessages;

        foreach (const StaticAnalysis::Message &message, messages) {
            if (interface->currentFile()->isCursorOn(message.location)) {
                result << new Operation(interface, message);
                return;
            }
        }
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
            changes.insert(insertLoc, QString::fromLatin1("// %1\n").arg(_message.suppressionString()));
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

} // namespace QmlJSEditor
