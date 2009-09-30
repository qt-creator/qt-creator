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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "cppsemanticsearch.h"
#include "cppmodelmanager.h"

#include <AST.h>
#include <Literals.h>
#include <TranslationUnit.h>

#include <QtCore/QDir>
#include <QtCore/QPointer>
#include <QtCore/QtConcurrentRun>
#include <QtCore/QFutureSynchronizer>
#include <qtconcurrent/runextensions.h>

using namespace CppTools::Internal;
using namespace CPlusPlus;

namespace {

class SearchClass: public SemanticSearch
{
    QString _text;
    QTextDocument::FindFlags _findFlags;

public:
    SearchClass(QFutureInterface<Core::Utils::FileSearchResult> &future,
                Document::Ptr doc, Snapshot snapshot)
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

    bool match(NameAST *name)
    {
        if (! name)
            return false;

        else if (SimpleNameAST *simpleName = name->asSimpleName()) {
            if (Identifier *id = identifier(simpleName->identifier_token)) {
                Qt::CaseSensitivity cs = Qt::CaseInsensitive;

                if (_findFlags & QTextDocument::FindCaseSensitively)
                    cs = Qt::CaseSensitive;

                QString s = QString::fromUtf8(id->chars(), id->size());
                int index = s.indexOf(_text, 0, cs);
                if (index != -1) {
                    reportResult(simpleName->identifier_token, index, _text.length());
                    return true;
                }
            }
        }

        else if (QualifiedNameAST *q = name->asQualifiedName()) {
            return match(q->unqualified_name);
        }

        return false;
    }

    virtual bool visit(ElaboratedTypeSpecifierAST *ast)
    {
        if (tokenKind(ast->classkey_token) != T_ENUM) {
            match(ast->name);
        }

        return true;
    }

    virtual bool visit(ClassSpecifierAST *ast)
    {
        match(ast->name);
        return true;
    }
};

class SearchFunctionCall: public SemanticSearch
{
    QString _text;
    QTextDocument::FindFlags _findFlags;

public:
    SearchFunctionCall(QFutureInterface<Core::Utils::FileSearchResult> &future,
                       Document::Ptr doc, Snapshot snapshot)
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

    bool match(NameAST *name)
    {
        if (! name)
            return false;

        else if (SimpleNameAST *simpleName = name->asSimpleName()) {
            if (Identifier *id = identifier(simpleName->identifier_token)) {
                Qt::CaseSensitivity cs = Qt::CaseInsensitive;

                if (_findFlags & QTextDocument::FindCaseSensitively)
                    cs = Qt::CaseSensitive;

                QString s = QString::fromUtf8(id->chars(), id->size());
                int index = s.indexOf(_text, 0, cs);
                if (index != -1) {
                    reportResult(simpleName->identifier_token, index, _text.length());
                    return true;
                }
            }
        }

        else if (QualifiedNameAST *q = name->asQualifiedName()) {
            return match(q->unqualified_name);
        }

        return false;
    }

    virtual bool visit(PostfixExpressionAST *ast)
    {
        NameAST *name = 0;

        if (ast->base_expression)
            name = ast->base_expression->asName();

        for (PostfixAST *fx = ast->postfix_expressions; fx; fx = fx->next) {
            if (fx->asCall() != 0) {
                match(name);
            } else if (MemberAccessAST *mem = fx->asMemberAccess()) {
                name = mem->member_name;
            }
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

void SemanticSearch::reportResult(unsigned tokenIndex, int offset, int len)
{
    const Token &tk = tokenAt(tokenIndex);
    const QString lineText = matchingLine(tk);

    unsigned line, col;
    getTokenStartPosition(tokenIndex, &line, &col);

    if (col)
        --col;  // adjust the column position.

    _future.reportResult(Core::Utils::FileSearchResult(QDir::toNativeSeparators(_doc->fileName()),
                                                       line, lineText, col + offset, len));
}

SemanticSearch *SearchClassDeclarationsFactory::create(QFutureInterface<Core::Utils::FileSearchResult> &future,
                                                       Document::Ptr doc,
                                                       Snapshot snapshot)
{
    SearchClass *search = new SearchClass(future, doc, snapshot);
    search->setText(_text);
    search->setFindFlags(_findFlags);
    return search;
}

SemanticSearch *SearchFunctionCallFactory::create(QFutureInterface<Core::Utils::FileSearchResult> &future,
                                                  Document::Ptr doc,
                                                  Snapshot snapshot)
{
    SearchFunctionCall *search = new SearchFunctionCall(future, doc, snapshot);
    search->setText(_text);
    search->setFindFlags(_findFlags);
    return search;
}

static void semanticSearch_helper(QFutureInterface<Core::Utils::FileSearchResult> &future,
                                  QPointer<CppModelManager> modelManager,
                                  QMap<QString, QString> wl,
                                  SemanticSearchFactory::Ptr factory)
{
    const Snapshot snapshot = modelManager->snapshot();

    future.setProgressRange(0, snapshot.size());
    future.setProgressValue(0);

    int progress = 0;
    foreach (Document::Ptr doc, snapshot) {
        const QString fileName = doc->fileName();

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

        Document::Ptr newDoc = snapshot.documentFromSource(source, fileName);
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
    return QtConcurrent::run(&semanticSearch_helper, modelManager,
                             modelManager->buildWorkingCopyList(), factory);
}
