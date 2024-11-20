// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "insertfunctiondefinition.h"

#include "../cppcodestylepreferences.h"
#include "../cppcodestylesettings.h"
#include "../cppeditortr.h"
#include "../cppeditorwidget.h"
#include "../cpprefactoringchanges.h"
#include "../cpptoolssettings.h"
#include "../insertionpointlocator.h"
#include "../symbolfinder.h"
#include "cppquickfix.h"
#include "cppquickfixhelpers.h"

#include <coreplugin/icore.h>
#include <cplusplus/CppRewriter.h>
#include <cplusplus/Overview.h>
#include <projectexplorer/projectmanager.h>
#include <utils/layoutbuilder.h>

#include <QDialogButtonBox>
#include <QGridLayout>
#include <QHBoxLayout>

#ifdef WITH_TESTS
#include "cppquickfix_test.h"
#include <QtTest>
#endif

using namespace CPlusPlus;
using namespace ProjectExplorer;
using namespace TextEditor;
using namespace Utils;

namespace CppEditor::Internal {
namespace {

enum DefPos {
    DefPosInsideClass,
    DefPosOutsideClass,
    DefPosImplementationFile
};

enum class InsertDefsFromDeclsMode {
    Off,         // Testing: simulates user canceling the dialog
    Impl,        // Testing: simulates user choosing cpp file for every function
    Alternating, // Testing: simulates user choosing a different DefPos for every function
    User         // Normal interactive mode
};

class InsertDefOperation: public CppQuickFixOperation
{
public:
    // Make sure that either loc is valid or targetFileName is not empty.
    InsertDefOperation(const CppQuickFixInterface &interface,
                       Declaration *decl, DeclaratorAST *declAST, const InsertionLocation &loc,
                       const DefPos defpos, const FilePath &targetFileName = {},
                       bool freeFunction = false)
        : CppQuickFixOperation(interface, 0)
        , m_decl(decl)
        , m_declAST(declAST)
        , m_loc(loc)
        , m_defpos(defpos)
        , m_targetFilePath(targetFileName)
    {
        if (m_defpos == DefPosImplementationFile) {
            const FilePath declFile = decl->filePath();
            const FilePath targetFile =  m_loc.isValid() ? m_loc.filePath() : m_targetFilePath;
            const FilePath resolved = targetFile.relativePathFrom(declFile.parentDir());
            setPriority(2);
            setDescription(Tr::tr("Add Definition in %1").arg(resolved.displayName()));
        } else if (freeFunction) {
            setDescription(Tr::tr("Add Definition Here"));
        } else if (m_defpos == DefPosInsideClass) {
            setDescription(Tr::tr("Add Definition Inside Class"));
        } else if (m_defpos == DefPosOutsideClass) {
            setPriority(1);
            setDescription(Tr::tr("Add Definition Outside Class"));
        }
    }

    static void insertDefinition(
        const CppQuickFixOperation *op,
        InsertionLocation loc,
        DefPos defPos,
        DeclaratorAST *declAST,
        Declaration *decl,
        const FilePath &targetFilePath,
        ChangeSet *changeSet = nullptr)
    {
        CppRefactoringChanges refactoring(op->snapshot());
        if (!loc.isValid())
            loc = insertLocationForMethodDefinition(decl, true, NamespaceHandling::Ignore,
                                                    refactoring, targetFilePath);
        QTC_ASSERT(loc.isValid(), return);

        CppRefactoringFilePtr targetFile = refactoring.cppFile(loc.filePath());
        Overview oo = CppCodeStyleSettings::currentProjectCodeStyleOverview();
        oo.showFunctionSignatures = true;
        oo.showReturnTypes = true;
        oo.showArgumentNames = true;
        oo.showEnclosingTemplate = true;

        // What we really want is to show template parameters for the class, but not for the
        // function, but we cannot express that. This is an approximation that will work
        // as long as either the surrounding class or the function is not a template.
        oo.showTemplateParameters = decl->enclosingClass()
                                    && decl->enclosingClass()->enclosingTemplate();

        if (defPos == DefPosInsideClass) {
            const int targetPos = targetFile->position(loc.line(), loc.column());
            ChangeSet localChangeSet;
            ChangeSet * const target = changeSet ? changeSet : &localChangeSet;
            target->replace(targetPos - 1, targetPos, QLatin1String("\n {\n\n}")); // replace ';'

            if (!changeSet) {
                targetFile->setOpenEditor(true, targetPos);
                targetFile->apply(*target);

                // Move cursor inside definition
                QTextCursor c = targetFile->cursor();
                c.setPosition(targetPos);
                c.movePosition(QTextCursor::Down);
                c.movePosition(QTextCursor::EndOfLine);
                op->editor()->setTextCursor(c);
            }
        } else {
            // make target lookup context
            Document::Ptr targetDoc = targetFile->cppDocument();
            Scope *targetScope = targetDoc->scopeAt(loc.line(), loc.column());

            // Correct scope in case of a function try-block. See QTCREATORBUG-14661.
            if (targetScope && targetScope->asBlock()) {
                if (Class * const enclosingClass = targetScope->enclosingClass())
                    targetScope = enclosingClass;
                else
                    targetScope = targetScope->enclosingNamespace();
            }

            LookupContext targetContext(targetDoc, op->snapshot());
            ClassOrNamespace *targetCoN = targetContext.lookupType(targetScope);
            if (!targetCoN)
                targetCoN = targetContext.globalNamespace();

            // setup rewriting to get minimally qualified names
            SubstitutionEnvironment env;
            env.setContext(op->context());
            env.switchScope(decl->enclosingScope());
            UseMinimalNames q(targetCoN);
            env.enter(&q);
            Control *control = op->context().bindings()->control().get();

            // rewrite the function type
            const FullySpecifiedType tn = rewriteType(decl->type(), &env, control);

            // rewrite the function name
            if (nameIncludesOperatorName(decl->name())) {
                const QString operatorNameText = op->currentFile()->textOf(declAST->core_declarator);
                oo.includeWhiteSpaceInOperatorName = operatorNameText.contains(QLatin1Char(' '));
            }
            const QString name = oo.prettyName(LookupContext::minimalName(decl, targetCoN,
                                                                          control));

            const QString inlinePref = inlinePrefix(targetFilePath, [defPos] {
                return defPos == DefPosOutsideClass;
            });

            const QString prettyType = oo.prettyType(tn, name);

            QString input = prettyType;
            int index = 0;
            while (input.startsWith("template")) {
                static const QRegularExpression templateRegex("template\\s*<[^>]*>");
                QRegularExpressionMatch match = templateRegex.match(input);
                if (match.hasMatch()) {
                    index += match.captured().size() + 1;
                    input = input.mid(match.captured().size() + 1);
                }
            }

            QString defText = prettyType;
            defText.insert(index, inlinePref);
            defText += QLatin1String("\n{\n\n}");

            ChangeSet localChangeSet;
            ChangeSet * const target = changeSet ? changeSet : &localChangeSet;
            const int targetPos = targetFile->position(loc.line(), loc.column());
            target->insert(targetPos,  loc.prefix() + defText + loc.suffix());

            if (!changeSet) {
                targetFile->setOpenEditor(true, targetPos);
                targetFile->apply(*target);

                // Move cursor inside definition
                QTextCursor c = targetFile->cursor();
                c.setPosition(targetPos);
                c.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor,
                               loc.prefix().count(QLatin1String("\n")) + 2);
                c.movePosition(QTextCursor::EndOfLine);
                if (defPos == DefPosImplementationFile) {
                    if (targetFile->editor())
                        targetFile->editor()->setTextCursor(c);
                } else {
                    op->editor()->setTextCursor(c);
                }
            }
        }
    }

private:
    void perform() override
    {
        insertDefinition(this, m_loc, m_defpos, m_declAST, m_decl, m_targetFilePath);
    }

    Declaration *m_decl;
    DeclaratorAST *m_declAST;
    InsertionLocation m_loc;
    const DefPos m_defpos;
    const FilePath m_targetFilePath;
};

class MemberFunctionImplSetting
{
public:
    Symbol *func = nullptr;
    DefPos defPos = DefPosImplementationFile;
};
using MemberFunctionImplSettings = QList<MemberFunctionImplSetting>;

class AddImplementationsDialog : public QDialog
{
public:
    AddImplementationsDialog(const QList<Symbol *> &candidates, const FilePath &implFile)
        : QDialog(Core::ICore::dialogParent()), m_candidates(candidates)
    {
        setWindowTitle(Tr::tr("Member Function Implementations"));

        const auto defaultImplTargetComboBox = new QComboBox;
        QStringList implTargetStrings{Tr::tr("None"), Tr::tr("Inline"), Tr::tr("Outside Class")};
        if (!implFile.isEmpty())
            implTargetStrings.append(implFile.fileName());
        defaultImplTargetComboBox->insertItems(0, implTargetStrings);
        connect(defaultImplTargetComboBox, &QComboBox::currentIndexChanged, this,
                [this](int index) {
                    for (int i = 0; i < m_implTargetBoxes.size(); ++i) {
                        if (!m_candidates.at(i)->type()->asFunctionType()->isPureVirtual())
                            static_cast<QComboBox *>(m_implTargetBoxes.at(i))->setCurrentIndex(index);
                    }
                });
        const auto defaultImplTargetLayout = new QHBoxLayout;
        defaultImplTargetLayout->addWidget(new QLabel(Tr::tr("Default implementation location:")));
        defaultImplTargetLayout->addWidget(defaultImplTargetComboBox);

        const auto candidatesLayout = new QGridLayout;
        Overview oo = CppCodeStyleSettings::currentProjectCodeStyleOverview();
        oo.showFunctionSignatures = true;
        oo.showReturnTypes = true;
        for (int i = 0; i < m_candidates.size(); ++i) {
            const Function * const func = m_candidates.at(i)->type()->asFunctionType();
            QTC_ASSERT(func, continue);
            const auto implTargetComboBox = new QComboBox;
            m_implTargetBoxes.append(implTargetComboBox);
            implTargetComboBox->insertItems(0, implTargetStrings);
            if (func->isPureVirtual())
                implTargetComboBox->setCurrentIndex(0);
            candidatesLayout->addWidget(new QLabel(oo.prettyType(func->type(), func->name())),
                                        i, 0);
            candidatesLayout->addWidget(implTargetComboBox, i, 1);
        }

        const auto buttonBox
            = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

        defaultImplTargetComboBox->setCurrentIndex(implTargetStrings.size() - 1);
        const auto mainLayout = new QVBoxLayout(this);
        mainLayout->addLayout(defaultImplTargetLayout);
        mainLayout->addWidget(Layouting::createHr(this));
        mainLayout->addLayout(candidatesLayout);
        mainLayout->addWidget(buttonBox);
    }

    MemberFunctionImplSettings settings() const
    {
        QTC_ASSERT(m_candidates.size() == m_implTargetBoxes.size(), return {});
        MemberFunctionImplSettings settings;
        for (int i = 0; i < m_candidates.size(); ++i) {
            MemberFunctionImplSetting setting;
            const int index = m_implTargetBoxes.at(i)->currentIndex();
            const bool addImplementation = index != 0;
            if (!addImplementation)
                continue;
            setting.func = m_candidates.at(i);
            setting.defPos = static_cast<DefPos>(index - 1);
            settings << setting;
        }
        return settings;
    }

private:
    const QList<Symbol *> m_candidates;
    QList<QComboBox *> m_implTargetBoxes;
};

class InsertDefsOperation: public CppQuickFixOperation
{
public:
    InsertDefsOperation(const CppQuickFixInterface &interface) : CppQuickFixOperation(interface)
    {
        setDescription(Tr::tr("Create Implementations for Member Functions"));

        m_classAST = astForClassOperations(interface);
        if (!m_classAST)
            return;
        const Class * const theClass = m_classAST->symbol;
        if (!theClass)
            return;

        // Collect all member functions.
        for (auto it = theClass->memberBegin(); it != theClass->memberEnd(); ++it) {
            Symbol * const s = *it;
            if (!s->identifier() || !s->type() || !s->asDeclaration() || s->asFunction())
                continue;
            Function * const func = s->type()->asFunctionType();
            if (!func || func->isSignal() || func->isFriend())
                continue;
            Overview oo = CppCodeStyleSettings::currentProjectCodeStyleOverview();
            oo.showFunctionSignatures = true;
            if (magicQObjectFunctions().contains(oo.prettyName(func->name())))
                continue;
            m_declarations << s;
        }
    }

    bool isApplicable() const { return !m_declarations.isEmpty(); }
    void setMode(InsertDefsFromDeclsMode mode) { m_mode = mode; }

private:
    void perform() override
    {
        QList<Symbol *> unimplemented;
        SymbolFinder symbolFinder;
        for (Symbol * const s : std::as_const(m_declarations)) {
            if (!symbolFinder.findMatchingDefinition(s, snapshot(), true))
                unimplemented << s;
        }
        if (unimplemented.isEmpty())
            return;

        CppRefactoringChanges refactoring(snapshot());
        const bool isHeaderFile = ProjectFile::isHeader(ProjectFile::classify(filePath().toString()));
        FilePath cppFile; // Only set if the class is defined in a header file.
        if (isHeaderFile) {
            InsertionPointLocator locator(refactoring);
            for (const InsertionLocation &location
                 : locator.methodDefinition(unimplemented.first(), false, {})) {
                if (!location.isValid())
                    continue;
                const FilePath filePath = location.filePath();
                if (ProjectFile::isHeader(ProjectFile::classify(filePath.path()))) {
                    const FilePath source = correspondingHeaderOrSource(filePath);
                    if (!source.isEmpty())
                        cppFile = source;
                } else {
                    cppFile = filePath;
                }
                break;
            }
        }

        MemberFunctionImplSettings settings;
        switch (m_mode) {
        case InsertDefsFromDeclsMode::User: {
            AddImplementationsDialog dlg(unimplemented, cppFile);
            if (dlg.exec() == QDialog::Accepted)
                settings = dlg.settings();
            break;
        }
        case InsertDefsFromDeclsMode::Impl: {
            for (Symbol * const func : std::as_const(unimplemented)) {
                MemberFunctionImplSetting setting;
                setting.func = func;
                setting.defPos = DefPosImplementationFile;
                settings << setting;
            }
            break;
        }
        case InsertDefsFromDeclsMode::Alternating: {
            int defPos = DefPosImplementationFile;
            const auto incDefPos = [&defPos] {
                defPos = (defPos + 1) % (DefPosImplementationFile + 2);
            };
            for (Symbol * const func : std::as_const(unimplemented)) {
                incDefPos();
                if (defPos > DefPosImplementationFile)
                    continue;
                MemberFunctionImplSetting setting;
                setting.func = func;
                setting.defPos = static_cast<DefPos>(defPos);
                settings << setting;
            }
            break;
        }
        case InsertDefsFromDeclsMode::Off:
            break;
        }

        if (settings.isEmpty())
            return;

        class DeclFinder : public ASTVisitor
        {
        public:
            DeclFinder(const CppRefactoringFile *file, const Symbol *func)
                : ASTVisitor(file->cppDocument()->translationUnit()), m_func(func) {}

            SimpleDeclarationAST *decl() const { return m_decl; }

        private:
            bool visit(SimpleDeclarationAST *decl) override
            {
                if (m_decl)
                    return false;
                if (decl->symbols && decl->symbols->value == m_func)
                    m_decl = decl;
                return !m_decl;
            }

            const Symbol * const m_func;
            SimpleDeclarationAST *m_decl = nullptr;
        };

        QHash<FilePath, ChangeSet> changeSets;
        for (const MemberFunctionImplSetting &setting : std::as_const(settings)) {
            DeclFinder finder(currentFile().data(), setting.func);
            finder.accept(m_classAST);
            QTC_ASSERT(finder.decl(), continue);
            InsertionLocation loc;
            const FilePath targetFilePath = setting.defPos == DefPosImplementationFile
                                                ? cppFile : filePath();
            QTC_ASSERT(!targetFilePath.isEmpty(), continue);
            if (setting.defPos == DefPosInsideClass) {
                int line, column;
                currentFile()->lineAndColumn(currentFile()->endOf(finder.decl()), &line, &column);
                loc = InsertionLocation(filePath(), QString(), QString(), line, column);
            }
            ChangeSet &changeSet = changeSets[targetFilePath];
            InsertDefOperation::insertDefinition(
                this, loc, setting.defPos, finder.decl()->declarator_list->value,
                setting.func->asDeclaration(),targetFilePath, &changeSet);
        }
        for (auto it = changeSets.cbegin(); it != changeSets.cend(); ++it)
            refactoring.cppFile(it.key())->apply(it.value());
    }

    ClassSpecifierAST *m_classAST = nullptr;
    InsertDefsFromDeclsMode m_mode = InsertDefsFromDeclsMode::User;
    QList<Symbol *> m_declarations;
};

class InsertDefFromDecl: public CppQuickFixFactory
{
public:
#ifdef WITH_TESTS
    static QObject *createTest();
#endif

    void setOutside() { m_defPosOutsideClass = true; }

private:
    void doMatch(const CppQuickFixInterface &interface, QuickFixOperations &result) override
    {
        const QList<AST *> &path = interface.path();

        int idx = path.size() - 1;
        for (; idx >= 0; --idx) {
            AST *node = path.at(idx);
            if (SimpleDeclarationAST *simpleDecl = node->asSimpleDeclaration()) {
                if (idx > 0 && path.at(idx - 1)->asStatement())
                    return;
                if (simpleDecl->symbols && !simpleDecl->symbols->next) {
                    if (Symbol *symbol = simpleDecl->symbols->value) {
                        if (Declaration *decl = symbol->asDeclaration()) {
                            if (Function *func = decl->type()->asFunctionType()) {
                                if (func->isSignal() || func->isPureVirtual() || func->isFriend())
                                    return;

                                const Project * const declProject
                                    = ProjectManager::projectForFile(func->filePath());
                                const ProjectNode * const declProduct
                                    = declProject
                                          ? declProject->productNodeForFilePath(func->filePath())
                                          : nullptr;

                                // Check if there is already a definition in this product.
                                SymbolFinder symbolFinder;
                                const QList<Function *> defs
                                    = symbolFinder.findMatchingDefinitions(
                                        decl, interface.snapshot(), true, false);
                                for (const Function * const def : defs) {
                                    const Project *const defProject
                                        = ProjectManager::projectForFile(def->filePath());
                                    if (declProject == defProject) {
                                        if (!declProduct)
                                            return;
                                        const ProjectNode * const defProduct
                                            = defProject ? defProject->productNodeForFilePath(
                                                  def->filePath())
                                                         : nullptr;
                                        if (!defProduct || declProduct == defProduct)
                                            return;
                                    }
                                }

                                // Insert Position: Implementation File
                                DeclaratorAST *declAST = simpleDecl->declarator_list->value;
                                InsertDefOperation *op = nullptr;
                                ProjectFile::Kind kind = ProjectFile::classify(interface.filePath().toString());
                                const bool isHeaderFile = ProjectFile::isHeader(kind);
                                if (isHeaderFile) {
                                    CppRefactoringChanges refactoring(interface.snapshot());
                                    InsertionPointLocator locator(refactoring);
                                    // find appropriate implementation file, but do not use this
                                    // location, because insertLocationForMethodDefinition() should
                                    // be used in perform() to get consistent insert positions.
                                    for (const InsertionLocation &location :
                                         locator.methodDefinition(decl, false, {})) {
                                        if (!location.isValid())
                                            continue;

                                        const FilePath filePath = location.filePath();
                                        const Project * const defProject
                                            = ProjectManager::projectForFile(filePath);
                                        if (declProject != defProject)
                                            continue;
                                        if (declProduct) {
                                            const ProjectNode * const defProduct = defProject
                                                ? defProject->productNodeForFilePath(filePath)
                                                : nullptr;
                                            if (defProduct && declProduct != defProduct)
                                                continue;
                                        }

                                        if (ProjectFile::isHeader(ProjectFile::classify(filePath.path()))) {
                                            const FilePath source = correspondingHeaderOrSource(filePath);
                                            if (!source.isEmpty()) {
                                                op = new InsertDefOperation(interface, decl, declAST,
                                                                            InsertionLocation(),
                                                                            DefPosImplementationFile,
                                                                            source);
                                            }
                                        } else {
                                            op = new InsertDefOperation(interface, decl, declAST,
                                                                        InsertionLocation(),
                                                                        DefPosImplementationFile,
                                                                        filePath);
                                        }

                                        if (op)
                                            result << op;
                                        break;
                                    }
                                }

                                // Determine if we are dealing with a free function
                                const bool isFreeFunction = func->enclosingClass() == nullptr;

                                // Insert Position: Outside Class
                                if (!isFreeFunction || m_defPosOutsideClass) {
                                    result << new InsertDefOperation(interface, decl, declAST,
                                                                     InsertionLocation(),
                                                                     DefPosOutsideClass,
                                                                     interface.filePath());
                                }

                                // Insert Position: Inside Class
                                // Determine insert location direct after the declaration.
                                int line, column;
                                const CppRefactoringFilePtr file = interface.currentFile();
                                file->lineAndColumn(file->endOf(simpleDecl), &line, &column);
                                const InsertionLocation loc
                                    = InsertionLocation(interface.filePath(), QString(),
                                                        QString(), line, column);
                                result << new InsertDefOperation(interface, decl, declAST, loc,
                                                                 DefPosInsideClass, FilePath(),
                                                                 isFreeFunction);

                                return;
                            }
                        }
                    }
                }
                break;
            }
        }
    }

    bool m_defPosOutsideClass = false;
};

//! Adds a definition for any number of member function declarations.
class InsertDefsFromDecls : public CppQuickFixFactory
{
public:
#ifdef WITH_TESTS
    static QObject *createTest();
#endif

    void setMode(InsertDefsFromDeclsMode mode) { m_mode = mode; }

private:
    void doMatch(const CppQuickFixInterface &interface, QuickFixOperations &result) override
    {
        const auto op = QSharedPointer<InsertDefsOperation>::create(interface);
        op->setMode(m_mode);
        if (op->isApplicable())
            result << op;
    }

private:
    InsertDefsFromDeclsMode m_mode = InsertDefsFromDeclsMode::User;
};

#ifdef WITH_TESTS
using namespace Tests;

static QList<TestDocumentPtr> singleHeader(const QByteArray &original, const QByteArray &expected)
{
    return {CppTestDocument::create("file.h", original, expected)};
}

class InsertDefFromDeclTest : public QObject
{
    Q_OBJECT

private slots:
    /// Check if definition is inserted right after class for insert definition outside
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
            "    void a@();\n"
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
            "inline void Foo::a()\n"
            "{\n\n}\n"
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

        InsertDefFromDecl factory;
        QuickFixOperationTest(testDocuments, &factory, ProjectExplorer::HeaderPaths(), 1);
    }

    /// Check from header file: If there is a source file, insert the definition in the source file.
    /// Case: Source file is empty.
    void testHeaderSourceBasic1()
    {
        QList<TestDocumentPtr> testDocuments;

        QByteArray original;
        QByteArray expected;

        // Header File
        original =
            "struct Foo\n"
            "{\n"
            "    Foo()@;\n"
            "};\n";
        expected = original;
        testDocuments << CppTestDocument::create("file.h", original, expected);

        // Source File
        original.resize(0);
        expected =
            "\n"
            "Foo::Foo()\n"
            "{\n\n"
            "}\n"
            ;
        testDocuments << CppTestDocument::create("file.cpp", original, expected);

        InsertDefFromDecl factory;
        QuickFixOperationTest(testDocuments, &factory);
    }

    /// Check from header file: If there is a source file, insert the definition in the source file.
    /// Case: Source file is not empty.
    void testHeaderSourceBasic2()
    {
        QList<TestDocumentPtr> testDocuments;

        QByteArray original;
        QByteArray expected;

        // Header File
        original = "void f(const std::vector<int> &v)@;\n";
        expected = original;
        testDocuments << CppTestDocument::create("file.h", original, expected);

        // Source File
        original =
            "#include \"file.h\"\n"
            "\n"
            "int x;\n"
            ;
        expected =
            "#include \"file.h\"\n"
            "\n"
            "int x;\n"
            "\n"
            "void f(const std::vector<int> &v)\n"
            "{\n"
            "\n"
            "}\n"
            ;
        testDocuments << CppTestDocument::create("file.cpp", original, expected);

        InsertDefFromDecl factory;
        QuickFixOperationTest(testDocuments, &factory);
    }

    /// Check from source file: Insert in source file, not header file.
    void testHeaderSourceBasic3()
    {
        QList<TestDocumentPtr> testDocuments;

        QByteArray original;
        QByteArray expected;

        // Empty Header File
        testDocuments << CppTestDocument::create("file.h", "", "");

        // Source File
        original =
            "struct Foo\n"
            "{\n"
            "    Foo()@;\n"
            "};\n";
        expected = original +
                   "\n"
                   "Foo::Foo()\n"
                   "{\n\n"
                   "}\n"
            ;
        testDocuments << CppTestDocument::create("file.cpp", original, expected);

        InsertDefFromDecl factory;
        QuickFixOperationTest(testDocuments, &factory);
    }

    /// Check from header file: If the class is in a namespace, the added function definition
    /// name must be qualified accordingly.
    void testHeaderSourceNamespace1()
    {
        QList<TestDocumentPtr> testDocuments;

        QByteArray original;
        QByteArray expected;

        // Header File
        original =
            "namespace N {\n"
            "struct Foo\n"
            "{\n"
            "    Foo()@;\n"
            "};\n"
            "}\n";
        expected = original;
        testDocuments << CppTestDocument::create("file.h", original, expected);

        // Source File
        original.resize(0);
        expected =
            "\n"
            "N::Foo::Foo()\n"
            "{\n\n"
            "}\n"
            ;
        testDocuments << CppTestDocument::create("file.cpp", original, expected);

        InsertDefFromDecl factory;
        QuickFixOperationTest(testDocuments, &factory);
    }

    /// Check from header file: If the class is in namespace N and the source file has a
    /// "using namespace N" line, the function definition name must be qualified accordingly.
    void testHeaderSourceNamespace2()
    {
        QList<TestDocumentPtr> testDocuments;

        QByteArray original;
        QByteArray expected;

        // Header File
        original =
            "namespace N {\n"
            "struct Foo\n"
            "{\n"
            "    Foo()@;\n"
            "};\n"
            "}\n";
        expected = original;
        testDocuments << CppTestDocument::create("file.h", original, expected);

        // Source File
        original =
            "#include \"file.h\"\n"
            "using namespace N;\n"
            ;
        expected = original +
                   "\n"
                   "Foo::Foo()\n"
                   "{\n\n"
                   "}\n"
            ;
        testDocuments << CppTestDocument::create("file.cpp", original, expected);

        InsertDefFromDecl factory;
        QuickFixOperationTest(testDocuments, &factory);
    }

    /// Check definition insert inside class
    void testInsideClass()
    {
        const QByteArray original =
            "class Foo {\n"
            "    void b@ar();\n"
            "};";
        const QByteArray expected =
            "class Foo {\n"
            "    void bar()\n"
            "    {\n\n"
            "    }\n"
            "};";

        InsertDefFromDecl factory;
        QuickFixOperationTest(singleDocument(original, expected), &factory, ProjectExplorer::HeaderPaths(),
                              1);
    }

    /// Check not triggering when definition exists
    void testNotTriggeringWhenDefinitionExists()
    {
        const QByteArray original =
            "class Foo {\n"
            "    void b@ar();\n"
            "};\n"
            "void Foo::bar() {}\n";

        InsertDefFromDecl factory;
        QuickFixOperationTest(singleDocument(original, ""), &factory, ProjectExplorer::HeaderPaths(), 1);
    }

    /// Find right implementation file.
    void testFindRightImplementationFile()
    {
        QList<TestDocumentPtr> testDocuments;

        QByteArray original;
        QByteArray expected;

        // Header File
        original =
            "struct Foo\n"
            "{\n"
            "    Foo();\n"
            "    void a();\n"
            "    void b@();\n"
            "};\n"
            "}\n";
        expected = original;
        testDocuments << CppTestDocument::create("file.h", original, expected);

        // Source File #1
        original =
            "#include \"file.h\"\n"
            "\n"
            "Foo::Foo()\n"
            "{\n\n"
            "}\n";
        expected = original;
        testDocuments << CppTestDocument::create("file.cpp", original, expected);


        // Source File #2
        original =
            "#include \"file.h\"\n"
            "\n"
            "void Foo::a()\n"
            "{\n\n"
            "}\n";
        expected = original +
                   "\n"
                   "void Foo::b()\n"
                   "{\n\n"
                   "}\n";
        testDocuments << CppTestDocument::create("file2.cpp", original, expected);

        InsertDefFromDecl factory;
        QuickFixOperationTest(testDocuments, &factory);
    }

    /// Ignore generated functions declarations when looking at the surrounding
    /// functions declarations in order to find the right implementation file.
    void testIgnoreSurroundingGeneratedDeclarations()
    {
        QList<TestDocumentPtr> testDocuments;

        QByteArray original;
        QByteArray expected;

        // Header File
        original =
            "#define DECLARE_HIDDEN_FUNCTION void hidden();\n"
            "struct Foo\n"
            "{\n"
            "    void a();\n"
            "    DECLARE_HIDDEN_FUNCTION\n"
            "    void b@();\n"
            "};\n"
            "}\n";
        expected = original;
        testDocuments << CppTestDocument::create("file.h", original, expected);

        // Source File #1
        original =
            "#include \"file.h\"\n"
            "\n"
            "void Foo::a()\n"
            "{\n\n"
            "}\n";
        expected =
            "#include \"file.h\"\n"
            "\n"
            "void Foo::a()\n"
            "{\n\n"
            "}\n"
            "\n"
            "void Foo::b()\n"
            "{\n\n"
            "}\n";
        testDocuments << CppTestDocument::create("file.cpp", original, expected);

        // Source File #2
        original =
            "#include \"file.h\"\n"
            "\n"
            "void Foo::hidden()\n"
            "{\n\n"
            "}\n";
        expected = original;
        testDocuments << CppTestDocument::create("file2.cpp", original, expected);

        InsertDefFromDecl factory;
        QuickFixOperationTest(testDocuments, &factory);
    }

    /// Check if whitespace is respected for operator functions
    void testRespectWsInOperatorNames1()
    {
        QByteArray original =
            "class Foo\n"
            "{\n"
            "    Foo &opera@tor =();\n"
            "};\n";
        QByteArray expected =
            "class Foo\n"
            "{\n"
            "    Foo &operator =();\n"
            "};\n"
            "\n"
            "Foo &Foo::operator =()\n"
            "{\n"
            "\n"
            "}\n";

        InsertDefFromDecl factory;
        QuickFixOperationTest(singleDocument(original, expected), &factory);
    }

    /// Check if whitespace is respected for operator functions
    void testRespectWsInOperatorNames2()
    {
        QByteArray original =
            "class Foo\n"
            "{\n"
            "    Foo &opera@tor=();\n"
            "};\n";
        QByteArray expected =
            "class Foo\n"
            "{\n"
            "    Foo &operator=();\n"
            "};\n"
            "\n"
            "Foo &Foo::operator=()\n"
            "{\n"
            "\n"
            "}\n";

        InsertDefFromDecl factory;
        QuickFixOperationTest(singleDocument(original, expected), &factory);
    }

    /// Check that the noexcept exception specifier is transferred
    void testNoexceptSpecifier()
    {
        QByteArray original =
            "class Foo\n"
            "{\n"
            "    void @foo() noexcept(false);\n"
            "};\n";
        QByteArray expected =
            "class Foo\n"
            "{\n"
            "    void foo() noexcept(false);\n"
            "};\n"
            "\n"
            "void Foo::foo() noexcept(false)\n"
            "{\n"
            "\n"
            "}\n";

        InsertDefFromDecl factory;
        QuickFixOperationTest(singleDocument(original, expected), &factory);
    }

    /// Check if a function like macro use is not separated by the function to insert
    /// Case: Macro preceded by preproceesor directives and declaration.
    void testMacroUsesAtEndOfFile1()
    {
        QList<TestDocumentPtr> testDocuments;

        QByteArray original;
        QByteArray expected;

        // Header File
        original = "void f()@;\n";
        expected = original;
        testDocuments << CppTestDocument::create("file.h", original, expected);

        // Source File
        original =
            "#include \"file.h\"\n"
            "#define MACRO(X) X x;\n"
            "int lala;\n"
            "\n"
            "MACRO(int)\n"
            ;
        expected =
            "#include \"file.h\"\n"
            "#define MACRO(X) X x;\n"
            "int lala;\n"
            "\n"
            "\n"
            "\n"
            "void f()\n"
            "{\n"
            "\n"
            "}\n"
            "\n"
            "MACRO(int)\n"
            ;
        testDocuments << CppTestDocument::create("file.cpp", original, expected);

        InsertDefFromDecl factory;
        QuickFixOperationTest(testDocuments, &factory);
    }

    /// Check if a function like macro use is not separated by the function to insert
    /// Case: Marco preceded only by preprocessor directives.
    void testMacroUsesAtEndOfFile2()
    {
        QList<TestDocumentPtr> testDocuments;

        QByteArray original;
        QByteArray expected;

        // Header File
        original = "void f()@;\n";
        expected = original;
        testDocuments << CppTestDocument::create("file.h", original, expected);

        // Source File
        original =
            "#include \"file.h\"\n"
            "#define MACRO(X) X x;\n"
            "\n"
            "MACRO(int)\n"
            ;
        expected =
            "#include \"file.h\"\n"
            "#define MACRO(X) X x;\n"
            "\n"
            "\n"
            "\n"
            "void f()\n"
            "{\n"
            "\n"
            "}\n"
            "\n"
            "MACRO(int)\n"
            ;
        testDocuments << CppTestDocument::create("file.cpp", original, expected);

        InsertDefFromDecl factory;
        QuickFixOperationTest(testDocuments, &factory);
    }

    /// Check if insertion happens before syntactically erroneous statements at end of file.
    void testErroneousStatementAtEndOfFile()
    {
        QList<TestDocumentPtr> testDocuments;

        QByteArray original;
        QByteArray expected;

        // Header File
        original = "void f()@;\n";
        expected = original;
        testDocuments << CppTestDocument::create("file.h", original, expected);

        // Source File
        original =
            "#include \"file.h\"\n"
            "\n"
            "MissingSemicolon(int)\n"
            ;
        expected =
            "#include \"file.h\"\n"
            "\n"
            "\n"
            "\n"
            "void f()\n"
            "{\n"
            "\n"
            "}\n"
            "\n"
            "MissingSemicolon(int)\n"
            ;
        testDocuments << CppTestDocument::create("file.cpp", original, expected);

        InsertDefFromDecl factory;
        QuickFixOperationTest(testDocuments, &factory);
    }

    /// Check: Respect rvalue references
    void testRvalueReference()
    {
        QList<TestDocumentPtr> testDocuments;

        QByteArray original;
        QByteArray expected;

        // Header File
        original = "void f(Foo &&)@;\n";
        expected = original;
        testDocuments << CppTestDocument::create("file.h", original, expected);

        // Source File
        original = "";
        expected =
            "\n"
            "void f(Foo &&)\n"
            "{\n"
            "\n"
            "}\n"
            ;
        testDocuments << CppTestDocument::create("file.cpp", original, expected);

        InsertDefFromDecl factory;
        QuickFixOperationTest(testDocuments, &factory);
    }

    void testFunctionTryBlock()
    {
        QList<TestDocumentPtr> testDocuments;

        QByteArray original;
        QByteArray expected;

        // Header File
        original = R"(
struct Foo {
    void tryCatchFunc();
    void @otherFunc();
};
)";
        expected = original;
        testDocuments << CppTestDocument::create("file.h", original, expected);

        // Source File
        original = R"(
#include "file.h"

void Foo::tryCatchFunc() try {} catch (...) {}
)";
        expected = R"(
#include "file.h"

void Foo::tryCatchFunc() try {} catch (...) {}

void Foo::otherFunc()
{

}
)";
        testDocuments << CppTestDocument::create("file.cpp", original, expected);

        InsertDefFromDecl factory;
        QuickFixOperationTest(testDocuments, &factory);
    }

    void testUsingDecl()
    {
        QList<TestDocumentPtr> testDocuments;

        QByteArray original;
        QByteArray expected;

        // Header File
        original = R"(
namespace N { struct S; }
using N::S;

void @func(const S &s);
)";
        expected = original;
        testDocuments << CppTestDocument::create("file.h", original, expected);

        // Source File
        original = R"(
#include "file.h"
)";
        expected = R"(
#include "file.h"

void func(const S &s)
{

}
)";
        testDocuments << CppTestDocument::create("file.cpp", original, expected);

        InsertDefFromDecl factory;
        QuickFixOperationTest(testDocuments, &factory);

        testDocuments.clear();
        original = R"(
namespace N1 {
namespace N2 { struct S; }
using N2::S;
}

void @func(const N1::S &s);
)";
        expected = original;
        testDocuments << CppTestDocument::create("file.h", original, expected);

        // Source File
        original = R"(
#include "file.h"
)";
        expected = R"(
#include "file.h"

void func(const N1::S &s)
{

}
)";
        testDocuments << CppTestDocument::create("file.cpp", original, expected);
        QuickFixOperationTest(testDocuments, &factory);

        // No using declarations here, but the code model has one. No idea why.
        testDocuments.clear();
        original = R"(
class B {};
class D : public B {
    @D();
};
)";
        expected = original;
        testDocuments << CppTestDocument::create("file.h", original, expected);

        // Source File
        original = R"(
#include "file.h"
)";
        expected = R"(
#include "file.h"

D::D()
{

}
)";
        testDocuments << CppTestDocument::create("file.cpp", original, expected);
        QuickFixOperationTest(testDocuments, &factory);

        testDocuments.clear();
        original = R"(
namespace ns1 { template<typename T> class span {}; }

namespace ns {
using ns1::span;
class foo
{
    void @bar(ns::span<int>);
};
}
)";
        expected = R"(
namespace ns1 { template<typename T> class span {}; }

namespace ns {
using ns1::span;
class foo
{
    void bar(ns::span<int>);
};

void foo::bar(ns::span<int>)
{

}

}
)";
        // TODO: Unneeded namespace gets inserted in RewriteName::visit(const QualifiedNameId *)
        testDocuments << CppTestDocument::create("file.cpp", original, expected);
        QuickFixOperationTest(testDocuments, &factory);
    }

    /// Find right implementation file. (QTCREATORBUG-10728)
    void testFindImplementationFile()
    {
        QList<TestDocumentPtr> testDocuments;

        QByteArray original;
        QByteArray expected;

        // Header File
        original =
            "class Foo {\n"
            "    void bar();\n"
            "    void ba@z();\n"
            "};\n"
            "\n"
            "void Foo::bar()\n"
            "{}\n";
        expected = original;
        testDocuments << CppTestDocument::create("file.h", original, expected);

        // Source File
        original =
            "#include \"file.h\"\n"
            ;
        expected =
            "#include \"file.h\"\n"
            "\n"
            "void Foo::baz()\n"
            "{\n"
            "\n"
            "}\n"
            ;
        testDocuments << CppTestDocument::create("file.cpp", original, expected);

        InsertDefFromDecl factory;
        QuickFixOperationTest(testDocuments, &factory);
    }

    void testUnicodeIdentifier()
    {
        QList<TestDocumentPtr> testDocuments;

        QByteArray original;
        QByteArray expected;

//
// The following "non-latin1" code points are used in the tests:
//
//   U+00FC  - 2 code units in UTF8, 1 in UTF16 - LATIN SMALL LETTER U WITH DIAERESIS
//   U+4E8C  - 3 code units in UTF8, 1 in UTF16 - CJK UNIFIED IDEOGRAPH-4E8C
//   U+10302 - 4 code units in UTF8, 2 in UTF16 - OLD ITALIC LETTER KE
//

#define UNICODE_U00FC "\xc3\xbc"
#define UNICODE_U4E8C "\xe4\xba\x8c"
#define UNICODE_U10302 "\xf0\x90\x8c\x82"
#define TEST_UNICODE_IDENTIFIER UNICODE_U00FC UNICODE_U4E8C UNICODE_U10302

        original =
            "class Foo {\n"
            "    void @" TEST_UNICODE_IDENTIFIER "();\n"
            "};\n";
        ;
        expected = original;
        expected +=
            "\n"
            "void Foo::" TEST_UNICODE_IDENTIFIER "()\n"
            "{\n"
            "\n"
            "}\n";
        testDocuments << CppTestDocument::create("file.cpp", original, expected);

#undef UNICODE_U00FC
#undef UNICODE_U4E8C
#undef UNICODE_U10302
#undef TEST_UNICODE_IDENTIFIER

        InsertDefFromDecl factory;
        QuickFixOperationTest(testDocuments, &factory);
    }

    void testTemplateClass()
    {
        QByteArray original =
            "template<class T>\n"
            "class Foo\n"
            "{\n"
            "    void fun@c1();\n"
            "    void func2();\n"
            "};\n\n"
            "template<class T>\n"
            "void Foo<T>::func2() {}\n";
        QByteArray expected =
            "template<class T>\n"
            "class Foo\n"
            "{\n"
            "    void func1();\n"
            "    void func2();\n"
            "};\n\n"
            "template<class T>\n"
            "void Foo<T>::func1()\n"
            "{\n"
            "\n"
            "}\n\n"
            "template<class T>\n"
            "void Foo<T>::func2() {}\n";

        InsertDefFromDecl factory;
        QuickFixOperationTest(singleDocument(original, expected), &factory);
    }

    void testTemplateClassWithValueParam()
    {
        QList<TestDocumentPtr> testDocuments;
        QByteArray original =
            "template<typename T, int size> struct MyArray {};\n"
            "MyArray<int, 1> @foo();";
        QByteArray expected = original;
        testDocuments << CppTestDocument::create("file.h", original, expected);

        original = "#include \"file.h\"\n";
        expected =
            "#include \"file.h\"\n\n"
            "MyArray<int, 1> foo()\n"
            "{\n\n"
            "}\n";
        testDocuments << CppTestDocument::create("file.cpp", original, expected);

        InsertDefFromDecl factory;
        QuickFixOperationTest(testDocuments, &factory);
    }

    void testTemplateFunction()
    {
        QByteArray original =
            "class Foo\n"
            "{\n"
            "    template<class T>\n"
            "    void fun@c();\n"
            "};\n";
        QByteArray expected =
            "class Foo\n"
            "{\n"
            "    template<class T>\n"
            "    void fun@c();\n"
            "};\n"
            "\n"
            "template<class T>\n"
            "void Foo::func()\n"
            "{\n"
            "\n"
            "}\n";

        InsertDefFromDecl factory;
        QuickFixOperationTest(singleDocument(original, expected), &factory);
    }

    void testTemplateClassAndTemplateFunction()
    {
        QByteArray original =
            "template<class T>"
            "class Foo\n"
            "{\n"
            "    template<class U>\n"
            "    T fun@c(U u);\n"
            "};\n";
        QByteArray expected =
            "template<class T>"
            "class Foo\n"
            "{\n"
            "    template<class U>\n"
            "    T fun@c(U u);\n"
            "};\n"
            "\n"
            "template<class T>\n"
            "template<class U>\n"
            "T Foo<T>::func(U u)\n"
            "{\n"
            "\n"
            "}\n";

        InsertDefFromDecl factory;
        QuickFixOperationTest(singleDocument(original, expected), &factory);
    }

    void testTemplateClassAndFunctionInsideNamespace()
    {
        QByteArray original =
            "namespace N {\n"
            "template<class T>"
            "class Foo\n"
            "{\n"
            "    template<class U>\n"
            "    T fun@c(U u);\n"
            "};\n"
            "}\n";
        QByteArray expected =
            "namespace N {\n"
            "template<class T>"
            "class Foo\n"
            "{\n"
            "    template<class U>\n"
            "    T fun@c(U u);\n"
            "};\n"
            "\n"
            "template<class T>\n"
            "template<class U>\n"
            "T Foo<T>::func(U u)\n"
            "{\n"
            "\n"
            "}\n"
            "\n"
            "}\n";

        InsertDefFromDecl factory;
        QuickFixOperationTest(singleDocument(original, expected), &factory);
    }

    void testFunctionWithSignedUnsignedArgument()
    {
        QByteArray original;
        QByteArray expected;
        InsertDefFromDecl factory;

        original =R"--(
class myclass
{
    myc@lass(QVector<signed> g);
    myclass(QVector<unsigned> g);
}
)--";
        expected =R"--(
class myclass
{
    myclass(QVector<signed> g);
    myclass(QVector<unsigned> g);
}

myclass::myclass(QVector<signed int> g)
{

}
)--";

        QuickFixOperationTest(singleDocument(original, expected), &factory);

        original =R"--(
class myclass
{
    myclass(QVector<signed> g);
    myc@lass(QVector<unsigned> g);
}
)--";
        expected =R"--(
class myclass
{
    myclass(QVector<signed> g);
    myclass(QVector<unsigned> g);
}

myclass::myclass(QVector<unsigned int> g)
{

}
)--";

        QuickFixOperationTest(singleDocument(original, expected), &factory);

        original =R"--(
class myclass
{
    unsigned f@oo(unsigned);
}
)--";
        expected =R"--(
class myclass
{
    unsigned foo(unsigned);
}

unsigned int myclass::foo(unsigned int)
{

}
)--";
        QuickFixOperationTest(singleDocument(original, expected), &factory);

        original =R"--(
class myclass
{
    signed f@oo(signed);
}
)--";
        expected =R"--(
class myclass
{
    signed foo(signed);
}

signed int myclass::foo(signed int)
{

}
)--";
        QuickFixOperationTest(singleDocument(original, expected), &factory);
    }

    void testNotTriggeredForFriendFunc()
    {
        const QByteArray contents =
            "class Foo\n"
            "{\n"
            "    friend void f@unc();\n"
            "};\n"
            "\n";

        InsertDefFromDecl factory;
        QuickFixOperationTest(singleDocument(contents, ""), &factory);
    }

    void testMinimalFunctionParameterType()
    {
        QList<TestDocumentPtr> testDocuments;

        QByteArray original;
        QByteArray expected;

        // Header File
        original = R"(
class C {
    typedef int A;
    A @foo(A);
};
)";
        expected = original;
        testDocuments << CppTestDocument::create("file.h", original, expected);

        // Source File
        original = R"(
#include "file.h"
)";
        expected = R"(
#include "file.h"

C::A C::foo(A)
{

}
)";
        testDocuments << CppTestDocument::create("file.cpp", original, expected);

        InsertDefFromDecl factory;
        QuickFixOperationTest(testDocuments, &factory);

        testDocuments.clear();
        // Header File
        original = R"(
namespace N {
    struct S;
    S @foo(const S &s);
};
)";
        expected = original;
        testDocuments << CppTestDocument::create("file.h", original, expected);

        // Source File
        original = R"(
#include "file.h"
)";
        expected = R"(
#include "file.h"

N::S N::foo(const S &s)
{

}
)";
        testDocuments << CppTestDocument::create("file.cpp", original, expected);

        QuickFixOperationTest(testDocuments, &factory);
    }

    void testAliasTemplateAsReturnType()
    {
        QList<TestDocumentPtr> testDocuments;

        QByteArray original;
        QByteArray expected;

        // Header File
        original = R"(
struct foo {
    struct foo2 {
        template <typename T> using MyType = T;
        MyType<int> @bar();
    };
};
)";
        expected = original;
        testDocuments << CppTestDocument::create("file.h", original, expected);

        // Source File
        original = R"(
#include "file.h"
)";
        expected = R"(
#include "file.h"

foo::foo2::MyType<int> foo::foo2::bar()
{

}
)";
        testDocuments << CppTestDocument::create("file.cpp", original, expected);

        InsertDefFromDecl factory;
        QuickFixOperationTest(testDocuments, &factory);
    }

    void test_data()
    {
        QTest::addColumn<QByteArray>("original");
        QTest::addColumn<QByteArray>("expected");

        // Check from source file: If there is no header file, insert the definition after the class.
        QByteArray original =
            "struct Foo\n"
            "{\n"
            "    Foo();@\n"
            "};\n";
        QByteArray expected = original +
                                  "\n"
                                  "Foo::Foo()\n"
                                  "{\n\n"
                                  "}\n";
        QTest::newRow("basic") << original << expected;

        original = "void free()@;\n";
        expected = "void free()\n{\n\n}\n";
        QTest::newRow("freeFunction") << original << expected;

        original = "class Foo {\n"
                   "public:\n"
                   "    Foo() {}\n"
                   "};\n"
                   "void freeFunc() {\n"
                   "    Foo @f();"
                   "}\n";

        // Check not triggering when it is a statement
        QTest::newRow("notTriggeringStatement") << original << QByteArray();
    }

    void test()
    {
        QFETCH(QByteArray, original);
        QFETCH(QByteArray, expected);

        InsertDefFromDecl factory;
        QuickFixOperationTest(singleDocument(original, expected), &factory);
    }

    void testOutsideTemplateClassAndTemplateFunction()
    {
        QByteArray original =
            "template<class T>"
            "class Foo\n"
            "{\n"
            "    template<class U>\n"
            "    void fun@c();\n"
            "};\n";
        QByteArray expected =
            "template<class T>"
            "class Foo\n"
            "{\n"
            "    template<class U>\n"
            "    void fun@c();\n"
            "};\n"
            "\n"
            "template<class T>\n"
            "template<class U>\n"
            "inline void Foo<T>::func()\n"
            "{\n"
            "\n"
            "}\n";

        InsertDefFromDecl factory;
        factory.setOutside();
        QuickFixOperationTest(singleHeader(original, expected), &factory);
    }

    void testOutsideTemplateClass()
    {
        QByteArray original =
            "template<class T>"
            "class Foo\n"
            "{\n"
            "    void fun@c();\n"
            "};\n";
        QByteArray expected =
            "template<class T>"
            "class Foo\n"
            "{\n"
            "    void fun@c();\n"
            "};\n"
            "\n"
            "template<class T>\n"
            "inline void Foo<T>::func()\n"
            "{\n"
            "\n"
            "}\n";

        InsertDefFromDecl factory;
        factory.setOutside();
        QuickFixOperationTest(singleHeader(original, expected), &factory);
    }

    void testOutsideTemplateFunction()
    {
        QByteArray original =
            "class Foo\n"
            "{\n"
            "    template<class U>\n"
            "    void fun@c();\n"
            "};\n";
        QByteArray expected =
            "class Foo\n"
            "{\n"
            "    template<class U>\n"
            "    void fun@c();\n"
            "};\n"
            "\n"
            "template<class U>\n"
            "inline void Foo::func()\n"
            "{\n"
            "\n"
            "}\n";

        InsertDefFromDecl factory;
        factory.setOutside();
        QuickFixOperationTest(singleHeader(original, expected), &factory);
    }

    void testOutsideFunction()
    {
        QByteArray original =
            "class Foo\n"
            "{\n"
            "    void fun@c();\n"
            "};\n";
        QByteArray expected =
            "class Foo\n"
            "{\n"
            "    void fun@c();\n"
            "};\n"
            "\n"
            "inline void Foo::func()\n"
            "{\n"
            "\n"
            "}\n";

        InsertDefFromDecl factory;
        factory.setOutside();
        QuickFixOperationTest(singleHeader(original, expected), &factory);

    }

};

class InsertDefsFromDeclsTest : public QObject
{
    Q_OBJECT

private slots:

    void test_data()
    {
        QTest::addColumn<QByteArrayList>("headers");
        QTest::addColumn<QByteArrayList>("sources");
        QTest::addColumn<int>("mode");

        QByteArray origHeader = R"(
namespace N {
class @C
{
public:
    friend void ignoredFriend();
    void ignoredImplemented() {};
    void ignoredImplemented2(); // Below
    void ignoredImplemented3(); // In cpp file
    void funcNotSelected();
    void funcInline();
    void funcBelow();
    void funcCppFile();

signals:
    void ignoredSignal();
};

inline void C::ignoredImplemented2() {}

} // namespace N)";
        QByteArray origSource = R"(
#include "file.h"

namespace N {

void C::ignoredImplemented3() {}

} // namespace N)";

        QByteArray expectedHeader = R"(
namespace N {
class C
{
public:
    friend void ignoredFriend();
    void ignoredImplemented() {};
    void ignoredImplemented2(); // Below
    void ignoredImplemented3(); // In cpp file
    void funcNotSelected();
    void funcInline()
    {

    }
    void funcBelow();
    void funcCppFile();

signals:
    void ignoredSignal();
};

inline void C::ignoredImplemented2() {}

inline void C::funcBelow()
{

}

} // namespace N)";
        QByteArray expectedSource = R"(
#include "file.h"

namespace N {

void C::ignoredImplemented3() {}

void C::funcCppFile()
{

}

} // namespace N)";
        QTest::addRow("normal case")
            << QByteArrayList{origHeader, expectedHeader}
            << QByteArrayList{origSource, expectedSource}
            << int(InsertDefsFromDeclsMode::Alternating);
        QTest::addRow("aborted dialog")
            << QByteArrayList{origHeader, origHeader}
            << QByteArrayList{origSource, origSource}
            << int(InsertDefsFromDeclsMode::Off);

        origHeader = R"(
        namespace N {
        class @C
        {
        public:
            friend void ignoredFriend();
            void ignoredImplemented() {};
            void ignoredImplemented2(); // Below
            void ignoredImplemented3(); // In cpp file

        signals:
            void ignoredSignal();
        };

        inline void C::ignoredImplemented2() {}

        } // namespace N)";
        QTest::addRow("no candidates")
            << QByteArrayList{origHeader, origHeader}
            << QByteArrayList{origSource, origSource}
            << int(InsertDefsFromDeclsMode::Alternating);

        origHeader = R"(
        namespace N {
        class @C
        {
        public:
            friend void ignoredFriend();
            void ignoredImplemented() {};

        signals:
            void ignoredSignal();
        };
        } // namespace N)";
        QTest::addRow("no member functions")
            << QByteArrayList{origHeader, ""}
            << QByteArrayList{origSource, ""}
            << int(InsertDefsFromDeclsMode::Alternating);


    }

    void test()
    {
        QFETCH(QByteArrayList, headers);
        QFETCH(QByteArrayList, sources);
        QFETCH(int, mode);

        QList<TestDocumentPtr> testDocuments(
            {CppTestDocument::create("file.h", headers.at(0), headers.at(1)),
             CppTestDocument::create("file.cpp", sources.at(0), sources.at(1))});
        InsertDefsFromDecls factory;
        factory.setMode(static_cast<InsertDefsFromDeclsMode>(mode));
        QuickFixOperationTest(testDocuments, &factory);
    }

    void testInsertAndFormat()
    {
        if (!isClangFormatPresent())
            QSKIP("This test reqires ClangFormat");

        const QByteArray origHeader = R"(
class @C
{
public:
    void func1 (int const &i);
    void func2 (double const d);
};
)";
        const QByteArray origSource = R"(
#include "file.h"
)";

        const QByteArray expectedSource = R"(
#include "file.h"

void C::func1 (int const &i)
{

}

void C::func2 (double const d)
{

}
)";

        const QByteArray clangFormatSettings = R"(
BreakBeforeBraces: Allman
QualifierAlignment: Right
SpaceBeforeParens: Always
)";

        const QList<TestDocumentPtr> testDocuments({
                                                    CppTestDocument::create("file.h", origHeader, origHeader),
                                                    CppTestDocument::create("file.cpp", origSource, expectedSource)});
        InsertDefsFromDecls factory;
        factory.setMode(InsertDefsFromDeclsMode::Impl);
        CppCodeStylePreferences * const prefs = CppToolsSettings::cppCodeStyle();
        const CppCodeStyleSettings settings = prefs->codeStyleSettings();
        CppCodeStyleSettings tempSettings = settings;
        tempSettings.forceFormatting = true;
        prefs->setCodeStyleSettings(tempSettings);
        QuickFixOperationTest(testDocuments, &factory, {}, {}, {}, clangFormatSettings);
        prefs->setCodeStyleSettings(settings);
    }
};

QObject *InsertDefFromDecl::createTest()
{
    return new InsertDefFromDeclTest;
}

QObject *InsertDefsFromDecls::createTest()
{
    return new InsertDefsFromDeclsTest;
}

#endif // WITH_TESTS
} // namespace

void registerInsertFunctionDefinitionQuickfixes()
{
    CppQuickFixFactory::registerFactory<InsertDefFromDecl>();
    CppQuickFixFactory::registerFactory<InsertDefsFromDecls>();
}

} // namespace CppEditor::Internal

#ifdef WITH_TESTS
#include <insertfunctiondefinition.moc>
#endif
