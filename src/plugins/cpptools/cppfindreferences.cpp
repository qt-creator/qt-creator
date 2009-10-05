/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
** contact the sales department at http://www.qtsoftware.com/contact.
**
**************************************************************************/

#include "cppfindreferences.h"
#include "cppmodelmanager.h"
#include "cpptoolsconstants.h"

#include <texteditor/basetexteditor.h>
#include <find/searchresultwindow.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/filesearch.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <coreplugin/icore.h>

#include <ASTVisitor.h>
#include <AST.h>
#include <Control.h>
#include <Literals.h>
#include <TranslationUnit.h>
#include <Symbols.h>
#include <Names.h>
#include <Scope.h>

#include <cplusplus/CppDocument.h>
#include <cplusplus/ExpressionUnderCursor.h>
#include <cplusplus/ResolveExpression.h>
#include <cplusplus/Overview.h>
#include <cplusplus/TypeOfExpression.h>
#include <cplusplus/FastPreprocessor.h>

#include <QtCore/QTime>
#include <QtCore/QtConcurrentRun>
#include <QtCore/QDir>

#include <qtconcurrent/runextensions.h>

using namespace CppTools::Internal;
using namespace CPlusPlus;

namespace {

struct Process: protected ASTVisitor
{
public:
    Process(Document::Ptr doc, const Snapshot &snapshot,
            QFutureInterface<Utils::FileSearchResult> *future)
            : ASTVisitor(doc->control()),
              _future(future),
              _doc(doc),
              _snapshot(snapshot),
              _source(_doc->source()),
              _sem(doc->control())
    {
        _snapshot.insert(_doc);
    }

    QList<int> operator()(Symbol *symbol, Identifier *id, AST *ast)
    {
        _references.clear();
        _declSymbol = symbol;
        _id = id;
        _exprDoc = Document::create("<references>");
        accept(ast);
        return _references;
    }

protected:
    using ASTVisitor::visit;

    QString matchingLine(const Token &tk) const
    {
        const char *beg = _source.constData();
        const char *cp = beg + tk.offset;
        for (; cp != beg - 1; --cp) {
            if (*cp == '\n')
                break;
        }

        ++cp;

        const char *lineEnd = cp + 1;
        for (; *lineEnd; ++lineEnd) {
            if (*lineEnd == '\n')
                break;
        }

        const QString matchingLine = QString::fromUtf8(cp, lineEnd - cp);
        return matchingLine;

    }

    void reportResult(unsigned tokenIndex, const QList<Symbol *> &candidates)
    {
        const bool isStrongResult = checkCandidates(candidates);

        if (isStrongResult)
            reportResult(tokenIndex);
    }

    void reportResult(unsigned tokenIndex)
    {
        const Token &tk = tokenAt(tokenIndex);
        const QString lineText = matchingLine(tk);

        unsigned line, col;
        getTokenStartPosition(tokenIndex, &line, &col);

        if (col)
            --col;  // adjust the column position.

        const int len = tk.f.length;

        if (_future)
            _future->reportResult(Utils::FileSearchResult(QDir::toNativeSeparators(_doc->fileName()),
                                                          line, lineText, col, len));

        _references.append(tokenIndex);
    }

    bool checkCandidates(const QList<Symbol *> &candidates) const
    {
        if (Symbol *canonicalSymbol = LookupContext::canonicalSymbol(candidates)) {
#if 0
            qDebug() << "*** canonical symbol:" << canonicalSymbol->fileName()
                    << canonicalSymbol->line() << canonicalSymbol->column()
                    << "candidates:" << candidates.size();
#endif

            return isDeclSymbol(canonicalSymbol);
        }

        return false;
    }

    bool isDeclSymbol(Symbol *symbol) const
    {
        if (! symbol)
            return false;

        else if (symbol == _declSymbol)
            return true;

        else if (symbol->line() == _declSymbol->line() && symbol->column() == _declSymbol->column()) {
            if (! qstrcmp(symbol->fileName(), _declSymbol->fileName()))
                return true;
        }

        return false;
    }

    LookupContext _previousContext;

    LookupContext currentContext(AST *ast)
    {
        unsigned line, column;
        getTokenStartPosition(ast->firstToken(), &line, &column);
        Symbol *lastVisibleSymbol = _doc->findSymbolAt(line, column);

        if (lastVisibleSymbol && lastVisibleSymbol == _previousContext.symbol())
            return _previousContext;

        LookupContext ctx(lastVisibleSymbol, _exprDoc, _doc, _snapshot);
        _previousContext = ctx;
        return ctx;
    }

    void ensureNameIsValid(NameAST *ast)
    {
        if (ast && ! ast->name)
            ast->name = _sem.check(ast, /*scope = */ 0);
    }

    virtual bool visit(MemInitializerAST *ast)
    {
        if (ast->name && ast->name->asSimpleName() != 0) {
            ensureNameIsValid(ast->name);

            SimpleNameAST *simple = ast->name->asSimpleName();
            if (identifier(simple->identifier_token) == _id) {
                LookupContext context = currentContext(ast);
                const QList<Symbol *> candidates = context.resolve(simple->name);
                reportResult(simple->identifier_token, candidates);
            }
        }
        accept(ast->expression);
        return false;
    }

    virtual bool visit(PostfixExpressionAST *ast)
    {
        _postfixExpressionStack.append(ast);
        return true;
    }

    virtual void endVisit(PostfixExpressionAST *)
    {
        _postfixExpressionStack.removeLast();
    }

    virtual bool visit(MemberAccessAST *ast)
    {
        if (ast->member_name) {
            if (SimpleNameAST *simple = ast->member_name->asSimpleName()) {
                if (identifier(simple->identifier_token) == _id) {
                    Q_ASSERT(! _postfixExpressionStack.isEmpty());

                    checkExpression(_postfixExpressionStack.last()->firstToken(),
                                    simple->identifier_token);

                    return false;
                }
            }
        }

        return true;
    }

    void checkExpression(unsigned startToken, unsigned endToken)
    {
        const unsigned begin = tokenAt(startToken).begin();
        const unsigned end = tokenAt(endToken).end();

        const QString expression = _source.mid(begin, end - begin);
        // qDebug() << "*** check expression:" << expression;

        TypeOfExpression typeofExpression;
        typeofExpression.setSnapshot(_snapshot);

        unsigned line, column;
        getTokenStartPosition(startToken, &line, &column);
        Symbol *lastVisibleSymbol = _doc->findSymbolAt(line, column);

        const QList<TypeOfExpression::Result> results =
                typeofExpression(expression, _doc, lastVisibleSymbol,
                                 TypeOfExpression::Preprocess);

        QList<Symbol *> candidates;

        foreach (TypeOfExpression::Result r, results) {
            FullySpecifiedType ty = r.first;
            Symbol *lastVisibleSymbol = r.second;

            candidates.append(lastVisibleSymbol);
        }

        reportResult(endToken, candidates);
    }

    virtual bool visit(QualifiedNameAST *ast)
    {
        for (NestedNameSpecifierAST *nested_name_specifier = ast->nested_name_specifier;
             nested_name_specifier; nested_name_specifier = nested_name_specifier->next) {

            if (NameAST *class_or_namespace_name = nested_name_specifier->class_or_namespace_name) {
                SimpleNameAST *simple_name = class_or_namespace_name->asSimpleName();

                TemplateIdAST *template_id = 0;
                if (! simple_name) {
                    template_id = class_or_namespace_name->asTemplateId();

                    if (template_id) {
                        for (TemplateArgumentListAST *template_arguments = template_id->template_arguments;
                             template_arguments; template_arguments = template_arguments->next) {
                            accept(template_arguments->template_argument);
                        }
                    }
                }

                if (simple_name || template_id) {
                    const unsigned identifier_token = simple_name
                               ? simple_name->identifier_token
                               : template_id->identifier_token;

                    if (identifier(identifier_token) == _id)
                        checkExpression(ast->firstToken(), identifier_token);
                }
            }
        }

        if (ast->unqualified_name) {
            SimpleNameAST *simple_name = ast->unqualified_name->asSimpleName();

            TemplateIdAST *template_id = 0;
            if (! simple_name) {
                template_id = ast->unqualified_name->asTemplateId();

                if (template_id) {
                    for (TemplateArgumentListAST *template_arguments = template_id->template_arguments;
                         template_arguments; template_arguments = template_arguments->next) {
                        accept(template_arguments->template_argument);
                    }
                }
            }

            if (simple_name || template_id) {
                const unsigned identifier_token = simple_name
                        ? simple_name->identifier_token
                        : template_id->identifier_token;

                if (identifier(identifier_token) == _id)
                    checkExpression(ast->firstToken(), identifier_token);
            }
        }

        return false;
    }

    virtual bool visit(SimpleNameAST *ast)
    {
        Identifier *id = identifier(ast->identifier_token);
        if (id == _id) {
            LookupContext context = currentContext(ast);
            const QList<Symbol *> candidates = context.resolve(ast->name);
            reportResult(ast->identifier_token, candidates);
        }

        return false;
    }

    virtual bool visit(DestructorNameAST *ast)
    {
        Identifier *id = identifier(ast->identifier_token);
        if (id == _id) {
            LookupContext context = currentContext(ast);
            const QList<Symbol *> candidates = context.resolve(ast->name);
            reportResult(ast->identifier_token, candidates);
        }

        return false;
    }

    virtual bool visit(TemplateIdAST *ast)
    {
        if (_id == identifier(ast->identifier_token)) {
            LookupContext context = currentContext(ast);
            const QList<Symbol *> candidates = context.resolve(ast->name);
            reportResult(ast->identifier_token, candidates);
        }

        for (TemplateArgumentListAST *template_arguments = ast->template_arguments;
             template_arguments; template_arguments = template_arguments->next) {
            accept(template_arguments->template_argument);
        }

        return false;
    }

    virtual bool visit(ParameterDeclarationAST *ast)
    {
        for (SpecifierAST *spec = ast->type_specifier; spec; spec = spec->next)
            accept(spec);

        if (DeclaratorAST *declarator = ast->declarator) {
            for (SpecifierAST *attr = declarator->attributes; attr; attr = attr->next)
                accept(attr);

            for (PtrOperatorAST *ptr_op = declarator->ptr_operators; ptr_op; ptr_op = ptr_op->next)
                accept(ptr_op);

            // ### TODO: well, not exactly. We need to look at qualified-name-ids and nested-declarators.
            // accept(declarator->core_declarator);

            for (PostfixDeclaratorAST *fx_op = declarator->postfix_declarators; fx_op; fx_op = fx_op->next)
                accept(fx_op);

            for (SpecifierAST *spec = declarator->post_attributes; spec; spec = spec->next)
                accept(spec);

            accept(declarator->initializer);
        }

        accept(ast->expression);
        return false;
    }

private:
    QFutureInterface<Utils::FileSearchResult> *_future;
    Identifier *_id; // ### remove me
    Symbol *_declSymbol;
    Document::Ptr _doc;
    Snapshot _snapshot;
    QByteArray _source;
    Document::Ptr _exprDoc;
    Semantic _sem;
    QList<PostfixExpressionAST *> _postfixExpressionStack;
    QList<QualifiedNameAST *> _qualifiedNameStack;
    QList<int> _references;
};

} // end of anonymous namespace

CppFindReferences::CppFindReferences(CppModelManager *modelManager)
    : _modelManager(modelManager),
      _resultWindow(ExtensionSystem::PluginManager::instance()->getObject<Find::SearchResultWindow>())
{
    m_watcher.setPendingResultsLimit(1);
    connect(&m_watcher, SIGNAL(resultReadyAt(int)), this, SLOT(displayResult(int)));
    connect(&m_watcher, SIGNAL(finished()), this, SLOT(searchFinished()));
}

CppFindReferences::~CppFindReferences()
{
}

QList<int> CppFindReferences::references(Symbol *symbol,
                                         Document::Ptr doc,
                                         const Snapshot& snapshot) const
{
    Identifier *id = 0;
    if (Identifier *symbolId = symbol->identifier())
        id = doc->control()->findIdentifier(symbolId->chars(), symbolId->size());

    QList<int> references;

    if (! id)
        return references;

    TranslationUnit *translationUnit = doc->translationUnit();
    Q_ASSERT(translationUnit != 0);

    Process process(doc, snapshot, /*future = */ 0);
    references = process(symbol, id, translationUnit->ast());

    return references;
}

static void find_helper(QFutureInterface<Utils::FileSearchResult> &future,
                        const QMap<QString, QString> wl,
                        Snapshot snapshot,
                        Symbol *symbol)
{
    QTime tm;
    tm.start();

    Identifier *symbolId = symbol->identifier();
    Q_ASSERT(symbolId != 0);

    const QString sourceFile = QString::fromUtf8(symbol->fileName(), symbol->fileNameLength());

    QStringList files(sourceFile);
    files += snapshot.dependsOn(sourceFile);
    qDebug() << "done in:" << tm.elapsed() << "number of files to parse:" << files.size();

    future.setProgressRange(0, files.size());

    for (int i = 0; i < files.size(); ++i) {
        if (future.isPaused())
            future.waitForResume();

        if (future.isCanceled())
            break;

        const QString &fileName = files.at(i);
        future.setProgressValueAndText(i, QFileInfo(fileName).fileName());

        if (Document::Ptr previousDoc = snapshot.value(fileName)) {
            Control *control = previousDoc->control();
            Identifier *id = control->findIdentifier(symbolId->chars(), symbolId->size());
            if (! id)
                continue; // skip this document, it's not using symbolId.
        }

        QByteArray source;

        if (wl.contains(fileName))
            source = snapshot.preprocessedCode(wl.value(fileName), fileName);
        else {
            QFile file(fileName);
            if (! file.open(QFile::ReadOnly))
                continue;

            const QString contents = QTextStream(&file).readAll(); // ### FIXME
            source = snapshot.preprocessedCode(contents, fileName);
        }

        Document::Ptr doc = snapshot.documentFromSource(source, fileName);
        doc->tokenize();

        Control *control = doc->control();
        if (Identifier *id = control->findIdentifier(symbolId->chars(), symbolId->size())) {
            QTime tm;
            tm.start();
            TranslationUnit *unit = doc->translationUnit();
            Control *control = doc->control();

            FastMacroResolver fastMacroResolver(unit, snapshot);
            control->setMacroResolver(&fastMacroResolver);
            doc->parse();
            control->setMacroResolver(0);

            //qDebug() << "***" << unit->fileName() << "parsed in:" << tm.elapsed();

            tm.start();
            doc->check();
            //qDebug() << "***" << unit->fileName() << "checked in:" << tm.elapsed();

            tm.start();

            Process process(doc, snapshot, &future);
            process(symbol, id, unit->ast());

            //qDebug() << "***" << unit->fileName() << "processed in:" << tm.elapsed();
        }
    }

    future.setProgressValue(files.size());
}

void CppFindReferences::findUsages(Symbol *symbol)
{
    _resultWindow->clearContents();
    findAll_helper(symbol);
}

void CppFindReferences::renameUsages(Symbol *symbol)
{
    _resultWindow->clearContents();
    _resultWindow->setShowReplaceUI(true);
    findAll_helper(symbol);
}

void CppFindReferences::findAll_helper(Symbol *symbol)
{
    _resultWindow->popup(true);

    const Snapshot snapshot = _modelManager->snapshot();
    const QMap<QString, QString> wl = _modelManager->buildWorkingCopyList();

    Core::ProgressManager *progressManager = Core::ICore::instance()->progressManager();

    QFuture<Utils::FileSearchResult> result = QtConcurrent::run(&find_helper, wl, snapshot, symbol);
    m_watcher.setFuture(result);

    Core::FutureProgress *progress = progressManager->addTask(result, tr("Searching..."),
                                                              CppTools::Constants::TASK_SEARCH,
                                                              Core::ProgressManager::CloseOnSuccess);

    connect(progress, SIGNAL(clicked()), _resultWindow, SLOT(popup()));
}

void CppFindReferences::displayResult(int index)
{
    Utils::FileSearchResult result = m_watcher.future().resultAt(index);
    Find::ResultWindowItem *item = _resultWindow->addResult(result.fileName,
                                                            result.lineNumber,
                                                            result.matchingLine,
                                                            result.matchStart,
                                                            result.matchLength);
    if (item)
        connect(item, SIGNAL(activated(const QString&,int,int)),
                this, SLOT(openEditor(const QString&,int,int)));
}

void CppFindReferences::searchFinished()
{
    emit changed();
}

void CppFindReferences::openEditor(const QString &fileName, int line, int column)
{
    TextEditor::BaseTextEditor::openEditorAt(fileName, line, column);
}

