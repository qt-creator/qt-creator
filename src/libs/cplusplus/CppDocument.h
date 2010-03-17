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

#ifndef CPPDOCUMENT_H
#define CPPDOCUMENT_H

#include <CPlusPlusForwardDeclarations.h>
#include "Macro.h"

#include <QtCore/QSharedPointer>
#include <QtCore/QDateTime>
#include <QtCore/QHash>
#include <QtCore/QFileInfo>

namespace CPlusPlus {

class Macro;
class MacroArgumentReference;
class NamespaceBinding;

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
                     unsigned beginLine, const QVector<MacroArgumentReference> &range,
                     bool inCondition);
    void addUndefinedMacroUse(const QByteArray &name, unsigned offset);

    Control *control() const;
    TranslationUnit *translationUnit() const;

    bool skipFunctionBody() const;
    void setSkipFunctionBody(bool skipFunctionBody);

    unsigned globalSymbolCount() const;
    Symbol *globalSymbolAt(unsigned index) const;
    Scope *globalSymbols() const; // ### deprecate?
    Namespace *globalNamespace() const;

    QList<Macro> definedMacros() const
    { return _definedMacros; }

    Symbol *findSymbolAt(unsigned line, unsigned column) const;

    QByteArray source() const;
    void setSource(const QByteArray &source);

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
        FullCheck,
        FastCheck
    };

    void check(CheckMode mode = FullCheck);

    void releaseSource();
    void releaseTranslationUnit();

    static Ptr create(const QString &fileName);

    class DiagnosticMessage
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
                          const QString &text)
            : _level(level),
              _fileName(fileName),
              _line(line),
              _column(column),
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

        QString text() const
        { return _text; }

    private:
        int _level;
        QString _fileName;
        unsigned _line;
        unsigned _column;
        QString _text;
    };

    void addDiagnosticMessage(const DiagnosticMessage &d)
    { _diagnosticMessages.append(d); }

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
        bool _inCondition;
        unsigned _beginLine;

    public:
        inline MacroUse(const Macro &macro,
                        unsigned begin, unsigned end, unsigned beginLine)
            : Block(begin, end),
              _macro(macro),
              _inCondition(false),
              _beginLine(beginLine)
        { }

        const Macro &macro() const
        { return _macro; }

        bool isFunctionLike() const
        { return _macro.isFunctionLike(); }

        QVector<Block> arguments() const
        { return _arguments; }

        bool isInCondition() const
        { return _inCondition; }

        unsigned beginLine() const
        { return _beginLine; }

    private:
        void setArguments(const QVector<Block> &arguments)
        { _arguments = arguments; }

        void addArgument(const Block &block)
        { _arguments.append(block); }

        void setInCondition(bool set)
        { _inCondition = set; }

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

    const Macro *findMacroDefinitionAt(unsigned line) const;
    const MacroUse *findMacroUseAt(unsigned offset) const;
    const UndefinedMacroUse *findUndefinedMacroUseAt(unsigned offset) const;

private:
    Symbol *findSymbolAt(unsigned line, unsigned column, Scope *scope) const;

private:
    QString _fileName;
    Control *_control;
    TranslationUnit *_translationUnit;
    Namespace *_globalNamespace;
    QList<DiagnosticMessage> _diagnosticMessages;
    QList<Include> _includes;
    QList<Macro> _definedMacros;
    QList<Block> _skippedBlocks;
    QList<MacroUse> _macroUses;
    QList<UndefinedMacroUse> _undefinedMacroUses;
    QByteArray _source;
    QDateTime _lastModified;
    unsigned _revision;
    unsigned _editorRevision;

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
    Document::Ptr operator[](const QString &fileName) const;

    const_iterator find(const QString &fileName) const;

    Snapshot simplified(Document::Ptr doc) const;

    QByteArray preprocessedCode(const QString &source,
                                const QString &fileName) const;

    Document::Ptr documentFromSource(const QByteArray &preprocessedCode,
                                     const QString &fileName) const;

    QSharedPointer<NamespaceBinding> globalNamespaceBinding(Document::Ptr doc) const;

private:
    void simplified_helper(Document::Ptr doc, Snapshot *snapshot) const;

private:
    _Base _documents;
};

} // end of namespace CPlusPlus

#endif // CPPDOCUMENT_H
