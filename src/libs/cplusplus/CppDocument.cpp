/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "CppDocument.h"
#include "FastPreprocessor.h"
#include "LookupContext.h"
#include "Overview.h"
#include "Bind.h"

#include <Control.h>
#include <TranslationUnit.h>
#include <DiagnosticClient.h>
#include <Literals.h>
#include <Symbols.h>
#include <Names.h>
#include <AST.h>
#include <ASTPatternBuilder.h>
#include <ASTMatcher.h>
#include <Scope.h>
#include <SymbolVisitor.h>
#include <NameVisitor.h>
#include <TypeVisitor.h>
#include <CoreTypes.h>

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
            if (s->line() < line || (s->line() == line && s->column() <= column)) {
                return true;
            }
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

                if (_line < endLine || (_line == endLine && _column <= endColumn))
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

    _globalNamespace = _control->newNamespace(0);
    Bind semantic(_translationUnit);
    if (mode == FastCheck)
        semantic.setSkipFunctionBodies(true);

    if (! _translationUnit->ast())
        return; // nothing to do.

    if (TranslationUnitAST *ast = _translationUnit->ast()->asTranslationUnit()) {
        semantic(ast, _globalNamespace);
    } else if (ExpressionAST *ast = _translationUnit->ast()->asExpression()) {
        semantic(ast, _globalNamespace);
    }
}

class FindExposedQmlTypes : protected ASTVisitor
{
    Document *_doc;
    QList<Document::ExportedQmlType> _exportedTypes;
    CompoundStatementAST *_compound;
    ASTMatcher _matcher;
    ASTPatternBuilder _builder;
    Overview _overview;

public:
    FindExposedQmlTypes(Document *doc)
        : ASTVisitor(doc->translationUnit())
        , _doc(doc)
        , _compound(0)
    {}

    QList<Document::ExportedQmlType> operator()()
    {
        _exportedTypes.clear();
        accept(translationUnit()->ast());
        return _exportedTypes;
    }

protected:
    virtual bool visit(CompoundStatementAST *ast)
    {
        CompoundStatementAST *old = _compound;
        _compound = ast;
        accept(ast->statement_list);
        _compound = old;
        return false;
    }

    virtual bool visit(CallAST *ast)
    {
        IdExpressionAST *idExp = ast->base_expression->asIdExpression();
        if (!idExp || !idExp->name)
            return false;
        TemplateIdAST *templateId = idExp->name->asTemplateId();
        if (!templateId || !templateId->identifier_token)
            return false;

        // check the name
        const Identifier *templateIdentifier = translationUnit()->identifier(templateId->identifier_token);
        if (!templateIdentifier)
            return false;
        const QString callName = QString::fromUtf8(templateIdentifier->chars());
        if (callName != QLatin1String("qmlRegisterType"))
            return false;

        // must have a single typeid template argument
        if (!templateId->template_argument_list || !templateId->template_argument_list->value
                || templateId->template_argument_list->next)
            return false;
        TypeIdAST *typeId = templateId->template_argument_list->value->asTypeId();
        if (!typeId)
            return false;

        // must have four arguments
        if (!ast->expression_list
                || !ast->expression_list->value || !ast->expression_list->next
                || !ast->expression_list->next->value || !ast->expression_list->next->next
                || !ast->expression_list->next->next->value || !ast->expression_list->next->next->next
                || !ast->expression_list->next->next->next->value
                || ast->expression_list->next->next->next->next)
            return false;

        // last argument must be a string literal
        const StringLiteral *nameLit = 0;
        if (StringLiteralAST *nameAst = ast->expression_list->next->next->next->value->asStringLiteral())
            nameLit = translationUnit()->stringLiteral(nameAst->literal_token);
        if (!nameLit) {
            // disable this warning for now, we don't want to encourage using string literals if they don't mean to
            // in the future, we will also accept annotations for the qmlRegisterType arguments in comments
//            translationUnit()->warning(ast->expression_list->next->next->next->value->firstToken(),
//                                       "The type will only be available in Qt Creator's QML editors when the type name is a string literal");
            return false;
        }

        // if the first argument is a string literal, things are easy
        QString packageName;
        if (StringLiteralAST *packageAst = ast->expression_list->value->asStringLiteral()) {
            const StringLiteral *packageLit = translationUnit()->stringLiteral(packageAst->literal_token);
            packageName = QString::fromUtf8(packageLit->chars(), packageLit->size());
        }
        // as a special case, allow an identifier package argument if there's a
        // Q_ASSERT(QLatin1String(uri) == QLatin1String("actual uri"));
        // in the enclosing compound statement
        IdExpressionAST *uriName = ast->expression_list->value->asIdExpression();
        if (packageName.isEmpty() && uriName && _compound) {
            for (StatementListAST *it = _compound->statement_list; it; it = it->next) {
                StatementAST *stmt = it->value;

                packageName = nameOfUriAssert(stmt, uriName);
                if (!packageName.isEmpty())
                    break;
            }
        }

        // second and third argument must be integer literals
        const NumericLiteral *majorLit = 0;
        const NumericLiteral *minorLit = 0;
        if (NumericLiteralAST *majorAst = ast->expression_list->next->value->asNumericLiteral())
            majorLit = translationUnit()->numericLiteral(majorAst->literal_token);
        if (NumericLiteralAST *minorAst = ast->expression_list->next->next->value->asNumericLiteral())
            minorLit = translationUnit()->numericLiteral(minorAst->literal_token);

        // build the descriptor
        Document::ExportedQmlType exportedType;
        exportedType.typeName = QString::fromUtf8(nameLit->chars(), nameLit->size());
        if (!packageName.isEmpty() && majorLit && minorLit && majorLit->isInt() && minorLit->isInt()) {
            exportedType.packageName = packageName;
            exportedType.majorVersion = QString::fromUtf8(majorLit->chars(), majorLit->size()).toInt();
            exportedType.minorVersion = QString::fromUtf8(minorLit->chars(), minorLit->size()).toInt();
        } else {
            // disable this warning, see above for details
//            translationUnit()->warning(ast->base_expression->firstToken(),
//                                       "The module will not be available in Qt Creator's QML editors because the uri and version numbers\n"
//                                       "cannot be determined by static analysis. The type will still be available globally.");
            exportedType.packageName = QLatin1String("<default>");
        }

        // we want to do lookup later, so also store the surrounding scope
        unsigned line, column;
        translationUnit()->getTokenStartPosition(ast->firstToken(), &line, &column);
        exportedType.scope = _doc->scopeAt(line, column);

        // and the expression
        const Token begin = translationUnit()->tokenAt(typeId->firstToken());
        const Token last = translationUnit()->tokenAt(typeId->lastToken() - 1);
        exportedType.typeExpression = _doc->source().mid(begin.begin(), last.end() - begin.begin());

        _exportedTypes += exportedType;

        return false;
    }

private:
    QString stringOf(AST *ast)
    {
        const Token begin = translationUnit()->tokenAt(ast->firstToken());
        const Token last = translationUnit()->tokenAt(ast->lastToken() - 1);
        return _doc->source().mid(begin.begin(), last.end() - begin.begin());
    }

    ExpressionAST *skipStringCall(ExpressionAST *exp)
    {
        if (!exp)
            return 0;

        IdExpressionAST *callName = _builder.IdExpression();
        CallAST *call = _builder.Call(callName);
        if (!exp->match(call, &_matcher))
            return exp;

        const QString name = stringOf(callName);
        if (name != QLatin1String("QLatin1String")
                && name != QLatin1String("QString"))
            return exp;

        if (!call->expression_list || call->expression_list->next)
            return exp;

        return call->expression_list->value;
    }

    QString nameOfUriAssert(StatementAST *stmt, IdExpressionAST *uriName)
    {
        QString null;

        IdExpressionAST *outerCallName = _builder.IdExpression();
        BinaryExpressionAST *binary = _builder.BinaryExpression();
        // assert(... == ...);
        ExpressionStatementAST *pattern = _builder.ExpressionStatement(
                    _builder.Call(outerCallName, _builder.ExpressionList(
                                     binary)));

        if (!stmt->match(pattern, &_matcher)) {
            outerCallName = _builder.IdExpression();
            binary = _builder.BinaryExpression();
            // the expansion of Q_ASSERT(...),
            // ((!(... == ...)) ? qt_assert(...) : ...);
            pattern = _builder.ExpressionStatement(
                        _builder.NestedExpression(
                            _builder.ConditionalExpression(
                                _builder.NestedExpression(
                                    _builder.UnaryExpression(
                                        _builder.NestedExpression(
                                            binary))),
                                _builder.Call(outerCallName))));

            if (!stmt->match(pattern, &_matcher))
                return null;
        }

        const QString outerCall = stringOf(outerCallName);
        if (outerCall != QLatin1String("qt_assert")
                && outerCall != QLatin1String("assert")
                && outerCall != QLatin1String("Q_ASSERT"))
            return null;

        if (translationUnit()->tokenAt(binary->binary_op_token).kind() != T_EQUAL_EQUAL)
            return null;

        ExpressionAST *lhsExp = skipStringCall(binary->left_expression);
        ExpressionAST *rhsExp = skipStringCall(binary->right_expression);
        if (!lhsExp || !rhsExp)
            return null;

        StringLiteralAST *uriString = lhsExp->asStringLiteral();
        IdExpressionAST *uriArgName = lhsExp->asIdExpression();
        if (!uriString)
            uriString = rhsExp->asStringLiteral();
        if (!uriArgName)
            uriArgName = rhsExp->asIdExpression();
        if (!uriString || !uriArgName)
            return null;

        if (stringOf(uriArgName) != stringOf(uriName))
            return null;

        const StringLiteral *packageLit = translationUnit()->stringLiteral(uriString->literal_token);
        return QString::fromUtf8(packageLit->chars(), packageLit->size());
    }
};

void Document::findExposedQmlTypes()
{
    if (! _translationUnit->ast())
        return;

    QByteArray qmlRegisterTypeToken("qmlRegisterType");
    if (_translationUnit->control()->findIdentifier(
                qmlRegisterTypeToken.constData(), qmlRegisterTypeToken.size())) {
        FindExposedQmlTypes finder(this);
        _exportedQmlTypes = finder();
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

namespace {
class FindMatchingDefinition: public SymbolVisitor
{
    Symbol *_declaration;
    const OperatorNameId *_oper;
    QList<Function *> _result;

public:
    FindMatchingDefinition(Symbol *declaration)
        : _declaration(declaration)
        , _oper(0)
    {
        if (_declaration->name())
            _oper = _declaration->name()->asOperatorNameId();
    }

    QList<Function *> result() const { return _result; }

    using SymbolVisitor::visit;

    virtual bool visit(Function *fun)
    {
        if (_oper) {
            if (const Name *name = fun->unqualifiedName()) {
                    if (_oper->isEqualTo(name))
                        _result.append(fun);
            }
        } else if (const Identifier *id = _declaration->identifier()) {
            if (id->isEqualTo(fun->identifier()))
                _result.append(fun);
        }

        return false;
    }

    virtual bool visit(Block *)
    {
        return false;
    }
};
} // end of anonymous namespace

Symbol *Snapshot::findMatchingDefinition(Symbol *declaration) const
{
    if (!declaration)
        return 0;

    Document::Ptr thisDocument = document(QString::fromUtf8(declaration->fileName(), declaration->fileNameLength()));
    if (! thisDocument) {
        qWarning() << "undefined document:" << declaration->fileName();
        return 0;
    }

    Function *declarationTy = declaration->type()->asFunctionType();
    if (! declarationTy) {
        qWarning() << "not a function:" << declaration->fileName() << declaration->line() << declaration->column();
        return 0;
    }

    foreach (Document::Ptr doc, *this) {
        const Identifier *id = declaration->identifier();
        if (id && ! doc->control()->findIdentifier(id->chars(),
                                                   id->size()))
            continue;
        if (!id) {
            if (!declaration->name())
                continue;
            const OperatorNameId *oper = declaration->name()->asOperatorNameId();
            if (!oper)
                continue;
            if (!doc->control()->findOperatorNameId(oper->kind()))
                continue;
        }

        FindMatchingDefinition candidates(declaration);
        candidates.accept(doc->globalNamespace());

        const QList<Function *> result = candidates.result();
        if (! result.isEmpty()) {
            LookupContext context(doc, *this);

            QList<Function *> viableFunctions;

            ClassOrNamespace *enclosingType = context.lookupType(declaration);
            if (! enclosingType)
                continue; // nothing to do

            foreach (Function *fun, result) {
                const QList<LookupItem> declarations = context.lookup(fun->name(), fun->enclosingScope());
                if (declarations.isEmpty())
                    continue;

                const LookupItem best = declarations.first();
                if (enclosingType == context.lookupType(best.declaration()))
                    viableFunctions.append(fun);
            }

            if (viableFunctions.isEmpty())
                continue;

            else if (viableFunctions.length() == 1)
                return viableFunctions.first();

            Function *best = 0;

            foreach (Function *fun, viableFunctions) {
                if (! (fun->unqualifiedName() && fun->unqualifiedName()->isEqualTo(declaration->unqualifiedName())))
                    continue;
                else if (fun->argumentCount() == declarationTy->argumentCount()) {
                    if (! best)
                        best = fun;

                    unsigned argc = 0;
                    for (; argc < declarationTy->argumentCount(); ++argc) {
                        Symbol *arg = fun->argumentAt(argc);
                        Symbol *otherArg = declarationTy->argumentAt(argc);
                        if (! arg->type().isEqualTo(otherArg->type()))
                            break;
                    }

                    if (argc == declarationTy->argumentCount())
                        best = fun;
                }
            }

            if (! best)
                best = viableFunctions.first();

            return best;
        }
    }

    return 0;
}

Class *Snapshot::findMatchingClassDeclaration(Symbol *declaration) const
{
    if (! declaration->identifier())
        return 0;

    foreach (Document::Ptr doc, *this) {
        if (! doc->control()->findIdentifier(declaration->identifier()->chars(),
                                             declaration->identifier()->size()))
            continue;

        LookupContext context(doc, *this);

        ClassOrNamespace *type = context.lookupType(declaration);
        if (!type)
            continue;

        foreach (Symbol *s, type->symbols()) {
            if (Class *c = s->asClass())
                return c;
        }
    }

    return 0;
}
