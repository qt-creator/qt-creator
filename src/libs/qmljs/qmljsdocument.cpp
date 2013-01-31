/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qmljsdocument.h"
#include "qmljsbind.h"
#include <qmljs/parser/qmljsast_p.h>
#include <qmljs/parser/qmljslexer_p.h>
#include <qmljs/parser/qmljsparser_p.h>
#include <qmljs/parser/qmljsastfwd_p.h>
#include <QDir>

using namespace QmlJS;
using namespace QmlJS::AST;

/*!
    \class QmlJS::Document
    \brief A Qml or JavaScript document.
    \sa Snapshot

    Documents are usually created by the ModelManagerInterface
    and stored in a Snapshot. They allow access to data such as
    the file path, source code, abstract syntax tree and the Bind
    instance for the document.

    To make sure unused and outdated documents are removed correctly, Document
    instances are usually accessed through a shared pointer, see Document::Ptr.

    Documents in a Snapshot are immutable: They, or anything reachable through them,
    must not be changed. This allows Documents to be shared freely among threads
    without extra synchronization.
*/

/*!
    \class QmlJS::LibraryInfo
    \brief A Qml library.
    \sa Snapshot

    A LibraryInfo is created when the ModelManagerInterface finds
    a Qml library and parses the qmldir file. The instance holds information about
    which Components the library provides and which plugins to load.

    The ModelManager will try to extract detailed information about the types
    defined in the plugins this library loads. Once it is done, the data will
    be available through the metaObjects() function.
*/

/*!
    \class QmlJS::Snapshot
    \brief A set of Document::Ptr and LibraryInfo instances.
    \sa Document LibraryInfo

    A Snapshot holds and offers access to a set of Document and LibraryInfo instances.

    Usually Snapshots are copies of the snapshot maintained and updated by the
    ModelManagerInterface that updates its instance as parsing
    threads finish and new information becomes available.
*/


bool Document::isQmlLikeLanguage(Document::Language language)
{
    switch (language) {
    case QmlLanguage:
    case QmlQtQuick1Language:
    case QmlQtQuick2Language:
    case QmlQbsLanguage:
    case QmlProjectLanguage:
    case QmlTypeInfoLanguage:
        return true;
    default:
        return false;
    }
}

Document::Document(const QString &fileName, Language language)
    : _engine(0)
    , _ast(0)
    , _bind(0)
    , _fileName(QDir::cleanPath(fileName))
    , _editorRevision(0)
    , _language(language)
    , _parsedCorrectly(false)
{
    QFileInfo fileInfo(fileName);
    _path = QDir::cleanPath(fileInfo.absolutePath());

    if (isQmlLikeLanguage(language)) {
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
}

Document::MutablePtr Document::create(const QString &fileName, Language language)
{
    Document::MutablePtr doc(new Document(fileName, language));
    doc->_ptr = doc;
    return doc;
}

Document::Language Document::guessLanguageFromSuffix(const QString &fileName)
{
    if (fileName.endsWith(QLatin1String(".qml"), Qt::CaseInsensitive))
        return QmlLanguage;
    if (fileName.endsWith(QLatin1String(".qbs"), Qt::CaseInsensitive))
        return QmlQbsLanguage;
    if (fileName.endsWith(QLatin1String(".js"), Qt::CaseInsensitive))
        return JavaScriptLanguage;
    if (fileName.endsWith(QLatin1String(".json"), Qt::CaseInsensitive))
        return JsonLanguage;
    return UnknownLanguage;
}

Document::Ptr Document::ptr() const
{
    return _ptr.toStrongRef();
}

bool Document::isQmlDocument() const
{
    return isQmlLikeLanguage(_language);
}

Document::Language Document::language() const
{
    return _language;
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

const QmlJS::Engine *Document::engine() const
{
    return _engine;
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

int Document::editorRevision() const
{
    return _editorRevision;
}

void Document::setEditorRevision(int revision)
{
    _editorRevision = revision;
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

namespace {
class CollectDirectives : public Directives
{
    QString documentPath;
public:
    CollectDirectives(const QString &documentPath)
        : documentPath(documentPath)
        , isLibrary(false)

    {}

    virtual void pragmaLibrary() { isLibrary = true; }
    virtual void importFile(const QString &jsfile, const QString &module)
    {
        imports += ImportInfo::pathImport(
                    documentPath, jsfile, LanguageUtils::ComponentVersion(), module);
    }

    virtual void importModule(const QString &uri, const QString &version, const QString &module)
    {
        imports += ImportInfo::moduleImport(uri, LanguageUtils::ComponentVersion(version), module);
    }

    bool isLibrary;
    QList<ImportInfo> imports;
};

} // anonymous namespace

bool Document::parse_helper(int startToken)
{
    Q_ASSERT(! _engine);
    Q_ASSERT(! _ast);
    Q_ASSERT(! _bind);

    _engine = new Engine();

    Lexer lexer(_engine);
    Parser parser(_engine);

    QString source = _source;
    lexer.setCode(source, /*line = */ 1, /*qmlMode = */isQmlLikeLanguage(_language));

    CollectDirectives collectDirectives(path());
    _engine->setDirectives(&collectDirectives);

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

    _bind = new Bind(this, &_diagnosticMessages, collectDirectives.isLibrary, collectDirectives.imports);

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

LibraryInfo::LibraryInfo(Status status)
    : _status(status)
    , _dumpStatus(NoTypeInfo)
{
}

LibraryInfo::LibraryInfo(const QmlDirParser &parser)
    : _status(Found)
    , _components(parser.components().values())
    , _plugins(parser.plugins())
    , _typeinfos(parser.typeInfos())
    , _dumpStatus(NoTypeInfo)
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

void Snapshot::insert(const Document::Ptr &document, bool allowInvalid)
{
    if (document && (allowInvalid || document->qmlProgram() || document->jsProgram())) {
        const QString fileName = document->fileName();
        const QString path = document->path();

        remove(fileName);
        _documentsByPath[path].append(document);
        _documents.insert(fileName, document);
    }
}

void Snapshot::insertLibraryInfo(const QString &path, const LibraryInfo &info)
{
    _libraries.insert(QDir::cleanPath(path), info);
}

void Snapshot::remove(const QString &fileName)
{
    Document::Ptr doc = _documents.value(fileName);
    if (!doc.isNull()) {
        const QString &path = doc->path();

        QList<Document::Ptr> docs = _documentsByPath.value(path);
        docs.removeAll(doc);
        _documentsByPath[path] = docs;

        _documents.remove(fileName);
    }
}

Document::MutablePtr Snapshot::documentFromSource(
        const QString &code, const QString &fileName,
        Document::Language language) const
{
    Document::MutablePtr newDoc = Document::create(fileName, language);

    if (Document::Ptr thisDocument = document(fileName))
        newDoc->_editorRevision = thisDocument->_editorRevision;

    newDoc->setSource(code);
    return newDoc;
}

Document::Ptr Snapshot::document(const QString &fileName) const
{
    return _documents.value(QDir::cleanPath(fileName));
}

QList<Document::Ptr> Snapshot::documentsInDirectory(const QString &path) const
{
    return _documentsByPath.value(QDir::cleanPath(path));
}

LibraryInfo Snapshot::libraryInfo(const QString &path) const
{
    return _libraries.value(QDir::cleanPath(path));
}
