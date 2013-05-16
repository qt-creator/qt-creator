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

#include "CppDocument.h"
#include "FastPreprocessor.h"
#include "LookupContext.h"
#include "Overview.h"

#include <cplusplus/Bind.h>
#include <cplusplus/Control.h>
#include <cplusplus/TranslationUnit.h>
#include <cplusplus/DiagnosticClient.h>
#include <cplusplus/Literals.h>
#include <cplusplus/Symbols.h>
#include <cplusplus/Names.h>
#include <cplusplus/AST.h>
#include <cplusplus/ASTPatternBuilder.h>
#include <cplusplus/ASTMatcher.h>
#include <cplusplus/Scope.h>
#include <cplusplus/SymbolVisitor.h>
#include <cplusplus/NameVisitor.h>
#include <cplusplus/TypeVisitor.h>
#include <cplusplus/CoreTypes.h>

#include <QByteArray>
#include <QBitArray>
#include <QDir>
#include <QDebug>

/*!
    \namespace CPlusPlus
    The namespace for C++ related tools.
*/

using namespace CPlusPlus;

namespace {

class LastVisibleSymbolAt: protected SymbolVisitor
{
    Symbol *root;
    unsigned line;
    unsigned column;
    Symbol *symbol;

public:
    LastVisibleSymbolAt(Symbol *root)
        : root(root), line(0), column(0), symbol(0) {}

    Symbol *operator()(unsigned line, unsigned column)
    {
        this->line = line;
        this->column = column;
        this->symbol = 0;
        accept(root);
        if (! symbol)
            symbol = root;
        return symbol;
    }

protected:
    bool preVisit(Symbol *s)
    {
        if (s->asBlock()) {
            if (s->line() < line || (s->line() == line && s->column() <= column))
                return true;
            // skip blocks
        } if (s->line() < line || (s->line() == line && s->column() <= column)) {
            symbol = s;
            return true;
        }

        return false;
    }
};

class FindScopeAt: protected SymbolVisitor
{
    TranslationUnit *_unit;
    unsigned _line;
    unsigned _column;
    Scope *_scope;

public:
    /** line and column should be 1-based */
    FindScopeAt(TranslationUnit *unit, unsigned line, unsigned column)
        : _unit(unit), _line(line), _column(column), _scope(0) {}

    Scope *operator()(Symbol *symbol)
    {
        accept(symbol);
        return _scope;
    }

protected:
    bool process(Scope *symbol)
    {
        if (! _scope) {
            Scope *scope = symbol;

            for (unsigned i = 0; i < scope->memberCount(); ++i) {
                accept(scope->memberAt(i));

                if (_scope)
                    return false;
            }

            unsigned startLine, startColumn;
            _unit->getPosition(scope->startOffset(), &startLine, &startColumn);

            if (_line > startLine || (_line == startLine && _column >= startColumn)) {
                unsigned endLine, endColumn;
                _unit->getPosition(scope->endOffset(), &endLine, &endColumn);

                if (_line < endLine || (_line == endLine && _column < endColumn))
                    _scope = scope;
            }
        }

        return false;
    }

    using SymbolVisitor::visit;

    virtual bool preVisit(Symbol *)
    { return ! _scope; }

    virtual bool visit(UsingNamespaceDirective *) { return false; }
    virtual bool visit(UsingDeclaration *) { return false; }
    virtual bool visit(NamespaceAlias *) { return false; }
    virtual bool visit(Declaration *) { return false; }
    virtual bool visit(Argument *) { return false; }
    virtual bool visit(TypenameArgument *) { return false; }
    virtual bool visit(BaseClass *) { return false; }
    virtual bool visit(ForwardClassDeclaration *) { return false; }

    virtual bool visit(Enum *symbol)
    { return process(symbol); }

    virtual bool visit(Function *symbol)
    { return process(symbol); }

    virtual bool visit(Namespace *symbol)
    { return process(symbol); }

    virtual bool visit(Class *symbol)
    { return process(symbol); }

    virtual bool visit(Block *symbol)
    { return process(symbol); }

    virtual bool visit(Template *symbol)
    {
        if (symbol->declaration() && symbol->declaration()->isFunction())
            return process(symbol);
        else
            return true;
    }

    // Objective-C
    virtual bool visit(ObjCBaseClass *) { return false; }
    virtual bool visit(ObjCBaseProtocol *) { return false; }
    virtual bool visit(ObjCForwardClassDeclaration *) { return false; }
    virtual bool visit(ObjCForwardProtocolDeclaration *) { return false; }
    virtual bool visit(ObjCPropertyDeclaration *) { return false; }

    virtual bool visit(ObjCClass *symbol)
    { return process(symbol); }

    virtual bool visit(ObjCProtocol *symbol)
    { return process(symbol); }

    virtual bool visit(ObjCMethod *symbol)
    { return process(symbol); }
};


#define DO_NOT_DUMP_ALL_PARSER_ERRORS

class DocumentDiagnosticClient : public DiagnosticClient
{
    enum { MAX_MESSAGE_COUNT = 10 };

public:
    DocumentDiagnosticClient(Document *doc, QList<Document::DiagnosticMessage> *messages)
        : doc(doc),
          messages(messages),
          errorCount(0)
    { }

    virtual void report(int level,
                        const StringLiteral *fileId,
                        unsigned line, unsigned column,
                        const char *format, va_list ap)
    {
        if (level == Error) {
            ++errorCount;

#ifdef DO_NOT_DUMP_ALL_PARSER_ERRORS
            if (errorCount >= MAX_MESSAGE_COUNT)
                return; // ignore the error
#endif // DO_NOT_DUMP_ALL_PARSER_ERRORS
        }

        const QString fileName = QString::fromUtf8(fileId->chars(), fileId->size());

        if (fileName != doc->fileName())
            return;

        QString message;
        message.vsprintf(format, ap);

#ifndef DO_NOT_DUMP_ALL_PARSER_ERRORS
        {
            const char *levelStr = "Unknown level";
            if (level == Document::DiagnosticMessage::Warning) levelStr = "Warning";
            if (level == Document::DiagnosticMessage::Error) levelStr = "Error";
            if (level == Document::DiagnosticMessage::Fatal) levelStr = "Fatal";
            qDebug("%s:%u:%u: %s: %s", fileId->chars(), line, column, levelStr, message.toUtf8().constData());
        }
#endif // DO_NOT_DUMP_ALL_PARSER_ERRORS

        Document::DiagnosticMessage m(convertLevel(level), doc->fileName(),
                                      line, column, message);
        messages->append(m);
    }

    static int convertLevel(int level) {
        switch (level) {
            case Warning: return Document::DiagnosticMessage::Warning;
            case Error:   return Document::DiagnosticMessage::Error;
            case Fatal:   return Document::DiagnosticMessage::Fatal;
            default:      return Document::DiagnosticMessage::Error;
        }
    }

private:
    Document *doc;
    QList<Document::DiagnosticMessage> *messages;
    int errorCount;
};

} // anonymous namespace


Document::Document(const QString &fileName)
    : _fileName(QDir::cleanPath(fileName)),
      _globalNamespace(0),
      _revision(0),
      _editorRevision(0),
      _checkMode(0)
{
    _control = new Control();

    _control->setDiagnosticClient(new DocumentDiagnosticClient(this, &_diagnosticMessages));

    const QByteArray localFileName = fileName.toUtf8();
    const StringLiteral *fileId = _control->stringLiteral(localFileName.constData(),
                                                                      localFileName.size());
    _translationUnit = new TranslationUnit(_control, fileId);
    _translationUnit->setQtMocRunEnabled(true);
    _translationUnit->setCxxOxEnabled(true);
    _translationUnit->setObjCEnabled(true);
    (void) _control->switchTranslationUnit(_translationUnit);
}

Document::~Document()
{
    delete _translationUnit;
    delete _control->diagnosticClient();
    delete _control;
}

Control *Document::control() const
{
    return _control;
}

unsigned Document::revision() const
{
    return _revision;
}

void Document::setRevision(unsigned revision)
{
    _revision = revision;
}

unsigned Document::editorRevision() const
{
    return _editorRevision;
}

void Document::setEditorRevision(unsigned editorRevision)
{
    _editorRevision = editorRevision;
}

QDateTime Document::lastModified() const
{
    return _lastModified;
}

void Document::setLastModified(const QDateTime &lastModified)
{
    _lastModified = lastModified;
}

QString Document::fileName() const
{
    return _fileName;
}

QStringList Document::includedFiles() const
{
    QStringList files;
    foreach (const Include &i, _includes)
        files.append(i.fileName());
    files.removeDuplicates();
    return files;
}

// This assumes to be called with a QDir::cleanPath cleaned fileName.
void Document::addIncludeFile(const QString &fileName, unsigned line)
{
    _includes.append(Include(fileName, line));
}

void Document::appendMacro(const Macro &macro)
{
    _definedMacros.append(macro);
}

void Document::addMacroUse(const Macro &macro, unsigned offset, unsigned length,
                           unsigned beginLine,
                           const QVector<MacroArgumentReference> &actuals)
{
    MacroUse use(macro, offset, offset + length, beginLine);

    foreach (const MacroArgumentReference &actual, actuals) {
        const Block arg(actual.position(), actual.position() + actual.length());

        use.addArgument(arg);
    }

    _macroUses.append(use);
}

void Document::addUndefinedMacroUse(const QByteArray &name, unsigned offset)
{
    QByteArray copy(name.data(), name.size());
    UndefinedMacroUse use(copy, offset);
    _undefinedMacroUses.append(use);
}

/*!
    \class Document::MacroUse
    \brief Represents the usage of a macro in a \l {Document}.
    \sa Document::UndefinedMacroUse
*/

/*!
    \class Document::UndefinedMacroUse
    \brief Represents a macro that was looked up, but not found.

    Holds data about the reference to a macro in an \tt{#ifdef} or \tt{#ifndef}
    or argument to the \tt{defined} operator inside an \tt{#if} or \tt{#elif} that does
    not exist.

    \sa Document::undefinedMacroUses(), Document::MacroUse, Macro
*/

/*!
    \fn QByteArray Document::UndefinedMacroUse::name() const

    Returns the name of the macro that was not found.
*/

/*!
    \fn QList<UndefinedMacroUse> Document::undefinedMacroUses() const

    Returns a list of referenced but undefined macros.

    \sa Document::macroUses(), Document::definedMacros(), Macro
*/

/*!
    \fn QList<MacroUse> Document::macroUses() const

    Returns a list of macros used.

    \sa Document::undefinedMacroUses(), Document::definedMacros(), Macro
*/

/*!
    \fn QList<Macro> Document::definedMacros() const

    Returns the list of macros defined.

    \sa Document::macroUses(), Document::undefinedMacroUses()
*/

TranslationUnit *Document::translationUnit() const
{
    return _translationUnit;
}

bool Document::skipFunctionBody() const
{
    return _translationUnit->skipFunctionBody();
}

void Document::setSkipFunctionBody(bool skipFunctionBody)
{
    _translationUnit->setSkipFunctionBody(skipFunctionBody);
}

unsigned Document::globalSymbolCount() const
{
    if (! _globalNamespace)
        return 0;

    return _globalNamespace->memberCount();
}

Symbol *Document::globalSymbolAt(unsigned index) const
{
    return _globalNamespace->memberAt(index);
}

Namespace *Document::globalNamespace() const
{
    return _globalNamespace;
}

void Document::setGlobalNamespace(Namespace *globalNamespace)
{
    _globalNamespace = globalNamespace;
}

Scope *Document::scopeAt(unsigned line, unsigned column)
{
    FindScopeAt findScopeAt(_translationUnit, line, column);
    if (Scope *scope = findScopeAt(_globalNamespace))
        return scope;
    return globalNamespace();
}

Symbol *Document::lastVisibleSymbolAt(unsigned line, unsigned column) const
{
    LastVisibleSymbolAt lastVisibleSymbolAt(globalNamespace());
    return lastVisibleSymbolAt(line, column);
}

const Macro *Document::findMacroDefinitionAt(unsigned line) const
{
    foreach (const Macro &macro, _definedMacros) {
        if (macro.line() == line)
            return &macro;
    }
    return 0;
}

const Document::MacroUse *Document::findMacroUseAt(unsigned offset) const
{
    foreach (const Document::MacroUse &use, _macroUses) {
        if (use.contains(offset) && (offset < use.begin() + use.macro().name().length()))
            return &use;
    }
    return 0;
}

const Document::UndefinedMacroUse *Document::findUndefinedMacroUseAt(unsigned offset) const
{
    foreach (const Document::UndefinedMacroUse &use, _undefinedMacroUses) {
        if (use.contains(offset) && (offset < use.begin() + use.name().length()))
            return &use;
    }
    return 0;
}

Document::Ptr Document::create(const QString &fileName)
{
    Document::Ptr doc(new Document(fileName));
    return doc;
}

QByteArray Document::utf8Source() const
{ return _source; }

void Document::setUtf8Source(const QByteArray &source)
{
    _source = source;
    _translationUnit->setSource(_source.constBegin(), _source.size());
}

void Document::startSkippingBlocks(unsigned start)
{
    _skippedBlocks.append(Block(start, 0));
}

void Document::stopSkippingBlocks(unsigned stop)
{
    if (_skippedBlocks.isEmpty())
        return;

    unsigned start = _skippedBlocks.back().begin();
    if (start > stop)
        _skippedBlocks.removeLast(); // Ignore this block, it's invalid.
    else
        _skippedBlocks.back() = Block(start, stop);
}

bool Document::isTokenized() const
{
    return _translationUnit->isTokenized();
}

void Document::tokenize()
{
    _translationUnit->tokenize();
}

bool Document::isParsed() const
{
    return _translationUnit->isParsed();
}

bool Document::parse(ParseMode mode)
{
    TranslationUnit::ParseMode m = TranslationUnit::ParseTranlationUnit;
    switch (mode) {
    case ParseTranlationUnit:
        m = TranslationUnit::ParseTranlationUnit;
        break;

    case ParseDeclaration:
        m = TranslationUnit::ParseDeclaration;
        break;

    case ParseExpression:
        m = TranslationUnit::ParseExpression;
        break;

    case ParseDeclarator:
        m = TranslationUnit::ParseDeclarator;
        break;

    case ParseStatement:
        m = TranslationUnit::ParseStatement;
        break;

    default:
        break;
    }

    return _translationUnit->parse(m);
}

void Document::check(CheckMode mode)
{
    Q_ASSERT(!_globalNamespace);

    _checkMode = mode;

    if (! isParsed())
        parse();

    _globalNamespace = _control->newNamespace(0);
    Bind semantic(_translationUnit);
    if (mode == FastCheck)
        semantic.setSkipFunctionBodies(true);

    if (! _translationUnit->ast())
        return; // nothing to do.

    if (TranslationUnitAST *ast = _translationUnit->ast()->asTranslationUnit())
        semantic(ast, _globalNamespace);
    else if (StatementAST *ast = _translationUnit->ast()->asStatement())
        semantic(ast, _globalNamespace);
    else if (ExpressionAST *ast = _translationUnit->ast()->asExpression())
        semantic(ast, _globalNamespace);
    else if (DeclarationAST *ast = translationUnit()->ast()->asDeclaration())
        semantic(ast, _globalNamespace);
}

void Document::keepSourceAndAST()
{
    _keepSourceAndASTCount.ref();
}

void Document::releaseSourceAndAST()
{
    if (!_keepSourceAndASTCount.deref()) {
        _source.clear();
        _translationUnit->release();
        _control->squeeze();
    }
}

bool Document::DiagnosticMessage::operator==(const Document::DiagnosticMessage &other) const
{
    return
            _line == other._line &&
            _column == other._column &&
            _length == other._length &&
            _level == other._level &&
            _fileName == other._fileName &&
            _text == other._text;
}

bool Document::DiagnosticMessage::operator!=(const Document::DiagnosticMessage &other) const
{
    return !operator==(other);
}

Snapshot::Snapshot()
{
}

Snapshot::~Snapshot()
{
}

int Snapshot::size() const
{
    return _documents.size();
}

bool Snapshot::isEmpty() const
{
    return _documents.isEmpty();
}

Snapshot::const_iterator Snapshot::find(const QString &fileName) const
{
    return _documents.find(fileName);
}

void Snapshot::remove(const QString &fileName)
{
    _documents.remove(fileName);
}

bool Snapshot::contains(const QString &fileName) const
{
    return _documents.contains(fileName);
}

void Snapshot::insert(Document::Ptr doc)
{
    if (doc)
        _documents.insert(doc->fileName(), doc);
}

Document::Ptr Snapshot::preprocessedDocument(const QString &source, const QString &fileName) const
{
    Document::Ptr newDoc = Document::create(fileName);
    if (Document::Ptr thisDocument = document(fileName)) {
        newDoc->_revision = thisDocument->_revision;
        newDoc->_editorRevision = thisDocument->_editorRevision;
        newDoc->_lastModified = thisDocument->_lastModified;
        newDoc->_includes = thisDocument->_includes;
    }

    FastPreprocessor pp(*this);
    const QByteArray preprocessedCode = pp.run(newDoc, source);
    newDoc->setUtf8Source(preprocessedCode);
    return newDoc;
}

Document::Ptr Snapshot::documentFromSource(const QByteArray &preprocessedCode,
                                           const QString &fileName) const
{
    Document::Ptr newDoc = Document::create(fileName);

    if (Document::Ptr thisDocument = document(fileName)) {
        newDoc->_revision = thisDocument->_revision;
        newDoc->_editorRevision = thisDocument->_editorRevision;
        newDoc->_lastModified = thisDocument->_lastModified;
        newDoc->_includes = thisDocument->_includes;
        newDoc->_definedMacros = thisDocument->_definedMacros;
        newDoc->_macroUses = thisDocument->_macroUses;
    }

    newDoc->setUtf8Source(preprocessedCode);
    return newDoc;
}

QSet<QString> Snapshot::allIncludesForDocument(const QString &fileName) const
{
    QSet<QString> result;
    allIncludesForDocument_helper(fileName, result);
    return result;
}

void Snapshot::allIncludesForDocument_helper(const QString &fileName, QSet<QString> &result) const
{
    if (Document::Ptr doc = document(fileName)) {
        foreach (const QString &inc, doc->includedFiles()) {
            if (!result.contains(inc)) {
                result.insert(inc);
                allIncludesForDocument_helper(inc, result);
            }
        }
    }
}

Document::Ptr Snapshot::document(const QString &fileName) const
{
    return _documents.value(fileName);
}

Snapshot Snapshot::simplified(Document::Ptr doc) const
{
    Snapshot snapshot;

    if (doc) {
        snapshot.insert(doc);
        foreach (const QString &fileName, allIncludesForDocument(doc->fileName()))
            if (Document::Ptr inc = document(fileName))
                snapshot.insert(inc);
    }

    return snapshot;
}
