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

#ifndef CPLUSPLUS_CPPDOCUMENT_H
#define CPLUSPLUS_CPPDOCUMENT_H

#include "Macro.h"

#include <cplusplus/CPlusPlusForwardDeclarations.h>

#include <QSharedPointer>
#include <QDateTime>
#include <QHash>
#include <QFileInfo>
#include <QAtomicInt>

// in debug mode: make dumpers widely available without an extra include
#ifdef QT_DEBUG
#include "Dumpers.h"
#endif

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

    QStringList includedFiles() const;
    void addIncludeFile(const QString &fileName, unsigned line);

    void appendMacro(const Macro &macro);
    void addMacroUse(const Macro &macro, unsigned offset, unsigned length,
                     unsigned beginLine,
                     const QVector<MacroArgumentReference> &range);
    void addUndefinedMacroUse(const QByteArray &name, unsigned offset);

    Control *control() const;
    TranslationUnit *translationUnit() const;

    bool skipFunctionBody() const;
    void setSkipFunctionBody(bool skipFunctionBody);

    unsigned globalSymbolCount() const;
    Symbol *globalSymbolAt(unsigned index) const;

    Namespace *globalNamespace() const;
    void setGlobalNamespace(Namespace *globalNamespace); // ### internal

    QList<Macro> definedMacros() const
    { return _definedMacros; }

    Symbol *lastVisibleSymbolAt(unsigned line, unsigned column = 0) const;
    Scope *scopeAt(unsigned line, unsigned column = 0);

    QByteArray utf8Source() const;
    void setUtf8Source(const QByteArray &utf8Source);

    void startSkippingBlocks(unsigned offset);
    void stopSkippingBlocks(unsigned offset);

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
                          unsigned line, unsigned column,
                          const QString &text,
                          unsigned length = 0)
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

        unsigned line() const
        { return _line; }

        unsigned column() const
        { return _column; }

        unsigned length() const
        { return _length; }

        QString text() const
        { return _text; }

        bool operator==(const DiagnosticMessage &other) const;
        bool operator!=(const DiagnosticMessage &other) const;

    private:
        int _level;
        unsigned _line;
        QString _fileName;
        unsigned _column;
        unsigned _length;
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
        unsigned _begin;
        unsigned _end;

    public:
        inline Block(unsigned begin = 0, unsigned end = 0)
            : _begin(begin), _end(end)
        { }

        inline bool isNull() const
        { return length() == 0; }

        inline unsigned position() const
        { return _begin; }

        inline unsigned length() const
        { return _end - _begin; }

        inline unsigned begin() const
        { return _begin; }

        inline unsigned end() const
        { return _end; }

        bool contains(unsigned pos) const
        { return pos >= _begin && pos < _end; }
    };

    class Include {
        QString _fileName;
        unsigned _line;

    public:
        Include(const QString &fileName, unsigned line)
            : _fileName(fileName), _line(line)
        { }

        QString fileName() const
        { return _fileName; }

        unsigned line() const
        { return _line; }

        bool resolved() const
        { return QFileInfo(_fileName).isAbsolute(); }
    };

    class MacroUse: public Block {
        Macro _macro;
        QVector<Block> _arguments;
        unsigned _beginLine;

    public:
        inline MacroUse(const Macro &macro, unsigned begin, unsigned end, unsigned beginLine)
            : Block(begin, end),
              _macro(macro),
              _beginLine(beginLine)
        { }

        const Macro &macro() const
        { return _macro; }

        bool isFunctionLike() const
        { return _macro.isFunctionLike(); }

        QVector<Block> arguments() const
        { return _arguments; }

        unsigned beginLine() const
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
                unsigned begin)
            : Block(begin, begin + name.length()),
              _name(name)
        { }

        QByteArray name() const
        {
            return _name;
        }
    };

    QList<Include> includes() const
    { return _includes; }

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

    const Macro *findMacroDefinitionAt(unsigned line) const;
    const MacroUse *findMacroUseAt(unsigned offset) const;
    const UndefinedMacroUse *findUndefinedMacroUseAt(unsigned offset) const;

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

    QList<Include> _includes;
    QList<Macro> _definedMacros;
    QList<Block> _skippedBlocks;
    QList<MacroUse> _macroUses;
    QList<UndefinedMacroUse> _undefinedMacroUses;

     /// the macro name of the include guard, if there is one.
    QByteArray _includeGuardMacroName;

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
    typedef QHash<QString, Document::Ptr> _Base;

public:
    Snapshot();
    ~Snapshot();

    typedef _Base::const_iterator iterator;
    typedef _Base::const_iterator const_iterator;

    int size() const; // ### remove
    bool isEmpty() const;

    void insert(Document::Ptr doc); // ### remove
    void remove(const QString &fileName); // ### remove

    const_iterator begin() const { return _documents.begin(); }
    const_iterator end() const { return _documents.end(); }

    bool contains(const QString &fileName) const;
    Document::Ptr document(const QString &fileName) const;

    const_iterator find(const QString &fileName) const;

    Snapshot simplified(Document::Ptr doc) const;

    Document::Ptr preprocessedDocument(const QString &source,
                                       const QString &fileName) const;

    Document::Ptr documentFromSource(const QByteArray &preprocessedDocument,
                                     const QString &fileName) const;

    QSet<QString> allIncludesForDocument(const QString &fileName) const;

private:
    void allIncludesForDocument_helper(const QString &fileName, QSet<QString> &result) const;

private:
    _Base _documents;
};

} // namespace CPlusPlus

#endif // CPLUSPLUS_CPPDOCUMENT_H
