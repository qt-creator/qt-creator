// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cmakequickfixes.h"

#include "cmakebuildsystem.h"
#include "cmakeprojectconstants.h"
#include "cmakeprojectmanagertr.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/messagemanager.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>

#include <texteditor/codeassist/assistinterface.h>
#include <texteditor/codeassist/genericproposal.h>
#include <texteditor/codeassist/iassistprocessor.h>
#include <texteditor/codeassist/iassistprovider.h>
#include <texteditor/quickfix.h>
#include <texteditor/refactoroverlay.h>
#include <texteditor/texteditor.h>
#include <texteditor/textdocument.h>

#include <cmakelang/cmakedocument.h>

#include <utils/algorithm.h>
#include <utils/async.h>
#include <utils/filepath.h>

#include <QTextBlock>
#include <QTextDocument>
#include <QTimer>

#include <optional>

#ifdef WITH_TESTS
#include <utils/temporarydirectory.h>
#include <QTest>
#endif

using namespace Core;
using namespace ProjectExplorer;
using namespace std::chrono_literals;
using namespace TextEditor;
using namespace Utils;

namespace CMakeProjectManager::Internal {

const char REFACTOR_MARKER_ID[] = "CMakeEditor.CreateSourceFile";

// The extensions CMake tries when it looks for a source file, plus the ones the
// Qt CMake API takes as file arguments.
static bool hasSourceFileSuffix(const FilePath &filePath)
{
    static const QSet<QString> suffixes = {
        "c",   "c++",  "cc",   "cpp",  "cxx", "cu",  "mpp", "m",    "mm",  "ixx",
        "cppm", "ccm", "cxxm", "c++m", "h",   "hh",  "h++", "hm",   "hpp", "hxx",
        "in",  "txx",  "f",    "for",  "f77", "f90", "f95", "f03",  "hip", "ispc",
        "qml", "qrc",  "ui",   "ts",   "js",  "json", "py", "qdoc", "rc"};

    return suffixes.contains(filePath.suffix().toLower());
}

// The commands that take source files as input, the ones a missing file is an
// error for. Commands such as add_custom_command, configure_file or
// qt_add_translations name files that the build produces, and CMake creates
// those itself.
static bool takesSourceFiles(CMakeLang::CommandAST *command)
{
    static const QSet<QString> commands = {
        "add_executable",       "add_library",           "target_sources",
        "qt_add_executable",    "qt_add_library",        "qt_add_plugin",
        "qt_add_qml_module",    "qt_target_qml_sources", "qt_add_resources",
        "qt_add_big_resources", "qt_wrap_ui",            "qt_wrap_cpp"};

    QString name = command->commandName().toLower();
    if (name.startsWith("qt5_") || name.startsWith("qt6_"))
        name.replace(0, 4, "qt_");
    return commands.contains(name);
}

// The path an argument stands for, as far as it can be told from the file
// alone.
static std::optional<FilePath> resolvedPath(const QString &argument, const FilePath &directory)
{
    // An OPTIONS list goes to moc or uic verbatim, and -f and --include= carry
    // a header of their own.
    if (argument.startsWith('-'))
        return {};

    QString path = argument;
    path.replace("${CMAKE_CURRENT_SOURCE_DIR}", directory.path());
    path.replace("${CMAKE_CURRENT_LIST_DIR}", directory.path());

    // Anything left to expand is unknown here: variables, generator
    // expressions, environment lookups.
    if (path.contains('$'))
        return {};

    return directory.resolvePath(path);
}

// The file an argument of a CMake command names, if it names a source file that
// is not on disk. CMake stops the configure run with "Cannot find source file"
// on those.
static FilePath missingSourceFile(const QString &argument, const FilePath &directory)
{
    const std::optional<FilePath> filePath = resolvedPath(argument, directory);
    if (!filePath || !hasSourceFileSuffix(*filePath) || filePath->exists())
        return {};
    return *filePath;
}

static bool covers(int begin, int end, int position)
{
    return position >= begin && position <= end;
}

// A source file a command names but which is not on disk, and the span of the
// argument that names it.
struct MissingSourceFile
{
    int begin = 0;
    int end = 0;
    FilePath filePath;
};

// add_subdirectory takes the source directory as its first argument, and a
// binary directory as its optional second one. CMake stops when the source
// directory does not exist or holds no CMakeLists.txt, and both cases are
// fixed by creating that file.
static QList<MissingSourceFile> missingSubdirectoryFile(CMakeLang::CommandAST *command,
                                                        const FilePath &directory)
{
    CMakeLang::ArgumentAST *argument = command->arguments().first();
    if (!argument)
        return {};

    const std::optional<FilePath> sourceDirectory = resolvedPath(argument->value(), directory);
    if (!sourceDirectory || (sourceDirectory->exists() && !sourceDirectory->isDir()))
        return {};

    const FilePath cmakeLists = sourceDirectory->pathAppended(Constants::CMAKE_LISTS_TXT);
    if (cmakeLists.exists())
        return {};

    return {{argument->token.begin(), argument->token.end(), cmakeLists}};
}

// The missing source files of one command, in source order and each of them
// once.
static QList<MissingSourceFile> missingSourceFilesOfCommand(CMakeLang::CommandAST *command,
                                                            const FilePath &directory)
{
    if (command->isNamed("add_subdirectory"))
        return missingSubdirectoryFile(command, directory);
    if (!takesSourceFiles(command))
        return {};

    QList<MissingSourceFile> result;
    FilePaths seen;
    for (CMakeLang::ArgumentAST *argument : command->arguments()) {
        const FilePath filePath = missingSourceFile(argument->value(), directory);
        if (filePath.isEmpty() || seen.contains(filePath))
            continue;
        seen.append(filePath);
        result.append({argument->token.begin(), argument->token.end(), filePath});
    }
    return result;
}

// The missing source files the whole file names. Takes the text instead of a
// document because it runs off the GUI thread.
static QList<MissingSourceFile> missingSourceFilesOfText(const QString &text,
                                                         const FilePath &directory)
{
    QList<MissingSourceFile> result;
    const CMakeLang::DocumentPtr document = CMakeLang::Document::fromSource(text);
    for (CMakeLang::CommandAST *command : document->commands())
        result += missingSourceFilesOfCommand(command, directory);
    return result;
}

// The missing source files the command around position names. The argument
// under the cursor wins; a cursor anywhere else in the call - the line CMake
// points at - takes the whole argument list.
static FilePaths missingSourceFiles(const CMakeLang::DocumentPtr &document,
                                    int position,
                                    const FilePath &directory)
{
    CMakeLang::CommandAST *command = nullptr;
    for (CMakeLang::CommandAST *candidate : document->commands()) {
        if (covers(candidate->name.begin(), candidate->rightParen.end(), position)) {
            command = candidate;
            break;
        }
    }
    if (!command)
        return {};

    const QList<MissingSourceFile> missing = missingSourceFilesOfCommand(command, directory);
    for (const MissingSourceFile &source : missing) {
        if (covers(source.begin, source.end, position))
            return {source.filePath};
    }
    return Utils::transform(missing, &MissingSourceFile::filePath);
}

class MarkerUpdater;

static QList<MarkerUpdater *> &markerUpdaters()
{
    static QList<MarkerUpdater *> theMarkerUpdaters;
    return theMarkerUpdaters;
}

// Creating a file leaves the text of the CMake file alone, so nothing else
// tells the light bulbs that they are stale.
static void refreshMarkers();

static QString createFileDescription(const FilePath &filePath, const FilePath &cmakeFile)
{
    return Tr::tr("Create File \"%1\"")
        .arg(filePath.relativePathFromDir(cmakeFile.absolutePath()));
}

static void createSourceFile(const FilePath &filePath, const FilePath &cmakeFile)
{
    if (const Result<> writable = filePath.parentDir().ensureWritableDir(); !writable) {
        MessageManager::writeDisrupting(writable.error());
        return;
    }
    if (!filePath.ensureExistingFile()) {
        MessageManager::writeDisrupting(
            Tr::tr("Failed to create \"%1\".").arg(filePath.toUserOutput()));
        return;
    }

    EditorManager::openEditor(filePath);
    refreshMarkers();

    const Project *project = ProjectManager::projectForFile(cmakeFile);
    if (const auto buildSystem = qobject_cast<CMakeBuildSystem *>(
            project ? project->activeBuildSystem() : nullptr)) {
        buildSystem->runCMake();
    }
}

class CreateSourceFileOperation final : public QuickFixOperation
{
public:
    CreateSourceFileOperation(const FilePath &filePath, const FilePath &cmakeFile)
        : m_filePath(filePath)
        , m_cmakeFile(cmakeFile)
    {
        setDescription(createFileDescription(filePath, cmakeFile));
    }

private:
    void perform() final { createSourceFile(m_filePath, m_cmakeFile); }

    const FilePath m_filePath;
    const FilePath m_cmakeFile;
};

class CMakeQuickFixAssistProcessor final : public IAssistProcessor
{
    IAssistProposal *perform() final
    {
        const AssistInterface *assistInterface = interface();
        const FilePath cmakeFile = assistInterface->filePath();
        const CMakeLang::DocumentPtr document
            = CMakeLang::Document::fromSource(assistInterface->textDocument()->toPlainText());

        QuickFixOperations operations;
        for (const FilePath &filePath : missingSourceFiles(document,
                                                           assistInterface->position(),
                                                           cmakeFile.absolutePath()))
            operations << new CreateSourceFileOperation(filePath, cmakeFile);

        return GenericProposal::createProposal(assistInterface, operations);
    }
};

class CMakeQuickFixAssistProvider final : public IAssistProvider
{
public:
    IAssistProcessor *createProcessor(const AssistInterface *) const final
    {
        return new CMakeQuickFixAssistProcessor;
    }
};

IAssistProvider &cmakeQuickFixAssistProvider()
{
    static CMakeQuickFixAssistProvider theCMakeQuickFixAssistProvider;
    return theCMakeQuickFixAssistProvider;
}

// Follows the text of a document and puts the quick fix light bulb next to
// every line that names a source file which is not on disk. Looking the files
// up touches the file system, which is why it happens off the GUI thread.
class MarkerUpdater final : public QObject
{
public:
    explicit MarkerUpdater(TextDocument *document)
        : QObject(document)
        , m_document(document)
    {
        m_timer.setSingleShot(true);
        m_timer.setInterval(500ms);
        connect(&m_timer, &QTimer::timeout, this, &MarkerUpdater::update);
        connect(document->document(), &QTextDocument::contentsChanged, this, [this] {
            restart();
        });
        // Markers live in the editor widget, so a split needs its own.
        connect(EditorManager::instance(), &EditorManager::editorOpened, this,
                [this](IEditor *editor) {
                    if (editor->document() == m_document)
                        restart();
                });
        markerUpdaters().append(this);
    }

    ~MarkerUpdater() final { markerUpdaters().removeOne(this); }

    void restart() { m_timer.start(); }

private:
    void update()
    {
        const FilePath cmakeFile = m_document->filePath();
        if (cmakeFile.isEmpty())
            return;

        const int revision = m_document->document()->revision();
        Utils::futureSynchronizer()->addFuture(Utils::onResultReady(
            Utils::asyncRun(&missingSourceFilesOfText,
                            m_document->plainText(),
                            cmakeFile.absolutePath()),
            this,
            [this, revision](const QList<MissingSourceFile> &missing) {
                setMarkers(missing, revision);
            }));
    }

    void setMarkers(const QList<MissingSourceFile> &missing, int revision)
    {
        QTextDocument *text = m_document->document();
        if (text->revision() != revision)
            return;

        const FilePath cmakeFile = m_document->filePath();
        QHash<int, RefactorMarker> markers;
        for (const MissingSourceFile &source : missing) {
            const QTextBlock block = text->findBlock(source.begin);
            if (markers.contains(block.blockNumber()))
                continue;

            RefactorMarker marker;
            marker.type = REFACTOR_MARKER_ID;
            marker.cursor = QTextCursor(block);
            marker.cursor.movePosition(QTextCursor::EndOfBlock);
            marker.tooltip = createFileDescription(source.filePath, cmakeFile);
            marker.callback = [filePath = source.filePath, cmakeFile](TextEditorWidget *) {
                createSourceFile(filePath, cmakeFile);
            };
            markers.insert(block.blockNumber(), marker);
        }

        for (TextEditorWidget *widget : TextEditorWidget::textEditorWidgetsForDocument(m_document))
            widget->setRefactorMarkers(markers.values(), REFACTOR_MARKER_ID);
    }

    TextDocument * const m_document;
    QTimer m_timer;
};

static void refreshMarkers()
{
    for (MarkerUpdater *updater : markerUpdaters())
        updater->restart();
}

void setupCMakeQuickFixMarkers(TextDocument *document)
{
    new MarkerUpdater(document);
}

#ifdef WITH_TESTS

class CMakeQuickFixesTest final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        m_directory = std::make_unique<TemporaryDirectory>("cmake-quickfixes-XXXXXX");
        QVERIFY(m_directory->isValid());
        QVERIFY(m_directory->filePath("existing.cpp").ensureExistingFile());
        QVERIFY(m_directory->filePath("existing_sub").ensureWritableDir());
        QVERIFY(m_directory->filePath("existing_sub/CMakeLists.txt").ensureExistingFile());
        QVERIFY(m_directory->filePath("bare_sub").ensureWritableDir());
    }

    void cleanupTestCase() { m_directory.reset(); }

    void testMissingSourceFiles_data()
    {
        // The cursor is where the source has a '|'.
        QTest::addColumn<QString>("source");
        QTest::addColumn<QStringList>("expected");

        QTest::newRow("argument under the cursor")
            << "add_executable(app ma|in.cpp other.cpp)\n"
            << QStringList{"main.cpp"};

        QTest::newRow("whole call from the command name")
            << "add_exec|utable(app\n"
               "    main.cpp\n"
               "    widget.cpp\n"
               ")\n"
            << QStringList{"main.cpp", "widget.cpp"};

        QTest::newRow("file on disk")
            << "add_executable(app exis|ting.cpp)\n"
            << QStringList();

        QTest::newRow("target name and keywords")
            << "add_executable(a|pp WIN32 MACOSX_BUNDLE existing.cpp)\n"
            << QStringList();

        QTest::newRow("version number")
            << "project(Foo| VERSION 1.0.0)\n"
            << QStringList();

        QTest::newRow("variable reference")
            << "add_executable(app ${SOURCES}|)\n"
            << QStringList();

        QTest::newRow("path behind an unknown variable")
            << "add_executable(app ${SRC_DIR}/ma|in.cpp)\n"
            << QStringList();

        QTest::newRow("generator expression")
            << "target_sources(app PRIVATE $<$<BOOL:${WIN32}>:win.cpp>|)\n"
            << QStringList();

        QTest::newRow("path in a subdirectory")
            << "add_executable(app src/ma|in.cpp)\n"
            << QStringList{"src/main.cpp"};

        QTest::newRow("current source directory")
            << "add_executable(app ${CMAKE_CURRENT_SOURCE_DIR}/gene|rated.cpp)\n"
            << QStringList{"generated.cpp"};

        QTest::newRow("current list directory")
            << "add_executable(app ${CMAKE_CURRENT_LIST_DIR}/gene|rated.cpp)\n"
            << QStringList{"generated.cpp"};

        QTest::newRow("compile definition")
            << "target_compile_definitions(app PRIVATE -DHEADER=fo|o.h)\n"
            << QStringList();

        QTest::newRow("quoted argument")
            << "add_executable(app \"with sp|ace.cpp\")\n"
            << QStringList{"with space.cpp"};

        QTest::newRow("outside any command")
            << "add_executable(app main.cpp)\n"
               "|\n"
            << QStringList();

        QTest::newRow("cmake module")
            << "include(cmake/help|ers.cmake)\n"
            << QStringList();

        QTest::newRow("target sources")
            << "target_sources(app PRIVATE wid|get.cpp)\n"
            << QStringList{"widget.cpp"};

        QTest::newRow("qml files")
            << "qt_add_qml_module(app QML_FILES Ma|in.qml)\n"
            << QStringList{"Main.qml"};

        QTest::newRow("the same file twice")
            << "add_executable(app| main.cpp main.cpp)\n"
            << QStringList{"main.cpp"};

        QTest::newRow("versioned command name")
            << "qt6_add_executable(app ma|in.cpp)\n"
            << QStringList{"main.cpp"};

        QTest::newRow("custom command output")
            << "add_custom_command(OUTPUT par|ser.cpp COMMAND bison)\n"
            << QStringList();

        QTest::newRow("custom target byproduct")
            << "add_custom_target(gen BYPRODUCTS gene|rated.cpp COMMAND foo)\n"
            << QStringList();

        QTest::newRow("configured file")
            << "configure_file(config.h.in con|fig.h)\n"
            << QStringList();

        QTest::newRow("translation file")
            << "qt_add_translations(app TS_FILES app_|de.ts)\n"
            << QStringList();

        QTest::newRow("source file property")
            << "set_source_files_properties(gene|rated.cpp PROPERTIES GENERATED TRUE)\n"
            << QStringList();

        QTest::newRow("moc option")
            << "qt_wrap_cpp(out existing.cpp OPTIONS -fpri|vate/qfoo_p.h)\n"
            << QStringList();

        QTest::newRow("uic option")
            << "qt_wrap_ui(out existing.cpp OPTIONS --incl|ude=widget.h)\n"
            << QStringList();

        QTest::newRow("subdirectory that does not exist")
            << "add_subdirectory(myl|ib)\n"
            << QStringList{"mylib/CMakeLists.txt"};

        QTest::newRow("subdirectory without a CMakeLists.txt")
            << "add_subdirectory(bare|_sub)\n"
            << QStringList{"bare_sub/CMakeLists.txt"};

        QTest::newRow("subdirectory that is set up")
            << "add_subdirectory(existing|_sub)\n"
            << QStringList();

        // The second argument is the binary directory, which CMake creates.
        QTest::newRow("binary directory of a subdirectory")
            << "add_subdirectory(mylib mylib_bu|ild)\n"
            << QStringList{"mylib/CMakeLists.txt"};

        QTest::newRow("subdirectory that is a file")
            << "add_subdirectory(existing|.cpp)\n"
            << QStringList();

        QTest::newRow("subdirectory behind a variable")
            << "add_subdirectory(${SUBDIR}|)\n"
            << QStringList();
    }

    void testMissingSourceFiles()
    {
        QFETCH(QString, source);
        QFETCH(QStringList, expected);

        const int position = static_cast<int>(source.indexOf('|'));
        QVERIFY(position != -1);
        source.remove(position, 1);

        const FilePath directory = m_directory->path();
        const FilePaths missing = missingSourceFiles(CMakeLang::Document::fromSource(source),
                                                     position,
                                                     directory);
        const QStringList actual = Utils::transform(missing, [&directory](const FilePath &file) {
            return file.relativePathFromDir(directory);
        });
        QCOMPARE(actual, expected);
    }

    // What the light bulb needs: every missing file of the file, and the span
    // of the argument that names it.
    void testMissingSourceFilesOfText()
    {
        const QString source = "add_executable(app main.cpp existing.cpp)\n"
                               "target_sources(app PRIVATE widget.cpp widget.cpp)\n";

        const QList<MissingSourceFile> missing
            = missingSourceFilesOfText(source, m_directory->path());

        QCOMPARE(missing.size(), 2);
        QCOMPARE(missing.at(0).filePath, m_directory->filePath("main.cpp"));
        QCOMPARE(source.sliced(missing.at(0).begin, missing.at(0).end - missing.at(0).begin),
                 QString("main.cpp"));
        QCOMPARE(missing.at(1).filePath, m_directory->filePath("widget.cpp"));
        QCOMPARE(source.sliced(missing.at(1).begin, missing.at(1).end - missing.at(1).begin),
                 QString("widget.cpp"));
    }

private:
    std::unique_ptr<TemporaryDirectory> m_directory;
};

QObject *createCMakeQuickFixesTest()
{
    return new CMakeQuickFixesTest;
}

#endif // WITH_TESTS

} // CMakeProjectManager::Internal

#include "cmakequickfixes.moc"
