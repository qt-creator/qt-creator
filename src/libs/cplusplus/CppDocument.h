/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "Macro.h"

#include <cplusplus/CPlusPlusForwardDeclarations.h>
#include <cplusplus/PreprocessorClient.h>
#include <cplusplus/DependencyTable.h>

#include <utils/fileutils.h>

#include <QSharedPointer>
#include <QDateTime>
#include <QHash>
#include <QFileInfo>
#include <QAtomicInt>

namespace CPlusPlus {

class Macro;
class MacroArgumentReference;
class LookupContext;

class CPLUSPLUS_EXPORT Document
{
    Document(const Document &other);
    void operator =(const Document &other);

    Document(const QString &fileName);

public:
    typedef QSharedPointer<Document> Ptr;

public:
    ~Document();

    unsigned revision() const;
    void setRevision(unsigned revision);

    unsigned editorRevision() const;
    void setEditorRevision(unsigned editorRevision);

    QDateTime lastModified() const;
    void setLastModified(const QDateTime &lastModified);

    QString fileName() const;

    void appendMacro(const Macro &macro);
    void addMacroUse(const Macro &macro,
                     int bytesOffset, int bytesLength,
                     int utf16charsOffset, int utf16charLength,
                     int beginLine, const QVector<MacroArgumentReference> &range);
    void addUndefinedMacroUse(const QByteArray &name,
                              int bytesOffset, int utf16charsOffset);

    Control *control() const;
    Control *swapControl(Control *newControl);
    TranslationUnit *translationUnit() const;

    bool skipFunctionBody() const;
    void setSkipFunctionBody(bool skipFunctionBody);

    int globalSymbolCount() const;
    Symbol *globalSymbolAt(int index) const;

    Namespace *globalNamespace() const;
    void setGlobalNamespace(Namespace *globalNamespace); // ### internal

    QList<Macro> definedMacros() const
    { return _definedMacros; }

    QString functionAt(int line, int column, int *lineOpeningDeclaratorParenthesis = nullptr,
                       int *lineClosingBrace = nullptr) const;
    Symbol *lastVisibleSymbolAt(int line, int column = 0) const;
    Scope *scopeAt(int line, int column = 0);

    QByteArray utf8Source() const;
    void setUtf8Source(const QByteArray &utf8Source);

    QByteArray fingerprint() const { return m_fingerprint; }
    void setFingerprint(const QByteArray &fingerprint)
    { m_fingerprint = fingerprint; }

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

    static Ptr create(const QString &fileName);

    class CPLUSPLUS_EXPORT DiagnosticMessage
    {
    public:
        enum Level {
            Warning,
            Error,
            Fatal
        };

    public:
        DiagnosticMessage(int level, const QString &fileName,
                          int line, int column,
                          const QString &text,
                          int length = 0)
            : _level(level),
              _line(line),
              _fileName(fileName),
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

        QString fileName() const
        { return _fileName; }

        int line() const
        { return _line; }

        int column() const
        { return _column; }

        int length() const
        { return _length; }

        QString text() const
        { return _text; }

        bool operator==(const DiagnosticMessage &other) const;
        bool operator!=(const DiagnosticMessage &other) const;

    private:
        int _level;
        int _line;
        QString _fileName;
        int _column;
        int _length;
        QString _text;
    };

    void addDiagnosticMessage(const DiagnosticMessage &d)
    { _diagnosticMessages.append(d); }

    void clearDiagnosticMessages()
    { _diagnosticMessages.clear(); }

    QList<DiagnosticMessage> diagnosticMessages() const
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
        QString _resolvedFileName;
        QString _unresolvedFileName;
        int _line;
        Client::IncludeType _type;

    public:
        Include(const QString &unresolvedFileName, const QString &resolvedFileName, int line,
                Client::IncludeType type)
            : _resolvedFileName(resolvedFileName)
            , _unresolvedFileName(unresolvedFileName)
            , _line(line)
            , _type(type)
        { }

        QString resolvedFileName() const
        { return _resolvedFileName; }

        QString unresolvedFileName() const
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

        QVector<Block> arguments() const
        { return _arguments; }

        int beginLine() const
        { return _beginLine; }

    private:
        void setArguments(const QVector<Block> &arguments)
        { _arguments = arguments; }

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

    QStringList includedFiles() const;
    void addIncludeFile(const Include &include);

    QList<Include> resolvedIncludes() const
    { return _resolvedIncludes; }

    QList<Include> unresolvedIncludes() const
    { return _unresolvedIncludes; }

    QList<Block> skippedBlocks() const
    { return _skippedBlocks; }

    QList<MacroUse> macroUses() const
    { return _macroUses; }

    QList<UndefinedMacroUse> undefinedMacroUses() const
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
    QString _fileName;
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
    void remove(const Utils::FilePath &fileName); // ### remove
    void remove(const QString &fileName)
    { remove(Utils::FilePath::fromString(fileName)); }

    const_iterator begin() const { return _documents.begin(); }
    const_iterator end() const { return _documents.end(); }

    bool contains(const Utils::FilePath &fileName) const;
    bool contains(const QString &fileName) const
    { return contains(Utils::FilePath::fromString(fileName)); }

    Document::Ptr document(const Utils::FilePath &fileName) const;
    Document::Ptr document(const QString &fileName) const
    { return document(Utils::FilePath::fromString(fileName)); }

    const_iterator find(const Utils::FilePath &fileName) const;
    const_iterator find(const QString &fileName) const
    { return find(Utils::FilePath::fromString(fileName)); }

    Snapshot simplified(Document::Ptr doc) const;

    Document::Ptr preprocessedDocument(const QByteArray &source,
                                       const Utils::FilePath &fileName,
                                       int withDefinedMacrosFromDocumentUntilLine = -1) const;
    Document::Ptr preprocessedDocument(const QByteArray &source,
                                       const QString &fileName,
                                       int withDefinedMacrosFromDocumentUntilLine = -1) const
    {
        return preprocessedDocument(source,
                                    Utils::FilePath::fromString(fileName),
                                    withDefinedMacrosFromDocumentUntilLine);
    }

    Document::Ptr documentFromSource(const QByteArray &preprocessedDocument,
                                     const QString &fileName) const;

    QSet<QString> allIncludesForDocument(const QString &fileName) const;
    QList<IncludeLocation> includeLocationsOfDocument(const QString &fileName) const;

    Utils::FilePaths filesDependingOn(const Utils::FilePath &fileName) const;
    Utils::FilePaths filesDependingOn(const QString &fileName) const
    { return filesDependingOn(Utils::FilePath::fromString(fileName)); }
    void updateDependencyTable() const;

    bool operator==(const Snapshot &other) const;

private:
    mutable DependencyTable m_deps;
    Base _documents;
};

} // namespace CPlusPlus
