/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "CppDocument.h"

#include <Control.h>
#include <TranslationUnit.h>
#include <DiagnosticClient.h>
#include <Semantic.h>
#include <Literals.h>
#include <Symbols.h>
#include <AST.h>
#include <Scope.h>

#include <QByteArray>
#include <QFile>
#include <QtDebug>

using namespace CPlusPlus;

namespace {

class DocumentDiagnosticClient : public DiagnosticClient
{
    enum { MAX_MESSAGE_COUNT = 10 };

public:
    DocumentDiagnosticClient(Document *doc, QList<Document::DiagnosticMessage> *messages)
        : doc(doc),
          messages(messages)
    { }

    virtual void report(int level,
                        StringLiteral *fileId,
                        unsigned line, unsigned column,
                        const char *format, va_list ap)
    {
        if (messages->count() == MAX_MESSAGE_COUNT)
            return;

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

    Document *doc;
    QList<Document::DiagnosticMessage> *messages;
};

} // anonymous namespace

Document::Document(const QString &fileName)
    : _fileName(fileName),
      _globalNamespace(0)
{
    _control = new Control();

    _control->setDiagnosticClient(new DocumentDiagnosticClient(this, &_diagnosticMessages));

    const QByteArray localFileName = fileName.toUtf8();
    StringLiteral *fileId = _control->findOrInsertFileName(localFileName.constData(),
                                                           localFileName.size());
    _translationUnit = new TranslationUnit(_control, fileId);
    _translationUnit->setQtMocRunEnabled(true);
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
    _includes.append(Include(fileName, line));
}

void Document::appendMacro(const Macro &macro)
{
    _definedMacros.append(macro);
}

void Document::addMacroUse(const Macro &macro, unsigned offset, unsigned length)
{
    _macroUses.append(MacroUse(macro, offset, offset + length));
}

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

Document::Ptr Document::create(const QString &fileName)
{
    Document::Ptr doc(new Document(fileName));
    return doc;
}

void Document::setSource(const QByteArray &source)
{
    _translationUnit->setSource(source.constBegin(), source.size());
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

    case ParseStatement:
        m = TranslationUnit::ParseStatement;
        break;

    default:
        break;
    }

    return _translationUnit->parse(m);
}

void Document::check()
{
    Q_ASSERT(!_globalNamespace);

    Semantic semantic(_control);

    _globalNamespace = _control->newNamespace(0);
    Scope *globals = _globalNamespace->members();
    if (! _translationUnit->ast())
        return; // nothing to do.

    if (TranslationUnitAST *ast = _translationUnit->ast()->asTranslationUnit()) {
        for (DeclarationAST *decl = ast->declarations; decl; decl = decl->next) {
            semantic.check(decl, globals);
        }
    }
}

void Document::releaseTranslationUnit()
{
    _translationUnit->release();
}
