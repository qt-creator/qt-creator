/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#ifndef CPPDOCUMENT_H
#define CPPDOCUMENT_H

#include <CPlusPlusForwardDeclarations.h>

#include "pp-macro.h"

#include <QByteArray>
#include <QList>
#include <QSet>
#include <QSharedPointer>
#include <QString>
#include <QStringList>

namespace CPlusPlus {

class Macro;

class CPLUSPLUS_EXPORT Document
{
    Document(const Document &other);
    void operator =(const Document &other);

    Document(const QString &fileName);

public:
    typedef QSharedPointer<Document> Ptr;

public:
    ~Document();

    QString fileName() const;

    QStringList includedFiles() const;
    void addIncludeFile(const QString &fileName, unsigned line);

    void appendMacro(const Macro &macro);
    void addMacroUse(const Macro &macro, unsigned offset, unsigned length);

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

    void setSource(const QByteArray &source);
    void startSkippingBlocks(unsigned offset);
    void stopSkippingBlocks(unsigned offset);

    enum ParseMode { // ### keep in sync with CPlusPlus::TranslationUnit
        ParseTranlationUnit,
        ParseDeclaration,
        ParseExpression,
        ParseStatement
    };

    bool parse(ParseMode mode = ParseTranlationUnit);
    void check();
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

        int line() const
        { return _line; }

        int column() const
        { return _column; }

        QString text() const
        { return _text; }

    private:
        int _level;
        QString _fileName;
        int _line;
        int _column;
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
    };

    class MacroUse: public Block {
        Macro _macro;

    public:
        inline MacroUse(const Macro &macro,
                        unsigned begin = 0,
                        unsigned end = 0)
            : Block(begin, end),
              _macro(macro)
        { }

        const Macro &macro() const
        { return _macro; }
    };

    QList<Include> includes() const
    { return _includes; }

    QList<Block> skippedBlocks() const
    { return _skippedBlocks; }

    QList<MacroUse> macroUses() const
    { return _macroUses; }

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
};

} // end of namespace CPlusPlus

#endif // CPPDOCUMENT_H
