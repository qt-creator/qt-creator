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

#include "qmlidcollector.h"
#include "qmldocument.h"
#include "qmljsast_p.h"
#include "qmljslexer_p.h"
#include "qmljsparser_p.h"
#include "qmljsnodepool_p.h"
#include "qmljsastfwd_p.h"

using namespace Qml;
using namespace QmlEditor;
using namespace QmlJS;

QmlDocument::QmlDocument(const QString &fileName)
    : _engine(0)
    , _pool(0)
    , _program(0)
    , _fileName(fileName)
    , _parsedCorrectly(false)
{
    const int slashIdx = fileName.lastIndexOf('/');
    if (slashIdx != -1)
        _path = fileName.left(slashIdx);

    if (fileName.toLower().endsWith(".qml"))
        _componentName = fileName.mid(slashIdx + 1, fileName.size() - (slashIdx + 1) - 4);
}

QmlDocument::~QmlDocument()
{
    if (_engine)
        delete _engine;

    if (_pool)
        delete _pool;

    qDeleteAll(_symbols);
}

QmlDocument::Ptr QmlDocument::create(const QString &fileName)
{
    QmlDocument::Ptr doc(new QmlDocument(fileName));
    return doc;
}

AST::UiProgram *QmlDocument::program() const
{
    return _program;
}

QList<DiagnosticMessage> QmlDocument::diagnosticMessages() const
{
    return _diagnosticMessages;
}

QString QmlDocument::source() const
{
    return _source;
}

void QmlDocument::setSource(const QString &source)
{
    _source = source;
}

bool QmlDocument::parse()
{
    Q_ASSERT(! _engine);
    Q_ASSERT(! _pool);
    Q_ASSERT(! _program);

    _engine = new Engine();
    _pool = new NodePool(_fileName, _engine);
    _ids.clear();

    Lexer lexer(_engine);
    Parser parser(_engine);

    lexer.setCode(_source, /*line = */ 1);

    _parsedCorrectly = parser.parse();
    _program = parser.ast();
    _diagnosticMessages = parser.diagnosticMessages();

    if (_program) {
        for (QmlJS::AST::UiObjectMemberList *iter = _program->members; iter; iter = iter->next)
            if (iter->member)
                _symbols.append(new QmlSymbolFromFile(_fileName, iter->member));

         Internal::QmlIdCollector collect;
        _ids = collect(*this);
    }

    return _parsedCorrectly;
}

QmlSymbolFromFile *QmlDocument::findSymbol(QmlJS::AST::Node *node) const
{
    foreach (QmlSymbol *symbol, _symbols)
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

void Snapshot::insert(const QmlDocument::Ptr &document)
{
    if (!document || !document->program())
        return;

    QMap<QString, QmlDocument::Ptr>::insert(document->fileName(), document);
}

QmlDocument::PtrList Snapshot::importedDocuments(const QmlDocument::Ptr &doc, const QString &importPath) const
{
    QmlDocument::PtrList result;

    const QString docPath = doc->path() + '/' + importPath;

    foreach (QmlDocument::Ptr candidate, *this) {
        if (candidate == doc)
            continue;

        if (candidate->path() == doc->path() || candidate->path() == docPath)
            result.append(candidate);
    }

    return result;
}

QMap<QString, QmlDocument::Ptr> Snapshot::componentsDefinedByImportedDocuments(const QmlDocument::Ptr &doc, const QString &importPath) const
{
    QMap<QString, QmlDocument::Ptr> result;

    const QString docPath = doc->path() + '/' + importPath;

    foreach (QmlDocument::Ptr candidate, *this) {
        if (candidate == doc)
            continue;

        if (candidate->path() == doc->path() || candidate->path() == docPath)
            result.insert(candidate->componentName(), candidate);
    }

    return result;
}
