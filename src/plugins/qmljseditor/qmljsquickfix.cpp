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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "qmljsquickfix.h"
#include "qmljscomponentfromobjectdef.h"
#include "qmljseditor.h"
#include "qmljs/parser/qmljsast_p.h"

#include <extensionsystem/iplugin.h>
#include <extensionsystem/pluginmanager.h>

#include <qmljs/qmljsmodelmanagerinterface.h>

#include <QtCore/QDebug>

using namespace QmlJS;
using namespace QmlJS::AST;
using namespace QmlJSEditor;
using namespace QmlJSEditor::Internal;
using namespace QmlJSTools;
using namespace TextEditor;
using TextEditor::RefactoringChanges;

QmlJSQuickFixState::QmlJSQuickFixState(TextEditor::BaseTextEditorWidget *editor)
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

const QmlJSRefactoringFile QmlJSQuickFixState::currentFile() const
{
    return QmlJSRefactoringFile(editor(), document());
}

QmlJSQuickFixOperation::QmlJSQuickFixOperation(const QmlJSQuickFixState &state, int priority)
    : QuickFixOperation(priority)
    , _state(state)
{
}

QmlJSQuickFixOperation::~QmlJSQuickFixOperation()
{
}

void QmlJSQuickFixOperation::perform()
{
    QmlJSRefactoringChanges refactoring(ExtensionSystem::PluginManager::instance()->getObject<QmlJS::ModelManagerInterface>(),
                                    _state.snapshot());
    QmlJSRefactoringFile current = refactoring.file(fileName());

    performChanges(&current, &refactoring);
}

const QmlJSQuickFixState &QmlJSQuickFixOperation::state() const
{
    return _state;
}

QString QmlJSQuickFixOperation::fileName() const
{
    return state().document()->fileName();
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

QList<QmlJSQuickFixOperation::Ptr> QmlJSQuickFixFactory::noResult()
{
    return QList<QmlJSQuickFixOperation::Ptr>();
}

QList<QmlJSQuickFixOperation::Ptr> QmlJSQuickFixFactory::singleResult(QmlJSQuickFixOperation *operation)
{
    QList<QmlJSQuickFixOperation::Ptr> result;
    result.append(QmlJSQuickFixOperation::Ptr(operation));
    return result;
}

QmlJSQuickFixCollector::QmlJSQuickFixCollector()
{
}

QmlJSQuickFixCollector::~QmlJSQuickFixCollector()
{
}

bool QmlJSQuickFixCollector::supportsEditor(TextEditor::ITextEditor *editable) const
{
    return qobject_cast<QmlJSTextEditorWidget *>(editable->widget()) != 0;
}

bool QmlJSQuickFixCollector::supportsPolicy(TextEditor::CompletionPolicy policy) const
{
    return policy == TextEditor::QuickFixCompletion;
}

TextEditor::QuickFixState *QmlJSQuickFixCollector::initializeCompletion(TextEditor::BaseTextEditorWidget *editor)
{
    if (QmlJSTextEditorWidget *qmljsEditor = qobject_cast<QmlJSTextEditorWidget *>(editor)) {
        const SemanticInfo info = qmljsEditor->semanticInfo();

        if (! info.isValid() || qmljsEditor->isOutdated()) {
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
