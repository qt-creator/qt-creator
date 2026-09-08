// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "conditionalsources.h"

#include <utils/algorithm.h>
#include <utils/mimeconstants.h>
#include <utils/mimeutils.h>

#include <QHash>

#ifdef WITH_TESTS
#include "cmakeutils.h"
#include "projecttreehelper.h"

#include <cppeditor/cpptoolstestcase.h>

#include <QPromise>
#include <QTest>
#endif

using namespace CMakeLang;
using namespace ProjectExplorer;
using namespace Utils;

namespace CMakeProjectManager::Internal {

// Node::fileTypeForFileName() calls anything with a valid MIME type a source,
// so it cannot tell a source file from a readme.
static bool hasSourceMimeType(const FilePath &path)
{
    using namespace Utils::Constants;
    static const QSet<QString> sourceMimeTypes{
        CPP_SOURCE_MIMETYPE,
        CPP_HEADER_MIMETYPE,
        C_SOURCE_MIMETYPE,
        C_HEADER_MIMETYPE,
        OBJECTIVE_C_SOURCE_MIMETYPE,
        OBJECTIVE_CPP_SOURCE_MIMETYPE,
        CUDA_SOURCE_MIMETYPE,
        FORM_MIMETYPE,
        RESOURCE_MIMETYPE,
        SCXML_MIMETYPE,
        QML_MIMETYPE,
        QMLUI_MIMETYPE,
        JS_MIMETYPE,
        LINGUIST_MIMETYPE,
        GLSL_MIMETYPE,
        GLSL_FRAG_MIMETYPE,
        GLSL_ES_FRAG_MIMETYPE,
        GLSL_VERT_MIMETYPE,
        GLSL_ES_VERT_MIMETYPE,
        GLSL_GEOM_MIMETYPE,
        GLSL_COMP_MIMETYPE,
        GLSL_TESS_MIMETYPE};

    const MimeType mimeType = Utils::mimeTypeForFile(path, MimeMatchMode::MatchExtension);
    return mimeType.isValid() && sourceMimeTypes.contains(mimeType.name());
}

// The built-in commands whose arguments are conditions rather than files, and
// those that name existing source files without handing them to a build.
// Everything else may add sources, a project's own functions and macros
// included.
static bool takesSources(const CommandAST *command)
{
    static const QSet<QString> otherCommands{
        "if",
        "elseif",
        "else",
        "endif",
        "while",
        "endwhile",
        "foreach",
        "endforeach",
        "function",
        "endfunction",
        "macro",
        "endmacro",
        "block",
        "endblock",
        "add_compile_definitions",
        "add_compile_options",
        "add_custom_command",
        "add_custom_target",
        "add_link_options",
        "add_subdirectory",
        "add_test",
        "cmake_parse_arguments",
        "cmake_path",
        "configure_file",
        "define_property",
        "execute_process",
        "export",
        "file",
        "find_file",
        "find_library",
        "find_package",
        "find_path",
        "find_program",
        "get_filename_component",
        "get_property",
        "get_source_file_property",
        "get_target_property",
        "include",
        "include_directories",
        "install",
        "link_directories",
        "load_cache",
        "mark_as_advanced",
        "message",
        "option",
        "separate_arguments",
        "set_directory_properties",
        "set_property",
        "set_source_files_properties",
        "set_target_properties",
        "source_group",
        "string",
        "target_compile_definitions",
        "target_compile_features",
        "target_compile_options",
        "target_include_directories",
        "target_link_directories",
        "target_link_libraries",
        "target_link_options",
        "try_compile",
        "try_run",
        "unset",
        "variable_watch",
        "write_file"};

    return !otherCommands.contains(command->commandName().toLower());
}

static bool isConditional(const DocumentPtr &document, const CommandAST *command)
{
    bool conditional = false;
    for (const AST *construct : document->enclosingConstructs(command)) {
        switch (construct->kind) {
        // A body runs in whatever directory calls it, or never at all.
        case AST::Kind_Function:
        case AST::Kind_Macro:
            return false;
        case AST::Kind_If:
        case AST::Kind_ElseIfClause:
        case AST::Kind_ElseClause:
            conditional = true;
            break;
        default:
            break;
        }
    }
    return conditional;
}

static bool isPlainPath(const QString &value)
{
    return !value.isEmpty() && !value.contains('$') && !value.contains('*')
           && !value.contains('?');
}

static QString firstArgumentValue(const CommandAST *command)
{
    ArgumentAST *argument = command->arguments().first();
    if (!argument)
        return {};
    const QString value = argument->value();
    return value.contains('$') ? QString() : value;
}

static bool assignsVariable(const CommandAST *command)
{
    return command->isNamed("set") || command->isNamed("list");
}

// set(VAR ...) fills its first argument, list(APPEND VAR ...) its second.
static QString assignedVariable(const CommandAST *command)
{
    const ListView<ArgumentAST *> arguments = command->arguments();
    ArgumentAST *argument = command->isNamed("list") ? arguments.at(1) : arguments.first();
    return argument ? argument->value() : QString();
}

static bool isExistingFile(QHash<FilePath, bool> &cache, const FilePath &path)
{
    const auto it = cache.constFind(path);
    if (it != cache.constEnd())
        return it.value();
    return cache.insert(path, path.isFile()).value();
}

QList<ConditionalSource> conditionalSourcesOf(const DocumentPtr &document,
                                              const FilePath &cmakeFile,
                                              const FilePath &sourceDir,
                                              const FilePath &buildDir)
{
    if (!document || !document->ast())
        return {};

    const FilePath directory = cmakeFile.parentDir();

    QList<ConditionalSource> sources;
    QHash<QString, QList<int>> variableSources;
    QHash<FilePath, bool> existingFiles;

    for (CommandAST *command : document->commands()) {
        if (!takesSources(command) || !isConditional(document, command))
            continue;

        const bool assigns = assignsVariable(command);
        const QString variable = assigns ? assignedVariable(command) : QString();
        const QString target = assigns ? QString() : firstArgumentValue(command);

        for (ArgumentAST *argument : command->arguments()) {
            const QStringList values = argument->value().split(';', Qt::SkipEmptyParts);
            for (const QString &value : values) {
                if (!isPlainPath(value))
                    continue;
                const FilePath path = directory.resolvePath(value);
                if (!path.isChildOf(sourceDir) || path.isChildOf(buildDir))
                    continue;
                if (!hasSourceMimeType(path) || !isExistingFile(existingFiles, path))
                    continue;

                if (!variable.isEmpty())
                    variableSources[variable].append(sources.size());
                sources.append({path, directory, target});
            }
        }
    }

    // A conditional set() or list() names no target, but whatever spells its
    // variable afterwards does.
    for (CommandAST *command : document->commands()) {
        if (variableSources.isEmpty())
            break;
        if (assignsVariable(command) || !takesSources(command))
            continue;
        const QString target = firstArgumentValue(command);
        if (target.isEmpty())
            continue;

        for (ArgumentAST *argument : command->arguments()) {
            const QString value = argument->value();
            for (auto it = variableSources.cbegin(); it != variableSources.cend(); ++it) {
                if (!value.contains("${" + it.key() + "}"))
                    continue;
                for (int index : it.value()) {
                    if (sources[index].target.isEmpty())
                        sources[index].target = target;
                }
            }
        }
    }

    return sources;
}

QList<ConditionalSource> conditionalSources(const QFuture<void> &cancelFuture,
                                            const QSet<CMakeFileInfo> &cmakeFiles,
                                            const QSet<FilePath> &knownFiles,
                                            const FilePath &sourceDir,
                                            const FilePath &buildDir)
{
    QList<ConditionalSource> result;
    QSet<FilePath> seen = knownFiles;

    for (const CMakeFileInfo &info : cmakeFiles) {
        if (cancelFuture.isCanceled())
            return {};

        if (!info.document || info.isExternal || info.isGenerated)
            continue;
        // A relative path in an included .cmake file resolves against the
        // directory of the includer, which is out of reach here.
        if (!info.isCMakeListsDotTxt || !info.path.isChildOf(sourceDir))
            continue;

        const QList<ConditionalSource> sources
            = conditionalSourcesOf(info.document, info.path, sourceDir, buildDir);
        for (const ConditionalSource &source : sources) {
            if (Utils::insert(seen, source.path))
                result.append(source);
        }
    }

    return result;
}

#ifdef WITH_TESTS

class ConditionalSourcesTest final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        m_projectDir = std::make_unique<CppEditor::Tests::TemporaryCopiedDir>(
            ":/cmakeprojectmanager/testcases/conditionalsources");
        m_directory = m_projectDir->filePath().canonicalPath();

        // Only a file that is there counts as a source, so absent.cpp is not
        // in this list.
        const QStringList fileNames{
            "main.cpp",
            "helper.cpp",
            "plain.cpp",
            "win.cpp",
            "win.h",
            "sub/nested.cpp",
            "listed1.cpp",
            "listed2.cpp",
            "build/generated.cpp",
            "wrapped.cpp",
            "tool.cpp",
            "grouped.cpp",
            "installed.cpp",
            "helper.cmake",
            "readme.md",
            "extra.cpp",
            "more.cpp",
            "mac.cpp",
            "other.cpp",
            "inhelper.cpp"};
        for (const QString &fileName : fileNames) {
            const FilePath file = m_directory.pathAppended(fileName);
            QVERIFY(file.parentDir().ensureWritableDir());
            QVERIFY(file.writeFileContents({}));
        }

        m_document = parseCMakeFile(m_directory.pathAppended("CMakeLists.txt"));
        QVERIFY(m_document);
        QVERIFY(m_document->isValid());
    }

    void testDetection()
    {
        const QList<QPair<QString, QString>> expected{
            {"win.cpp", "app"},
            {"win.h", "app"},
            {"sub/nested.cpp", "app"},
            {"listed1.cpp", "app"},
            {"listed2.cpp", "app"},
            {"wrapped.cpp", "app"},
            {"tool.cpp", "tool"},
            {"extra.cpp", "app"},
            {"more.cpp", {}},
            {"mac.cpp", "helper"},
            {"other.cpp", "app"}};

        QCOMPARE(found(conditionalSourcesOf(m_document,
                                            m_directory.pathAppended("CMakeLists.txt"),
                                            m_directory,
                                            m_directory.pathAppended("build"))),
                 expected);
    }

    void testKnownFilesAreSkipped()
    {
        // What the configure run did compile is what the branch taken named.
        const QSet<FilePath> knownFiles{m_directory.pathAppended("win.cpp"),
                                        m_directory.pathAppended("win.h")};

        CMakeFileInfo info;
        info.path = m_directory.pathAppended("CMakeLists.txt");
        info.isCMakeListsDotTxt = true;
        info.document = m_document;

        QPromise<void> promise;
        promise.start();

        const QList<QPair<QString, QString>> expected{
            {"sub/nested.cpp", "app"},
            {"listed1.cpp", "app"},
            {"listed2.cpp", "app"},
            {"wrapped.cpp", "app"},
            {"tool.cpp", "tool"},
            {"extra.cpp", "app"},
            {"more.cpp", {}},
            {"mac.cpp", "helper"},
            {"other.cpp", "app"}};

        QCOMPARE(found(conditionalSources(promise.future(),
                                          {info},
                                          knownFiles,
                                          m_directory,
                                          m_directory.pathAppended("build"))),
                 expected);
    }

    void testIncludedCMakeFilesAreSkipped()
    {
        const FilePath included = m_directory.pathAppended("included.cmake");
        QVERIFY(m_directory.pathAppended("CMakeLists.txt").copyFile(included));

        CMakeFileInfo info;
        info.path = included;
        info.isCMake = true;
        info.document = parseCMakeFile(included);
        QVERIFY(info.document);

        QPromise<void> promise;
        promise.start();

        QVERIFY(conditionalSources(promise.future(),
                                   {info},
                                   {},
                                   m_directory,
                                   m_directory.pathAppended("build"))
                    .isEmpty());
    }

    void testTreePlacement()
    {
        auto root = std::make_unique<CMakeProjectNode>(m_directory);
        auto listsNode = std::make_unique<CMakeListsNode>(m_directory);
        CMakeListsNode *lists = listsNode.get();
        root->addNode(std::move(listsNode));
        CMakeTargetNode *app = addTargetNode(lists, "app");
        CMakeTargetNode *helper = addTargetNode(lists, "helper");

        const QHash<FilePath, ProjectNode *> cmakeListsNodes{{m_directory, lists}};
        addConditionalSources(root.get(),
                              cmakeListsNodes,
                              conditionalSourcesOf(m_document,
                                                   m_directory.pathAppended("CMakeLists.txt"),
                                                   m_directory,
                                                   m_directory.pathAppended("build")));

        QVERIFY(app->fileNode(m_directory.pathAppended("win.cpp")));
        QVERIFY(app->fileNode(m_directory.pathAppended("wrapped.cpp")));
        QVERIFY(app->fileNode(m_directory.pathAppended("extra.cpp")));
        QVERIFY(helper->fileNode(m_directory.pathAppended("mac.cpp")));

        FolderNode *sub = app->folderNode(m_directory.pathAppended("sub"));
        QVERIFY(sub);
        QVERIFY(sub->fileNode(m_directory.pathAppended("sub/nested.cpp")));

        // The tool target is not in the tree, and no command names a target
        // for more.cpp while the directory holds more than one.
        QVERIFY(lists->fileNode(m_directory.pathAppended("tool.cpp")));
        QVERIFY(lists->fileNode(m_directory.pathAppended("more.cpp")));

        int fileCount = 0;
        root->forEachNode([&fileCount](FileNode *file) {
            ++fileCount;
            QVERIFY(!file->isEnabled());
        });
        QCOMPARE(fileCount, 11);
    }

    void testSingleTargetFallback()
    {
        auto root = std::make_unique<CMakeProjectNode>(m_directory);
        auto listsNode = std::make_unique<CMakeListsNode>(m_directory);
        CMakeListsNode *lists = listsNode.get();
        root->addNode(std::move(listsNode));
        CMakeTargetNode *app = addTargetNode(lists, "app");

        const QHash<FilePath, ProjectNode *> cmakeListsNodes{{m_directory, lists}};
        addConditionalSources(root.get(),
                              cmakeListsNodes,
                              conditionalSourcesOf(m_document,
                                                   m_directory.pathAppended("CMakeLists.txt"),
                                                   m_directory,
                                                   m_directory.pathAppended("build")));

        // No command names a target for more.cpp, and app is the only one here.
        QVERIFY(app->fileNode(m_directory.pathAppended("more.cpp")));
        // A named target that is not in the tree does not become app.
        QVERIFY(lists->fileNode(m_directory.pathAppended("tool.cpp")));
        QVERIFY(lists->fileNode(m_directory.pathAppended("mac.cpp")));
    }

private:
    CMakeTargetNode *addTargetNode(FolderNode *parent, const QString &title)
    {
        CMakeBuildTarget target;
        target.title = title;
        auto node = std::make_unique<CMakeTargetNode>(m_directory, target);
        CMakeTargetNode *raw = node.get();
        parent->addNode(std::move(node));
        return raw;
    }

    QList<QPair<QString, QString>> found(const QList<ConditionalSource> &sources) const
    {
        return Utils::transform(sources, [this](const ConditionalSource &source) {
            return qMakePair(source.path.relativePathFromDir(m_directory), source.target);
        });
    }

    std::unique_ptr<CppEditor::Tests::TemporaryCopiedDir> m_projectDir;
    FilePath m_directory;
    DocumentPtr m_document;
};

QObject *createConditionalSourcesTest()
{
    return new ConditionalSourcesTest;
}

#endif // WITH_TESTS

} // CMakeProjectManager::Internal

#ifdef WITH_TESTS
#include "conditionalsources.moc"
#endif
