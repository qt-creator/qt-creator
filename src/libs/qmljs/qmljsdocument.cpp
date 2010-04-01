/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "qmljsdocument.h"
#include "qmljsbind.h"
#include <qmljs/parser/qmljsast_p.h>
#include <qmljs/parser/qmljslexer_p.h>
#include <qmljs/parser/qmljsparser_p.h>
#include <qmljs/parser/qmljsnodepool_p.h>
#include <qmljs/parser/qmljsastfwd_p.h>
#include <QtCore/QDir>

using namespace QmlJS;
using namespace QmlJS::AST;

Document::Document(const QString &fileName)
    : _engine(0)
    , _pool(0)
    , _ast(0)
    , _bind(0)
    , _isQmlDocument(false)
    , _documentRevision(0)
    , _parsedCorrectly(false)
    , _fileName(fileName)
{
    QFileInfo fileInfo(fileName);
    _path = fileInfo.absolutePath();

    // ### Should use mime type
    if (fileInfo.suffix() == QLatin1String("qml")
            || fileInfo.suffix() == QLatin1String("qmlproject")) {
        _isQmlDocument = true;
        _componentName = fileInfo.baseName();

        if (! _componentName.isEmpty()) {
            // ### TODO: check the component name.

            if (! _componentName.at(0).isUpper())
                _componentName.clear();
        }
    }
}

Document::~Document()
{
    if (_bind)
        delete _bind;

    if (_engine)
        delete _engine;

    if (_pool)
        delete _pool;
}

Document::Ptr Document::create(const QString &fileName)
{
    Document::Ptr doc(new Document(fileName));
    return doc;
}

bool Document::isQmlDocument() const
{
    return _isQmlDocument;
}

bool Document::isJSDocument() const
{
    return ! _isQmlDocument;
}

AST::UiProgram *Document::qmlProgram() const
{
    return cast<UiProgram *>(_ast);
}

AST::Program *Document::jsProgram() const
{
    return cast<Program *>(_ast);
}

AST::ExpressionNode *Document::expression() const
{
    if (_ast)
        return _ast->expressionCast();

    return 0;
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

int Document::documentRevision() const
{
    return _documentRevision;
}

void Document::setDocumentRevision(int revision)
{
    _documentRevision = revision;
}

QString Document::fileName() const
{
    return _fileName;

}

QString Document::path() const
{
    return _path;
}

QString Document::componentName() const
{
    return _componentName;
}

bool Document::parse_helper(int startToken)
{
    Q_ASSERT(! _engine);
    Q_ASSERT(! _pool);
    Q_ASSERT(! _ast);
    Q_ASSERT(! _bind);

    _engine = new Engine();
    _pool = new NodePool(_fileName, _engine);

    Lexer lexer(_engine);
    Parser parser(_engine);

    lexer.setCode(_source, /*line = */ 1);

    switch (startToken) {
    case QmlJSGrammar::T_FEED_UI_PROGRAM:
        _parsedCorrectly = parser.parse();
        break;
    case QmlJSGrammar::T_FEED_JS_PROGRAM:
        _parsedCorrectly = parser.parseProgram();
        break;
    case QmlJSGrammar::T_FEED_JS_EXPRESSION:
        _parsedCorrectly = parser.parseExpression();
        break;
    default:
        Q_ASSERT(0);
    }

    _ast = parser.rootNode();
    _diagnosticMessages = parser.diagnosticMessages();

    _bind = new Bind(this);

    return _parsedCorrectly;
}

bool Document::parse()
{
    if (isQmlDocument())
        return parseQml();

    return parseJavaScript();
}

bool Document::parseQml()
{
    return parse_helper(QmlJSGrammar::T_FEED_UI_PROGRAM);
}

bool Document::parseJavaScript()
{
    return parse_helper(QmlJSGrammar::T_FEED_JS_PROGRAM);
}

bool Document::parseExpression()
{
    return parse_helper(QmlJSGrammar::T_FEED_JS_EXPRESSION);
}

Bind *Document::bind() const
{
    return _bind;
}

LibraryInfo::LibraryInfo()
    : _valid(false)
{
}

LibraryInfo::LibraryInfo(const QmlDirParser &parser)
    : _valid(true)
    , _components(parser.components())
    , _plugins(parser.plugins())
{
}

LibraryInfo::~LibraryInfo()
{
}

Snapshot::Snapshot()
{
}

Snapshot::~Snapshot()
{
}

void Snapshot::insert(const Document::Ptr &document)
{
    if (document && (document->qmlProgram() || document->jsProgram())) {
        _documents.insert(document->fileName(), document);
        _documentsByPath.insert(document->path(), document);
    }
}

void Snapshot::insertLibraryInfo(const QString &path, const LibraryInfo &info)
{
    _libraries.insert(path, info);
}

Document::Ptr Snapshot::documentFromSource(const QString &code,
                                           const QString &fileName) const
{
    Document::Ptr newDoc = Document::create(fileName);

    if (Document::Ptr thisDocument = document(fileName)) {
        newDoc->_documentRevision = thisDocument->_documentRevision;
    }

    newDoc->setSource(code);
    return newDoc;
}

QList<Document::Ptr> Snapshot::importedDocuments(const Document::Ptr &doc, const QString &importPath) const
{
    // ### TODO: maybe we should add all imported documents in the parse Document::parse() method, regardless of whether they're in the path or not.

    QList<Document::Ptr> result;

    QString docPath = doc->path();
    docPath += QLatin1Char('/');
    docPath += importPath;
    docPath = QDir::cleanPath(docPath);

    foreach (Document::Ptr candidate, _documents) {
        if (candidate == doc)
            continue; // ignore this document
        else if (candidate->isJSDocument())
            continue; // skip JS documents

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
