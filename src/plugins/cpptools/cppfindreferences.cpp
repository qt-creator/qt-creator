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
    Process(QFutureInterface<Core::Utils::FileSearchResult> &future,
            Document::Ptr doc, const Snapshot &snapshot)
            : ASTVisitor(doc->control()),
              _future(future),
              _doc(doc),
              _snapshot(snapshot),
              _source(_doc->source()),
              _sem(doc->control())
    {
        _snapshot.insert(_doc);
    }

    void operator()(Symbol *symbol, Identifier *id, AST *ast)
    {
        _declSymbol = symbol;
        _id = id;
        _exprDoc = Document::create("<references>");
        accept(ast);
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

    void reportResult(unsigned tokenIndex)
    {
        const Token &tk = tokenAt(tokenIndex);
        const QString lineText = matchingLine(tk);

        unsigned line, col;
        getTokenStartPosition(tokenIndex, &line, &col);

        if (col)
            --col;  // adjust the column position.

        int len = tk.f.length;

        _future.reportResult(Core::Utils::FileSearchResult(QDir::toNativeSeparators(_doc->fileName()),
                                                           line, lineText, col, len));
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

    LookupContext currentContext(AST *ast) const
    {
        unsigned line, column;
        getTokenStartPosition(ast->firstToken(), &line, &column);
        Symbol *lastVisibleSymbol = _doc->findSymbolAt(line, column);
        return LookupContext(lastVisibleSymbol, _exprDoc, _doc, _snapshot);
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
                if (checkCandidates(candidates))
                    reportResult(simple->identifier_token);
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
        if (! ast->member_name)
            return false;

        SimpleNameAST *simple = ast->member_name->asSimpleName();
        if (! simple)
            return true; // ### TODO handle pseudo-destructors and qualified names.

        Q_ASSERT(! _postfixExpressionStack.isEmpty());

        if (identifier(simple->identifier_token) == _id) {
            unsigned startOfPostfixExpression = _postfixExpressionStack.last()->firstToken();

            unsigned begin = tokenAt(startOfPostfixExpression).begin();
            unsigned end = tokenAt(ast->member_name->lastToken() - 1).end();

            const QString expression = _source.mid(begin, end - begin);
            // qDebug() << "*** expression:" << expression;

            TypeOfExpression typeofExpression;
            typeofExpression.setSnapshot(_snapshot);

            unsigned line, column;
            getTokenStartPosition(startOfPostfixExpression, &line, &column);
            Symbol *lastVisibleSymbol = _doc->findSymbolAt(line, column);

            const QList<TypeOfExpression::Result> results =
                    typeofExpression(expression, _doc, lastVisibleSymbol,
                                     TypeOfExpression::NoPreprocess);

            QList<Symbol *> candidates;

            foreach (TypeOfExpression::Result r, results) {
                FullySpecifiedType ty = r.first;
                Symbol *lastVisibleSymbol = r.second;

                candidates.append(lastVisibleSymbol);
            }

            if (checkCandidates(candidates))
                reportResult(simple->identifier_token);
        }

        return false;
    }

    virtual bool visit(QualifiedNameAST *ast)
    {
        if (! ast->name) {
            //qWarning() << "invalid AST at" << _doc->fileName() << line << column;
            ast->name = _sem.check(ast, /*scope */ static_cast<Scope *>(0));
        }

        Q_ASSERT(ast->name != 0);
        Identifier *id = ast->name->identifier();
        if (id == _id && ast->unqualified_name) {
            LookupContext context = currentContext(ast);
            const QList<Symbol *> candidates = context.resolve(ast->name);
            if (checkCandidates(candidates))
                reportResult(ast->unqualified_name->firstToken());
        }

        return false;
    }

    virtual bool visit(SimpleNameAST *ast)
    {
        Identifier *id = identifier(ast->identifier_token);
        if (id == _id) {
            LookupContext context = currentContext(ast);
            const QList<Symbol *> candidates = context.resolve(ast->name);
            if (checkCandidates(candidates))
                reportResult(ast->identifier_token);
        }

        return false;
    }

    virtual bool visit(TemplateIdAST *ast)
    {
        Identifier *id = identifier(ast->identifier_token);
        if (id == _id) {
            LookupContext context = currentContext(ast);
            const QList<Symbol *> candidates = context.resolve(ast->name);
            if (checkCandidates(candidates))
                reportResult(ast->identifier_token);
        }

        return false;
    }

private:
    QFutureInterface<Core::Utils::FileSearchResult> &_future;
    Identifier *_id; // ### remove me
    Symbol *_declSymbol;
    Document::Ptr _doc;
    Snapshot _snapshot;
    QByteArray _source;
    Document::Ptr _exprDoc;
    Semantic _sem;
    QList<PostfixExpressionAST *> _postfixExpressionStack;
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

static void find_helper(QFutureInterface<Core::Utils::FileSearchResult> &future,
                        const QMap<QString, QString> wl,
                        Snapshot snapshot,
                        Symbol *symbol)
{
    QTime tm;
    tm.start();

    Identifier *symbolId = symbol->identifier();
    Q_ASSERT(symbolId != 0);

    const QString fileName = QString::fromUtf8(symbol->fileName(), symbol->fileNameLength());

    QStringList files(fileName);
    files += snapshot.dependsOn(fileName);
    qDebug() << "done in:" << tm.elapsed() << "number of files to parse:" << files.size();

    future.setProgressRange(0, files.size());

    for (int i = 0; i < files.size(); ++i) {
        const QString &fn = files.at(i);
        future.setProgressValueAndText(i, QFileInfo(fn).fileName());

        Document::Ptr previousDoc = snapshot.value(fn);
        if (previousDoc) {
            Control *control = previousDoc->control();
            Identifier *id = control->findIdentifier(symbolId->chars(), symbolId->size());
            if (! id)
                continue; // skip this document, it's not using symbolId.
        }

        QFile f(fn);
        if (! f.open(QFile::ReadOnly))
            continue;

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
            doc->check();
            TranslationUnit *unit = doc->translationUnit();
            Process process(future, doc, snapshot);
            process(symbol, id, unit->ast());
        }
    }

    future.setProgressValue(files.size());
}

void CppFindReferences::findAll(Symbol *symbol)
{
    _resultWindow->clearContents();
    _resultWindow->popup(true);

    const Snapshot snapshot = _modelManager->snapshot();
    const QMap<QString, QString> wl = _modelManager->buildWorkingCopyList();

    Core::ProgressManager *progressManager = Core::ICore::instance()->progressManager();

    QFuture<Core::Utils::FileSearchResult> result = QtConcurrent::run(&find_helper, wl, snapshot, symbol);
    m_watcher.setFuture(result);

    Core::FutureProgress *progress = progressManager->addTask(result, tr("Searching..."),
                                                              CppTools::Constants::TASK_SEARCH,
                                                              Core::ProgressManager::CloseOnSuccess);

    connect(progress, SIGNAL(clicked()), _resultWindow, SLOT(popup()));
}

void CppFindReferences::displayResult(int index)
{
    Core::Utils::FileSearchResult result = m_watcher.future().resultAt(index);
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

