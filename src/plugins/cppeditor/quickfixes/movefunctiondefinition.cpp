// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "movefunctiondefinition.h"

#include "../cppcodestylesettings.h"
#include "../cppeditortr.h"
#include "../cpprefactoringchanges.h"
#include "../insertionpointlocator.h"
#include "../symbolfinder.h"
#include "cppquickfix.h"
#include "cppquickfixhelpers.h"

#include <cplusplus/ASTPath.h>
#include <cplusplus/CppRewriter.h>
#include <cplusplus/Overview.h>
#include <projectexplorer/projectmanager.h>

using namespace CPlusPlus;
using namespace ProjectExplorer;
using namespace TextEditor;
using namespace Utils;

#ifdef WITH_TESTS
#include "cppquickfix_test.h"
#include <QtTest>
#endif

namespace CppEditor::Internal {
namespace {

static QString definitionSignature(
    const CppQuickFixInterface *assist,
    FunctionDefinitionAST *functionDefinitionAST,
    CppRefactoringFilePtr &baseFile,
    CppRefactoringFilePtr &targetFile,
    Scope *scope)
{
    QTC_ASSERT(assist, return QString());
    QTC_ASSERT(functionDefinitionAST, return QString());
    QTC_ASSERT(scope, return QString());
    Function *func = functionDefinitionAST->symbol;
    QTC_ASSERT(func, return QString());

    LookupContext cppContext(targetFile->cppDocument(), assist->snapshot());
    ClassOrNamespace *cppCoN = cppContext.lookupType(scope);
    if (!cppCoN)
        cppCoN = cppContext.globalNamespace();
    SubstitutionEnvironment env;
    env.setContext(assist->context());
    env.switchScope(func->enclosingScope());
    UseMinimalNames q(cppCoN);
    env.enter(&q);
    Control *control = assist->context().bindings()->control().get();
    Overview oo = CppCodeStyleSettings::currentProjectCodeStyleOverview();
    oo.showFunctionSignatures = true;
    oo.showReturnTypes = true;
    oo.showArgumentNames = true;
    oo.showEnclosingTemplate = true;
    oo.showTemplateParameters = true;
    oo.trailingReturnType = functionDefinitionAST->declarator
                            && functionDefinitionAST->declarator->postfix_declarator_list
                            && functionDefinitionAST->declarator->postfix_declarator_list->value
                            && functionDefinitionAST->declarator->postfix_declarator_list
                                   ->value->asFunctionDeclarator()
                            && functionDefinitionAST->declarator->postfix_declarator_list
                                   ->value->asFunctionDeclarator()->trailing_return_type;
    const Name *name = func->name();
    if (name && nameIncludesOperatorName(name)) {
        CoreDeclaratorAST *coreDeclarator = functionDefinitionAST->declarator->core_declarator;
        const QString operatorNameText = baseFile->textOf(coreDeclarator);
        oo.includeWhiteSpaceInOperatorName = operatorNameText.contains(QLatin1Char(' '));
    }
    const QString nameText = oo.prettyName(LookupContext::minimalName(func, cppCoN, control));
    oo.showTemplateParameters = false;
    const FullySpecifiedType tn = rewriteType(func->type(), &env, control);

    return oo.prettyType(tn, nameText);
}

class MoveFuncDefRefactoringHelper
{
public:
    enum MoveType {
        MoveOutside,
        MoveToCppFile,
        MoveOutsideMemberToCppFile
    };

    MoveFuncDefRefactoringHelper(CppQuickFixOperation *operation, MoveType type,
                                 const FilePath &toFile)
        : m_operation(operation), m_type(type), m_changes(m_operation->snapshot())
    {
        m_fromFile = operation->currentFile();
        m_toFile = (m_type == MoveOutside) ? m_fromFile : m_changes.cppFile(toFile);
    }

    void performMove(FunctionDefinitionAST *funcAST)
    {
        // Determine file, insert position and scope
        InsertionLocation l = insertLocationForMethodDefinition(
            funcAST->symbol, false, NamespaceHandling::Ignore,
            m_changes, m_toFile->filePath());
        const QString prefix = l.prefix();
        const QString suffix = l.suffix();
        const int insertPos = m_toFile->position(l.line(), l.column());
        Scope *scopeAtInsertPos = m_toFile->cppDocument()->scopeAt(l.line(), l.column());

        // construct definition
        const QString funcDec = inlinePrefix(m_toFile->filePath(), [this] { return m_type == MoveOutside; })
                                + definitionSignature(m_operation, funcAST, m_fromFile, m_toFile,
                                                      scopeAtInsertPos);
        QString funcDef = prefix + funcDec;
        const int startPosition = m_fromFile->endOf(funcAST->declarator);
        const int endPosition = m_fromFile->endOf(funcAST);
        funcDef += m_fromFile->textOf(startPosition, endPosition);
        funcDef += suffix;

        // insert definition at new position
        m_toFileChangeSet.insert(insertPos, funcDef);
        m_toFile->setOpenEditor(true, insertPos);

        // remove definition from fromFile
        if (m_type == MoveOutsideMemberToCppFile) {
            m_fromFileChangeSet.remove(m_fromFile->range(funcAST));
        } else {
            QString textFuncDecl = m_fromFile->textOf(funcAST);
            textFuncDecl.truncate(startPosition - m_fromFile->startOf(funcAST));
            if (textFuncDecl.left(7) == QLatin1String("inline "))
                textFuncDecl = textFuncDecl.mid(7);
            else
                textFuncDecl.replace(" inline ", QLatin1String(" "));
            textFuncDecl = textFuncDecl.trimmed() + QLatin1Char(';');
            m_fromFileChangeSet.replace(m_fromFile->range(funcAST), textFuncDecl);
        }
    }

    void applyChanges()
    {
        m_toFile->apply(m_toFileChangeSet);
        m_fromFile->apply(m_fromFileChangeSet);
    }

private:
    CppQuickFixOperation *m_operation;
    MoveType m_type;
    CppRefactoringChanges m_changes;
    CppRefactoringFilePtr m_fromFile;
    CppRefactoringFilePtr m_toFile;
    ChangeSet m_fromFileChangeSet;
    ChangeSet m_toFileChangeSet;
};

class MoveFuncDefOutsideOp : public CppQuickFixOperation
{
public:
    MoveFuncDefOutsideOp(const CppQuickFixInterface &interface,
                         MoveFuncDefRefactoringHelper::MoveType type,
                         FunctionDefinitionAST *funcDef, const FilePath &cppFilePath)
        : CppQuickFixOperation(interface, 0)
        , m_funcDef(funcDef)
        , m_type(type)
        , m_cppFilePath(cppFilePath)
    {
        if (m_type == MoveFuncDefRefactoringHelper::MoveOutside) {
            setDescription(Tr::tr("Move Definition Outside Class"));
        } else {
            const FilePath resolved = m_cppFilePath.relativePathFrom(filePath().parentDir());
            setDescription(Tr::tr("Move Definition to %1").arg(resolved.displayName()));
        }
    }

    void perform() override
    {
        MoveFuncDefRefactoringHelper helper(this, m_type, m_cppFilePath);
        helper.performMove(m_funcDef);
        helper.applyChanges();
    }

private:
    FunctionDefinitionAST *m_funcDef;
    MoveFuncDefRefactoringHelper::MoveType m_type;
    const FilePath m_cppFilePath;
};

class MoveAllFuncDefOutsideOp : public CppQuickFixOperation
{
public:
    MoveAllFuncDefOutsideOp(const CppQuickFixInterface &interface,
                            MoveFuncDefRefactoringHelper::MoveType type,
                            ClassSpecifierAST *classDef, const FilePath &cppFileName)
        : CppQuickFixOperation(interface, 0)
        , m_type(type)
        , m_classDef(classDef)
        , m_cppFilePath(cppFileName)
    {
        if (m_type == MoveFuncDefRefactoringHelper::MoveOutside) {
            setDescription(Tr::tr("Definitions Outside Class"));
        } else {
            const FilePath resolved = m_cppFilePath.relativePathFrom(filePath().parentDir());
            setDescription(Tr::tr("Move All Function Definitions to %1")
                               .arg(resolved.displayName()));
        }
    }

    void perform() override
    {
        MoveFuncDefRefactoringHelper helper(this, m_type, m_cppFilePath);
        for (DeclarationListAST *it = m_classDef->member_specifier_list; it; it = it->next) {
            if (FunctionDefinitionAST *funcAST = it->value->asFunctionDefinition()) {
                if (funcAST->symbol && !funcAST->symbol->isGenerated())
                    helper.performMove(funcAST);
            }
        }
        helper.applyChanges();
    }

private:
    MoveFuncDefRefactoringHelper::MoveType m_type;
    ClassSpecifierAST *m_classDef;
    const FilePath m_cppFilePath;
};

class MoveFuncDefToDeclOp : public CppQuickFixOperation
{
public:
    enum Type { Push, Pull };
    MoveFuncDefToDeclOp(const CppQuickFixInterface &interface,
                        const FilePath &fromFilePath, const FilePath &toFilePath,
                        FunctionDefinitionAST *funcAst, Function *func, const QString &declText,
                        const ChangeSet::Range &fromRange,
                        const ChangeSet::Range &toRange,
                        Type type)
        : CppQuickFixOperation(interface, 0)
        , m_fromFilePath(fromFilePath)
        , m_toFilePath(toFilePath)
        , m_funcAST(funcAst)
        , m_func(func)
        , m_declarationText(declText)
        , m_fromRange(fromRange)
        , m_toRange(toRange)
    {
        if (type == Type::Pull) {
            setDescription(Tr::tr("Move Definition Here"));
        } else if (m_toFilePath == m_fromFilePath) {
            setDescription(Tr::tr("Move Definition to Class"));
        } else {
            const FilePath resolved = m_toFilePath.relativePathFrom(m_fromFilePath.parentDir());
            setDescription(Tr::tr("Move Definition to %1").arg(resolved.displayName()));
        }
    }

private:
    void perform() override
    {
        CppRefactoringChanges refactoring(snapshot());
        CppRefactoringFilePtr fromFile = refactoring.cppFile(m_fromFilePath);
        CppRefactoringFilePtr toFile = refactoring.cppFile(m_toFilePath);

        ensureFuncDefAstAndRange(*fromFile);
        if (!m_funcAST)
            return;

        const QString wholeFunctionText = m_declarationText
                                          + fromFile->textOf(fromFile->endOf(m_funcAST->declarator),
                                                             fromFile->endOf(m_funcAST->function_body));

        // Replace declaration with function and delete old definition
        ChangeSet toTarget;
        toTarget.replace(m_toRange, wholeFunctionText);
        if (m_toFilePath == m_fromFilePath)
            toTarget.remove(m_fromRange);
        toFile->setOpenEditor(true, m_toRange.start);
        toFile->apply(toTarget);
        if (m_toFilePath != m_fromFilePath)
            fromFile->apply(ChangeSet::makeRemove(m_fromRange));
    }

    void ensureFuncDefAstAndRange(CppRefactoringFile &defFile)
    {
        if (m_funcAST) {
            QTC_CHECK(m_fromRange.end > m_fromRange.start);
            return;
        }
        QTC_ASSERT(m_func, return);
        const QList<AST *> astPath = ASTPath(defFile.cppDocument())(m_func->line(),
                                                                    m_func->column());
        if (astPath.isEmpty())
            return;
        for (auto it = std::rbegin(astPath); it != std::rend(astPath); ++it) {
            m_funcAST = (*it)->asFunctionDefinition();
            if (!m_funcAST)
                continue;
            AST *astForRange = m_funcAST;
            const auto prev = std::next(it);
            if (prev != std::rend(astPath)) {
                if (const auto templAst = (*prev)->asTemplateDeclaration())
                    astForRange = templAst;
            }
            m_fromRange = defFile.range(astForRange);
            return;
        }
    }

    const FilePath m_fromFilePath;
    const FilePath m_toFilePath;
    FunctionDefinitionAST *m_funcAST;
    Function *m_func;
    const QString m_declarationText;
    ChangeSet::Range m_fromRange;
    const ChangeSet::Range m_toRange;
};

/*!
 Moves the definition of a member function outside the class or moves the definition of a member
 function or a normal function to the implementation file.
 */
class MoveFuncDefOutside : public CppQuickFixFactory
{
public:
#ifdef WITH_TESTS
    static QObject *createTest();
#endif

private:
    void doMatch(const CppQuickFixInterface &interface, QuickFixOperations &result) override
    {
        const QList<AST *> &path = interface.path();
        SimpleDeclarationAST *classAST = nullptr;
        FunctionDefinitionAST *funcAST = nullptr;
        bool moveOutsideMemberDefinition = false;

        const int pathSize = path.size();
        for (int idx = 1; idx < pathSize; ++idx) {
            if ((funcAST = path.at(idx)->asFunctionDefinition())) {
                // check cursor position
                if (idx != pathSize - 1  // Do not allow "void a() @ {..."
                    && funcAST->function_body
                    && !interface.isCursorOn(funcAST->function_body)) {
                    if (path.at(idx - 1)->asTranslationUnit()) { // normal function
                        if (idx + 3 < pathSize && path.at(idx + 3)->asQualifiedName()) // Outside member
                            moveOutsideMemberDefinition = true;                        // definition
                        break;
                    }

                    if (idx > 1) {
                        if ((classAST = path.at(idx - 2)->asSimpleDeclaration())) // member function
                            break;
                        if (path.at(idx - 2)->asNamespace())  // normal function in namespace
                            break;
                    }
                    if (idx > 2 && path.at(idx - 1)->asTemplateDeclaration()) {
                        if ((classAST = path.at(idx - 3)->asSimpleDeclaration())) // member template
                            break;
                    }
                }
                funcAST = nullptr;
            }
        }

        if (!funcAST || !funcAST->symbol)
            return;

        bool isHeaderFile = false;
        const FilePath cppFileName = correspondingHeaderOrSource(interface.filePath(), &isHeaderFile);

        if (isHeaderFile && !cppFileName.isEmpty()) {
            const MoveFuncDefRefactoringHelper::MoveType type = moveOutsideMemberDefinition
                                                                    ? MoveFuncDefRefactoringHelper::MoveOutsideMemberToCppFile
                                                                    : MoveFuncDefRefactoringHelper::MoveToCppFile;
            result << new MoveFuncDefOutsideOp(interface, type, funcAST, cppFileName);
        }

        if (classAST)
            result << new MoveFuncDefOutsideOp(interface, MoveFuncDefRefactoringHelper::MoveOutside,
                                               funcAST, FilePath());

        return;
    }
};

//! Moves all member function definitions outside the class or to the implementation file.
class MoveAllFuncDefOutside : public CppQuickFixFactory
{
public:
#ifdef WITH_TESTS
    static QObject *createTest();
#endif

private:
    void doMatch(const CppQuickFixInterface &interface, QuickFixOperations &result) override
    {
        ClassSpecifierAST * const classAST = astForClassOperations(interface);
        if (!classAST)
            return;

        // Determine if the class has at least one function definition
        bool classContainsFunctions = false;
        for (DeclarationListAST *it = classAST->member_specifier_list; it; it = it->next) {
            if (FunctionDefinitionAST *funcAST = it->value->asFunctionDefinition()) {
                if (funcAST->symbol && !funcAST->symbol->isGenerated()) {
                    classContainsFunctions = true;
                    break;
                }
            }
        }
        if (!classContainsFunctions)
            return;

        bool isHeaderFile = false;
        const FilePath cppFileName = correspondingHeaderOrSource(interface.filePath(), &isHeaderFile);
        if (isHeaderFile && !cppFileName.isEmpty()) {
            result << new MoveAllFuncDefOutsideOp(interface,
                                                  MoveFuncDefRefactoringHelper::MoveToCppFile,
                                                  classAST, cppFileName);
        }
        result << new MoveAllFuncDefOutsideOp(interface, MoveFuncDefRefactoringHelper::MoveOutside,
                                              classAST, FilePath());
    }
};

//! Moves the definition of a function to its declaration, with the cursor on the definition.
class MoveFuncDefToDeclPush : public CppQuickFixFactory
{
public:
#ifdef WITH_TESTS
    static QObject *createTest();
#endif

private:
    void doMatch(const CppQuickFixInterface &interface, QuickFixOperations &result) override
    {
        const QList<AST *> &path = interface.path();
        AST *completeDefAST = nullptr;
        FunctionDefinitionAST *funcAST = nullptr;

        const int pathSize = path.size();
        for (int idx = 1; idx < pathSize; ++idx) {
            if ((funcAST = path.at(idx)->asFunctionDefinition())) {
                AST *enclosingAST = path.at(idx - 1);
                if (enclosingAST->asClassSpecifier())
                    return;

                // check cursor position
                if (idx != pathSize - 1  // Do not allow "void a() @ {..."
                    && funcAST->function_body
                    && !interface.isCursorOn(funcAST->function_body)) {
                    completeDefAST = enclosingAST->asTemplateDeclaration() ? enclosingAST : funcAST;
                    break;
                }
                funcAST = nullptr;
            }
        }

        if (!funcAST || !funcAST->symbol)
            return;

        const CppRefactoringChanges refactoring(interface.snapshot());
        const CppRefactoringFilePtr defFile = interface.currentFile();
        const ChangeSet::Range defRange = defFile->range(completeDefAST);

        // Determine declaration (file, range, text);
        ChangeSet::Range declRange;
        QString declText;
        FilePath declFilePath;

        Function *func = funcAST->symbol;
        if (Class *matchingClass = isMemberFunction(interface.context(), func)) {
            // Dealing with member functions
            const QualifiedNameId *qName = func->name()->asQualifiedNameId();
            for (Symbol *symbol = matchingClass->find(qName->identifier());
                 symbol; symbol = symbol->next()) {
                Symbol *s = symbol;
                if (func->enclosingScope()->asTemplate()) {
                    if (const Template *templ = s->type()->asTemplateType()) {
                        if (Symbol *decl = templ->declaration()) {
                            if (decl->type()->asFunctionType())
                                s = decl;
                        }
                    }
                }
                if (!s->name()
                    || !qName->identifier()->match(s->identifier())
                    || !s->type()->asFunctionType()
                    || !s->type().match(func->type())
                    || s->asFunction()) {
                    continue;
                }

                declFilePath = matchingClass->filePath();
                const CppRefactoringFilePtr declFile = refactoring.cppFile(declFilePath);
                ASTPath astPath(declFile->cppDocument());
                const QList<AST *> path = astPath(s->line(), s->column());
                for (int idx = path.size() - 1; idx > 0; --idx) {
                    AST *node = path.at(idx);
                    if (SimpleDeclarationAST *simpleDecl = node->asSimpleDeclaration()) {
                        if (simpleDecl->symbols && !simpleDecl->symbols->next) {
                            declRange = declFile->range(simpleDecl);
                            declText = declFile->textOf(simpleDecl);
                            declText.remove(-1, 1); // remove ';' from declaration text
                            break;
                        }
                    }
                }

                if (!declText.isEmpty())
                    break;
            }
        } else if (Namespace *matchingNamespace = isNamespaceFunction(interface.context(), func)) {
            // Dealing with free functions
            bool isHeaderFile = false;
            declFilePath = correspondingHeaderOrSource(interface.filePath(), &isHeaderFile);
            if (isHeaderFile)
                return;

            const CppRefactoringFilePtr declFile = refactoring.cppFile(declFilePath);
            const LookupContext lc(declFile->cppDocument(), interface.snapshot());
            const QList<LookupItem> candidates = lc.lookup(func->name(), matchingNamespace);
            for (const LookupItem &candidate : candidates) {
                if (Symbol *s = candidate.declaration()) {
                    if (s->asDeclaration()) {
                        ASTPath astPath(declFile->cppDocument());
                        const QList<AST *> path = astPath(s->line(), s->column());
                        for (AST *node : path) {
                            if (SimpleDeclarationAST *simpleDecl = node->asSimpleDeclaration()) {
                                declRange = declFile->range(simpleDecl);
                                declText = declFile->textOf(simpleDecl);
                                declText.remove(-1, 1); // remove ';' from declaration text
                                break;
                            }
                        }
                    }
                }

                if (!declText.isEmpty()) {
                    declText.prepend(inlinePrefix(declFilePath));
                    break;
                }
            }
        }

        if (!declFilePath.isEmpty() && !declText.isEmpty())
            result << new MoveFuncDefToDeclOp(interface,
                                              interface.filePath(),
                                              declFilePath,
                                              funcAST, func, declText,
                                              defRange, declRange, MoveFuncDefToDeclOp::Push);
    }
};

//! Moves the definition of a function to its declaration, with the cursor on the declaration.
class MoveFuncDefToDeclPull : public CppQuickFixFactory
{
public:
#ifdef WITH_TESTS
    static QObject *createTest();
#endif

private:
    void doMatch(const CppQuickFixInterface &interface, QuickFixOperations &result) override
    {
        const QList<AST *> &path = interface.path();
        for (auto it = std::rbegin(path); it != std::rend(path); ++it) {
            SimpleDeclarationAST * const simpleDecl = (*it)->asSimpleDeclaration();
            if (!simpleDecl)
                continue;
            const auto prev = std::next(it);
            if (prev != std::rend(path) && (*prev)->asStatement())
                return;
            if (!simpleDecl->symbols || !simpleDecl->symbols->value || simpleDecl->symbols->next)
                return;
            Declaration * const decl = simpleDecl->symbols->value->asDeclaration();
            if (!decl)
                return;
            Function * const funcDecl = decl->type()->asFunctionType();
            if (!funcDecl)
                return;
            if (funcDecl->isSignal() || funcDecl->isPureVirtual() || funcDecl->isFriend())
                return;

            // Is there a definition in the same product?
            Project * const declProject = ProjectManager::projectForFile(funcDecl->filePath());
            const ProjectNode * const declProduct
                = declProject ? declProject->productNodeForFilePath(funcDecl->filePath()) : nullptr;

            SymbolFinder symbolFinder;
            const QList<Function *> defs
                = symbolFinder.findMatchingDefinitions(decl, interface.snapshot(), true, false);
            Function *funcDef = nullptr;
            for (Function * const f : defs) {
                Project * const defProject = ProjectManager::projectForFile(f->filePath());
                if (defProject == declProject) {
                    if (!declProduct) {
                        funcDef = f;
                        break;
                    }
                    const ProjectNode * const defProduct
                        = defProject ? defProject->productNodeForFilePath(f->filePath()) : nullptr;
                    if (!defProduct || declProduct == defProduct)
                        funcDef = f;
                    break;
                }
            }
            if (!funcDef)
                return;

            QString declText = interface.currentFile()->textOf(simpleDecl);
            declText.chop(1); // semicolon
            declText.prepend(inlinePrefix(interface.filePath(), [funcDecl] {
                return !funcDecl->enclosingScope()->asClass();
            }));
            result << new MoveFuncDefToDeclOp(interface, funcDef->filePath(), decl->filePath(), nullptr,
                                              funcDef, declText, {},
                                              interface.currentFile()->range(simpleDecl),
                                              MoveFuncDefToDeclOp::Pull);
            return;
        }
    }
};

#ifdef WITH_TESTS
using namespace Tests;

class MoveFuncDefOutsideTest : public QObject
{
    Q_OBJECT

private slots:
    /// Check: Move definition from header to cpp.
    void testMemberFuncToCpp()
    {
        QList<TestDocumentPtr> testDocuments;
        QByteArray original;
        QByteArray expected;

        // Header File
        original =
            "class Foo {\n"
            "  inline int numbe@r() const\n"
            "  {\n"
            "    return 5;\n"
            "  }\n"
            "\n"
            "    void bar();\n"
            "};\n";
        expected =
            "class Foo {\n"
            "  int number() const;\n"
            "\n"
            "    void bar();\n"
            "};\n";
        testDocuments << CppTestDocument::create("file.h", original, expected);

        // Source File
        original =
            "#include \"file.h\"\n";
        expected =
            "#include \"file.h\"\n"
            "\n"
            "int Foo::number() const\n"
            "{\n"
            "    return 5;\n"
            "}\n"
            ;
        testDocuments << CppTestDocument::create("file.cpp", original, expected);

        MoveFuncDefOutside factory;
        QuickFixOperationTest(testDocuments, &factory);
    }

    void testMemberFuncToCppInsideNS()
    {
        QList<TestDocumentPtr> testDocuments;
        QByteArray original;
        QByteArray expected;

        // Header File
        original =
            "namespace SomeNamespace {\n"
            "class Foo {\n"
            "  int ba@r()\n"
            "  {\n"
            "    return 5;\n"
            "  }\n"
            "};\n"
            "}\n";
        expected =
            "namespace SomeNamespace {\n"
            "class Foo {\n"
            "  int ba@r();\n"
            "};\n"
            "}\n";
        testDocuments << CppTestDocument::create("file.h", original, expected);

        // Source File
        original =
            "#include \"file.h\"\n"
            "namespace SomeNamespace {\n"
            "\n"
            "}\n";
        expected =
            "#include \"file.h\"\n"
            "namespace SomeNamespace {\n"
            "\n"
            "int Foo::bar()\n"
            "{\n"
            "    return 5;\n"
            "}\n"
            "\n"
            "}\n";
        testDocuments << CppTestDocument::create("file.cpp", original, expected);

        MoveFuncDefOutside factory;
        QuickFixOperationTest(testDocuments, &factory);
    }

    /// Check: Move definition outside class
    void testMemberFuncOutside1()
    {
        QByteArray original =
            "class Foo {\n"
            "    void f1();\n"
            "    inline int f2@() const\n"
            "    {\n"
            "        return 1;\n"
            "    }\n"
            "    void f3();\n"
            "    void f4();\n"
            "};\n"
            "\n"
            "void Foo::f4() {}\n";
        QByteArray expected =
            "class Foo {\n"
            "    void f1();\n"
            "    int f2@() const;\n"
            "    void f3();\n"
            "    void f4();\n"
            "};\n"
            "\n"
            "int Foo::f2() const\n"
            "{\n"
            "    return 1;\n"
            "}\n"
            "\n"
            "void Foo::f4() {}\n";

        MoveFuncDefOutside factory;
        QuickFixOperationTest(singleDocument(original, expected), &factory);
    }

    /// Check: Move definition outside class
    void testMemberFuncOutside2()
    {
        QList<TestDocumentPtr> testDocuments;
        QByteArray original;
        QByteArray expected;

        // Header File
        original =
            "class Foo {\n"
            "    void f1();\n"
            "    int f2@()\n"
            "    {\n"
            "        return 1;\n"
            "    }\n"
            "    void f3();\n"
            "};\n";
        expected =
            "class Foo {\n"
            "    void f1();\n"
            "    int f2();\n"
            "    void f3();\n"
            "};\n"
            "\n"
            "inline int Foo::f2()\n"
            "{\n"
            "    return 1;\n"
            "}\n";
        testDocuments << CppTestDocument::create("file.h", original, expected);

        // Source File
        original =
            "#include \"file.h\"\n"
            "void Foo::f1() {}\n"
            "void Foo::f3() {}\n";
        expected = original;
        testDocuments << CppTestDocument::create("file.cpp", original, expected);

        MoveFuncDefOutside factory;
        QuickFixOperationTest(testDocuments, &factory, ProjectExplorer::HeaderPaths(), 1);
    }

    /// Check: Move definition from header to cpp (with namespace).
    void testMemberFuncToCppNS()
    {
        QList<TestDocumentPtr> testDocuments;
        QByteArray original;
        QByteArray expected;

        // Header File
        original =
            "namespace MyNs {\n"
            "class Foo {\n"
            "  inline int numbe@r() const\n"
            "  {\n"
            "    return 5;\n"
            "  }\n"
            "};\n"
            "}\n";
        expected =
            "namespace MyNs {\n"
            "class Foo {\n"
            "  int number() const;\n"
            "};\n"
            "}\n";
        testDocuments << CppTestDocument::create("file.h", original, expected);

        // Source File
        original =
            "#include \"file.h\"\n";
        expected =
            "#include \"file.h\"\n"
            "\n"
            "int MyNs::Foo::number() const\n"
            "{\n"
            "    return 5;\n"
            "}\n";
        testDocuments << CppTestDocument::create("file.cpp", original, expected);

        MoveFuncDefOutside factory;
        QuickFixOperationTest(testDocuments, &factory);
    }

    /// Check: Move definition from header to cpp (with namespace + using).
    void testMemberFuncToCppNSUsing()
    {
        QList<TestDocumentPtr> testDocuments;
        QByteArray original;
        QByteArray expected;

        // Header File
        original =
            "namespace MyNs {\n"
            "class Foo {\n"
            "  inline int numbe@r() const\n"
            "  {\n"
            "    return 5;\n"
            "  }\n"
            "};\n"
            "}\n";
        expected =
            "namespace MyNs {\n"
            "class Foo {\n"
            "  int number() const;\n"
            "};\n"
            "}\n";
        testDocuments << CppTestDocument::create("file.h", original, expected);

        // Source File
        original =
            "#include \"file.h\"\n"
            "using namespace MyNs;\n";
        expected =
            "#include \"file.h\"\n"
            "using namespace MyNs;\n"
            "\n"
            "int Foo::number() const\n"
            "{\n"
            "    return 5;\n"
            "}\n";
        testDocuments << CppTestDocument::create("file.cpp", original, expected);

        MoveFuncDefOutside factory;
        QuickFixOperationTest(testDocuments, &factory);
    }

    /// Check: Move definition outside class with Namespace
    void testMemberFuncOutsideWithNs()
    {
        QByteArray original =
            "namespace MyNs {\n"
            "class Foo {\n"
            "    inline int numbe@r() const\n"
            "    {\n"
            "        return 5;\n"
            "    }\n"
            "};}\n";
        QByteArray expected =
            "namespace MyNs {\n"
            "class Foo {\n"
            "    int number() const;\n"
            "};\n"
            "\n"
            "int Foo::number() const\n"
            "{\n"
            "    return 5;\n"
            "}\n"
            "\n}\n";

        MoveFuncDefOutside factory;
        QuickFixOperationTest(singleDocument(original, expected), &factory);
    }

    /// Check: Move free function from header to cpp.
    void testFreeFuncToCpp()
    {
        QList<TestDocumentPtr> testDocuments;
        QByteArray original;
        QByteArray expected;

        // Header File
        original =
            "int numbe@r() const\n"
            "{\n"
            "    return 5;\n"
            "}\n";
        expected =
            "int number() const;\n"
            ;
        testDocuments << CppTestDocument::create("file.h", original, expected);

        // Source File
        original =
            "#include \"file.h\"\n";
        expected =
            "#include \"file.h\"\n"
            "\n"
            "int number() const\n"
            "{\n"
            "    return 5;\n"
            "}\n";
        testDocuments << CppTestDocument::create("file.cpp", original, expected);

        MoveFuncDefOutside factory;
        QuickFixOperationTest(testDocuments, &factory);
    }

    /// Check: Move free function from header to cpp (with namespace).
    void testFreeFuncToCppNS()
    {
        QList<TestDocumentPtr> testDocuments;
        QByteArray original;
        QByteArray expected;

        // Header File
        original =
            "namespace MyNamespace {\n"
            "int numbe@r() const\n"
            "{\n"
            "    return 5;\n"
            "}\n"
            "}\n";
        expected =
            "namespace MyNamespace {\n"
            "int number() const;\n"
            "}\n";
        testDocuments << CppTestDocument::create("file.h", original, expected);

        // Source File
        original =
            "#include \"file.h\"\n";
        expected =
            "#include \"file.h\"\n"
            "\n"
            "int MyNamespace::number() const\n"
            "{\n"
            "    return 5;\n"
            "}\n";
        testDocuments << CppTestDocument::create("file.cpp", original, expected);

        MoveFuncDefOutside factory;
        QuickFixOperationTest(testDocuments, &factory);
    }

    /// Check: Move Ctor with member initialization list (QTCREATORBUG-9157).
    void testCtorWithInitialization1()
    {
        QList<TestDocumentPtr> testDocuments;
        QByteArray original;
        QByteArray expected;

        // Header File
        original =
            "class Foo {\n"
            "public:\n"
            "    Fo@o() : a(42), b(3.141) {}\n"
            "private:\n"
            "    int a;\n"
            "    float b;\n"
            "};\n";
        expected =
            "class Foo {\n"
            "public:\n"
            "    Foo();\n"
            "private:\n"
            "    int a;\n"
            "    float b;\n"
            "};\n";
        testDocuments << CppTestDocument::create("file.h", original, expected);

        // Source File
        original ="#include \"file.h\"\n";
        expected =
            "#include \"file.h\"\n"
            "\n"
            "Foo::Foo() : a(42), b(3.141) {}\n"
            ;
        testDocuments << CppTestDocument::create("file.cpp", original, expected);

        MoveFuncDefOutside factory;
        QuickFixOperationTest(testDocuments, &factory);
    }

    /// Check: Move Ctor with member initialization list (QTCREATORBUG-9462).
    void testCtorWithInitialization2()
    {
        QList<TestDocumentPtr> testDocuments;
        QByteArray original;
        QByteArray expected;

        // Header File
        original =
            "class Foo\n"
            "{\n"
            "public:\n"
            "    Fo@o() : member(2)\n"
            "    {\n"
            "    }\n"
            "\n"
            "    int member;\n"
            "};\n";

        expected =
            "class Foo\n"
            "{\n"
            "public:\n"
            "    Foo();\n"
            "\n"
            "    int member;\n"
            "};\n";
        testDocuments << CppTestDocument::create("file.h", original, expected);

        // Source File
        original ="#include \"file.h\"\n";
        expected =
            "#include \"file.h\"\n"
            "\n"
            "Foo::Foo() : member(2)\n"
            "{\n"
            "}\n"
            ;
        testDocuments << CppTestDocument::create("file.cpp", original, expected);

        MoveFuncDefOutside factory;
        QuickFixOperationTest(testDocuments, &factory);
    }

    /// Check if definition is inserted right after class for move definition outside
    void testAfterClass()
    {
        QList<TestDocumentPtr> testDocuments;
        QByteArray original;
        QByteArray expected;

        // Header File
        original =
            "class Foo\n"
            "{\n"
            "    Foo();\n"
            "    void a@() {}\n"
            "};\n"
            "\n"
            "class Bar {};\n";
        expected =
            "class Foo\n"
            "{\n"
            "    Foo();\n"
            "    void a();\n"
            "};\n"
            "\n"
            "inline void Foo::a() {}\n"
            "\n"
            "class Bar {};\n";
        testDocuments << CppTestDocument::create("file.h", original, expected);

        // Source File
        original =
            "#include \"file.h\"\n"
            "\n"
            "Foo::Foo()\n"
            "{\n\n"
            "}\n";
        expected = original;
        testDocuments << CppTestDocument::create("file.cpp", original, expected);

        MoveFuncDefOutside factory;
        QuickFixOperationTest(testDocuments, &factory, ProjectExplorer::HeaderPaths(), 1);
    }

    /// Check if whitespace is respected for operator functions
    void testRespectWsInOperatorNames1()
    {
        QByteArray original =
            "class Foo\n"
            "{\n"
            "    Foo &opera@tor =() {}\n"
            "};\n";
        QByteArray expected =
            "class Foo\n"
            "{\n"
            "    Foo &operator =();\n"
            "};\n"
            "\n"
            "Foo &Foo::operator =() {}\n"
            ;

        MoveFuncDefOutside factory;
        QuickFixOperationTest(singleDocument(original, expected), &factory);
    }

    /// Check if whitespace is respected for operator functions
    void testRespectWsInOperatorNames2()
    {
        QByteArray original =
            "class Foo\n"
            "{\n"
            "    Foo &opera@tor=() {}\n"
            "};\n";
        QByteArray expected =
            "class Foo\n"
            "{\n"
            "    Foo &operator=();\n"
            "};\n"
            "\n"
            "Foo &Foo::operator=() {}\n"
            ;

        MoveFuncDefOutside factory;
        QuickFixOperationTest(singleDocument(original, expected), &factory);
    }

    void testMacroUses()
    {
        QByteArray original =
            "#define CONST const\n"
            "#define VOLATILE volatile\n"
            "class Foo\n"
            "{\n"
            "    int fu@nc(int a, int b) CONST VOLATILE\n"
            "    {\n"
            "        return 42;\n"
            "    }\n"
            "};\n";
        QByteArray expected =
            "#define CONST const\n"
            "#define VOLATILE volatile\n"
            "class Foo\n"
            "{\n"
            "    int func(int a, int b) CONST VOLATILE;\n"
            "};\n"
            "\n"
            "\n"
            // const volatile become lowercase: QTCREATORBUG-12620
            "int Foo::func(int a, int b) const volatile\n"
            "{\n"
            "    return 42;\n"
            "}\n"
            ;

        MoveFuncDefOutside factory;
        QuickFixOperationTest(singleDocument(original, expected), &factory,
                              ProjectExplorer::HeaderPaths(), 0, "QTCREATORBUG-12314");
    }

    void testTemplate()
    {
        QByteArray original =
            "template<class T>\n"
            "class Foo { void fu@nc() {} };\n";
        QByteArray expected =
            "template<class T>\n"
            "class Foo { void fu@nc(); };\n"
            "\n"
            "template<class T>\n"
            "void Foo<T>::func() {}\n";
        ;

        MoveFuncDefOutside factory;
        QuickFixOperationTest(singleDocument(original, expected), &factory);
    }

    void testMemberFunctionTemplate()
    {
        const QByteArray original = R"(
struct S {
    template<typename In>
    void @foo(In in) { (void)in; }
};
)";
        const QByteArray expected = R"(
struct S {
    template<typename In>
    void foo(In in);
};

template<typename In>
void S::foo(In in) { (void)in; }
)";

        MoveFuncDefOutside factory;
        QuickFixOperationTest(singleDocument(original, expected), &factory);
    }

    void testTemplateSpecializedClass()
    {
        QByteArray original = R"(
template<typename T> class base {};
template<>
class base<int>
{
public:
    void @bar() {}
};
)";
        QByteArray expected = R"(
template<typename T> class base {};
template<>
class base<int>
{
public:
    void bar();
};

void base<int>::bar() {}
)";

        MoveFuncDefOutside factory;
        QuickFixOperationTest(singleDocument(original, expected), &factory);
    }

    void testUnnamedTemplate()
    {
        QByteArray original =
            "template<typename T, typename>\n"
            "class Foo { void fu@nc() {} };\n";
        QByteArray expected =
            "template<typename T, typename>\n"
            "class Foo { void fu@nc(); };\n"
            "\n"
            "template<typename T, typename T2>\n"
            "void Foo<T, T2>::func() {}\n";
        ;

        MoveFuncDefOutside factory;
        QuickFixOperationTest(singleDocument(original, expected), &factory);
    }

    void testMemberFuncToCppStatic()
    {
        QList<TestDocumentPtr> testDocuments;
        QByteArray original;
        QByteArray expected;

        // Header File
        original =
            "class Foo {\n"
            "  static inline int numbe@r() const\n"
            "  {\n"
            "    return 5;\n"
            "  }\n"
            "\n"
            "    void bar();\n"
            "};\n";
        expected =
            "class Foo {\n"
            "  static int number() const;\n"
            "\n"
            "    void bar();\n"
            "};\n";
        testDocuments << CppTestDocument::create("file.h", original, expected);

        // Source File
        original =
            "#include \"file.h\"\n";
        expected =
            "#include \"file.h\"\n"
            "\n"
            "int Foo::number() const\n"
            "{\n"
            "    return 5;\n"
            "}\n";
        testDocuments << CppTestDocument::create("file.cpp", original, expected);

        MoveFuncDefOutside factory;
        QuickFixOperationTest(testDocuments, &factory);
    }

    void testMemberFuncToCppWithInlinePartOfName()
    {
        QList<TestDocumentPtr> testDocuments;
        QByteArray original;
        QByteArray expected;

        // Header File
        original =
            "class Foo {\n"
            "  static inline int numbe@r_inline () const\n"
            "  {\n"
            "    return 5;\n"
            "  }\n"
            "\n"
            "    void bar();\n"
            "};\n";
        expected =
            "class Foo {\n"
            "  static int number_inline () const;\n"
            "\n"
            "    void bar();\n"
            "};\n";
        testDocuments << CppTestDocument::create("file.h", original, expected);

        // Source File
        original =
            "#include \"file.h\"\n";
        expected =
            "#include \"file.h\"\n"
            "\n"
            "int Foo::number_inline() const\n"
            "{\n"
            "    return 5;\n"
            "}\n";
        testDocuments << CppTestDocument::create("file.cpp", original, expected);

        MoveFuncDefOutside factory;
        QuickFixOperationTest(testDocuments, &factory);
    }

    void testMixedQualifiers()
    {
        QList<TestDocumentPtr> testDocuments;
        QByteArray original;
        QByteArray expected;

        // Header File
        original = R"(
struct Base {
    virtual auto func() const && noexcept -> void = 0;
};
struct Derived : public Base {
    auto @func() const && noexcept -> void override {}
};)";
        expected = R"(
struct Base {
    virtual auto func() const && noexcept -> void = 0;
};
struct Derived : public Base {
    auto func() const && noexcept -> void override;
};)";
        testDocuments << CppTestDocument::create("file.h", original, expected);

        // Source File
        original = "#include \"file.h\"\n";
        expected = R"DELIM(#include "file.h"

auto Derived::func() const && noexcept -> void {}
)DELIM";
        testDocuments << CppTestDocument::create("file.cpp", original, expected);

        MoveFuncDefOutside factory;
        QuickFixOperationTest(testDocuments, &factory);
    }

};

class MoveAllFuncDefOutsideTest : public QObject
{
    Q_OBJECT

private slots:
    void testMemberFuncToCpp()
    {
        QList<TestDocumentPtr> testDocuments;
        QByteArray original;
        QByteArray expected;

        // Header File
        original =
            "class Foo {@\n"
            "  int numberA() const\n"
            "  {\n"
            "    return 5;\n"
            "  }\n"
            "  int numberB() const\n"
            "  {\n"
            "    return 5;\n"
            "  }\n"
            "};\n";
        expected =
            "class Foo {\n"
            "  int numberA() const;\n"
            "  int numberB() const;\n"
            "};\n";
        testDocuments << CppTestDocument::create("file.h", original, expected);

        // Source File
        original =
            "#include \"file.h\"\n";
        expected =
            "#include \"file.h\"\n"
            "\n"
            "int Foo::numberA() const\n"
            "{\n"
            "    return 5;\n"
            "}\n"
            "\n"
            "int Foo::numberB() const\n"
            "{\n"
            "    return 5;\n"
            "}\n"
            ;
        testDocuments << CppTestDocument::create("file.cpp", original, expected);

        MoveAllFuncDefOutside factory;
        QuickFixOperationTest(testDocuments, &factory);
    }

    void testMemberFuncOutside()
    {
        QByteArray original =
            "class F@oo {\n"
            "    int f1()\n"
            "    {\n"
            "        return 1;\n"
            "    }\n"
            "    int f2() const\n"
            "    {\n"
            "        return 2;\n"
            "    }\n"
            "};\n";
        QByteArray expected =
            "class Foo {\n"
            "    int f1();\n"
            "    int f2() const;\n"
            "};\n"
            "\n"
            "int Foo::f1()\n"
            "{\n"
            "    return 1;\n"
            "}\n"
            "\n"
            "int Foo::f2() const\n"
            "{\n"
            "    return 2;\n"
            "}\n";

        MoveAllFuncDefOutside factory;
        QuickFixOperationTest(singleDocument(original, expected), &factory);
    }

    void testDoNotTriggerOnBaseClass()
    {
        QByteArray original =
            "class Bar;\n"
            "class Foo : public Ba@r {\n"
            "    int f1()\n"
            "    {\n"
            "        return 1;\n"
            "    }\n"
            "};\n";

        MoveAllFuncDefOutside factory;
        QuickFixOperationTest(singleDocument(original, ""), &factory);
    }

    void testClassWithBaseClass()
    {
        QByteArray original =
            "class Bar;\n"
            "class Fo@o : public Bar {\n"
            "    int f1()\n"
            "    {\n"
            "        return 1;\n"
            "    }\n"
            "};\n";
        QByteArray expected =
            "class Bar;\n"
            "class Foo : public Bar {\n"
            "    int f1();\n"
            "};\n"
            "\n"
            "int Foo::f1()\n"
            "{\n"
            "    return 1;\n"
            "}\n";

        MoveAllFuncDefOutside factory;
        QuickFixOperationTest(singleDocument(original, expected), &factory);
    }

    /// Check: Do not take macro expanded code into account (QTCREATORBUG-13900)
    void testIgnoreMacroCode()
    {
        QByteArray original =
            "#define FAKE_Q_OBJECT int bar() {return 5;}\n"
            "class Fo@o {\n"
            "    FAKE_Q_OBJECT\n"
            "    int f1()\n"
            "    {\n"
            "        return 1;\n"
            "    }\n"
            "};\n";
        QByteArray expected =
            "#define FAKE_Q_OBJECT int bar() {return 5;}\n"
            "class Foo {\n"
            "    FAKE_Q_OBJECT\n"
            "    int f1();\n"
            "};\n"
            "\n"
            "int Foo::f1()\n"
            "{\n"
            "    return 1;\n"
            "}\n";

        MoveAllFuncDefOutside factory;
        QuickFixOperationTest(singleDocument(original, expected), &factory);
    }

};

class MoveFuncDefToDeclTest : public QObject
{
    Q_OBJECT

private slots:
    void test_data()
    {
        QTest::addColumn<QByteArrayList>("headers");
        QTest::addColumn<QByteArrayList>("sources");

        QByteArray originalHeader;
        QByteArray expectedHeader;
        QByteArray originalSource;
        QByteArray expectedSource;

        originalHeader =
            "class Foo {\n"
            "    inline int @number() const;\n"
            "};\n";
        expectedHeader =
            "class Foo {\n"
            "    inline int number() const {return 5;}\n"
            "};\n";
        originalSource =
            "#include \"file.h\"\n"
            "\n"
            "int Foo::num@ber() const {return 5;}\n";
        expectedSource =
            "#include \"file.h\"\n"
            "\n\n";
        QTest::newRow("member function, two files") << QByteArrayList{originalHeader, expectedHeader}
                                                    << QByteArrayList{originalSource, expectedSource};

        originalSource =
            "class Foo {\n"
            "  inline int @number() const;\n"
            "};\n"
            "\n"
            "int Foo::num@ber() const\n"
            "{\n"
            "    return 5;\n"
            "}\n";

        expectedSource =
            "class Foo {\n"
            "    inline int number() const\n"
            "    {\n"
            "        return 5;\n"
            "    }\n"
            "};\n\n\n";
        QTest::newRow("member function, one file") << QByteArrayList()
                                                   << QByteArrayList{originalSource, expectedSource};

        originalHeader =
            "namespace MyNs {\n"
            "class Foo {\n"
            "  inline int @number() const;\n"
            "};\n"
            "}\n";
        expectedHeader =
            "namespace MyNs {\n"
            "class Foo {\n"
            "    inline int number() const\n"
            "    {\n"
            "        return 5;\n"
            "    }\n"
            "};\n"
            "}\n";
        originalSource =
            "#include \"file.h\"\n"
            "\n"
            "int MyNs::Foo::num@ber() const\n"
            "{\n"
            "    return 5;\n"
            "}\n";
        expectedSource = "#include \"file.h\"\n\n\n";
        QTest::newRow("member function, two files, namespace")
            << QByteArrayList{originalHeader, expectedHeader}
            << QByteArrayList{originalSource, expectedSource};

        originalHeader =
            "namespace MyNs {\n"
            "class Foo {\n"
            "  inline int numbe@r() const;\n"
            "};\n"
            "}\n";
        expectedHeader =
            "namespace MyNs {\n"
            "class Foo {\n"
            "    inline int number() const\n"
            "    {\n"
            "        return 5;\n"
            "    }\n"
            "};\n"
            "}\n";
        originalSource =
            "#include \"file.h\"\n"
            "using namespace MyNs;\n"
            "\n"
            "int Foo::num@ber() const\n"
            "{\n"
            "    return 5;\n"
            "}\n";
        expectedSource =
            "#include \"file.h\"\n"
            "using namespace MyNs;\n"
            "\n\n";
        QTest::newRow("member function, two files, namespace with using-directive")
            << QByteArrayList{originalHeader, expectedHeader}
            << QByteArrayList{originalSource, expectedSource};

        originalSource =
            "namespace MyNs {\n"
            "class Foo {\n"
            "  inline int @number() const;\n"
            "};\n"
            "\n"
            "int Foo::numb@er() const\n"
            "{\n"
            "    return 5;\n"
            "}"
            "\n}\n";
        expectedSource =
            "namespace MyNs {\n"
            "class Foo {\n"
            "    inline int number() const\n"
            "    {\n"
            "        return 5;\n"
            "    }\n"
            "};\n\n\n}\n";

        QTest::newRow("member function, one file, namespace")
            << QByteArrayList() << QByteArrayList{originalSource, expectedSource};

        originalHeader = "int nu@mber() const;\n";
        expectedHeader =
            "inline int number() const\n"
            "{\n"
            "    return 5;\n"
            "}\n";
        originalSource =
            "#include \"file.h\"\n"
            "\n"
            "\n"
            "int numb@er() const\n"
            "{\n"
            "    return 5;\n"
            "}\n";
        expectedSource = "#include \"file.h\"\n\n\n\n";
        QTest::newRow("free function") << QByteArrayList{originalHeader, expectedHeader}
                                       << QByteArrayList{originalSource, expectedSource};

        originalHeader =
            "namespace MyNamespace {\n"
            "int n@umber() const;\n"
            "}\n";
        expectedHeader =
            "namespace MyNamespace {\n"
            "inline int number() const\n"
            "{\n"
            "    return 5;\n"
            "}\n"
            "}\n";
        originalSource =
            "#include \"file.h\"\n"
            "\n"
            "int MyNamespace::nu@mber() const\n"
            "{\n"
            "    return 5;\n"
            "}\n";
        expectedSource =
            "#include \"file.h\"\n"
            "\n\n";
        QTest::newRow("free function, namespace") << QByteArrayList{originalHeader, expectedHeader}
                                                  << QByteArrayList{originalSource, expectedSource};

        originalHeader =
            "class Foo {\n"
            "public:\n"
            "    Fo@o();\n"
            "private:\n"
            "    int a;\n"
            "    float b;\n"
            "};\n";
        expectedHeader =
            "class Foo {\n"
            "public:\n"
            "    Foo() : a(42), b(3.141) {}\n"
            "private:\n"
            "    int a;\n"
            "    float b;\n"
            "};\n";
        originalSource =
            "#include \"file.h\"\n"
            "\n"
            "Foo::F@oo() : a(42), b(3.141) {}"
            ;
        expectedSource ="#include \"file.h\"\n\n";
        QTest::newRow("constructor") << QByteArrayList{originalHeader, expectedHeader}
                                     << QByteArrayList{originalSource, expectedSource};

        originalSource =
            "struct Foo\n"
            "{\n"
            "    void f@oo();\n"
            "} bar;\n"
            "void Foo::fo@o()\n"
            "{\n"
            "    return;\n"
            "}";
        expectedSource =
            "struct Foo\n"
            "{\n"
            "    void foo()\n"
            "    {\n"
            "        return;\n"
            "    }\n"
            "} bar;\n";
        QTest::newRow("QTCREATORBUG-10303") << QByteArrayList()
                                            << QByteArrayList{originalSource, expectedSource};

        originalSource =
            "struct Base {\n"
            "    virtual int foo() = 0;\n"
            "};\n"
            "struct Derived : Base {\n"
            "    int @foo() override;\n"
            "};\n"
            "\n"
            "int Derived::fo@o()\n"
            "{\n"
            "    return 5;\n"
            "}\n";
        expectedSource =
            "struct Base {\n"
            "    virtual int foo() = 0;\n"
            "};\n"
            "struct Derived : Base {\n"
            "    int foo() override\n"
            "    {\n"
            "        return 5;\n"
            "    }\n"
            "};\n\n\n";
        QTest::newRow("overridden virtual") << QByteArrayList()
                                            << QByteArrayList{originalSource, expectedSource};

        originalSource =
            "template<class T>\n"
            "class Foo { void @func(); };\n"
            "\n"
            "template<class T>\n"
            "void Foo<T>::fu@nc() {}\n";
        expectedSource =
            "template<class T>\n"
            "class Foo { void fu@nc() {} };\n\n\n";
        QTest::newRow("class template") << QByteArrayList()
                                        << QByteArrayList{originalSource, expectedSource};

        originalSource =
            "class Foo\n"
            "{\n"
            "    template<class T>\n"
            "    void @func();\n"
            "};\n"
            "\n"
            "template<class T>\n"
            "void Foo::fu@nc() {}\n";
        expectedSource =
            "class Foo\n"
            "{\n"
            "    template<class T>\n"
            "    void func() {}\n"
            "};\n\n\n";
        QTest::newRow("function template") << QByteArrayList()
                                           << QByteArrayList{originalSource, expectedSource};
    }

    void test()
    {
        QFETCH(QByteArrayList, headers);
        QFETCH(QByteArrayList, sources);

        QVERIFY(headers.isEmpty() || headers.size() == 2);
        QVERIFY(sources.size() == 2);

        QByteArray &declDoc = !headers.empty() ? headers.first() : sources.first();
        const int declCursorPos = declDoc.indexOf('@');
        QVERIFY(declCursorPos != -1);
        const int defCursorPos = sources.first().lastIndexOf('@');
        QVERIFY(defCursorPos != -1);
        QVERIFY(declCursorPos != defCursorPos);

        declDoc.remove(declCursorPos, 1);
        QList<TestDocumentPtr> testDocuments;
        if (!headers.isEmpty())
            testDocuments << CppTestDocument::create("file.h", headers.first(), headers.last());
        testDocuments << CppTestDocument::create("file.cpp", sources.first(), sources.last());

        MoveFuncDefToDeclPush pushFactory;
        QuickFixOperationTest(testDocuments, &pushFactory);

        declDoc.insert(declCursorPos, '@');
        sources.first().remove(defCursorPos, 1);
        testDocuments.clear();
        if (!headers.isEmpty())
            testDocuments << CppTestDocument::create("file.h", headers.first(), headers.last());
        testDocuments << CppTestDocument::create("file.cpp", sources.first(), sources.last());

        MoveFuncDefToDeclPull pullFactory;
        QuickFixOperationTest(testDocuments, &pullFactory);
    }

    void testMacroUses()
    {
        QByteArray original =
            "#define CONST const\n"
            "#define VOLATILE volatile\n"
            "class Foo\n"
            "{\n"
            "    int func(int a, int b) CONST VOLATILE;\n"
            "};\n"
            "\n"
            "\n"
            "int Foo::fu@nc(int a, int b) CONST VOLATILE"
            "{\n"
            "    return 42;\n"
            "}\n";
        QByteArray expected =
            "#define CONST const\n"
            "#define VOLATILE volatile\n"
            "class Foo\n"
            "{\n"
            "    int func(int a, int b) CONST VOLATILE\n"
            "    {\n"
            "        return 42;\n"
            "    }\n"
            "};\n\n\n\n";

        MoveFuncDefToDeclPush factory;
        QuickFixOperationTest(singleDocument(original, expected), &factory,
                              ProjectExplorer::HeaderPaths(), 0, "QTCREATORBUG-12314");
    }
};

QObject *MoveFuncDefOutside::createTest()
{
    return new MoveFuncDefOutsideTest;
}

QObject *MoveAllFuncDefOutside::createTest()
{
    return new MoveAllFuncDefOutsideTest;
}

QObject *MoveFuncDefToDeclPush::createTest()
{
    return new MoveFuncDefToDeclTest;
}

QObject *MoveFuncDefToDeclPull::createTest()
{
    return new QObject; // The test for the push factory handled both cases.
}

#endif // WITH_TESTS

} // namespace

void registerMoveFunctionDefinitionQuickfixes()
{
    CppQuickFixFactory::registerFactory<MoveFuncDefOutside>();
    CppQuickFixFactory::registerFactory<MoveAllFuncDefOutside>();
    CppQuickFixFactory::registerFactory<MoveFuncDefToDeclPush>();
    CppQuickFixFactory::registerFactory<MoveFuncDefToDeclPull>();
}

} // namespace CppEditor::Internal

#ifdef WITH_TESTS
#include <movefunctiondefinition.moc>
#endif
