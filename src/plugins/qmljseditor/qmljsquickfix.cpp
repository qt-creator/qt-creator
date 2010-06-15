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

#include <extensionsystem/pluginmanager.h>

#include <qmljs/qmljsmodelmanagerinterface.h>

#include <QtGui/QApplication>
#include <QtCore/QDebug>

using namespace QmlJSEditor;
using namespace QmlJSEditor::Internal;
using TextEditor::RefactoringChanges;

class QmlJSQuickFixState: public TextEditor::QuickFixState
{
public:
    SemanticInfo semanticInfo;
};

namespace {

class SplitInitializerOp: public QmlJSQuickFixOperation {
public:
    SplitInitializerOp(TextEditor::BaseTextEditor *editor)
        : QmlJSQuickFixOperation(editor), _objectInitializer(0)
    {}

    virtual QString description() const
    {
        return QApplication::translate("QmlJSEditor::QuickFix", "Split initializer");
    }

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

    virtual int check()
    {
        _objectInitializer = 0;

        const int pos = textCursor().position();

        if (QmlJS::AST::Node *member = semanticInfo().declaringMember(pos)) {
            if (QmlJS::AST::UiObjectBinding *b = QmlJS::AST::cast<QmlJS::AST::UiObjectBinding *>(member)) {
                if (b->initializer->lbraceToken.startLine == b->initializer->rbraceToken.startLine)
                    _objectInitializer = b->initializer;

            } else if (QmlJS::AST::UiObjectDefinition *b = QmlJS::AST::cast<QmlJS::AST::UiObjectDefinition *>(member)) {
                if (b->initializer->lbraceToken.startLine == b->initializer->rbraceToken.startLine)
                    _objectInitializer = b->initializer;
            }
        }

        if (! _objectInitializer)
            return -1;

        return 0; // very high priority
    }

private:
    QmlJS::AST::UiObjectInitializer *_objectInitializer;
};


} // end of anonymous namespace


QmlJSQuickFixOperation::QmlJSQuickFixOperation(TextEditor::BaseTextEditor *editor)
    : TextEditor::QuickFixOperation(editor)
    , _refactoringChanges(0)
{
}

QmlJSQuickFixOperation::~QmlJSQuickFixOperation()
{
    if (_refactoringChanges)
        delete _refactoringChanges;
}

QmlJS::Document::Ptr QmlJSQuickFixOperation::document() const
{
    return _semanticInfo.document;
}

const QmlJS::Snapshot &QmlJSQuickFixOperation::snapshot() const
{
    return _semanticInfo.snapshot;
}

const SemanticInfo &QmlJSQuickFixOperation::semanticInfo() const
{
    return _semanticInfo;
}

int QmlJSQuickFixOperation::match(TextEditor::QuickFixState *state)
{
    QmlJS::ModelManagerInterface *modelManager = ExtensionSystem::PluginManager::instance()->getObject<QmlJS::ModelManagerInterface>();
    QmlJSQuickFixState *s = static_cast<QmlJSQuickFixState *>(state);
    _semanticInfo = s->semanticInfo;
    if (_refactoringChanges) {
        delete _refactoringChanges;
    }
    _refactoringChanges = new QmlJSRefactoringChanges(modelManager, _semanticInfo.snapshot);
    return check();
}

QString QmlJSQuickFixOperation::fileName() const
{
    return document()->fileName();
}

void QmlJSQuickFixOperation::apply()
{
    _refactoringChanges->apply();
}

QmlJSRefactoringChanges *QmlJSQuickFixOperation::qmljsRefactoringChanges() const
{ return _refactoringChanges; }

RefactoringChanges *QmlJSQuickFixOperation::refactoringChanges() const
{ return qmljsRefactoringChanges(); }

unsigned QmlJSQuickFixOperation::startPosition(const QmlJS::AST::SourceLocation &loc) const
{
    return position(loc.startLine, loc.startColumn);
}

QmlJSQuickFixCollector::QmlJSQuickFixCollector()
{
}

QmlJSQuickFixCollector::~QmlJSQuickFixCollector()
{
}

bool QmlJSQuickFixCollector::supportsEditor(TextEditor::ITextEditable *editable)
{
    if (qobject_cast<QmlJSTextEditor *>(editable->widget()) != 0)
        return true;

    return false;
}

TextEditor::QuickFixState *QmlJSQuickFixCollector::initializeCompletion(TextEditor::ITextEditable *editable)
{
    if (QmlJSTextEditor *editor = qobject_cast<QmlJSTextEditor *>(editable->widget())) {
        const SemanticInfo info = editor->semanticInfo();

        if (editor->isOutdated()) {
            // outdated
            qWarning() << "TODO: outdated semantic info, force a reparse.";
            return 0;
        }

        QmlJSQuickFixState *state = new QmlJSQuickFixState;
        state->semanticInfo = info;
        return state;
    }

    return 0;
}

QList<TextEditor::QuickFixOperation::Ptr> QmlJSQuickFixCollector::quickFixOperations(TextEditor::BaseTextEditor *editor) const
{
    QList<TextEditor::QuickFixOperation::Ptr> quickFixOperations;
    quickFixOperations.append(TextEditor::QuickFixOperation::Ptr(new SplitInitializerOp(editor)));
    quickFixOperations.append(TextEditor::QuickFixOperation::Ptr(new ComponentFromObjectDef(editor)));
    return quickFixOperations;
}
