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

#include "qmljsidcollector.h"
#include "qmljsdocument.h"
#include <qmljs/parser/qmljsast_p.h>
#include <qmljs/parser/qmljslexer_p.h>
#include <qmljs/parser/qmljsparser_p.h>
#include <qmljs/parser/qmljsnodepool_p.h>
#include <qmljs/parser/qmljsastfwd_p.h>

using namespace QmlJS;
using namespace QmlJS;
using namespace QmlJS::AST;

Document::Document(const QString &fileName)
    : _engine(0)
    , _pool(0)
    , _ast(0)
    , _fileName(fileName)
    , _parsedCorrectly(false)
{
    const int slashIdx = fileName.lastIndexOf('/');
    if (slashIdx != -1)
        _path = fileName.left(slashIdx);

    if (fileName.toLower().endsWith(".qml"))
        _componentName = fileName.mid(slashIdx + 1, fileName.size() - (slashIdx + 1) - 4);
}

Document::~Document()
{
    if (_engine)
        delete _engine;

    if (_pool)
        delete _pool;

    qDeleteAll(_symbols);
}

Document::Ptr Document::create(const QString &fileName)
{
    Document::Ptr doc(new Document(fileName));
    return doc;
}

AST::UiProgram *Document::qmlProgram() const
{
    return cast<UiProgram *>(_ast);
}

AST::Program *Document::jsProgram() const
{
    return cast<Program *>(_ast);
}

AST::Node *Document::ast() const
{
    return _ast;
}

QList<DiagnosticMessage> Document::diagnosticMessages() const
{
    return _diagnosticMessages;
}

QString Document::source() const
{
    return _source;
}

void Document::setSource(const QString &source)
{
    _source = source;
}

bool Document::parseQml()
{
    Q_ASSERT(! _engine);
    Q_ASSERT(! _pool);
    Q_ASSERT(! _ast);

    _engine = new Engine();
    _pool = new NodePool(_fileName, _engine);
    _ids.clear();

    Lexer lexer(_engine);
    Parser parser(_engine);

    lexer.setCode(_source, /*line = */ 1);

    _parsedCorrectly = parser.parse();
    _ast = parser.ast();
    _diagnosticMessages = parser.diagnosticMessages();

    if (qmlProgram()) {
        for (QmlJS::AST::UiObjectMemberList *iter = qmlProgram()->members; iter; iter = iter->next)
            if (iter->member)
                _symbols.append(new SymbolFromFile(_fileName, iter->member));

         Internal::IdCollector collect;
        _ids = collect(*this);
        if (_diagnosticMessages.isEmpty())
            _diagnosticMessages = collect.diagnosticMessages();
    }

    return _parsedCorrectly;
}

bool Document::parseJavaScript()
{
    Q_ASSERT(! _engine);
    Q_ASSERT(! _pool);
    Q_ASSERT(! _ast);

    _engine = new Engine();
    _pool = new NodePool(_fileName, _engine);
    _ids.clear();

    Lexer lexer(_engine);
    Parser parser(_engine);

    lexer.setCode(_source, /*line = */ 1);

    _parsedCorrectly = parser.parseProgram();
    _ast = cast<Program*>(parser.rootNode());
    _diagnosticMessages = parser.diagnosticMessages();

    return _parsedCorrectly;
}

bool Document::parseExpression()
{
    Q_ASSERT(! _engine);
    Q_ASSERT(! _pool);
    Q_ASSERT(! _ast);

    _engine = new Engine();
    _pool = new NodePool(_fileName, _engine);
    _ids.clear();

    Lexer lexer(_engine);
    Parser parser(_engine);

    lexer.setCode(_source, /*line = */ 1);

    _parsedCorrectly = parser.parseExpression();
    _ast = parser.rootNode();
    if (_ast)
        _ast = _ast->expressionCast();
    _diagnosticMessages = parser.diagnosticMessages();

    return _parsedCorrectly;
}

SymbolFromFile *Document::findSymbol(QmlJS::AST::Node *node) const
{
    foreach (Symbol *symbol, _symbols)
        if (symbol->isSymbolFromFile())
            if (symbol->asSymbolFromFile()->node() == node)
                return symbol->asSymbolFromFile();

    return 0;
}

Snapshot::Snapshot()
{
}

Snapshot::~Snapshot()
{
}

void Snapshot::insert(const Document::Ptr &document)
{
    if (document && (document->qmlProgram() || document->jsProgram()))
        QMap<QString, Document::Ptr>::insert(document->fileName(), document);
}

Document::PtrList Snapshot::importedDocuments(const Document::Ptr &doc, const QString &importPath) const
{
    Document::PtrList result;

    const QString docPath = doc->path() + '/' + importPath;

    foreach (Document::Ptr candidate, *this) {
        if (candidate == doc)
            continue;

        if (candidate->path() == doc->path() || candidate->path() == docPath)
            result.append(candidate);
    }

    return result;
}

QMap<QString, Document::Ptr> Snapshot::componentsDefinedByImportedDocuments(const Document::Ptr &doc, const QString &importPath) const
{
    QMap<QString, Document::Ptr> result;

    const QString docPath = doc->path() + '/' + importPath;

    foreach (Document::Ptr candidate, *this) {
        if (candidate == doc)
            continue;

        if (candidate->path() == doc->path() || candidate->path() == docPath)
            result.insert(candidate->componentName(), candidate);
    }

    return result;
}
