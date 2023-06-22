// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmljseditortr.h"
#include "qmljsfindreferences.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/find/searchresultwindow.h>
#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/futureprogress.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <extensionsystem/pluginmanager.h>
#include <texteditor/basefilefind.h>
#include <utils/algorithm.h>
#include <utils/async.h>
#include <utils/filesearch.h>

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

#include <QDebug>
#include <QFuture>
#include <QtConcurrentMap>

using namespace Core;
using namespace QmlJS;
using namespace QmlJS::AST;
using namespace QmlJSEditor;

namespace {

// ### These visitors could be useful in general

class FindUsages: protected Visitor
{
public:
    using Result = QList<SourceLocation>;

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
    void accept(AST::Node *node) { AST::Node::accept(node, this); }

    using Visitor::visit;

    bool visit(AST::UiPublicMember *node) override
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

    bool visit(AST::UiObjectDefinition *node) override
    {
        _builder.push(node);
        Node::accept(node->initializer, this);
        _builder.pop();
        return false;
    }

    bool visit(AST::TemplateLiteral *el) override
    {
        Node::accept(el->expression, this);
        return true;
    }

    bool visit(AST::UiObjectBinding *node) override
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

    bool visit(AST::UiScriptBinding *node) override
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

    bool visit(AST::UiArrayBinding *node) override
    {
        if (node->qualifiedId
                && !node->qualifiedId->next
                && node->qualifiedId->name == _name
                && checkQmlScope()) {
            _usages.append(node->qualifiedId->identifierToken);
        }
        return true;
    }

    bool visit(AST::IdentifierExpression *node) override
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

    bool visit(AST::FieldMemberExpression *node) override
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

    bool visit(AST::FunctionDeclaration *node) override
    {
        return visit(static_cast<FunctionExpression *>(node));
    }

    bool visit(AST::FunctionExpression *node) override
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

    bool visit(AST::PatternElement *node) override
    {
        if (node->isVariableDeclaration() && node->bindingIdentifier == _name) {
            if (checkLookup())
                _usages.append(node->identifierToken);
        }
        return true;
    }

    void throwRecursionDepthError() override
    {
        qWarning("Warning: Hit maximum recursion depth while visitin AST in FindUsages");
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

        const QList<const QmlComponentChain *> parents = chain->instantiatingComponents();
        for (const QmlComponentChain *parent : parents) {
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
        const QList<const ObjectValue *> scopes = _scopeChain.qmlScopeObjects();
        for (const ObjectValue *s : scopes) {
            if (check(s))
                return true;
        }
        return false;
    }

    bool checkLookup()
    {
        const ObjectValue *scope = nullptr;
        _scopeChain.lookup(_name, &scope);
        return check(scope);
    }

    Result _usages;

    Document::Ptr _doc;
    ScopeChain _scopeChain;
    ScopeBuilder _builder;

    QString _name;
    const ObjectValue *_scope = nullptr;
};

class FindTypeUsages: protected Visitor
{
public:
    using Result = QList<SourceLocation>;

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
    void accept(AST::Node *node) { AST::Node::accept(node, this); }

    using Visitor::visit;

    bool visit(AST::UiPublicMember *node) override
    {
        if (UiQualifiedId *memberType = node->memberType) {
            if (memberType->name == _name) {
                const ObjectValue * tVal = _context->lookupType(_doc.data(), QStringList(_name));
                if (tVal == _typeValue)
                    _usages.append(node->typeToken);
            }
        }
        if (AST::cast<Block *>(node->statement)) {
            _builder.push(node);
            Node::accept(node->statement, this);
            _builder.pop();
            return false;
        }
        return true;
    }

    bool visit(AST::UiObjectDefinition *node) override
    {
        checkTypeName(node->qualifiedTypeNameId);
        _builder.push(node);
        Node::accept(node->initializer, this);
        _builder.pop();
        return false;
    }

    bool visit(AST::UiObjectBinding *node) override
    {
        checkTypeName(node->qualifiedTypeNameId);
        _builder.push(node);
        Node::accept(node->initializer, this);
        _builder.pop();
        return false;
    }

    bool visit(AST::UiScriptBinding *node) override
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

    bool visit(AST::IdentifierExpression *node) override
    {
        if (node->name != _name)
            return false;

        const ObjectValue *scope;
        const Value *objV = _scopeChain.lookup(_name, &scope);
        if (objV == _typeValue)
            _usages.append(node->identifierToken);
        return false;
    }

    bool visit(AST::FieldMemberExpression *node) override
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

    bool visit(AST::TemplateLiteral *el) override
    {
        Node::accept(el->expression, this);
        return true;
    }

    bool visit(AST::FunctionDeclaration *node) override
    {
        return visit(static_cast<FunctionExpression *>(node));
    }

    bool visit(AST::FunctionExpression *node) override
    {
        Node::accept(node->formals, this);
        _builder.push(node);
        Node::accept(node->body, this);
        _builder.pop();
        return false;
    }

    bool visit(AST::PatternElement *node) override
    {
        if (node->isVariableDeclaration())
            Node::accept(node->initializer, this);
        return false;
    }

    bool visit(UiImport *ast) override
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

    void throwRecursionDepthError() override
    {
        qWarning("Warning: Hit maximum recursion depth while visitin AST in FindTypeUsages");
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
    const ObjectValue *_typeValue = nullptr;
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
        _scope = nullptr;
        _objectNode = nullptr;
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
    void accept(AST::Node *node) { AST::Node::accept(node, this); }

    using Visitor::visit;

    bool preVisit(Node *node) override
    {
        if (Statement *stmt = node->statementCast())
            return containsOffset(stmt->firstSourceLocation(), stmt->lastSourceLocation());
        else if (ExpressionNode *exp = node->expressionCast())
            return containsOffset(exp->firstSourceLocation(), exp->lastSourceLocation());
        else if (UiObjectMember *ui = node->uiObjectMemberCast())
            return containsOffset(ui->firstSourceLocation(), ui->lastSourceLocation());
        return true;
    }

    bool visit(IdentifierExpression *node) override
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

    bool visit(FieldMemberExpression *node) override
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

    bool visit(AST::TemplateLiteral *el) override
    {
        Node::accept(el->expression, this);
        return true;
    }

    bool visit(UiScriptBinding *node) override
    {
        return !checkBindingName(node->qualifiedId);
    }

    bool visit(UiArrayBinding *node) override
    {
        return !checkBindingName(node->qualifiedId);
    }

    bool visit(UiObjectBinding *node) override
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

    bool visit(UiObjectDefinition *node) override
    {
        if (!checkTypeName(node->qualifiedTypeNameId)) {
            Node *oldObjectNode = _objectNode;
            _objectNode = node;
            accept(node->initializer);
            _objectNode = oldObjectNode;
        }
        return false;
    }

    bool visit(UiPublicMember *node) override
    {
        if (containsOffset(node->typeToken)){
            if (node->defaultToken().isValid()) {
                _name = node->memberType->name.toString();
                _targetValue = _scopeChain->context()->lookupType(_doc.data(), QStringList(_name));
                _scope = nullptr;
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

    bool visit(FunctionDeclaration *node) override
    {
        return visit(static_cast<FunctionExpression *>(node));
    }

    bool visit(FunctionExpression *node) override
    {
        if (containsOffset(node->identifierToken)) {
            _name = node->name.toString();
            return false;
        }
        return true;
    }

    bool visit(PatternElement *node) override
    {
        if (node->isVariableDeclaration() && containsOffset(node->identifierToken)) {
            _name = node->bindingIdentifier.toString();
            return false;
        }
        return true;
    }

    void throwRecursionDepthError() override
    {
        qWarning("Warning: Hit maximum recursion depth visiting AST in FindUsages");
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
                _scope = nullptr;
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
    const ObjectValue *_scope = nullptr;
    const Value *_targetValue = nullptr;
    Node *_objectNode = nullptr;
    Document::Ptr _doc;
    const ScopeChain *_scopeChain = nullptr;
    quint32 _offset = 0;
    Kind _typeKind = ExpKind;
};

static QString matchingLine(unsigned position, const QString &source)
{
    int start = source.lastIndexOf(QLatin1Char('\n'), position);
    start += 1;
    int end = source.indexOf(QLatin1Char('\n'), position);

    return source.mid(start, end - start);
}

class ProcessFile
{
    ContextPtr context;
    using Usage = FindReferences::Usage;
    const QString name;
    const ObjectValue *scope;
    QPromise<Usage> &m_promise;

public:
    // needed by QtConcurrent
    using argument_type = const QString &;
    using result_type = QList<Usage>;

    ProcessFile(const ContextPtr &context,
                const QString &name,
                const ObjectValue *scope,
                QPromise<Usage> &promise)
        : context(context), name(name), scope(scope), m_promise(promise)
    { }

    QList<Usage> operator()(const Utils::FilePath &fileName)
    {
        QList<Usage> usages;
        m_promise.suspendIfRequested();
        if (m_promise.isCanceled())
            return usages;
        ModelManagerInterface *modelManager = ModelManagerInterface::instance();
        Document::Ptr doc = context->snapshot().document(fileName);
        if (!doc)
            return usages;

        // find all idenfifier expressions, try to resolve them and check if the result is in scope
        FindUsages findUsages(doc, context);
        const FindUsages::Result results = findUsages(name, scope);
        for (const SourceLocation &loc : results)
            usages.append(Usage(modelManager->fileToSource(fileName),
                                matchingLine(loc.offset, doc->source()),
                                loc.startLine,
                                loc.startColumn - 1,
                                loc.length));
        m_promise.suspendIfRequested();
        return usages;
    }
};

class SearchFileForType
{
    ContextPtr context;
    using Usage = FindReferences::Usage;
    const QString name;
    const ObjectValue *scope;
    QPromise<Usage> &m_promise;

public:
    // needed by QtConcurrent
    using argument_type = const QString &;
    using result_type = QList<Usage>;

    SearchFileForType(const ContextPtr &context,
                      const QString &name,
                      const ObjectValue *scope,
                      QPromise<Usage> &promise)
        : context(context), name(name), scope(scope), m_promise(promise)
    { }

    QList<Usage> operator()(const Utils::FilePath &fileName)
    {
        QList<Usage> usages;
        m_promise.suspendIfRequested();
        if (m_promise.isCanceled())
            return usages;
        Document::Ptr doc = context->snapshot().document(fileName);
        if (!doc)
            return usages;

        // find all idenfifier expressions, try to resolve them and check if the result is in scope
        FindTypeUsages findUsages(doc, context);
        const FindTypeUsages::Result results = findUsages(name, scope);
        for (const SourceLocation &loc : results)
            usages.append(Usage(fileName, matchingLine(loc.offset, doc->source()), loc.startLine, loc.startColumn - 1, loc.length));
        m_promise.suspendIfRequested();
        return usages;
    }
};

class UpdateUI
{
    using Usage = FindReferences::Usage;
    QPromise<Usage> &m_promise;

public:
    // needed by QtConcurrent
    using first_argument_type = QList<Usage> &;
    using second_argument_type = const QList<Usage> &;
    using result_type = void;

    UpdateUI(QPromise<Usage> &promise): m_promise(promise) {}

    void operator()(QList<Usage> &, const QList<Usage> &usages)
    {
        for (const Usage &u : usages)
            m_promise.addResult(u);
        m_promise.setProgressValue(m_promise.future().progressValue() + 1);
    }
};

} // end of anonymous namespace

FindReferences::FindReferences(QObject *parent)
    : QObject(parent)
{
    m_watcher.setPendingResultsLimit(1);
    connect(&m_watcher, &QFutureWatcherBase::resultsReadyAt, this, &FindReferences::displayResults);
    connect(&m_watcher, &QFutureWatcherBase::finished, this, &FindReferences::searchFinished);
}

FindReferences::~FindReferences() = default;

static void find_helper(QPromise<FindReferences::Usage> &promise,
                        const ModelManagerInterface::WorkingCopy &workingCopy,
                        Snapshot snapshot,
                        const Utils::FilePath &fileName,
                        quint32 offset,
                        QString replacement)
{
    // update snapshot from workingCopy to make sure it's up to date
    // ### remove?
    // ### this is a great candidate for map-reduce
    const ModelManagerInterface::WorkingCopy::Table &all = workingCopy.all();
    for (auto it = all.cbegin(), end = all.cend(); it != end; ++it) {
        const Utils::FilePath fileName = it.key();
        Document::Ptr oldDoc = snapshot.document(fileName);
        if (oldDoc && oldDoc->editorRevision() == it.value().second)
            continue;

        Dialect language;
        if (oldDoc)
            language = oldDoc->language();
        else
            language = ModelManagerInterface::guessLanguageOfFile(fileName);
        if (language == Dialect::NoLanguage) {
            qCDebug(qmljsLog) << "NoLanguage in qmljsfindreferences.cpp find_helper for " << fileName;
            language = Dialect::AnyLanguage;
        }

        Document::MutablePtr newDoc = snapshot.documentFromSource(
                    it.value().first, fileName, language);
        newDoc->parse();
        snapshot.insert(newDoc);
    }

    // find the scope for the name we're searching
    Document::Ptr doc = snapshot.document(fileName);
    if (!doc)
        return;

    ModelManagerInterface *modelManager = ModelManagerInterface::instance();

    Link link(snapshot, modelManager->defaultVContext(doc->language(), doc), modelManager->builtins(doc));
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

    Utils::FilePaths files;
    for (const Document::Ptr &doc : std::as_const(snapshot)) {
        // ### skip files that don't contain the name token
        files.append(modelManager->fileToSource(doc->fileName()));
    }
    files = Utils::filteredUnique(files);

    promise.setProgressRange(0, files.size());

    // report a dummy usage to indicate the search is starting
    FindReferences::Usage searchStarting(Utils::FilePath::fromString(replacement), name, 0, 0, 0);

    if (findTarget.typeKind() == findTarget.TypeKind){
        const ObjectValue *typeValue = value_cast<ObjectValue>(findTarget.targetValue());
        if (!typeValue)
            return;
        promise.addResult(searchStarting);

        SearchFileForType process(context, name, typeValue, promise);
        UpdateUI reduce(promise);

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
        promise.addResult(searchStarting);

        ProcessFile process(context, name, scope, promise);
        UpdateUI reduce(promise);

        QtConcurrent::blockingMappedReduced<QList<FindReferences::Usage> > (files, process, reduce);
    }
    promise.setProgressValue(files.size());
}

void FindReferences::findUsages(const Utils::FilePath &fileName, quint32 offset)
{
    ModelManagerInterface *modelManager = ModelManagerInterface::instance();

    QFuture<Usage> result = Utils::asyncRun(&find_helper, ModelManagerInterface::workingCopy(),
                                            modelManager->snapshot(), fileName, offset, QString());
    m_watcher.setFuture(result);
    m_synchronizer.addFuture(result);
}

void FindReferences::renameUsages(const Utils::FilePath &fileName,
                                  quint32 offset,
                                  const QString &replacement)
{
    ModelManagerInterface *modelManager = ModelManagerInterface::instance();

    // an empty non-null string asks the future to use the current name as base
    QString newName = replacement;
    if (newName.isNull())
        newName = QLatin1String("");

    QFuture<Usage> result = Utils::asyncRun(&find_helper, ModelManagerInterface::workingCopy(),
                                            modelManager->snapshot(), fileName, offset, newName);
    m_watcher.setFuture(result);
    m_synchronizer.addFuture(result);
}

QList<FindReferences::Usage> FindReferences::findUsageOfType(const Utils::FilePath &fileName,
                                                             const QString &typeName)
{
    QList<Usage> usages;
    ModelManagerInterface *modelManager = ModelManagerInterface::instance();

    Document::Ptr doc = modelManager->snapshot().document(fileName);
    if (!doc)
        return usages;

    Link link(modelManager->snapshot(), modelManager->defaultVContext(doc->language(), doc), modelManager->builtins(doc));
    ContextPtr context = link();
    ScopeChain scopeChain(doc, context);

    const ObjectValue *targetValue = scopeChain.context()->lookupType(doc.data(), QStringList(typeName));

    QmlJS::Snapshot snapshot =  modelManager->snapshot();

    QSet<Utils::FilePath> docDone;
    for (const QmlJS::Document::Ptr &doc : std::as_const(snapshot)) {
        Utils::FilePath sourceFile = modelManager->fileToSource(doc->fileName());
        if (!Utils::insert(docDone, sourceFile))
            continue;
        QmlJS::Document::Ptr sourceDoc = doc;
        if (sourceFile != doc->fileName())
            sourceDoc = snapshot.document(sourceFile);
        FindTypeUsages findUsages(sourceDoc, context);
        const FindTypeUsages::Result results = findUsages(typeName, targetValue);
        for (const SourceLocation &loc : results) {
            usages.append(Usage(sourceFile,
                                matchingLine(loc.offset, doc->source()),
                                loc.startLine,
                                loc.startColumn - 1,
                                loc.length));
        }
    }
    return usages;
}

void FindReferences::displayResults(int first, int last)
{
    // the first usage is always a dummy to indicate we now start searching
    if (first == 0) {
        Usage dummy = m_watcher.future().resultAt(0);
        const QString replacement = dummy.path.toString();
        const QString symbolName = dummy.lineText;
        const QString label = Tr::tr("QML/JS Usages:");

        if (replacement.isEmpty()) {
            m_currentSearch = SearchResultWindow::instance()->startNewSearch(
                        label, QString(), symbolName, SearchResultWindow::SearchOnly);
        } else {
            m_currentSearch = SearchResultWindow::instance()->startNewSearch(
                        label, QString(), symbolName, SearchResultWindow::SearchAndReplace,
                        SearchResultWindow::PreserveCaseDisabled);
            m_currentSearch->setTextToReplace(replacement);
            connect(m_currentSearch.data(), &SearchResult::replaceButtonClicked,
                    this, &FindReferences::onReplaceButtonClicked);
        }
        connect(m_currentSearch.data(), &SearchResult::activated,
                [](const Utils::SearchResultItem &item) {
                    Core::EditorManager::openEditorAtSearchResult(item);
                });
        connect(m_currentSearch.data(), &SearchResult::canceled, this, &FindReferences::cancel);
        connect(m_currentSearch.data(), &SearchResult::paused, this, &FindReferences::setPaused);
        SearchResultWindow::instance()->popup(IOutputPane::Flags(IOutputPane::ModeSwitch | IOutputPane::WithFocus));

        FutureProgress *progress = ProgressManager::addTask(m_watcher.future(),
                                                            Tr::tr("Searching for Usages"),
                                                            "QmlJSEditor.TaskSearch");
        connect(progress, &FutureProgress::clicked, m_currentSearch.data(), &SearchResult::popup);

        ++first;
    }

    if (!m_currentSearch) {
        m_watcher.cancel();
        return;
    }
    for (int index = first; index != last; ++index) {
        Usage result = m_watcher.future().resultAt(index);
        Utils::SearchResultItem item;
        item.setFilePath(result.path);
        item.setLineText(result.lineText);
        item.setMainRange(result.line, result.col, result.len);
        item.setUseTextEditorFont(true);
        m_currentSearch->addResult(item);
    }
}

void FindReferences::searchFinished()
{
    if (m_currentSearch)
        m_currentSearch->finishSearch(m_watcher.isCanceled());
    m_currentSearch = nullptr;
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

void FindReferences::onReplaceButtonClicked(const QString &text,
                     const Utils::SearchResultItems &items, bool preserveCase)
{
    const Utils::FilePaths filePaths = TextEditor::BaseFileFind::replaceAll(text,
                                                                            items,
                                                                            preserveCase);

    // files that are opened in an editor are changed, but not saved
    Utils::FilePaths changedOnDisk;
    Utils::FilePaths changedUnsavedEditors;
    for (const Utils::FilePath &filePath : filePaths) {
        if (DocumentModel::documentForFilePath(filePath))
            changedOnDisk += filePath;
        else
            changedUnsavedEditors += filePath;
    }

    if (!changedOnDisk.isEmpty())
        ModelManagerInterface::instance()->updateSourceFiles(changedOnDisk, true);
    if (!changedUnsavedEditors.isEmpty())
        ModelManagerInterface::instance()->updateSourceFiles(changedUnsavedEditors, false);

    SearchResultWindow::instance()->hide();
}
