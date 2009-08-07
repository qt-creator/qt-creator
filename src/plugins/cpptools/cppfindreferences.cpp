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

#include <cplusplus/CppDocument.h>
#include <cplusplus/ExpressionUnderCursor.h>
#include <cplusplus/ResolveExpression.h>

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
              _source(_doc->source())
    { }

    void operator()(Identifier *id, AST *ast)
    {
        _id = id;
        _currentSymbol = _doc->globalNamespace();
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

    LookupContext currentContext() const
    {
        return LookupContext(_currentSymbol, _exprDoc, _doc, _snapshot);
    }

    virtual bool visit(ClassSpecifierAST *ast)
    {
        _currentSymbol = ast->symbol;
        return true;
    }

    virtual bool visit(NamespaceAST *ast)
    {
        _currentSymbol = ast->symbol;
        return true;
    }

    virtual bool visit(CompoundStatementAST *ast)
    {
        _currentSymbol = ast->symbol;
        return true;
    }

    virtual bool visit(FunctionDefinitionAST *ast)
    {
        _currentSymbol = ast->symbol;
        return true;
    }

    virtual bool visit(QualifiedNameAST *ast)
    {
        return true;
    }

    virtual bool visit(SimpleNameAST *ast)
    {
        Identifier *id = identifier(ast->identifier_token);
        if (id == _id) {
#if 0
            LookupContext context = currentContext();
            ResolveExpression resolveExpression(context);
            QList<ResolveExpression::Result> results = resolveExpression(ast);
            if (! results.isEmpty()) {
                ResolveExpression::Result result = results.first();
                Symbol *resolvedSymbol = result.second;
                qDebug() << "resolves to:" << resolvedSymbol->fileName() << resolvedSymbol->line();
            }
#endif
            reportResult(ast->identifier_token);
        }

        return false;
    }

    virtual bool visit(TemplateIdAST *ast)
    {
        Identifier *id = identifier(ast->identifier_token);
        if (id == _id)
            reportResult(ast->identifier_token);

        return true;
    }

private:
    QFutureInterface<Core::Utils::FileSearchResult> &_future;
    Identifier *_id;
    Document::Ptr _doc;
    Snapshot _snapshot;
    QByteArray _source;
    Symbol *_currentSymbol;
    Document::Ptr _exprDoc;
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
                        QString word,
                        QString fileName,
                        Snapshot snapshot)
{
    QTime tm;
    tm.start();
    QStringList files(fileName);
    files += snapshot.dependsOn(fileName);
    qDebug() << "done in:" << tm.elapsed() << "number of files to parse:" << files.size();

    future.setProgressRange(0, files.size());

    tm.start();
    for (int i = 0; i < files.size(); ++i) {
        const QString &fn = files.at(i);
        future.setProgressValueAndText(i, QFileInfo(fn).fileName());
        QFile f(fn);
        if (! f.open(QFile::ReadOnly))
            continue;

        const QString source = QTextStream(&f).readAll();
        const QByteArray preprocessedCode = snapshot.preprocessedCode(source, fn);
        Document::Ptr doc = snapshot.documentFromSource(preprocessedCode, fn);
        doc->check();

        Control *control = doc->control();
        Identifier *id = control->findOrInsertIdentifier(word.toLatin1().constData());

        TranslationUnit *unit = doc->translationUnit();
        Process process(future, doc, snapshot);
        process(id, unit->ast());
    }
    future.setProgressValue(files.size());
}

void CppFindReferences::findAll(const QString &fileName, const QString &text)
{
    _resultWindow->clearContents();
    _resultWindow->popup(true);

    Core::ProgressManager *progressManager = Core::ICore::instance()->progressManager();

    const Snapshot snapshot = _modelManager->snapshot();

    QFuture<Core::Utils::FileSearchResult> result =
            QtConcurrent::run(&find_helper, text, fileName, snapshot);

    m_watcher.setFuture(result);

    Core::FutureProgress *progress = progressManager->addTask(result, tr("Searching..."),
                                                              CppTools::Constants::TASK_INDEX,
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

