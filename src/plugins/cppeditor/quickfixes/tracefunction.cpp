// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "tracefunction.h"

#include "../cppeditortr.h"
#include "../cppeditorwidget.h"
#include "../cpprefactoringchanges.h"
#include "cppquickfix.h"
#include "cppquickfixhelpers.h"

#include <cplusplus/Overview.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/messagemanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projectnodes.h>
#include <texteditor/refactoringchanges.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>
#include <utils/algorithm.h>
#include <utils/filepath.h>

#include <QRegularExpression>

#ifdef WITH_TESTS
#include "../cpptoolstestcase.h"
#include <projectexplorer/kit.h>
#include <projectexplorer/kitmanager.h>
#include <utils/mimeutils.h>
#include <QTemporaryDir>
#include <QTest>
#include <QTextCursor>
#endif

using namespace CPlusPlus;
using namespace ProjectExplorer;
using namespace TextEditor;
using namespace Utils;

namespace CppEditor::Internal {
namespace {

// The types tracegen writes into a Common Trace Format trace. It drops the
// whole payload of a tracepoint that carries one it does not know, the source
// location with it, so such an argument is left out of the declaration rather
// than traced. The list is what its own type table spells: a "short" or a
// "quint64" is not in it, and neither is a QByteArray, which makes it stop
// with "Unhandled type" and take the configuration of the project down.
bool isTraceableType(QString type)
{
    static const QRegularExpression constness("\\bconst\\b");
    type.remove(constness);
    type.remove('&');
    type = type.simplified();
    type.remove(' ');

    static const QSet<QString> types = {
        "bool", "char", "shortint", "signedshort", "signedshortint",
        "unsignedshort", "unsignedshortint", "int", "signed", "signedint", "unsigned",
        "unsignedint", "long", "longint", "signedlong", "signedlongint", "unsignedlong",
        "unsignedlongint", "longlong", "longlongint", "signedlonglong",
        "signedlonglongint", "unsignedlonglong", "qint64",
        "float", "double", "intptr_t", "uintptr_t", "std::intptr_t",
        "std::uintptr_t", "QString", "QUrl", "QRect", "QSize", "QRectF",
        "QSizeF"};

    // A pointer is traced by its value, whatever it points at, but tracegen
    // writes the type into the generated header the way it reads it. That
    // header is also compiled on its own, with nothing of the project in it,
    // so a pointer is followed only where what it points at is a type the
    // header names by itself.
    if (type.endsWith('*')) {
        while (type.endsWith('*'))
            type.chop(1);
        return type == "void" || types.contains(type);
    }

    return types.contains(type);
}

// The contents of a file as they stand, which are the ones of its document
// where it is open in an editor: the wizard leaves the provider it writes open,
// and a refactoring appends to that document rather than to the disk, so what
// is on the disk is not what a second one has to read.
std::optional<QString> currentContents(const FilePath &file)
{
    if (const TextDocument *document = TextDocument::textDocumentForFilePath(file))
        return document->plainText();

    const Result<QByteArray> contents = file.fileContents();
    if (!contents)
        return {};
    return QString::fromUtf8(*contents);
}

// A trace point provider of the project, and what goes with it: the header the
// wizard writes next to it, and the macro that header spells the source
// location with. Without that header a tracepoint carries no location.
class Provider
{
public:
    static std::optional<Provider> read(const FilePath &file)
    {
        const std::optional<QString> contents = currentContents(file);
        if (!contents)
            return {};

        Provider provider;
        provider.m_file = file;
        provider.m_name = file.completeBaseName();
        provider.m_declarations = *contents;

        const FilePath header = file.parentDir().pathAppended(provider.m_name + "_tracing.h");
        static const QRegularExpression definition("^#define\\s+(\\w+_TRACE_LOCATION)\\b",
                                                   QRegularExpression::MultilineOption);
        if (const std::optional<QString> headerContents = currentContents(header)) {
            const QRegularExpressionMatch match = definition.match(*headerContents);
            if (match.hasMatch()) {
                provider.m_header = header;
                provider.m_locationMacro = match.captured(1);
            }
        }
        return provider;
    }

    const FilePath &file() const { return m_file; }
    const FilePath &header() const { return m_header; }
    const QString &name() const { return m_name; }
    const QString &locationMacro() const { return m_locationMacro; }

    bool declares(const QString &tracepoint) const
    {
        const QRegularExpression declaration(QString("^%1\\s*\\(").arg(tracepoint),
                                             QRegularExpression::MultilineOption);
        return declaration.match(m_declarations).hasMatch();
    }

    // The entry and its exit, as a block of its own at the end of the file.
    static QString tracepoint(const QString &name, const QString &arguments)
    {
        return QString("\n%1_entry(%2)\n%1_exit()\n").arg(name, arguments);
    }

private:
    FilePath m_file;
    FilePath m_header;
    QString m_name;
    QString m_locationMacro;
    QString m_declarations;
};

// How many products the project builds. A provider of another product is
// compiled into that product, and the header the wizard writes beside it is on
// the include path of that product alone, so reading the whole project is the
// same thing as reading the product of a file only where there is one.
int productCount(const FolderNode *root)
{
    int count = 0;
    root->forEachProjectNode([&count](const ProjectNode *node) {
        if (node->isProduct())
            ++count;
    });
    return count;
}

// The providers that apply to the file: the ones of the product it belongs to,
// or of the whole project where the product has none of its own and the project
// builds a single one.
QList<Provider> providersFor(const FilePath &filePath)
{
    Project *project = ProjectManager::projectForFile(filePath);
    if (!project)
        return {};

    FilePaths files;
    const auto collect = [&files](const FolderNode *folder) {
        folder->forEachNode([&files](FileNode *file) {
            if (file->filePath().suffix() == "tracepoints")
                files.append(file->filePath());
        });
    };
    if (ProjectNode *product = project->productNodeForFilePath(filePath))
        collect(product);
    if (files.isEmpty()) {
        ProjectNode *root = project->rootProjectNode();
        if (root && productCount(root) <= 1)
            collect(root);
    }

    QList<Provider> providers;
    for (const FilePath &file : Utils::filteredUnique(files)) {
        if (const std::optional<Provider> provider = Provider::read(file))
            providers.append(*provider);
    }
    return providers;
}

// Recording the tracepoint over the scope of the function. A tracepoint that
// carries nothing is spelled with its name alone, the way Qt spells one.
QString traceScope(const QString &tracepoint, const QStringList &arguments)
{
    if (arguments.isEmpty())
        return QString("\nQ_TRACE_SCOPE(%1);").arg(tracepoint);
    return QString("\nQ_TRACE_SCOPE(%1, %2);").arg(tracepoint, arguments.join(", "));
}

class TraceFunctionOp : public CppQuickFixOperation
{
public:
    TraceFunctionOp(const CppQuickFixInterface &interface,
                    CompoundStatementAST *body,
                    const QString &tracepoint,
                    const QStringList &parameterDeclarations,
                    const QStringList &parameterNames,
                    const Provider &provider,
                    bool nameProvider)
        : CppQuickFixOperation(interface)
        , m_body(body)
        , m_tracepoint(tracepoint)
        , m_parameterDeclarations(parameterDeclarations)
        , m_parameterNames(parameterNames)
        , m_provider(provider)
    {
        setDescription(nameProvider ? Tr::tr("Trace This Function with \"%1\"")
                                          .arg(provider.name())
                                    : Tr::tr("Trace This Function"));
    }

    void perform() override
    {
        QStringList declarations = m_parameterDeclarations;
        QStringList arguments = m_parameterNames;
        if (!m_provider.locationMacro().isEmpty()) {
            declarations.append("const QString &location");
            arguments.append(m_provider.locationMacro());
        }

        // A RefactoringFile that cannot read its file hands out an empty
        // document rather than reporting the failure, and appending to that
        // would write the new declaration over the ones already there.
        if (!currentContents(m_provider.file())) {
            Core::MessageManager::writeDisrupting(
                Tr::tr("Failed to read the trace point provider \"%1\".")
                    .arg(m_provider.file().toUserOutput()));
            return;
        }

        PlainRefactoringFileFactory factory;
        const RefactoringFilePtr provider = factory.file(m_provider.file());
        // A document spells the end of a line with a paragraph separator, so a
        // file that ends in a newline ends in one of those.
        const int end = provider->document()->characterCount() - 1;
        const QChar last = end > 0 ? provider->charAt(end - 1) : QChar::ParagraphSeparator;
        QString declaration = Provider::tracepoint(m_tracepoint, declarations.join(", "));
        if (last != '\n' && last != QChar::ParagraphSeparator)
            declaration.prepend('\n');
        if (!provider->apply(ChangeSet::makeInsert(end, declaration))) {
            Core::MessageManager::writeDisrupting(
                Tr::tr("Failed to write the trace point provider \"%1\".")
                    .arg(m_provider.file().toUserOutput()));
            return;
        }

        const CppRefactoringFilePtr file = currentFile();
        ChangeSet changes;
        if (!m_provider.header().isEmpty()) {
            const QString directive = '"'
                                      + m_provider.header().relativePathFromDir(
                                          file->filePath().parentDir())
                                      + '"';
            if (!includesHeader(file, directive))
                insertNewIncludeDirective(directive, file, semanticInfo().doc, changes);
        }
        changes.insert(file->endOf(m_body->lbrace_token), traceScope(m_tracepoint, arguments));
        if (!file->apply(changes)) {
            // The declaration written above is left where it is: it costs
            // nothing but a line, and removing it would undo one that a
            // second attempt then has to write again.
            Core::MessageManager::writeDisrupting(
                Tr::tr("Failed to write \"%1\".").arg(file->filePath().toUserOutput()));
        }
    }

private:
    // Tracing a second function of the same file would otherwise include the
    // header of the provider a second time.
    bool includesHeader(const CppRefactoringFilePtr &file, const QString &directive) const
    {
        const FilePath header = m_provider.header().canonicalPath();
        for (const Document::Include &include : semanticInfo().doc->resolvedIncludes()) {
            if (include.resolvedFileName().canonicalPath() == header)
                return true;
        }
        // The document of the code model is the one of the last parse, which
        // is not the one this operation reads where an earlier one has just
        // written the directive into it.
        return !file->document()->find("#include " + directive).isNull();
    }

    CompoundStatementAST * const m_body;
    const QString m_tracepoint;
    const QStringList m_parameterDeclarations;
    const QStringList m_parameterNames;
    const Provider m_provider;
};

// What tracegen reads as the name of a tracepoint: a letter of the ASCII
// alphabet, then letters, digits and underscores. A name it does not read,
// among them one starting with an underscore, makes it stop and takes the
// configuration of the project down with it, so such a function is not offered.
bool isTraceableName(const QString &name)
{
    static const QRegularExpression identifier("^[A-Za-z][A-Za-z0-9_]*$");
    return identifier.match(name).hasMatch();
}

// What the function is called in a trace: its own name, and the name of the
// class it belongs to where it has one, so that two classes that spell a
// function the same way do not share a row on the timeline.
QString tracepointName(Function *function)
{
    Overview overview;
    QString name = overview.prettyName(function->name());
    if (Class *enclosing = function->enclosingClass())
        name.prepend(overview.prettyName(enclosing->name()) + "::");

    name.replace("::", "_");
    return isTraceableName(name) ? name : QString();
}

class TraceFunction : public CppQuickFixFactory
{
#ifdef WITH_TESTS
public:
    static QObject *createTest();
#endif

private:
    void doMatch(const CppQuickFixInterface &interface, QuickFixOperations &result) override
    {
        const QList<AST *> &path = interface.path();
        FunctionDefinitionAST *definition = nullptr;
        for (int i = path.size() - 1; i >= 0 && !definition; --i)
            definition = path.at(i)->asFunctionDefinition();
        if (!definition || !definition->symbol)
            return;

        CompoundStatementAST *body = definition->function_body
                                         ? definition->function_body->asCompoundStatement()
                                         : nullptr;
        if (!body || !body->lbrace_token)
            return;

        // On the signature of the function, not anywhere in its body: the
        // operation acts on the function as a whole.
        const CppRefactoringFilePtr file = interface.currentFile();
        const int position = file->cursor().position();
        if (position < file->startOf(definition) || position > file->startOf(body->lbrace_token))
            return;

        Function *function = definition->symbol;
        const QString tracepoint = tracepointName(function);
        if (tracepoint.isEmpty())
            return;

        Overview overview;
        QStringList declarations;
        QStringList names;
        for (int i = 0, count = function->argumentCount(); i < count; ++i) {
            Argument *argument = function->argumentAt(i)->asArgument();
            if (!argument || !argument->name())
                continue;
            const QString type = overview.prettyType(argument->type());
            const QString name = overview.prettyName(argument->name());
            // "location" is what the trace viewer reads as the source location.
            if (name == "location" || !isTraceableType(type))
                continue;
            declarations.append(overview.prettyType(argument->type(), name));
            names.append(name);
        }

        const QList<Provider> providers = providersFor(file->filePath());
        for (const Provider &provider : providers) {
            if (!provider.declares(tracepoint + "_entry")) {
                result << new TraceFunctionOp(interface, body, tracepoint, declarations, names,
                                              provider, providers.size() > 1);
            }
        }
    }
};

} // namespace

#ifdef WITH_TESTS
using namespace CppEditor::Tests;

class TraceFunctionTest : public QObject
{
    Q_OBJECT

private slots:
    void testTraceableTypes_data()
    {
        QTest::addColumn<QString>("type");
        QTest::addColumn<bool>("traceable");

        QTest::newRow("int") << "int" << true;
        QTest::newRow("unsigned long long") << "unsigned long long" << true;
        QTest::newRow("double") << "double" << true;
        QTest::newRow("QString reference") << "const QString &" << true;
        QTest::newRow("string") << "const char *" << true;
        QTest::newRow("opaque pointer") << "void *" << true;

        // tracegen has no way to write these, and drops the whole payload of a
        // tracepoint that names one.
        QTest::newRow("value of a type of its own") << "const Widget &" << false;
        QTest::newRow("standard string") << "std::string" << false;
        QTest::newRow("list") << "const QList<int> &" << false;
        QTest::newRow("short") << "short" << false;
        QTest::newRow("unsigned long long int") << "unsigned long long int" << false;
        QTest::newRow("long double") << "long double" << false;
        QTest::newRow("Qt integer typedef") << "quint64" << false;

        // tracegen stops with "Unhandled type" over this one, which takes the
        // configuration of the whole project down with it.
        QTest::newRow("QByteArray") << "const QByteArray &" << false;

        // The generated header is compiled on its own and declares nothing of
        // the project, so it cannot name a type of it even through a pointer.
        QTest::newRow("pointer to a type of its own") << "const Widget *" << false;
    }

    void testTraceableTypes()
    {
        QFETCH(QString, type);
        QFETCH(bool, traceable);
        QCOMPARE(isTraceableType(type), traceable);
    }

    void testTraceableNames_data()
    {
        QTest::addColumn<QString>("name");
        QTest::addColumn<bool>("traceable");

        QTest::newRow("function") << "renderFrame" << true;
        QTest::newRow("member function") << "Widget_renderFrame" << true;
        QTest::newRow("digits") << "render2Frame" << true;

        // tracegen reads a name that starts with anything but a letter of the
        // ASCII alphabet as a line it cannot parse, and stops.
        QTest::newRow("leading underscore") << "_renderFrame" << false;
        QTest::newRow("underscored member function") << "_Widget_renderFrame" << false;
        QTest::newRow("non-ASCII letter") << QString::fromUtf8("z\xc3\xa4hlen") << false;
        QTest::newRow("operator") << "operator()" << false;
        QTest::newRow("empty") << QString() << false;
    }

    void testTraceableNames()
    {
        QFETCH(QString, name);
        QFETCH(bool, traceable);
        QCOMPARE(isTraceableName(name), traceable);
    }

    void testTraceScope()
    {
        QCOMPARE(traceScope("render", {"width", "DEMO_TRACE_LOCATION"}),
                 QString("\nQ_TRACE_SCOPE(render, width, DEMO_TRACE_LOCATION);"));

        // A tracepoint that carries nothing, which is what a function with no
        // traceable parameter comes to where the provider has no header to
        // spell the location with.
        QCOMPARE(traceScope("render", {}), QString("\nQ_TRACE_SCOPE(render);"));
    }

    // The provider the wizard writes is left open in an editor, and what the
    // operation appends goes to that document. Reading the disk instead would
    // offer the operation a second time and declare the tracepoint twice.
    void testProviderIsReadFromTheEditor()
    {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const FilePath providerFile = FilePath::fromString(directory.path())
                                          .pathAppended("demo.tracepoints");
        QVERIFY(providerFile.writeFileContents("existing_entry(int value)\nexisting_exit()\n"));

        Core::IEditor * const editor = Core::EditorManager::openEditor(providerFile);
        QVERIFY(editor);
        TextDocument * const doc = TextDocument::textDocumentForFilePath(providerFile);
        QVERIFY(doc);

        QTextCursor cursor(doc->document());
        cursor.movePosition(QTextCursor::End);
        cursor.insertText("\nrender_entry()\nrender_exit()\n");
        QVERIFY(doc->isModified());

        const std::optional<Provider> provider = Provider::read(providerFile);
        QVERIFY(provider);
        QVERIFY(provider->declares("render_entry"));

        Core::EditorManager::closeEditors({editor}, false);
    }

    void testTracepointIsAdded()
    {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const FilePath providerFile = FilePath::fromString(directory.path())
                                          .pathAppended("demo.tracepoints");
        QVERIFY(providerFile.writeFileContents("existing_entry(int value)\nexisting_exit()\n"));
        const FilePath headerFile = FilePath::fromString(directory.path())
                                        .pathAppended("demo_tracing.h");
        QVERIFY(headerFile.writeFileContents(
            "#pragma once\n#define DEMO_TRACE_LOCATION QString()\n"));

        const std::optional<Provider> provider = Provider::read(providerFile);
        QVERIFY(provider);
        QCOMPARE(provider->name(), QString("demo"));
        QCOMPARE(provider->locationMacro(), QString("DEMO_TRACE_LOCATION"));
        QVERIFY(provider->declares("existing_entry"));
        QVERIFY(!provider->declares("render_entry"));

        QCOMPARE(Provider::tracepoint("render", "int width, const QString &location"),
                 QString("\nrender_entry(int width, const QString &location)\nrender_exit()\n"));
    }

    void testQuickFix()
    {
        const auto projectDir = std::make_unique<TemporaryCopiedDir>(
            ":/cppeditor/testcases/trace-function");
        const FilePath projectFilePath = projectDir->absolutePath("CMakeLists.txt");

        // Tracepoints are a CMake affair, and so is the project of the test
        // case: opening it takes the CMake project manager, which a run of the
        // CppEditor tests alone does not load, and a kit whose CMake can
        // configure it.
        if (!ProjectManager::canOpenProjectForMimeType(Utils::mimeTypeForFile(projectFilePath)))
            QSKIP("The test requires the CMake project manager to be loaded");
        Kit * const kit = Utils::findOr(KitManager::kits(), nullptr, [](const Kit *k) {
            return k->isValid() && !k->hasWarning()
                   && k->value("CMakeProjectManager.CMakeKitInformation").isValid();
        });
        if (!kit)
            QSKIP("The test requires at least one valid kit with a CMake tool");

        SourceFilesRefreshGuard refreshGuard;
        ProjectOpenerAndCloser projectMgr;
        // A kit that carries a CMake tool is not yet a kit whose CMake can
        // configure a project, and there is nothing to ask beforehand that
        // tells the two apart. A project of three lines that does not come up
        // is a statement about the machine rather than about the quick fix,
        // which is asserted below once it is open.
        if (!projectMgr.open(projectFilePath, kit))
            QSKIP("The CMake of the kit could not configure the project of the test");
        QVERIFY(refreshGuard.wait());

        const FilePath sourceFilePath = projectDir->absolutePath("main.cpp");
        QVERIFY2(sourceFilePath.exists(), qPrintable(sourceFilePath.toUserOutput()));
        const auto editor = qobject_cast<BaseTextEditor *>(
            Core::EditorManager::openEditor(sourceFilePath));
        QVERIFY(editor);
        const auto doc = qobject_cast<TextEditor::TextDocument *>(editor->document());
        QVERIFY(doc);
        const auto editorWidget = qobject_cast<CppEditorWidget *>(editor->editorWidget());
        QVERIFY(editorWidget);

        TraceFunction factory;
        const auto trace = [&](const QString &function) {
            const QTextCursor functionCursor = doc->document()->find(function);
            QVERIFY(!functionCursor.isNull());
            editor->setCursorPosition(functionCursor.position());
            QVERIFY(TestCase::waitForRehighlightedSemanticDocument(editorWidget));

            CppQuickFixInterface quickFixInterface(editorWidget, ExplicitlyInvoked);
            QuickFixOperations operations;
            factory.match(quickFixInterface, operations);
            QCOMPARE(operations.size(), qsizetype(1));
            operations.first()->perform();
            QVERIFY(doc->save());
        };
        trace("renderFrame");

        // The parameter of a type tracegen cannot write is left out, so that
        // the location the viewer links the event with survives.
        const FilePath providerFile = projectDir->absolutePath("demo.tracepoints");
        const Result<QByteArray> provider = providerFile.fileContents();
        QVERIFY(provider);
        QCOMPARE(QString::fromUtf8(*provider),
                 QString("existing_entry(int value)\nexisting_exit()\n"
                         "\nrenderFrame_entry(int width, const QString &name, "
                         "const QString &location)\nrenderFrame_exit()\n"));

        // The second function of the file finds the header of the provider
        // already included, and does not include it again.
        trace("updateFrame");

        const Result<QByteArray> source = sourceFilePath.fileContents();
        QVERIFY(source);
        QCOMPARE(QString::fromUtf8(*source),
                 QString("#include \"demo_tracing.h\"\n"
                         "\n"
                         "class QString;\n"
                         "class Widget;\n"
                         "\n"
                         "int renderFrame(int width, const QString &name, const Widget &widget)\n"
                         "{\n"
                         "    Q_TRACE_SCOPE(renderFrame, width, name, DEMO_TRACE_LOCATION);\n"
                         "    return width;\n"
                         "}\n"
                         "\n"
                         "int updateFrame(int height)\n"
                         "{\n"
                         "    Q_TRACE_SCOPE(updateFrame, height, DEMO_TRACE_LOCATION);\n"
                         "    return height;\n"
                         "}\n"));
    }

    void testHeaderlessProviderHasNoLocation()
    {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const FilePath providerFile = FilePath::fromString(directory.path())
                                          .pathAppended("demo.tracepoints");
        QVERIFY(providerFile.writeFileContents("render_entry()\nrender_exit()\n"));

        const std::optional<Provider> provider = Provider::read(providerFile);
        QVERIFY(provider);
        QVERIFY(provider->header().isEmpty());
        QVERIFY(provider->locationMacro().isEmpty());
    }
};

QObject *TraceFunction::createTest() { return new TraceFunctionTest; }
#endif

void registerTraceFunctionQuickfix()
{
    CppQuickFixFactory::registerFactory<TraceFunction>();
}

} // namespace CppEditor::Internal

#ifdef WITH_TESTS
#include <tracefunction.moc>
#endif
