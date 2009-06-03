/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "cppsemanticsearch.h"
#include "cppmodelmanager.h"

#include <AST.h>
#include <TranslationUnit.h>

#include <cplusplus/PreprocessorClient.h>
#include <cplusplus/pp.h>

#include <QtCore/QDir>
#include <QtCore/QPointer>
#include <QtCore/QtConcurrentRun>
#include <QtCore/QFutureSynchronizer>
#include <qtconcurrent/runextensions.h>

using namespace CppTools::Internal;
using namespace CPlusPlus;

namespace {

class SimpleClient: public Client
{
    Environment _env;
    QPointer<CppModelManager> _modelManager;
    Snapshot _snapshot;
    Preprocessor _preproc;
    QSet<QString> _merged;

public:
    SimpleClient(QPointer<CppModelManager> modelManager)
        : _modelManager(modelManager),
          _snapshot(_modelManager->snapshot()),
          _preproc(this, &_env)
    { }

    QByteArray run(QString fileName, const QByteArray &source)
    {
        const QByteArray preprocessed = _preproc(fileName, source);
        return preprocessed;
    }

    virtual void sourceNeeded(QString &fileName, IncludeType, unsigned)
    { mergeEnvironment(fileName); }

    virtual void macroAdded(const Macro &) {}

    virtual void startExpandingMacro(unsigned,
                                     const Macro &,
                                     const QByteArray &,
                                     const QVector<MacroArgumentReference> &) {}

    virtual void stopExpandingMacro(unsigned, const Macro &) {}

    virtual void startSkippingBlocks(unsigned) {}
    virtual void stopSkippingBlocks(unsigned) {}

    void mergeEnvironment(const QString &fileName)
    {
        if (! _merged.contains(fileName)) {
            _merged.insert(fileName);

            if (Document::Ptr doc = _snapshot.value(fileName)) {
                foreach (const Document::Include &i, doc->includes())
                    mergeEnvironment(i.fileName());

                _env.addMacros(doc->definedMacros());
            }
        }
    }
};

class FindClass: public SemanticSearch
{
    QString _text;
    QTextDocument::FindFlags _findFlags;

public:
    FindClass(QFutureInterface<Core::Utils::FileSearchResult> &future, Document::Ptr doc, Snapshot snapshot)
        : SemanticSearch(future, doc, snapshot)
    { }

    void setText(const QString &text)
    { _text = text; }

    void setFindFlags(QTextDocument::FindFlags findFlags)
    { _findFlags = findFlags; }

    virtual void run(AST *ast)
    { accept(ast); }

protected:
    using ASTVisitor::visit;

    virtual bool visit(ClassSpecifierAST *ast)
    {
        if (ast->name) {
            Qt::CaseSensitivity cs = Qt::CaseInsensitive;

            if (_findFlags & QTextDocument::FindCaseSensitively)
                cs = Qt::CaseSensitive;

            Token start = tokenAt(ast->name->firstToken());
            Token end = tokenAt(ast->name->lastToken() - 1);
            const QString className = QString::fromUtf8(source().constData() + start.begin(),
                                                        end.end() - start.begin());

            if (className.contains(_text, cs))
                reportResult(ast->name->firstToken());
        }

        return true;
    }
};

} // end of anonymous namespace

SemanticSearch::SemanticSearch(QFutureInterface<Core::Utils::FileSearchResult> &future,
                               Document::Ptr doc,
                               Snapshot snapshot)
    : ASTVisitor(doc->control()),
      _future(future),
      _doc(doc),
      _snapshot(snapshot)
{
    _thisDocument = _snapshot.value(_doc->fileName());
}

SemanticSearch::~SemanticSearch()
{ }

const QByteArray &SemanticSearch::source() const
{ return _source; }

void SemanticSearch::setSource(const QByteArray &source)
{ _source = source; }

QString SemanticSearch::matchingLine(const Token &tk) const
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

void SemanticSearch::reportResult(unsigned tokenIndex)
{
    const Token &tk = tokenAt(tokenIndex);
    const QString lineText = matchingLine(tk);

    unsigned line, col;
    getTokenStartPosition(tokenIndex, &line, &col);

    if (col)
        --col;  // adjust the column position.

    _future.reportResult(Core::Utils::FileSearchResult(QDir::toNativeSeparators(_doc->fileName()),
                                                       line, lineText, col, tk.length));
}

SemanticSearch *SearchClassDeclarationsFactory::create(QFutureInterface<Core::Utils::FileSearchResult> &future,
                                                       Document::Ptr doc,
                                                       Snapshot snapshot)
{
    FindClass *findClass = new FindClass(future, doc, snapshot);
    findClass->setText(_text);
    findClass->setFindFlags(_findFlags);
    return findClass;
}

static void semanticSearch_helper(QFutureInterface<Core::Utils::FileSearchResult> &future,
                                  QPointer<CppModelManager> modelManager,
                                  SemanticSearchFactory::Ptr factory)
{
    const Snapshot snapshot = modelManager->snapshot();

    future.setProgressRange(0, snapshot.size());
    future.setProgressValue(0);

    int progress = 0;
    foreach (Document::Ptr doc, snapshot) {
        const QString fileName = doc->fileName();

        QFile file(fileName);
        if (! file.open(QFile::ReadOnly))
            continue;

        const QString contents = QTextStream(&file).readAll(); // ### FIXME

        SimpleClient r(modelManager);
        const QByteArray source = r.run(fileName, contents.toUtf8());

        Document::Ptr newDoc = Document::create(fileName);
        newDoc->setSource(source);
        newDoc->parse();

        if (SemanticSearch *search = factory->create(future, newDoc, snapshot)) {
            search->setSource(source);
            search->run(newDoc->translationUnit()->ast());
            delete search;
        }

        future.setProgressValue(++progress);
    }
}

QFuture<Core::Utils::FileSearchResult> CppTools::Internal::semanticSearch(QPointer<CppModelManager> modelManager,
                                                                          SemanticSearchFactory::Ptr factory)
{
    return QtConcurrent::run(&semanticSearch_helper, modelManager, factory);
}
