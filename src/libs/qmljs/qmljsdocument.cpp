// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmljsdocument.h"
#include "qmljsbind.h"
#include "qmljsconstants.h"
#include "qmljsimportdependencies.h"
#include <qmljs/parser/qmljslexer_p.h>
#include <qmljs/parser/qmljsparser_p.h>

#include <utils/qtcassert.h>

#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>

#include <algorithm>

static constexpr int sizeofQChar = int(sizeof(QChar));

using namespace QmlJS;
using namespace QmlJS::AST;

/*!
    \class QmlJS::Document
    \brief The Document class creates a QML or JavaScript document.
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
    \brief The LibraryInfo class creates a QML library.
    \sa Snapshot

    A LibraryInfo is created when the ModelManagerInterface finds
    a QML library and parses the qmldir file. The instance holds information about
    which Components the library provides and which plugins to load.

    The ModelManager will try to extract detailed information about the types
    defined in the plugins this library loads. Once it is done, the data will
    be available through the metaObjects() function.
*/

/*!
    \class QmlJS::Snapshot
    \brief The Snapshot class holds and offers access to a set of
    Document::Ptr and LibraryInfo instances.
    \sa Document LibraryInfo

    Usually Snapshots are copies of the snapshot maintained and updated by the
    ModelManagerInterface that updates its instance as parsing
    threads finish and new information becomes available.
*/

Document::Document(const Utils::FilePath &fileName, Dialect language)
    : _engine(nullptr)
    , _ast(nullptr)
    , _bind(nullptr)
    , _fileName(fileName.cleanPath())
    , _editorRevision(0)
    , _language(language)
    , _parsedCorrectly(false)
{
    _path = fileName.absoluteFilePath().parentDir().cleanPath();

    if (language.isQmlLikeLanguage()) {
        _componentName = fileName.baseName();

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

Document::MutablePtr Document::create(const Utils::FilePath &fileName, Dialect language)
{
    Document::MutablePtr doc(new Document(fileName, language));
    doc->_ptr = doc;
    return doc;
}

Document::Ptr Document::ptr() const
{
    return _ptr.toStrongRef();
}

bool Document::isQmlDocument() const
{
    return _language.isQmlLikeLanguage();
}

Dialect Document::language() const
{
    return _language;
}

void Document::setLanguage(Dialect l)
{
    _language = l;
}

QString Document::importId() const
{
    return _fileName.toString();
}

QByteArray Document::fingerprint() const
{
    return _fingerprint;
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

    return nullptr;
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
    QCryptographicHash sha(QCryptographicHash::Sha1);
    sha.addData(source.toUtf8());
    _fingerprint = sha.result();
}

int Document::editorRevision() const
{
    return _editorRevision;
}

void Document::setEditorRevision(int revision)
{
    _editorRevision = revision;
}

Utils::FilePath Document::fileName() const
{
    return _fileName;

}

Utils::FilePath Document::path() const
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
    void addLocation(int line, int column) {
        const SourceLocation loc = SourceLocation(
                    0,  // placeholder
                    0,  // placeholder
                    static_cast<quint32>(line),
                    static_cast<quint32>(column));
        _locations += loc;
    }

    QList<SourceLocation> _locations;

public:
    CollectDirectives(const Utils::FilePath &documentPath)
        : documentPath(documentPath)
        , isLibrary(false)

    {}

    void importFile(const QString &jsfile, const QString &module,
                    int line, int column) override
    {
        imports += ImportInfo::pathImport(
                    documentPath, jsfile, LanguageUtils::ComponentVersion(), module);
        addLocation(line, column);
    }

    void importModule(const QString &uri, const QString &version, const QString &module,
                      int line, int column) override
    {
        imports += ImportInfo::moduleImport(uri, LanguageUtils::ComponentVersion(version), module);
        addLocation(line, column);
    }

    void pragmaLibrary() override
    {
        isLibrary = true;
    }

    virtual QList<SourceLocation> locations() { return _locations; }

    const Utils::FilePath documentPath;
    bool isLibrary;
    QList<ImportInfo> imports;
};

} // anonymous namespace


QList<SourceLocation> Document::jsDirectives() const
{
    return _jsdirectives;
}

bool Document::parse_helper(int startToken)
{
    Q_ASSERT(! _engine);
    Q_ASSERT(! _ast);
    Q_ASSERT(! _bind);

    _engine = new Engine();

    Lexer lexer(_engine);
    Parser parser(_engine);

    QString source = _source;
    lexer.setCode(source, /*line = */ 1, /*qmlMode = */_language.isQmlLikeLanguage());

    CollectDirectives directives = CollectDirectives(path());
    _engine->setDirectives(&directives);

    switch (startToken) {
    case QmlJSGrammar::T_FEED_UI_PROGRAM:
        _parsedCorrectly = parser.parse();
        break;
    case QmlJSGrammar::T_FEED_JS_SCRIPT:
    case QmlJSGrammar::T_FEED_JS_MODULE: {
        _parsedCorrectly = parser.parseProgram();
        const QList<SourceLocation> locations = directives.locations();
        for (const auto &d : locations) {
            _jsdirectives << d;
        }
    } break;

    case QmlJSGrammar::T_FEED_JS_EXPRESSION:
        _parsedCorrectly = parser.parseExpression();
        break;
    default:
        Q_ASSERT(0);
    }

    _ast = parser.rootNode();
    _diagnosticMessages = parser.diagnosticMessages();

    _bind = new Bind(this, &_diagnosticMessages, directives.isLibrary, directives.imports);

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
    return parse_helper(QmlJSGrammar::T_FEED_JS_SCRIPT);
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
{
    static const QByteArray emptyFingerprint = calculateFingerprint();
    _fingerprint = emptyFingerprint;
}

LibraryInfo::LibraryInfo(Status status)
    : _status(status)
{
    updateFingerprint();
}

LibraryInfo::LibraryInfo(const QString &typeInfo)
    : _status(Found)
{
    _typeinfos.append(typeInfo);
    updateFingerprint();
}

LibraryInfo::LibraryInfo(const QmlDirParser &parser, const QByteArray &fingerprint)
    : _status(Found)
    , _components(parser.components().values())
    , _plugins(parser.plugins())
    , _typeinfos(parser.typeInfos())
    , _imports(parser.imports())
    , _fingerprint(fingerprint)
{
    if (_fingerprint.isEmpty())
        updateFingerprint();
}

QByteArray LibraryInfo::calculateFingerprint() const
{
    QCryptographicHash hash(QCryptographicHash::Sha1);
    auto addData = [&hash](auto p, size_t len) {
        hash.addData(reinterpret_cast<const char *>(p), len);
    };

    addData(&_status, sizeof(_status));
    int len = _components.size();
    addData(&len, sizeof(len));
    for (const QmlDirParser::Component &component : _components) {
        len = component.fileName.size();
        addData(&len, sizeof(len));
        addData(component.fileName.constData(), len * sizeofQChar);
        addData(&component.majorVersion, sizeof(component.majorVersion));
        addData(&component.minorVersion, sizeof(component.minorVersion));
        len = component.typeName.size();
        addData(&len, sizeof(len));
        addData(component.typeName.constData(), component.typeName.size() * sizeofQChar);
        int flags = (component.singleton ?  (1 << 0) : 0) + (component.internal ? (1 << 1) : 0);
        addData(&flags, sizeof(flags));
    }
    len = _plugins.size();
    addData(&len, sizeof(len));
    for (const QmlDirParser::Plugin &plugin : _plugins) {
        len = plugin.path.size();
        addData(&len, sizeof(len));
        addData(plugin.path.constData(), len * sizeofQChar);
        len = plugin.name.size();
        addData(&len, sizeof(len));
        addData(plugin.name.constData(), len * sizeofQChar);
    }
    len = _typeinfos.size();
    addData(&len, sizeof(len));
    for (const QString &typeinfo : _typeinfos) {
        len = typeinfo.size();
        addData(&len, sizeof(len));
        addData(typeinfo.constData(), len * sizeofQChar);
    }
    len = _metaObjects.size();
    addData(&len, sizeof(len));
    QList<QByteArray> metaFingerprints;
    for (const LanguageUtils::FakeMetaObject::ConstPtr &metaObject : _metaObjects)
        metaFingerprints.append(metaObject->fingerprint());
    std::sort(metaFingerprints.begin(), metaFingerprints.end());
    for (const QByteArray &fp : std::as_const(metaFingerprints))
        hash.addData(fp);
    addData(&_dumpStatus, sizeof(_dumpStatus));
    len = _dumpError.size(); // localization dependent (avoid?)
    addData(&len, sizeof(len));
    addData(_dumpError.constData(), len * sizeofQChar);

    len = _moduleApis.size();
    addData(&len, sizeof(len));
    for (const ModuleApiInfo &moduleInfo : _moduleApis)
        moduleInfo.addToHash(hash); // make it order independent?

    len = _imports.size();
    addData(&len, sizeof(len));
    for (const QmlDirParser::Import &import : _imports)
        hash.addData(import.module.toUtf8()); // import order matters, keep order-dependent

    QByteArray res(hash.result());
    res.append('L');
    return res;
}

void LibraryInfo::updateFingerprint()
{
    _fingerprint = calculateFingerprint();
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
        const Utils::FilePath fileName = document->fileName();
        const Utils::FilePath path = document->path();
        remove(fileName);
        _documentsByPath[path].append(document);
        _documents.insert(fileName, document);
        CoreImport cImport;
        cImport.importId = document->importId();
        cImport.language = document->language();
        cImport.addPossibleExport(
            Export(ImportKey(ImportType::File, fileName.toString()), {}, true, fileName.baseName()));
        cImport.fingerprint = document->fingerprint();
        _dependencies.addCoreImport(cImport);
    }
}

void Snapshot::insertLibraryInfo(const Utils::FilePath &path, const LibraryInfo &info)
{
    QTC_CHECK(!path.isEmpty());
    QTC_CHECK(info.fingerprint() == info.calculateFingerprint());
    _libraries.insert(path.cleanPath(), info);
    if (!info.wasFound()) return;
    CoreImport cImport;
    cImport.importId = path.toString();
    cImport.language = Dialect::AnyLanguage;
    QSet<ImportKey> packages;
    for (const ModuleApiInfo &moduleInfo : info.moduleApis()) {
        ImportKey iKey(ImportType::Library, moduleInfo.uri, moduleInfo.version.majorVersion(),
                       moduleInfo.version.minorVersion());
        packages.insert(iKey);
    }
    const QList<LanguageUtils::FakeMetaObject::ConstPtr> metaObjects = info.metaObjects();
    for (const LanguageUtils::FakeMetaObject::ConstPtr &metaO : metaObjects) {
        for (const LanguageUtils::FakeMetaObject::Export &e : metaO->exports()) {
            ImportKey iKey(ImportType::Library, e.package, e.version.majorVersion(),
                           e.version.minorVersion());
            packages.insert(iKey);
        }
    }

    QStringList splitPath = path.path().split(QLatin1Char('/'));
    const QRegularExpression vNr(QLatin1String("^(.+)\\.([0-9]+)(?:\\.([0-9]+))?$"));
    const QRegularExpression safeName(QLatin1String("^[a-zA-Z_][[a-zA-Z0-9_]*$"));
    for (const ImportKey &importKey : std::as_const(packages)) {
        if (importKey.splitPath.size() == 1 && importKey.splitPath.at(0).isEmpty() && splitPath.length() > 0) {
            // relocatable
            QStringList myPath = splitPath;
            QRegularExpressionMatch match = vNr.match(myPath.last());
            if (match.hasMatch())
                myPath.last() = match.captured(1);
            for (int iPath = myPath.size(); iPath != 1; ) {
                --iPath;
                match = safeName.match(myPath.at(iPath));
                if (!match.hasMatch())
                    break;
                ImportKey iKey(ImportType::Library, QStringList(myPath.mid(iPath)).join(QLatin1Char('.')),
                               importKey.majorVersion, importKey.minorVersion);
                Utils::FilePath newP = path.withNewPath(
                            (iPath == 1)
                                 ? QLatin1String("/")
                                 : QStringList(myPath.mid(0, iPath)).join(QLatin1Char('/')));
                cImport.addPossibleExport(Export(iKey, newP, true));
            }
        } else {
            Utils::FilePath requiredPath = path.withNewPath(
                QStringList(splitPath.mid(0, splitPath.size() - importKey.splitPath.size()))
                    .join(QLatin1String("/")));
            cImport.addPossibleExport(Export(importKey, requiredPath, true));
        }
    }
    if (cImport.possibleExports.isEmpty() && splitPath.size() > 0) {
        const QRegularExpression vNr(QLatin1String("^(.+)\\.([0-9]+)(?:\\.([0-9]+))?$"));
        const QRegularExpression safeName(QLatin1String("^[a-zA-Z_][[a-zA-Z0-9_]*$"));
        int majorVersion = LanguageUtils::ComponentVersion::NoVersion;
        int minorVersion = LanguageUtils::ComponentVersion::NoVersion;

        for (const QmlDirParser::Component &component : info.components()) {
            if (component.majorVersion > majorVersion)
                majorVersion = component.majorVersion;
            if (component.minorVersion > minorVersion)
                minorVersion = component.minorVersion;
        }

        QRegularExpressionMatch match = vNr.match(splitPath.last());
        if (match.hasMatch()) {
            splitPath.last() = match.captured(1);
            bool ok;
            majorVersion = match.captured(2).toInt(&ok);
            if (!ok)
                majorVersion = LanguageUtils::ComponentVersion::NoVersion;
            minorVersion = match.captured(3).toInt(&ok);
            if (match.captured(3).isEmpty() || !ok)
                minorVersion = LanguageUtils::ComponentVersion::NoVersion;
        }

        for (int iPath = splitPath.size(); iPath != 1; ) {
            --iPath;
            match = safeName.match(splitPath.at(iPath));
            if (!match.hasMatch())
                break;
            ImportKey iKey(ImportType::Library, QStringList(splitPath.mid(iPath)).join(QLatin1Char('.')),
                           majorVersion, minorVersion);
            Utils::FilePath newP = path.withNewPath(
                        iPath == 1
                             ? QLatin1String("/")
                             : QStringList(splitPath.mid(0, iPath)).join(QLatin1Char('/')));
            cImport.addPossibleExport(Export(iKey, newP, true));
        }
    }
    for (const QmlDirParser::Component &component : info.components()) {
        for (const Export &e : std::as_const(cImport.possibleExports))
            _dependencies.addExport(component.fileName, e.exportName, e.pathRequired, e.typeName);
    }

    cImport.fingerprint = info.fingerprint();
    _dependencies.addCoreImport(cImport);
}

void Snapshot::remove(const Utils::FilePath &fileName)
{
    Document::Ptr doc = _documents.value(fileName);
    if (!doc.isNull()) {
        const Utils::FilePath &path = doc->path();

        QList<Document::Ptr> docs = _documentsByPath.value(path);
        docs.removeAll(doc);
        _documentsByPath[path] = docs;

        _documents.remove(fileName);
    }
}

const QmlJS::ImportDependencies *Snapshot::importDependencies() const
{
    return &_dependencies;
}

QmlJS::ImportDependencies *Snapshot::importDependencies()
{
    return &_dependencies;
}

Document::MutablePtr Snapshot::documentFromSource(const QString &code,
                                                  const Utils::FilePath &fileName,
                                                  Dialect language) const
{
    Document::MutablePtr newDoc = Document::create(fileName, language);

    if (Document::Ptr thisDocument = document(fileName))
        newDoc->_editorRevision = thisDocument->_editorRevision;

    newDoc->setSource(code);
    return newDoc;
}

Document::Ptr Snapshot::document(const Utils::FilePath &fileName) const
{
    return _documents.value(fileName.cleanPath());
}

QList<Document::Ptr> Snapshot::documentsInDirectory(const Utils::FilePath &path) const
{
    return _documentsByPath.value(path.cleanPath());
}

LibraryInfo Snapshot::libraryInfo(const Utils::FilePath &path) const
{
    return _libraries.value(path.cleanPath());
}

void ModuleApiInfo::addToHash(QCryptographicHash &hash) const
{
    int len = uri.length();
    hash.addData(reinterpret_cast<const char *>(&len), sizeof(len));
    hash.addData(reinterpret_cast<const char *>(uri.constData()), len * sizeofQChar);
    version.addToHash(hash);
    len = cppName.length();
    hash.addData(reinterpret_cast<const char *>(&len), sizeof(len));
    hash.addData(reinterpret_cast<const char *>(cppName.constData()), len * sizeofQChar);
}
