// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "bringidentifierintoscope.h"

#include "../cppeditortr.h"
#include "../cpplocatordata.h"
#include "../cppprojectfile.h"
#include "../cpprefactoringchanges.h"
#include "../indexitem.h"
#include "../insertionpointlocator.h"
#include "cppquickfix.h"
#include "cppquickfixhelpers.h"

#include <cplusplus/Overview.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/projecttree.h>
#include <texteditor/quickfix.h>

#ifdef WITH_TESTS
#include "../cppsourceprocessertesthelper.h"
#include "../cpptoolstestcase.h"
#include "cppquickfix_test.h"
#include <QtTest>
#endif

using namespace CPlusPlus;
using namespace ProjectExplorer;
using namespace Utils;

#ifdef WITH_TESTS
namespace CppEditor::Internal::Tests {
typedef QList<TestDocumentPtr> QuickFixTestDocuments;
}
Q_DECLARE_METATYPE(CppEditor::Internal::Tests::QuickFixTestDocuments)
#endif

namespace CppEditor::Internal {
namespace {

static QString findShortestInclude(
    const QString currentDocumentFilePath,
    const QString candidateFilePath,
    const HeaderPaths &headerPaths)
{
    QString result;

    const QFileInfo fileInfo(candidateFilePath);

    if (fileInfo.path() == QFileInfo(currentDocumentFilePath).path()) {
        result = QLatin1Char('"') + fileInfo.fileName() + QLatin1Char('"');
    } else {
        for (const HeaderPath &headerPath : headerPaths) {
            if (!candidateFilePath.startsWith(headerPath.path))
                continue;
            QString relativePath = candidateFilePath.mid(headerPath.path.size());
            if (!relativePath.isEmpty() && relativePath.at(0) == QLatin1Char('/'))
                relativePath = relativePath.mid(1);
            if (result.isEmpty() || relativePath.size() + 2 < result.size())
                result = QLatin1Char('<') + relativePath + QLatin1Char('>');
        }
    }

    return result;
}

static QString findMatchingInclude(const QString &className, const HeaderPaths &headerPaths)
{
    const QStringList candidateFileNames{
        className,
        className + ".h",
        className + ".hpp",
        className.toLower(),
        className.toLower() + ".h",
        className.toLower() + ".hpp"};
    for (const QString &fileName : candidateFileNames) {
        for (const HeaderPath &headerPath : headerPaths) {
            const QString headerPathCandidate = headerPath.path + QLatin1Char('/') + fileName;
            const QFileInfo fileInfo(headerPathCandidate);
            if (fileInfo.exists() && fileInfo.isFile())
                return '<' + fileName + '>';
        }
    }
    return {};
}

static HeaderPaths relevantHeaderPaths(const QString &filePath)
{
    HeaderPaths headerPaths;

    const QList<ProjectPart::ConstPtr> projectParts = CppModelManager::projectPart(filePath);
    if (projectParts.isEmpty()) { // Not part of any project, better use all include paths than none
        headerPaths += CppModelManager::headerPaths();
    } else {
        for (const ProjectPart::ConstPtr &part : projectParts)
            headerPaths += part->headerPaths;
    }

    return headerPaths;
}

static NameAST *nameUnderCursor(const QList<AST *> &path)
{
    if (path.isEmpty())
        return nullptr;

    NameAST *nameAst = nullptr;
    for (int i = path.size() - 1; i >= 0; --i) {
        AST * const ast = path.at(i);
        if (SimpleNameAST *simpleName = ast->asSimpleName()) {
            nameAst = simpleName;
        } else if (TemplateIdAST *templateId = ast->asTemplateId()) {
            nameAst = templateId;
        } else if (nameAst && ast->asNamedTypeSpecifier()) {
            break; // Stop at "Foo" for "N::Bar<@Foo>"
        } else if (QualifiedNameAST *qualifiedName = ast->asQualifiedName()) {
            nameAst = qualifiedName;
            break;
        }
    }

    return nameAst;
}

enum class LookupResult { Declared, ForwardDeclared, NotDeclared };
static LookupResult lookUpDefinition(const CppQuickFixInterface &interface, const NameAST *nameAst)
{
    QTC_ASSERT(nameAst && nameAst->name, return LookupResult::NotDeclared);

    // Find the enclosing scope
    int line, column;
    const Document::Ptr doc = interface.semanticInfo().doc;
    doc->translationUnit()->getTokenPosition(nameAst->firstToken(), &line, &column);
    Scope *scope = doc->scopeAt(line, column);
    if (!scope)
        return LookupResult::NotDeclared;

    // Try to find the class/template definition
    const Name *name = nameAst->name;
    const QList<LookupItem> results = interface.context().lookup(name, scope);
    LookupResult best = LookupResult::NotDeclared;
    for (const LookupItem &item : results) {
        if (Symbol *declaration = item.declaration()) {
            if (declaration->asClass())
                return LookupResult::Declared;
            if (declaration->asForwardClassDeclaration()) {
                best = LookupResult::ForwardDeclared;
                continue;
            }
            if (Template *templ = declaration->asTemplate()) {
                if (Symbol *declaration = templ->declaration()) {
                    if (declaration->asClass())
                        return LookupResult::Declared;
                    if (declaration->asForwardClassDeclaration()) {
                        best = LookupResult::ForwardDeclared;
                        continue;
                    }
                }
            }
            return LookupResult::Declared;
        }
    }

    return best;
}

static QString templateNameAsString(const TemplateNameId *templateName)
{
    const Identifier *id = templateName->identifier();
    return QString::fromUtf8(id->chars(), id->size());
}

static Snapshot forwardingHeaders(const CppQuickFixInterface &interface)
{
    Snapshot result;

    const Snapshot docs = interface.snapshot();
    for (Document::Ptr doc : docs) {
        if (doc->globalSymbolCount() == 0 && doc->resolvedIncludes().size() == 1)
            result.insert(doc);
    }

    return result;
}

static QList<IndexItem::Ptr> matchName(const Name *name, QString *className)
{
    if (!name)
        return {};

    QString simpleName;
    QList<IndexItem::Ptr> matches;
    CppLocatorData *locatorData = CppModelManager::locatorData();
    const Overview oo;
    if (const QualifiedNameId *qualifiedName = name->asQualifiedNameId()) {
        const Name *name = qualifiedName->name();
        if (const TemplateNameId *templateName = name->asTemplateNameId()) {
            *className = templateNameAsString(templateName);
        } else {
            simpleName = oo.prettyName(name);
            *className = simpleName;
            matches = locatorData->findSymbols(IndexItem::Class, *className);
            if (matches.isEmpty()) {
                if (const Name *name = qualifiedName->base()) {
                    if (const TemplateNameId *templateName = name->asTemplateNameId())
                        *className = templateNameAsString(templateName);
                    else
                        *className = oo.prettyName(name);
                }
            }
        }
    } else if (const TemplateNameId *templateName = name->asTemplateNameId()) {
        *className = templateNameAsString(templateName);
    } else {
        *className = oo.prettyName(name);
    }

    if (matches.isEmpty())
        matches = locatorData->findSymbols(IndexItem::Class, *className);

    if (matches.isEmpty() && !simpleName.isEmpty())
        *className = simpleName;

    return matches;
}

class AddIncludeForUndefinedIdentifierOp : public CppQuickFixOperation
{
public:
    AddIncludeForUndefinedIdentifierOp(const CppQuickFixInterface &interface, int priority,
                                       const QString &include);
    void perform() override;

    QString include() const { return m_include; }

private:
    QString m_include;
};

class AddForwardDeclForUndefinedIdentifierOp : public CppQuickFixOperation
{
public:
    AddForwardDeclForUndefinedIdentifierOp(const CppQuickFixInterface &interface, int priority,
                                           const QString &fqClassName, int symbolPos);
private:
    void perform() override
    {
        const QStringList parts = m_className.split("::");
        QTC_ASSERT(!parts.isEmpty(), return);
        const QStringList namespaces = parts.mid(0, parts.length() - 1);

        CppRefactoringFilePtr file = currentFile();

        NSVisitor visitor(file.data(), namespaces, m_symbolPos);
        visitor.accept(file->cppDocument()->translationUnit()->ast());
        const auto stringToInsert = [&visitor, symbol = parts.last()] {
            QString s = "\n";
            for (const QString &ns : visitor.remainingNamespaces())
                s += "namespace " + ns + " { ";
            s += "class " + symbol + ';';
            for (int i = 0; i < visitor.remainingNamespaces().size(); ++i)
                s += " }";
            return s;
        };

        int insertPos = 0;

        // Find the position to insert:
        //   If we have a matching namespace, we do the insertion there.
        //   If we don't have a matching namespace, but there is another namespace in the file,
        //   we assume that to be a good position for our insertion.
        //   Otherwise, do the insertion after the last include that comes before the use of the symbol.
        //   If there is no such include, do the insertion before the first token.
        if (visitor.enclosingNamespace()) {
            insertPos = file->startOf(visitor.enclosingNamespace()->linkage_body) + 1;
        } else if (visitor.firstNamespace()) {
            insertPos = file->startOf(visitor.firstNamespace());
        } else {
            static const QRegularExpression regexp("^\\s*#include .*$");
            const QTextCursor tc = file->document()->find(
                regexp, m_symbolPos,
                QTextDocument::FindBackward | QTextDocument::FindCaseSensitively);
            if (!tc.isNull())
                insertPos = tc.position() + 1;
            else if (visitor.firstToken())
                insertPos = file->startOf(visitor.firstToken());
        }

        QString insertion = stringToInsert();
        if (file->charAt(insertPos - 1) != QChar::ParagraphSeparator)
            insertion.prepend('\n');
        if (file->charAt(insertPos) != QChar::ParagraphSeparator)
            insertion.append('\n');
        file->apply(ChangeSet::makeInsert(insertPos, insertion));
    }

    const QString m_className;
    const int m_symbolPos;
};

AddIncludeForUndefinedIdentifierOp::AddIncludeForUndefinedIdentifierOp(
    const CppQuickFixInterface &interface, int priority, const QString &include)
    : CppQuickFixOperation(interface, priority)
    , m_include(include)
{
    setDescription(Tr::tr("Add #include %1").arg(m_include));
}

void AddIncludeForUndefinedIdentifierOp::perform()
{
    CppRefactoringFilePtr file = currentFile();

    ChangeSet changes;
    insertNewIncludeDirective(m_include, file, semanticInfo().doc, changes);
    file->apply(changes);
}

AddForwardDeclForUndefinedIdentifierOp::AddForwardDeclForUndefinedIdentifierOp(
    const CppQuickFixInterface &interface,
    int priority,
    const QString &fqClassName,
    int symbolPos)
    : CppQuickFixOperation(interface, priority), m_className(fqClassName), m_symbolPos(symbolPos)
{
    setDescription(Tr::tr("Add Forward Declaration for %1").arg(m_className));
}

/*!
  Adds an include for an undefined identifier or only forward declared identifier.
  Activates on: the undefined identifier
*/
class AddIncludeForUndefinedIdentifier : public CppQuickFixFactory
{
#ifdef WITH_TESTS
public:
    static QObject *createTest();
#endif

private:
    void doMatch(const CppQuickFixInterface &interface, QuickFixOperations &result) override
    {
        const NameAST *nameAst = nameUnderCursor(interface.path());
        if (!nameAst || !nameAst->name)
            return;

        const LookupResult lookupResult = lookUpDefinition(interface, nameAst);
        if (lookupResult == LookupResult::Declared)
            return;

        QString className;
        const QString currentDocumentFilePath = interface.semanticInfo().doc->filePath().toString();
        const HeaderPaths headerPaths = relevantHeaderPaths(currentDocumentFilePath);
        FilePaths headers;

        const QList<IndexItem::Ptr> matches = matchName(nameAst->name, &className);
        // Find an include file through the locator
        if (!matches.isEmpty()) {
            QList<IndexItem::Ptr> indexItems;
            const Snapshot forwardHeaders = forwardingHeaders(interface);
            for (const IndexItem::Ptr &info : matches) {
                if (!info || info->symbolName() != className)
                    continue;
                indexItems << info;

                Snapshot localForwardHeaders = forwardHeaders;
                localForwardHeaders.insert(interface.snapshot().document(info->filePath()));
                FilePaths headerAndItsForwardingHeaders;
                headerAndItsForwardingHeaders << info->filePath();
                headerAndItsForwardingHeaders += localForwardHeaders.filesDependingOn(info->filePath());

                for (const FilePath &header : std::as_const(headerAndItsForwardingHeaders)) {
                    const QString include = findShortestInclude(currentDocumentFilePath,
                                                                header.toString(),
                                                                headerPaths);
                    if (include.size() > 2) {
                        const QString headerFileName = info->filePath().fileName();
                        QTC_ASSERT(!headerFileName.isEmpty(), break);

                        int priority = 0;
                        if (headerFileName == className)
                            priority = 2;
                        else if (headerFileName.at(1).isUpper())
                            priority = 1;

                        result << new AddIncludeForUndefinedIdentifierOp(interface, priority,
                                                                         include);
                        headers << header;
                    }
                }
            }

            if (lookupResult == LookupResult::NotDeclared && indexItems.size() == 1) {
                QString qualifiedName = Overview().prettyName(nameAst->name);
                if (qualifiedName.startsWith("::"))
                    qualifiedName.remove(0, 2);
                if (indexItems.first()->scopedSymbolName().endsWith(qualifiedName)) {
                    const Node * const node = ProjectTree::nodeForFile(interface.filePath());
                    FileType fileType = node && node->asFileNode() ? node->asFileNode()->fileType()
                                                                   : FileType::Unknown;
                    if (fileType == FileType::Unknown
                        && ProjectFile::isHeader(ProjectFile::classify(interface.filePath()))) {
                        fileType = FileType::Header;
                    }
                    if (fileType == FileType::Header) {
                        result << new AddForwardDeclForUndefinedIdentifierOp(
                            interface, 0, indexItems.first()->scopedSymbolName(),
                            interface.currentFile()->startOf(nameAst));
                    }
                }
            }
        }

        if (className.isEmpty())
            return;

        // Fallback: Check the include paths for files that look like candidates
        //           for the given name.
        if (!Utils::contains(headers,
                             [&className](const FilePath &fp) { return fp.fileName() == className; })) {
            const QString include = findMatchingInclude(className, headerPaths);
            const auto matcher = [&include](const TextEditor::QuickFixOperation::Ptr &o) {
                const auto includeOp = o.dynamicCast<AddIncludeForUndefinedIdentifierOp>();
                return includeOp && includeOp->include() == include;
            };
            if (!include.isEmpty() && !contains(result, matcher))
                result << new AddIncludeForUndefinedIdentifierOp(interface, 1, include);
        }
    }
};

#ifdef WITH_TESTS
using namespace Tests;
using namespace ::CppEditor::Tests;
using namespace ::CppEditor::Tests::Internal; // FIXME: Get rid of this nonsense namespace

/// Delegates directly to AddIncludeForUndefinedIdentifierOp for easier testing.
class AddIncludeForUndefinedIdentifierTestFactory : public CppQuickFixFactory
{
public:
    AddIncludeForUndefinedIdentifierTestFactory(const QString &include)
        : m_include(include) {}

    void doMatch(const CppQuickFixInterface &cppQuickFixInterface, QuickFixOperations &result) override
    {
        result << new AddIncludeForUndefinedIdentifierOp(cppQuickFixInterface, 0, m_include);
    }

private:
    const QString m_include;
};

class AddForwardDeclForUndefinedIdentifierTestFactory : public CppQuickFixFactory
{
public:
    AddForwardDeclForUndefinedIdentifierTestFactory(const QString &className, int symbolPos)
        : m_className(className), m_symbolPos(symbolPos) {}

    void doMatch(const CppQuickFixInterface &cppQuickFixInterface, QuickFixOperations &result) override
    {
        result << new AddForwardDeclForUndefinedIdentifierOp(cppQuickFixInterface, 0,
                                                             m_className, m_symbolPos);
    }

private:
    const QString m_className;
    const int m_symbolPos;
};

class BringIdentifierIntoScopeTest : public QObject
{
    Q_OBJECT

private slots:
    void testAddIncludeForUndefinedIdentifier_data()
    {
        QTest::addColumn<QString>("headerPath");
        QTest::addColumn<QuickFixTestDocuments>("testDocuments");
        QTest::addColumn<int>("refactoringOperationIndex");
        QTest::addColumn<QString>("includeForTestFactory");

        const int firstRefactoringOperation = 0;
        const int secondRefactoringOperation = 1;

        QList<TestDocumentPtr> testDocuments;
        QByteArray original;
        QByteArray expected;

        // -------------------------------------------------------------------------------------------

        // Header File
        original = "class Foo {};\n";
        expected = original;
        testDocuments << CppTestDocument::create("afile.h", original, expected);

        // Source File
        original =
            "#include \"header.h\"\n"
            "\n"
            "void f()\n"
            "{\n"
            "    Fo@o foo;\n"
            "}\n"
            ;
        expected =
            "#include \"afile.h\"\n"
            "#include \"header.h\"\n"
            "\n"
            "void f()\n"
            "{\n"
            "    Foo foo;\n"
            "}\n"
            ;
        testDocuments << CppTestDocument::create("afile.cpp", original, expected);
        QTest::newRow("onSimpleName")
            << TestIncludePaths::globalIncludePath()
            << testDocuments << firstRefactoringOperation << "";
        testDocuments.clear();

        // -------------------------------------------------------------------------------------------

        // Header File
        original = "namespace N { class Foo {}; }\n";
        expected = original;
        testDocuments << CppTestDocument::create("afile.h", original, expected);

        // Source File
        original =
            "#include \"header.h\"\n"
            "\n"
            "void f()\n"
            "{\n"
            "    N::Fo@o foo;\n"
            "}\n"
            ;
        expected =
            "#include \"afile.h\"\n"
            "#include \"header.h\"\n"
            "\n"
            "void f()\n"
            "{\n"
            "    N::Foo foo;\n"
            "}\n"
            ;
        testDocuments << CppTestDocument::create("afile.cpp", original, expected);
        QTest::newRow("onNameOfQualifiedName")
            << TestIncludePaths::globalIncludePath()
            << testDocuments << firstRefactoringOperation << "";
        testDocuments.clear();

        // -------------------------------------------------------------------------------------------

        // Header File
        original = "namespace N { class Foo {}; }\n";
        expected = original;
        testDocuments << CppTestDocument::create("afile.h", original, expected);

        // Source File
        original =
            "#include \"header.h\"\n"
            "\n"
            "void f()\n"
            "{\n"
            "    @N::Foo foo;\n"
            "}\n"
            ;
        expected =
            "#include \"afile.h\"\n"
            "#include \"header.h\"\n"
            "\n"
            "void f()\n"
            "{\n"
            "    N::Foo foo;\n"
            "}\n"
            ;
        testDocuments << CppTestDocument::create("afile.cpp", original, expected);
        QTest::newRow("onBaseOfQualifiedName")
            << TestIncludePaths::globalIncludePath()
            << testDocuments << firstRefactoringOperation << "";
        testDocuments.clear();

        // -------------------------------------------------------------------------------------------

        // Header File
        original = "class Foo { static void bar() {} };\n";
        expected = original;
        testDocuments << CppTestDocument::create("afile.h", original, expected);

        // Source File
        original =
            "#include \"header.h\"\n"
            "\n"
            "void f()\n"
            "{\n"
            "    @Foo::bar();\n"
            "}\n"
            ;
        expected =
            "#include \"afile.h\"\n"
            "#include \"header.h\"\n"
            "\n"
            "void f()\n"
            "{\n"
            "    Foo::bar();\n"
            "}\n"
            ;
        testDocuments << CppTestDocument::create("afile.cpp", original, expected);
        QTest::newRow("onBaseOfQualifiedClassName")
            << TestIncludePaths::globalIncludePath()
            << testDocuments << firstRefactoringOperation << "";
        testDocuments.clear();

        // -------------------------------------------------------------------------------------------

        // Header File
        original = "template <typename T> class Foo { static void bar() {} };\n";
        expected = original;
        testDocuments << CppTestDocument::create("afile.h", original, expected);

        // Source File
        original =
            "#include \"header.h\"\n"
            "\n"
            "void f()\n"
            "{\n"
            "    @Foo<int>::bar();\n"
            "}\n"
            ;
        expected =
            "#include \"afile.h\"\n"
            "#include \"header.h\"\n"
            "\n"
            "void f()\n"
            "{\n"
            "    Foo<int>::bar();\n"
            "}\n"
            ;
        testDocuments << CppTestDocument::create("afile.cpp", original, expected);
        QTest::newRow("onBaseOfQualifiedTemplateClassName")
            << TestIncludePaths::globalIncludePath()
            << testDocuments << firstRefactoringOperation << "";
        testDocuments.clear();

        // -------------------------------------------------------------------------------------------

        // Header File
        original = "namespace N { template <typename T> class Foo {}; }\n";
        expected = original;
        testDocuments << CppTestDocument::create("afile.h", original, expected);

        // Source File
        original =
            "#include \"header.h\"\n"
            "\n"
            "void f()\n"
            "{\n"
            "    @N::Foo<Bar> foo;\n"
            "}\n"
            ;
        expected =
            "#include \"afile.h\"\n"
            "#include \"header.h\"\n"
            "\n"
            "void f()\n"
            "{\n"
            "    N::Foo<Bar> foo;\n"
            "}\n"
            ;
        testDocuments << CppTestDocument::create("afile.cpp", original, expected);
        QTest::newRow("onTemplateName")
            << TestIncludePaths::globalIncludePath()
            << testDocuments << firstRefactoringOperation << "";
        testDocuments.clear();

        // -------------------------------------------------------------------------------------------

        // Header File
        original = "namespace N { template <typename T> class Foo {}; }\n";
        expected = original;
        testDocuments << CppTestDocument::create("afile.h", original, expected);

        // Source File
        original =
            "#include \"header.h\"\n"
            "\n"
            "void f()\n"
            "{\n"
            "    N::Bar<@Foo> foo;\n"
            "}\n"
            ;
        expected =
            "#include \"afile.h\"\n"
            "#include \"header.h\"\n"
            "\n"
            "void f()\n"
            "{\n"
            "    N::Bar<Foo> foo;\n"
            "}\n"
            ;
        testDocuments << CppTestDocument::create("afile.cpp", original, expected);
        QTest::newRow("onTemplateNameInsideArguments")
            << TestIncludePaths::globalIncludePath()
            << testDocuments << firstRefactoringOperation << "";
        testDocuments.clear();

        // -------------------------------------------------------------------------------------------

        // Header File
        original = "class Foo {};\n";
        expected = original;
        testDocuments << CppTestDocument::create("afile.h", original, expected);

        // Source File
        original =
            "#include \"header.h\"\n"
            "\n"
            "class Foo;\n"
            "\n"
            "void f()\n"
            "{\n"
            "    @Foo foo;\n"
            "}\n"
            ;
        expected =
            "#include \"afile.h\"\n"
            "#include \"header.h\"\n"
            "\n"
            "class Foo;\n"
            "\n"
            "void f()\n"
            "{\n"
            "    Foo foo;\n"
            "}\n"
            ;
        testDocuments << CppTestDocument::create("afile.cpp", original, expected);
        QTest::newRow("withForwardDeclaration")
            << TestIncludePaths::globalIncludePath()
            << testDocuments << firstRefactoringOperation << "";
        testDocuments.clear();

        // -------------------------------------------------------------------------------------------

        // Header File
        original = "template<class T> class Foo {};\n";
        expected = original;
        testDocuments << CppTestDocument::create("afile.h", original, expected);

        // Source File
        original =
            "#include \"header.h\"\n"
            "\n"
            "template<class T> class Foo;\n"
            "\n"
            "void f()\n"
            "{\n"
            "    @Foo foo;\n"
            "}\n"
            ;
        expected =
            "#include \"afile.h\"\n"
            "#include \"header.h\"\n"
            "\n"
            "template<class T> class Foo;\n"
            "\n"
            "void f()\n"
            "{\n"
            "    Foo foo;\n"
            "}\n"
            ;
        testDocuments << CppTestDocument::create("afile.cpp", original, expected);
        QTest::newRow("withForwardDeclaration2")
            << TestIncludePaths::globalIncludePath()
            << testDocuments << firstRefactoringOperation << "";
        testDocuments.clear();

        // -------------------------------------------------------------------------------------------

        // Header File
        original = "template<class T> class QMyClass {};\n";
        expected = original;
        testDocuments << CppTestDocument::create("qmyclass.h", original, expected);

        // Forward Header File
        original = "#include \"qmyclass.h\"\n";
        expected = original;
        testDocuments << CppTestDocument::create("QMyClass", original, expected);

        // Source File
        original =
            "#include \"header.h\"\n"
            "\n"
            "void f()\n"
            "{\n"
            "    @QMyClass c;\n"
            "}\n"
            ;
        expected =
            "#include \"QMyClass\"\n"
            "#include \"header.h\"\n"
            "\n"
            "void f()\n"
            "{\n"
            "    QMyClass c;\n"
            "}\n"
            ;
        testDocuments << CppTestDocument::create("afile.cpp", original, expected);
        QTest::newRow("withForwardHeader")
            << TestIncludePaths::globalIncludePath()
            << testDocuments << secondRefactoringOperation << "";
        testDocuments.clear();

        // -------------------------------------------------------------------------------------------

        original =
            "void @f();\n"
            "#include \"file.moc\";\n"
            ;
        expected =
            "#include \"file.h\"\n"
            "\n"
            "void f();\n"
            "#include \"file.moc\";\n"
            ;
        testDocuments << CppTestDocument::create("file.cpp", original, expected);
        QTest::newRow("insertingIgnoreMoc")
            << TestIncludePaths::globalIncludePath()
            << testDocuments << firstRefactoringOperation << "\"file.h\"";
        testDocuments.clear();

        // -------------------------------------------------------------------------------------------

        original =
            "#include \"y.h\"\n"
            "#include \"z.h\"\n"
            "\n@"
            ;
        expected =
            "#include \"file.h\"\n"
            "#include \"y.h\"\n"
            "#include \"z.h\"\n"
            "\n"
            ;
        testDocuments << CppTestDocument::create("file.cpp", original, expected);
        QTest::newRow("insertingSortingTop")
            << TestIncludePaths::globalIncludePath()
            << testDocuments << firstRefactoringOperation << "\"file.h\"";
        testDocuments.clear();

        // -------------------------------------------------------------------------------------------

        original =
            "#include \"a.h\"\n"
            "#include \"z.h\"\n"
            "\n@"
            ;
        expected =
            "#include \"a.h\"\n"
            "#include \"file.h\"\n"
            "#include \"z.h\"\n"
            "\n"
            ;
        testDocuments << CppTestDocument::create("file.cpp", original, expected);
        QTest::newRow("insertingSortingMiddle")
            << TestIncludePaths::globalIncludePath()
            << testDocuments << firstRefactoringOperation << "\"file.h\"";
        testDocuments.clear();

        // -------------------------------------------------------------------------------------------

        original =
            "#include \"a.h\"\n"
            "#include \"b.h\"\n"
            "\n@"
            ;
        expected =
            "#include \"a.h\"\n"
            "#include \"b.h\"\n"
            "#include \"file.h\"\n"
            "\n"
            ;
        testDocuments << CppTestDocument::create("file.cpp", original, expected);
        QTest::newRow("insertingSortingBottom")
            << TestIncludePaths::globalIncludePath()
            << testDocuments << firstRefactoringOperation << "\"file.h\"";
        testDocuments.clear();

        // -------------------------------------------------------------------------------------------

        original =
            "#include \"b.h\"\n"
            "#include \"a.h\"\n"
            "\n@"
            ;
        expected =
            "#include \"b.h\"\n"
            "#include \"a.h\"\n"
            "#include \"file.h\"\n"
            "\n"
            ;
        testDocuments << CppTestDocument::create("file.cpp", original, expected);
        QTest::newRow("inserting_appendToUnsorted")
            << TestIncludePaths::globalIncludePath()
            << testDocuments << firstRefactoringOperation << "\"file.h\"";
        testDocuments.clear();

        // -------------------------------------------------------------------------------------------

        original =
            "#include <a.h>\n"
            "#include <b.h>\n"
            "\n@"
            ;
        expected =
            "#include \"file.h\"\n"
            "\n"
            "#include <a.h>\n"
            "#include <b.h>\n"
            "\n"
            ;
        testDocuments << CppTestDocument::create("file.cpp", original, expected);
        QTest::newRow("inserting_firstLocalIncludeAtFront")
            << TestIncludePaths::globalIncludePath()
            << testDocuments << firstRefactoringOperation << "\"file.h\"";
        testDocuments.clear();

        // -------------------------------------------------------------------------------------------

        original =
            "#include \"a.h\"\n"
            "#include \"b.h\"\n"
            "\n"
            "void @f();\n"
            ;
        expected =
            "#include \"a.h\"\n"
            "#include \"b.h\"\n"
            "\n"
            "#include <file.h>\n"
            "\n"
            "void f();\n"
            ;
        testDocuments << CppTestDocument::create("file.cpp", original, expected);
        QTest::newRow("firstGlobalIncludeAtBack")
            << TestIncludePaths::globalIncludePath()
            << testDocuments << firstRefactoringOperation << "<file.h>";
        testDocuments.clear();

        // -------------------------------------------------------------------------------------------

        original =
            "#include \"prefixa.h\"\n"
            "#include \"prefixb.h\"\n"
            "\n"
            "#include \"foo.h\"\n"
            "\n@"
            ;
        expected =
            "#include \"prefixa.h\"\n"
            "#include \"prefixb.h\"\n"
            "#include \"prefixc.h\"\n"
            "\n"
            "#include \"foo.h\"\n"
            "\n"
            ;
        testDocuments << CppTestDocument::create("file.cpp", original, expected);
        QTest::newRow("inserting_preferGroupWithLongerMatchingPrefix")
            << TestIncludePaths::globalIncludePath()
            << testDocuments << firstRefactoringOperation << "\"prefixc.h\"";
        testDocuments.clear();

        // -------------------------------------------------------------------------------------------

        original =
            "#include \"lib/file.h\"\n"
            "#include \"lib/fileother.h\"\n"
            "\n@"
            ;
        expected =
            "#include \"lib/file.h\"\n"
            "#include \"lib/fileother.h\"\n"
            "\n"
            "#include \"file.h\"\n"
            "\n"
            ;
        testDocuments << CppTestDocument::create("file.cpp", original, expected);
        QTest::newRow("inserting_newGroupIfOnlyDifferentIncludeDirs")
            << TestIncludePaths::globalIncludePath()
            << testDocuments << firstRefactoringOperation << "\"file.h\"";
        testDocuments.clear();

        // -------------------------------------------------------------------------------------------

        original =
            "#include <lib/file.h>\n"
            "#include <otherlib/file.h>\n"
            "#include <utils/file.h>\n"
            "\n@"
            ;
        expected =
            "#include <firstlib/file.h>\n"
            "#include <lib/file.h>\n"
            "#include <otherlib/file.h>\n"
            "#include <utils/file.h>\n"
            "\n"
            ;
        testDocuments << CppTestDocument::create("file.cpp", original, expected);
        QTest::newRow("inserting_mixedDirsSorted")
            << TestIncludePaths::globalIncludePath()
            << testDocuments << firstRefactoringOperation << "<firstlib/file.h>";
        testDocuments.clear();

        // -------------------------------------------------------------------------------------------

        original =
            "#include <otherlib/file.h>\n"
            "#include <lib/file.h>\n"
            "#include <utils/file.h>\n"
            "\n@"
            ;
        expected =
            "#include <otherlib/file.h>\n"
            "#include <lib/file.h>\n"
            "#include <utils/file.h>\n"
            "#include <lastlib/file.h>\n"
            "\n"
            ;
        testDocuments << CppTestDocument::create("file.cpp", original, expected);
        QTest::newRow("inserting_mixedDirsUnsorted")
            << TestIncludePaths::globalIncludePath()
            << testDocuments << firstRefactoringOperation << "<lastlib/file.h>";
        testDocuments.clear();

        // -------------------------------------------------------------------------------------------

        original =
            "#include \"a.h\"\n"
            "#include <global.h>\n"
            "\n@"
            ;
        expected =
            "#include \"a.h\"\n"
            "#include \"z.h\"\n"
            "#include <global.h>\n"
            "\n"
            ;
        testDocuments << CppTestDocument::create("file.cpp", original, expected);
        QTest::newRow("inserting_mixedIncludeTypes1")
            << TestIncludePaths::globalIncludePath()
            << testDocuments << firstRefactoringOperation << "\"z.h\"";
        testDocuments.clear();

        // -------------------------------------------------------------------------------------------

        original =
            "#include \"z.h\"\n"
            "#include <global.h>\n"
            "\n@"
            ;
        expected =
            "#include \"a.h\"\n"
            "#include \"z.h\"\n"
            "#include <global.h>\n"
            "\n"
            ;
        testDocuments << CppTestDocument::create("file.cpp", original, expected);
        QTest::newRow("inserting_mixedIncludeTypes2")
            << TestIncludePaths::globalIncludePath()
            << testDocuments << firstRefactoringOperation << "\"a.h\"";
        testDocuments.clear();

        // -------------------------------------------------------------------------------------------

        original =
            "#include \"z.h\"\n"
            "#include <global.h>\n"
            "\n@"
            ;
        expected =
            "#include \"z.h\"\n"
            "#include \"lib/file.h\"\n"
            "#include <global.h>\n"
            "\n"
            ;
        testDocuments << CppTestDocument::create("file.cpp", original, expected);
        QTest::newRow("inserting_mixedIncludeTypes3")
            << TestIncludePaths::globalIncludePath()
            << testDocuments << firstRefactoringOperation << "\"lib/file.h\"";
        testDocuments.clear();

        // -------------------------------------------------------------------------------------------

        original =
            "#include \"z.h\"\n"
            "#include <global.h>\n"
            "\n@"
            ;
        expected =
            "#include \"z.h\"\n"
            "#include <global.h>\n"
            "#include <lib/file.h>\n"
            "\n"
            ;
        testDocuments << CppTestDocument::create("file.cpp", original, expected);
        QTest::newRow("inserting_mixedIncludeTypes4")
            << TestIncludePaths::globalIncludePath()
            << testDocuments << firstRefactoringOperation << "<lib/file.h>";
        testDocuments.clear();

        // -------------------------------------------------------------------------------------------

        original =
            "void @f();\n"
            ;
        expected =
            "#include \"file.h\"\n"
            "\n"
            "void f();\n"
            ;
        testDocuments << CppTestDocument::create("file.cpp", original, expected);
        QTest::newRow("inserting_noinclude")
            << TestIncludePaths::globalIncludePath()
            << testDocuments << firstRefactoringOperation << "\"file.h\"";
        testDocuments.clear();

        // -------------------------------------------------------------------------------------------

        original =
            "#ifndef FOO_H\n"
            "#define FOO_H\n"
            "void @f();\n"
            "#endif\n"
            ;
        expected =
            "#ifndef FOO_H\n"
            "#define FOO_H\n"
            "\n"
            "#include \"file.h\"\n"
            "\n"
            "void f();\n"
            "#endif\n"
            ;
        testDocuments << CppTestDocument::create("file.cpp", original, expected);
        QTest::newRow("inserting_onlyIncludeGuard")
            << TestIncludePaths::globalIncludePath()
            << testDocuments << firstRefactoringOperation << "\"file.h\"";
        testDocuments.clear();

        // -------------------------------------------------------------------------------------------

        original =
            "#pragma once\n"
            "void @f();\n"
            ;
        expected =
            "#pragma once\n"
            "\n"
            "#include \"file.h\"\n"
            "\n"
            "void f();\n"
            ;
        testDocuments << CppTestDocument::create("file.cpp", original, expected);
        QTest::newRow("inserting_onlyPragmaOnce")
            << TestIncludePaths::globalIncludePath()
            << testDocuments << firstRefactoringOperation << "\"file.h\"";
        testDocuments.clear();

        // -------------------------------------------------------------------------------------------

        original =
            "\n"
            "// comment\n"
            "\n"
            "void @f();\n"
            ;
        expected =
            "\n"
            "// comment\n"
            "\n"
            "#include \"file.h\"\n"
            "\n"
            "void @f();\n"
            ;
        testDocuments << CppTestDocument::create("file.cpp", original, expected);
        QTest::newRow("inserting_veryFirstIncludeCppStyleCommentOnTop")
            << TestIncludePaths::globalIncludePath()
            << testDocuments << firstRefactoringOperation << "\"file.h\"";
        testDocuments.clear();

        // -------------------------------------------------------------------------------------------

        original =
            "\n"
            "/*\n"
            " comment\n"
            " */\n"
            "\n"
            "void @f();\n"
            ;
        expected =
            "\n"
            "/*\n"
            " comment\n"
            " */\n"
            "\n"
            "#include \"file.h\"\n"
            "\n"
            "void @f();\n"
            ;
        testDocuments << CppTestDocument::create("file.cpp", original, expected);
        QTest::newRow("inserting_veryFirstIncludeCStyleCommentOnTop")
            << TestIncludePaths::globalIncludePath()
            << testDocuments << firstRefactoringOperation << "\"file.h\"";
        testDocuments.clear();

        // -------------------------------------------------------------------------------------------

        original =
            "@QDir dir;\n"
            ;
        expected =
            "#include <QDir>\n"
            "\n"
            "QDir dir;\n"
            ;
        testDocuments << CppTestDocument::create("file.cpp", original, expected);
        QTest::newRow("inserting_checkQSomethingInQtIncludePaths")
            << TestIncludePaths::globalQtCoreIncludePath()
            << testDocuments << firstRefactoringOperation << "";
        testDocuments.clear();

        original =
            "std::s@tring s;\n"
            ;
        expected =
            "#include <string>\n"
            "\n"
            "std::string s;\n"
            ;
        testDocuments << CppTestDocument::create("file.cpp", original, expected);
        QTest::newRow("inserting_std::string")
            << TestIncludePaths::globalIncludePath()
            << testDocuments << firstRefactoringOperation << "";
        testDocuments.clear();

        original = "class A{};";
        testDocuments << CppTestDocument::create("a.h", original, original);
        original = "class B{};";
        testDocuments << CppTestDocument::create("b.h", original, original);
        original =
            "#include \"b.h\"\n"
            "@A a;\n"
            "B b;";
        expected =
            "#include \"b.h\"\n\n"
            "#include \"a.h\"\n"
            "A a;\n"
            "B b;";
        testDocuments << CppTestDocument::create("b.cpp", original, expected);
        QTest::newRow("preserve first header")
            << TestIncludePaths::globalIncludePath()
            << testDocuments << firstRefactoringOperation << "";
        testDocuments.clear();

        original = "class C{};";
        testDocuments << CppTestDocument::create("c.h", original, original);
        original = "class B{};";
        testDocuments << CppTestDocument::create("b.h", original, original);
        original =
            "#include \"c.h\"\n"
            "C c;\n"
            "@B b;";
        expected =
            "#include \"b.h\"\n"
            "#include \"c.h\"\n"
            "C c;\n"
            "B b;";
        testDocuments << CppTestDocument::create("x.cpp", original, expected);
        QTest::newRow("do not preserve first header")
            << TestIncludePaths::globalIncludePath()
            << testDocuments << firstRefactoringOperation << "";
        testDocuments.clear();
    }

    void testAddIncludeForUndefinedIdentifier()
    {
        QFETCH(QString, headerPath);
        QFETCH(QuickFixTestDocuments, testDocuments);
        QFETCH(int, refactoringOperationIndex);
        QFETCH(QString, includeForTestFactory);

        TemporaryDir temporaryDir;
        QVERIFY(temporaryDir.isValid());
        for (const TestDocumentPtr &testDocument : std::as_const(testDocuments))
            testDocument->setBaseDirectory(temporaryDir.path());

        QScopedPointer<CppQuickFixFactory> factory;
        if (includeForTestFactory.isEmpty())
            factory.reset(new AddIncludeForUndefinedIdentifier);
        else
            factory.reset(new AddIncludeForUndefinedIdentifierTestFactory(includeForTestFactory));

        QuickFixOperationTest::run(testDocuments, factory.data(), headerPath,
                                   refactoringOperationIndex);
    }

    void testAddIncludeForUndefinedIdentifierNoDoubleQtHeaderInclude()
    {
        TemporaryDir temporaryDir;
        QVERIFY(temporaryDir.isValid());

        QList<TestDocumentPtr> testDocuments;
        QByteArray original;
        QByteArray expected;

        const QByteArray base = temporaryDir.path().toUtf8();

        // This file makes the QDir definition available so that locator finds it.
        original = expected = "#include <QDir>\n"
                              "void avoidBeingRecognizedAsForwardingHeader();";
        testDocuments << CppTestDocument::create(base + "/fileUsingQDir.cpp", original, expected);

        original = expected = "@QDir dir;\n";
        testDocuments << CppTestDocument::create(base + "/fileWantsToUseQDir.cpp", original, expected);

        AddIncludeForUndefinedIdentifier factory;
        const QStringList expectedOperations = QStringList("Add #include <QDir>");
        QuickFixOfferedOperationsTest(
            testDocuments,
            &factory,
            ProjectExplorer::toUserHeaderPaths(
                QStringList{TestIncludePaths::globalQtCoreIncludePath()}),
            expectedOperations);
    }

    void testAddForwardDeclForUndefinedIdentifier_data()
    {
        QTest::addColumn<QuickFixTestDocuments>("testDocuments");
        QTest::addColumn<QString>("symbol");
        QTest::addColumn<int>("symbolPos");

        QByteArray original;
        QByteArray expected;

        original =
            "#pragma once\n"
            "\n"
            "void f(const Blu@bb &b)\n"
            "{\n"
            "}\n"
            ;
        expected =
            "#pragma once\n"
            "\n"
            "\n"
            "class Blubb;\n"
            "void f(const Blubb &b)\n"
            "{\n"
            "}\n"
            ;
        QTest::newRow("unqualified symbol")
            << QuickFixTestDocuments{CppTestDocument::create("theheader.h", original, expected)}
            << "Blubb" << original.indexOf('@');

        original =
            "#pragma once\n"
            "\n"
            "namespace NS {\n"
            "class C;\n"
            "}\n"
            "void f(const NS::Blu@bb &b)\n"
            "{\n"
            "}\n"
            ;
        expected =
            "#pragma once\n"
            "\n"
            "namespace NS {\n"
            "\n"
            "class Blubb;\n"
            "class C;\n"
            "}\n"
            "void f(const NS::Blubb &b)\n"
            "{\n"
            "}\n"
            ;
        QTest::newRow("qualified symbol, full namespace present")
            << QuickFixTestDocuments{CppTestDocument::create("theheader.h", original, expected)}
            << "NS::Blubb" << original.indexOf('@');

        original =
            "#pragma once\n"
            "\n"
            "namespace NS {\n"
            "class C;\n"
            "}\n"
            "void f(const NS::NS2::Blu@bb &b)\n"
            "{\n"
            "}\n"
            ;
        expected =
            "#pragma once\n"
            "\n"
            "namespace NS {\n"
            "\n"
            "namespace NS2 { class Blubb; }\n"
            "class C;\n"
            "}\n"
            "void f(const NS::NS2::Blubb &b)\n"
            "{\n"
            "}\n"
            ;
        QTest::newRow("qualified symbol, partial namespace present")
            << QuickFixTestDocuments{CppTestDocument::create("theheader.h", original, expected)}
            << "NS::NS2::Blubb" << original.indexOf('@');

        original =
            "#pragma once\n"
            "\n"
            "namespace NS {\n"
            "class C;\n"
            "}\n"
            "void f(const NS2::Blu@bb &b)\n"
            "{\n"
            "}\n"
            ;
        expected =
            "#pragma once\n"
            "\n"
            "\n"
            "namespace NS2 { class Blubb; }\n"
            "namespace NS {\n"
            "class C;\n"
            "}\n"
            "void f(const NS2::Blubb &b)\n"
            "{\n"
            "}\n"
            ;
        QTest::newRow("qualified symbol, other namespace present")
            << QuickFixTestDocuments{CppTestDocument::create("theheader.h", original, expected)}
            << "NS2::Blubb" << original.indexOf('@');

        original =
            "#pragma once\n"
            "\n"
            "void f(const NS2::Blu@bb &b)\n"
            "{\n"
            "}\n"
            ;
        expected =
            "#pragma once\n"
            "\n"
            "\n"
            "namespace NS2 { class Blubb; }\n"
            "void f(const NS2::Blubb &b)\n"
            "{\n"
            "}\n"
            ;
        QTest::newRow("qualified symbol, no namespace present")
            << QuickFixTestDocuments{CppTestDocument::create("theheader.h", original, expected)}
            << "NS2::Blubb" << original.indexOf('@');

        original =
            "#pragma once\n"
            "\n"
            "void f(const NS2::Blu@bb &b)\n"
            "{\n"
            "}\n"
            "namespace NS2 {}\n"
            ;
        expected =
            "#pragma once\n"
            "\n"
            "\n"
            "namespace NS2 { class Blubb; }\n"
            "void f(const NS2::Blubb &b)\n"
            "{\n"
            "}\n"
            "namespace NS2 {}\n"
            ;
        QTest::newRow("qualified symbol, existing namespace after symbol")
            << QuickFixTestDocuments{CppTestDocument::create("theheader.h", original, expected)}
            << "NS2::Blubb" << original.indexOf('@');
    }

    void testAddForwardDeclForUndefinedIdentifier()
    {
        QFETCH(QuickFixTestDocuments, testDocuments);
        QFETCH(QString, symbol);
        QFETCH(int, symbolPos);

        TemporaryDir temporaryDir;
        QVERIFY(temporaryDir.isValid());
        testDocuments.first()->setBaseDirectory(temporaryDir.path());

        QScopedPointer<CppQuickFixFactory> factory(
            new AddForwardDeclForUndefinedIdentifierTestFactory(symbol, symbolPos));
        QuickFixOperationTest::run({testDocuments}, factory.data(), ".", 0);
    }
};

QObject *AddIncludeForUndefinedIdentifier::createTest()
{
    return new BringIdentifierIntoScopeTest;
}

#endif // WITH_TESTS
} // namespace

void registerBringIdentifierIntoScopeQuickfixes()
{
    CppQuickFixFactory::registerFactory<AddIncludeForUndefinedIdentifier>();
}

} // namespace CppEditor::Internal

#ifdef WITH_TESTS
#include <bringidentifierintoscope.moc>
#endif
