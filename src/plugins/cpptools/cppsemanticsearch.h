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

#ifndef CPPSEMANTICSEARCH_H
#define CPPSEMANTICSEARCH_H

#include <ASTVisitor.h>
#include <cplusplus/CppDocument.h>

#include <utils/filesearch.h>

#include <QtCore/QFutureInterface>
#include <QtCore/QPointer>
#include <QtCore/QSharedPointer>
#include <QtGui/QTextDocument>

namespace CppTools {
namespace Internal {

class CppModelManager;
class SemanticSearchFactory;

class SemanticSearch: protected CPlusPlus::ASTVisitor
{
    QFutureInterface<Utils::FileSearchResult> &_future;
    CPlusPlus::Document::Ptr _doc;
    CPlusPlus::Snapshot _snapshot;
    CPlusPlus::Document::Ptr _thisDocument;
    QByteArray _source;

public:
    SemanticSearch(QFutureInterface<Utils::FileSearchResult> &future,
                   CPlusPlus::Document::Ptr doc,
                   CPlusPlus::Snapshot snapshot);

    virtual ~SemanticSearch();

    virtual void run(CPlusPlus::AST *ast) = 0;

    const QByteArray &source() const;
    void setSource(const QByteArray &source);

protected:
    QString matchingLine(const CPlusPlus::Token &tk) const;
    void reportResult(unsigned tokenIndex, int offset, int len);
};

class SemanticSearchFactory
{
    Q_DISABLE_COPY(SemanticSearchFactory)

public:
    typedef QSharedPointer<SemanticSearchFactory> Ptr;

    SemanticSearchFactory() {}
    virtual ~SemanticSearchFactory() {}

    virtual SemanticSearch *create(QFutureInterface<Utils::FileSearchResult> &future,
                                   CPlusPlus::Document::Ptr doc,
                                   CPlusPlus::Snapshot snapshot) = 0;
};

class SearchClassDeclarationsFactory: public SemanticSearchFactory
{
    QString _text;
    QTextDocument::FindFlags _findFlags;

public:
    SearchClassDeclarationsFactory(const QString &text, QTextDocument::FindFlags findFlags)
        : _text(text), _findFlags(findFlags)
    { }

    virtual SemanticSearch *create(QFutureInterface<Utils::FileSearchResult> &future,
                                   CPlusPlus::Document::Ptr doc,
                                   CPlusPlus::Snapshot snapshot);
};

class SearchFunctionCallFactory: public SemanticSearchFactory
{
    QString _text;
    QTextDocument::FindFlags _findFlags;

public:
    SearchFunctionCallFactory(const QString &text, QTextDocument::FindFlags findFlags)
        : _text(text), _findFlags(findFlags)
    { }

    virtual SemanticSearch *create(QFutureInterface<Utils::FileSearchResult> &future,
                                   CPlusPlus::Document::Ptr doc,
                                   CPlusPlus::Snapshot snapshot);
};

QFuture<Utils::FileSearchResult> semanticSearch(QPointer<CppModelManager> modelManager,
                                                      SemanticSearchFactory::Ptr factory);


} // end of namespace Internal
} // end of namespace CppTools

#endif // CPPSEMANTICSEARCH_H
