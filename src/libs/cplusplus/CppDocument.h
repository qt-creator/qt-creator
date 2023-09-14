// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "Macro.h"

#include <cplusplus/CPlusPlusForwardDeclarations.h>
#include <cplusplus/PreprocessorClient.h>
#include <cplusplus/DependencyTable.h>

#include <utils/filepath.h>

#include <QSharedPointer>
#include <QDateTime>
#include <QHash>
#include <QFuture>
#include <QAtomicInt>

namespace CPlusPlus {

class Macro;
class MacroArgumentReference;
class LookupContext;

class CPLUSPLUS_EXPORT Document
{
    Document(const Document &other);
    void operator =(const Document &other);

    Document(const Utils::FilePath &filePath);

public:
    typedef QSharedPointer<Document> Ptr;

public:
    ~Document();

    unsigned revision() const { return _revision; }
    void setRevision(unsigned revision) { _revision = revision; }

    unsigned editorRevision() const { return _editorRevision; }
    void setEditorRevision(unsigned editorRevision) { _editorRevision = editorRevision; }

    const QDateTime &lastModified() const { return _lastModified; }
    void setLastModified(const QDateTime &lastModified);

    const Utils::FilePath &filePath() const { return _filePath; }

    void appendMacro(const Macro &macro);
    void addMacroUse(const Macro &macro,
                     int bytesOffset, int bytesLength,
                     int utf16charsOffset, int utf16charLength,
                     int beginLine, const QVector<MacroArgumentReference> &range);
    void addUndefinedMacroUse(const QByteArray &name,
                              int bytesOffset, int utf16charsOffset);

    Control *control() const { return _control; }
    Control *swapControl(Control *newControl);
    TranslationUnit *translationUnit() const { return _translationUnit; }

    bool skipFunctionBody() const;
    void setSkipFunctionBody(bool skipFunctionBody);

    int globalSymbolCount() const;
    Symbol *globalSymbolAt(int index) const;

    Namespace *globalNamespace() const { return _globalNamespace; }
    void setGlobalNamespace(Namespace *globalNamespace); // ### internal

    const QList<Macro> &definedMacros() const { return _definedMacros; }

    QString functionAt(int line, int column, int *lineOpeningDeclaratorParenthesis = nullptr,
                       int *lineClosingBrace = nullptr) const;
    Symbol *lastVisibleSymbolAt(int line, int column = 0) const;
    Scope *scopeAt(int line, int column = 0);

    const QByteArray &utf8Source() const { return _source; }
    void setUtf8Source(const QByteArray &utf8Source);

    const QByteArray &fingerprint() const { return m_fingerprint; }
    void setFingerprint(const QByteArray &fingerprint) { m_fingerprint = fingerprint; }

    LanguageFeatures languageFeatures() const;
    void setLanguageFeatures(LanguageFeatures features);

    void startSkippingBlocks(int utf16charsOffset);
    void stopSkippingBlocks(int utf16charsOffset);

    enum ParseMode { // ### keep in sync with CPlusPlus::TranslationUnit
        ParseTranlationUnit,
        ParseDeclaration,
        ParseExpression,
        ParseDeclarator,
        ParseStatement
    };

    bool isTokenized() const;
    void tokenize();

    bool isParsed() const;
    bool parse(ParseMode mode = ParseTranlationUnit);

    enum CheckMode {
        Unchecked,
        FullCheck,
        FastCheck
    };

    void check(CheckMode mode = FullCheck);

    static Ptr create(const Utils::FilePath &filePath);

    class CPLUSPLUS_EXPORT DiagnosticMessage
    {
    public:
        enum Level {
            Warning,
            Error,
            Fatal
        };

    public:
        DiagnosticMessage(int level, const Utils::FilePath &filePath,
                          int line, int column,
                          const QString &text,
                          int length = 0)
            : _level(level),
              _line(line),
              _filePath(filePath),
              _column(column),
              _length(length),
              _text(text)
        { }

        int level() const
        { return _level; }

        bool isWarning() const
        { return _level == Warning; }

        bool isError() const
        { return _level == Error; }

        bool isFatal() const
        { return _level == Fatal; }

        const Utils::FilePath &filePath() const
        { return _filePath; }

        int line() const
        { return _line; }

        int column() const
        { return _column; }

        int length() const
        { return _length; }

        const QString &text() const
        { return _text; }

        bool operator==(const DiagnosticMessage &other) const;
        bool operator!=(const DiagnosticMessage &other) const;

    private:
        int _level;
        int _line;
        Utils::FilePath _filePath;
        int _column;
        int _length;
        QString _text;
    };

    void addDiagnosticMessage(const DiagnosticMessage &d)
    { _diagnosticMessages.append(d); }

    void clearDiagnosticMessages()
    { _diagnosticMessages.clear(); }

    const QList<DiagnosticMessage> &diagnosticMessages() const
    { return _diagnosticMessages; }

    class Block
    {
        int _bytesBegin;
        int _bytesEnd;
        int _utf16charsBegin;
        int _utf16charsEnd;

    public:
        inline Block(int bytesBegin = 0, int bytesEnd = 0,
                     int utf16charsBegin = 0, int utf16charsEnd = 0)
            : _bytesBegin(bytesBegin),
              _bytesEnd(bytesEnd),
              _utf16charsBegin(utf16charsBegin),
              _utf16charsEnd(utf16charsEnd)
        {}

        inline int bytesBegin() const
        { return _bytesBegin; }

        inline int bytesEnd() const
        { return _bytesEnd; }

        inline int utf16charsBegin() const
        { return _utf16charsBegin; }

        inline int utf16charsEnd() const
        { return _utf16charsEnd; }

        bool containsUtf16charOffset(int utf16charOffset) const
        { return utf16charOffset >= _utf16charsBegin && utf16charOffset < _utf16charsEnd; }
    };

    class Include {
        Utils::FilePath _resolvedFileName;
        QString _unresolvedFileName;
        int _line;
        Client::IncludeType _type;

    public:
        Include(const QString &unresolvedFileName, const Utils::FilePath &resolvedFileName, int line,
                Client::IncludeType type)
            : _resolvedFileName(resolvedFileName)
            , _unresolvedFileName(unresolvedFileName)
            , _line(line)
            , _type(type)
        { }

        const Utils::FilePath &resolvedFileName() const
        { return _resolvedFileName; }

        const QString &unresolvedFileName() const
        { return _unresolvedFileName; }

        int line() const
        { return _line; }

        Client::IncludeType type() const
        { return _type; }
    };

    class MacroUse: public Block {
        Macro _macro;
        QVector<Block> _arguments;
        int _beginLine;

    public:
        inline MacroUse(const Macro &macro,
                        int bytesBegin, int bytesEnd,
                        int utf16charsBegin, int utf16charsEnd,
                        int beginLine)
            : Block(bytesBegin, bytesEnd, utf16charsBegin, utf16charsEnd),
              _macro(macro),
              _beginLine(beginLine)
        { }

        const Macro &macro() const
        { return _macro; }

        bool isFunctionLike() const
        { return _macro.isFunctionLike(); }

        const QVector<Block> &arguments() const
        { return _arguments; }

        int beginLine() const
        { return _beginLine; }

    private:
        void addArgument(const Block &block)
        { _arguments.append(block); }

        friend class Document;
    };

    class UndefinedMacroUse: public Block {
        QByteArray _name;

    public:
        inline UndefinedMacroUse(
                const QByteArray &name,
                int bytesBegin,
                int utf16charsBegin)
            : Block(bytesBegin,
                    bytesBegin + name.length(),
                    utf16charsBegin,
                    utf16charsBegin + QString::fromUtf8(name, name.size()).size()),
              _name(name)
        { }

        QByteArray name() const
        {
            return _name;
        }
    };

    enum class Duplicates {
        Remove,
        Keep,
    };

    Utils::FilePaths includedFiles(Duplicates duplicates = Duplicates::Remove) const;
    void addIncludeFile(const Include &include);

    const QList<Include> &resolvedIncludes() const
    { return _resolvedIncludes; }

    const QList<Include> &unresolvedIncludes() const
    { return _unresolvedIncludes; }

    const QList<Block> &skippedBlocks() const
    { return _skippedBlocks; }

    const QList<MacroUse> macroUses() const
    { return _macroUses; }

    const QList<UndefinedMacroUse> &undefinedMacroUses() const
    { return _undefinedMacroUses; }

    void setIncludeGuardMacroName(const QByteArray &includeGuardMacroName)
    { _includeGuardMacroName = includeGuardMacroName; }
    QByteArray includeGuardMacroName() const
    { return _includeGuardMacroName; }

    const Macro *findMacroDefinitionAt(int line) const;
    const MacroUse *findMacroUseAt(int utf16charsOffset) const;
    const UndefinedMacroUse *findUndefinedMacroUseAt(int utf16charsOffset) const;

    void keepSourceAndAST();
    void releaseSourceAndAST();

    CheckMode checkMode() const
    { return static_cast<CheckMode>(_checkMode); }

private:
    Utils::FilePath _filePath;
    Control *_control;
    TranslationUnit *_translationUnit;
    Namespace *_globalNamespace;

    /// All messages generated during lexical/syntactic/semantic analysis.
    QList<DiagnosticMessage> _diagnosticMessages;

    QList<Include> _resolvedIncludes;
    QList<Include> _unresolvedIncludes;
    QList<Macro> _definedMacros;
    QList<Block> _skippedBlocks;
    QList<MacroUse> _macroUses;
    QList<UndefinedMacroUse> _undefinedMacroUses;

     /// the macro name of the include guard, if there is one.
    QByteArray _includeGuardMacroName;

    QByteArray m_fingerprint;

    QByteArray _source;
    QDateTime _lastModified;
    QAtomicInt _keepSourceAndASTCount;
    unsigned _revision;
    unsigned _editorRevision;
    quint8 _checkMode;

    friend class Snapshot;
};

class CPLUSPLUS_EXPORT Snapshot
{
    typedef QHash<Utils::FilePath, Document::Ptr> Base;

public:
    Snapshot();
    ~Snapshot();

    typedef Base::const_iterator iterator;
    typedef Base::const_iterator const_iterator;
    typedef QPair<Document::Ptr, int> IncludeLocation;

    int size() const; // ### remove
    bool isEmpty() const;

    void insert(Document::Ptr doc); // ### remove
    void remove(const Utils::FilePath &filePath); // ### remove

    const_iterator begin() const { return _documents.begin(); }
    const_iterator end() const { return _documents.end(); }

    bool contains(const Utils::FilePath &filePath) const;

    Document::Ptr document(const Utils::FilePath &filePath) const;

    const_iterator find(const Utils::FilePath &filePath) const;

    Snapshot simplified(Document::Ptr doc) const;

    Document::Ptr preprocessedDocument(const QByteArray &source,
                                       const Utils::FilePath &filePath,
                                       int withDefinedMacrosFromDocumentUntilLine = -1) const;

    Document::Ptr documentFromSource(const QByteArray &preprocessedDocument,
                                     const Utils::FilePath &filePath) const;

    QSet<Utils::FilePath> allIncludesForDocument(const Utils::FilePath &filePath) const;

    QList<IncludeLocation> includeLocationsOfDocument(const Utils::FilePath &fileNameOrPath) const;

    Utils::FilePaths filesDependingOn(const Utils::FilePath &filePath) const;

    void updateDependencyTable(const std::optional<QFuture<void>> &future = {}) const;

    bool operator==(const Snapshot &other) const;

private:
    mutable DependencyTable m_deps;
    Base _documents;
};

} // namespace CPlusPlus
