/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qmljssemantichighlighter.h"

#include "qmljseditor.h"

#include <qmljs/qmljsdocument.h>
#include <qmljs/qmljsscopechain.h>
#include <qmljs/qmljsscopebuilder.h>
#include <qmljs/qmljsevaluate.h>
#include <qmljs/qmljscontext.h>
#include <qmljs/qmljsbind.h>
#include <qmljs/qmljsutils.h>
#include <qmljs/parser/qmljsast_p.h>
#include <qmljs/parser/qmljsastvisitor_p.h>
#include <texteditor/syntaxhighlighter.h>
#include <texteditor/basetextdocument.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/fontsettings.h>
#include <utils/qtcassert.h>

#include <QThreadPool>
#include <QFutureInterface>
#include <QRunnable>

using namespace QmlJSEditor;
using namespace QmlJSEditor::Internal;
using namespace QmlJS;
using namespace QmlJS::AST;

namespace {

template <typename T>
class PriorityTask :
        public QFutureInterface<T>,
        public QRunnable
{
public:
    typedef QFuture<T> Future;

    Future start(QThread::Priority priority)
    {
        this->setRunnable(this);
        this->reportStarted();
        Future future = this->future();
        QThreadPool::globalInstance()->start(this, priority);
        return future;
    }
};

static bool isIdScope(const ObjectValue *scope, const QList<const QmlComponentChain *> &chain)
{
    foreach (const QmlComponentChain *c, chain) {
        if (c->idScope() == scope)
            return true;
        if (isIdScope(scope, c->instantiatingComponents()))
            return true;
    }
    return false;
}

class CollectStateNames : protected Visitor
{
    QStringList m_stateNames;
    bool m_inStateType;
    ScopeChain m_scopeChain;
    const CppComponentValue *m_statePrototype;

public:
    CollectStateNames(const ScopeChain &scopeChain)
        : m_scopeChain(scopeChain)
    {
        m_statePrototype = scopeChain.context()->valueOwner()->cppQmlTypes().objectByCppName(QLatin1String("QDeclarativeState"));
    }

    QStringList operator()(Node *ast)
    {
        m_stateNames.clear();
        if (!m_statePrototype)
            return m_stateNames;
        m_inStateType = false;
        accept(ast);
        return m_stateNames;
    }

protected:
    void accept(Node *ast)
    {
        if (ast)
            ast->accept(this);
    }

    bool preVisit(Node *ast)
    {
        return ast->uiObjectMemberCast()
                || cast<UiProgram *>(ast)
                || cast<UiObjectInitializer *>(ast)
                || cast<UiObjectMemberList *>(ast)
                || cast<UiArrayMemberList *>(ast);
    }

    bool hasStatePrototype(Node *ast)
    {
        Bind *bind = m_scopeChain.document()->bind();
        const ObjectValue *v = bind->findQmlObject(ast);
        if (!v)
            return false;
        PrototypeIterator it(v, m_scopeChain.context());
        while (it.hasNext()) {
            const ObjectValue *proto = it.next();
            const CppComponentValue *qmlProto = value_cast<CppComponentValue>(proto);
            if (!qmlProto)
                continue;
            if (qmlProto->metaObject() == m_statePrototype->metaObject())
                return true;
        }
        return false;
    }

    bool visit(UiObjectDefinition *ast)
    {
        const bool old = m_inStateType;
        m_inStateType = hasStatePrototype(ast);
        accept(ast->initializer);
        m_inStateType = old;
        return false;
    }

    bool visit(UiObjectBinding *ast)
    {
        const bool old = m_inStateType;
        m_inStateType = hasStatePrototype(ast);
        accept(ast->initializer);
        m_inStateType = old;
        return false;
    }

    bool visit(UiScriptBinding *ast)
    {
        if (!m_inStateType)
            return false;
        if (!ast->qualifiedId || ast->qualifiedId->name.isEmpty() || ast->qualifiedId->next)
            return false;
        if (ast->qualifiedId->name != QLatin1String("name"))
            return false;

        ExpressionStatement *expStmt = cast<ExpressionStatement *>(ast->statement);
        if (!expStmt)
            return false;
        StringLiteral *strLit = cast<StringLiteral *>(expStmt->expression);
        if (!strLit || strLit->value.isEmpty())
            return false;

        m_stateNames += strLit->value.toString();

        return false;
    }
};

class CollectionTask :
        public PriorityTask<SemanticHighlighter::Use>,
        protected Visitor
{
public:
    CollectionTask(const ScopeChain &scopeChain)
        : m_scopeChain(scopeChain)
        , m_scopeBuilder(&m_scopeChain)
        , m_lineOfLastUse(0)
    {}

protected:
    void accept(Node *ast)
    {
        if (ast)
            ast->accept(this);
    }

    void scopedAccept(Node *ast, Node *child)
    {
        m_scopeBuilder.push(ast);
        accept(child);
        m_scopeBuilder.pop();
    }

    void processName(const QStringRef &name, SourceLocation location)
    {
        if (name.isEmpty())
            return;

        const QString &nameStr = name.toString();
        const ObjectValue *scope = 0;
        const Value *value = m_scopeChain.lookup(nameStr, &scope);
        if (!value || !scope)
            return;

        SemanticHighlighter::UseType type = SemanticHighlighter::UnknownType;
        if (m_scopeChain.qmlTypes() == scope) {
            type = SemanticHighlighter::QmlTypeType;
        } else if (m_scopeChain.qmlScopeObjects().contains(scope)) {
            type = SemanticHighlighter::ScopeObjectPropertyType;
        } else if (m_scopeChain.jsScopes().contains(scope)) {
            type = SemanticHighlighter::JsScopeType;
        } else if (m_scopeChain.jsImports() == scope) {
            type = SemanticHighlighter::JsImportType;
        } else if (m_scopeChain.globalScope() == scope) {
            type = SemanticHighlighter::JsGlobalType;
        } else if (QSharedPointer<const QmlComponentChain> chain = m_scopeChain.qmlComponentChain()) {
            if (scope == chain->idScope()) {
                type = SemanticHighlighter::LocalIdType;
            } else if (isIdScope(scope, chain->instantiatingComponents())) {
                type = SemanticHighlighter::ExternalIdType;
            } else if (scope == chain->rootObjectScope()) {
                type = SemanticHighlighter::RootObjectPropertyType;
            } else  { // check for this?
                type = SemanticHighlighter::ExternalObjectPropertyType;
            }
        }

        if (type != SemanticHighlighter::UnknownType)
            addUse(location, type);
    }

    void processTypeId(UiQualifiedId *typeId)
    {
        if (!typeId)
            return;
        if (m_scopeChain.context()->lookupType(m_scopeChain.document().data(), typeId))
            addUse(fullLocationForQualifiedId(typeId), SemanticHighlighter::QmlTypeType);
    }

    void processBindingName(UiQualifiedId *localId)
    {
        if (!localId)
            return;
        addUse(fullLocationForQualifiedId(localId), SemanticHighlighter::BindingNameType);
    }

    bool visit(UiImport *ast)
    {
        processName(ast->importId, ast->importIdToken);
        return true;
    }

    bool visit(UiObjectDefinition *ast)
    {
        if (m_scopeChain.document()->bind()->isGroupedPropertyBinding(ast))
            processBindingName(ast->qualifiedTypeNameId);
        else
            processTypeId(ast->qualifiedTypeNameId);
        scopedAccept(ast, ast->initializer);
        return false;
    }

    bool visit(UiObjectBinding *ast)
    {
        processTypeId(ast->qualifiedTypeNameId);
        processBindingName(ast->qualifiedId);
        scopedAccept(ast, ast->initializer);
        return false;
    }

    bool visit(UiScriptBinding *ast)
    {
        processBindingName(ast->qualifiedId);
        scopedAccept(ast, ast->statement);
        return false;
    }

    bool visit(UiArrayBinding *ast)
    {
        processBindingName(ast->qualifiedId);
        return true;
    }

    bool visit(UiPublicMember *ast)
    {
        if (ast->typeToken.isValid() && !ast->memberType.isEmpty()) {
            if (m_scopeChain.context()->lookupType(m_scopeChain.document().data(), QStringList(ast->memberType.toString())))
                addUse(ast->typeToken, SemanticHighlighter::QmlTypeType);
        }
        if (ast->identifierToken.isValid())
            addUse(ast->identifierToken, SemanticHighlighter::BindingNameType);
        scopedAccept(ast, ast->statement);
        return false;
    }

    bool visit(FunctionExpression *ast)
    {
        processName(ast->name, ast->identifierToken);
        scopedAccept(ast, ast->body);
        return false;
    }

    bool visit(FunctionDeclaration *ast)
    {
        return visit(static_cast<FunctionExpression *>(ast));
    }

    bool visit(VariableDeclaration *ast)
    {
        processName(ast->name, ast->identifierToken);
        return true;
    }

    bool visit(IdentifierExpression *ast)
    {
        processName(ast->name, ast->identifierToken);
        return false;
    }

    bool visit(StringLiteral *ast)
    {
        if (ast->value.isEmpty())
            return false;

        const QString &value = ast->value.toString();
        if (m_stateNames.contains(value))
            addUse(ast->literalToken, SemanticHighlighter::LocalStateNameType);

        return false;
    }

private:
    void run()
    {
        Node *root = m_scopeChain.document()->ast();
        m_stateNames = CollectStateNames(m_scopeChain)(root);
        accept(root);
        flush();
        reportFinished();
    }

    void addUse(const SourceLocation &location, SemanticHighlighter::UseType type)
    {
        addUse(SemanticHighlighter::Use(location.startLine, location.startColumn, location.length, type));
    }

    static const int chunkSize = 50;

    void addUse(const SemanticHighlighter::Use &use)
    {
        if (m_uses.size() >= chunkSize) {
            if (use.line > m_lineOfLastUse)
                flush();
        }

        m_lineOfLastUse = qMax(m_lineOfLastUse, use.line);
        m_uses.append(use);
    }

    static bool sortByLinePredicate(const SemanticHighlighter::Use &lhs, const SemanticHighlighter::Use &rhs)
    {
        return lhs.line < rhs.line;
    }

    void flush()
    {
        m_lineOfLastUse = 0;

        if (m_uses.isEmpty())
            return;

        qSort(m_uses.begin(), m_uses.end(), sortByLinePredicate);
        reportResults(m_uses);
        m_uses.clear();
        m_uses.reserve(chunkSize);
    }

    ScopeChain m_scopeChain;
    ScopeBuilder m_scopeBuilder;
    QStringList m_stateNames;
    QVector<SemanticHighlighter::Use> m_uses;
    unsigned m_lineOfLastUse;
};

} // anonymous namespace

SemanticHighlighter::SemanticHighlighter(QmlJSTextEditorWidget *editor)
    : QObject(editor)
    , m_editor(editor)
    , m_startRevision(0)
{
    connect(&m_watcher, SIGNAL(resultsReadyAt(int,int)),
            this, SLOT(applyResults(int,int)));
    connect(&m_watcher, SIGNAL(finished()),
            this, SLOT(finished()));
}

void SemanticHighlighter::rerun(const ScopeChain &scopeChain)
{
    m_watcher.cancel();

    // this does not simply use QtConcurrentRun because we want a low-priority future
    // the thread pool deletes the task when it is done
    CollectionTask::Future f = (new CollectionTask(scopeChain))->start(QThread::LowestPriority);
    m_startRevision = m_editor->editorRevision();
    m_watcher.setFuture(f);
}

void SemanticHighlighter::cancel()
{
    m_watcher.cancel();
}

void SemanticHighlighter::applyResults(int from, int to)
{
    if (m_watcher.isCanceled())
        return;
    if (m_startRevision != m_editor->editorRevision())
        return;

    TextEditor::BaseTextDocument *baseTextDocument = m_editor->baseTextDocument().data();
    QTC_ASSERT(baseTextDocument, return);
    TextEditor::SyntaxHighlighter *highlighter = qobject_cast<TextEditor::SyntaxHighlighter *>(baseTextDocument->syntaxHighlighter());
    QTC_ASSERT(highlighter, return);

    TextEditor::SemanticHighlighter::incrementalApplyExtraAdditionalFormats(
                highlighter, m_watcher.future(), from, to, m_formats);
}

void SemanticHighlighter::finished()
{
    if (m_watcher.isCanceled())
        return;
    if (m_startRevision != m_editor->editorRevision())
        return;

    TextEditor::BaseTextDocument *baseTextDocument = m_editor->baseTextDocument().data();
    QTC_ASSERT(baseTextDocument, return);
    TextEditor::SyntaxHighlighter *highlighter = qobject_cast<TextEditor::SyntaxHighlighter *>(baseTextDocument->syntaxHighlighter());
    QTC_ASSERT(highlighter, return);

    TextEditor::SemanticHighlighter::clearExtraAdditionalFormatsUntilEnd(
                highlighter, m_watcher.future());
}

void SemanticHighlighter::updateFontSettings(const TextEditor::FontSettings &fontSettings)
{
    m_formats[LocalIdType] = fontSettings.toTextCharFormat(TextEditor::C_QML_LOCAL_ID);
    m_formats[ExternalIdType] = fontSettings.toTextCharFormat(TextEditor::C_QML_EXTERNAL_ID);
    m_formats[QmlTypeType] = fontSettings.toTextCharFormat(TextEditor::C_QML_TYPE_ID);
    m_formats[RootObjectPropertyType] = fontSettings.toTextCharFormat(TextEditor::C_QML_ROOT_OBJECT_PROPERTY);
    m_formats[ScopeObjectPropertyType] = fontSettings.toTextCharFormat(TextEditor::C_QML_SCOPE_OBJECT_PROPERTY);
    m_formats[ExternalObjectPropertyType] = fontSettings.toTextCharFormat(TextEditor::C_QML_EXTERNAL_OBJECT_PROPERTY);
    m_formats[JsScopeType] = fontSettings.toTextCharFormat(TextEditor::C_JS_SCOPE_VAR);
    m_formats[JsImportType] = fontSettings.toTextCharFormat(TextEditor::C_JS_IMPORT_VAR);
    m_formats[JsGlobalType] = fontSettings.toTextCharFormat(TextEditor::C_JS_GLOBAL_VAR);
    m_formats[LocalStateNameType] = fontSettings.toTextCharFormat(TextEditor::C_QML_STATE_NAME);
    m_formats[BindingNameType] = fontSettings.toTextCharFormat(TextEditor::C_BINDING);
    m_formats[FieldType] = fontSettings.toTextCharFormat(TextEditor::C_FIELD);
}

int SemanticHighlighter::startRevision() const
{
    return m_startRevision;
}
