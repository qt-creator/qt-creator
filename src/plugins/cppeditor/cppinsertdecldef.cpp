/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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

#include "cppinsertdecldef.h"
#include "cppquickfixassistant.h"

#include <CPlusPlus.h>
#include <cplusplus/ASTPath.h>
#include <cplusplus/CppRewriter.h>
#include <cplusplus/LookupContext.h>
#include <cplusplus/Overview.h>
#include <cplusplus/ASTVisitor.h>
#include <cpptools/insertionpointlocator.h>
#include <cpptools/cpprefactoringchanges.h>
#include <cpptools/cpptoolsreuse.h>

#include <utils/qtcassert.h>

#include <QCoreApplication>
#include <QDir>
#include <QHash>
#include <QStringBuilder>
#include <QTextDocument>
#include <QTextBlock>
#include <QInputDialog>
#include <QMessageBox>

using namespace CPlusPlus;
using namespace CppEditor;
using namespace CppEditor::Internal;
using namespace CppTools;
using namespace TextEditor;

namespace {

class InsertDeclOperation: public CppQuickFixOperation
{
public:
    InsertDeclOperation(const QSharedPointer<const CppEditor::Internal::CppQuickFixAssistInterface> &interface,
                        const QString &targetFileName, const Class *targetSymbol,
                        InsertionPointLocator::AccessSpec xsSpec,
                        const QString &decl)
        : CppQuickFixOperation(interface, 0)
        , m_targetFileName(targetFileName)
        , m_targetSymbol(targetSymbol)
        , m_xsSpec(xsSpec)
        , m_decl(decl)
    {
        QString type;
        switch (xsSpec) {
        case InsertionPointLocator::Public: type = QLatin1String("public"); break;
        case InsertionPointLocator::Protected: type = QLatin1String("protected"); break;
        case InsertionPointLocator::Private: type = QLatin1String("private"); break;
        case InsertionPointLocator::PublicSlot: type = QLatin1String("public slot"); break;
        case InsertionPointLocator::ProtectedSlot: type = QLatin1String("protected slot"); break;
        case InsertionPointLocator::PrivateSlot: type = QLatin1String("private slot"); break;
        default: break;
        }

        setDescription(QCoreApplication::translate("CppEditor::InsertDeclOperation",
                                                   "Add %1 Declaration").arg(type));
    }

    void perform()
    {
        CppRefactoringChanges refactoring(snapshot());

        InsertionPointLocator locator(refactoring);
        const InsertionLocation loc = locator.methodDeclarationInClass(
                    m_targetFileName, m_targetSymbol, m_xsSpec);
        QTC_ASSERT(loc.isValid(), return);

        CppRefactoringFilePtr targetFile = refactoring.file(m_targetFileName);
        int targetPosition1 = targetFile->position(loc.line(), loc.column());
        int targetPosition2 = qMax(0, targetFile->position(loc.line(), 1) - 1);

        Utils::ChangeSet target;
        target.insert(targetPosition1, loc.prefix() + m_decl);
        targetFile->setChangeSet(target);
        targetFile->appendIndentRange(Utils::ChangeSet::Range(targetPosition2, targetPosition1));
        targetFile->setOpenEditor(true, targetPosition1);
        targetFile->apply();
    }

    static QString generateDeclaration(Function *function);

private:
    QString m_targetFileName;
    const Class *m_targetSymbol;
    InsertionPointLocator::AccessSpec m_xsSpec;
    QString m_decl;
};

Class *isMemberFunction(const LookupContext &context, Function *function)
{
    QTC_ASSERT(function, return 0);

    Scope *enclosingScope = function->enclosingScope();
    while (! (enclosingScope->isNamespace() || enclosingScope->isClass()))
        enclosingScope = enclosingScope->enclosingScope();
    QTC_ASSERT(enclosingScope != 0, return 0);

    const Name *functionName = function->name();
    if (! functionName)
        return 0; // anonymous function names are not valid c++

    if (! functionName->isQualifiedNameId())
        return 0; // trying to add a declaration for a global function

    const QualifiedNameId *q = functionName->asQualifiedNameId();
    if (!q->base())
        return 0;

    if (ClassOrNamespace *binding = context.lookupType(q->base(), enclosingScope)) {
        foreach (Symbol *s, binding->symbols()) {
            if (Class *matchingClass = s->asClass())
                return matchingClass;
        }
    }

    return 0;
}

} // anonymous namespace

void DeclFromDef::match(const CppQuickFixInterface &interface, QuickFixOperations &result)
{
    const QList<AST *> &path = interface->path();
    CppRefactoringFilePtr file = interface->currentFile();

    FunctionDefinitionAST *funDef = 0;
    int idx = 0;
    for (; idx < path.size(); ++idx) {
        AST *node = path.at(idx);
        if (idx > 1) {
            if (DeclaratorIdAST *declId = node->asDeclaratorId()) {
                if (file->isCursorOn(declId)) {
                    if (FunctionDefinitionAST *candidate = path.at(idx - 2)->asFunctionDefinition()) {
                        if (funDef)
                            return;
                        funDef = candidate;
                        break;
                    }
                }
            }
        }

        if (node->asClassSpecifier())
            return;
    }

    if (!funDef || !funDef->symbol)
        return;

    Function *fun = funDef->symbol;
    if (Class *matchingClass = isMemberFunction(interface->context(), fun)) {
        const QualifiedNameId *qName = fun->name()->asQualifiedNameId();
        for (Symbol *s = matchingClass->find(qName->identifier()); s; s = s->next()) {
            if (!s->name()
                    || !qName->identifier()->isEqualTo(s->identifier())
                    || !s->type()->isFunctionType())
                continue;

            if (s->type().isEqualTo(fun->type())) {
                // Declaration exists.
                return;
            }
        }
        QString fileName = QString::fromUtf8(matchingClass->fileName(),
                                             matchingClass->fileNameLength());
        const QString decl = InsertDeclOperation::generateDeclaration(fun);
        result.append(TextEditor::QuickFixOperation::Ptr(new InsertDeclOperation(interface, fileName, matchingClass,
                                                    InsertionPointLocator::Public, decl)));
    }
}

QString InsertDeclOperation::generateDeclaration(Function *function)
{
    Overview oo;
    oo.showFunctionSignatures = true;
    oo.showReturnTypes = true;
    oo.showArgumentNames = true;

    QString decl;
    decl += oo.prettyType(function->type(), function->unqualifiedName());
    decl += QLatin1String(";\n");

    return decl;
}

namespace {

class InsertDefOperation: public CppQuickFixOperation
{
public:
    InsertDefOperation(const QSharedPointer<const CppEditor::Internal::CppQuickFixAssistInterface> &interface,
                       Declaration *decl, const InsertionLocation &loc)
        : CppQuickFixOperation(interface, 0)
        , m_decl(decl)
        , m_loc(loc)
    {
        const QString declFile = QString::fromUtf8(decl->fileName(), decl->fileNameLength());
        const QDir dir = QFileInfo(declFile).dir();
        setDescription(QCoreApplication::translate("CppEditor::InsertDefOperation",
                                                   "Add Definition in %1")
                       .arg(dir.relativeFilePath(m_loc.fileName())));
    }

    void perform()
    {
        QTC_ASSERT(m_loc.isValid(), return);
        CppRefactoringChanges refactoring(snapshot());
        CppRefactoringFilePtr targetFile = refactoring.file(m_loc.fileName());

        Overview oo;
        oo.showFunctionSignatures = true;
        oo.showReturnTypes = true;
        oo.showArgumentNames = true;

        // make target lookup context
        Document::Ptr targetDoc = targetFile->cppDocument();
        Scope *targetScope = targetDoc->scopeAt(m_loc.line(), m_loc.column());
        LookupContext targetContext(targetDoc, assistInterface()->snapshot());
        ClassOrNamespace *targetCoN = targetContext.lookupType(targetScope);
        if (!targetCoN)
            targetCoN = targetContext.globalNamespace();

        // setup rewriting to get minimally qualified names
        SubstitutionEnvironment env;
        env.setContext(assistInterface()->context());
        env.switchScope(m_decl->enclosingScope());
        UseMinimalNames q(targetCoN);
        env.enter(&q);
        Control *control = assistInterface()->context().control().data();

        // rewrite the function type
        FullySpecifiedType tn = rewriteType(m_decl->type(), &env, control);

        // rewrite the function name
        QString name = oo.prettyName(LookupContext::minimalName(m_decl, targetCoN, control));

        QString defText = oo.prettyType(tn, name) + QLatin1String("\n{\n}");

        int targetPos = targetFile->position(m_loc.line(), m_loc.column());
        int targetPos2 = qMax(0, targetFile->position(m_loc.line(), 1) - 1);

        Utils::ChangeSet target;
        target.insert(targetPos,  m_loc.prefix() + defText + m_loc.suffix());
        targetFile->setChangeSet(target);
        targetFile->appendIndentRange(Utils::ChangeSet::Range(targetPos2, targetPos));
        targetFile->setOpenEditor(true, targetPos);
        targetFile->apply();
    }

private:
    Declaration *m_decl;
    InsertionLocation m_loc;
};

} // anonymous namespace

void DefFromDecl::match(const CppQuickFixInterface &interface, QuickFixOperations &result)
{
    const QList<AST *> &path = interface->path();

    int idx = path.size() - 1;
    for (; idx >= 0; --idx) {
        AST *node = path.at(idx);
        if (SimpleDeclarationAST *simpleDecl = node->asSimpleDeclaration()) {
            if (simpleDecl->symbols && ! simpleDecl->symbols->next) {
                if (Symbol *symbol = simpleDecl->symbols->value) {
                    if (Declaration *decl = symbol->asDeclaration()) {
                        if (decl->type()->isFunctionType()
                                && decl->enclosingScope()
                                && decl->enclosingScope()->isClass()) {
                            CppRefactoringChanges refactoring(interface->snapshot());
                            InsertionPointLocator locator(refactoring);
                            foreach (const InsertionLocation &loc, locator.methodDefinition(decl)) {
                                if (loc.isValid())
                                    result.append(CppQuickFixOperation::Ptr(new InsertDefOperation(interface, decl, loc)));
                            }
                            return;
                        }
                    }
                }
            }
            break;
        }
    }
}

namespace {

class ExtractFunctionOperation : public CppQuickFixOperation
{
public:
    ExtractFunctionOperation(const CppQuickFixInterface &interface,
                    int extractionStart,
                    int extractionEnd,
                    FunctionDefinitionAST *refFuncDef,
                    Symbol *funcReturn,
                    QList<QPair<QString, QString> > relevantDecls)
        : CppQuickFixOperation(interface)
        , m_extractionStart(extractionStart)
        , m_extractionEnd(extractionEnd)
        , m_refFuncDef(refFuncDef)
        , m_funcReturn(funcReturn)
        , m_relevantDecls(relevantDecls)
    {
        setDescription(QCoreApplication::translate("QuickFix::ExtractFunction", "Extract Function"));
    }

    void perform()
    {
        QTC_ASSERT(!m_funcReturn || !m_relevantDecls.isEmpty(), return);
        CppRefactoringChanges refactoring(snapshot());
        CppRefactoringFilePtr currentFile = refactoring.file(fileName());

        const QString &funcName = getFunctionName();
        if (funcName.isEmpty())
            return;

        Function *refFunc = m_refFuncDef->symbol;

        // We don't need to rewrite the type for declarations made inside the reference function,
        // since their scope will remain the same. Then we preserve the original spelling style.
        // However, we must do so for the return type in the definition.
        SubstitutionEnvironment env;
        env.setContext(assistInterface()->context());
        env.switchScope(refFunc);
        ClassOrNamespace *targetCoN =
                assistInterface()->context().lookupType(refFunc->enclosingScope());
        if (!targetCoN)
            targetCoN = assistInterface()->context().globalNamespace();
        UseMinimalNames subs(targetCoN);
        env.enter(&subs);

        Overview printer;
        Control *control = assistInterface()->context().control().data();
        QString funcDef;
        QString funcDecl; // We generate a declaration only in the case of a member function.
        QString funcCall;

        Class *matchingClass = isMemberFunction(assistInterface()->context(), refFunc);

        // Write return type.
        if (!m_funcReturn) {
            funcDef.append(QLatin1String("void "));
            if (matchingClass)
                funcDecl.append(QLatin1String("void "));
        } else {
            const FullySpecifiedType &fullType = rewriteType(m_funcReturn->type(), &env, control);
            funcDef.append(printer.prettyType(fullType) + QLatin1Char(' '));
            funcDecl.append(printer.prettyType(m_funcReturn->type()) + QLatin1Char(' '));
        }

        // Write class qualification, if any.
        if (matchingClass) {
            const Name *name = rewriteName(matchingClass->name(), &env, control);
            funcDef.append(printer.prettyName(name));
            funcDef.append(QLatin1String("::"));
        }

        // Write the extracted function itself and its call.
        funcDef.append(funcName);
        if (matchingClass)
            funcDecl.append(funcName);
        funcCall.append(funcName);
        funcDef.append(QLatin1Char('('));
        if (matchingClass)
            funcDecl.append(QLatin1Char('('));
        funcCall.append(QLatin1Char('('));
        for (int i = m_funcReturn ? 1 : 0; i < m_relevantDecls.length(); ++i) {
            QPair<QString, QString> p = m_relevantDecls.at(i);
            funcCall.append(p.first);
            funcDef.append(p.second);
            if (matchingClass)
                funcDecl.append(p.second);
            if (i < m_relevantDecls.length() - 1) {
                funcCall.append(QLatin1String(", "));
                funcDef.append(QLatin1String(", "));
                if (matchingClass)
                    funcDecl.append(QLatin1String(", "));
            }
        }
        funcDef.append(QLatin1Char(')'));
        if (matchingClass)
            funcDecl.append(QLatin1Char(')'));
        funcCall.append(QLatin1Char(')'));
        if (refFunc->isConst()) {
            funcDef.append(QLatin1String(" const"));
            funcDecl.append(QLatin1String(" const"));
        }
        funcDef.append(QLatin1String("\n{\n"));
        if (matchingClass)
            funcDecl.append(QLatin1String(";\n"));
        if (m_funcReturn) {
            funcDef.append(QLatin1String("\nreturn ")
                        % m_relevantDecls.at(0).first
                        % QLatin1String(";"));
            funcCall.prepend(m_relevantDecls.at(0).second % QLatin1String(" = "));
        }
        funcDef.append(QLatin1String("\n}\n\n"));
        funcDef.replace(QChar::ParagraphSeparator, QLatin1String("\n"));
        funcCall.append(QLatin1Char(';'));

        // Get starting indentation from original code.
        int indentedExtractionStart = m_extractionStart;
        QChar current = currentFile->document()->characterAt(indentedExtractionStart - 1);
        while (current == QLatin1Char(' ') || current == QLatin1Char('\t')) {
            --indentedExtractionStart;
            current = currentFile->document()->characterAt(indentedExtractionStart - 1);
        }
        QString extract = currentFile->textOf(indentedExtractionStart, m_extractionEnd);
        extract.replace(QChar::ParagraphSeparator, QLatin1String("\n"));
        if (!extract.endsWith(QLatin1Char('\n')) && m_funcReturn)
            extract.append(QLatin1Char('\n'));

        // Since we need an indent range and a nested reindent range (based on the original
        // formatting) it's simpler to have two different change sets.
        Utils::ChangeSet change;
        int position = currentFile->startOf(m_refFuncDef);
        change.insert(position, funcDef);
        change.replace(m_extractionStart, m_extractionEnd, funcCall);
        currentFile->setChangeSet(change);
        currentFile->appendIndentRange(Utils::ChangeSet::Range(position, position + 1));
        currentFile->apply();

        QTextCursor tc = currentFile->document()->find(QLatin1String("{"), position);
        QTC_ASSERT(tc.hasSelection(), return);
        position = tc.selectionEnd() + 1;
        change.clear();
        change.insert(position, extract);
        currentFile->setChangeSet(change);
        currentFile->appendReindentRange(Utils::ChangeSet::Range(position, position + 1));
        currentFile->apply();

        // Write declaration, if necessary.
        if (matchingClass) {
            InsertionPointLocator locator(refactoring);
            const QString fileName = QLatin1String(matchingClass->fileName());
            const InsertionLocation &location =
                    locator.methodDeclarationInClass(fileName, matchingClass,
                                                     InsertionPointLocator::Public);
            CppTools::CppRefactoringFilePtr declFile = refactoring.file(fileName);
            change.clear();
            position = declFile->position(location.line(), location.column());
            change.insert(position, funcDecl);
            declFile->setChangeSet(change);
            declFile->appendIndentRange(Utils::ChangeSet::Range(position, position + 1));
            declFile->apply();
        }
    }

    QString getFunctionName() const
    {
        bool ok;
        QString name =
                QInputDialog::getText(0,
                                      QCoreApplication::translate("QuickFix::ExtractFunction",
                                                                  "Extract Function Refactoring"),
                                      QCoreApplication::translate("QuickFix::ExtractFunction",
                                                                  "Enter function name"),
                                      QLineEdit::Normal,
                                      QString(),
                                      &ok);
        name = name.trimmed();
        if (!ok || name.isEmpty())
            return QString();

        if (!isValidIdentifier(name)) {
            QMessageBox::critical(0,
                                  QCoreApplication::translate("QuickFix::ExtractFunction",
                                                              "Extract Function Refactoring"),
                                  QCoreApplication::translate("QuickFix::ExtractFunction",
                                                              "Invalid function name"));
            return QString();
        }

        return name;
    }

    int m_extractionStart;
    int m_extractionEnd;
    FunctionDefinitionAST *m_refFuncDef;
    Symbol *m_funcReturn;
    QList<QPair<QString, QString> > m_relevantDecls;
};

QPair<QString, QString> assembleDeclarationData(const QString &specifiers,
                                                DeclaratorAST *decltr,
                                                const CppRefactoringFilePtr &file,
                                                const Overview &printer)
{
    QTC_ASSERT(decltr, return (QPair<QString, QString>()));
    if (decltr->core_declarator
            && decltr->core_declarator->asDeclaratorId()
            && decltr->core_declarator->asDeclaratorId()->name) {
        QString decltrText = file->textOf(file->startOf(decltr),
                                          file->endOf(decltr->core_declarator));
        if (!decltrText.isEmpty()) {
            const QString &name = printer.prettyName(
                    decltr->core_declarator->asDeclaratorId()->name->name);
            QString completeDecl = specifiers;
            if (!decltrText.contains(QLatin1Char(' ')))
                completeDecl.append(QLatin1Char(' ') + decltrText);
            else
                completeDecl.append(decltrText);
            return qMakePair(name, completeDecl);
        }
    }
    return QPair<QString, QString>();
}


class FunctionExtractionAnalyser : public ASTVisitor
{
public:
    FunctionExtractionAnalyser(TranslationUnit *unit,
                               const int selStart,
                               const int selEnd,
                               const CppRefactoringFilePtr &file,
                               const Overview &printer)
        : ASTVisitor(unit)
        , m_done(false)
        , m_failed(false)
        , m_selStart(selStart)
        , m_selEnd(selEnd)
        , m_extractionStart(0)
        , m_extractionEnd(0)
        , m_file(file)
        , m_printer(printer)
    {}

    bool operator()(FunctionDefinitionAST *refFunDef)
    {
        accept(refFunDef);

        if (!m_failed && m_extractionStart == m_extractionEnd)
            m_failed = true;

        return !m_failed;
    }

    bool preVisit(AST *)
    {
        if (m_done)
            return false;
        return true;
    }

    void statement(StatementAST *stmt)
    {
        if (!stmt)
            return;

        const int stmtStart = m_file->startOf(stmt);
        const int stmtEnd = m_file->endOf(stmt);

        if (stmtStart >= m_selEnd
                || (m_extractionStart && stmtEnd > m_selEnd)) {
            m_done = true;
            return;
        }

        if (stmtStart >= m_selStart && !m_extractionStart)
            m_extractionStart = stmtStart;
        if (stmtEnd > m_extractionEnd && m_extractionStart)
            m_extractionEnd = stmtEnd;

        accept(stmt);
    }

    bool visit(CaseStatementAST *stmt)
    {
        statement(stmt->statement);
        return false;
    }

    bool visit(CompoundStatementAST *stmt)
    {
        for (StatementListAST *it = stmt->statement_list; it; it = it->next) {
            statement(it->value);
            if (m_done)
                break;
        }
        return false;
    }

    bool visit(DoStatementAST *stmt)
    {
        statement(stmt->statement);
        return false;
    }

    bool visit(ForeachStatementAST *stmt)
    {
        statement(stmt->statement);
        return false;
    }

    bool visit(RangeBasedForStatementAST *stmt)
    {
        statement(stmt->statement);
        return false;
    }

    bool visit(ForStatementAST *stmt)
    {
        statement(stmt->initializer);
        if (!m_done)
            statement(stmt->statement);
        return false;
    }

    bool visit(IfStatementAST *stmt)
    {
        statement(stmt->statement);
        if (!m_done)
            statement(stmt->else_statement);
        return false;
    }

    bool visit(TryBlockStatementAST *stmt)
    {
        statement(stmt->statement);
        for (CatchClauseListAST *it = stmt->catch_clause_list; it; it = it->next) {
            statement(it->value);
            if (m_done)
                break;
        }
        return false;
    }

    bool visit(WhileStatementAST *stmt)
    {
        statement(stmt->statement);
        return false;
    }

    bool visit(DeclarationStatementAST *declStmt)
    {
        // We need to collect the declarations we see before the extraction or even inside it.
        // They might need to be used as either a parameter or return value. Actually, we could
        // still obtain their types from the local uses, but it's good to preserve the original
        // typing style.
        if (declStmt
                && declStmt->declaration
                && declStmt->declaration->asSimpleDeclaration()) {
            SimpleDeclarationAST *simpleDecl = declStmt->declaration->asSimpleDeclaration();
            if (simpleDecl->decl_specifier_list
                    && simpleDecl->declarator_list) {
                const QString &specifiers =
                        m_file->textOf(m_file->startOf(simpleDecl),
                                     m_file->endOf(simpleDecl->decl_specifier_list->lastValue()));
                for (DeclaratorListAST *decltrList = simpleDecl->declarator_list;
                     decltrList;
                     decltrList = decltrList->next) {
                    const QPair<QString, QString> p =
                        assembleDeclarationData(specifiers, decltrList->value, m_file, m_printer);
                    if (!p.first.isEmpty())
                        m_knownDecls.insert(p.first, p.second);
                }
            }
        }

        return false;
    }

    bool visit(ReturnStatementAST *)
    {
        if (m_extractionStart) {
            m_done = true;
            m_failed = true;
        }

        return false;
    }

    bool m_done;
    bool m_failed;
    const int m_selStart;
    const int m_selEnd;
    int m_extractionStart;
    int m_extractionEnd;
    QHash<QString, QString> m_knownDecls;
    CppRefactoringFilePtr m_file;
    const Overview &m_printer;
};

} // anonymous namespace

void ExtractFunction::match(const CppQuickFixInterface &interface, QuickFixOperations &result)
{
    CppRefactoringFilePtr file = interface->currentFile();

    QTextCursor cursor = file->cursor();
    if (!cursor.hasSelection())
        return;

    const QList<AST *> &path = interface->path();
    FunctionDefinitionAST *refFuncDef = 0; // The "reference" function, which we will extract from.
    for (int i = path.size() - 1; i >= 0; --i) {
        refFuncDef = path.at(i)->asFunctionDefinition();
        if (refFuncDef)
            break;
    }

    if (!refFuncDef
            || !refFuncDef->function_body
            || !refFuncDef->function_body->asCompoundStatement()
            || !refFuncDef->function_body->asCompoundStatement()->statement_list
            || !refFuncDef->symbol
            || !refFuncDef->symbol->name()
            || refFuncDef->symbol->enclosingScope()->isTemplate() /* TODO: Templates... */) {
        return;
    }

    // Adjust selection ends.
    int selStart = cursor.selectionStart();
    int selEnd = cursor.selectionEnd();
    if (selStart > selEnd)
        qSwap(selStart, selEnd);

    Overview printer;

    // Analyse the content to be extracted, which consists of determining the statements
    // which are complete and collecting the declarations seen.
    FunctionExtractionAnalyser analyser(interface->semanticInfo().doc->translationUnit(),
                                        selStart, selEnd,
                                        file,
                                        printer);
    if (!analyser(refFuncDef))
        return;

    // We also need to collect the declarations of the parameters from the reference function.
    QSet<QString> refFuncParams;
    if (refFuncDef->declarator->postfix_declarator_list
            && refFuncDef->declarator->postfix_declarator_list->value
            && refFuncDef->declarator->postfix_declarator_list->value->asFunctionDeclarator()) {
        FunctionDeclaratorAST *funcDecltr =
            refFuncDef->declarator->postfix_declarator_list->value->asFunctionDeclarator();
        if (funcDecltr->parameter_declaration_clause
                && funcDecltr->parameter_declaration_clause->parameter_declaration_list) {
            for (ParameterDeclarationListAST *it =
                    funcDecltr->parameter_declaration_clause->parameter_declaration_list;
                 it;
                 it = it->next) {
                ParameterDeclarationAST *paramDecl = it->value->asParameterDeclaration();
                if (paramDecl->declarator) {
                    const QString &specifiers =
                            file->textOf(file->startOf(paramDecl),
                                         file->endOf(paramDecl->type_specifier_list->lastValue()));
                    const QPair<QString, QString> &p =
                            assembleDeclarationData(specifiers, paramDecl->declarator, file, printer);
                    if (!p.first.isEmpty()) {
                        analyser.m_knownDecls.insert(p.first, p.second);
                        refFuncParams.insert(p.first);
                    }
                }
            }
        }
    }

    // Identify what would be parameters for the new function and its return value, if any.
    Symbol *funcReturn = 0;
    QList<QPair<QString, QString> > relevantDecls;
    SemanticInfo::LocalUseIterator it(interface->semanticInfo().localUses);
    while (it.hasNext()) {
        it.next();

        bool usedBeforeExtraction = false;
        bool usedAfterExtraction = false;
        bool usedInsideExtraction = false;
        const QList<SemanticInfo::Use> &uses = it.value();
        foreach (const SemanticInfo::Use &use, uses) {
            const int position = file->position(use.line, use.column);
            if (position < analyser.m_extractionStart)
                usedBeforeExtraction = true;
            else if (position >= analyser.m_extractionEnd)
                usedAfterExtraction = true;
            else
                usedInsideExtraction = true;
        }

        const QString &name = printer.prettyName(it.key()->name());

        if ((usedBeforeExtraction && usedInsideExtraction)
                || (usedInsideExtraction && refFuncParams.contains(name))) {
            QTC_ASSERT(analyser.m_knownDecls.contains(name), return);
            relevantDecls.append(qMakePair(name, analyser.m_knownDecls.value(name)));
        }

        // We assume that the first use of a local corresponds to its declaration.
        if (usedInsideExtraction && usedAfterExtraction && !usedBeforeExtraction) {
            if (!funcReturn) {
                QTC_ASSERT(analyser.m_knownDecls.contains(name), return);
                // The return, if any, is stored as the first item in the list.
                relevantDecls.prepend(qMakePair(name, analyser.m_knownDecls.value(name)));
                funcReturn = it.key();
            } else {
                // Would require multiple returns. (Unless we do fancy things, as pointed below.)
                return;
            }
        }
    }

    // The current implementation doesn't try to be too smart since it preserves the original form
    // of the declarations. This might be or not the desired effect. An improvement would be to
    // let the user somehow customize the function interface.
    result.append(CppQuickFixOperation::Ptr(new ExtractFunctionOperation(interface,
                                                     analyser.m_extractionStart,
                                                     analyser.m_extractionEnd,
                                                     refFuncDef,
                                                     funcReturn,
                                                     relevantDecls)));
}
