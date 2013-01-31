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

#include "qmljsfindreferences.h"

#include <texteditor/basetexteditor.h>
#include <texteditor/basefilefind.h>
#include <find/searchresultwindow.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/filesearch.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <coreplugin/progressmanager/futureprogress.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>

#include <qmljs/qmljsmodelmanagerinterface.h>
#include <qmljs/qmljsbind.h>
#include <qmljs/qmljslink.h>
#include <qmljs/qmljsevaluate.h>
#include <qmljs/qmljsscopebuilder.h>
#include <qmljs/qmljsscopeastpath.h>
#include <qmljs/qmljscontext.h>
#include <qmljs/parser/qmljsastvisitor_p.h>
#include <qmljs/parser/qmljsast_p.h>
#include <qmljstools/qmljsmodelmanager.h>

#include "qmljseditorconstants.h"

#include <QTime>
#include <QTimer>
#include <QtConcurrentRun>
#include <QtConcurrentMap>
#include <QDir>
#include <QApplication>
#include <QLabel>
#include <utils/runextensions.h>

#include <functional>

using namespace QmlJS;
using namespace QmlJS::AST;
using namespace QmlJSEditor;

namespace {

// ### These visitors could be useful in general

class FindUsages: protected Visitor
{
public:
    typedef QList<AST::SourceLocation> Result;

    FindUsages(Document::Ptr doc, const ContextPtr &context)
        : _doc(doc)
        , _scopeChain(doc, context)
        , _builder(&_scopeChain)
    {
    }

    Result operator()(const QString &name, const ObjectValue *scope)
    {
        _name = name;
        _scope = scope;
        _usages.clear();
        if (_doc)
            Node::accept(_doc->ast(), this);
        return _usages;
    }

protected:
    void accept(AST::Node *node)
    { AST::Node::acceptChild(node, this); }

    using Visitor::visit;

    virtual bool visit(AST::UiPublicMember *node)
    {
        if (node->name == _name
                && _scopeChain.qmlScopeObjects().contains(_scope)) {
            _usages.append(node->identifierToken);
        }
        if (AST::cast<Block *>(node->statement)) {
            _builder.push(node);
            Node::accept(node->statement, this);
            _builder.pop();
            return false;
        }
        return true;
    }

    virtual bool visit(AST::UiObjectDefinition *node)
    {
        _builder.push(node);
        Node::accept(node->initializer, this);
        _builder.pop();
        return false;
    }

    virtual bool visit(AST::UiObjectBinding *node)
    {
        if (node->qualifiedId
                && !node->qualifiedId->next
                && node->qualifiedId->name == _name
                && checkQmlScope()) {
            _usages.append(node->qualifiedId->identifierToken);
        }

        _builder.push(node);
        Node::accept(node->initializer, this);
        _builder.pop();
        return false;
    }

    virtual bool visit(AST::UiScriptBinding *node)
    {
        if (node->qualifiedId
                && !node->qualifiedId->next
                && node->qualifiedId->name == _name
                && checkQmlScope()) {
            _usages.append(node->qualifiedId->identifierToken);
        }
        if (AST::cast<Block *>(node->statement)) {
            Node::accept(node->qualifiedId, this);
            _builder.push(node);
            Node::accept(node->statement, this);
            _builder.pop();
            return false;
        }
        return true;
    }

    virtual bool visit(AST::UiArrayBinding *node)
    {
        if (node->qualifiedId
                && !node->qualifiedId->next
                && node->qualifiedId->name == _name
                && checkQmlScope()) {
            _usages.append(node->qualifiedId->identifierToken);
        }
        return true;
    }

    virtual bool visit(AST::IdentifierExpression *node)
    {
        if (node->name.isEmpty() || node->name != _name)
            return false;

        const ObjectValue *scope;
        _scopeChain.lookup(_name, &scope);
        if (!scope)
            return false;
        if (check(scope)) {
            _usages.append(node->identifierToken);
            return false;
        }

        // the order of scopes in 'instantiatingComponents' is undefined,
        // so it might still be a use - we just found a different value in a different scope first

        // if scope is one of these, our match wasn't inside the instantiating components list
        const ScopeChain &chain = _scopeChain;
        if (chain.jsScopes().contains(scope)
                || chain.qmlScopeObjects().contains(scope)
                || chain.qmlTypes() == scope
                || chain.globalScope() == scope)
            return false;

        if (contains(chain.qmlComponentChain().data()))
            _usages.append(node->identifierToken);

        return false;
    }

    virtual bool visit(AST::FieldMemberExpression *node)
    {
        if (node->name != _name)
            return true;

        Evaluate evaluate(&_scopeChain);
        const Value *lhsValue = evaluate(node->base);
        if (!lhsValue)
            return true;

        if (check(lhsValue->asObjectValue())) // passing null is ok
            _usages.append(node->identifierToken);

        return true;
    }

    virtual bool visit(AST::FunctionDeclaration *node)
    {
        return visit(static_cast<FunctionExpression *>(node));
    }

    virtual bool visit(AST::FunctionExpression *node)
    {
        if (node->name == _name) {
            if (checkLookup())
                _usages.append(node->identifierToken);
        }
        Node::accept(node->formals, this);
        _builder.push(node);
        Node::accept(node->body, this);
        _builder.pop();
        return false;
    }

    virtual bool visit(AST::VariableDeclaration *node)
    {
        if (node->name == _name) {
            if (checkLookup())
                _usages.append(node->identifierToken);
        }
        return true;
    }

private:
    bool contains(const QmlComponentChain *chain)
    {
        if (!chain || !chain->document() || !chain->document()->bind())
            return false;

        const ObjectValue *idEnv = chain->document()->bind()->idEnvironment();
        if (idEnv && idEnv->lookupMember(_name, _scopeChain.context()))
            return idEnv == _scope;
        const ObjectValue *root = chain->document()->bind()->rootObjectValue();
        if (root && root->lookupMember(_name, _scopeChain.context()))
            return check(root);

        foreach (const QmlComponentChain *parent, chain->instantiatingComponents()) {
            if (contains(parent))
                return true;
        }
        return false;
    }

    bool check(const ObjectValue *s)
    {
        if (!s)
            return false;
        const ObjectValue *definingObject;
        s->lookupMember(_name, _scopeChain.context(), &definingObject);
        return definingObject == _scope;
    }

    bool checkQmlScope()
    {
        foreach (const ObjectValue *s, _scopeChain.qmlScopeObjects()) {
            if (check(s))
                return true;
        }
        return false;
    }

    bool checkLookup()
    {
        const ObjectValue *scope = 0;
        _scopeChain.lookup(_name, &scope);
        return check(scope);
    }

    Result _usages;

    Document::Ptr _doc;
    ScopeChain _scopeChain;
    ScopeBuilder _builder;

    QString _name;
    const ObjectValue *_scope;
};

class FindTypeUsages: protected Visitor
{
public:
    typedef QList<AST::SourceLocation> Result;

    FindTypeUsages(Document::Ptr doc, const ContextPtr &context)
        : _doc(doc)
        , _context(context)
        , _scopeChain(doc, context)
        , _builder(&_scopeChain)
    {
    }

    Result operator()(const QString &name, const ObjectValue *typeValue)
    {
        _name = name;
        _typeValue = typeValue;
        _usages.clear();
        if (_doc)
            Node::accept(_doc->ast(), this);
        return _usages;
    }

protected:
    void accept(AST::Node *node)
    { AST::Node::acceptChild(node, this); }

    using Visitor::visit;

    virtual bool visit(AST::UiPublicMember *node)
    {
        if (node->memberType == _name){
            const ObjectValue * tVal = _context->lookupType(_doc.data(), QStringList(_name));
            if (tVal == _typeValue)
                _usages.append(node->typeToken);
        }
        if (AST::cast<Block *>(node->statement)) {
            _builder.push(node);
            Node::accept(node->statement, this);
            _builder.pop();
            return false;
        }
        return true;
    }

    virtual bool visit(AST::UiObjectDefinition *node)
    {
        checkTypeName(node->qualifiedTypeNameId);
        _builder.push(node);
        Node::accept(node->initializer, this);
        _builder.pop();
        return false;
    }

    virtual bool visit(AST::UiObjectBinding *node)
    {
        checkTypeName(node->qualifiedTypeNameId);
        _builder.push(node);
        Node::accept(node->initializer, this);
        _builder.pop();
        return false;
    }

    virtual bool visit(AST::UiScriptBinding *node)
    {
        if (AST::cast<Block *>(node->statement)) {
            Node::accept(node->qualifiedId, this);
            _builder.push(node);
            Node::accept(node->statement, this);
            _builder.pop();
            return false;
        }
        return true;
    }

    virtual bool visit(AST::IdentifierExpression *node)
    {
        if (node->name != _name)
            return false;

        const ObjectValue *scope;
        const Value *objV = _scopeChain.lookup(_name, &scope);
        if (objV == _typeValue)
            _usages.append(node->identifierToken);
        return false;
    }

    virtual bool visit(AST::FieldMemberExpression *node)
    {
        if (node->name != _name)
            return true;
        Evaluate evaluate(&_scopeChain);
        const Value *lhsValue = evaluate(node->base);
        if (!lhsValue)
            return true;
        const ObjectValue *lhsObj = lhsValue->asObjectValue();
        if (lhsObj && lhsObj->lookupMember(_name, _context) == _typeValue)
            _usages.append(node->identifierToken);
        return true;
    }

    virtual bool visit(AST::FunctionDeclaration *node)
    {
        return visit(static_cast<FunctionExpression *>(node));
    }

    virtual bool visit(AST::FunctionExpression *node)
    {
        Node::accept(node->formals, this);
        _builder.push(node);
        Node::accept(node->body, this);
        _builder.pop();
        return false;
    }

    virtual bool visit(AST::VariableDeclaration *node)
    {
        Node::accept(node->expression, this);
        return false;
    }

    virtual bool visit(UiImport *ast)
    {
        if (ast && ast->importId == _name) {
            const Imports *imp = _context->imports(_doc.data());
            if (!imp)
                return false;
            if (_context->lookupType(_doc.data(), QStringList(_name)) == _typeValue)
                _usages.append(ast->importIdToken);
        }
        return false;
    }


private:
    bool checkTypeName(UiQualifiedId *id)
    {
        for (UiQualifiedId *att = id; att; att = att->next){
            if (att->name == _name) {
                const ObjectValue *objectValue = _context->lookupType(_doc.data(), id, att->next);
                if (_typeValue == objectValue){
                    _usages.append(att->identifierToken);
                    return true;
                }
            }
        }
        return false;
    }

    Result _usages;

    Document::Ptr _doc;
    ContextPtr _context;
    ScopeChain _scopeChain;
    ScopeBuilder _builder;

    QString _name;
    const ObjectValue *_typeValue;
};

class FindTargetExpression: protected Visitor
{
public:
    enum Kind {
        ExpKind,
        TypeKind
    };

    FindTargetExpression(Document::Ptr doc, const ScopeChain *scopeChain)
        : _doc(doc), _scopeChain(scopeChain)
    {
    }

    void operator()(quint32 offset)
    {
        _name.clear();
        _scope = 0;
        _objectNode = 0;
        _offset = offset;
        _typeKind = ExpKind;
        if (_doc)
            Node::accept(_doc->ast(), this);
    }

    QString name() const
    { return _name; }

    const ObjectValue *scope()
    {
        if (!_scope)
            _scopeChain->lookup(_name, &_scope);
        return _scope;
    }

    Kind typeKind(){
        return _typeKind;
    }

    const Value *targetValue(){
        return _targetValue;
    }

protected:
    void accept(AST::Node *node)
    { AST::Node::acceptChild(node, this); }

    using Visitor::visit;

    virtual bool preVisit(Node *node)
    {
        if (Statement *stmt = node->statementCast())
            return containsOffset(stmt->firstSourceLocation(), stmt->lastSourceLocation());
        else if (ExpressionNode *exp = node->expressionCast())
            return containsOffset(exp->firstSourceLocation(), exp->lastSourceLocation());
        else if (UiObjectMember *ui = node->uiObjectMemberCast())
            return containsOffset(ui->firstSourceLocation(), ui->lastSourceLocation());
        return true;
    }

    virtual bool visit(IdentifierExpression *node)
    {
        if (containsOffset(node->identifierToken)) {
            _name = node->name.toString();
            if ((!_name.isEmpty()) && _name.at(0).isUpper()) {
                // a possible type
                _targetValue = _scopeChain->lookup(_name, &_scope);
                if (value_cast<ObjectValue>(_targetValue))
                    _typeKind = TypeKind;
            }
        }
        return true;
    }

    virtual bool visit(FieldMemberExpression *node)
    {
        if (containsOffset(node->identifierToken)) {
            setScope(node->base);
            _name = node->name.toString();
            if ((!_name.isEmpty()) && _name.at(0).isUpper()) {
                // a possible type
                Evaluate evaluate(_scopeChain);
                const Value *lhsValue = evaluate(node->base);
                if (!lhsValue)
                    return true;
                const ObjectValue *lhsObj = lhsValue->asObjectValue();
                if (lhsObj) {
                    _scope = lhsObj;
                    _targetValue = lhsObj->lookupMember(_name, _scopeChain->context());
                    _typeKind = TypeKind;
                }
            }
            return false;
        }
        return true;
    }

    virtual bool visit(UiScriptBinding *node)
    {
        return !checkBindingName(node->qualifiedId);
    }

    virtual bool visit(UiArrayBinding *node)
    {
        return !checkBindingName(node->qualifiedId);
    }

    virtual bool visit(UiObjectBinding *node)
    {
        if ((!checkTypeName(node->qualifiedTypeNameId)) &&
                (!checkBindingName(node->qualifiedId))) {
            Node *oldObjectNode = _objectNode;
            _objectNode = node;
            accept(node->initializer);
            _objectNode = oldObjectNode;
        }
        return false;
    }

    virtual bool visit(UiObjectDefinition *node)
    {
        if (!checkTypeName(node->qualifiedTypeNameId)) {
            Node *oldObjectNode = _objectNode;
            _objectNode = node;
            accept(node->initializer);
            _objectNode = oldObjectNode;
        }
        return false;
    }

    virtual bool visit(UiPublicMember *node)
    {
        if (containsOffset(node->typeToken)){
            if (!node->memberType.isEmpty()) {
                _name = node->memberType.toString();
                _targetValue = _scopeChain->context()->lookupType(_doc.data(), QStringList(_name));
                _scope = 0;
                _typeKind = TypeKind;
            }
            return false;
        } else if (containsOffset(node->identifierToken)) {
            _scope = _doc->bind()->findQmlObject(_objectNode);
            _name = node->name.toString();
            return false;
        }
        return true;
    }

    virtual bool visit(FunctionDeclaration *node)
    {
        return visit(static_cast<FunctionExpression *>(node));
    }

    virtual bool visit(FunctionExpression *node)
    {
        if (containsOffset(node->identifierToken)) {
            _name = node->name.toString();
            return false;
        }
        return true;
    }

    virtual bool visit(VariableDeclaration *node)
    {
        if (containsOffset(node->identifierToken)) {
            _name = node->name.toString();
            return false;
        }
        return true;
    }

private:
    bool containsOffset(SourceLocation start, SourceLocation end)
    {
        return _offset >= start.begin() && _offset <= end.end();
    }

    bool containsOffset(SourceLocation loc)
    {
        return _offset >= loc.begin() && _offset <= loc.end();
    }

    bool checkBindingName(UiQualifiedId *id)
    {
        if (id && !id->name.isEmpty() && !id->next && containsOffset(id->identifierToken)) {
            _scope = _doc->bind()->findQmlObject(_objectNode);
            _name = id->name.toString();
            return true;
        }
        return false;
    }

    bool checkTypeName(UiQualifiedId *id)
    {
        for (UiQualifiedId *att = id; att; att = att->next) {
            if (!att->name.isEmpty() && containsOffset(att->identifierToken)) {
                _targetValue = _scopeChain->context()->lookupType(_doc.data(), id, att->next);
                _scope = 0;
                _name = att->name.toString();
                _typeKind = TypeKind;
                return true;
            }
        }
        return false;
    }

    void setScope(Node *node)
    {
        Evaluate evaluate(_scopeChain);
        const Value *v = evaluate(node);
        if (v)
            _scope = v->asObjectValue();
    }

    QString _name;
    const ObjectValue *_scope;
    const Value *_targetValue;
    Node *_objectNode;
    Document::Ptr _doc;
    const ScopeChain *_scopeChain;
    quint32 _offset;
    Kind _typeKind;
};

static QString matchingLine(unsigned position, const QString &source)
{
    int start = source.lastIndexOf(QLatin1Char('\n'), position);
    start += 1;
    int end = source.indexOf(QLatin1Char('\n'), position);

    return source.mid(start, end - start);
}

class ProcessFile: public std::unary_function<QString, QList<FindReferences::Usage> >
{
    ContextPtr context;
    typedef FindReferences::Usage Usage;
    QString name;
    const ObjectValue *scope;
    QFutureInterface<Usage> *future;

public:
    ProcessFile(const ContextPtr &context,
                QString name,
                const ObjectValue *scope,
                QFutureInterface<Usage> *future)
        : context(context), name(name), scope(scope), future(future)
    { }

    QList<Usage> operator()(const QString &fileName)
    {
        QList<Usage> usages;
        if (future->isPaused())
            future->waitForResume();
        if (future->isCanceled())
            return usages;
        Document::Ptr doc = context->snapshot().document(fileName);
        if (!doc)
            return usages;

        // find all idenfifier expressions, try to resolve them and check if the result is in scope
        FindUsages findUsages(doc, context);
        FindUsages::Result results = findUsages(name, scope);
        foreach (const AST::SourceLocation &loc, results)
            usages.append(Usage(fileName, matchingLine(loc.offset, doc->source()), loc.startLine, loc.startColumn - 1, loc.length));
        if (future->isPaused())
            future->waitForResume();
        return usages;
    }
};

class SearchFileForType: public std::unary_function<QString, QList<FindReferences::Usage> >
{
    ContextPtr context;
    typedef FindReferences::Usage Usage;
    QString name;
    const ObjectValue *scope;
    QFutureInterface<Usage> *future;

public:
    SearchFileForType(const ContextPtr &context,
                      QString name,
                      const ObjectValue *scope,
                      QFutureInterface<Usage> *future)
        : context(context), name(name), scope(scope), future(future)
    { }

    QList<Usage> operator()(const QString &fileName)
    {
        QList<Usage> usages;
        if (future->isPaused())
            future->waitForResume();
        if (future->isCanceled())
            return usages;
        Document::Ptr doc = context->snapshot().document(fileName);
        if (!doc)
            return usages;

        // find all idenfifier expressions, try to resolve them and check if the result is in scope
        FindTypeUsages findUsages(doc, context);
        FindTypeUsages::Result results = findUsages(name, scope);
        foreach (const AST::SourceLocation &loc, results)
            usages.append(Usage(fileName, matchingLine(loc.offset, doc->source()), loc.startLine, loc.startColumn - 1, loc.length));
        if (future->isPaused())
            future->waitForResume();
        return usages;
    }
};

class UpdateUI: public std::binary_function<QList<FindReferences::Usage> &, QList<FindReferences::Usage>, void>
{
    typedef FindReferences::Usage Usage;
    QFutureInterface<Usage> *future;

public:
    UpdateUI(QFutureInterface<Usage> *future): future(future) {}

    void operator()(QList<Usage> &, const QList<Usage> &usages)
    {
        foreach (const Usage &u, usages)
            future->reportResult(u);

        future->setProgressValue(future->progressValue() + 1);
    }
};

} // end of anonymous namespace

FindReferences::FindReferences(QObject *parent)
    : QObject(parent)
{
    m_watcher.setPendingResultsLimit(1);
    connect(&m_watcher, SIGNAL(resultsReadyAt(int,int)), this, SLOT(displayResults(int,int)));
    connect(&m_watcher, SIGNAL(finished()), this, SLOT(searchFinished()));
}

FindReferences::~FindReferences()
{
}

static void find_helper(QFutureInterface<FindReferences::Usage> &future,
                        const ModelManagerInterface::WorkingCopy workingCopy,
                        Snapshot snapshot,
                        const QString fileName,
                        quint32 offset,
                        QString replacement)
{
    // update snapshot from workingCopy to make sure it's up to date
    // ### remove?
    // ### this is a great candidate for map-reduce
    QHashIterator< QString, QPair<QString, int> > it(workingCopy.all());
    while (it.hasNext()) {
        it.next();
        const QString fileName = it.key();
        Document::Ptr oldDoc = snapshot.document(fileName);
        if (oldDoc && oldDoc->editorRevision() == it.value().second)
            continue;

        Document::Language language;
        if (oldDoc)
            language = oldDoc->language();
        else
            language = QmlJSTools::languageOfFile(fileName);

        Document::MutablePtr newDoc = snapshot.documentFromSource(
                    it.value().first, fileName, language);
        newDoc->parse();
        snapshot.insert(newDoc);
    }

    // find the scope for the name we're searching
    Document::Ptr doc = snapshot.document(fileName);
    if (!doc)
        return;

    QmlJS::ModelManagerInterface *modelManager = QmlJS::ModelManagerInterface::instance();

    Link link(snapshot, modelManager->importPaths(), modelManager->builtins(doc));
    ContextPtr context = link();

    ScopeChain scopeChain(doc, context);
    ScopeBuilder builder(&scopeChain);
    ScopeAstPath astPath(doc);
    builder.push(astPath(offset));

    FindTargetExpression findTarget(doc, &scopeChain);
    findTarget(offset);
    const QString &name = findTarget.name();
    if (name.isEmpty())
        return;
    if (!replacement.isNull() && replacement.isEmpty())
        replacement = name;

    QStringList files;
    foreach (const Document::Ptr &doc, snapshot) {
        // ### skip files that don't contain the name token
        files.append(doc->fileName());
    }

    future.setProgressRange(0, files.size());

    // report a dummy usage to indicate the search is starting
    FindReferences::Usage searchStarting(replacement, name, 0, 0, 0);

    if (findTarget.typeKind() == findTarget.TypeKind){
        const ObjectValue *typeValue = value_cast<ObjectValue>(findTarget.targetValue());
        if (!typeValue)
            return;
        future.reportResult(searchStarting);

        SearchFileForType process(context, name, typeValue, &future);
        UpdateUI reduce(&future);

        QtConcurrent::blockingMappedReduced<QList<FindReferences::Usage> > (files, process, reduce);
    } else {
        const ObjectValue *scope = findTarget.scope();
        if (!scope)
            return;
        scope->lookupMember(name, context, &scope);
        if (!scope)
            return;
        if (!scope->className().isEmpty())
            searchStarting.lineText.prepend(scope->className() + QLatin1Char('.'));
        future.reportResult(searchStarting);

        ProcessFile process(context, name, scope, &future);
        UpdateUI reduce(&future);

        QtConcurrent::blockingMappedReduced<QList<FindReferences::Usage> > (files, process, reduce);
    }
    future.setProgressValue(files.size());
}

void FindReferences::findUsages(const QString &fileName, quint32 offset)
{
    ModelManagerInterface *modelManager = ModelManagerInterface::instance();

    QFuture<Usage> result = QtConcurrent::run(
                &find_helper, modelManager->workingCopy(),
                modelManager->snapshot(), fileName, offset,
                QString());
    m_watcher.setFuture(result);
}

void FindReferences::renameUsages(const QString &fileName, quint32 offset,
                                  const QString &replacement)
{
    ModelManagerInterface *modelManager = ModelManagerInterface::instance();

    // an empty non-null string asks the future to use the current name as base
    QString newName = replacement;
    if (newName.isNull())
        newName = QLatin1String("");

    QFuture<Usage> result = QtConcurrent::run(
                &find_helper, modelManager->workingCopy(),
                modelManager->snapshot(), fileName, offset,
                newName);
    m_watcher.setFuture(result);
}

void FindReferences::displayResults(int first, int last)
{
    // the first usage is always a dummy to indicate we now start searching
    if (first == 0) {
        Usage dummy = m_watcher.future().resultAt(0);
        const QString replacement = dummy.path;
        const QString symbolName = dummy.lineText;
        const QString label = tr("QML/JS Usages:");

        if (replacement.isEmpty()) {
            m_currentSearch = Find::SearchResultWindow::instance()->startNewSearch(
                        label, QString(), symbolName, Find::SearchResultWindow::SearchOnly);
        } else {
            m_currentSearch = Find::SearchResultWindow::instance()->startNewSearch(
                        label, QString(), symbolName, Find::SearchResultWindow::SearchAndReplace);
            m_currentSearch->setTextToReplace(replacement);
            connect(m_currentSearch, SIGNAL(replaceButtonClicked(QString,QList<Find::SearchResultItem>,bool)),
                    SLOT(onReplaceButtonClicked(QString,QList<Find::SearchResultItem>,bool)));
        }
        connect(m_currentSearch, SIGNAL(activated(Find::SearchResultItem)),
                this, SLOT(openEditor(Find::SearchResultItem)));
        connect(m_currentSearch, SIGNAL(cancelled()), this, SLOT(cancel()));
        connect(m_currentSearch, SIGNAL(paused(bool)), this, SLOT(setPaused(bool)));
        Find::SearchResultWindow::instance()->popup(Core::IOutputPane::Flags(Core::IOutputPane::ModeSwitch | Core::IOutputPane::WithFocus));

        Core::ProgressManager *progressManager = Core::ICore::progressManager();
        Core::FutureProgress *progress = progressManager->addTask(
                    m_watcher.future(), tr("Searching"),
                    QLatin1String(QmlJSEditor::Constants::TASK_SEARCH));
        connect(progress, SIGNAL(clicked()), m_currentSearch, SLOT(popup()));

        ++first;
    }

    if (!m_currentSearch) {
        m_watcher.cancel();
        return;
    }
    for (int index = first; index != last; ++index) {
        Usage result = m_watcher.future().resultAt(index);
        m_currentSearch->addResult(result.path,
                                 result.line,
                                 result.lineText,
                                 result.col,
                                 result.len);
    }
}

void FindReferences::searchFinished()
{
    if (m_currentSearch)
        m_currentSearch->finishSearch(m_watcher.isCanceled());
    m_currentSearch = 0;
    emit changed();
}

void FindReferences::cancel()
{
    m_watcher.cancel();
}

void FindReferences::setPaused(bool paused)
{
    if (!paused || m_watcher.isRunning()) // guard against pausing when the search is finished
        m_watcher.setPaused(paused);
}

void FindReferences::openEditor(const Find::SearchResultItem &item)
{
    if (item.path.size() > 0) {
        TextEditor::BaseTextEditorWidget::openEditorAt(QDir::fromNativeSeparators(item.path.first()),
                                                       item.lineNumber, item.textMarkPos, Core::Id(),
                                                       Core::EditorManager::ModeSwitch);
    } else {
        Core::EditorManager::openEditor(QDir::fromNativeSeparators(item.text),
                                        Core::Id(), Core::EditorManager::ModeSwitch);
    }
}

void FindReferences::onReplaceButtonClicked(const QString &text, const QList<Find::SearchResultItem> &items, bool preserveCase)
{
    const QStringList fileNames = TextEditor::BaseFileFind::replaceAll(text, items, preserveCase);

    // files that are opened in an editor are changed, but not saved
    QStringList changedOnDisk;
    QStringList changedUnsavedEditors;
    Core::EditorManager *editorManager = Core::EditorManager::instance();
    foreach (const QString &fileName, fileNames) {
        if (editorManager->editorsForFileName(fileName).isEmpty())
            changedOnDisk += fileName;
        else
            changedUnsavedEditors += fileName;
    }

    if (!changedOnDisk.isEmpty())
        QmlJS::ModelManagerInterface::instance()->updateSourceFiles(changedOnDisk, true);
    if (!changedUnsavedEditors.isEmpty())
        QmlJS::ModelManagerInterface::instance()->updateSourceFiles(changedUnsavedEditors, false);

    Find::SearchResultWindow::instance()->hide();
}
