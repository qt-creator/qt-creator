// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "extractfunction.h"

#include "../cppcodestylesettings.h"
#include "../cppeditortr.h"
#include "../cpprefactoringchanges.h"
#include "../insertionpointlocator.h"
#include "cppquickfix.h"
#include "cppquickfixhelpers.h"

#include <coreplugin/icore.h>
#include <cplusplus/CppRewriter.h>
#include <cplusplus/declarationcomments.h>
#include <cplusplus/Overview.h>

#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QPushButton>

#include <functional>

#ifdef WITH_TESTS
#include "cppquickfix_test.h"
#endif

using namespace CPlusPlus;
using namespace Utils;

namespace CppEditor::Internal {
namespace {
using FunctionNameGetter = std::function<QString()>;

class ExtractFunctionOptions
{
public:
    static bool isValidFunctionName(const QString &name)
    {
        return !name.isEmpty() && isValidIdentifier(name);
    }

    bool hasValidFunctionName() const
    {
        return isValidFunctionName(funcName);
    }

    QString funcName;
    InsertionPointLocator::AccessSpec access = InsertionPointLocator::Public;
};

class ExtractFunctionOperation : public CppQuickFixOperation
{
public:
    ExtractFunctionOperation(
        const CppQuickFixInterface &interface,
        int extractionStart,
        int extractionEnd,
        FunctionDefinitionAST *refFuncDef,
        Symbol *funcReturn,
        QList<QPair<QString, QString>> relevantDecls,
        FunctionNameGetter functionNameGetter = {})
        : CppQuickFixOperation(interface)
        , m_extractionStart(extractionStart)
        , m_extractionEnd(extractionEnd)
        , m_refFuncDef(refFuncDef)
        , m_funcReturn(funcReturn)
        , m_relevantDecls(relevantDecls)
        , m_functionNameGetter(functionNameGetter)
    {
        setDescription(Tr::tr("Extract Function"));
    }

    void perform() override
    {
        QTC_ASSERT(!m_funcReturn || !m_relevantDecls.isEmpty(), return);

        CppRefactoringChanges refactoring(snapshot());
        ExtractFunctionOptions options;
        if (m_functionNameGetter)
            options.funcName = m_functionNameGetter();
        else
            options = getOptions();

        if (!options.hasValidFunctionName())
            return;
        const QString &funcName = options.funcName;

        Function *refFunc = m_refFuncDef->symbol;

        // We don't need to rewrite the type for declarations made inside the reference function,
        // since their scope will remain the same. Then we preserve the original spelling style.
        // However, we must do so for the return type in the definition.
        SubstitutionEnvironment env;
        env.setContext(context());
        env.switchScope(refFunc);
        ClassOrNamespace *targetCoN = context().lookupType(refFunc->enclosingScope());
        if (!targetCoN)
            targetCoN = context().globalNamespace();
        UseMinimalNames subs(targetCoN);
        env.enter(&subs);

        Overview printer = CppCodeStyleSettings::currentProjectCodeStyleOverview();
        Control *control = context().bindings()->control().get();
        QString funcDef;
        QString funcDecl; // We generate a declaration only in the case of a member function.
        QString funcCall;

        Class *matchingClass = isMemberFunction(context(), refFunc);

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
            const Scope *current = matchingClass;
            QList<const Name *> classes{matchingClass->name()};
            while (current->enclosingScope()->asClass()) {
                current = current->enclosingScope()->asClass();
                classes.prepend(current->name());
            }
            while (current->enclosingScope() && current->enclosingScope()->asNamespace()) {
                current = current->enclosingScope()->asNamespace();
                if (current->name())
                    classes.prepend(current->name());
            }
            for (const Name *n : classes) {
                const Name *name = rewriteName(n, &env, control);
                funcDef.append(printer.prettyName(name));
                funcDef.append(QLatin1String("::"));
            }
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
        QString extract = currentFile()->textOf(m_extractionStart, m_extractionEnd);
        extract.replace(QChar::ParagraphSeparator, QLatin1String("\n"));
        if (!extract.endsWith(QLatin1Char('\n')) && m_funcReturn)
            extract.append(QLatin1Char('\n'));
        funcDef.append(extract);
        if (matchingClass)
            funcDecl.append(QLatin1String(";\n"));
        if (m_funcReturn) {
            funcDef.append(QLatin1String("\nreturn ")
                           + m_relevantDecls.at(0).first
                           + QLatin1Char(';'));
            funcCall.prepend(m_relevantDecls.at(0).second + QLatin1String(" = "));
        }
        funcDef.append(QLatin1String("\n}\n\n"));
        funcDef.replace(QChar::ParagraphSeparator, QLatin1String("\n"));
        funcDef.prepend(inlinePrefix(currentFile()->filePath()));
        funcCall.append(QLatin1Char(';'));

        // Do not insert right between the function and an associated comment.
        int position = currentFile()->startOf(m_refFuncDef);
        const QList<Token> functionDoc = commentsForDeclaration(
            m_refFuncDef->symbol, m_refFuncDef, *currentFile()->document(),
            currentFile()->cppDocument());
        if (!functionDoc.isEmpty()) {
            position = currentFile()->cppDocument()->translationUnit()->getTokenPositionInDocument(
                functionDoc.first(), currentFile()->document());
        }

        ChangeSet change;
        change.insert(position, funcDef);
        change.replace(m_extractionStart, m_extractionEnd, funcCall);
        currentFile()->apply(change);

        // Write declaration, if necessary.
        if (matchingClass) {
            InsertionPointLocator locator(refactoring);
            const FilePath filePath = FilePath::fromUtf8(matchingClass->fileName());
            const InsertionLocation &location =
                locator.methodDeclarationInClass(filePath, matchingClass, options.access);
            CppRefactoringFilePtr declFile = refactoring.cppFile(filePath);
            declFile->apply(ChangeSet::makeInsert(
                declFile->position(location.line(), location.column()),
                location.prefix() + funcDecl + location.suffix()));
        }
    }

    ExtractFunctionOptions getOptions() const
    {
        QDialog dlg(Core::ICore::dialogParent());
        dlg.setWindowTitle(Tr::tr("Extract Function Refactoring"));
        auto layout = new QFormLayout(&dlg);

        auto funcNameEdit = new FancyLineEdit;
        funcNameEdit->setValidationFunction([](FancyLineEdit *edit) -> Result<> {
            if (ExtractFunctionOptions::isValidFunctionName(edit->text()))
                return ResultOk;
            return ResultError(QString());
        });
        layout->addRow(Tr::tr("Function name"), funcNameEdit);

        auto accessCombo = new QComboBox;
        accessCombo->addItem(
            InsertionPointLocator::accessSpecToString(InsertionPointLocator::Public),
            InsertionPointLocator::Public);
        accessCombo->addItem(
            InsertionPointLocator::accessSpecToString(InsertionPointLocator::PublicSlot),
            InsertionPointLocator::PublicSlot);
        accessCombo->addItem(
            InsertionPointLocator::accessSpecToString(InsertionPointLocator::Protected),
            InsertionPointLocator::Protected);
        accessCombo->addItem(
            InsertionPointLocator::accessSpecToString(InsertionPointLocator::ProtectedSlot),
            InsertionPointLocator::ProtectedSlot);
        accessCombo->addItem(
            InsertionPointLocator::accessSpecToString(InsertionPointLocator::Private),
            InsertionPointLocator::Private);
        accessCombo->addItem(
            InsertionPointLocator::accessSpecToString(InsertionPointLocator::PrivateSlot),
            InsertionPointLocator::PrivateSlot);
        layout->addRow(Tr::tr("Access"), accessCombo);

        auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        QObject::connect(buttonBox, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
        QObject::connect(buttonBox, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
        QPushButton *ok = buttonBox->button(QDialogButtonBox::Ok);
        ok->setEnabled(false);
        QObject::connect(funcNameEdit, &Utils::FancyLineEdit::validChanged,
                         ok, &QPushButton::setEnabled);
        layout->addWidget(buttonBox);

        if (dlg.exec() == QDialog::Accepted) {
            ExtractFunctionOptions options;
            options.funcName = funcNameEdit->text();
            options.access = static_cast<InsertionPointLocator::AccessSpec>(accessCombo->
                                                                            currentData().toInt());
            return options;
        }
        return ExtractFunctionOptions();
    }

    int m_extractionStart;
    int m_extractionEnd;
    FunctionDefinitionAST *m_refFuncDef;
    Symbol *m_funcReturn;
    QList<QPair<QString, QString> > m_relevantDecls;
    FunctionNameGetter m_functionNameGetter;
};

static QPair<QString, QString> assembleDeclarationData(
    const QString &specifiers,
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
            return {name, completeDecl};
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

    bool preVisit(AST *) override
    {
        return !m_done;
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

    bool visit(CaseStatementAST *stmt) override
    {
        statement(stmt->statement);
        return false;
    }

    bool visit(CompoundStatementAST *stmt) override
    {
        for (StatementListAST *it = stmt->statement_list; it; it = it->next) {
            statement(it->value);
            if (m_done)
                break;
        }
        return false;
    }

    bool visit(DoStatementAST *stmt) override
    {
        statement(stmt->statement);
        return false;
    }

    bool visit(ForeachStatementAST *stmt) override
    {
        statement(stmt->statement);
        return false;
    }

    bool visit(RangeBasedForStatementAST *stmt) override
    {
        statement(stmt->statement);
        return false;
    }

    bool visit(ForStatementAST *stmt) override
    {
        statement(stmt->initializer);
        if (!m_done)
            statement(stmt->statement);
        return false;
    }

    bool visit(IfStatementAST *stmt) override
    {
        statement(stmt->statement);
        if (!m_done)
            statement(stmt->else_statement);
        return false;
    }

    bool visit(TryBlockStatementAST *stmt) override
    {
        statement(stmt->statement);
        for (CatchClauseListAST *it = stmt->catch_clause_list; it; it = it->next) {
            statement(it->value);
            if (m_done)
                break;
        }
        return false;
    }

    bool visit(WhileStatementAST *stmt) override
    {
        statement(stmt->statement);
        return false;
    }

    bool visit(DeclarationStatementAST *declStmt) override
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

    bool visit(ReturnStatementAST *) override
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

//! Extracts the selected code and puts it to a function
class ExtractFunction : public CppQuickFixFactory
{
    void doMatch(const CppQuickFixInterface &interface, QuickFixOperations &result) override
    {
        const CppRefactoringFilePtr file = interface.currentFile();

        // TODO: Fix upstream and uncomment; see QTCREATORBUG-28030.
        //    if (CppModelManager::usesClangd(file->editor()->textDocument())
        //            && file->cppDocument()->languageFeatures().cxxEnabled) {
        //        return;
        //    }

        QTextCursor cursor = file->cursor();
        if (!cursor.hasSelection())
            return;

        const QList<AST *> &path = interface.path();
        FunctionDefinitionAST *refFuncDef = nullptr; // The "reference" function, which we will extract from.
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
            || refFuncDef->symbol->enclosingScope()->asTemplate() /* TODO: Templates... */) {
            return;
        }

        // Adjust selection ends.
        int selStart = cursor.selectionStart();
        int selEnd = cursor.selectionEnd();
        if (selStart > selEnd)
            std::swap(selStart, selEnd);

        Overview printer;

        // Analyze the content to be extracted, which consists of determining the statements
        // which are complete and collecting the declarations seen.
        FunctionExtractionAnalyser analyser(interface.semanticInfo().doc->translationUnit(),
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
                            assembleDeclarationData(specifiers, paramDecl->declarator,
                                                    file, printer);
                        if (!p.first.isEmpty()) {
                            analyser.m_knownDecls.insert(p.first, p.second);
                            refFuncParams.insert(p.first);
                        }
                    }
                }
            }
        }

        // Identify what would be parameters for the new function and its return value, if any.
        Symbol *funcReturn = nullptr;
        QList<QPair<QString, QString> > relevantDecls;
        const SemanticInfo::LocalUseMap localUses = interface.semanticInfo().localUses;
        for (auto it = localUses.cbegin(), end = localUses.cend(); it != end; ++it) {
            bool usedBeforeExtraction = false;
            bool usedAfterExtraction = false;
            bool usedInsideExtraction = false;
            const QList<SemanticInfo::Use> &uses = it.value();
            for (const SemanticInfo::Use &use : uses) {
                if (use.isInvalid())
                    continue;

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
                relevantDecls.push_back({name, analyser.m_knownDecls.value(name)});
            }

            // We assume that the first use of a local corresponds to its declaration.
            if (usedInsideExtraction && usedAfterExtraction && !usedBeforeExtraction) {
                if (!funcReturn) {
                    QTC_ASSERT(analyser.m_knownDecls.contains(name), return);
                    // The return, if any, is stored as the first item in the list.
                    relevantDecls.push_front({name, analyser.m_knownDecls.value(name)});
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
        FunctionNameGetter nameGetter;
        if (testMode())
            nameGetter = []() { return QLatin1String("extracted"); };
        result << new ExtractFunctionOperation(interface,
                                               analyser.m_extractionStart,
                                               analyser.m_extractionEnd,
                                               refFuncDef, funcReturn, relevantDecls,
                                               nameGetter);
    }
};

#ifdef WITH_TESTS
class ExtractFunctionTest : public Tests::CppQuickFixTestObject
{
    Q_OBJECT
public:
    using CppQuickFixTestObject::CppQuickFixTestObject;
};
#endif

} // namespace

void registerExtractFunctionQuickfix()
{
    REGISTER_QUICKFIX_FACTORY_WITH_STANDARD_TEST(ExtractFunction);
}

} // namespace CppEditor::Internal

#ifdef WITH_TESTS
#include <extractfunction.moc>
#endif
