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

#include "CppDocument.h"
#include "CppBindings.h"
#include "FastPreprocessor.h"

#include <Control.h>
#include <TranslationUnit.h>
#include <DiagnosticClient.h>
#include <Semantic.h>
#include <Literals.h>
#include <Symbols.h>
#include <AST.h>
#include <Scope.h>

#include <QtCore/QByteArray>
#include <QtCore/QBitArray>
#include <QtCore/QDir>
#include <QtCore/QtDebug>

/*!
    \namespace CPlusPlus
    The namespace for C++ related tools.
*/

using namespace CPlusPlus;

namespace {

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

            if (errorCount >= MAX_MESSAGE_COUNT)
                return; // ignore the error
        }

        const QString fileName = QString::fromUtf8(fileId->chars(), fileId->size());

        if (fileName != doc->fileName())
            return;

        QString message;
        message.vsprintf(format, ap);

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
      _editorRevision(0)
{
    _control = new Control();

    _control->setDiagnosticClient(new DocumentDiagnosticClient(this, &_diagnosticMessages));

    const QByteArray localFileName = fileName.toUtf8();
    const StringLiteral *fileId = _control->findOrInsertStringLiteral(localFileName.constData(),
                                                                      localFileName.size());
    _translationUnit = new TranslationUnit(_control, fileId);
    _translationUnit->setQtMocRunEnabled(true);
    _translationUnit->setCxxOxEnabled(false);
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

void Document::addIncludeFile(const QString &fileName, unsigned line)
{
    _includes.append(Include(QDir::cleanPath(fileName), line));
}

void Document::appendMacro(const Macro &macro)
{
    _definedMacros.append(macro);
}

void Document::addMacroUse(const Macro &macro, unsigned offset, unsigned length,
                           unsigned beginLine,
                           const QVector<MacroArgumentReference> &actuals, bool inCondition)
{
    MacroUse use(macro, offset, offset + length, beginLine);
    use.setInCondition(inCondition);

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

Scope *Document::globalSymbols() const
{
    if (! _globalNamespace)
        return 0;

    return _globalNamespace->members();
}

Namespace *Document::globalNamespace() const
{
    return _globalNamespace;
}

void Document::setGlobalNamespace(Namespace *globalNamespace)
{
    _globalNamespace = globalNamespace;
}

Symbol *Document::findSymbolAt(unsigned line, unsigned column) const
{
    return findSymbolAt(line, column, globalSymbols());
}

Symbol *Document::findSymbolAt(unsigned line, unsigned column, Scope *scope) const
{
    Symbol *previousSymbol = 0;

    for (unsigned i = 0; i < scope->symbolCount(); ++i) {
        Symbol *symbol = scope->symbolAt(i);
        if (symbol->line() > line)
            break;

        previousSymbol = symbol;
    }

    if (previousSymbol) {
        if (ScopedSymbol *scoped = previousSymbol->asScopedSymbol()) {
            if (Symbol *member = findSymbolAt(line, column, scoped->members()))
                return member;
        }
    }

    return previousSymbol;
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
        if (use.contains(offset))
            return &use;
    }
    return 0;
}

const Document::UndefinedMacroUse *Document::findUndefinedMacroUseAt(unsigned offset) const
{
    foreach (const Document::UndefinedMacroUse &use, _undefinedMacroUses) {
        if (use.contains(offset))
            return &use;
    }
    return 0;
}

Document::Ptr Document::create(const QString &fileName)
{
    Document::Ptr doc(new Document(fileName));
    return doc;
}

QByteArray Document::source() const
{ return _source; }

void Document::setSource(const QByteArray &source)
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

    if (! isParsed())
        parse();

    Semantic semantic(_translationUnit);
    if (mode == FastCheck)
        semantic.setSkipFunctionBodies(true);

    _globalNamespace = _control->newNamespace(0);
    Scope *globals = _globalNamespace->members();
    if (! _translationUnit->ast())
        return; // nothing to do.

    if (TranslationUnitAST *ast = _translationUnit->ast()->asTranslationUnit()) {
        for (DeclarationListAST *decl = ast->declaration_list; decl; decl = decl->next) {
            semantic.check(decl->value, globals);
        }
    } else if (ExpressionAST *ast = _translationUnit->ast()->asExpression()) {
        semantic.check(ast, globals);
    }
}

void Document::releaseSource()
{
    _source.clear();
}

void Document::releaseTranslationUnit()
{
    _translationUnit->release();
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

Document::Ptr Snapshot::operator[](const QString &fileName) const
{
    return _documents.value(fileName, Document::Ptr());
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

QByteArray Snapshot::preprocessedCode(const QString &source, const QString &fileName) const
{
    FastPreprocessor pp(*this);
    return pp.run(fileName, source);
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

    newDoc->setSource(preprocessedCode);
    return newDoc;
}

QSharedPointer<NamespaceBinding> Snapshot::globalNamespaceBinding(Document::Ptr doc) const
{
    return CPlusPlus::bind(doc, *this);
}

Document::Ptr Snapshot::document(const QString &fileName) const
{
    return _documents.value(fileName);
}

Snapshot Snapshot::simplified(Document::Ptr doc) const
{
    Snapshot snapshot;
    simplified_helper(doc, &snapshot);
    return snapshot;
}

void Snapshot::simplified_helper(Document::Ptr doc, Snapshot *snapshot) const
{
    if (! doc)
        return;

    if (! snapshot->contains(doc->fileName())) {
        snapshot->insert(doc);

        foreach (const Document::Include &incl, doc->includes()) {
            Document::Ptr includedDoc = document(incl.fileName());
            simplified_helper(includedDoc, snapshot);
        }
    }
}
