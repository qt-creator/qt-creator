// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "removeusingnamespace.h"

#include "../cppeditortr.h"
#include "../cppprojectfile.h"
#include "../cpprefactoringchanges.h"
#include "cppquickfix.h"

#include <cplusplus/Overview.h>
#include <projectexplorer/projectmanager.h>

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

/**
 * @brief The NameCounter class counts the parts of a name. E.g. 2 for std::vector or 1 for variant
 */
class NameCounter : private NameVisitor
{
public:
    int count(const Name *name)
    {
        counter = 0;
        accept(name);
        return counter;
    }

private:
    void visit(const Identifier *) override { ++counter; }
    void visit(const DestructorNameId *) override { ++counter; }
    void visit(const TemplateNameId *) override { ++counter; }
    void visit(const QualifiedNameId *name) override
    {
        if (name->base())
            accept(name->base());
        accept(name->name());
    }
    int counter;
};

/**
 * @brief getBaseName returns the base name of a qualified name or nullptr.
 * E.g.: foo::bar => foo; bar => bar
 * @param name The Name, maybe qualified
 * @return The base name of the qualified name or nullptr
 */
const Identifier *getBaseName(const Name *name)
{
    class GetBaseName : public NameVisitor
    {
        void visit(const Identifier *name) override { baseName = name; }
        void visit(const QualifiedNameId *name) override
        {
            if (name->base())
                accept(name->base());
            else
                accept(name->name());
        }

    public:
        const Identifier *baseName = nullptr;
    };
    GetBaseName getter;
    getter.accept(name);
    return getter.baseName;
}

/**
 * @brief countNames counts the parts of the Name.
 * E.g. if the name is std::vector, the function returns 2, if the name is variant, returns 1
 * @param name The name that should be counted
 * @return the number of parts of the name
 */
int countNames(const Name *name)
{
    return NameCounter{}.count(name);
}

/**
 * @brief removeLine removes the whole line in which the ast node is located if there are otherwise only whitespaces
 * @param file The file in which the AST node is located
 * @param ast The ast node
 * @param changeSet The ChangeSet of the file
 */
void removeLine(const CppRefactoringFile *file, AST *ast, ChangeSet &changeSet)
{
    RefactoringFile::Range range = file->range(ast);
    --range.start;
    while (range.start >= 0) {
        QChar current = file->charAt(range.start);
        if (!current.isSpace()) {
            ++range.start;
            break;
        }
        if (current == QChar::ParagraphSeparator)
            break;
        --range.start;
    }
    range.start = std::max(0, range.start);
    while (range.end < file->document()->characterCount()) {
        QChar current = file->charAt(range.end);
        if (!current.isSpace())
            break;
        if (current == QChar::ParagraphSeparator)
            break;
        ++range.end;
    }
    range.end = std::min(file->document()->characterCount(), range.end);
    const bool newLineStart = file->charAt(range.start) == QChar::ParagraphSeparator;
    const bool newLineEnd = file->charAt(range.end) == QChar::ParagraphSeparator;
    if (!newLineEnd && newLineStart)
        ++range.start;
    changeSet.remove(range);
}

/**
 * @brief The RemoveNamespaceVisitor class removes a using namespace and rewrites all types that
 * are in the namespace if needed
 */
class RemoveNamespaceVisitor : public ASTVisitor
{
public:
    constexpr static int SearchGlobalUsingDirectivePos = std::numeric_limits<int>::max();
    RemoveNamespaceVisitor(const CppRefactoringFile *file,
                           const Snapshot &snapshot,
                           const Name *namespace_,
                           int symbolPos,
                           bool removeAllAtGlobalScope)
        : ASTVisitor(file->cppDocument()->translationUnit())
        , m_file(file)
        , m_snapshot(snapshot)
        , m_namespace(namespace_)
        , m_missingNamespace(toString(namespace_) + "::")
        , m_context(m_file->cppDocument(), m_snapshot)
        , m_symbolPos(symbolPos)
        , m_removeAllAtGlobalScope(removeAllAtGlobalScope)

    {}

    const ChangeSet &getChanges() { return m_changeSet; }

    /**
     * @brief isGlobalUsingNamespace return true if the using namespace that should be removed
     * is not scoped and other files that include this file will also use the using namespace
     * @return true if using namespace statement is global and not scoped, false otherwise
     */
    bool isGlobalUsingNamespace() const { return m_parentNode == nullptr; }

    /**
     * @brief foundGlobalUsingNamespace return true if removeAllAtGlobalScope is false and
     * another using namespace is found at the global scope, so that other files that include this
     * file don't have to be processed
     * @return true if there was a 'global' second using namespace in this file and
     * removeAllAtGlobalScope is false
     */
    bool foundGlobalUsingNamespace() const { return m_foundNamespace; }

private:
    bool preVisit(AST *ast) override
    {
        if (!m_start) {
            if (ast->asTranslationUnit())
                return true;
            if (UsingDirectiveAST *usingDirective = ast->asUsingDirective()) {
                if (nameEqual(usingDirective->name->name, m_namespace)) {
                    if (m_symbolPos == SearchGlobalUsingDirectivePos) {
                        // we have found a global using directive, so lets start
                        m_start = true;
                        removeLine(m_file, ast, m_changeSet);
                        return false;
                    }
                    // ignore the using namespace that should be removed
                    if (m_file->endOf(ast) != m_symbolPos) {
                        if (m_removeAllAtGlobalScope)
                            removeLine(m_file, ast, m_changeSet);
                        else
                            m_done = true;
                    }
                }
            }
            // if the end of the ast is before we should start, we are not interested in the node
            if (m_file->endOf(ast) <= m_symbolPos)
                return false;

            if (m_file->startOf(ast) > m_symbolPos)
                m_start = true;
        }
        return !m_foundNamespace && !m_done;
    }

    bool visit(NamespaceAST *ast) override
    {
        if (m_start && nameEqual(m_namespace, ast->symbol->name()))
            return false;

        return m_start;
    }

    // scopes for using namespace statements:
    bool visit(LinkageBodyAST *ast) override { return visitNamespaceScope(ast); }
    bool visit(CompoundStatementAST *ast) override { return visitNamespaceScope(ast); }
    bool visitNamespaceScope(AST *ast)
    {
        ++m_namespaceScopeCounter;
        if (!m_start)
            m_parentNode = ast;
        return true;
    }

    void endVisit(LinkageBodyAST *ast) override { endVisitNamespaceScope(ast); }
    void endVisit(CompoundStatementAST *ast) override { endVisitNamespaceScope(ast); }
    void endVisitNamespaceScope(AST *ast)
    {
        --m_namespaceScopeCounter;
        m_foundNamespace = false;
        // if we exit the scope of the using namespace we are done
        if (ast == m_parentNode)
            m_done = true;
    }

    bool visit(UsingDirectiveAST *ast) override
    {
        if (nameEqual(ast->name->name, m_namespace)) {
            if (m_removeAllAtGlobalScope && m_namespaceScopeCounter == 0)
                removeLine(m_file, ast, m_changeSet);
            else
                m_foundNamespace = true;
            return false;
        }
        return handleAstWithLongestName(ast);
    }

    bool visit(DeclaratorIdAST *ast) override
    {
        // e.g. we have the following code and get the following Lookup items:
        // namespace test {
        //   struct foo { // 1. item with test::foo
        //     foo();     // 2. item with test::foo::foo
        //   };
        // }
        // using namespace foo;
        // foo::foo() { ... } // 3. item with foo::foo
        // Our current name is foo::foo so we have to match with the 2. item / longest name
        return handleAstWithLongestName(ast);
    }

    template<typename AST>
    bool handleAstWithLongestName(AST *ast)
    {
        if (m_start) {
            Scope *scope = m_file->scopeAt(ast->firstToken());
            const QList<LookupItem> localLookup = m_context.lookup(ast->name->name, scope);
            QList<const Name *> longestName;
            for (const LookupItem &item : localLookup) {
                QList<const Name *> names
                    = m_context.fullyQualifiedName(item.declaration(),
                                                   LookupContext::HideInlineNamespaces);
                if (names.length() > longestName.length())
                    longestName = names;
            }
            const int currentNameCount = countNames(ast->name->name);
            const bool needNew = needMissingNamespaces(std::move(longestName), currentNameCount);
            if (needNew)
                insertMissingNamespace(ast);
        }
        return false;
    }

    bool visit(NamedTypeSpecifierAST *ast) override { return handleAstWithName(ast); }

    bool visit(IdExpressionAST *ast) override { return handleAstWithName(ast); }

    template<typename AST>
    bool handleAstWithName(AST *ast)
    {
        if (m_start) {
            Scope *scope = m_file->scopeAt(ast->firstToken());
            const Name *wantToLookup = ast->name->name;
            // first check if the base name is a typedef. Consider the following example:
            // using namespace std;
            // using vec = std::vector<int>;
            // vec::iterator it; // we have to lookup 'vec' and not iterator (would result in
            //   std::vector<int>::iterator => std::vec::iterator, which is wrong)
            const Name *baseName = getBaseName(wantToLookup);
            QList<LookupItem> typedefCandidates = m_context.lookup(baseName, scope);
            if (!typedefCandidates.isEmpty()) {
                if (typedefCandidates.front().declaration()->isTypedef())
                    wantToLookup = baseName;
            }

            const QList<LookupItem> lookups = m_context.lookup(wantToLookup, scope);
            if (!lookups.empty()) {
                QList<const Name *> fullName
                    = m_context.fullyQualifiedName(lookups.first().declaration(),
                                                   LookupContext::HideInlineNamespaces);
                const int currentNameCount = countNames(wantToLookup);
                const bool needNamespace = needMissingNamespaces(std::move(fullName),
                                                                 currentNameCount);
                if (needNamespace)
                    insertMissingNamespace(ast);
            }
        }
        return true;
    }

    template<typename AST>
    void insertMissingNamespace(AST *ast)
    {
        DestructorNameAST *destructorName = ast->name->asDestructorName();
        if (destructorName)
            m_changeSet.insert(m_file->startOf(destructorName->unqualified_name), m_missingNamespace);
        else
            m_changeSet.insert(m_file->startOf(ast->name), m_missingNamespace);
        m_changeSet.operationList().last().setFormat1(false);
    }

    bool needMissingNamespaces(QList<const Name *> &&fullName, int currentNameCount)
    {
        if (currentNameCount > fullName.length())
            return false;

        // eg. fullName = std::vector, currentName = vector => result should be std
        fullName.erase(fullName.end() - currentNameCount, fullName.end());
        if (fullName.empty())
            return false;
        return nameEqual(m_namespace, fullName.last());
    }

    static bool nameEqual(const Name *name1, const Name *name2)
    {
        return Matcher::match(name1, name2);
    }

    QString toString(const Name *id)
    {
        const Identifier *identifier = id->asNameId();
        QTC_ASSERT(identifier, return {});
        return QString::fromUtf8(identifier->chars(), identifier->size());
    }

    const CppRefactoringFile *const m_file;
    const Snapshot &m_snapshot;

    const Name *m_namespace;          // the name of the namespace that should be removed
    const QString m_missingNamespace; // that should be added if a type was using the namespace
    LookupContext m_context;
    ChangeSet m_changeSet;
    const int m_symbolPos; // the end position of the start symbol
    bool m_done = false;
    bool m_start = false;
    // true if a using namespace was found at a scope and the scope should be left
    bool m_foundNamespace = false;
    bool m_removeAllAtGlobalScope;
    // the scope where the using namespace that should be removed is valid
    AST *m_parentNode = nullptr;
    int m_namespaceScopeCounter = 0;
};

class RemoveUsingNamespaceOperation : public CppQuickFixOperation
{
    struct Node
    {
        Document::Ptr document;
        bool hasGlobalUsingDirective = false;
        int unprocessedParents;
        std::vector<std::reference_wrapper<Node>> includes;
        std::vector<std::reference_wrapper<Node>> includedBy;
        Node() = default;
        Node(const Node &) = delete;
        Node(Node &&) = delete;
    };

public:
    RemoveUsingNamespaceOperation(const CppQuickFixInterface &interface,
                                  UsingDirectiveAST *usingDirective,
                                  bool removeAllAtGlobalScope)
        : CppQuickFixOperation(interface, 1)
        , m_usingDirective(usingDirective)
        , m_removeAllAtGlobalScope(removeAllAtGlobalScope)
    {
        const QString name = Overview{}.prettyName(usingDirective->name->name);
        if (m_removeAllAtGlobalScope) {
            setDescription(Tr::tr(
                               "Remove All Occurrences of \"using namespace %1\" in Global Scope "
                               "and Adjust Type Names Accordingly")
                               .arg(name));
        } else {
            setDescription(Tr::tr("Remove \"using namespace %1\" and "
                                  "Adjust Type Names Accordingly")
                               .arg(name));
        }
    }

private:
    std::map<Utils::FilePath, Node> buildIncludeGraph(CppRefactoringChanges &refactoring)
    {
        using namespace ProjectExplorer;
        using namespace Utils;

        const Snapshot &s = refactoring.snapshot();
        std::map<Utils::FilePath, Node> includeGraph;

        auto handleFile = [&](const FilePath &filePath, Document::Ptr doc, auto shouldHandle) {
            Node &node = includeGraph[filePath];
            node.document = doc;
            for (const Document::Include &include : doc->resolvedIncludes()) {
                const FilePath filePath = include.resolvedFileName();
                if (shouldHandle(filePath)) {
                    Node &includedNode = includeGraph[filePath];
                    includedNode.includedBy.push_back(node);
                    node.includes.push_back(includedNode);
                }
            }
        };

        if (const Project *project = ProjectManager::projectForFile(filePath())) {
            const FilePaths files = project->files(ProjectExplorer::Project::SourceFiles);
            QSet<FilePath> projectFiles(files.begin(), files.end());
            for (const auto &file : files) {
                const Document::Ptr doc = s.document(file);
                if (!doc)
                    continue;
                handleFile(file, doc, [&](const FilePath &file) {
                    return projectFiles.contains(file);
                });
            }
        } else {
            for (auto i = s.begin(); i != s.end(); ++i) {
                if (ProjectFile::classify(i.key().toString()) != ProjectFile::Unsupported) {
                    handleFile(i.key(), i.value(), [](const FilePath &file) {
                        return ProjectFile::classify(file.toString()) != ProjectFile::Unsupported;
                    });
                }
            }
        }
        for (auto &[_, node] : includeGraph) {
            Q_UNUSED(_)
            node.unprocessedParents = static_cast<int>(node.includes.size());
        }
        return includeGraph;
    }

    void removeAllUsingsAtGlobalScope(CppRefactoringChanges &refactoring)
    {
        auto includeGraph = buildIncludeGraph(refactoring);
        std::vector<std::reference_wrapper<Node>> nodesWithProcessedParents;
        for (auto &[_, node] : includeGraph) {
            Q_UNUSED(_)
            if (!node.unprocessedParents)
                nodesWithProcessedParents.push_back(node);
        }
        while (!nodesWithProcessedParents.empty()) {
            Node &node = nodesWithProcessedParents.back();
            nodesWithProcessedParents.pop_back();
            CppRefactoringFilePtr file = refactoring.cppFile(node.document->filePath());
            const bool parentHasUsing = Utils::anyOf(node.includes, &Node::hasGlobalUsingDirective);
            const int startPos = parentHasUsing
                                     ? 0
                                     : RemoveNamespaceVisitor::SearchGlobalUsingDirectivePos;
            const bool noGlobalUsing = refactorFile(file, refactoring.snapshot(), startPos);
            node.hasGlobalUsingDirective = !noGlobalUsing || parentHasUsing;

            for (Node &subNode : node.includedBy) {
                --subNode.unprocessedParents;
                if (subNode.unprocessedParents == 0)
                    nodesWithProcessedParents.push_back(subNode);
            }
        }
    }

    void perform() override
    {
        CppRefactoringChanges refactoring(snapshot());
        CppRefactoringFilePtr currentFile = refactoring.cppFile(filePath());
        if (m_removeAllAtGlobalScope) {
            removeAllUsingsAtGlobalScope(refactoring);
        } else if (refactorFile(currentFile,
                                refactoring.snapshot(),
                                currentFile->endOf(m_usingDirective),
                                true)) {
            processIncludes(refactoring, filePath());
        }

        for (auto &file : std::as_const(m_changes))
            file->apply();
    }

    /**
     * @brief refactorFile remove using namespace xyz in the given file and rewrite types
     * @param file The file that should be processed
     * @param snapshot The snapshot to work on
     * @param startSymbol start processing after this index
     * @param removeUsing if the using directive is in this file, remove it
     * @return true if the using statement is global and there is no other global using namespace
     */
    bool refactorFile(CppRefactoringFilePtr &file,
                      const Snapshot &snapshot,
                      int startSymbol,
                      bool removeUsing = false)
    {
        RemoveNamespaceVisitor visitor(file.get(),
                                       snapshot,
                                       m_usingDirective->name->name,
                                       startSymbol,
                                       m_removeAllAtGlobalScope);
        visitor.accept(file->cppDocument()->translationUnit()->ast());
        Utils::ChangeSet changes = visitor.getChanges();
        if (removeUsing)
            removeLine(file.get(), m_usingDirective, changes);
        if (!changes.isEmpty()) {
            file->setChangeSet(changes);
            // apply changes at the end, otherwise the symbol finder will fail to resolve symbols if
            // the using namespace is missing
            m_changes.insert(file);
        }
        return visitor.isGlobalUsingNamespace() && !visitor.foundGlobalUsingNamespace();
    }

    void processIncludes(CppRefactoringChanges &refactoring, const FilePath &filePath)
    {
        QList<Snapshot::IncludeLocation>
            includeLocationsOfDocument = refactoring.snapshot().includeLocationsOfDocument(filePath);
        for (Snapshot::IncludeLocation &loc : includeLocationsOfDocument) {
            if (!Utils::insert(m_processed, loc.first))
                continue;

            CppRefactoringFilePtr file = refactoring.cppFile(loc.first->filePath());
            const bool noGlobalUsing = refactorFile(file,
                                                    refactoring.snapshot(),
                                                    file->position(loc.second, 1));
            if (noGlobalUsing)
                processIncludes(refactoring, loc.first->filePath());
        }
    }

    QSet<Document::Ptr> m_processed;
    QSet<CppRefactoringFilePtr> m_changes;

    UsingDirectiveAST *m_usingDirective;
    bool m_removeAllAtGlobalScope;
};

//! Removes a using directive (using namespace xyz).
class RemoveUsingNamespace : public CppQuickFixFactory
{
public:
    RemoveUsingNamespace() { setClangdReplacement({10}); }
#ifdef WITH_TESTS
    static QObject *createTest();
#endif

private:
    void doMatch(const CppQuickFixInterface &interface, QuickFixOperations &result) override
    {
        const QList<AST *> &path = interface.path();
        // We expect something like
        // [0] TranslationUnitAST
        // ...
        // [] UsingDirectiveAST : if activated at 'using namespace'
        // [] NameAST (optional): if activated at the name e.g. 'std'
        int n = path.size() - 1;
        if (n <= 0)
            return;
        if (path.last()->asName())
            --n;
        UsingDirectiveAST *usingDirective = path.at(n)->asUsingDirective();
        if (usingDirective && usingDirective->name->name->asNameId()) {
            result << new RemoveUsingNamespaceOperation(interface, usingDirective, false);
            const bool isHeader = ProjectFile::isHeader(ProjectFile::classify(interface.filePath().toString()));
            if (isHeader && path.at(n - 1)->asTranslationUnit()) // using namespace at global scope
                result << new RemoveUsingNamespaceOperation(interface, usingDirective, true);
        }
    }
};

#ifdef WITH_TESTS
using namespace Tests;
class RemoveUsingNamespaceTest : public QObject
{
    Q_OBJECT

private slots:
    void test_data()
    {
        QTest::addColumn<QByteArray>("header1");
        QTest::addColumn<QByteArray>("header2");
        QTest::addColumn<QByteArray>("header3");
        QTest::addColumn<QByteArray>("expected1");
        QTest::addColumn<QByteArray>("expected2");
        QTest::addColumn<QByteArray>("expected3");
        QTest::addColumn<int>("operation");

        const QByteArray header1 = "namespace std{\n"
                                   "    template<typename T>\n"
                                   "    class vector{};\n"
                                   "    namespace chrono{\n"
                                   "        using seconds = int;\n"
                                   "    }\n"
                                   "}\n"
                                   "using namespace std;\n"
                                   "namespace test{\n"
                                   "    class vector{\n"
                                   "       std::vector<int> ints;\n"
                                   "    };\n"
                                   "}\n";
        const QByteArray header2 = "#include \"header1.h\"\n"
                                   "using foo = test::vector;\n"
                                   "using namespace std;\n"
                                   "using namespace test;\n"
                                   "vector<int> others;\n";

        const QByteArray header3 = "#include \"header2.h\"\n"
                                   "using namespace std;\n"
                                   "using namespace chrono;\n"
                                   "namespace test{\n"
                                   "   vector vec;\n"
                                   "   seconds t;\n"
                                   "}\n"
                                   "void scope(){\n"
                                   "    for (;;) {\n"
                                   "        using namespace std;\n"
                                   "        vector<int> fori;\n"
                                   "    }\n"
                                   "    vector<int> no;\n"
                                   "    using namespace std;\n"
                                   "    vector<int> _no_change;\n"
                                   "}\n"
                                   "foo foos;\n";

        QByteArray h3 = "#include \"header2.h\"\n"
                        "using namespace s@td;\n"
                        "using namespace chrono;\n"
                        "namespace test{\n"
                        "   vector vec;\n"
                        "   seconds t;\n"
                        "}\n"
                        "void scope(){\n"
                        "    for (;;) {\n"
                        "        using namespace std;\n"
                        "        vector<int> fori;\n"
                        "    }\n"
                        "    vector<int> no;\n"
                        "    using namespace std;\n"
                        "    vector<int> _no_change;\n"
                        "}\n"
                        "foo foos;\n";

        // like header1 but without "using namespace std;\n"
        QByteArray expected1 = "namespace std{\n"
                               "    template<typename T>\n"
                               "    class vector{};\n"
                               "    namespace chrono{\n"
                               "        using seconds = int;\n"
                               "    }\n"
                               "}\n"
                               "namespace test{\n"
                               "    class vector{\n"
                               "       std::vector<int> ints;\n"
                               "    };\n"
                               "}\n";

        // like header2 but without "using namespace std;\n" and with std::vector
        QByteArray expected2 = "#include \"header1.h\"\n"
                               "using foo = test::vector;\n"
                               "using namespace test;\n"
                               "std::vector<int> others;\n";

        QByteArray expected3 = "#include \"header2.h\"\n"
                               "using namespace std::chrono;\n"
                               "namespace test{\n"
                               "   vector vec;\n"
                               "   seconds t;\n"
                               "}\n"
                               "void scope(){\n"
                               "    for (;;) {\n"
                               "        using namespace std;\n"
                               "        vector<int> fori;\n"
                               "    }\n"
                               "    std::vector<int> no;\n"
                               "    using namespace std;\n"
                               "    vector<int> _no_change;\n"
                               "}\n"
                               "foo foos;\n";

        QTest::newRow("remove only in one file local")
            << header1 << header2 << h3 << header1 << header2 << expected3 << 0;
        QTest::newRow("remove only in one file globally")
            << header1 << header2 << h3 << expected1 << expected2 << expected3 << 1;

        QByteArray h2 = "#include \"header1.h\"\n"
                        "using foo = test::vector;\n"
                        "using namespace s@td;\n"
                        "using namespace test;\n"
                        "vector<int> others;\n";

        QTest::newRow("remove across two files only this")
            << header1 << h2 << header3 << header1 << expected2 << header3 << 0;
        QTest::newRow("remove across two files globally1")
            << header1 << h2 << header3 << expected1 << expected2 << expected3 << 1;

        QByteArray h1 = "namespace std{\n"
                        "    template<typename T>\n"
                        "    class vector{};\n"
                        "    namespace chrono{\n"
                        "        using seconds = int;\n"
                        "    }\n"
                        "}\n"
                        "using namespace s@td;\n"
                        "namespace test{\n"
                        "    class vector{\n"
                        "       std::vector<int> ints;\n"
                        "    };\n"
                        "}\n";

        QTest::newRow("remove across tree files only this")
            << h1 << header2 << header3 << expected1 << header2 << header3 << 0;
        QTest::newRow("remove across tree files globally")
            << h1 << header2 << header3 << expected1 << expected2 << expected3 << 1;

        expected3 = "#include \"header2.h\"\n"
                    "using namespace std::chrono;\n"
                    "namespace test{\n"
                    "   vector vec;\n"
                    "   seconds t;\n"
                    "}\n"
                    "void scope(){\n"
                    "    for (;;) {\n"
                    "        using namespace s@td;\n"
                    "        vector<int> fori;\n"
                    "    }\n"
                    "    std::vector<int> no;\n"
                    "    using namespace std;\n"
                    "    vector<int> _no_change;\n"
                    "}\n"
                    "foo foos;\n";

        QByteArray expected3_new = "#include \"header2.h\"\n"
                                   "using namespace std::chrono;\n"
                                   "namespace test{\n"
                                   "   vector vec;\n"
                                   "   seconds t;\n"
                                   "}\n"
                                   "void scope(){\n"
                                   "    for (;;) {\n"
                                   "        std::vector<int> fori;\n"
                                   "    }\n"
                                   "    std::vector<int> no;\n"
                                   "    using namespace std;\n"
                                   "    vector<int> _no_change;\n"
                                   "}\n"
                                   "foo foos;\n";

        QTest::newRow("scoped remove")
            << expected1 << expected2 << expected3 << expected1 << expected2 << expected3_new << 0;

        h2 = "#include \"header1.h\"\n"
             "using foo = test::vector;\n"
             "using namespace std;\n"
             "using namespace t@est;\n"
             "vector<int> others;\n";
        expected2 = "#include \"header1.h\"\n"
                    "using foo = test::vector;\n"
                    "using namespace std;\n"
                    "vector<int> others;\n";

        QTest::newRow("existing namespace")
            << header1 << h2 << header3 << header1 << expected2 << header3 << 1;

        // test: remove using directive at global scope in every file
        h1 = "using namespace tes@t;";
        h2 = "using namespace test;";
        h3 = "using namespace test;";

        expected1 = expected2 = expected3 = "";
        QTest::newRow("global scope remove in every file")
            << h1 << h2 << h3 << expected1 << expected2 << expected3 << 1;

        // test: dont print inline namespaces
        h1 = R"--(
namespace test {
  inline namespace test {
    class Foo{
      void foo1();
      void foo2();
    };
    inline int TEST = 42;
  }
}
)--";
        h2 = R"--(
#include "header1.h"
using namespace tes@t;
)--";
        h3 = R"--(
#include "header2.h"
Foo f1;
test::Foo f2;
using T1 = Foo;
using T2 = test::Foo;
int i1 = TEST;
int i2 = test::TEST;
void Foo::foo1(){};
void test::Foo::foo2(){};
)--";

        expected1 = h1;
        expected2 = R"--(
#include "header1.h"
)--";
        expected3 = R"--(
#include "header2.h"
test::Foo f1;
test::Foo f2;
using T1 = test::Foo;
using T2 = test::Foo;
int i1 = test::TEST;
int i2 = test::TEST;
void test::Foo::foo1(){};
void test::Foo::foo2(){};
)--";
        QTest::newRow("don't insert inline namespaces")
            << h1 << h2 << h3 << expected1 << expected2 << expected3 << 0;
    }

    void test()
    {
        QFETCH(QByteArray, header1);
        QFETCH(QByteArray, header2);
        QFETCH(QByteArray, header3);
        QFETCH(QByteArray, expected1);
        QFETCH(QByteArray, expected2);
        QFETCH(QByteArray, expected3);
        QFETCH(int, operation);

        QList<TestDocumentPtr> testDocuments;
        testDocuments << CppTestDocument::create("header1.h", header1, expected1);
        testDocuments << CppTestDocument::create("header2.h", header2, expected2);
        testDocuments << CppTestDocument::create("header3.h", header3, expected3);

        RemoveUsingNamespace factory;
        QuickFixOperationTest(testDocuments, &factory, ProjectExplorer::HeaderPaths(), operation);
    }

    void testSimple_data()
    {
        QTest::addColumn<QByteArray>("header");
        QTest::addColumn<QByteArray>("expected");

        const QByteArray common = R"--(
namespace N{
    template<typename T>
    struct vector{
        using iterator = T*;
    };
    using int_vector = vector<int>;
}
)--";
        const QByteArray header = common + R"--(
using namespace N@;
int_vector ints;
int_vector::iterator intIter;
using vec = vector<int>;
vec::iterator it;
)--";
        const QByteArray expected = common + R"--(
N::int_vector ints;
N::int_vector::iterator intIter;
using vec = N::vector<int>;
vec::iterator it;
)--";

        QTest::newRow("nested typedefs with Namespace") << header << expected;
    }

    void testSimple()
    {
        QFETCH(QByteArray, header);
        QFETCH(QByteArray, expected);

        QList<TestDocumentPtr> testDocuments;
        testDocuments << CppTestDocument::create("header.h", header, expected);

        RemoveUsingNamespace factory;
        QuickFixOperationTest(testDocuments, &factory, ProjectExplorer::HeaderPaths());
    }

    void testDifferentSymbols()
    {
        QByteArray header = "namespace test{\n"
                            "  struct foo{\n"
                            "    foo();\n"
                            "    void bar();\n"
                            "  };\n"
                            "  void func();\n"
                            "  enum E {E1, E2};\n"
                            "  int bar;\n"
                            "}\n"
                            "using namespace t@est;\n"
                            "foo::foo(){}\n"
                            "void foo::bar(){}\n"
                            "void test(){\n"
                            "  int i = bar * 4;\n"
                            "  E val = E1;\n"
                            "  auto p = &foo::bar;\n"
                            "  func()\n"
                            "}\n";
        QByteArray expected = "namespace test{\n"
                              "  struct foo{\n"
                              "    foo();\n"
                              "    void bar();\n"
                              "  };\n"
                              "  void func();\n"
                              "  enum E {E1, E2};\n"
                              "  int bar;\n"
                              "}\n"
                              "test::foo::foo(){}\n"
                              "void test::foo::bar(){}\n"
                              "void test(){\n"
                              "  int i = test::bar * 4;\n"
                              "  test::E val = test::E1;\n"
                              "  auto p = &test::foo::bar;\n"
                              "  test::func()\n"
                              "}\n";

        QList<TestDocumentPtr> testDocuments;
        testDocuments << CppTestDocument::create("file.h", header, expected);
        RemoveUsingNamespace factory;
        QuickFixOperationTest(testDocuments, &factory, ProjectExplorer::HeaderPaths(), 0);
    }
};

QObject *RemoveUsingNamespace::createTest() { return new RemoveUsingNamespaceTest; }
#endif // WITH_TESTS

} // namespace

void registerRemoveUsingNamespaceQuickfix()
{
    CppQuickFixFactory::registerFactory<RemoveUsingNamespace>();
}

} // namespace CppEditor::Internal

#ifdef WITH_TESTS
#include <removeusingnamespace.moc>
#endif
