// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QList>
#include <QSharedPointer>
#include <QString>

#include <languageutils/fakemetaobject.h>

#include "qmljsdialect.h"
#include "parser/qmldirparser_p.h"
#include "parser/qmljsastfwd_p.h"
#include "qmljs_global.h"
#include "qmljsconstants.h"
#include "qmljsimportdependencies.h"

namespace QmlJS {

class Bind;
class DiagnosticMessage;
class Engine;
class Snapshot;
class ImportDependencies;

class QMLJS_EXPORT Document
{
    Q_DISABLE_COPY(Document)
public:
    typedef QSharedPointer<const Document> Ptr;
    typedef QSharedPointer<Document> MutablePtr;
protected:
    Document(const Utils::FilePath &fileName, Dialect language);

public:
    ~Document();

    static MutablePtr create(const Utils::FilePath &fileName, Dialect language);

    Document::Ptr ptr() const;

    bool isQmlDocument() const;
    Dialect language() const;
    void setLanguage(Dialect l);

    QString importId() const;
    QByteArray fingerprint() const;
    AST::UiProgram *qmlProgram() const;
    AST::Program *jsProgram() const;
    AST::ExpressionNode *expression() const;
    AST::Node *ast() const;

    const QmlJS::Engine *engine() const;

    QList<DiagnosticMessage> diagnosticMessages() const;

    QString source() const;
    void setSource(const QString &source);

    bool parse();
    bool parseQml();
    bool parseJavaScript();
    bool parseExpression();

    bool isParsedCorrectly() const
    { return _parsedCorrectly; }

    Bind *bind() const;

    int editorRevision() const;
    void setEditorRevision(int revision);

    Utils::FilePath fileName() const;
    Utils::FilePath path() const;
    QString componentName() const;

    QList<SourceLocation> jsDirectives() const;

private:
    bool parse_helper(int kind);

private:
    QmlJS::Engine *_engine;
    AST::Node *_ast;
    Bind *_bind;
    QList<QmlJS::DiagnosticMessage> _diagnosticMessages;
    Utils::FilePath _fileName;
    Utils::FilePath _path;
    QString _componentName;
    QString _source;
    QList<SourceLocation> _jsdirectives;
    QWeakPointer<Document> _ptr;
    QByteArray _fingerprint;
    int _editorRevision;
    Dialect _language;
    bool _parsedCorrectly;

    // for documentFromSource
    friend class Snapshot;
};

class QMLJS_EXPORT ModuleApiInfo
{
public:
    QString uri;
    LanguageUtils::ComponentVersion version;
    QString cppName;

    void addToHash(QCryptographicHash &hash) const;
};

class QMLJS_EXPORT LibraryInfo
{
public:
    enum PluginTypeInfoStatus {
        NoTypeInfo,
        DumpDone,
        DumpError,
        TypeInfoFileDone,
        TypeInfoFileError
    };

    enum Status {
        NotScanned,
        NotFound,
        Found
    };

private:
    Status _status = NotScanned;
    QList<QmlDirParser::Component> _components;
    QList<QmlDirParser::Plugin> _plugins;
    QStringList _typeinfos;
    typedef QList<LanguageUtils::FakeMetaObject::ConstPtr> FakeMetaObjectList;
    FakeMetaObjectList _metaObjects;
    QList<ModuleApiInfo> _moduleApis;
    QStringList _dependencies; // from qmltypes "dependencies: [...]"
    QList<QmlDirParser::Import> _imports; // from qmldir "import" commands
    QByteArray _fingerprint;

    PluginTypeInfoStatus _dumpStatus = NoTypeInfo;
    QString _dumpError;

public:
    LibraryInfo();
    explicit LibraryInfo(Status status);
    explicit LibraryInfo(const QString &typeInfo);
    explicit LibraryInfo(const QmlDirParser &parser, const QByteArray &fingerprint = QByteArray());
    ~LibraryInfo() = default;

    QByteArray calculateFingerprint() const;
    void updateFingerprint();
    QByteArray fingerprint() const
    { return _fingerprint; }

    const QList<QmlDirParser::Component> components() const
    { return _components; }

    QList<QmlDirParser::Plugin> plugins() const
    { return _plugins; }

    const QStringList typeInfos() const
    { return _typeinfos; }

    FakeMetaObjectList metaObjects() const
    { return _metaObjects; }

    void setMetaObjects(const FakeMetaObjectList &objects)
    { _metaObjects = objects; }

    const QList<ModuleApiInfo> moduleApis() const
    { return _moduleApis; }

    void setModuleApis(const QList<ModuleApiInfo> &apis)
    { _moduleApis = apis; }

    QStringList dependencies() const
    { return _dependencies; }

    void setDependencies(const QStringList &deps)
    { _dependencies = deps; }

    QList<QmlDirParser::Import> imports() const
    { return _imports; }

    void setImports(const QList<QmlDirParser::Import> &imports)
    { _imports = imports; }

    bool isValid() const
    { return _status == Found; }

    bool wasScanned() const
    { return _status != NotScanned; }

    bool wasFound() const
    { return _status != NotFound; }

    PluginTypeInfoStatus pluginTypeInfoStatus() const
    { return _dumpStatus; }

    QString pluginTypeInfoError() const
    { return _dumpError; }

    void setPluginTypeInfoStatus(PluginTypeInfoStatus dumped, const QString &error = QString())
    { _dumpStatus = dumped; _dumpError = error; }
};

class QMLJS_EXPORT Snapshot
{
    typedef QHash<Utils::FilePath, Document::Ptr> Base;
    QHash<Utils::FilePath, Document::Ptr> _documents;
    QHash<Utils::FilePath, QList<Document::Ptr>> _documentsByPath;
    QHash<Utils::FilePath, LibraryInfo> _libraries;
    ImportDependencies _dependencies;

public:
    Snapshot();
    ~Snapshot();

    typedef Base::iterator iterator;
    typedef Base::const_iterator const_iterator;

    const_iterator begin() const { return _documents.begin(); }
    const_iterator end() const { return _documents.end(); }

    void insert(const Document::Ptr &document, bool allowInvalid = false);
    void insertLibraryInfo(const Utils::FilePath &path, const LibraryInfo &info);
    void remove(const Utils::FilePath &fileName);

    const ImportDependencies *importDependencies() const;
    ImportDependencies *importDependencies();

    Document::Ptr document(const Utils::FilePath &fileName) const;
    QList<Document::Ptr> documentsInDirectory(const Utils::FilePath &path) const;
    LibraryInfo libraryInfo(const Utils::FilePath &path) const;

    Document::MutablePtr documentFromSource(const QString &code,
                                            const Utils::FilePath &fileName,
                                            Dialect language) const;
};

} // namespace QmlJS
