// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cmakebuildsystem.h"

#include "builddirparameters.h"
#include "cmakebuildconfiguration.h"
#include "cmakebuildstep.h"
#include "cmakebuildtarget.h"
#include "cmakeinstallstep.h"
#include "cmakekitaspect.h"
#include "cmakeproject.h"
#include "cmakeprojectconstants.h"
#include "cmakeprojectmanagertr.h"
#include "cmakespecificsettings.h"
#include "cmaketoolmanager.h"
#include "cmakeutils.h"
#include "presetsmacros.h"
#include "projecttreehelper.h"
#include "targethelper.h"

#include <android/androidconstants.h>
#include <harmonyos/harmonyosconstants.h>

#include <cmakelang/cmakerewriter.h>

#include <coreplugin/icore.h>
#include <coreplugin/documentmanager.h>
#include <coreplugin/editormanager/documentmodel.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/progressmanager/progressmanager.h>

#include <cppeditor/cppprojectfile.h>
#include <cppeditor/cpptoolsreuse.h>

#include <ios/iosconstants.h>

#include <projectexplorer/buildmanager.h>
#include <projectexplorer/deploymentdata.h>
#include <projectexplorer/devicesupport/devicekitaspects.h>
#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/extracompiler.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projectupdater.h>
#include <projectexplorer/target.h>
#include <projectexplorer/taskhub.h>
#include <projectexplorer/toolchainkitaspect.h>
#include <projectexplorer/treescanner.h>

#include <texteditor/refactoringchanges.h>
#include <texteditor/texteditor.h>

#include <qtapplicationmanager/appmanagerconstants.h>

#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtcppkitinfo.h>
#include <qtsupport/qtkitaspect.h>
#include <qtsupport/qtsupportconstants.h>

#include <utils/algorithm.h>
#include <utils/async.h>
#include <utils/checkablemessagebox.h>
#include <utils/fileutils.h>
#include <utils/layoutbuilder.h>
#include <utils/macroexpander.h>
#include <utils/mimeconstants.h>
#include <utils/mimeutils.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QClipboard>
#include <QDialogButtonBox>
#include <QFile>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QPushButton>

#ifdef WITH_TESTS
#include "cmakecodestyle.h"

#include <coreplugin/iwizardfactory.h>
#include <cppeditor/cpptoolstestcase.h>
#include <projectexplorer/jsonwizard/jsonwizard.h>
#include <texteditor/icodestylepreferences.h>
#include <texteditor/icodestylepreferencesfactory.h>
#include <QTemporaryDir>
#include <QTest>
#endif

using namespace CMakeLang;
using namespace ProjectExplorer;
using namespace QtTaskTree;
using namespace TextEditor;
using namespace Utils;

namespace CMakeProjectManager::Internal {

static Q_LOGGING_CATEGORY(cmakeBuildSystemLog, "qtc.cmake.buildsystem", QtWarningMsg);

QString quoteString(const QString &fileName)
{
    return fileName.contains(QChar::Space) ? QString(R"("%1")").arg(fileName) : fileName;
}

// Where an offset into the parsed text falls: the line 1-based, the column
// 0-based. The document and the text that was parsed can spell their line
// endings differently, so an offset does not carry from the one to the other,
// while a line and a column do.
static Text::Position positionInSource(QStringView source, int offset)
{
    const int end = std::min<int>(offset, source.size());
    Text::Position position{1, 0};
    int lineStart = 0;
    for (int i = 0; i < end; ++i) {
        if (source.at(i) == u'\n') {
            ++position.line;
            lineStart = i + 1;
        }
    }
    position.column = end - lineStart;
    return position;
}

// Applies what the rewriter collected to the file, wherever it is: a file that
// no editor has open is written, one that is open is changed and saved.
static Result<bool> applyEdits(const FilePath &cmakeFile,
                               const DocumentPtr &document,
                               const QList<Edit> &edits)
{
    if (edits.isEmpty())
        return true;

    PlainRefactoringFileFactory factory;
    const RefactoringFilePtr file = factory.file(cmakeFile);
    if (!file->isValid())
        return ResultError("Changes to " + cmakeFile.toUserOutput() + " could not be prepared.");

    const QStringView source = document->source();
    ChangeSet changeSet;
    for (const Edit &edit : edits) {
        changeSet.replace(file->position(positionInSource(source, edit.position)),
                          file->position(positionInSource(source, edit.position + edit.length)),
                          edit.text);
    }
    if (changeSet.hadErrors())
        return ResultError("Changes to " + cmakeFile.toUserOutput() + " overlap.");

    if (!file->apply(changeSet))
        return ResultError("Changes to " + cmakeFile.toUserOutput() + " could not be applied.");

    Core::IDocument *openDocument = Core::DocumentModel::documentForFilePath(cmakeFile);
    if (openDocument && openDocument->isModified()
        && !Core::DocumentManager::saveDocument(openDocument)) {
        return ResultError("Changes to " + cmakeFile.toUserOutput() + " could not be saved.");
    }
    return true;
}

// --------------------------------------------------------------------
// CMakeBuildSystem:
// --------------------------------------------------------------------

CMakeBuildSystem::CMakeBuildSystem(BuildConfiguration *bc)
    : BuildSystem(bc)
    , m_cppCodeModelUpdater(ProjectUpdaterFactory::createCppProjectUpdater())
{
    connect(&m_reader, &FileApiReader::configurationStarted, this, [this] {
        clearError(ForceEnabledChanged::True);
    });

    connect(&m_reader, &FileApiReader::dataAvailable, this, &CMakeBuildSystem::handleParsingSucceeded);
    connect(&m_reader, &FileApiReader::errorOccurred, this, &CMakeBuildSystem::handleParsingFailed);
    connect(&m_reader, &FileApiReader::dirty, this, &CMakeBuildSystem::becameDirty);
    connect(&m_reader, &FileApiReader::debuggingStarted, this, &BuildSystem::debuggingStarted);

    wireUpConnections();

    wireUpHeaderDependencySetting();

    m_isMultiConfig = CMakeGeneratorKitAspect::isMultiConfigGenerator(bc->kit());
}

// The setting lives in the global settings or in the project's own, and which of the two
// applies is itself a setting, so all three have to be watched for the value they add up to.
void CMakeBuildSystem::wireUpHeaderDependencySetting()
{
    m_scanningHeaderDependencies
        = cmakeSettingsForProject(project()).scanHeaderDependencies();

    const auto onChanged = [this] {
        const bool scanning = cmakeSettingsForProject(project()).scanHeaderDependencies();
        if (scanning == m_scanningHeaderDependencies)
            return;

        m_scanningHeaderDependencies = scanning;
        if (!scanning)
            m_headerDependencyUpdater.cancel();
        reparse(REPARSE_DEFAULT);
    };

    cmakeSettingsForProject(nullptr).scanHeaderDependencies.addOnChanged(this, onChanged);

    if (auto cmakeProject = qobject_cast<CMakeProject *>(project())) {
        CMakeSpecificSettings &projectSettings = cmakeProject->settings();
        projectSettings.scanHeaderDependencies.addOnChanged(this, onChanged);
        projectSettings.useGlobalSettings.addOnChanged(this, onChanged);
    }
}

CMakeBuildSystem::~CMakeBuildSystem()
{
    // Trigger any pending parsingFinished signals before destroying any other build system part
    // but tell it to not do anything
    m_isDestructing = true;
    m_currentGuard = {};
    m_taskTreeRunner.reset();
    delete m_cppCodeModelUpdater;
    qDeleteAll(m_extraCompilers);
}

void CMakeBuildSystem::triggerParsing()
{
    qCDebug(cmakeBuildSystemLog) << buildConfiguration()->displayName() << "Parsing has been triggered";

    auto guard = guardParsingRun();

    if (!guard.guardsProject()) {
        // This can legitimately trigger if e.g. Build->Run CMake
        // is selected while this here is already running.

        // Stop old parse run and keep that ParseGuard!
        qCDebug(cmakeBuildSystemLog) << "Stopping current parsing run!";
        stopParsingAndClearState();
    } else {
        // Use new ParseGuard
        m_currentGuard = std::move(guard);
    }
    QTC_ASSERT(!m_reader.isParsing(), return );

    qCDebug(cmakeBuildSystemLog) << "ParseGuard acquired.";

    int reparseParameters = takeReparseParameters();

    m_waitingForParse = true;
    m_combinedScanAndParseResult = true;

    QTC_ASSERT(m_parameters.isValid(), return );

    TaskHub::clearTasks(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM);

    qCDebug(cmakeBuildSystemLog) << "Parse called with flags:"
                                 << reparseParametersString(reparseParameters);

    const FilePath cache = m_parameters.buildDirectory.pathAppended(Constants::CMAKE_CACHE_TXT);
    if (!cache.exists()) {
        reparseParameters |= REPARSE_FORCE_INITIAL_CONFIGURATION | REPARSE_FORCE_CMAKE_RUN;
        qCDebug(cmakeBuildSystemLog)
            << "No" << cache
            << "file found, new flags:" << reparseParametersString(reparseParameters);
    }

    if ((0 == (reparseParameters & REPARSE_FORCE_EXTRA_CONFIGURATION))
        && mustApplyConfigurationChangesArguments(m_parameters)) {
        reparseParameters |= REPARSE_FORCE_CMAKE_RUN | REPARSE_FORCE_EXTRA_CONFIGURATION;
    }

    // The code model will be updated after the CMake run. There is no need to have an
    // active code model updater when the next one will be triggered.
    m_cppCodeModelUpdater->cancel();

    const CMakeTool *tool = CMakeToolManager::findByCommand(m_parameters.cmakeExecutable);
    CMakeTool::Version version = tool ? tool->version() : CMakeTool::Version();
    const bool isDebuggable = (version.major == 3 && version.minor >= 27) || version.major > 3;

    qCDebug(cmakeBuildSystemLog) << "Asking reader to parse";
    m_reader.parse(reparseParameters & REPARSE_FORCE_CMAKE_RUN,
                   reparseParameters & REPARSE_FORCE_INITIAL_CONFIGURATION,
                   reparseParameters & REPARSE_FORCE_EXTRA_CONFIGURATION,
                   (reparseParameters & REPARSE_DEBUG) && isDebuggable,
                   reparseParameters & REPARSE_PROFILING);
}

void CMakeBuildSystem::requestDebugging()
{
    qCDebug(cmakeBuildSystemLog) << "Requesting parse due to \"Rescan Project\" command";
    reparse(REPARSE_FORCE_CMAKE_RUN | REPARSE_FORCE_EXTRA_CONFIGURATION | REPARSE_URGENT
            | REPARSE_DEBUG);
}

bool CMakeBuildSystem::supportsAction(Node *context, ProjectAction action, const Node *node) const
{
    const auto cmakeTargetNode = dynamic_cast<CMakeTargetNode *>(context);

    if (cmakeTargetNode) {
        if (cmakeTargetNode->cmakeBuildTarget().targetType != UtilityType)
            return action == ProjectAction::AddNewFile || action == ProjectAction::AddExistingFile
                   || action == ProjectAction::AddExistingDirectory
                   || action == ProjectAction::Rename || action == ProjectAction::RemoveFile;
    }

    const auto cmakeProject = dynamic_cast<CMakeProjectNode *>(context);
    const auto cmakeListsNode = dynamic_cast<CMakeListsNode *>(context);
    if (cmakeProject || cmakeListsNode)
        return action == ProjectAction::AddSubProject
               || action == ProjectAction::AddExistingProject;

    return BuildSystem::supportsAction(context, action, node);
}

static QString relativeFilePaths(const FilePaths &filePaths, const FilePath &projectDir)
{
    return Utils::transform(
               filePaths,
               [projectDir](const FilePath &path) {
                   return quoteString(path.canonicalPath().relativePathFromDir(projectDir));
               })
        .join(' ');
};

// The last value the keyword takes in the call, the keyword itself when it
// takes none, or null when the call does not spell the keyword.
static ArgumentAST *lastValueOfKeyword(CommandAST *command,
                                       const Signature &signature,
                                       const QString &keyword)
{
    for (const KeywordArguments &group : groupArguments(command, signature)) {
        if (!group.keyword || group.keyword->value() != keyword)
            continue;
        return group.values.isEmpty() ? group.keyword : group.values.last();
    }
    return nullptr;
}

// The values the keyword takes in the call.
static QList<ArgumentAST *> valuesOfKeyword(CommandAST *command,
                                            const Signature &signature,
                                            const QString &keyword)
{
    for (const KeywordArguments &group : groupArguments(command, signature)) {
        if (group.keyword && group.keyword->value() == keyword)
            return group.values;
    }
    return {};
}

// The keyword that the argument is the only value of, and that taking the
// argument away would leave without any.
static ArgumentAST *keywordLeftEmpty(CommandAST *command,
                                     const Signature &signature,
                                     ArgumentAST *argument)
{
    for (const KeywordArguments &group : groupArguments(command, signature)) {
        if (!group.keyword || group.values.size() != 1 || group.values.first() != argument)
            continue;
        if (signature.arity(group.keyword->value()) == Signature::MultiValue)
            return group.keyword;
    }
    return nullptr;
}

struct KeywordedFiles
{
    QString keyword;
    QString files;
};

static QList<KeywordedFiles> newFilesForCommand(CommandAST *command,
                                                const FilePaths &filePaths,
                                                const FilePath &projDir)
{
    if (command->isNamed("qt_add_qml_module") || command->isNamed("qt6_add_qml_module")) {
        FilePaths sourceFiles;
        FilePaths resourceFiles;
        FilePaths qmlFiles;

        for (const auto &file : filePaths) {
            using namespace Utils::Constants;
            const auto mimeType = Utils::mimeTypeForFile(file);
            if (mimeType.matchesName(CPP_SOURCE_MIMETYPE)
                || mimeType.matchesName(CPP_HEADER_MIMETYPE)
                || mimeType.matchesName(OBJECTIVE_C_SOURCE_MIMETYPE)
                || mimeType.matchesName(OBJECTIVE_CPP_SOURCE_MIMETYPE)) {
                sourceFiles << file;
            } else if (mimeType.matchesName(QML_MIMETYPE)
                       || mimeType.matchesName(QMLUI_MIMETYPE)
                       || mimeType.matchesName(QMLPROJECT_MIMETYPE)
                       || mimeType.matchesName(JS_MIMETYPE)
                       || mimeType.matchesName(JSON_MIMETYPE)) {
                qmlFiles << file;
            } else {
                resourceFiles << file;
            }
        }

        QList<KeywordedFiles> result;
        if (!sourceFiles.isEmpty())
            result.append({"SOURCES", relativeFilePaths(sourceFiles, projDir)});
        if (!resourceFiles.isEmpty())
            result.append({"RESOURCES", relativeFilePaths(resourceFiles, projDir)});
        if (!qmlFiles.isEmpty())
            result.append({"QML_FILES", relativeFilePaths(qmlFiles, projDir)});

        return result;
    }

    return {{{}, relativeFilePaths(filePaths, projDir)}};
}

static std::optional<Link> cmakeFileForBuildKey(const QString &buildKey,
                                                const QList<CMakeBuildTarget> &targets)
{
    auto target = Utils::findOrDefault(targets, [buildKey](const CMakeBuildTarget &target) {
        return target.title == buildKey;
    });
    if (target.backtrace.isEmpty()) {
        qCCritical(cmakeBuildSystemLog) << "target.backtrace for" << buildKey << "is empty."
                                        << "The location where to add the files is unknown.";
        return std::nullopt;
    }
    return std::make_optional(Link(target.backtrace.last().path, target.backtrace.last().line));
}

static DocumentPtr getUncachedCMakeFile(const FilePath &targetCMakeFile)
{
    // Have a fresh look at the CMake file, not relying on a cached value
    Core::DocumentManager::saveModifiedDocumentSilently(
        Core::DocumentModel::documentForFilePath(targetCMakeFile));

    const DocumentPtr document = parseCMakeFile(targetCMakeFile);
    if (!document->isValid()) {
        qCCritical(cmakeBuildSystemLog).noquote()
            << targetCMakeFile.toUserOutput()
            << "failed to parse! Error:" << document->errorString();
        return {};
    }
    return document;
}

using CommandPredicate = std::function<bool(CommandAST *)>;

static CommandAST *findCommand(const DocumentPtr &document, const CommandPredicate &predicate,
                               bool reverse = false)
{
    const QList<CommandAST *> &commands = document->commands();
    if (reverse) {
        const auto command = std::find_if(commands.rbegin(), commands.rend(), predicate);
        return command == commands.rend() ? nullptr : *command;
    }
    const auto command = std::find_if(commands.begin(), commands.end(), predicate);
    return command == commands.end() ? nullptr : *command;
}

// Of the commands the predicate accepts, the one that runs closest to the
// anchor: a command in a conditional branch the anchor is not part of does not
// apply to it, and a command sharing the anchor's branch beats one further out.
static CommandAST *findCommandNear(const DocumentPtr &document, CommandAST *anchor,
                                   const CommandPredicate &predicate)
{
    CommandAST *nearest = nullptr;
    int nearestDepth = -1;
    for (CommandAST *command : document->commands()) {
        if (!predicate(command) || !document->runsWith(command, anchor))
            continue;
        const int depth = document->enclosingConstructs(command).size();
        if (depth > nearestDepth) {
            nearest = command;
            nearestDepth = depth;
        }
    }
    return nearest;
}

static CommandPredicate namedWithFirstArgument(QAnyStringView name, const QString &firstArgument)
{
    return [name, firstArgument](CommandAST *command) {
        ArgumentAST *argument = command->arguments().first();
        return command->isNamed(name) && argument && argument->value() == firstArgument;
    };
}

static CMakeBuildSystem::SnippetAndLocation snippetAfterLastArgument(CommandAST *command,
                                                                     const QString &newSourceFiles)
{
    const Token &lastArgument = command->arguments().last()->token;
    return {QString("\n%1").arg(newSourceFiles),
            lastArgument.line,
            lastArgument.column + lastArgument.length - 1};
}

// The whitespace the line of the token starts with, where the token is the
// first thing on it. Nothing where something else stands there already: a
// value that shares a line has no indentation of its own to hand on.
static std::optional<QString> indentationOf(const DocumentPtr &document, const Token &token)
{
    const QStringView source = document->source();
    const int position = qBound(0, token.position, int(source.size()));
    int lineStart = position;
    while (lineStart > 0 && source.at(lineStart - 1) != QLatin1Char('\n'))
        --lineStart;

    const QStringView indentation = source.sliced(lineStart, position - lineStart);
    if (!Utils::allOf(indentation, [](QChar character) { return character.isSpace(); }))
        return std::nullopt;
    return indentation.toString();
}

// The keywords of target_sources(), which no signature of the project spells
// out: the scopes it hands the sources to, and the file set form of the call.
static Signature targetSourcesSignature()
{
    Signature signature;
    signature.add({"PRIVATE", "PUBLIC", "INTERFACE", "BASE_DIRS", "FILES"}, Signature::MultiValue);
    signature.add({"FILE_SET", "TYPE"}, Signature::OneValue);
    return signature;
}

// The scope that target_sources() has to name for the target: an interface
// library compiles nothing of its own, and CMake takes INTERFACE sources only
// for it.
static QString sourcesScope(CommandAST *command)
{
    static const QSet<QString> libraryCommands{"add_library",
                                               "qt_add_library",
                                               "qt6_add_library"};
    const bool isInterface = Utils::anyOf(command->arguments(), [](ArgumentAST *argument) {
        return argument->value() == "INTERFACE";
    });
    return libraryCommands.contains(command->commandName().toLower()) && isInterface
               ? QString("INTERFACE")
               : QString("PRIVATE");
}

// The target_sources() that runs with the target, or a new one behind the call
// that defined it. The files join the group of the scope the target takes:
// another scope hands them to whoever links it, and a FILE_SET group would
// take them for files of that set, which are not compiled sources.
static CMakeBuildSystem::SnippetAndLocation generateSnippetAndLocationForTargetSources(
        const QString &newSourceFiles,
        const DocumentPtr &document,
        CommandAST *command,
        const QString &targetName)
{
    const QString scope = sourcesScope(command);
    CommandAST *targetSources = findCommandNear(document, command,
                                                namedWithFirstArgument("target_sources",
                                                                       targetName));
    if (targetSources) {
        ArgumentAST *lastValue = lastValueOfKeyword(targetSources,
                                                    targetSourcesSignature(),
                                                    scope);
        if (!lastValue) {
            return snippetAfterLastArgument(targetSources,
                                            QString("    %1\n        %2")
                                                .arg(scope, newSourceFiles));
        }

        // The file lays the new value out the way it lays out the one it
        // follows: under it where that has a line of its own, and next to it
        // where the values stand together on one.
        const Token &token = lastValue->token;
        const std::optional<QString> indentation = indentationOf(document, token);
        return {indentation ? QString("\n%1%2").arg(*indentation, newSourceFiles)
                            : QString(" %1").arg(newSourceFiles),
                token.line,
                token.column + token.length - 1};
    }

    return {QString("\ntarget_sources(%1\n    %2\n        %3\n)\n")
                .arg(targetName, scope, newSourceFiles),
            command->lineEnd() + 1,
            0};
}

static CMakeBuildSystem::SnippetAndLocation generateSnippetAndLocationForSources(
        const QString &newSourceFiles,
        const DocumentPtr &document,
        CommandAST *command,
        const QString &targetName)
{
    static const QSet<QString> knownCommands{"add_executable",
                                             "add_library",
                                             "qt_add_executable",
                                             "qt_add_library",
                                             "qt6_add_executable",
                                             "qt6_add_library",
                                             "qt_add_qml_module",
                                             "qt6_add_qml_module"};

    if (knownCommands.contains(command->commandName().toLower()))
        return snippetAfterLastArgument(command, newSourceFiles);

    return generateSnippetAndLocationForTargetSources(newSourceFiles, document, command, targetName);
}

// The files go after the last value of the keyword that already takes their
// kind, so that the keyword is not spelled a second time. Only a keyword the
// call does not have yet is added with them.
static QList<CMakeBuildSystem::SnippetAndLocation> generateSnippetsAndLocationsForSources(
        const QList<KeywordedFiles> &newFiles,
        const DocumentPtr &document,
        CommandAST *command,
        const QString &targetName,
        const Signature &signature)
{
    QList<CMakeBuildSystem::SnippetAndLocation> result;
    for (const KeywordedFiles &newFile : newFiles) {
        ArgumentAST *lastValue = newFile.keyword.isEmpty()
                                     ? nullptr
                                     : lastValueOfKeyword(command, signature, newFile.keyword);
        if (!lastValue) {
            const QString files = newFile.keyword.isEmpty()
                                      ? newFile.files
                                      : QString("%1 %2").arg(newFile.keyword, newFile.files);
            result.append(
                generateSnippetAndLocationForSources(files, document, command, targetName));
            continue;
        }

        const Token &token = lastValue->token;
        result.append({QString("\n%1").arg(newFile.files),
                       token.line,
                       token.column + token.length - 1});
    }

    // Insert from the last location towards the first, where an insertion
    // cannot move a location that is still to come. Locations that coincide
    // keep the order the files came in.
    std::reverse(result.begin(), result.end());
    std::stable_sort(result.begin(),
                     result.end(),
                     [](const CMakeBuildSystem::SnippetAndLocation &first,
                        const CMakeBuildSystem::SnippetAndLocation &second) {
                         return std::tie(first.line, first.column)
                                > std::tie(second.line, second.column);
                     });
    return result;
}

// A snippet that carries no indentation of its own is laid out by the
// indenter, which reads the code style the user set for CMake. One that
// spells its own out is left alone, so that what it writes is what it says.
enum SnippetIndentation {
    IndentSnippet,
    SnippetIsIndented
};

static Result<bool> insertSnippetSilently(const FilePath &cmakeFile,
                                          const CMakeBuildSystem::SnippetAndLocation &snippetLocation,
                                          SnippetIndentation indentation = IndentSnippet)
{
    BaseTextEditor *editor = qobject_cast<BaseTextEditor *>(Core::EditorManager::openEditorAt(
        {cmakeFile, int(snippetLocation.line), int(snippetLocation.column)},
        Constants::CMAKE_EDITOR_ID,
        Core::EditorManager::DoNotMakeVisible | Core::EditorManager::DoNotChangeCurrentEditor));
    if (!editor) {
        return ResultError("BaseTextEditor cannot be obtained for " + cmakeFile.toUserOutput()
                               + ":" + QString::number(snippetLocation.line) + ":"
                               + QString::number(snippetLocation.column));
    }
    editor->insert(snippetLocation.snippet);
    if (indentation == IndentSnippet)
        editor->editorWidget()->autoIndent();
    if (!Core::DocumentManager::saveDocument(editor->document()))
        return ResultError("Changes to " + cmakeFile.toUserOutput() + " could not be saved.");
    return true;
}

static CMakeBuildSystem::SnippetAndLocation generateSnippetWithTargetPropertyBlock(
    const QString &projectName, const QString &snippet, const DocumentPtr &document)
{
    CMakeBuildSystem::SnippetAndLocation result;

    result.snippet = QString("\nset_target_properties(%1 PROPERTIES\n   %2)\n").arg(projectName, snippet);

    CommandAST *addExecutable = findCommand(document, [](CommandAST *command) {
        return command->isNamed("qt_add_executable") || command->isNamed("add_executable");
    });
    if (addExecutable) {
        result.line = addExecutable->lineEnd() + 1;
        result.column = 0;
    }
    return result;
}

// The commands the target runs into: the ones it reaches unconditionally, plus
// those a condition mentioning `condition` or an else branch guards.
static QList<CommandAST *> findConditionalCommands(const DocumentPtr &document,
                                                   QAnyStringView name,
                                                   const QString &condition)
{
    auto governs = [&condition](AST *construct) {
        if (construct->asElseClause())
            return true;

        CommandAST *branch = nullptr;
        if (ElseIfClauseAST *elseIf = construct->asElseIfClause())
            branch = elseIf->command;
        else if (IfAST *ifAst = construct->asIf())
            branch = ifAst->ifCommand;
        if (!branch)
            return false;

        return Utils::anyOf(branch->arguments(), [&condition](ArgumentAST *argument) {
            return argument->value().contains(condition);
        });
    };

    QList<CommandAST *> commands;
    for (CommandAST *command : document->commands()) {
        if (!command->isNamed(name))
            continue;
        const QList<AST *> constructs = document->enclosingConstructs(command);
        if (constructs.isEmpty() || Utils::anyOf(constructs, governs))
            commands.append(command);
    }
    return commands;
}

static CMakeBuildSystem::SnippetAndLocation generateSnippetForExistingTargetPropertyBlock(
    const QString &snippet, CommandAST *command)
{
    CMakeBuildSystem::SnippetAndLocation result;
    int insertLine = command->name.line;

    if (ArgumentAST *lastArgument = command->arguments().last()) {
        insertLine = lastArgument->token.line - 1;
        result.column = lastArgument->token.column + lastArgument->token.length + 1;
    }

    result.line = insertLine + 1;
    result.snippet = "\n" + snippet;

    return result;
}

bool CMakeBuildSystem::addTargetProperty(ProjectExplorer::Node *context, const QString &property,
                                         const QString &value, const std::string &condition)
{
    if (!context)
        return false;

    auto node = dynamic_cast<CMakeTargetNode *>(context);
    const QString targetName = node->buildKey();

    const std::optional<Link> cmakeFile = cmakeFileForBuildKey(targetName, buildTargets());
    if (!cmakeFile)
        return false;

    const DocumentPtr document = getUncachedCMakeFile(cmakeFile->targetFilePath);
    if (!document)
        return false;

    const QList<CommandAST *> commands = findConditionalCommands(document,
                                                                 "set_target_properties",
                                                                 QString::fromStdString(condition));

    for (CommandAST *command : commands) {
        for (ArgumentAST *argument : command->arguments()) {
            if (argument->value() == property)
                return false;
        }
    }

    SnippetAndLocation insertLocation;
    QString snippet = QStringLiteral("%1 \"%2\"").arg(property, value);

    if (commands.isEmpty())
        insertLocation = generateSnippetWithTargetPropertyBlock(targetName, snippet, document);
    else
        insertLocation = generateSnippetForExistingTargetPropertyBlock(snippet, commands.first());

    Result<bool> inserted = insertSnippetSilently(cmakeFile->targetFilePath, insertLocation);
    return inserted.value_or(false);
}

static void findLastRelevantArgument(CommandAST *command,
                                     int minimumArgPos,
                                     const QSet<QString> &lowerCaseStopParams,
                                     QString *lastRelevantArg,
                                     int *lastRelevantPos)
{
    const ListView<ArgumentAST *> arguments = command->arguments();
    const int size = arguments.size();
    *lastRelevantPos = size - 1;
    for (int i = minimumArgPos; i < size; ++i) {
        const QString lowerArg = arguments.at(i)->value().toLower();
        if (lowerCaseStopParams.contains(lowerArg)) {
            *lastRelevantPos = i - 1;
            break;
        }
        *lastRelevantArg = lowerArg;
    }
}

static CommandAST *findSetCommandFor(const DocumentPtr &document,
                                     const QString &lowerVariableName)
{
    return findCommand(document, [&lowerVariableName](CommandAST *command) {
        ArgumentAST *argument = command->arguments().first();
        return command->isNamed("set") && argument
               && argument->value().toLower() == lowerVariableName;
    });
}

static CommandAST *handleTSAddVariant(const DocumentPtr &document,
                                      const QStringList &commandNames,
                                      std::optional<QString> targetName,
                                      const QSet<QString> &stopParams,
                                      int *lastArgumentPos)
{
    CommandAST *current = findCommand(document, [&commandNames, &targetName](CommandAST *command) {
        if (!Utils::anyOf(commandNames, [command](const QString &name) {
                return command->isNamed(name);
            })) {
            return false;
        }
        if (!targetName)
            return true;
        ArgumentAST *argument = command->arguments().first();
        return argument && argument->value() == *targetName;
    });
    if (!current)
        return nullptr;

    QString lastRelevant;
    const int argsMinimumPos = targetName.has_value() ? 2 : 1;
    findLastRelevantArgument(current, argsMinimumPos, stopParams, &lastRelevant, lastArgumentPos);

    if (lastRelevant.startsWith('$')) {
        QString var = lastRelevant.mid(1);
        if (var.startsWith('{') && var.endsWith('}'))
            var = var.mid(1, var.size() - 2);
        if (!var.isEmpty()) {
            if (CommandAST *setCommand = findSetCommandFor(document, var)) {
                *lastArgumentPos = setCommand->arguments().size() - 1;
                return setCommand;
            }
        }
    }
    // No variable used or we failed to find the respective set().
    return current;
}

static CommandAST *handleQtAddTranslations(const DocumentPtr &document,
                                           std::optional<QString> targetName,
                                           int *lastArgumentPos)
{
    const QSet<QString> stopParams{"resource_prefix", "output_targets",
                                   "qm_files_output_variable", "sources", "include_directories",
                                   "lupdate_options", "lrelease_options"};
    return handleTSAddVariant(document, {"qt6_add_translations", "qt_add_translations"},
                              targetName, stopParams, lastArgumentPos);
}

static CommandAST *handleQtAddLupdate(const DocumentPtr &document,
                                      std::optional<QString> targetName,
                                      int *lastArgumentPos)
{
    const QSet<QString> stopParams{"sources", "include_directories", "no_global_target", "options"};
    return handleTSAddVariant(document, {"qt6_add_lupdate", "qt_add_lupdate"},
                              targetName, stopParams, lastArgumentPos);
}

static CommandAST *handleQtCreateTranslation(const DocumentPtr &document, int *lastArgumentPos)
{
    return handleTSAddVariant(document, {"qt_create_translation", "qt5_create_translation"},
                              std::nullopt, {"options"}, lastArgumentPos);
}

static CommandPredicate startsOnLine(int line)
{
    return [line](CommandAST *command) { return command->name.line == line; };
}

static Result<bool> insertQtAddTranslations(const DocumentPtr &document,
                                                  const FilePath &targetCmakeFile,
                                                  const QString &targetName,
                                                  int targetDefinitionLine,
                                                  const QString &filesToAdd,
                                                  int qtMajorVersion,
                                                  bool addLinguist)
{
    CommandAST *command = findCommand(document, startsOnLine(targetDefinitionLine));
    if (!command)
        return false;

    // FIXME: room for improvement
    // * this just updates "the current cmake path" for e.g. conditional setups like
    //   differentiating between desktop and device build config we do not update all
    QString snippet;
    if (qtMajorVersion == 5)
        snippet = QString("\nqt_create_translation(QM_FILES %1)\n").arg(filesToAdd);
    else
        snippet = QString("\nqt_add_translations(%1 TS_FILES %2)\n").arg(targetName, filesToAdd);

    const int insertionLine = command->lineEnd() + 1;
    Result<bool> inserted = insertSnippetSilently(targetCmakeFile,
                                                        {snippet, insertionLine, 0});
    if (!inserted || !addLinguist)
        return inserted;

    command = findCommand(document, [](CommandAST *command) {
        return command->isNamed("find_package");
    }, /* reverse = */ true);
    if (!command) {
        qCCritical(cmakeBuildSystemLog) << "Failed to find a find_package().";
        return inserted; // we just fail to insert LinguistTool, but otherwise succeeded
    }
    if (insertionLine < command->lineEnd() + 1) {
        qCCritical(cmakeBuildSystemLog) << "find_package() calls after old insertion. "
                                           "Refusing to process.";
        return inserted; // we just fail to insert LinguistTool, but otherwise succeeded
    }

    snippet = QString("find_package(Qt%1 REQUIRED COMPONENTS LinguistTools)\n").arg(qtMajorVersion);
    return insertSnippetSilently(targetCmakeFile, {snippet, command->lineEnd() + 1, 0});
}

bool CMakeBuildSystem::addTsFiles(Node *context, const FilePaths &filePaths, FilePaths *notAdded)
{
    if (notAdded)
        notAdded->append(filePaths);

    if (auto n = dynamic_cast<CMakeTargetNode *>(context)) {
        const QString targetName = n->buildKey();
        const std::optional<Link> cmakeFile = cmakeFileForBuildKey(targetName, buildTargets());
        if (!cmakeFile.has_value())
            return false;

        const FilePath targetCMakeFile = cmakeFile->targetFilePath;
        const DocumentPtr document = getUncachedCMakeFile(targetCMakeFile);
        if (!document)
            return false;

        int lastArgumentPos = -1;
        CommandAST *command = handleQtAddTranslations(document, targetName, &lastArgumentPos);
        if (!command)
            command = handleQtAddLupdate(document, targetName, &lastArgumentPos);
        if (!command)
            command = handleQtCreateTranslation(document, &lastArgumentPos);

        const QString filesToAdd = relativeFilePaths(filePaths, n->filePath().canonicalPath());
        bool linguistToolsMissing = false;
        int qtMajorVersion = -1;
        if (!command) {
            if (auto qt = m_findPackagesFilesHash.value("Qt6Core"); qt.hasValidTarget())
                qtMajorVersion = 6;
            else if (auto qt = m_findPackagesFilesHash.value("Qt5Core"); qt.hasValidTarget())
                qtMajorVersion = 5;

            if (qtMajorVersion != -1) {
                const QString linguistTools = QString("Qt%1LinguistTools").arg(qtMajorVersion);
                auto linguist = m_findPackagesFilesHash.value(linguistTools);
                linguistToolsMissing = !linguist.hasValidTarget();
            }

            // we failed to find any pre-existing, add one ourself
            Result<bool> inserted = insertQtAddTranslations(document,
                                                                  targetCMakeFile,
                                                                  targetName,
                                                                  cmakeFile->target.line,
                                                                  filesToAdd,
                                                                  qtMajorVersion,
                                                                  linguistToolsMissing);
            if (!inserted)
                qCCritical(cmakeBuildSystemLog) << inserted.error();
            else if (notAdded)
                notAdded->removeIf([filePaths](const FilePath &p) { return filePaths.contains(p); });

            return inserted.value_or(false);
        }

        ArgumentAST *lastArgument = command->arguments().at(lastArgumentPos);
        QTC_ASSERT(lastArgument, return false);

        const Token &token = lastArgument->token;
        const SnippetAndLocation snippetLocation{QString("\n%1").arg(filesToAdd),
                                                 token.line,
                                                 token.column + token.length - 1};

        Result<bool> inserted = insertSnippetSilently(targetCMakeFile, snippetLocation);
        if (!inserted) {
            qCCritical(cmakeBuildSystemLog) << inserted.error();
            return false;
        }

        if (notAdded)
            notAdded->removeIf([filePaths](const FilePath &p) { return filePaths.contains(p); });
        return true;
    }
    return false;
}

// Qt generates the code of a tracepoint provider for its own modules only, so
// a project that adds tracepoints of its own carries the rule that runs
// tracegen over the provider. It comes as a module of its own, next to the
// CMakeLists.txt that includes it, so that the file keeps naming what the
// target is rather than how a provider is built.
static const char QT_TRACING_MODULE[] = "QtTracing.cmake";

static Result<bool> writeQtTracingModule(const FilePath &directory)
{
    const FilePath module = directory.pathAppended(QT_TRACING_MODULE);
    if (module.exists())
        return true;

    QFile contents(":/cmakeproject/cmake/QtTracing.cmake");
    if (!contents.open(QIODevice::ReadOnly))
        return ResultError(QString("%1 could not be read.").arg(QT_TRACING_MODULE));

    const Result<qint64> written = module.writeFileContents(contents.readAll());
    if (!written)
        return ResultError(written.error());
    return true;
}

static bool includesQtTracing(CommandAST *command)
{
    ArgumentAST *argument = command->arguments().first();
    return command->isNamed("include") && argument
           && argument->value().endsWith(QLatin1String(QT_TRACING_MODULE));
}

static CommandPredicate addsTracepoints(const QString &targetName, const QString &provider)
{
    return [targetName, provider](CommandAST *command) {
        const ListView<ArgumentAST *> arguments = command->arguments();
        return command->isNamed("qt_add_tracepoints") && arguments.size() > 1
               && arguments.at(0)->value() == targetName && arguments.at(1)->value() == provider;
    };
}

// A call per provider, after the one that defined the target, and the include()
// that brings qt_add_tracepoints() in where the file does not have it yet. A
// provider the file already names is left alone, so that adding it a second
// time changes nothing.
static Result<bool> insertQtAddTracepoints(const DocumentPtr &document,
                                           const FilePath &targetCMakeFile,
                                           const QString &targetName,
                                           int targetDefinitionLine,
                                           const FilePaths &filePaths)
{
    CommandAST *command = findCommand(document, startsOnLine(targetDefinitionLine));
    if (!command) {
        return ResultError(
            QString("Failed to locate the target defining command at %1").arg(targetDefinitionLine));
    }

    // Both sides of the relative path have to be canonical, or a project
    // reached through a symbolic link gets a path that leads back out of it.
    const FilePath directory = targetCMakeFile.parentDir().canonicalPath();
    QString snippet;
    for (const FilePath &filePath : filePaths) {
        const QString provider = filePath.canonicalPath().relativePathFromDir(directory);
        if (!findCommandNear(document, command, addsTracepoints(targetName, provider)))
            snippet += QString("qt_add_tracepoints(%1 %2)\n").arg(targetName, quoteString(provider));
    }
    if (snippet.isEmpty())
        return true;

    // The module is written only where the include() of it is: a file that
    // includes one of its own, which may well be a copy elsewhere in the
    // project, is not given a second one that nothing reads.
    if (!findCommandNear(document, command, includesQtTracing)) {
        const Result<bool> written = writeQtTracingModule(directory);
        if (!written)
            return written;
        snippet.prepend(QString("include(%1)\n").arg(QT_TRACING_MODULE));
    }

    return insertSnippetSilently(targetCMakeFile,
                                 {QString("\n%1").arg(snippet), command->lineEnd() + 1, 0});
}

bool CMakeBuildSystem::addTracepointFiles(Node *context, const FilePaths &filePaths,
                                          FilePaths *notAdded)
{
    if (notAdded)
        notAdded->append(filePaths);

    auto n = dynamic_cast<CMakeTargetNode *>(context);
    if (!n)
        return false;

    const QString targetName = n->buildKey();
    const std::optional<Link> cmakeFile = cmakeFileForBuildKey(targetName, buildTargets());
    if (!cmakeFile)
        return false;

    const DocumentPtr document = getUncachedCMakeFile(cmakeFile->targetFilePath);
    if (!document)
        return false;

    const Result<bool> inserted = insertQtAddTracepoints(document,
                                                         cmakeFile->targetFilePath,
                                                         targetName,
                                                         cmakeFile->target.line,
                                                         filePaths);
    if (!inserted) {
        qCCritical(cmakeBuildSystemLog) << inserted.error();
        return false;
    }

    if (notAdded)
        notAdded->removeIf([filePaths](const FilePath &p) { return filePaths.contains(p); });
    return true;
}

// The value the call gives the target the property, where it gives it one:
// set_target_properties() names the targets before PROPERTIES and takes a
// property and its value over and over, set_property() names them after TARGET
// and takes one property with the values it holds. A name read wherever it
// stands would take the value of another property for one.
static std::optional<QString> targetPropertyValue(CommandAST *command,
                                                  const QString &targetName,
                                                  const QString &property)
{
    const bool setsProperties = command->isNamed("set_target_properties");
    if (!setsProperties && !command->isNamed("set_property"))
        return std::nullopt;

    Signature signature;
    if (setsProperties) {
        signature.add({"PROPERTIES"}, Signature::MultiValue);
    } else {
        signature.add({"TARGET", "PROPERTY"}, Signature::MultiValue);
        signature.add({"APPEND", "APPEND_STRING"}, Signature::Option);
    }

    QList<ArgumentAST *> targets;
    if (setsProperties) {
        const QList<KeywordArguments> groups = groupArguments(command, signature);
        if (!groups.isEmpty() && !groups.first().keyword)
            targets = groups.first().values;
    } else {
        targets = valuesOfKeyword(command, signature, "TARGET");
    }
    if (!Utils::anyOf(targets, [&targetName](ArgumentAST *argument) {
            return argument->value() == targetName;
        })) {
        return std::nullopt;
    }

    const QList<ArgumentAST *> properties
        = valuesOfKeyword(command,
                          signature,
                          setsProperties ? QString("PROPERTIES") : QString("PROPERTY"));
    if (!setsProperties) {
        if (properties.size() > 1 && properties.first()->value() == property)
            return properties.at(1)->value();
        return std::nullopt;
    }
    for (int i = 0, last = properties.size() - 1; i < last; i += 2) {
        if (properties.at(i)->value() == property)
            return properties.at(i + 1)->value();
    }
    return std::nullopt;
}

// The value the call leaves CMAKE_AUTORCC with in the scope the target is
// defined in: one that spells PARENT_SCOPE writes the scope around this one
// instead, and one that hands over no value takes the variable away.
static std::optional<bool> autorccFromVariable(CommandAST *command)
{
    const bool unsets = command->isNamed("unset");
    if (!unsets && !command->isNamed("set"))
        return std::nullopt;

    const ListView<ArgumentAST *> arguments = command->arguments();
    ArgumentAST *variable = arguments.first();
    if (!variable || variable->value() != "CMAKE_AUTORCC")
        return std::nullopt;

    auto spells = [&arguments](const QString &keyword) {
        return Utils::anyOf(arguments, [&keyword](ArgumentAST *argument) {
            return argument->value() == keyword;
        });
    };
    if (spells("PARENT_SCOPE"))
        return std::nullopt;
    // unset(<variable> CACHE) takes the cache entry away and leaves the
    // variable of the scope standing.
    if (unsets)
        return spells("CACHE") ? std::nullopt : std::make_optional(false);
    // set(<variable>) is how set() takes one away.
    if (arguments.size() < 2)
        return false;
    // A value that cannot be read, set(CMAKE_AUTORCC ${ENABLE_RESOURCES})
    // among them, counts as off: nothing is what the variable stands for
    // where it is empty or never set, and taking it for on leaves the .qrc
    // a source that nothing compiles. Taking it for off costs a line that
    // changes nothing where AUTORCC is on already.
    return CMakeConfigItem::toBool(arguments.at(1)->value()).value_or(false);
}

// Whether rcc already runs over the .qrc files among the sources of the target:
// CMAKE_AUTORCC held a true value where the target was defined, or the target
// carries the property itself. Only the file that defines the target is read,
// so a value a directory further up sets is missed and the property is written
// a second time, where it changes nothing. A property whose value cannot be
// read counts as true: it stands behind the call that defines the target and
// is what the target ends up with, whatever is written above it.
static bool usesAutorcc(const DocumentPtr &document, CommandAST *command, const QString &targetName)
{
    bool autorcc = false;
    for (CommandAST *candidate : document->commands()) {
        if (!document->runsWith(candidate, command))
            continue;

        // A variable the file sets behind the call does not reach the target:
        // it takes the properties it is created with.
        if (candidate->name.begin() < command->name.begin()) {
            if (const std::optional<bool> value = autorccFromVariable(candidate)) {
                autorcc = *value;
                continue;
            }
        }
        if (const std::optional<QString> value
            = targetPropertyValue(candidate, targetName, "AUTORCC")) {
            autorcc = CMakeConfigItem::toBool(*value).value_or(true);
        }
    }
    return autorcc;
}

// A .qrc file is compiled by rcc, which AUTORCC runs over the .qrc files the
// sources of a target name. The keywords that take resources take the files a
// resource is assembled out of instead: qt_add_resources(<target> <name> FILES
// ...) and the RESOURCES of qt_add_qml_module() write a .qrc of their own
// listing them, so a .qrc handed to one of them ends up embedded as a file
// rather than compiled. Only the variable form, qt_add_resources(<var>
// file.qrc), compiles one, and what it hands back is the generated source, so
// the .qrc itself would no longer be part of the target and would drop out of
// the project tree.
static Result<bool> insertQrcFiles(const DocumentPtr &document,
                                   const FilePath &targetCMakeFile,
                                   const QString &targetName,
                                   int targetDefinitionLine,
                                   const FilePaths &filePaths,
                                   const FilePath &projDir)
{
    CommandAST *command = findCommand(document, startsOnLine(targetDefinitionLine));
    if (!command) {
        return ResultError(
            QString("Failed to locate the target defining command at %1").arg(targetDefinitionLine));
    }

    // The Qt CMake API leaves AUTORCC off - qt_standard_project_setup() turns
    // on AUTOMOC and AUTOUIC only - so the target is given the property along
    // with the first .qrc that needs it.
    QList<CMakeBuildSystem::SnippetAndLocation> snippets;
    if (!usesAutorcc(document, command, targetName)) {
        snippets.append({QString("\nset_target_properties(%1 PROPERTIES AUTORCC ON)\n")
                             .arg(targetName),
                         command->lineEnd() + 1,
                         0});
    }
    // The keywords of the call that defined the target are each meant for a
    // kind of file, and a .qrc is none of them, so it goes in as a source.
    snippets.append(
        generateSnippetAndLocationForTargetSources(relativeFilePaths(filePaths, projDir),
                                                   document,
                                                   command,
                                                   targetName));

    // Insert from the last location towards the first, where an insertion
    // cannot move a location that is still to come. Where both go to the same
    // place, inserting the one behind first leaves them in this order.
    std::reverse(snippets.begin(), snippets.end());
    std::stable_sort(snippets.begin(),
                     snippets.end(),
                     [](const CMakeBuildSystem::SnippetAndLocation &first,
                        const CMakeBuildSystem::SnippetAndLocation &second) {
                         return std::tie(first.line, first.column)
                                > std::tie(second.line, second.column);
                     });
    for (const CMakeBuildSystem::SnippetAndLocation &snippet : snippets) {
        const Result<bool> inserted = insertSnippetSilently(targetCMakeFile,
                                                            snippet,
                                                            SnippetIsIndented);
        if (!inserted)
            return inserted;
    }
    return true;
}

bool CMakeBuildSystem::addQrcFiles(Node *context, const FilePaths &filePaths, FilePaths *notAdded)
{
    if (notAdded)
        notAdded->append(filePaths);

    auto n = dynamic_cast<CMakeTargetNode *>(context);
    if (!n)
        return false;

    const QString targetName = n->buildKey();
    const std::optional<Link> cmakeFile = cmakeFileForBuildKey(targetName, buildTargets());
    if (!cmakeFile)
        return false;

    const DocumentPtr document = getUncachedCMakeFile(cmakeFile->targetFilePath);
    if (!document)
        return false;

    const Result<bool> inserted = insertQrcFiles(document,
                                                 cmakeFile->targetFilePath,
                                                 targetName,
                                                 cmakeFile->target.line,
                                                 filePaths,
                                                 n->filePath().canonicalPath());
    if (!inserted) {
        qCCritical(cmakeBuildSystemLog) << inserted.error();
        return false;
    }

    if (notAdded)
        notAdded->removeIf([filePaths](const FilePath &p) { return filePaths.contains(p); });
    return true;
}

// The call that takes files the keywords of the command do not cover: the
// command itself where it takes sources, or a target_sources() next to it.
static CommandAST *commandTakingSources(const DocumentPtr &document,
                                        CommandAST *command,
                                        const QString &targetName)
{
    static const QSet<QString> knownCommands{"add_executable",
                                             "add_library",
                                             "qt_add_executable",
                                             "qt_add_library",
                                             "qt6_add_executable",
                                             "qt6_add_library",
                                             "qt_add_qml_module",
                                             "qt6_add_qml_module"};
    if (knownCommands.contains(command->commandName().toLower()))
        return command;

    return findCommandNear(document, command,
                           namedWithFirstArgument("target_sources", targetName));
}

// Whether values can go at the end of the set(): a CACHE entry or a
// PARENT_SCOPE gives what stands there a meaning a file would take.
static bool takesMoreValues(CommandAST *command)
{
    return !Utils::anyOf(command->arguments(), [](ArgumentAST *argument) {
        const QString value = argument->value();
        return value == "CACHE" || value == "PARENT_SCOPE";
    });
}

// The set() call that fills the variable the argument expands, when the
// argument is nothing but that expansion: of those that run with the command,
// the last one before it, which is the one whose value the command reads.
static CommandAST *setCommandBehind(const DocumentPtr &document,
                                    CommandAST *command,
                                    ArgumentAST *argument)
{
    CommandAST *result = nullptr;
    for (CommandAST *candidate : document->commands()) {
        if (candidate->name.begin() >= command->name.begin())
            break;
        ArgumentAST *variable = candidate->arguments().first();
        if (!candidate->isNamed("set") || !variable
            || argument->value() != QString("${%1}").arg(variable->value())) {
            continue;
        }
        // The last one is the one whose value the command reads, so a file
        // that cannot go there does not go into an earlier list either.
        if (document->runsWith(candidate, command))
            result = takesMoreValues(candidate) ? candidate : nullptr;
    }
    return result;
}

// The set() call the arguments take their files out of, when they all take
// them out of the one. Where they name several such variables, no one of them
// is the list a new file belongs to, and it goes into the call itself.
static CommandAST *setCommandForArguments(const DocumentPtr &document,
                                          CommandAST *command,
                                          const QList<ArgumentAST *> &arguments)
{
    CommandAST *result = nullptr;
    for (ArgumentAST *argument : arguments) {
        CommandAST *set = setCommandBehind(document, command, argument);
        if (!set)
            continue;
        if (result && result != set)
            return nullptr;
        result = set;
    }
    return result;
}

// The set() call that keeps the sources of a call that takes them out of a
// variable, the way the wizards write them. The files go there, so that the
// list stays the one place the sources of the target are named, and the
// branches that all expand it keep building the same ones.
static CommandAST *setCommandForSources(const DocumentPtr &document,
                                        CommandAST *command,
                                        const Signature &signature)
{
    ArgumentAST *target = command->arguments().first();
    QList<ArgumentAST *> sources;
    for (const KeywordArguments &group : groupArguments(command, signature)) {
        if (group.keyword)
            continue;
        for (ArgumentAST *value : group.values) {
            if (value != target)
                sources.append(value);
        }
    }
    return setCommandForArguments(document, command, sources);
}

static Result<bool> insertFilesSilently(const FilePath &targetCMakeFile,
                                        const DocumentPtr &document,
                                        const SignatureTable &signatures,
                                        CommandAST *command,
                                        const QString &targetName,
                                        const FilePaths &filePaths,
                                        const FilePath &projDir)
{
    const QList<KeywordedFiles> newFiles = newFilesForCommand(command, filePaths, projDir);
    const Signature signature = signatures.signature(command->commandName());
    auto lastValue = [&](const KeywordedFiles &newFile) -> ArgumentAST * {
        return newFile.keyword.isEmpty()
                   ? nullptr
                   : lastValueOfKeyword(command, signature, newFile.keyword);
    };

    // Where no call takes sources yet, one is written, which still goes
    // through the snippet that spells it out.
    CommandAST *takesSources = commandTakingSources(document, command, targetName);
    if (!takesSources && !Utils::allOf(newFiles, lastValue)) {
        const QList<CMakeBuildSystem::SnippetAndLocation> snippetLocations
            = generateSnippetsAndLocationsForSources(newFiles,
                                                     document,
                                                     command,
                                                     targetName,
                                                     signature);
        for (const CMakeBuildSystem::SnippetAndLocation &snippetLocation : snippetLocations) {
            const Result<bool> inserted = insertSnippetSilently(targetCMakeFile, snippetLocation);
            if (!inserted)
                return inserted;
        }
        return true;
    }

    // The files go after the last value of the keyword that already takes
    // their kind, so that the keyword is not spelled a second time. Only a
    // keyword the call does not have yet is written with them. Where the place
    // they would take is an expansion of a variable, they go into the set()
    // that fills it instead.
    CommandAST *sourcesSet = takesSources ? setCommandForSources(
                                                document,
                                                takesSources,
                                                signatures.signature(takesSources->commandName()))
                                          : nullptr;

    Rewriter rewriter(document);
    QStringList newKeywords;
    for (const KeywordedFiles &newFile : newFiles) {
        if (ArgumentAST *argument = lastValue(newFile)) {
            const QList<ArgumentAST *> values = valuesOfKeyword(command, signature, newFile.keyword);
            if (CommandAST *set = setCommandForArguments(document, command, values))
                rewriter.append(set, {newFile.files});
            else
                rewriter.insertAfter(argument, {newFile.files});
            continue;
        }
        if (newFile.keyword.isEmpty() && sourcesSet) {
            rewriter.append(sourcesSet, {newFile.files});
            continue;
        }
        newKeywords.append(newFile.keyword.isEmpty()
                               ? newFile.files
                               : QString("%1 %2").arg(newFile.keyword, newFile.files));
    }
    rewriter.append(takesSources, newKeywords);

    return applyEdits(targetCMakeFile, document, rewriter.edits());
}

// Whether any argument of the command expands a variable that file(GLOB) filled.
static bool usesGlobbing(const DocumentPtr &document, CommandAST *command)
{
    QSet<QString> globVariables;
    for (CommandAST *candidate : document->commands()) {
        const ListView<ArgumentAST *> arguments = candidate->arguments();
        if (!candidate->isNamed("file") || arguments.size() < 3)
            continue;
        const QString mode = arguments.at(0)->value();
        if (mode == "GLOB" || mode == "GLOB_RECURSE")
            globVariables.insert(QString("${%1}").arg(arguments.at(1)->value()));
    }

    return Utils::anyOf(command->arguments(), [&globVariables](ArgumentAST *argument) {
        return globVariables.contains(argument->value());
    });
}

bool CMakeBuildSystem::addSrcFiles(Node *context, const FilePaths &filePaths, FilePaths *notAdded)
{
    if (notAdded)
        notAdded->append(filePaths);

    if (auto n = dynamic_cast<CMakeTargetNode *>(context)) {
        const QString targetName = n->buildKey();
        const std::optional<Link> cmakeFile = cmakeFileForBuildKey(targetName, buildTargets());
        if (!cmakeFile)
            return false;

        const FilePath targetCMakeFile = cmakeFile->targetFilePath;
        const int targetDefinitionLine = cmakeFile->target.line;

        const DocumentPtr document = getUncachedCMakeFile(targetCMakeFile);
        if (!document)
            return false;

        CommandAST *command = findCommand(document, startsOnLine(targetDefinitionLine));
        if (!command) {
            qCCritical(cmakeBuildSystemLog) << "Command that defined the target" << targetName
                                            << "could not be found at" << targetDefinitionLine;
            return false;
        }
        ArgumentAST *definedTarget = command->arguments().first();
        if (!definedTarget) {
            qCCritical(cmakeBuildSystemLog) << "Command that defined the target" << targetName
                                            << "has zero arguments.";
            return false;
        }

        const bool haveGlobbing = usesGlobbing(document, command);
        n->setVisibleAfterAddFileAction(!haveGlobbing);
        if (haveGlobbing && cmakeSettingsForProject(project()).autorunCMake()) {
            runCMake();
        } else {
            // Special case: when qt_add_executable and qt_add_qml_module use the same target name
            // then the qt_add_qml_module command should be used
            const QString definedTargetName = definedTarget->value();
            CommandAST *qmlModule = findCommandNear(document, command, [&](CommandAST *candidate) {
                ArgumentAST *argument = candidate->arguments().first();
                return (candidate->isNamed("qt_add_qml_module")
                        || candidate->isNamed("qt6_add_qml_module"))
                       && argument && argument->value() == definedTargetName;
            });
            if (qmlModule)
                command = qmlModule;

            const Result<bool> inserted = insertFilesSilently(targetCMakeFile,
                                                              document,
                                                              m_commandSignatures,
                                                              command,
                                                              targetName,
                                                              filePaths,
                                                              n->filePath().canonicalPath());
            if (!inserted) {
                qCCritical(cmakeBuildSystemLog) << inserted.error();
                return false;
            }
        }

        if (notAdded)
            notAdded->removeIf([filePaths](const FilePath &p) { return filePaths.contains(p); });
        return true;
    }

    return false;
}

bool CMakeBuildSystem::addFiles(Node *context, const FilePaths &filePaths, FilePaths *notAdded)
{
    FilePaths tsFiles, srcFiles;
    std::tie(tsFiles, srcFiles) = Utils::partition(filePaths, [](const FilePath &fp) {
        return Utils::mimeTypeForFile(fp).name() == Utils::Constants::LINGUIST_MIMETYPE;
    });
    FilePaths tracepointFiles;
    std::tie(tracepointFiles, srcFiles) = Utils::partition(srcFiles, [](const FilePath &fp) {
        return fp.suffix() == "tracepoints";
    });
    // The MIME type of a .qrc comes with the resource editor, which a project
    // is built without, so the suffix is what tells one.
    FilePaths qrcFiles;
    std::tie(qrcFiles, srcFiles) = Utils::partition(srcFiles, [](const FilePath &fp) {
        return fp.suffix() == "qrc";
    });
    // Each kind of file is added off the line the target was last parsed on,
    // so the ones that only ever write behind that line go first: the sources
    // may grow a set() the call reads, and the translations the find_package()
    // that comes with them, either of which would leave the target a line
    // further down than the one the others go looking for it on.
    bool success = true;
    if (!qrcFiles.isEmpty() && !addQrcFiles(context, qrcFiles, notAdded))
        success = false;

    if (!tracepointFiles.isEmpty() && !addTracepointFiles(context, tracepointFiles, notAdded))
        success = false;

    if (!srcFiles.isEmpty())
        success = addSrcFiles(context, srcFiles, notAdded) && success;

    if (!tsFiles.isEmpty())
        success = addTsFiles(context, tsFiles, notAdded) && success;

    if (success)
        return true;
    return BuildSystem::addFiles(context, filePaths, notAdded);
}

static Result<CMakeBuildSystem::ProjectFileArgumentPosition> fileArgumentPosition(
        const DocumentPtr &document,
        const SignatureTable &signatures,
        const FilePath &targetCMakeFile,
        int targetDefinitionLine,
        const QString &targetName,
        const QString &fileName)
{
    using ProjectFileArgumentPosition = CMakeBuildSystem::ProjectFileArgumentPosition;

    CommandAST *targetCommand = findCommand(document, startsOnLine(targetDefinitionLine));
    if (!targetCommand) {
        return ResultError(QString("Command that defined the target %1 could not be found at %2")
                               .arg(targetName)
                               .arg(targetDefinitionLine));
    }

    // A file counts wherever it is spelled out, conditional branches included.
    auto namedWithTarget = [&targetName](QAnyStringView name) {
        return [name, &targetName](CommandAST *command) {
            const ListView<ArgumentAST *> arguments = command->arguments();
            return command->isNamed(name) && arguments.size() > 1
                   && arguments.at(0)->value() == targetName;
        };
    };

    CommandAST *targetSources = findCommand(document, namedWithTarget("target_sources"));
    CommandAST *addQmlModule = findCommand(document, namedWithTarget("qt_add_qml_module"));
    if (!addQmlModule)
        addQmlModule = findCommand(document, namedWithTarget("qt6_add_qml_module"));
    CommandAST *setSourceFileProperties = findCommand(document, [](CommandAST *command) {
        return command->isNamed("set_source_files_properties");
    });

    auto position = [&](CommandAST *command, ArgumentAST *argument) {
        const Signature signature = signatures.signature(command->commandName());
        return ProjectFileArgumentPosition{targetCMakeFile,
                                           fileName,
                                           document,
                                           argument,
                                           keywordLeftEmpty(command, signature, argument)};
    };
    auto namesFile = [&fileName](ArgumentAST *argument) { return argument->value() == fileName; };

    for (CommandAST *command :
         {targetCommand, targetSources, addQmlModule, setSourceFileProperties}) {
        if (!command)
            continue;

        if (ArgumentAST *argument = Utils::findOrDefault(command->arguments(), namesFile))
            return position(command, argument);

        // Check if the filename is part of globbing variable result
        if (usesGlobbing(document, command)) {
            ProjectFileArgumentPosition globbed;
            globbed.cmakeFile = targetCMakeFile;
            globbed.relativeFileName = fileName;
            globbed.fromGlobbing = true;
            return globbed;
        }

        // Check if the filename is part of a variable set by the user
        for (ArgumentAST *argument : command->arguments()) {
            for (CommandAST *candidate : document->commands()) {
                const ListView<ArgumentAST *> arguments = candidate->arguments();
                if (!candidate->isNamed("set") || arguments.size() < 2
                    || argument->value() != QString("${%1}").arg(arguments.at(0)->value())) {
                    continue;
                }

                if (ArgumentAST *found = Utils::findOrDefault(arguments, namesFile))
                    return position(candidate, found);
            }
        }
    }

    return ResultError(QString("Command that adds the file %1 to the target %2 could not be found.")
                           .arg(fileName)
                           .arg(targetName));
}

Result<CMakeBuildSystem::ProjectFileArgumentPosition>
CMakeBuildSystem::projectFileArgumentPosition(const QString &targetName, const QString &fileName)
{
    const std::optional<Link> cmakeFile = cmakeFileForBuildKey(targetName, buildTargets());
    if (!cmakeFile)
        return ResultError(QString("Couldn't find CMake file for target: %1").arg(targetName));

    const FilePath targetCMakeFile = cmakeFile->targetFilePath;
    const DocumentPtr document = getUncachedCMakeFile(targetCMakeFile);
    if (!document) {
        return ResultError(
            QString("Cannot parse CMake file: %1").arg(targetCMakeFile.toUserOutput()));
    }

    return fileArgumentPosition(document,
                                m_commandSignatures,
                                targetCMakeFile,
                                cmakeFile->target.line,
                                targetName,
                                fileName);
}

QString CMakeBuildSystem::cmakeGenerator() const
{
    return m_reader.cmakeGenerator();
}

bool CMakeBuildSystem::hasSubprojectBuildSupport() const
{
    return cmakeGenerator().contains("Ninja") || cmakeGenerator().contains("Makefiles");
}

QVariant CMakeBuildSystem::additionalData(Id id) const
{
    if (id == "FoundPackages") {
        // for analytics
        return QVariant::fromValue(m_findPackagesFilesHash);
    }
    return {};
}

static Result<bool> removeFileArgumentSilently(
        const CMakeBuildSystem::ProjectFileArgumentPosition &filePos)
{
    // A keyword that the file is the only value of goes with it.
    Rewriter rewriter(filePos.document);
    rewriter.remove(filePos.keyword ? filePos.keyword : filePos.argument, filePos.argument);
    return applyEdits(filePos.cmakeFile, filePos.document, rewriter.edits());
}

// The argument keeps the place it had: what the line is indented by is the
// author's, and a name is no reason to reconsider it.
static Result<bool> renameFileArgumentSilently(
        const CMakeBuildSystem::ProjectFileArgumentPosition &filePos, const QString &newFileName)
{
    Rewriter rewriter(filePos.document);
    rewriter.replaceValue(filePos.argument, newFileName);
    return applyEdits(filePos.cmakeFile, filePos.document, rewriter.edits());
}

RemovedFilesFromProject CMakeBuildSystem::removeFiles(Node *context,
                                                      const FilePaths &filePaths,
                                                      FilePaths *notRemoved)
{
    FilePaths badFiles;
    if (auto n = dynamic_cast<CMakeTargetNode *>(context)) {
        const FilePath projDir = n->filePath().canonicalPath();
        const QString targetName = n->buildKey();

        bool haveGlobbing = false;
        for (const FilePath &file : filePaths) {
            const QString fileName = file.canonicalPath().relativePathFromDir(projDir);

            auto filePos = projectFileArgumentPosition(targetName, fileName);
            QTC_ASSERT_RESULT(filePos, badFiles << file; continue);

            if (filePos->fromGlobbing) {
                haveGlobbing = true;
                continue;
            }

            const Result<bool> removed = removeFileArgumentSilently(*filePos);
            if (!removed) {
                badFiles << file;
                qCCritical(cmakeBuildSystemLog).noquote() << removed.error();
                continue;
            }
        }

        if (notRemoved && !badFiles.isEmpty())
            *notRemoved = badFiles;

        if (haveGlobbing && cmakeSettingsForProject(project()).autorunCMake())
            runCMake();

        return badFiles.isEmpty() ? RemovedFilesFromProject::Ok : RemovedFilesFromProject::Error;
    }

    return RemovedFilesFromProject::Error;
}

bool CMakeBuildSystem::canRenameFile(Node *context,
                                     const FilePath &oldFilePath,
                                     const FilePath &/*newFilePath*/)
{
    if (auto n = dynamic_cast<CMakeTargetNode *>(context)) {
        const FilePath projDir = n->filePath().canonicalPath();
        const QString oldRelPathName = oldFilePath.canonicalPath().relativePathFromDir(projDir);

        const QString targetName = n->buildKey();

        auto filePos = projectFileArgumentPosition(targetName, oldRelPathName);
        QTC_ASSERT_RESULT(filePos, return false);

        return true;
    }
    return false;
}

bool CMakeBuildSystem::renameFiles(Node *context, const FilePairs &filesToRename, FilePaths *notRenamed)
{
    const auto n = dynamic_cast<CMakeTargetNode *>(context);
    if (!n) {
        if (notRenamed)
            *notRenamed = firstPaths(filesToRename);
        return false;
    }

    bool shouldRunCMake = false;
    bool success = true;
    for (const auto &[oldFilePath, newFilePath] : filesToRename) {
        if (!renameFile(n, oldFilePath, newFilePath, shouldRunCMake)) {
            success = false;
            if (notRenamed)
                *notRenamed << oldFilePath;
        }
    }

    if (shouldRunCMake && cmakeSettingsForProject(project()).autorunCMake())
        runCMake();

    return success;
}

bool CMakeBuildSystem::renameFile(
        CMakeTargetNode *context,
        const Utils::FilePath &oldFilePath,
        const Utils::FilePath &newFilePath,
        bool &shouldRunCMake)
{
    const FilePath projDir = context->filePath();
    const QString newRelPathNameRaw = newFilePath.relativePathFromDir(projDir);
    const QString oldRelPathName = oldFilePath.relativePathFromDir(projDir);

    const QString targetName = context->buildKey();

    auto fileToRename = projectFileArgumentPosition(targetName, oldRelPathName);
    QTC_ASSERT_RESULT(fileToRename, return false);

    bool haveGlobbing = false;
    do {
        if (!fileToRename->fromGlobbing) {
            const Result<bool> renamed = renameFileArgumentSilently(*fileToRename,
                                                                    newRelPathNameRaw);
            if (!renamed) {
                qCCritical(cmakeBuildSystemLog).noquote() << renamed.error();
                return false;
            }
        } else {
            haveGlobbing = true;
        }

        // Try the next occurrence. This can happen if set_source_file_properties is used
        fileToRename = projectFileArgumentPosition(targetName, fileToRename->relativeFileName);
    } while (fileToRename && !fileToRename->fromGlobbing);

    if (haveGlobbing)
        shouldRunCMake = true;
    return true;
}

void CMakeBuildSystem::buildNamedTarget(const QString &target)
{
    CMakeProjectManager::Internal::buildTarget(this, target);
}

void CMakeBuildSystem::buildFile(ProjectExplorer::FileNode *file)
{
    CMakeTargetNode *targetNode = dynamic_cast<CMakeTargetNode *>(file->parentProjectNode());
    if (!targetNode)
        return;
    FilePath filePath = file->filePath();
    if (filePath.fileName().contains(".h")) {
        bool wasHeader = false;
        const FilePath sourceFile = CppEditor::correspondingHeaderOrSource(filePath, &wasHeader);
        if (wasHeader && !sourceFile.isEmpty())
            filePath = sourceFile;
    }
    const QString generator = CMakeGeneratorKitAspect::generator(project()->activeKit());
    const FilePath relativeSource = filePath.relativeChildPath(targetNode->filePath());
    Utils::FilePath targetBase;
    if (generator == "Ninja") {
        const Utils::FilePath relativeBuildDir = targetNode->buildDirectory().relativeChildPath(
                    buildConfiguration()->buildDirectory());
        targetBase = relativeBuildDir / "CMakeFiles" / (targetNode->displayName() + ".dir");
    } else if (!generator.contains("Makefiles")) {
        Core::MessageManager::writeFlashing(addCMakePrefix(
            Tr::tr("Build File is not supported for generator \"%1\"").arg(generator)));
        return;
    }

    const QString sourceFile = targetBase.resolvePath(relativeSource).path();
    const QString objExtension = [&]() -> QString {
        const auto sourceKind = CppEditor::ProjectFile::classify(relativeSource);
        const QByteArray cmakeLangExtension = CppEditor::ProjectFile::isCxx(sourceKind)
                                                  ? "CMAKE_CXX_OUTPUT_EXTENSION"
                                                  : "CMAKE_C_OUTPUT_EXTENSION";
        const QString extension = configurationFromCMake().stringValueOf(cmakeLangExtension);
        if (!extension.isEmpty())
            return extension;

        const auto toolchain = CppEditor::ProjectFile::isCxx(sourceKind)
                                   ? ToolchainKitAspect::cxxToolchain(project()->activeKit())
                                   : ToolchainKitAspect::cToolchain(project()->activeKit());
        using namespace ProjectExplorer::Constants;
        static QSet<Id> objIds{
            CLANG_CL_TOOLCHAIN_TYPEID,
            MSVC_TOOLCHAIN_TYPEID,
            MINGW_TOOLCHAIN_TYPEID,
        };
        if (toolchain && objIds.contains(toolchain->typeId()))
            return ".obj";
        return ".o";
    }();

    buildCMakeTarget(sourceFile + objExtension);
}

bool CMakeBuildSystem::canBuildFile(ProjectExplorer::FileNode *file) const
{
    const QString generator = CMakeGeneratorKitAspect::generator(file->getProject()->activeKit());
    if (generator != "Ninja" && !generator.contains("Makefiles"))
        return false;

    const FileType type = file->fileType();
    if (type != FileType::Source && type != FileType::Header)
        return false;

    return dynamic_cast<CMakeTargetNode *>(file->parentProjectNode());
}

ProjectNode *CMakeBuildSystem::buildableSubProject(ProjectExplorer::FileNode *file) const
{
    if (!file)
        return nullptr;
    const auto targetNode = dynamic_cast<const CMakeTargetNode *>(file->parentProjectNode());
    if (!targetNode)
        return nullptr;
    const CMakeBuildTarget cmakeBuildTarget = targetNode->cmakeBuildTarget();
    if (cmakeBuildTarget.backtrace.isEmpty())
        return nullptr;
    const FilePath targetDefinitionDir = cmakeBuildTarget.backtrace.last().path.parentDir();
    return project()->rootProjectNode()->findProjectNode(
        [&targetDefinitionDir](const ProjectNode *node) {
            if (auto cmakeListsNode = dynamic_cast<const CMakeListsNode *>(node))
                return cmakeListsNode->path() == targetDefinitionDir;
            return false;
        });
}

static Result<bool> insertDependencies(
    const QString &targetName,
    const FilePath &targetCMakeFile,
    int targetDefinitionLine,
    const QStringList &dependencies,
    const QString &qtMajorVersion)
{
    DocumentPtr document = getUncachedCMakeFile(targetCMakeFile);
    if (!document)
        return ResultError("Failed to read " + targetCMakeFile.toUserOutput());

    CommandAST *command = findCommand(document, startsOnLine(targetDefinitionLine));
    if (!command) {
        return ResultError(
            QString("Failed to locate the target defining command at %1").arg(targetDefinitionLine));
    }
    const int targetDefinitionLastLine = command->lineEnd();

    //
    // find_package
    //
    const QString qtPackage = QString("Qt%1").arg(qtMajorVersion);
    command = findCommand(document, namedWithFirstArgument("find_package", qtPackage),
                          /* reverse = */ true);

    const QString findComponents = transform(dependencies, [](const QString &dep) {
                                       QTC_ASSERT(dep.size() > 3, return dep);
                                       return dep.mid(3);
                                   }).join(" ");
    QString snippet = QString("find_package(%1 REQUIRED COMPONENTS %2)\n%3")
                          .arg(qtPackage)
                          .arg(findComponents)
                          .arg(!command ? QString("\n") : QString(""));

    int insertionLine = command ? command->lineEnd() + 1 : targetDefinitionLine;
    Result<bool> inserted = insertSnippetSilently(targetCMakeFile, {snippet, insertionLine, 0});
    if (!inserted)
        return inserted;
    const int insertedFindPackageOffset = 2;

    //
    // target_link_libraries
    //
    document = getUncachedCMakeFile(targetCMakeFile);
    if (!document)
        return ResultError("Failed to re-read " + targetCMakeFile.toUserOutput());

    command = findCommand(document, namedWithFirstArgument("target_link_libraries", targetName),
                          /* reverse = */ true);

    const QString targetPrefix = QString("Qt%1::").arg(qtMajorVersion);
    const QString linkLibraries
        = transform(dependencies, [targetPrefix](const QString &dep) -> QString {
              QTC_ASSERT(dep.size() > 3, return targetPrefix + dep);
              return targetPrefix + dep.mid(3);
          }).join(" ");
    snippet = QString("%1target_link_libraries(%2 PRIVATE %3)\n")
                  .arg(!command ? QString("\n") : QString(""))
                  .arg(targetName)
                  .arg(linkLibraries);

    insertionLine = (command ? command->lineEnd()
                             : targetDefinitionLastLine + insertedFindPackageOffset)
                    + 1;
    return insertSnippetSilently(targetCMakeFile, {snippet, insertionLine, 0});
}

bool CMakeBuildSystem::addDependencies(
    ProjectExplorer::Node *context, const QStringList &dependencies)
{
    if (auto n = dynamic_cast<CMakeTargetNode *>(context)) {
        const QString targetName = n->buildKey();
        const std::optional<Link> cmakeFile = cmakeFileForBuildKey(targetName, buildTargets());
        if (!cmakeFile)
            return false;

        QStringList dependenciesToAdd = dependencies;

        QString qtMajorVersion = "6";
        if (auto qt = m_findPackagesFilesHash.value("Qt5Core"); qt.hasValidTarget())
            qtMajorVersion = "5";

        // Skip existing components
        Utils::erase(dependenciesToAdd, [this, qtMajorVersion](const QString &dependency) {
            // Dependency is given as for example Qt.<Component>
            QTC_ASSERT(dependency.size() > 3, return true);
            return m_findPackagesFilesHash.contains(
                QString("Qt%1%2").arg(qtMajorVersion).arg(dependency.mid(3)));
        });

        if (dependenciesToAdd.isEmpty())
            return true;

        // Special case for Qt5 and Qt6 wizard code resulted from:
        // `find_package(QT NAMES Qt6 Qt5 ...)`
        if (!m_configurationFromCMake.stringValueOf("QT_DIR").isEmpty())
            qtMajorVersion = "${QT_VERSION_MAJOR}";

        Result<bool> inserted = insertDependencies(
            targetName,
            cmakeFile->targetFilePath,
            cmakeFile->target.line,
            dependenciesToAdd,
            qtMajorVersion);
        if (!inserted) {
            qCCritical(cmakeBuildSystemLog) << inserted.error();
            return false;
        }

        return true;
    }
    return BuildSystem::addDependencies(context, dependencies);
}

static FilePaths binariesLinkingTarget(const QList<CMakeBuildTarget> &targets,
                                       const QString &startTarget)
{
    FilePaths binaries;
    QStringList pendingTargets{startTarget};
    QSet<QString> seenTargets;
    while (!pendingTargets.isEmpty()) {
        const QString title = pendingTargets.takeLast();
        if (!Utils::insert(seenTargets, title))
            continue;
        const CMakeBuildTarget target
            = Utils::findOrDefault(targets, Utils::equal(&CMakeBuildTarget::title, title));
        if (target.targetType == ExecutableType || target.targetType == DynamicLibraryType) {
            if (!target.executable.isEmpty())
                binaries << target.executable;
            continue;
        }
        if (target.targetType == ObjectLibraryType) {
            // An object library is not archived and so appears on no link line.
            // Its objects turn up among the sources of whoever uses it, all of
            // them, so the first one (resolved in "executable") finds them all.
            if (target.executable.isEmpty())
                continue;
            for (const CMakeBuildTarget &other : targets) {
                if (other.sourceFiles.contains(target.executable))
                    pendingTargets << other.title;
            }
            continue;
        }
        // Code from a static library ends up in whatever links it.
        const QString artifact = target.artifact.fileName();
        if (artifact.isEmpty())
            continue;
        for (const CMakeBuildTarget &other : targets) {
            if (other.linkedLibraryFileNames.contains(artifact))
                pendingTargets << other.title;
        }
    }
    return binaries;
}

static FilePaths linkingBinaries(const QList<CMakeBuildTarget> &targets,
                                 const FilePath &sourceFile)
{
    QList<const CMakeBuildTarget *> startTargets;
    for (const CMakeBuildTarget &target : targets) {
        if (target.sourceFiles.contains(sourceFile))
            startTargets << &target;
    }

    FilePaths binaries;
    for (const CMakeBuildTarget * const start : std::as_const(startTargets)) {
        FilePaths reached = binariesLinkingTarget(targets, start->title);
        if (reached.isEmpty()) {
            // Nothing links this archive, so it is the only binary carrying the
            // code, exactly as the generic fallback would have answered.
            if (start->targetType != StaticLibraryType || start->executable.isEmpty())
                continue;
            reached = {start->executable};
        }
        for (const FilePath &binary : std::as_const(reached)) {
            if (!binaries.contains(binary))
                binaries << binary;
        }
    }
    return binaries;
}

// A dependency keeps part of its library directories to itself. A shared library
// resolves its private dependencies, so they stay off the link line of whoever uses it
// - while the loader still has to find them at startup. A static library does pass them
// on, but an imported one arrives as an import library, which routinely lives beside
// the shared library instead of with it: "sdk/lib/x64/foo.lib" next to
// "sdk/bin/x64/foo.dll". Either way a directory is missing, so the walk descends into
// every dependency; what a binary already names itself the uniqueness of the result
// absorbs.
static FilePaths librarySearchPaths(const QList<CMakeBuildTarget> &targets,
                                    const QString &buildKey)
{
    QHash<QString, const CMakeBuildTarget *> targetByTitle;
    for (const CMakeBuildTarget &target : targets) {
        if (target.targetType != UtilityType)
            targetByTitle.insert(target.title, &target);
    }

    const CMakeBuildTarget *root = targetByTitle.value(buildKey);
    if (!root)
        return {};

    FilePaths paths = root->libraryDirectories;
    QSet<QString> seenTargets{buildKey};
    QStringList pendingTargets = root->dependencyTitles;
    while (!pendingTargets.isEmpty()) {
        const QString title = pendingTargets.takeLast();
        if (!Utils::insert(seenTargets, title))
            continue;
        const CMakeBuildTarget *target = targetByTitle.value(title);
        if (!target)
            continue;
        paths += target->libraryDirectories;
        pendingTargets += target->dependencyTitles;
    }
    return Utils::filteredUnique(paths);
}

static Link linkTo(const CMakeFileInfo &cmakeFile, const ArgumentAST *argument)
{
    Link link;
    link.targetFilePath = cmakeFile.path;
    link.target.line = argument->token.line;
    link.target.column = argument->token.column - 1;
    return link;
}

// The symbol a target's name makes: the place in the project that spells the
// name out, so that following the name arrives there and Find Usages takes the
// name for the project's own.
//
// The file-api gives the chain of commands that created the target, innermost
// first, and no column. Neither end of that chain names the target. The
// innermost frame belongs to whichever helper of Qt or CMake ran last, and the
// outermost one is the call the file made, which a macro shares with every
// target it creates. The innermost frame the project itself owns is the one
// that names it - and one such frame can still stand for several targets, as
// a command that creates a target plus its companions does.
static QHash<QString, Link> targetSymbols(const QList<CMakeBuildTarget> &targets,
                                          const QSet<CMakeFileInfo> &cmakeFiles)
{
    QSet<FilePath> ownFiles;
    for (const CMakeFileInfo &cmakeFile : cmakeFiles) {
        if (!cmakeFile.isExternal && !cmakeFile.isGenerated)
            ownFiles.insert(cmakeFile.path);
    }

    QHash<FilePath, QMultiHash<int, QString>> titlesByFileAndLine;
    for (const CMakeBuildTarget &target : targets) {
        if (target.targetType == UtilityType)
            continue;
        if (target.backtrace.isEmpty())
            continue;

        const FolderNode::LocationInfo *definition = &target.backtrace.first();
        for (const FolderNode::LocationInfo &frame : target.backtrace) {
            if (ownFiles.contains(frame.path)) {
                definition = &frame;
                break;
            }
        }
        titlesByFileAndLine[definition->path].insert(definition->line, target.title);
    }

    QHash<QString, Link> symbols;
    for (const CMakeFileInfo &cmakeFile : cmakeFiles) {
        const auto titles = titlesByFileAndLine.constFind(cmakeFile.path);
        if (titles == titlesByFileAndLine.constEnd() || !cmakeFile.document)
            continue;

        for (CommandAST *command : cmakeFile.document->commands()) {
            const QStringList lineTitles = titles->values(command->name.line);
            if (lineTitles.isEmpty())
                continue;
            ArgumentAST *argument = command->arguments().first();
            if (!argument)
                continue;

            const Link link = linkTo(cmakeFile, argument);
            for (const QString &title : lineTitles)
                symbols.insert(title, link);
        }
    }
    return symbols;
}

#ifdef WITH_TESTS
// Compares every file of the directory against its _expected.cmake sibling.
static void compareWithExpected(const FilePath &directory)
{
    static const QString suffix = "_expected.cmake";
    const FileFilter filter({"*" + suffix}, DirFilterFlag::Files);
    const FilePaths expectedDocuments = directory.dirEntries(filter);
    QVERIFY(!expectedDocuments.isEmpty());

    for (const FilePath &expected : expectedDocuments) {
        const FilePath actual = expected.parentDir().pathAppended(
            expected.fileName().chopped(suffix.size()) + ".cmake");
        QVERIFY(actual.exists());
        const auto actualContents = actual.fileContents();
        QVERIFY(actualContents);
        const auto expectedContents = expected.fileContents();
        const QByteArrayList actualLines = actualContents->split('\n');
        const QByteArrayList expectedLines = expectedContents->split('\n');
        if (actualLines.size() != expectedLines.size()) {
            qDebug().noquote().nospace() << "---\n" << *expectedContents << "EOF";
            qDebug().noquote().nospace() << "+++\n" << *actualContents << "EOF";
        }
        QCOMPARE(actualLines.size(), expectedLines.size());
        for (int i = 0; i < actualLines.size(); ++i) {
            const QByteArray actualLine = actualLines.at(i);
            const QByteArray expectedLine = expectedLines.at(i);
            if (actualLine != expectedLine) {
                qDebug() << "Unexpected content in line" << (i + 1) << "of file"
                         << actual.fileName();
            }
            QCOMPARE(actualLine, expectedLine);
        }
    }
}

class AddDependenciesTest final : public QObject
{
    Q_OBJECT

private slots:
    void test()
    {
        const auto projectDir = std::make_unique<CppEditor::Tests::TemporaryCopiedDir>(
            ":/cmakeprojectmanager/testcases/adddependencies");

        QVERIFY(insertDependencies(
            "HelloQt",
            projectDir->filePath().pathAppended("existing_qt5.cmake"),
            18,
            {"Qt.Concurrent"},
            "5"));
        QVERIFY(insertDependencies(
            "HelloQt",
            projectDir->filePath().pathAppended("existing_qt6.cmake"),
            8,
            {"Qt.Concurrent"},
            "6"));
        QVERIFY(insertDependencies(
            "HelloQt",
            projectDir->filePath().pathAppended("existing_qt5_and_qt6.cmake"),
            15,
            {"Qt.Concurrent"},
            "${QT_VERSION_MAJOR}"));
        QVERIFY(insertDependencies(
            "HelloCpp",
            projectDir->filePath().pathAppended("no_qt6.cmake"),
            8,
            {"Qt.Concurrent"},
            "6"));

        compareWithExpected(projectDir->filePath());
    }
};

QObject *createAddDependenciesTest()
{
    return new AddDependenciesTest;
}

class BinariesForSourceFileTest final : public QObject
{
    Q_OBJECT

private slots:
    void test()
    {
        CMakeBuildTarget app;
        app.title = "App";
        app.targetType = ExecutableType;
        app.executable = "/b/App.exe";
        app.sourceFiles = {"/s/main.cpp", "/s/shared.h"};
        app.linkedLibraryFileNames = {"Shared.lib", "Direct.lib"};

        CMakeBuildTarget shared;
        shared.title = "Shared";
        shared.targetType = DynamicLibraryType;
        shared.artifact = "Shared.dll";
        shared.executable = "/b/Shared.dll";
        shared.sourceFiles = {"/s/shared.cpp", "/b/Objects.dir/objects.cpp.o"};
        shared.linkedLibraryFileNames = {"Direct.lib"};

        CMakeBuildTarget direct;
        direct.title = "Direct";
        direct.targetType = StaticLibraryType;
        direct.artifact = "Direct.lib";
        direct.sourceFiles = {"/s/direct.cpp", "/b/Objects.dir/objects.cpp.o"};
        direct.linkedLibraryFileNames = {"Nested.lib"};

        CMakeBuildTarget nested;
        nested.title = "Nested";
        nested.targetType = StaticLibraryType;
        nested.artifact = "Nested.lib";
        nested.sourceFiles = {"/s/nested.cpp"};

        // An object library reaching a binary directly through "Shared", and
        // through the static library "Direct" as well.
        CMakeBuildTarget objects;
        objects.title = "Objects";
        objects.targetType = ObjectLibraryType;
        objects.artifact = "Objects.dir/objects.cpp.o";
        objects.executable = "/b/Objects.dir/objects.cpp.o";
        objects.sourceFiles = {"/s/objects.cpp"};

        CMakeBuildTarget orphan;
        orphan.title = "Orphan";
        orphan.targetType = StaticLibraryType;
        orphan.artifact = "Orphan.lib";
        orphan.executable = "/b/Orphan.lib";
        orphan.sourceFiles = {"/s/orphan.cpp", "/s/shared.h"};

        CMakeBuildTarget utility;
        utility.title = "Utility";
        utility.sourceFiles = {"/s/utility.cpp"};

        const QList<CMakeBuildTarget> targets{app, shared, direct, nested, objects, orphan,
                                              utility};

        QCOMPARE(linkingBinaries(targets, "/s/main.cpp"), FilePaths{"/b/App.exe"});
        QCOMPARE(linkingBinaries(targets, "/s/shared.cpp"), FilePaths{"/b/Shared.dll"});
        QCOMPARE(Utils::sorted(linkingBinaries(targets, "/s/direct.cpp")),
                 FilePaths({"/b/App.exe", "/b/Shared.dll"}));
        QCOMPARE(Utils::sorted(linkingBinaries(targets, "/s/nested.cpp")),
                 FilePaths({"/b/App.exe", "/b/Shared.dll"}));
        QCOMPARE(Utils::sorted(linkingBinaries(targets, "/s/objects.cpp")),
                 FilePaths({"/b/App.exe", "/b/Shared.dll"}));

        // A static library nothing links is where its code stops, so it
        // answers with its own archive rather than with nothing.
        QCOMPARE(linkingBinaries(targets, "/s/orphan.cpp"), FilePaths{"/b/Orphan.lib"});
        QCOMPARE(Utils::sorted(linkingBinaries(targets, "/s/shared.h")),
                 FilePaths({"/b/App.exe", "/b/Orphan.lib"}));

        QCOMPARE(linkingBinaries(targets, "/s/utility.cpp"), FilePaths());
        QCOMPARE(linkingBinaries(targets, "/s/unknown.cpp"), FilePaths());
    }
};

QObject *createBinariesForSourceFileTest()
{
    return new BinariesForSourceFileTest;
}

class LibrarySearchPathsTest final : public QObject
{
    Q_OBJECT

private slots:
    void test()
    {
        CMakeBuildTarget app;
        app.title = "App";
        app.targetType = ExecutableType;
        app.libraryDirectories = {"/b/middle"};
        app.dependencyTitles = {"Middle", "Generate", "Archive"};

        // Linked privately by Middle, so /b/inner is unknown to App.
        CMakeBuildTarget middle;
        middle.title = "Middle";
        middle.targetType = DynamicLibraryType;
        middle.libraryDirectories = {"/b/inner"};
        middle.dependencyTitles = {"Inner"};

        CMakeBuildTarget inner;
        inner.title = "Inner";
        inner.targetType = DynamicLibraryType;
        inner.libraryDirectories = {"/b/deep", "/b/middle"};
        inner.dependencyTitles = {"Deep"};

        CMakeBuildTarget deep;
        deep.title = "Deep";
        deep.targetType = DynamicLibraryType;
        // A cycle must not send the walk in circles.
        deep.dependencyTitles = {"Middle"};

        CMakeBuildTarget generate;
        generate.title = "Generate";
        generate.targetType = UtilityType;
        generate.libraryDirectories = {"/b/utility"};

        // A static library hands its dependencies to App, but as import libraries,
        // which say nothing about where the loader will look. Both /b/archive and
        // /b/hidden have to come out of the walk.
        CMakeBuildTarget archive;
        archive.title = "Archive";
        archive.targetType = StaticLibraryType;
        archive.libraryDirectories = {"/b/archive"};
        archive.dependencyTitles = {"Hidden"};

        CMakeBuildTarget hidden;
        hidden.title = "Hidden";
        hidden.targetType = DynamicLibraryType;
        hidden.libraryDirectories = {"/b/hidden"};

        CMakeBuildTarget other;
        other.title = "Other";
        other.targetType = ExecutableType;
        other.libraryDirectories = {"/b/other"};

        const QList<CMakeBuildTarget> targets{
            app, middle, inner, deep, generate, archive, hidden, other};

        QCOMPARE(librarySearchPaths(targets, "App"),
                 FilePaths({"/b/middle", "/b/archive", "/b/hidden", "/b/inner", "/b/deep"}));
        QCOMPARE(librarySearchPaths(targets, "Middle"),
                 FilePaths({"/b/inner", "/b/deep", "/b/middle"}));
        QCOMPARE(librarySearchPaths(targets, "Other"), FilePaths{"/b/other"});
        QCOMPARE(librarySearchPaths(targets, "Generate"), FilePaths());
        QCOMPARE(librarySearchPaths(targets, "Unknown"), FilePaths());
    }
};

QObject *createLibrarySearchPathsTest()
{
    return new LibrarySearchPathsTest;
}

class TargetSymbolsTest final : public QObject
{
    Q_OBJECT

    // A frame of what the file-api calls the backtrace of a target.
    static FolderNode::LocationInfo frame(const QString &command, const QString &file, int line)
    {
        return FolderNode::LocationInfo(command, FilePath::fromString(file), line);
    }

    static CMakeBuildTarget target(const QString &title, const Backtrace &backtrace)
    {
        CMakeBuildTarget result;
        result.title = title;
        result.targetType = StaticLibraryType;
        result.backtrace = backtrace;
        return result;
    }

    static CMakeFileInfo cmakeFile(const QString &file, const QString &source, bool isExternal)
    {
        CMakeFileInfo info;
        info.path = FilePath::fromString(file);
        info.isExternal = isExternal;
        info.document = Document::fromSource(source);
        return info;
    }

    // "line:column" of where the name of the target is written, the way a Link
    // carries it.
    static QString dumped(const QHash<QString, Link> &symbols, const QString &title)
    {
        const auto it = symbols.constFind(title);
        if (it == symbols.constEnd())
            return "<none>";
        return QString("%1:%2:%3")
            .arg(it->targetFilePath.fileName())
            .arg(it->target.line)
            .arg(it->target.column);
    }

private slots:
    void test()
    {
        const QString listsTxt = "macro(add_two a b)\n"  // 1
                                 "  add_library(${a})\n" // 2
                                 "  add_library(${b})\n" // 3
                                 "endmacro()\n"          // 4
                                 "include(inc.cmake)\n"  // 5
                                 "add_two(m1 m2)\n"      // 6
                                 "add_library(direct)\n" // 7
                                 "add_library(other)\n"  // 8
                                 "qt_add_qml_module(mod)\n"; // 9

        const QSet<CMakeFileInfo> cmakeFiles{
            cmakeFile("/src/CMakeLists.txt", listsTxt, false),
            cmakeFile("/src/inc.cmake", "add_library(included)\n", false),
            cmakeFile("/qt/Qt6QmlMacros.cmake", "add_library(${target})\n", true)};

        // Two targets the file itself adds, one per line.
        const QList<CMakeBuildTarget> targets{
            target("direct", {frame("add_library", "/src/CMakeLists.txt", 7)}),
            target("other", {frame("add_library", "/src/CMakeLists.txt", 8)}),

            // Two targets one macro of the file adds, which share the call
            // that reached the macro but not the line that names them.
            target("m1",
                   {frame("add_library", "/src/CMakeLists.txt", 2),
                    frame("add_two", "/src/CMakeLists.txt", 6)}),
            target("m2",
                   {frame("add_library", "/src/CMakeLists.txt", 3),
                    frame("add_two", "/src/CMakeLists.txt", 6)}),

            // A target an included file of the project adds.
            target("included",
                   {frame("add_library", "/src/inc.cmake", 1),
                    frame("include", "/src/CMakeLists.txt", 5)}),

            // Two targets a command of Qt adds off one call of the project.
            target("mod",
                   {frame("add_library", "/qt/Qt6QmlMacros.cmake", 1),
                    frame("qt_add_qml_module", "/src/CMakeLists.txt", 9)}),
            target("modplugin",
                   {frame("add_library", "/qt/Qt6QmlMacros.cmake", 1),
                    frame("qt_add_qml_module", "/src/CMakeLists.txt", 9)})};

        const QHash<QString, Link> symbols = targetSymbols(targets, cmakeFiles);

        QCOMPARE(dumped(symbols, "direct"), "CMakeLists.txt:7:12");
        QCOMPARE(dumped(symbols, "other"), "CMakeLists.txt:8:12");
        QCOMPARE(dumped(symbols, "m1"), "CMakeLists.txt:2:14");
        QCOMPARE(dumped(symbols, "m2"), "CMakeLists.txt:3:14");
        QCOMPARE(dumped(symbols, "included"), "inc.cmake:1:12");
        QCOMPARE(dumped(symbols, "mod"), "CMakeLists.txt:9:18");
        QCOMPARE(dumped(symbols, "modplugin"), "CMakeLists.txt:9:18");
        QCOMPARE(symbols.size(), targets.size());
    }

    void testUtilityTargetsAndEmptyBacktraces()
    {
        const QSet<CMakeFileInfo> cmakeFiles{
            cmakeFile("/src/CMakeLists.txt", "add_custom_target(run)\nadd_library(lib)\n", false)};

        CMakeBuildTarget utility
            = target("run", {frame("add_custom_target", "/src/CMakeLists.txt", 1)});
        utility.targetType = UtilityType;

        const QList<CMakeBuildTarget> targets{utility, target("lib", {})};

        QVERIFY(targetSymbols(targets, cmakeFiles).isEmpty());
    }
};

QObject *createTargetSymbolsTest()
{
    return new TargetSymbolsTest;
}

// Stands in for what Qt6QmlMacros.cmake declares: the keywords of the command
// come out of its definition, not out of a list kept here.
static const char qmlModuleDefinition[] = R"(function(qt6_add_qml_module target)
    set(args_option STATIC SHARED)
    set(args_single URI VERSION)
    set(args_multi SOURCES QML_FILES RESOURCES)
    cmake_parse_arguments(PARSE_ARGV 1 arg "${args_option}" "${args_single}" "${args_multi}")
endfunction()

if(NOT QT_NO_CREATE_VERSIONLESS_FUNCTIONS)
    function(qt_add_qml_module)
        qt6_add_qml_module(${ARGV})
    endfunction()
endif()
)";

class QmlModuleFilesTest final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        m_signatures.addDocument(
            Document::fromSource(QString::fromLatin1(qmlModuleDefinition)));
        QVERIFY(!m_signatures.signature("qt_add_qml_module").isEmpty());
    }

    void cleanup()
    {
        Core::EditorManager::closeAllEditors(/*askAboutModifiedEditors=*/false);
        m_projectDir.reset();
    }

    void testAddFiles()
    {
        copyTestcases("add");

        addTo("existing_keyword.cmake", {"Item.qml"});
        addTo("indented_keyword.cmake", {"Item.qml", "extra.cpp"});
        addTo("new_keyword.cmake", {"Item.qml", "logo.png"});
        addTo("keyword_variable.cmake", {"Item.qml"});

        compareWithExpected(m_directory);
    }

    void testRemoveFiles()
    {
        copyTestcases("remove");

        removeFrom("last.cmake", {"Main.qml"});
        removeFrom("one.cmake", {"Main.qml"});
        removeFrom("all.cmake", {"Main.qml", "Item.qml"});

        compareWithExpected(m_directory);
    }

    void testRenameFiles()
    {
        copyTestcases("rename");

        renameIn("indented.cmake", "Main.qml", "Main2.qml");
        renameIn("same_line.cmake", "Main.qml", "Main2.qml");

        compareWithExpected(m_directory);
    }

private:
    void copyTestcases(const QString &name)
    {
        m_projectDir = std::make_unique<CppEditor::Tests::TemporaryCopiedDir>(
            ":/cmakeprojectmanager/testcases/qmlmodulefiles/" + name);
        m_directory = m_projectDir->filePath().canonicalPath();

        // The files go in relative to the project, so they have to be there.
        for (const QString &fileName : QStringList{"Item.qml", "extra.cpp", "logo.png"})
            QVERIFY(m_directory.pathAppended(fileName).writeFileContents({}));
    }

    void addTo(const QString &cmakeFileName, const QStringList &fileNames)
    {
        const FilePath cmakeFile = m_directory.pathAppended(cmakeFileName);
        const DocumentPtr document = getUncachedCMakeFile(cmakeFile);
        QVERIFY(document);

        CommandAST *command = findCommand(document, [](CommandAST *candidate) {
            return candidate->isNamed("qt_add_qml_module");
        });
        QVERIFY(command);

        const FilePaths filePaths = Utils::transform(fileNames, [this](const QString &name) {
            return m_directory.pathAppended(name);
        });
        const Result<bool> inserted = insertFilesSilently(cmakeFile,
                                                          document,
                                                          m_signatures,
                                                          command,
                                                          "appQuickApp",
                                                          filePaths,
                                                          m_directory);
        if (!inserted)
            QFAIL(qPrintable(inserted.error()));
    }

    void removeFrom(const QString &cmakeFileName, const QStringList &fileNames)
    {
        const FilePath cmakeFile = m_directory.pathAppended(cmakeFileName);
        for (const QString &fileName : fileNames) {
            const DocumentPtr document = getUncachedCMakeFile(cmakeFile);
            QVERIFY(document);

            const Result<ProjectFileArgumentPosition> position
                = fileArgumentPosition(document, m_signatures, cmakeFile, 1, "appQuickApp",
                                       fileName);
            if (!position)
                QFAIL(qPrintable(position.error()));

            const Result<bool> removed = removeFileArgumentSilently(*position);
            if (!removed)
                QFAIL(qPrintable(removed.error()));
        }
    }

    void renameIn(const QString &cmakeFileName, const QString &fileName, const QString &newFileName)
    {
        const FilePath cmakeFile = m_directory.pathAppended(cmakeFileName);
        const DocumentPtr document = getUncachedCMakeFile(cmakeFile);
        QVERIFY(document);

        const Result<ProjectFileArgumentPosition> position
            = fileArgumentPosition(document, m_signatures, cmakeFile, 1, "appTestQuick", fileName);
        if (!position)
            QFAIL(qPrintable(position.error()));

        const Result<bool> renamed = renameFileArgumentSilently(*position, newFileName);
        if (!renamed)
            QFAIL(qPrintable(renamed.error()));
    }

    using ProjectFileArgumentPosition = CMakeBuildSystem::ProjectFileArgumentPosition;

    std::unique_ptr<CppEditor::Tests::TemporaryCopiedDir> m_projectDir;
    FilePath m_directory;
    SignatureTable m_signatures;
};

QObject *createQmlModuleFilesTest()
{
    return new QmlModuleFilesTest;
}

class SourceFilesTest final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        m_projectDir = std::make_unique<CppEditor::Tests::TemporaryCopiedDir>(
            ":/cmakeprojectmanager/testcases/sourcefiles/add");
        m_directory = m_projectDir->filePath().canonicalPath();

        // The file goes in relative to the project, so it has to be there.
        QVERIFY(m_directory.pathAppended("newwidget.cpp").writeFileContents({}));
    }

    void cleanupTestCase()
    {
        Core::EditorManager::closeAllEditors(/*askAboutModifiedEditors=*/false);
        m_projectDir.reset();
    }

    void testAddFiles_data()
    {
        QTest::addColumn<QString>("cmakeFileName");
        QTest::addColumn<int>("targetDefinitionLine");
        QTest::addColumn<QString>("targetName");

        QTest::newRow("wizard variable") << "wizard_variable.cmake" << 16 << "HelloQt";
        QTest::newRow("spelled out") << "spelled_out.cmake" << 5 << "HelloQt";
        QTest::newRow("two on a line") << "two_on_a_line.cmake" << 5 << "HelloQt";
        QTest::newRow("variable target") << "variable_target.cmake" << 7 << "HelloQt";
        QTest::newRow("target_sources variable")
            << "target_sources_variable.cmake" << 9 << "HelloQt";
        QTest::newRow("two variables") << "two_variables.cmake" << 13 << "HelloQt";

        // A call and a list that stand on one line keep standing on one.
        QTest::newRow("one line") << "one_line.cmake" << 7 << "HelloQt";
        QTest::newRow("one line call") << "one_line_call.cmake" << 5 << "HelloQt";

        // What the end of the set() spells out is not a file, so the files go
        // into the call instead.
        QTest::newRow("cache variable") << "cache_variable.cmake" << 7 << "HelloQt";
        QTest::newRow("parent scope") << "parent_scope.cmake" << 7 << "HelloQt";

        // The library is written first, so that the line the application is on
        // still holds once the set() before it has grown a file.
        QTest::newRow("reused variable, library") << "reused_variable.cmake" << 15 << "HelloLib";
        QTest::newRow("reused variable, application") << "reused_variable.cmake" << 9 << "HelloQt";
    }

    void testAddFiles()
    {
        QFETCH(QString, cmakeFileName);
        QFETCH(int, targetDefinitionLine);
        QFETCH(QString, targetName);

        const FilePath cmakeFile = m_directory.pathAppended(cmakeFileName);
        const DocumentPtr document = getUncachedCMakeFile(cmakeFile);
        QVERIFY(document);

        CommandAST *command = findCommand(document, startsOnLine(targetDefinitionLine));
        QVERIFY(command);

        const Result<bool> inserted
            = insertFilesSilently(cmakeFile,
                                  document,
                                  m_signatures,
                                  command,
                                  targetName,
                                  {m_directory.pathAppended("newwidget.cpp")},
                                  m_directory);
        if (!inserted)
            QFAIL(qPrintable(inserted.error()));
    }

    void testExpectedContents() { compareWithExpected(m_directory); }

private:
    std::unique_ptr<CppEditor::Tests::TemporaryCopiedDir> m_projectDir;
    FilePath m_directory;
    SignatureTable m_signatures;
};

QObject *createSourceFilesTest()
{
    return new SourceFilesTest;
}

class ResourceFilesTest final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        m_projectDir = std::make_unique<CppEditor::Tests::TemporaryCopiedDir>(
            ":/cmakeprojectmanager/testcases/resourcefiles");
        m_directory = m_projectDir->filePath().canonicalPath();

        // The file goes in relative to the project, so it has to be there.
        QVERIFY(m_directory.pathAppended("assets.qrc").writeFileContents({}));
    }

    void cleanupTestCase()
    {
        Core::EditorManager::closeAllEditors(/*askAboutModifiedEditors=*/false);
        m_projectDir.reset();
    }

    void testAddQrcFiles_data()
    {
        QTest::addColumn<QString>("cmakeFileName");
        QTest::addColumn<int>("targetDefinitionLine");

        QTest::newRow("plain") << "plain.cmake" << 5;

        // The keywords of qt_add_qml_module() take the files a resource is
        // assembled out of, so a .qrc goes in as a source of the target.
        QTest::newRow("qml module") << "qml_module.cmake" << 7;

        // The .qrc joins the target_sources() the target already has, in the
        // group of the scope it compiles: the files of another scope go to
        // whoever links the target, and those of a file set are not sources.
        QTest::newRow("target_sources in place") << "has_target_sources.cmake" << 5;
        QTest::newRow("target_sources with a file set") << "file_set.cmake" << 5;
        QTest::newRow("target_sources with public sources") << "public_sources.cmake" << 5;

        // A call whose values stand on one line keeps standing on one.
        QTest::newRow("sources on one line") << "one_line_sources.cmake" << 5;

        // An interface library takes INTERFACE sources only.
        QTest::newRow("interface library") << "interface_library.cmake" << 5;

        // AUTORCC is left alone where it is on already, and written where the
        // file turns it off or sets it behind the target, which no longer
        // reaches it.
        QTest::newRow("autorcc on") << "autorcc_on.cmake" << 7;
        QTest::newRow("autorcc off") << "autorcc_off.cmake" << 7;
        QTest::newRow("autorcc behind the target") << "autorcc_behind.cmake" << 5;

        // A variable whose value cannot be read counts as off, a property
        // whose value cannot be read as on: what is written lands below the
        // variable, which no longer reaches the target, and above the
        // property, which has the last word either way.
        QTest::newRow("autorcc unknown") << "autorcc_unknown.cmake" << 7;
        QTest::newRow("autorcc property unknown") << "autorcc_property_unknown.cmake" << 5;

        // A value that goes to the scope around the file, and one the file
        // takes away again, leave the target without it.
        QTest::newRow("autorcc in the parent scope") << "autorcc_parent_scope.cmake" << 7;
        QTest::newRow("autorcc unset") << "autorcc_unset.cmake" << 9;

        // A second .qrc finds the property on the target and adds no other,
        // whichever call put it there. A property of another target, and one
        // whose value is the name of this one, are not this target's.
        QTest::newRow("autorcc property") << "autorcc_property.cmake" << 5;
        QTest::newRow("autorcc property of another target") << "autorcc_other_target.cmake" << 5;
        QTest::newRow("autorcc set_property") << "autorcc_set_property.cmake" << 5;
    }

    void testAddQrcFiles()
    {
        QFETCH(QString, cmakeFileName);
        QFETCH(int, targetDefinitionLine);

        const FilePath cmakeFile = m_directory.pathAppended(cmakeFileName);
        const DocumentPtr document = getUncachedCMakeFile(cmakeFile);
        QVERIFY(document);

        const Result<bool> inserted = insertQrcFiles(document,
                                                     cmakeFile,
                                                     "HelloQt",
                                                     targetDefinitionLine,
                                                     {m_directory.pathAppended("assets.qrc")},
                                                     m_directory);
        if (!inserted)
            QFAIL(qPrintable(inserted.error()));
    }

    // What the .qrc is written with is what lands, whatever the user set the
    // CMake code style to: the layout of a project is the project's, and a
    // setting of the one who adds the file has no business rewriting it.
    void testCodeStyleDoesNotLayTheFileOut()
    {
        ICodeStylePreferences *codeStyle
            = codeStyleFactory(Constants::CMAKE_LANGUAGE_ID)->globalCodeStyle();
        QVERIFY(codeStyle);

        // The setting that moves the values of a keyword is the one the
        // indenter would lay the added line out with.
        CMakeCodeStyleSettings settings
            = codeStyle->currentValue().value<CMakeCodeStyleSettings>();
        QVERIFY(settings.indentKeywordValues);
        settings.indentKeywordValues = false;

        const FilePath cmakeFile = m_directory.pathAppended("code_style.cmake");
        const DocumentPtr document = getUncachedCMakeFile(cmakeFile);
        QVERIFY(document);

        // Nothing between here and where the style is put back may leave the
        // function, or the rest of the tests run with the style of this one.
        ICodeStylePreferences *delegate = codeStyle->currentDelegate();
        const QVariant value = codeStyle->value();
        codeStyle->setCurrentDelegate(nullptr);
        codeStyle->setValue(QVariant::fromValue(settings));

        const Result<bool> inserted = insertQrcFiles(document,
                                                     cmakeFile,
                                                     "HelloQt",
                                                     5,
                                                     {m_directory.pathAppended("assets.qrc")},
                                                     m_directory);

        codeStyle->setValue(value);
        codeStyle->setCurrentDelegate(delegate);

        if (!inserted)
            QFAIL(qPrintable(inserted.error()));
    }

    void testExpectedContents() { compareWithExpected(m_directory); }

private:
    std::unique_ptr<CppEditor::Tests::TemporaryCopiedDir> m_projectDir;
    FilePath m_directory;
};

QObject *createResourceFilesTest()
{
    return new ResourceFilesTest;
}

class TracepointFilesTest final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        m_projectDir = std::make_unique<CppEditor::Tests::TemporaryCopiedDir>(
            ":/cmakeprojectmanager/testcases/tracepointfiles");
        m_directory = m_projectDir->filePath().canonicalPath();

        // The provider goes in relative to the project, so it has to be there.
        QVERIFY(m_directory.pathAppended("hello.tracepoints").writeFileContents({}));
    }

    void cleanupTestCase()
    {
        Core::EditorManager::closeAllEditors(/*askAboutModifiedEditors=*/false);
        m_projectDir.reset();
    }

    void testAddTracepointFiles_data()
    {
        QTest::addColumn<QString>("cmakeFileName");
        QTest::addColumn<int>("targetDefinitionLine");

        QTest::newRow("plain") << "plain.cmake" << 5;

        // The include() is written once, however many providers the file names.
        QTest::newRow("include in place") << "has_include.cmake" << 7;

        // A provider the file already names is not named a second time.
        QTest::newRow("already added") << "already_added.cmake" << 7;
    }

    void testAddTracepointFiles()
    {
        QFETCH(QString, cmakeFileName);
        QFETCH(int, targetDefinitionLine);

        const FilePath cmakeFile = m_directory.pathAppended(cmakeFileName);
        const DocumentPtr document = getUncachedCMakeFile(cmakeFile);
        QVERIFY(document);

        const Result<bool> inserted
            = insertQtAddTracepoints(document,
                                     cmakeFile,
                                     "HelloQt",
                                     targetDefinitionLine,
                                     {m_directory.pathAppended("hello.tracepoints")});
        if (!inserted)
            QFAIL(qPrintable(inserted.error()));
    }

    // What the "Qt Tracing" wizard writes. Its pages name no target path of
    // their own, so the wizard has to, and the files it generates are named
    // after what the provider is called.
    void testWizardGeneratesFiles()
    {
        Core::IWizardFactory *factory
            = findOrDefault(Core::IWizardFactory::allWizardFactories(),
                            [](Core::IWizardFactory *f) { return f->id() == "R.QtTracing"; });
        QVERIFY(factory);

        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const FilePath path = FilePath::fromString(directory.path());
        const auto wizard = qobject_cast<JsonWizard *>(
            factory->runWizard(path, Utils::Id(), {}, /*showWizard=*/false));
        QVERIFY(wizard);
        const QScopeGuard cleanup([wizard] { delete wizard; });

        wizard->setValue("Path", path.toUrlishString());
        wizard->setValue("ProviderName", "myapp");

        // Without it the generators have nothing to resolve what they write
        // against, and generating the files fails.
        QCOMPARE(wizard->stringValue("TargetPath"), path.toUrlishString());

        const JsonWizard::GeneratorFiles files = wizard->generateFileList();
        const QStringList names = transform(files, [](const JsonWizard::GeneratorFile &file) {
            return file.file.filePath().fileName();
        });
        QCOMPARE(names, QStringList({"myapp.tracepoints", "myapp_tracing.h"}));

        // The header reaches the generated one and spells the macro that the
        // quick fix reads the source location out of.
        const QString header = files.at(1).file.contents();
        QVERIFY2(header.contains("#include \"myapp_tracepoints_p.h\""), qPrintable(header));
        QVERIFY2(header.contains("#define MYAPP_TRACE_LOCATION"), qPrintable(header));
        // The provider carries the calls that record what it declares, to be
        // copied from, so the macro has to be spelled out there as well.
        const QString provider = files.at(0).file.contents();
        QVERIFY2(provider.contains("render_entry("), qPrintable(provider));
        QVERIFY2(provider.contains(
                     "Q_TRACE_SCOPE(render, width, height, MYAPP_TRACE_LOCATION);"),
                 qPrintable(provider));
        QVERIFY2(provider.contains("Q_TRACE(message, \"frame started\", MYAPP_TRACE_LOCATION);"),
                 qPrintable(provider));
    }

    // What adding the files of the wizard to a target comes down to: the
    // header goes in as a source, the provider brings the call with it, and
    // both happen to the one file, one after the other.
    void testAddedNextToSources()
    {
        const FilePath cmakeFile = m_directory.pathAppended("with_sources.cmake");
        const FilePath header = m_directory.pathAppended("hello_tracing.h");
        QVERIFY(header.writeFileContents({}));

        DocumentPtr document = getUncachedCMakeFile(cmakeFile);
        QVERIFY(document);
        CommandAST *command = findCommand(document, startsOnLine(5));
        QVERIFY(command);
        const Result<bool> added = insertFilesSilently(cmakeFile, document, m_signatures, command,
                                                       "HelloQt", {header}, m_directory);
        if (!added)
            QFAIL(qPrintable(added.error()));

        document = getUncachedCMakeFile(cmakeFile);
        QVERIFY(document);
        const Result<bool> inserted
            = insertQtAddTracepoints(document, cmakeFile, "HelloQt", 5,
                                     {m_directory.pathAppended("hello.tracepoints")});
        if (!inserted)
            QFAIL(qPrintable(inserted.error()));
    }

    // A project reached through a symbolic link, which is what one under the
    // temporary directory of macOS is. Only the canonical path of the provider
    // is known, so the directory it is made relative to has to be canonical
    // too, or the call names a path that leads back out of the project.
    void testProviderOfAProjectBehindASymlink()
    {
        QTemporaryDir linkDirectory;
        FilePath project = m_projectDir->filePath();
        if (project.canonicalPath() == project) {
            QVERIFY(linkDirectory.isValid());
            project = FilePath::fromString(linkDirectory.path()).pathAppended("project");
            if (!m_directory.createSymLink(project))
                QSKIP("A symbolic link to a directory cannot be created here");
        }

        const FilePath cmakeFile = project.pathAppended("behind_a_symlink.cmake");
        QVERIFY(cmakeFile.writeFileContents("cmake_minimum_required(VERSION 3.16)\n"
                                            "\n"
                                            "project(HelloQt VERSION 0.1 LANGUAGES CXX)\n"
                                            "\n"
                                            "add_executable(HelloQt\n"
                                            "    main.cpp\n"
                                            ")\n"));

        const DocumentPtr document = getUncachedCMakeFile(cmakeFile);
        QVERIFY(document);
        const Result<bool> inserted
            = insertQtAddTracepoints(document,
                                     cmakeFile,
                                     "HelloQt",
                                     5,
                                     {project.pathAppended("hello.tracepoints")});
        if (!inserted)
            QFAIL(qPrintable(inserted.error()));

        const Result<QByteArray> contents = cmakeFile.fileContents();
        QVERIFY(contents);
        QVERIFY2(contents->contains("qt_add_tracepoints(HelloQt hello.tracepoints)"),
                 contents->constData());
    }

    void testExpectedContents() { compareWithExpected(m_directory); }

    void testModuleIsWritten()
    {
        const FilePath module = m_directory.pathAppended(QT_TRACING_MODULE);
        QVERIFY(module.exists());
        const DocumentPtr document = getUncachedCMakeFile(module);
        QVERIFY(document);
        QVERIFY(findCommand(document, namedWithFirstArgument("function", "qt_add_tracepoints")));
    }

    // A file that includes a module of its own, which may well be a copy
    // elsewhere in the project, is left with the one it names rather than
    // given a second one beside it that nothing reads.
    void testModuleIsNotWrittenWhereItIsIncluded()
    {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const FilePath project = FilePath::fromString(directory.path()).canonicalPath();
        const FilePath provider = project.pathAppended("hello.tracepoints");
        QVERIFY(provider.writeFileContents({}));

        const FilePath cmakeFile = project.pathAppended("includes_a_module.cmake");
        QVERIFY(cmakeFile.writeFileContents("cmake_minimum_required(VERSION 3.16)\n"
                                            "\n"
                                            "project(HelloQt VERSION 0.1 LANGUAGES CXX)\n"
                                            "\n"
                                            "include(cmake/QtTracing.cmake)\n"
                                            "\n"
                                            "add_executable(HelloQt\n"
                                            "    main.cpp\n"
                                            ")\n"));

        const DocumentPtr document = getUncachedCMakeFile(cmakeFile);
        QVERIFY(document);
        const Result<bool> inserted
            = insertQtAddTracepoints(document, cmakeFile, "HelloQt", 7, {provider});
        if (!inserted)
            QFAIL(qPrintable(inserted.error()));

        QVERIFY(!project.pathAppended(QT_TRACING_MODULE).exists());
        const Result<QByteArray> contents = cmakeFile.fileContents();
        QVERIFY(contents);
        QVERIFY2(contents->contains("qt_add_tracepoints(HelloQt hello.tracepoints)"),
                 contents->constData());
        QVERIFY2(!contents->contains("include(QtTracing.cmake)"), contents->constData());
    }

private:
    std::unique_ptr<CppEditor::Tests::TemporaryCopiedDir> m_projectDir;
    FilePath m_directory;
    SignatureTable m_signatures;
};

QObject *createTracepointFilesTest()
{
    return new TracepointFilesTest;
}
#endif

FilePaths CMakeBuildSystem::binariesForSourceFile(const FilePath &sourceFile) const
{
    return linkingBinaries(m_buildTargets, sourceFile);
}

FilePaths CMakeBuildSystem::filesGeneratedFrom(const FilePath &sourceFile) const
{
    FilePath project = projectDirectory();
    FilePath baseDirectory = sourceFile.parentDir();

    while (baseDirectory.isChildOf(project)) {
        const FilePath cmakeListsTxt = baseDirectory.pathAppended(Constants::CMAKE_LISTS_TXT);
        if (cmakeListsTxt.exists())
            break;
        baseDirectory = baseDirectory.parentDir();
    }

    const QString relativePath = baseDirectory.relativePathFromDir(project);
    FilePath generatedFilePath = buildConfiguration()->buildDirectory().resolvePath(relativePath);

    if (sourceFile.suffix() == "ui") {
        const QString generatedFileName = "ui_" + sourceFile.completeBaseName() + ".h";

        auto targetNode = this->project()->nodeForFilePath(sourceFile);
        while (targetNode && !dynamic_cast<const CMakeTargetNode *>(targetNode))
            targetNode = targetNode->parentFolderNode();

        FilePaths generatedFilePaths;
        if (targetNode) {
            const QString autogenSignature = targetNode->buildKey() + "_autogen/include";

            // If AUTOUIC reports the generated header file name, use that path
            generatedFilePaths = this->project()->files(
                [autogenSignature, generatedFileName](const Node *n) {
                if (!Project::GeneratedFiles(n))
                    return false;
                const FilePath &filePath = n->filePath();
                return filePath.endsWith(generatedFileName) && filePath.contains(autogenSignature);
            });
        }

        if (generatedFilePaths.empty())
            generatedFilePaths = {generatedFilePath.pathAppended(generatedFileName)};

        return generatedFilePaths;
    }
    if (sourceFile.suffix() == "scxml") {
        generatedFilePath = generatedFilePath.pathAppended(sourceFile.completeBaseName());
        return {generatedFilePath.stringAppended(".h"), generatedFilePath.stringAppended(".cpp")};
    }

    // TODO: Other types will be added when adapters for their compilers become available.
    return {};
}

QString CMakeBuildSystem::reparseParametersString(int reparseFlags)
{
    QString result;
    if (reparseFlags == REPARSE_DEFAULT) {
        result = "<NONE>";
    } else {
        if (reparseFlags & REPARSE_URGENT)
            result += " URGENT";
        if (reparseFlags & REPARSE_FORCE_CMAKE_RUN)
            result += " FORCE_CMAKE_RUN";
        if (reparseFlags & REPARSE_FORCE_INITIAL_CONFIGURATION)
            result += " FORCE_CONFIG";
    }
    return result.trimmed();
}

void CMakeBuildSystem::reparse(int reparseParameters)
{
    setParametersAndRequestParse(BuildDirParameters(this), reparseParameters);
}

void CMakeBuildSystem::setParametersAndRequestParse(const BuildDirParameters &parameters,
                                                    const int reparseParameters)
{
    project()->clearIssues();

    qCDebug(cmakeBuildSystemLog) << buildConfiguration()->displayName()
                                 << "setting parameters and requesting reparse"
                                 << reparseParametersString(reparseParameters);

    const CMakeTool *tool = CMakeToolManager::findByCommand(parameters.cmakeExecutable);
    if (!tool || !tool->isValid()) {
        TaskHub::addTask<BuildSystemTask>(
                    Task::Error, Tr::tr("The kit needs to define a CMake tool to parse this project."));
        return;
    }
    if (!tool->hasFileApi()) {
        TaskHub::addTask<BuildSystemTask>(
                    Task::Error, CMakeKitAspect::msgUnsupportedVersion(tool->version().fullVersion));
        return;
    }
    QTC_ASSERT(parameters.isValid(), return );

    m_parameters = parameters;
    ensureBuildDirectory(parameters);
    updateReparseParameters(reparseParameters);

    m_reader.setParameters(m_parameters);
    m_reader.setProjectHeaders(configureHeaderDependencyUpdater()
                                   ? m_headerDependencyUpdater.projectHeaderMap()
                                   : QHash<FilePath, FilePaths>());

    if (reparseParameters & REPARSE_URGENT) {
        qCDebug(cmakeBuildSystemLog) << "calling requestReparse";
        requestParse();
    } else {
        qCDebug(cmakeBuildSystemLog) << "calling requestDelayedReparse";
        requestDelayedParse();
    }
}

bool CMakeBuildSystem::mustApplyConfigurationChangesArguments(const BuildDirParameters &parameters) const
{
    if (parameters.configurationChangesArguments.isEmpty())
        return false;

    // The question needs someone to answer it, and this dialog blocks the thread it is raised
    // on until then - which stalls Qt Creator when it is driven from the outside instead. Apply
    // the changes, the answer the dialog offers by default, when asking is turned off.
    if (!cmakeSettingsForProject(project()).askBeforeApplyingConfigurationChanges())
        return true;

    QDialog question(Core::ICore::dialogParent());
    question.resize(600, 300);
    question.setWindowTitle(Tr::tr("Apply Configuration Changes?"));

    QDialogButtonBox buttonBox;
    buttonBox.setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Discard);
    buttonBox.setCenterButtons(true);
    QPushButton *apply = buttonBox.button(QDialogButtonBox::Ok);
    apply->setDefault(true);
    apply->setAutoDefault(true);
    apply->setText(Tr::tr("Apply"));
    QPushButton *discard = buttonBox.button(QDialogButtonBox::Discard);
    discard->setAutoDefault(false);

    QObject::connect(apply, &QPushButton::clicked, &question, &QDialog::accept);
    QObject::connect(discard, &QPushButton::clicked, &question, &QDialog::reject);

    using namespace Layouting;
    // clang-format off
    Column {
        Tr::tr("Run CMake with configuration changes?"),
        TextEdit {
            markdown("```\n" +
                    parameters.configurationChangesArguments.join("\n") +
                    "\n```"),
            readOnly(true),
        }, br,
        Row {
            &buttonBox,
        }
    }.attachTo(&question);
    // clang-format on

    return question.exec() == QDialog::Accepted;
}

void CMakeBuildSystem::runCMake()
{
    qCDebug(cmakeBuildSystemLog) << "Requesting parse due \"Run CMake\" command";
    reparse(REPARSE_FORCE_CMAKE_RUN | REPARSE_URGENT);
}

void CMakeBuildSystem::runCMakeAndScanProjectTree()
{
    qCDebug(cmakeBuildSystemLog) << "Requesting parse due to \"Rescan Project\" command";
    reparse(REPARSE_FORCE_CMAKE_RUN | REPARSE_URGENT);
}

void CMakeBuildSystem::runCMakeWithExtraArguments()
{
    qCDebug(cmakeBuildSystemLog) << "Requesting parse due to \"Rescan Project\" command";
    reparse(REPARSE_FORCE_CMAKE_RUN | REPARSE_FORCE_EXTRA_CONFIGURATION | REPARSE_URGENT);
}

void CMakeBuildSystem::runCMakeWithProfiling()
{
    qCDebug(cmakeBuildSystemLog) << "Requesting parse due \"CMake Profiler\" command";
    reparse(REPARSE_FORCE_CMAKE_RUN | REPARSE_URGENT | REPARSE_FORCE_EXTRA_CONFIGURATION
            | REPARSE_PROFILING);
}

void CMakeBuildSystem::stopCMakeRun()
{
    qCDebug(cmakeBuildSystemLog) << buildConfiguration()->displayName()
                                 << "stopping CMake's run";
    m_reader.stopCMakeRun();
}

void CMakeBuildSystem::buildCMakeTarget(const QString &buildTarget)
{
    QTC_ASSERT(!buildTarget.isEmpty(), return);
    if (ProjectExplorerPlugin::saveModifiedFiles())
        cmakeBuildConfiguration()->buildTarget(buildTarget);
}

void CMakeBuildSystem::reBuildCMakeTarget(const QString &cleanTarget, const QString &buildTarget)
{
    QTC_ASSERT(!cleanTarget.isEmpty() && !buildTarget.isEmpty(), return);
    if (ProjectExplorerPlugin::saveModifiedFiles())
        cmakeBuildConfiguration()->reBuildTarget(cleanTarget, buildTarget);
}

bool CMakeBuildSystem::persistCMakeState()
{
    BuildDirParameters parameters(this);
    QTC_ASSERT(parameters.isValid(), return false);

    const bool hadBuildDirectory = parameters.buildDirectory.exists();
    ensureBuildDirectory(parameters);

    int reparseFlags = REPARSE_DEFAULT;
    qCDebug(cmakeBuildSystemLog) << "Checking whether build system needs to be persisted:"
                                 << "buildDir:" << parameters.buildDirectory
                                 << "Has extraargs:" << !parameters.configurationChangesArguments.isEmpty();

    if (mustApplyConfigurationChangesArguments(parameters)) {
        reparseFlags = REPARSE_FORCE_EXTRA_CONFIGURATION;
        qCDebug(cmakeBuildSystemLog) << "   -> must run CMake with extra arguments.";
    }
    if (!hadBuildDirectory) {
        reparseFlags = REPARSE_FORCE_INITIAL_CONFIGURATION;
        qCDebug(cmakeBuildSystemLog) << "   -> must run CMake with initial arguments.";
    }

    if (reparseFlags == REPARSE_DEFAULT) {
        if (buildConfiguration()->isEnabled())
            return false;

        qCDebug(cmakeBuildSystemLog) << "Requesting parse the CMake project";
        setParametersAndRequestParse(parameters, REPARSE_URGENT | reparseFlags);
        return true;
    }

    qCDebug(cmakeBuildSystemLog) << "Requesting parse to persist CMake State";
    setParametersAndRequestParse(parameters,
                                 REPARSE_URGENT | REPARSE_FORCE_CMAKE_RUN | reparseFlags);
    return true;
}

void CMakeBuildSystem::clearCMakeCache()
{
    QTC_ASSERT(m_parameters.isValid(), return );
    QTC_ASSERT(!m_isHandlingError, return );

    stopParsingAndClearState();

    const FilePath pathsToDelete[] = {
        m_parameters.buildDirectory / Constants::CMAKE_CACHE_TXT,
        m_parameters.buildDirectory / Constants::CMAKE_CACHE_TXT_PREV,
        m_parameters.buildDirectory / "CMakeFiles",
        m_parameters.buildDirectory / ".cmake/api/v1/reply",
        m_parameters.buildDirectory / ".cmake/api/v1/reply.prev",
        m_parameters.buildDirectory / packageManagerDir(),
        m_parameters.buildDirectory / "conan-dependencies",
        m_parameters.buildDirectory / "vcpkg-dependencies"
    };

    for (const FilePath &path : pathsToDelete)
        path.removeRecursively();

    emit configurationCleared();
}

void CMakeBuildSystem::disableCMakeBuildMenuActions()
{
    emitParsingStarted();
    emitParsingFinished(false);
}

void CMakeBuildSystem::combineScanAndParse(bool restoredFromBackup)
{
    if (m_waitingForParse)
        return;

    if (m_combinedScanAndParseResult) {
        updateProjectData();
        m_currentGuard.markAsSuccess();

        if (restoredFromBackup)
            project()->addIssue(
                CMakeProject::IssueType::Warning,
                Tr::tr("<b>CMake configuration failed<b>"
                       "<p>The backup of the previous configuration has been restored.</p>"
                       "<p>Issues and \"Projects > Build\" settings "
                       "show more information about the failure.</p>"));

        m_reader.resetData();

        m_currentGuard = {};
        m_testNames.clear();

        emitBuildSystemUpdated();

        runCTest();
    } else {
        updateFallbackProjectData();

        project()->addIssue(CMakeProject::IssueType::Warning,
                            Tr::tr("<b>Failed to load project<b>"
                                   "<p>Issues and \"Projects > Build\" settings "
                                   "show more information about the failure.</p>"));
    }
}

void CMakeBuildSystem::checkAndReportError(QString &errorMessage)
{
    if (!errorMessage.isEmpty()) {
        setError(errorMessage);
        errorMessage.clear();
    }
}

static QSet<FilePath> projectFilesToWatch(const QSet<CMakeFileInfo> &cmakeFiles)
{
    QSet<FilePath> result;
    for (const CMakeFileInfo &info : cmakeFiles) {
        if (!info.isGenerated && !info.isCMake)
            result.insert(info.path);
    }
    return result;
}

void CMakeBuildSystem::updateProjectData()
{
    qCDebug(cmakeBuildSystemLog) << "Updating CMake project data";

    QTC_ASSERT(!m_taskTreeRunner.isRunning() && !m_reader.isParsing(), return );

    buildConfiguration()->project()->setExtraProjectFiles(projectFilesToWatch(m_cmakeFiles));

    CMakeConfig patchedConfig = configurationFromCMake();

    Project *p = project();
    // Show the Project view only for the active build configuration
    if (buildConfiguration()->isActive()) {
        auto newRoot = m_reader.rootProjectNode();
        if (newRoot) {
            setRootProjectNode(std::move(newRoot));

            if (QTC_GUARD(p->rootProjectNode())) {
                const QString nodeName = p->rootProjectNode()->displayName();
                p->setDisplayName(nodeName);

                // set config on target nodes
                const QSet<QString> buildKeys = Utils::transform<QSet>(m_buildTargets,
                                                                       &CMakeBuildTarget::title);
                p->rootProjectNode()->forEachProjectNode(
                    [patchedConfig, buildKeys](const ProjectNode *node) {
                        if (buildKeys.contains(node->buildKey())) {
                            auto targetNode = const_cast<CMakeTargetNode *>(
                                dynamic_cast<const CMakeTargetNode *>(node));
                            if (QTC_GUARD(targetNode))
                                targetNode->setConfig(patchedConfig);
                        }
                    });
            }
        }
    }

    {
        qDeleteAll(m_extraCompilers);
        m_extraCompilers = findExtraCompilers();
        qCDebug(cmakeBuildSystemLog) << "Extra compilers created.";
    }

    QtSupport::CppKitInfo kitInfo(kit());
    QTC_ASSERT(kitInfo.isValid(), return );

    struct QtMajorToPkgNames
    {
        QtMajorVersion major = QtMajorVersion::None;
        QStringList pkgNames;
    };

    auto qtVersionFromCMake = [this](const QList<QtMajorToPkgNames> &mapping) {
        for (const QtMajorToPkgNames &m : mapping) {
            for (const QString &pkgName : m.pkgNames) {
                auto qt = m_findPackagesFilesHash.value(pkgName);
                if (qt.hasValidTarget())
                    return m.major;
            }
        }
        return QtMajorVersion::None;
    };

    QtMajorVersion qtVersion = qtVersionFromCMake(
        {{QtMajorVersion::Qt6, {"Qt6", "Qt6Core"}},
         {QtMajorVersion::Qt5, {"Qt5", "Qt5Core"}},
         {QtMajorVersion::Qt4, {"Qt4", "Qt4Core"}}});

    QString errorMessage;
    RawProjectParts rpps = m_reader.createRawProjectParts(errorMessage);
    if (!errorMessage.isEmpty())
        setError(errorMessage);
    qCDebug(cmakeBuildSystemLog) << "Raw project parts created." << errorMessage;

    for (RawProjectPart &rpp : rpps) {
        rpp.setQtVersion(qtVersion);
        const FilePath includeFileBaseDir = buildConfiguration()->buildDirectory();
        QStringList cxxFlags = rpp.flagsForCxx.commandLineFlags;
        QStringList cFlags = rpp.flagsForC.commandLineFlags;
        addTargetFlagForIos(cFlags, cxxFlags, this, [this] {
            return m_configurationFromCMake.stringValueOf("CMAKE_OSX_DEPLOYMENT_TARGET");
        });
        if (kitInfo.cxxToolchain)
            rpp.setFlagsForCxx({kitInfo.cxxToolchain, cxxFlags, includeFileBaseDir});
        if (kitInfo.cToolchain)
            rpp.setFlagsForC({kitInfo.cToolchain, cFlags, includeFileBaseDir});
    }

    const ProjectUpdateInfo updateInfo{p, kitInfo, buildConfiguration()->environment(), rpps};
    m_cppCodeModelUpdater->update(updateInfo, m_extraCompilers);
    updateHeaderDependencies(updateInfo);

    {
        const bool mergedHeaderPathsAndQmlImportPaths = kit()->value(
                    QtSupport::Constants::KIT_HAS_MERGED_HEADER_PATHS_WITH_QML_IMPORT_PATHS, false).toBool();
        m_extraHeaderPaths.clear();
        m_moduleMappings.clear();
        for (const RawProjectPart &rpp : std::as_const(rpps)) {
            FilePath moduleMapFile = buildConfiguration()->buildDirectory()
                    .pathAppended("qml_module_mappings/" + rpp.buildSystemTarget);
            if (Result<QByteArray> content = moduleMapFile.fileContents()) {
                const QList<QByteArray> lines = content->split('\n');
                for (const QByteArray &line : lines) {
                    if (!line.isEmpty())
                        m_moduleMappings.append(line.simplified());
                }
            }

            if (mergedHeaderPathsAndQmlImportPaths) {
                for (const auto &headerPath : rpp.headerPaths) {
                    if (headerPath.type == HeaderPathType::User || headerPath.type == HeaderPathType::System)
                        m_extraHeaderPaths.append(headerPath.path.path());
                }
            }
        }
        updateQmlCodeModel();
    }
    updateInitialCMakeExpandableVars();

    emit buildConfiguration()->buildTypeChanged();

    qCDebug(cmakeBuildSystemLog) << "All CMake project data up to date.";
}

/*!
    Returns the prefix that \c cl prints in front of every \c /showIncludes
    line, as \a config has it.

    CMake detects the prefix per language, so a project without C++ in it only
    has the C variant. CMake keeps the value in the compiler information file
    of the build directory rather than in the cache, which is why
    cmake-helper/qtcreator-project.cmake puts it there.
*/
static QString showIncludesPrefix(const CMakeConfig &config)
{
    const QString cxxPrefix = config.stringValueOf("CMAKE_CXX_CL_SHOWINCLUDES_PREFIX");
    if (!cxxPrefix.isEmpty())
        return cxxPrefix;

    return config.stringValueOf("CMAKE_C_CL_SHOWINCLUDES_PREFIX");
}

bool CMakeBuildSystem::configureHeaderDependencyUpdater()
{
    if (!cmakeSettingsForProject(project()).scanHeaderDependencies()) {
        m_headerDependencyUpdater.cancel();
        return false;
    }

    const FilePath buildDirectory = buildConfiguration()->buildDirectory();
    m_headerDependencyUpdater.setStoreFile(
        buildDirectory / ProjectExplorer::Constants::PROJECT_QTC_DIR / "header-deps");
    m_headerDependencyUpdater.setProjectDirectories(m_parameters.sourceDirectory, buildDirectory);
    m_headerDependencyUpdater.setShowIncludesPrefix(showIncludesPrefix(m_configurationFromCMake));
    return true;
}

void CMakeBuildSystem::updateHeaderDependencies(const ProjectUpdateInfo &updateInfo)
{
    if (!configureHeaderDependencyUpdater())
        return;

    m_headerDependencyUpdater.update(
        updateInfo,
        buildConfiguration()->environment(),
        [this, updateInfo](const RawProjectParts &parts, bool storedResultsChanged) {
            ProjectUpdateInfo enriched = updateInfo;
            enriched.rawProjectParts = parts;
            m_cppCodeModelUpdater->update(enriched, m_extraCompilers);

            if (!storedResultsChanged)
                return;

            // The tree was generated before the scan ran, so generate it again from
            // the CMake reply, now that the headers are known. A source that stays
            // stale is scanned again on that run, but records the same result and
            // asks for nothing.
            reparse(REPARSE_DEFAULT);
        });
}

void CMakeBuildSystem::updateFileSystemNodes(std::unique_ptr<FolderNode> &&folderNode)
{
    auto newRoot = std::make_unique<CMakeProjectNode>(m_parameters.sourceDirectory);
    newRoot->setDisplayName(m_parameters.sourceDirectory.fileName());

    if (!m_reader.topCmakeFile().isEmpty()) {
        auto node = std::make_unique<FileNode>(m_reader.topCmakeFile(), FileType::Project);
        node->setIsGenerated(false);

        std::vector<std::unique_ptr<FileNode>> fileNodes;
        fileNodes.emplace_back(std::move(node));

        addCMakeLists(newRoot.get(), std::move(fileNodes));
    }

    if (QTC_GUARD(folderNode)) {
        folderNode->setPriority(Node::DefaultPriority - 6);
        folderNode->setDisplayName(Tr::tr("<File System>"));
        folderNode->setIcon(DirectoryIcon(ProjectExplorer::Constants::FILEOVERLAY_UNKNOWN));

        if (!folderNode->isEmpty()) {
            // make file system nodes less probable to be selected when syncing with the current document
            folderNode->forEachGenericNode([](Node *n) {
                n->setPriority(n->priority() + Node::DefaultProjectFilePriority + 1);
                n->setEnabled(false);
            });
            newRoot->addNode(std::move(folderNode));
        }
    }

    setRootProjectNode(std::move(newRoot));

    m_reader.resetData();

    m_currentGuard = {};
    emitBuildSystemUpdated();

    qCDebug(cmakeBuildSystemLog) << "All fallback CMake project data up to date.";
}

static bool mimeFileFilter(const MimeType &mimeType, const FilePath &fn)
{
    // Mime checks requires more resources, so keep it last in check list
    return TreeScanner::isWellKnownBinary(fn) || TreeScanner::isMimeTypeIgnored(mimeType);
}

static FileType fileTypeFactory(const MimeType &mimeType)
{
    auto type = TreeScanner::genericFileType(mimeType);
    if (type == FileType::Unknown) {
        if (mimeType.isValid()) {
            const QString mt = mimeType.name();
            if (mt == Utils::Constants::CMAKE_PROJECT_MIMETYPE
                || mt == Utils::Constants::CMAKE_MIMETYPE) {
                type = FileType::Project;
            }
        }
    }
    return type;
}

void CMakeBuildSystem::updateFallbackProjectData()
{
    qCDebug(cmakeBuildSystemLog) << "Updating fallback CMake project data";
    qCDebug(cmakeBuildSystemLog) << "Starting TreeScanner";
    QTC_CHECK(!m_taskTreeRunner.isRunning());

    using ResultType = TreeScanner::Result;
    const auto onSetup = [this](Async<ResultType> &task) {
        task.setConcurrentCallData(&TreeScanner::scanForFiles, projectDirectory(), mimeFileFilter,
                                   DirFilterFlag::AllEntries | DirFilterFlag::NoDotAndDotDot,
                                   &fileTypeFactory);
        QObject::connect(&task, &AsyncBase::started, this, [this, taskPtr = &task] {
            Core::ProgressManager::addTask(taskPtr->future(),
                                           Tr::tr("Scan \"%1\" project tree")
                                               .arg(project()->displayName()),
                                           "CMake.Scan.Tree");
        });
    };
    const auto onDone = [this](const Async<ResultType> &task) {
        if (!task.isResultAvailable())
            return;

        TreeScanner::Result result = task.takeResult();
        auto allFiles = std::make_unique<FolderNode>(projectDirectory());
        for (auto &node : result.firstLevelNodes)
            allFiles->addNode(std::move(node));

        updateFileSystemNodes(std::move(allFiles));
    };

    m_taskTreeRunner.start({AsyncTask<ResultType>(onSetup, onDone)});

    // A failed configuration could be the result of an compiler update
    // which then would cause CMake to fail. Make sure to offer an upgrade path
    // to the new Kit compiler values.
    updateInitialCMakeExpandableVars();
}

void CMakeBuildSystem::updateExtraData()
{
    // Data formerly served on demand from CMakeTargetNode::data(). The config
    // values are global, so they are the same for every target node.
    const CMakeConfig cfg = configurationFromCMake();
    auto value = [&cfg](const QByteArray &key) -> QVariant {
        for (const CMakeConfigItem &item : cfg) {
            if (item.key == key)
                return item.value;
        }
        return {};
    };
    const QString generator = cfg.stringValueOf("CMAKE_GENERATOR");
    const QVariant androidAbi = value(Android::Constants::ANDROID_ABI);
    const QVariant androidAbis = value(Android::Constants::ANDROID_ABIS);
    const QVariant androidDeploySettings
        = value(Android::Constants::ANDROID_DEPLOYMENT_SETTINGS_FILE);
    const QVariant androidExtraLibs = value(Android::Constants::ANDROID_EXTRA_LIBS);
    const QVariant androidPackageSourceDir = value(Android::Constants::ANDROID_PACKAGE_SOURCE_DIR);

    // Read by the androiddeployqt settings generation for projects that set
    // these as cache variables.
    const QVariant qmlImportPath = value("QML_IMPORT_PATH");
    const QVariant qmlRootPath = value("QML_ROOT_PATH");

    // Formerly the TARGETS_BUILD_PATH config value, patched onto every node.
    QStringList androidTargets;
    // Formerly the ANDROID_SO_LIBS_PATHS config value, patched onto every node.
    QSet<QString> androidSoLibDirs;
    for (const CMakeBuildTarget &ct : std::as_const(m_buildTargets)) {
        if (ct.targetType == DynamicLibraryType) {
            androidTargets.push_back(ct.executable.toUserOutput());
            androidSoLibDirs.insert(ct.executable.parentDir().path());
        }
    }
    androidTargets.sort();
    const QStringList androidSoLibPaths = Utils::toList(androidSoLibDirs);

    for (const CMakeBuildTarget &ct : std::as_const(m_buildTargets)) {
        setExtraData(ct.title, Android::Constants::AndroidAbi, androidAbi);
        setExtraData(ct.title, Android::Constants::AndroidAbis, androidAbis);
        setExtraData(ct.title, Android::Constants::AndroidDeploySettingsFile, androidDeploySettings);
        setExtraData(ct.title, Android::Constants::AndroidTargets, androidTargets);
        setExtraData(ct.title, Android::Constants::AndroidSoLibPath, androidSoLibPaths);
        setExtraData(ct.title, Android::Constants::AndroidExtraLibs, androidExtraLibs);
        setExtraData(ct.title, Android::Constants::AndroidPackageSourceDir, androidPackageSourceDir);
        setExtraData(ct.title, "QML_IMPORT_PATH", qmlImportPath);
        setExtraData(ct.title, "QML_ROOT_PATH", qmlRootPath);

        if (ct.artifact.isEmpty())
            continue;
        // The iOS plugin wants the app bundle name without ".app".
        setExtraData(ct.title, Ios::Constants::IosTarget, ct.artifact.fileName());
        // dir/target.app/target -> dir, relative to the root build directory.
        setExtraData(ct.title, Ios::Constants::IosBuildDir,
                     ct.artifact.parentDir().parentDir().path());
        setExtraData(ct.title, Ios::Constants::IosCmakeGenerator, generator);
    }
}

void CMakeBuildSystem::updateCMakeConfiguration()
{
    CMakeConfig cmakeConfig = m_reader.takeParsedConfiguration();
    for (auto &ci : cmakeConfig)
        ci.inCMakeCache = true;
    const CMakeConfig changes = configurationChanges();
    for (const auto &ci : changes) {
        if (ci.isInitial)
            continue;
        const bool haveConfigItem = Utils::contains(cmakeConfig, [ci](const CMakeConfigItem& i) {
            return i.key == ci.key;
        });
        if (!haveConfigItem)
            cmakeConfig.insert(ci);
    }

    const bool hasAndroidTargetBuildDirSupport
        = CMakeConfigItem::toBool(
              cmakeConfig.stringValueOf("QT_INTERNAL_ANDROID_TARGET_BUILD_DIR_SUPPORT"))
              .value_or(false);

    const bool useAndroidTargetBuildDir
        = CMakeConfigItem::toBool(cmakeConfig.stringValueOf("QT_USE_TARGET_ANDROID_BUILD_DIR"))
              .value_or(false);

    buildConfiguration()->setExtraData(
        Android::Constants::AndroidBuildTargetDirSupport,
        QVariant::fromValue(hasAndroidTargetBuildDirSupport));
    buildConfiguration()->setExtraData(
        Android::Constants::UseAndroidBuildTargetDir, QVariant::fromValue(useAndroidTargetBuildDir));

    QVariantList packageTargets;
    for (const CMakeBuildTarget &buildTarget : buildTargets()) {
        bool isBuiltinPackage = false;
        bool isInstallablePackage = false;
        for (const ProjectExplorer::FolderNode::LocationInfo &bs : buildTarget.backtrace) {
            if (bs.displayName == "qt6_am_create_builtin_package")
                isBuiltinPackage = true;
            else if (bs.displayName == "qt6_am_create_installable_package")
                isInstallablePackage = true;
        }

        if (!isBuiltinPackage && !isInstallablePackage)
            continue;

        QVariantMap packageTarget;
        for (const FilePath &sourceFile : buildTarget.sourceFiles) {
            if (sourceFile.fileName() == "info.yaml") {
                packageTarget.insert("manifestFilePath", QVariant::fromValue(sourceFile.absoluteFilePath()));
                packageTarget.insert("cmakeTarget", buildTarget.title);
                packageTarget.insert("isBuiltinPackage", isBuiltinPackage);
                for (const FilePath &osf : buildTarget.sourceFiles) {
                    if (osf.fileName().endsWith(".ampkg.rule")) {
                        packageTarget.insert("packageFilePath", QVariant::fromValue(osf.absoluteFilePath().chopped(5)));
                    }
                }
            }
        }
        packageTargets.append(packageTarget);
    }
    buildConfiguration()->setExtraData(AppManager::Constants::APPMAN_PACKAGE_TARGETS, packageTargets);

    setConfigurationFromCMake(cmakeConfig);
}

void CMakeBuildSystem::handleParsingSucceeded(bool restoredFromBackup)
{
    if (restoredFromBackup)
        setError(Tr::tr("CMake failed, using data from previous successful run."));
    else
        clearError();

    QString errorMessage;
    {
        m_buildTargets = Utils::transform(CMakeBuildStep::specialTargets(m_reader.usesAllCapsTargets()), [this](const QString &t) {
            CMakeBuildTarget result;
            result.title = t;
            result.workingDirectory = m_parameters.buildDirectory;
            result.sourceDirectory = m_parameters.sourceDirectory;
            return result;
        });
        m_buildTargets += m_reader.takeBuildTargets(errorMessage);
        m_cmakeFiles = m_reader.takeCMakeFileInfos(errorMessage);
        setupCommandSignatures();
        setupCMakeSymbolsHash();

        checkAndReportError(errorMessage);
    }

    updateCMakeConfiguration();

    m_ctestPath = m_parameters.cmakeExecutable.withNewPath(m_reader.ctestPath());

    setApplicationTargets(appTargets());
    updateExtraData();

    m_deploymentFromInstallRules = m_reader.takeDeployment();
    updateDeploymentData();

    QTC_ASSERT(m_waitingForParse, return );
    m_waitingForParse = false;

    combineScanAndParse(restoredFromBackup);
}

void CMakeBuildSystem::handleParsingFailed(const QString &msg)
{
    setError(msg);

    updateCMakeConfiguration();

    m_ctestPath.clear();

    QTC_CHECK(m_waitingForParse);
    m_waitingForParse = false;
    m_combinedScanAndParseResult = false;

    combineScanAndParse(false);
}

void CMakeBuildSystem::wireUpConnections()
{
    // At this point the entire project will be fully configured, so let's connect everything and
    // trigger an initial parser run

    // Became active/inactive:
    connect(target(), &Target::activeBuildConfigurationChanged, this, [this] {
        if (!buildConfiguration()->isActive())
            return;

        // Build configuration has changed:
        qCDebug(cmakeBuildSystemLog) << "Requesting parse due to active BC changed";
        reparse(CMakeBuildSystem::REPARSE_DEFAULT);
    });
    connect(project(), &Project::activeTargetChanged, this, [this] {
        if (!buildConfiguration()->isActive())
            return;

        // Build configuration has changed:
        qCDebug(cmakeBuildSystemLog) << "Requesting parse due to active target changed";
        reparse(CMakeBuildSystem::REPARSE_DEFAULT);
    });

    connect(ProjectManager::instance(), &ProjectManager::currentBuildConfigurationChanged,
            this, [this](BuildConfiguration *bc){
                if (!bc || !bc->isActive())
                    return;
                if (hasInstallDeployPreset(project())) {
                    DeployConfiguration *dc = target()->activeDeployConfiguration();
                    QTC_ASSERT(dc, return);
                    BuildStepList *stepsList = dc->stepList();
                    QTC_ASSERT(stepsList, return);
                    if (stepsList->contains(Constants::CMAKE_INSTALL_STEP_ID)) {
                        qCDebug(cmakeBuildSystemLog) << "cmake --install deploy step already added";
                        return;
                    }
                    const auto factories = BuildStepFactory::allBuildStepFactories();
                    auto factoryIt = std::find_if(factories.begin(), factories.end(), [](const BuildStepFactory *factory){
                        return factory->stepId() == Constants::CMAKE_INSTALL_STEP_ID;
                    });
                    QTC_ASSERT(factoryIt != factories.end(), return);
                    BuildStep *newStep = (*factoryIt)->create(stepsList);
                    QTC_ASSERT(newStep, return);
                    stepsList->appendStep(newStep);
                    qCDebug(cmakeBuildSystemLog) << "Added cmake --install deploy step";
                }
    });

    connect(buildConfiguration(), &BuildConfiguration::environmentChanged, this, [this] {
        if (!buildConfiguration()->isActive())
            return;

        // The environment on our BC has changed, force CMake run to catch up with possible changes
        qCDebug(cmakeBuildSystemLog) << "Requesting parse due to environment change";
        reparse(CMakeBuildSystem::REPARSE_FORCE_CMAKE_RUN);
    });
    connect(buildConfiguration(), &BuildConfiguration::buildDirectoryChanged, this, [this] {
        if (!buildConfiguration()->isActive())
            return;

        // The build directory of our BC has changed:
        // Does the directory contain a CMakeCache ? Existing build, just parse
        // No CMakeCache? Run with initial arguments!
        qCDebug(cmakeBuildSystemLog) << "Requesting parse due to build directory change";
        const BuildDirParameters parameters(this);
        const FilePath cmakeCacheTxt = parameters.buildDirectory.pathAppended(
            Constants::CMAKE_CACHE_TXT);
        const bool hasCMakeCache = cmakeCacheTxt.exists();
        const int options = hasCMakeCache
                ? REPARSE_DEFAULT
                : REPARSE_FORCE_INITIAL_CONFIGURATION | REPARSE_FORCE_CMAKE_RUN;
        if (hasCMakeCache) {
            QString errorMessage;
            const CMakeConfig config = CMakeConfig::fromFile(cmakeCacheTxt, &errorMessage);
            if (!config.isEmpty() && errorMessage.isEmpty()) {
                QString cmakeBuildTypeName = config.stringValueOf("CMAKE_BUILD_TYPE");
                cmakeBuildConfiguration()->setCMakeBuildType(cmakeBuildTypeName, true);
            }
        }
        reparse(options);
    });

    connect(project(), &Project::projectFileIsDirty, this, [this] {
        const bool isBuilding = BuildManager::isBuilding(project());
        if (buildConfiguration()->isActive() && !isParsing() && !isBuilding) {
            if (cmakeSettingsForProject(project()).autorunCMake()) {
                qCDebug(cmakeBuildSystemLog) << "Requesting parse due to dirty project file";
                reparse(CMakeBuildSystem::REPARSE_FORCE_CMAKE_RUN);
            }
        }
    });
}

void CMakeBuildSystem::setupCommandSignatures()
{
    m_commandSignatures = {};
    for (const CMakeFileInfo &cmakeFile : std::as_const(m_cmakeFiles))
        m_commandSignatures.addDocument(cmakeFile.document);
    ++m_commandSignaturesGeneration;
}

void CMakeBuildSystem::setupCMakeSymbolsHash()
{
    m_cmakeSymbolsHash.clear();

    m_projectKeywords.functions.clear();
    m_projectKeywords.variables.clear();

    auto handleFunctionMacroOption = [&](const CMakeFileInfo &cmakeFile, CommandAST *command) {
        if (!command->isNamed("function") && !command->isNamed("macro")
            && !command->isNamed("option"))
            return;

        ArgumentAST *argument = command->arguments().first();
        if (!argument)
            return;

        const QString name = argument->value();
        m_cmakeSymbolsHash.insert(name, linkTo(cmakeFile, argument));

        if (command->isNamed("option")) {
            m_projectKeywords.variables[name] = FilePath();
        } else {
            // What the project says about the function stands in the file
            // that defines it.
            m_projectKeywords.functions[name] = cmakeFile.path;
        }
    };

    m_projectImportedTargets.clear();
    auto handleImportedTargets = [&](const CMakeFileInfo &cmakeFile, CommandAST *command) {
        if (!command->isNamed("add_library"))
            return;

        ArgumentAST *argument = command->arguments().first();
        if (!argument)
            return;
        const QString targetName = argument->value();

        const bool haveImported = Utils::contains(command->arguments(),
                                                  [](ArgumentAST *argument) {
                                                      return argument->value() == "IMPORTED";
                                                  });
        if (haveImported && !targetName.contains("${")) {
            m_projectImportedTargets << targetName;

            // Allow navigation to the imported target
            m_cmakeSymbolsHash.insert(targetName, linkTo(cmakeFile, argument));
        }
    };

    // Gather the exported variables for the Find<Package> CMake packages
    m_projectFindPackageVariables.clear();

    const QString fphsCommandName = "find_package_handle_standard_args";
    CMakeKeywords keywords = CMakeKitAspect::cmakeKeywords(kit());
    const QSet<QString> fphsCommandArgs = Utils::toSet(
        keywords.functionArgs.value(fphsCommandName));

    auto handleFindPackageVariables = [&](const CMakeFileInfo &cmakeFile, CommandAST *command) {
        if (!command->isNamed(fphsCommandName))
            return;

        const ListView<ArgumentAST *> arguments = command->arguments();
        // The first argument names the package, the rest can name variables.
        for (int i = 1, size = arguments.size(); i < size; ++i) {
            ArgumentAST *argument = arguments.at(i);
            const QString value = argument->value();
            if (fphsCommandArgs.contains(value))
                continue;
            if (value.contains("${") || (value.startsWith('"') && value.endsWith('"'))
                || (value.startsWith("'") && value.endsWith("'")))
                continue;

            m_projectFindPackageVariables << value;
            m_cmakeSymbolsHash.insert(value, linkTo(cmakeFile, argument));
        }
    };

    // Prepare a hash with all .cmake files
    m_dotCMakeFilesHash.clear();
    auto handleDotCMakeFiles = [&](const CMakeFileInfo &cmakeFile) {
        if (cmakeFile.path.suffix() == "cmake") {
            Utils::Link link;
            link.targetFilePath = cmakeFile.path;
            link.target.line = 1;
            link.target.column = 0;
            m_dotCMakeFilesHash.insert(cmakeFile.path.completeBaseName(), link);
        }
    };

    // Gather all Find<Package>.cmake and <Package>Config.cmake / <Package>-config.cmake files
    m_findPackagesFilesHash.clear();
    auto handleFindPackageCMakeFiles = [&](const CMakeFileInfo &cmakeFile) {
        const QString fileName = cmakeFile.path.fileName();

        const QString findPackageName = [fileName]() -> QString {
            auto findIdx = fileName.indexOf("Find");
            auto endsWithCMakeIdx = fileName.lastIndexOf(".cmake");
            if (findIdx == 0 && endsWithCMakeIdx > 0)
                return fileName.mid(4, endsWithCMakeIdx - 4);
            return QString();
        }();

        const QString configPackageName = [fileName]() -> QString {
            auto configCMakeIdx = fileName.lastIndexOf("Config.cmake");
            if (configCMakeIdx > 0)
                return fileName.left(configCMakeIdx);
            auto dashConfigCMakeIdx = fileName.lastIndexOf("-config.cmake");
            if (dashConfigCMakeIdx > 0)
                return fileName.left(dashConfigCMakeIdx);
            return QString();
        }();

        if (!findPackageName.isEmpty() || !configPackageName.isEmpty()) {
            Utils::Link link;
            link.targetFilePath = cmakeFile.path;
            link.target.line = 1;
            link.target.column = 0;
            m_findPackagesFilesHash.insert(!findPackageName.isEmpty() ? findPackageName
                                                                      : configPackageName,
                                           link);
        }
    };

    for (const auto &cmakeFile : std::as_const(m_cmakeFiles)) {
        if (cmakeFile.document) {
            for (CommandAST *command : cmakeFile.document->commands()) {
                handleFunctionMacroOption(cmakeFile, command);
                handleImportedTargets(cmakeFile, command);
                handleFindPackageVariables(cmakeFile, command);
            }
        }
        handleDotCMakeFiles(cmakeFile);
        handleFindPackageCMakeFiles(cmakeFile);
    }

    m_cmakeSymbolsHash.insert(targetSymbols(buildTargets(), m_cmakeFiles));

    m_projectFindPackageVariables.removeDuplicates();
}

void CMakeBuildSystem::ensureBuildDirectory(const BuildDirParameters &parameters)
{
    const FilePath bdir = parameters.buildDirectory;

    if (!buildConfiguration()->createBuildDirectory()) {
        handleParsingFailed(Tr::tr("Failed to create build directory \"%1\".").arg(bdir.toUserOutput()));
        return;
    }

    if (parameters.cmakeExecutable.isEmpty()) {
        handleParsingFailed(Tr::tr("No CMake tool set up in kit."));
        return;
    }

    if (!parameters.cmakeExecutable.isLocal()) {
        if (!parameters.cmakeExecutable.ensureReachable(bdir)) {
            // Make sure that the build directory is available on the device.
            handleParsingFailed(
                Tr::tr("The remote CMake executable cannot write to the local build directory."));
        }
    }
}

void CMakeBuildSystem::stopParsingAndClearState()
{
    qCDebug(cmakeBuildSystemLog) << buildConfiguration()->displayName()
                                 << "stopping parsing run!";
    m_reader.stop();
    m_reader.resetData();
}

void CMakeBuildSystem::becameDirty()
{
    qCDebug(cmakeBuildSystemLog) << "CMakeBuildSystem: becameDirty was triggered.";
    if (isParsing())
        return;

    reparse(REPARSE_DEFAULT);
}

void CMakeBuildSystem::updateReparseParameters(const int parameters)
{
    m_reparseParameters |= parameters;
}

int CMakeBuildSystem::takeReparseParameters()
{
    int result = m_reparseParameters;
    m_reparseParameters = REPARSE_DEFAULT;
    return result;
}

void CMakeBuildSystem::runCTest()
{
    if (!m_error.isEmpty() || m_ctestPath.isEmpty()) {
        qCDebug(cmakeBuildSystemLog) << "Cancel ctest run after failed cmake run";
        emit testInformationUpdated();
        return;
    }
    qCDebug(cmakeBuildSystemLog) << "Requesting ctest run after cmake run";

    const BuildDirParameters parameters(this);
    QTC_ASSERT(parameters.isValid(), return);

    ensureBuildDirectory(parameters);
    m_ctestProcess.reset(new Process);
    m_ctestProcess->setEnvironment(buildConfiguration()->environment());
    m_ctestProcess->setWorkingDirectory(parameters.buildDirectory);
    m_ctestProcess->setCommand({m_ctestPath, { "-N", "--show-only=json-v1"}});
    connect(m_ctestProcess.get(), &Process::done, this, [this] {
        if (m_ctestProcess->result() == ProcessResult::FinishedWithSuccess) {
            const QJsonDocument json = QJsonDocument::fromJson(m_ctestProcess->rawStdOut());
            if (!json.isEmpty() && json.isObject()) {
                const QJsonObject jsonObj = json.object();
                const QJsonObject btGraph = jsonObj.value("backtraceGraph").toObject();
                const QJsonArray cmakelists = btGraph.value("files").toArray();
                const QJsonArray nodes = btGraph.value("nodes").toArray();
                const QJsonArray tests = jsonObj.value("tests").toArray();
                int counter = 0;
                for (const auto &testVal : tests) {
                    ++counter;
                    const QJsonObject test = testVal.toObject();
                    QTC_ASSERT(!test.isEmpty(), continue);
                    int file = -1;
                    int line = -1;
                    const int bt = test.value("backtrace").toInt(-1);
                    // we may have no real backtrace due to different registering
                    if (bt != -1) {
                        QSet<int> seen;
                        std::function<QJsonObject(int)> findAncestor = [&](int index){
                            QJsonObject node = nodes.at(index).toObject();
                            const int parent = node.value("parent").toInt(-1);
                            if (parent < 0 || !Utils::insert(seen, parent))
                                return node;
                            return findAncestor(parent);
                        };
                        const QJsonObject btRef = findAncestor(bt);
                        file = btRef.value("file").toInt(-1);
                        line = btRef.value("line").toInt(-1);
                    }
                    // we may have no CMakeLists.txt file reference due to different registering
                    const FilePath cmakeFile = file != -1
                            ? FilePath::fromString(cmakelists.at(file).toString()) : FilePath();
                    m_testNames.append({ test.value("name").toString(), counter, cmakeFile, line });
                }
            }
        }
        emit testInformationUpdated();
    });
    m_ctestProcess->start();
}

CMakeBuildConfiguration *CMakeBuildSystem::cmakeBuildConfiguration() const
{
    return static_cast<CMakeBuildConfiguration *>(BuildSystem::buildConfiguration());
}

static bool isTestTarget(const QString &targetName, const FilePath &buildDir)
{
    // The CTest file is always named CTestTestfile.cmake and lives in the build dir
    const FilePath testFile = buildDir.pathAppended("CTestTestfile.cmake");
    if (!testFile.exists())
        return false;

    // Parse the file
    const DocumentPtr document = getUncachedCMakeFile(testFile);
    if (!document)
        return false;

    // Find an add_test() call whose first argument equals the target name
    return findCommand(document, namedWithFirstArgument("add_test", targetName)) != nullptr;
}

// Run configurations are identified by their build key, which stays the CMake target name even
// when several run settings target it. They are told apart by their display name instead, so it
// has to be both stable and unique.
static QString runSettingsDisplayName(const PresetsDetails::RunSettings &rs,
                                      const QString &targetName,
                                      int index)
{
    if (rs.displayName && !rs.displayName->isEmpty())
        return *rs.displayName;
    return index == 0 ? targetName : targetName + QString::number(index + 1);
}

static BuildTargetInfo createBuildTargetInfo(
    const CMakeBuildSystem *bs, const CMakeBuildTarget &ct, const FilePath &projectFilePath)
{
    BuildTargetInfo bti;
    bti.displayName = ct.title;
    bti.buildKey = ct.title;
    bti.targetFilePath = ct.executable;
    bti.projectFilePath = projectFilePath;
    bti.usesTerminal = !ct.linksToQtGui;
    bti.isQtcRunnable = ct.qtcRunnable;

    // Workaround for QTCREATORBUG-19354:
    const FilePath qtBinPath = [bs] {
        if (const QtSupport::QtVersion *qt = QtSupport::QtKitAspect::qtVersion(bs->kit()))
            return qt->binPath();
        return FilePath();
    }();
    bti.runEnvModifierHash = qHashMulti(0, bti.buildKey, qtBinPath);
    bti.runEnvModifier = [bs, buildKey = ct.title, qtBinPath](Environment &env, bool enabled) {
        if (!enabled)
            return;
        env.prependOrSetLibrarySearchPaths(librarySearchPaths(bs->buildTargets(), buildKey));
        // On Windows an application locates its Qt runtime DLLs via PATH. CMake's
        // library directories point at Qt's import-lib (lib) directory, not the DLL
        // (bin) directory, so add Qt's bin explicitly. Without it a Qt application
        // started on a (remote) Windows device exits before main() because Qt6*.dll
        // cannot be found.
        if (env.osType() == OsTypeWindows && !qtBinPath.isEmpty())
            env.prependOrSetPath(qtBinPath);
    };
    return bti;
}

static BuildTargetInfo createUtilityBuildTargetInfo(
    const CMakeBuildTarget &ct,
    const FilePath &cmakeExecutable,
    const FilePath &workingDirectory,
    const FilePath &projectFilePath,
    bool qtcRunnable)
{
    BuildTargetInfo bti;
    bti.displayName = ct.title;
    bti.buildKey = ct.title;
    bti.targetFilePath = cmakeExecutable;
    bti.projectFilePath = projectFilePath;
    bti.workingDirectory = workingDirectory;
    bti.isQtcRunnable = qtcRunnable;
    bti.additionalData = QVariantMap{
        {"arguments", QStringList{"--build", ".", "--target", ct.title}}};
    return bti;
}

static BuildTargetInfo createStandaloneBuildTargetInfo(
    const PresetsDetails::RunSettings &rs,
    const FilePath &executable,
    const FilePath &cmakeExecutable,
    const FilePath &workingDirectory,
    const FilePath &projectFilePath)
{
    BuildTargetInfo bti;
    bti.buildKey = rs.target;
    bti.targetFilePath = executable.isEmpty() ? cmakeExecutable : executable;
    bti.projectFilePath = projectFilePath;
    bti.workingDirectory = workingDirectory;
    bti.usesTerminal = rs.useTerminal.value_or(true);
    bti.isQtcRunnable = true;
    if (executable.isEmpty()) {
        bti.additionalData = QVariantMap{
            {"arguments", QStringList{"--build", ".", "--target", rs.target}}};
    }
    return bti;
}

const QList<BuildTargetInfo> CMakeBuildSystem::appTargets() const
{
    const CMakeConfig &cm = configurationFromCMake();
    QString emulator = cm.stringValueOf("CMAKE_CROSSCOMPILING_EMULATOR");

    const FilePath cmakeExecutable = CMakeKitAspect::cmakeExecutable(kit());
    const FilePath workingDirectory
        = Utils::findOrDefault(m_buildTargets, [](const CMakeBuildTarget &bt) {
              return bt.title == "clean";
          }).workingDirectory;

    QList<BuildTargetInfo> appTargetList;
    // Android and HarmonyOS build applications as module libraries, not executables. That
    // holds for a build device running HarmonyOS as well: what Qt Creator builds there for
    // the device it runs on is a module for the runner to load.
    const Utils::Id deviceType = RunDeviceTypeKitAspect::deviceTypeId(kit());
    const bool moduleLibraryIsApp
        = deviceType == Android::Constants::ANDROID_DEVICE_TYPE
          || deviceType == HarmonyOs::Constants::HARMONYOS_DEVICE_TYPE
          || deviceType == HarmonyOs::Constants::HARMONYOS_BUILD_DEVICE_TYPE;

    auto isAppTarget = [moduleLibraryIsApp](const CMakeBuildTarget &ct) {
        return ct.targetType == ExecutableType
               || (moduleLibraryIsApp && ct.targetType == DynamicLibraryType);
    };

    // Collect runSettings from the active configure preset, grouped by target name. The presets
    // data has to outlive the loops below, which refer to the run settings it owns.
    const CMakeProject *cmakeProject = project();
    const PresetsData presetsData = cmakeProject ? cmakeProject->presetsData() : PresetsData();
    const PresetsDetails::ConfigurePreset *activePreset = nullptr;
    QMap<QString, QList<const PresetsDetails::RunSettings *>> runSettingsByTarget;
    if (presetsData.havePresets) {
        const CMakeConfigItem presetItem = CMakeConfigurationKitAspect::cmakePresetConfigItem(kit());
        if (!presetItem.isNull()) {
            const QString presetName = presetItem.expandedValue(kit());
            for (const PresetsDetails::ConfigurePreset &cp : presetsData.configurePresets) {
                if (cp.name == presetName) {
                    activePreset = &cp;
                    for (const PresetsDetails::RunSettings &rs : cp.runSettings)
                        runSettingsByTarget[rs.target].append(&rs);
                    break;
                }
            }
        }
    }

    for (const CMakeBuildTarget &ct : m_buildTargets) {
        if (CMakeBuildSystem::filteredOutTarget(ct))
            continue;

        const FilePath projectFilePath = ct.sourceDirectory.cleanPath().pathAppended(
            Constants::CMAKE_LISTS_TXT);

        // One run configuration per run setting, or a single one if the target has none.
        const QList<const PresetsDetails::RunSettings *> runSettings
            = runSettingsByTarget.value(ct.title);

        if (isAppTarget(ct)) {
            BuildTargetInfo bti = createBuildTargetInfo(this, ct, projectFilePath);

            if (ct.launchers.size() > 0)
                bti.launchers = ct.launchers;
            else if (!emulator.isEmpty()) {
                // fallback for cmake < 3.29
                QStringList args = emulator.split(";");
                FilePath command = FilePath::fromString(args.takeFirst());
                LauncherInfo launcherInfo = { "emulator", command, args };
                bti.launchers.append(Launcher(launcherInfo, ct.sourceDirectory));
            }

            // Remove the test launchers if the target is not a test
            if (bti.launchers.size() > 0
                && !isTestTarget(ct.title, bti.targetFilePath.parentDir())) {
                bti.launchers = Utils::filtered(bti.launchers, [](const Launcher &l) {
                    return !l.id.startsWith("test");
                });
            }

            if (runSettings.isEmpty()) {
                appTargetList.append(bti);
            } else {
                for (int idx = 0; idx < runSettings.size(); ++idx) {
                    BuildTargetInfo rsBti = bti;
                    rsBti.displayName
                        = runSettingsDisplayName(*runSettings.at(idx), ct.title, idx);
                    appTargetList.append(rsBti);
                }
            }
        } else if (ct.targetType == UtilityType) {
            // Skip the "all", "clean", "install" special targets.
            if (CMakeBuildStep::specialTargets(m_reader.usesAllCapsTargets()).contains(ct.title))
                continue;

            if (cmakeExecutable.isEmpty())
                continue;

            // Create targets from runSettings if present, otherwise from CMake info
            if (!runSettings.isEmpty()) {
                for (int idx = 0; idx < runSettings.size(); ++idx) {
                    BuildTargetInfo bti = createUtilityBuildTargetInfo(
                        ct,
                        cmakeExecutable,
                        workingDirectory.isEmpty() ? m_parameters.buildDirectory : workingDirectory,
                        projectFilePath,
                        true);
                    bti.displayName = runSettingsDisplayName(*runSettings.at(idx), ct.title, idx);
                    appTargetList.append(bti);
                }
            } else if (ct.qtcRunnable) {
                appTargetList.append(createUtilityBuildTargetInfo(
                    ct, cmakeExecutable, workingDirectory, projectFilePath, ct.qtcRunnable));
            }
        }
    }

    // Handle runSettings without a matching CMake build target (standalone executables)
    if (activePreset && !runSettingsByTarget.isEmpty() && !cmakeExecutable.isEmpty()) {
        const FilePath projectFilePath = cmakeProject->projectFilePath();
        const FilePath sourceDirectory = cmakeProject->projectDirectory();
        const Environment env = buildConfiguration()->environment();
        const QSet<QString> cmakeTargets = Utils::transform<QSet>(m_buildTargets,
                                                                 &CMakeBuildTarget::title);

        for (auto it = runSettingsByTarget.constBegin(); it != runSettingsByTarget.constEnd(); ++it) {
            if (cmakeTargets.contains(it.key()))
                continue;

            for (int idx = 0; idx < it.value().size(); ++idx) {
                const PresetsDetails::RunSettings &rs = *it.value().at(idx);
                FilePath executable;
                if (rs.executable) {
                    QString exe = *rs.executable;
                    CMakePresets::Macros::expand(*activePreset, env, sourceDirectory, exe);
                    executable = FilePath::fromUserInput(exe);
                }
                BuildTargetInfo bti = createStandaloneBuildTargetInfo(
                    rs,
                    executable,
                    cmakeExecutable,
                    workingDirectory.isEmpty() ? m_parameters.buildDirectory : workingDirectory,
                    projectFilePath);
                bti.displayName = runSettingsDisplayName(rs, rs.target, idx);
                appTargetList.append(bti);
            }
        }
    }

    return appTargetList;
}

QStringList CMakeBuildSystem::buildTargetTitles() const
{
    auto nonAutogenTargets = filtered(m_buildTargets, [](const CMakeBuildTarget &target){
        return !CMakeBuildSystem::filteredOutTarget(target);
    });
    return transform(nonAutogenTargets, &CMakeBuildTarget::title);
}

const QList<CMakeBuildTarget> &CMakeBuildSystem::buildTargets() const
{
    return m_buildTargets;
}

bool CMakeBuildSystem::filteredOutTarget(const CMakeBuildTarget &target)
{
    return target.title.endsWith("_autogen") ||
           target.title.endsWith("_autogen_timestamp_deps");
}

bool CMakeBuildSystem::isMultiConfig() const
{
    return m_isMultiConfig;
}

void CMakeBuildSystem::setIsMultiConfig(bool isMultiConfig)
{
    m_isMultiConfig = isMultiConfig;
}

bool CMakeBuildSystem::isMultiConfigReader() const
{
    return m_reader.isMultiConfig();
}

bool CMakeBuildSystem::usesAllCapsTargets() const
{
    return m_reader.usesAllCapsTargets();
}

CMakeProject *CMakeBuildSystem::project() const
{
    return static_cast<CMakeProject *>(ProjectExplorer::BuildSystem::project());
}

const QList<TestCaseInfo> CMakeBuildSystem::testcasesInfo() const
{
    return m_testNames;
}

FilePath CMakeBuildSystem::ctestPath() const
{
    return m_ctestPath;
}

CommandLine CMakeBuildSystem::commandLineForTests(const QStringList &tests,
                                                  const QStringList &options) const
{
    const QSet<QString> testsSet = Utils::toSet(tests);
    const auto current = Utils::transform<QSet<QString>>(m_testNames, &TestCaseInfo::name);
    if (tests.isEmpty() || current == testsSet)
        return {m_ctestPath, options};

    QString testNumbers("0,0,0"); // start, end, stride
    for (const TestCaseInfo &info : m_testNames) {
        if (testsSet.contains(info.name))
            testNumbers += QString(",%1").arg(info.number);
    }
    return {m_ctestPath, {options, "-I", testNumbers}};
}

FilePath CMakeBuildSystem::activeBuildTool() const
{
    Kit *k = kit();
    return k ? CMakeKitAspect::cmakeExecutable(k)
             : FilePath::fromString("cmake");
}

std::optional<DeploymentData> CMakeBuildSystem::deploymentDataFromFile() const
{
    DeploymentData result;

    FilePath sourceDir = project()->projectDirectory();
    FilePath buildDir = buildConfiguration()->buildDirectory();

    QString deploymentPrefix;
    FilePath deploymentFilePath = sourceDir.pathAppended("QtCreatorDeployment.txt");

    bool hasDeploymentFile = deploymentFilePath.exists();
    if (!hasDeploymentFile) {
        deploymentFilePath = buildDir.pathAppended("QtCreatorDeployment.txt");
        hasDeploymentFile = deploymentFilePath.exists();
    }
    if (!hasDeploymentFile)
        return {};

    deploymentPrefix = result.addFilesFromDeploymentFile(deploymentFilePath, sourceDir);
    for (const CMakeBuildTarget &ct : m_buildTargets) {
        if (ct.targetType == ExecutableType || ct.targetType == DynamicLibraryType) {
            if (!ct.executable.isEmpty()
                    && result.deployableForLocalFile(ct.executable).localFilePath() != ct.executable) {
                result.addFile(
                    ct.executable,
                    deploymentPrefix + buildDir.relativeChildPath(ct.executable).path(),
                    DeployableFile::TypeExecutable);
            }
        }
    }

    return result;
}

QString CMakeBuildSystem::deploymentHint() const
{
    return Tr::tr("For CMake projects, add install(TARGETS ...) rules so that Qt Creator adds a "
                  "CMake Install deploy step.");
}

QList<ExtraCompiler *> CMakeBuildSystem::findExtraCompilers()
{
    qCDebug(cmakeBuildSystemLog) << "Finding Extra Compilers: start.";

    QList<ExtraCompiler *> extraCompilers;
    const QList<ExtraCompilerFactory *> factories = ExtraCompilerFactory::extraCompilerFactories();

    qCDebug(cmakeBuildSystemLog) << "Finding Extra Compilers: Got factories.";

    const QSet<QString> fileExtensions = Utils::transform<QSet>(factories,
                                                                &ExtraCompilerFactory::sourceTag);

    qCDebug(cmakeBuildSystemLog) << "Finding Extra Compilers: Got file extensions:"
                                 << fileExtensions;

    // Find all files generated by any of the extra compilers, in a rather crude way.
    Project *p = project();
    const FilePaths fileList = p->files([&fileExtensions](const Node *n) {
        if (!Project::SourceFiles(n) || !n->isEnabled()) // isEnabled excludes nodes from the file system tree
            return false;
        const QString suffix = n->filePath().suffix();
        return !suffix.isEmpty() && fileExtensions.contains(suffix);
    });

    qCDebug(cmakeBuildSystemLog) << "Finding Extra Compilers: Got list of files to check.";

    // Generate the necessary information:
    for (const FilePath &file : fileList) {
        qCDebug(cmakeBuildSystemLog)
            << "Finding Extra Compilers: Processing" << file.toUserOutput();
        ExtraCompilerFactory *factory = Utils::findOrDefault(factories,
                                                             [&file](const ExtraCompilerFactory *f) {
                                                                 return file.endsWith(
                                                                     '.' + f->sourceTag());
                                                             });
        QTC_ASSERT(factory, continue);

        FilePaths generated = filesGeneratedFrom(file);
        qCDebug(cmakeBuildSystemLog)
            << "Finding Extra Compilers:     generated files:" << generated;
        if (generated.isEmpty())
            continue;

        extraCompilers.append(factory->create(p, file, generated));
        qCDebug(cmakeBuildSystemLog)
            << "Finding Extra Compilers:     done with" << file.toUserOutput();
    }

    qCDebug(cmakeBuildSystemLog) << "Finding Extra Compilers: done.";

    return extraCompilers;
}

void CMakeBuildSystem::updateQmlCodeModelInfo(QmlCodeModelInfo &projectInfo)
{
    auto addImports = [&projectInfo](const QString &imports) {
        const QStringList importList = CMakeConfigItem::cmakeSplitValue(imports);
        for (const QString &import : importList)
            projectInfo.qmlImportPaths.append(FilePath::fromUserInput(import));
    };

    const CMakeConfig &cm = configurationFromCMake();
    addImports(cm.stringValueOf("QML_IMPORT_PATH"));
    addImports(kit()->value(QtSupport::Constants::KIT_QML_IMPORT_PATH).toString());

    for (const QString &extraHeaderPath : std::as_const(m_extraHeaderPaths))
        projectInfo.qmlImportPaths.append(FilePath::fromString(extraHeaderPath));

    for (const QByteArray &mm : std::as_const(m_moduleMappings)) {
        auto kvPair = mm.split('=');
        if (kvPair.size() != 2)
            continue;
        QString from = QString::fromUtf8(kvPair.at(0).trimmed());
        QString to = QString::fromUtf8(kvPair.at(1).trimmed());
        if (!from.isEmpty() && !to.isEmpty() && from != to) {
            // The QML code-model does not support sub-projects, so if there are multiple mappings for a single module,
            // choose the shortest one.
            if (projectInfo.moduleMappings.contains(from)) {
                if (to.size() < projectInfo.moduleMappings.value(from).size())
                    projectInfo.moduleMappings.insert(from, to);
            } else {
                projectInfo.moduleMappings.insert(from, to);
            }
        }
    }

    project()->setProjectLanguage(ProjectExplorer::Constants::QMLJS_LANGUAGE_ID,
                                  !projectInfo.sourceFiles.isEmpty());
}

void CMakeBuildSystem::updateInitialCMakeExpandableVars()
{
    const CMakeConfig &cm = configurationFromCMake();
    const CMakeConfig &initialConfig = CMakeBuildConfiguration::updateCMakeHelperConfig(
        cmakeBuildConfiguration()->initialCMakeArguments.cmakeConfiguration());

    CMakeConfig config;

    const FilePath projectDirectory = project()->projectDirectory();
    const auto samePath = [projectDirectory](const FilePath &first, const FilePath &second) {
        // if a path is relative, resolve it relative to the project directory
        // this is not 100% correct since CMake resolve them to CMAKE_CURRENT_SOURCE_DIR
        // depending on context, but we cannot do better here
        return first == second
               || projectDirectory.resolvePath(first)
                      == projectDirectory.resolvePath(second)
               || projectDirectory.resolvePath(first).canonicalPath()
                      == projectDirectory.resolvePath(second).canonicalPath();
    };

    // Replace path values that do not  exist on file system
    const QByteArrayList singlePathList = {
        "CMAKE_C_COMPILER",
        "CMAKE_CXX_COMPILER",
        "QT_QMAKE_EXECUTABLE",
        "QT_HOST_PATH",
        "CMAKE_TOOLCHAIN_FILE"
    };
    for (const auto &var : singlePathList) {
        auto it = std::find_if(cm.cbegin(), cm.cend(), [var](const CMakeConfigItem &item) {
            return item.key == var && !item.isInitial;
        });

        if (it != cm.cend()) {
            const QByteArray initialValue = initialConfig.expandedValueOf(kit(), var).toUtf8();
            const FilePath initialPath = FilePath::fromUserInput(QString::fromUtf8(initialValue));
            const FilePath path = FilePath::fromUserInput(QString::fromUtf8(it->value));

            if (!initialValue.isEmpty() && !samePath(path, initialPath) && !path.exists()) {
                CMakeConfigItem item(*it);
                item.value = initialValue;

                config.insert(item);
            }
        }
    }

    // Prepend new values to existing path lists
    const QByteArrayList multiplePathList = {
        "CMAKE_PREFIX_PATH",
        "CMAKE_FIND_ROOT_PATH"
    };
    for (const auto &var : multiplePathList) {
        auto it = std::find_if(cm.cbegin(), cm.cend(), [var](const CMakeConfigItem &item) {
            return item.key == var && !item.isInitial;
        });

        if (it != cm.cend()) {
            const QByteArrayList initialValueList = initialConfig.expandedValueOf(kit(), var).toUtf8().split(';');

            for (const auto &initialValue: initialValueList) {
                const FilePath initialPath = FilePath::fromUserInput(QString::fromUtf8(initialValue));

                const bool pathIsContained
                    = Utils::contains(it->value.split(';'), [samePath, initialPath](const QByteArray &p) {
                          return samePath(FilePath::fromUserInput(QString::fromUtf8(p)), initialPath);
                      });
                if (!initialValue.isEmpty() && !pathIsContained) {
                    CMakeConfigItem item(*it);
                    item.value = initialValue;
                    item.value.append(";");
                    item.value.append(it->value);

                    config.insert(item);
                }
            }
        }
    }

    // Handle MSVC C/C++ compiler update, by udating also the linker and librarian,
    // otherwise projects will fail to compile by using a linker (link.exe) and
    // a librarian (lib.exe) that do not exit
    const FilePath cxxCompiler = config.filePathValueOf("CMAKE_CXX_COMPILER");
    if (!cxxCompiler.isEmpty() && cxxCompiler.fileName() == "cl.exe") {
        const FilePath linker = cm.filePathValueOf("CMAKE_LINKER");
        if (!linker.exists()) {
            const QString linkerFileName = linker.fileName() != "CMAKE_LINKER-NOTFOUND"
                                               ? linker.fileName()
                                               : "link.exe";
            config.insert(CMakeConfigItem(
                "CMAKE_LINKER",
                CMakeConfigItem::FILEPATH,
                cxxCompiler.parentDir().pathAppended(linkerFileName).path().toUtf8()));
        }
        const FilePath librarian = cm.filePathValueOf("CMAKE_AR");
        if (!librarian.exists()) {
            const QString librarianFileName = librarian.fileName() != "CMAKE_AR-NOTFOUND"
                                                  ? librarian.fileName()
                                                  : "lib.exe";
            config.insert(CMakeConfigItem(
                "CMAKE_AR",
                CMakeConfigItem::FILEPATH,
                cxxCompiler.parentDir().pathAppended(librarianFileName).path().toUtf8()));
        }
    }

    if (!config.isEmpty())
        emit configurationChanged(config);
}

// What the install rules of the project name is what an install would put on the target. A
// QtCreatorDeployment.txt file stays ahead of them: it is written for the very case its
// author found the rules not to cover, which an empty one says too. Which of the two the
// data came from decides how good it is, so both are settled here.
void CMakeBuildSystem::updateDeploymentData()
{
    const std::optional<DeploymentData> fromFile = deploymentDataFromFile();
    m_deploymentKnowledge = fromFile ? DeploymentKnowledge::Approximative
                                     : m_deploymentFromInstallRules.knowledge;
    setDeploymentData(fromFile ? *fromFile : m_deploymentFromInstallRules.data);
}

DeploymentKnowledge CMakeBuildSystem::deploymentKnowledge() const
{
    return m_deploymentKnowledge;
}

MakeInstallCommand CMakeBuildSystem::makeInstallCommand(const FilePath &installRoot) const
{
    MakeInstallCommand cmd;
    cmd.command.setExecutable(CMakeKitAspect::cmakeExecutable(kit()));

    QString installTarget = "install";
    if (usesAllCapsTargets())
        installTarget = "INSTALL";

    FilePath buildDirectory = ".";
    Project *project = nullptr;
    if (auto bc = buildConfiguration()) {
        buildDirectory = bc->buildDirectory();
        project = bc->project();
    }

    cmd.command.addArg("--build");
    cmd.command.addArg(CMakeToolManager::mappedFilePath(project, buildDirectory).path());
    cmd.command.addArg("--target");
    cmd.command.addArg(installTarget);

    if (isMultiConfigReader())
        cmd.command.addArgs({"--config", cmakeBuildType()});

    cmd.environment.set("DESTDIR", installRoot.nativePath());
    return cmd;
}

QList<QPair<Id, QString>> CMakeBuildSystem::generators() const
{
    if (!buildConfiguration())
        return {};
    const CMakeTool * const cmakeTool =
        CMakeToolManager::findByCommand(m_parameters.cmakeExecutable);
    if (!cmakeTool)
        return {};
    QList<QPair<Id, QString>> result;
    const QList<CMakeTool::Generator> &generators = cmakeTool->supportedGenerators();
    for (const CMakeTool::Generator &generator : generators) {
        result << qMakePair(Id::fromSetting(generator.name),
                            Tr::tr("%1 (via CMake)").arg(generator.name));
    }
    return result;
}

void CMakeBuildSystem::runGenerator(Id id)
{
    QTC_ASSERT(cmakeBuildConfiguration(), return);
    TaskHub::clearAndRemoveTask(m_generatorError);
    const auto showError = [this](const QString &detail) {
        m_generatorError = OtherTask(Task::DisruptingError,
                                     Tr::tr("CMake generator failed.").append('\n').append(detail));
        TaskHub::addTask(m_generatorError);
    };
    const FilePath cmakeExecutable = CMakeKitAspect::cmakeExecutable(kit());
    if (cmakeExecutable.isEmpty()) {
        showError(Tr::tr("Kit does not have a CMake binary set."));
        return;
    }
    if (!cmakeExecutable.isExecutableFile()) {
        showError(Tr::tr("No valid CMake executable."));
        return;
    }
    const QString generator = id.toSetting().toString();
    const FilePath outDir = buildConfiguration()->buildDirectory()
            / ("qtc_" + FileUtils::fileSystemFriendlyName(generator));
    if (!outDir.ensureWritableDir()) {
        showError(Tr::tr("Cannot create output directory \"%1\".").arg(outDir.toFSPathString()));
        return;
    }
    CommandLine cmdLine(cmakeExecutable, {"-S", buildConfiguration()
                        ->project()->projectDirectory().toUserOutput(), "-G", generator});
    const auto itemFilter = [](const CMakeConfigItem &item) {
        return !item.isNull()
                && item.type != CMakeConfigItem::STATIC
                && item.type != CMakeConfigItem::INTERNAL
                && !item.key.contains("GENERATOR");
    };
    QList<CMakeConfigItem> configItems = Utils::filtered(m_configurationChanges.toList(),
                                                         itemFilter);
    const QList<CMakeConfigItem> initialConfigItems
            = Utils::filtered(cmakeBuildConfiguration()->initialCMakeArguments.cmakeConfiguration().toList(),
                          itemFilter);
    for (const CMakeConfigItem &item : std::as_const(initialConfigItems)) {
        if (!Utils::contains(configItems, [&item](const CMakeConfigItem &existingItem) {
            return existingItem.key == item.key;
        })) {
            configItems << item;
        }
    }
    for (const CMakeConfigItem &item : std::as_const(configItems))
        cmdLine.addArg(item.toArgument(buildConfiguration()->macroExpander()));

    cmdLine.addArgs(cmakeBuildConfiguration()->additionalCMakeOptions(), CommandLine::Raw);

    const auto proc = new Process(this);
    connect(proc, &Process::done, proc, &Process::deleteLater);
    connect(proc, &Process::readyReadStandardOutput, this, [proc] {
        Core::MessageManager::writeFlashing(
            addCMakePrefix(QString::fromLocal8Bit(proc->readAllRawStandardOutput()).split('\n')));
    });
    connect(proc, &Process::readyReadStandardError, this, [proc] {
        Core::MessageManager::writeDisrupting(
            addCMakePrefix(QString::fromLocal8Bit(proc->readAllRawStandardError()).split('\n')));
    });
    proc->setWorkingDirectory(outDir);
    proc->setEnvironment(buildConfiguration()->environment());
    proc->setCommand(cmdLine);
    Core::MessageManager::writeFlashing(addCMakePrefix(
        Tr::tr("Running in \"%1\": %2.").arg(outDir.toUserOutput(), cmdLine.toUserOutput())));
    proc->start();
}

ExtraCompiler *CMakeBuildSystem::findExtraCompiler(const ExtraCompilerFilter &filter) const
{
    return Utils::findOrDefault(m_extraCompilers, filter);
}

} // CMakeProjectManager::Internal

#ifdef WITH_TESTS
#include <cmakebuildsystem.moc>
#endif
