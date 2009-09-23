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

#include "idcollector.h"
#include "duidocument.h"
#include "qmljsast_p.h"
#include "qmljslexer_p.h"
#include "qmljsparser_p.h"
#include "qmljsengine_p.h"
#include "qmljsnodepool_p.h"

using namespace DuiEditor;
using namespace QmlJS;

DuiDocument::DuiDocument(const QString &fileName)
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

DuiDocument::~DuiDocument()
{
    if (_engine)
        delete _engine;

    if (_pool)
        delete _pool;

    qDeleteAll(_ids.values());
}

DuiDocument::Ptr DuiDocument::create(const QString &fileName)
{
    DuiDocument::Ptr doc(new DuiDocument(fileName));
    return doc;
}

AST::UiProgram *DuiDocument::program() const
{
    return _program;
}

QList<DiagnosticMessage> DuiDocument::diagnosticMessages() const
{
    return _diagnosticMessages;
}

QString DuiDocument::source() const
{
    return _source;
}

void DuiDocument::setSource(const QString &source)
{
    _source = source;
}

bool DuiDocument::parse()
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

    if (_parsedCorrectly && _program) {
        Internal::IdCollector collect;
        _ids = collect(_fileName, _program);
    }

    return _parsedCorrectly;
}

Snapshot::Snapshot()
{
}

Snapshot::~Snapshot()
{
}

void Snapshot::insert(const DuiDocument::Ptr &document)
{
    QMap<QString, DuiDocument::Ptr>::insert(document->fileName(), document);
}

DuiDocument::PtrList Snapshot::importedDocuments(const DuiDocument::Ptr &doc, const QString &importPath) const
{
    DuiDocument::PtrList result;

    const QString docPath = doc->path() + '/' + importPath;

    foreach (DuiDocument::Ptr candidate, *this) {
        if (candidate == doc)
            continue;

        if (candidate->path() == doc->path() || candidate->path() == docPath)
            result.append(candidate);
    }

    return result;
}

QMap<QString, DuiDocument::Ptr> Snapshot::componentsDefinedByImportedDocuments(const DuiDocument::Ptr &doc, const QString &importPath) const
{
    QMap<QString, DuiDocument::Ptr> result;

    const QString docPath = doc->path() + '/' + importPath;

    foreach (DuiDocument::Ptr candidate, *this) {
        if (candidate == doc)
            continue;

        if (candidate->path() == doc->path() || candidate->path() == docPath)
            result.insert(candidate->componentName(), candidate);
    }

    return result;
}
