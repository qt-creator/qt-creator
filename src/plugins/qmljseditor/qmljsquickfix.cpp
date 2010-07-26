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
#include "qmljs/parser/qmljsast_p.h"

#include <extensionsystem/iplugin.h>
#include <extensionsystem/pluginmanager.h>

#include <qmljs/qmljsmodelmanagerinterface.h>

#include <QtGui/QApplication>
#include <QtCore/QDebug>

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

    virtual QString description() const
    {
        return QApplication::translate("QmlJSEditor::QuickFix", "Split initializer");
    }

private:
    class Operation: public QmlJSQuickFixOperation
    {
        UiObjectInitializer *_objectInitializer;

    public:
        Operation(const QmlJSQuickFixState &state, UiObjectInitializer *objectInitializer)
            : QmlJSQuickFixOperation(state, 0)
            , _objectInitializer(objectInitializer)
        {}

        virtual void createChanges()
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

            refactoringChanges()->changeFile(fileName(), changes);
            refactoringChanges()->reindent(fileName(),
                                           range(startPosition(_objectInitializer->lbraceToken),
                                                 startPosition(_objectInitializer->rbraceToken)));

        }
    };
};

} // end of anonymous namespace

QmlJSQuickFixState::QmlJSQuickFixState(TextEditor::BaseTextEditor *editor)
    : QuickFixState(editor)
{
}

SemanticInfo QmlJSQuickFixState::semanticInfo() const
{
    return _semanticInfo;
}

Snapshot QmlJSQuickFixState::snapshot() const
{
    return _semanticInfo.snapshot;
}

Document::Ptr QmlJSQuickFixState::document() const
{
    return _semanticInfo.document;
}

unsigned QmlJSQuickFixState::startPosition(const QmlJS::AST::SourceLocation &loc) const
{
    return position(loc.startLine, loc.startColumn);
}

QmlJSQuickFixOperation::QmlJSQuickFixOperation(const QmlJSQuickFixState &state, int priority)
    : QuickFixOperation(priority)
    , _state(state)
    , _refactoringChanges(new QmlJSRefactoringChanges(ExtensionSystem::PluginManager::instance()->getObject<QmlJS::ModelManagerInterface>(),
                                                      state.snapshot()))
{
}

QmlJSQuickFixOperation::~QmlJSQuickFixOperation()
{
}

const QmlJSQuickFixState &QmlJSQuickFixOperation::state() const
{
    return _state;
}

QString QmlJSQuickFixOperation::fileName() const
{
    return state().document()->fileName();
}

QmlJSRefactoringChanges *QmlJSQuickFixOperation::refactoringChanges() const
{
    return _refactoringChanges.data();
}

QmlJSQuickFixFactory::QmlJSQuickFixFactory()
{
}

QmlJSQuickFixFactory::~QmlJSQuickFixFactory()
{
}

QList<QuickFixOperation::Ptr> QmlJSQuickFixFactory::matchingOperations(QuickFixState *state)
{
    if (QmlJSQuickFixState *qmljsState = static_cast<QmlJSQuickFixState *>(state))
        return match(*qmljsState);
    else
        return QList<TextEditor::QuickFixOperation::Ptr>();
}


QmlJSQuickFixCollector::QmlJSQuickFixCollector()
{
}

QmlJSQuickFixCollector::~QmlJSQuickFixCollector()
{
}

bool QmlJSQuickFixCollector::supportsEditor(TextEditor::ITextEditable *editable)
{
    return qobject_cast<QmlJSTextEditor *>(editable->widget()) != 0;
}

TextEditor::QuickFixState *QmlJSQuickFixCollector::initializeCompletion(TextEditor::BaseTextEditor *editor)
{
    if (QmlJSTextEditor *qmljsEditor = qobject_cast<QmlJSTextEditor *>(editor)) {
        const SemanticInfo info = qmljsEditor->semanticInfo();

        if (qmljsEditor->isOutdated()) {
            // outdated
            qWarning() << "TODO: outdated semantic info, force a reparse.";
            return 0;
        }

        QmlJSQuickFixState *state = new QmlJSQuickFixState(editor);
        state->_semanticInfo = info;
        return state;
    }

    return 0;
}

QList<TextEditor::QuickFixFactory *> QmlJSQuickFixCollector::quickFixFactories() const
{
    QList<TextEditor::QuickFixFactory *> results;
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    foreach (QmlJSQuickFixFactory *f, pm->getObjects<QmlJSEditor::QmlJSQuickFixFactory>())
        results.append(f);
    return results;
}

void QmlJSQuickFixCollector::registerQuickFixes(ExtensionSystem::IPlugin *plugIn)
{
    plugIn->addAutoReleasedObject(new SplitInitializerOp);
    plugIn->addAutoReleasedObject(new ComponentFromObjectDef);
}
