// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cmakebuildsystem.h"

#include "builddirparameters.h"
#include "cmakebuildconfiguration.h"
#include "cmakebuildstep.h"
#include "cmakebuildtarget.h"
#include "cmakekitaspect.h"
#include "cmakeprocess.h"
#include "cmakeproject.h"
#include "cmakeprojectconstants.h"
#include "cmakeprojectmanagertr.h"
#include "cmakespecificsettings.h"
#include "projecttreehelper.h"

#include <android/androidconstants.h>

#include <coreplugin/icore.h>
#include <coreplugin/documentmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/progressmanager/progressmanager.h>

#include <projectexplorer/extracompiler.h>
#include <projectexplorer/kitaspects.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectupdater.h>
#include <projectexplorer/target.h>
#include <projectexplorer/taskhub.h>

#include <texteditor/texteditor.h>
#include <texteditor/textdocument.h>

#include <qmljs/qmljsmodelmanagerinterface.h>

#include <qtsupport/qtcppkitinfo.h>
#include <qtsupport/qtsupportconstants.h>

#include <utils/algorithm.h>
#include <utils/checkablemessagebox.h>
#include <utils/macroexpander.h>
#include <utils/mimeconstants.h>
#include <utils/process.h>
#include <utils/qtcassert.h>

#include <QClipboard>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>

using namespace ProjectExplorer;
using namespace TextEditor;
using namespace Utils;

namespace CMakeProjectManager::Internal {

static Q_LOGGING_CATEGORY(cmakeBuildSystemLog, "qtc.cmake.buildsystem", QtWarningMsg);

// --------------------------------------------------------------------
// CMakeBuildSystem:
// --------------------------------------------------------------------

CMakeBuildSystem::CMakeBuildSystem(CMakeBuildConfiguration *bc)
    : BuildSystem(bc)
    , m_cppCodeModelUpdater(ProjectUpdaterFactory::createCppProjectUpdater())
{
    // TreeScanner:
    connect(&m_treeScanner, &TreeScanner::finished,
            this, &CMakeBuildSystem::handleTreeScanningFinished);

    m_treeScanner.setFilter([this](const MimeType &mimeType, const FilePath &fn) {
        // Mime checks requires more resources, so keep it last in check list
        auto isIgnored = TreeScanner::isWellKnownBinary(mimeType, fn);

        // Cache mime check result for speed up
        if (!isIgnored) {
            auto it = m_mimeBinaryCache.find(mimeType.name());
            if (it != m_mimeBinaryCache.end()) {
                isIgnored = *it;
            } else {
                isIgnored = TreeScanner::isMimeBinary(mimeType, fn);
                m_mimeBinaryCache[mimeType.name()] = isIgnored;
            }
        }

        return isIgnored;
    });

    m_treeScanner.setTypeFactory([](const MimeType &mimeType, const FilePath &fn) {
        auto type = TreeScanner::genericFileType(mimeType, fn);
        if (type == FileType::Unknown) {
            if (mimeType.isValid()) {
                const QString mt = mimeType.name();
                if (mt == Utils::Constants::CMAKE_PROJECT_MIMETYPE
                    || mt == Utils::Constants::CMAKE_MIMETYPE)
                    type = FileType::Project;
            }
        }
        return type;
    });

    connect(&m_reader, &FileApiReader::configurationStarted, this, [this] {
        clearError(ForceEnabledChanged::True);
    });

    connect(&m_reader,
            &FileApiReader::dataAvailable,
            this,
            &CMakeBuildSystem::handleParsingSucceeded);
    connect(&m_reader, &FileApiReader::errorOccurred, this, &CMakeBuildSystem::handleParsingFailed);
    connect(&m_reader, &FileApiReader::dirty, this, &CMakeBuildSystem::becameDirty);
    connect(&m_reader, &FileApiReader::debuggingStarted, this, &BuildSystem::debuggingStarted);

    wireUpConnections();

    m_isMultiConfig = CMakeGeneratorKitAspect::isMultiConfigGenerator(bc->kit());
}

CMakeBuildSystem::~CMakeBuildSystem()
{
    if (!m_treeScanner.isFinished()) {
        auto future = m_treeScanner.future();
        future.cancel();
        future.waitForFinished();
    }

    delete m_cppCodeModelUpdater;
    qDeleteAll(m_extraCompilers);
}

void CMakeBuildSystem::triggerParsing()
{
    qCDebug(cmakeBuildSystemLog) << buildConfiguration()->displayName() << "Parsing has been triggered";

    if (!buildConfiguration()->isActive()) {
        qCDebug(cmakeBuildSystemLog)
            << "Parsing has been triggered: SKIPPING since BC is not active -- clearing state.";
        stopParsingAndClearState();
        return; // ignore request, this build configuration is not active!
    }

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

    const FilePath cache = m_parameters.buildDirectory.pathAppended("CMakeCache.txt");
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

    const CMakeTool *tool = m_parameters.cmakeTool();
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
    if (dynamic_cast<CMakeTargetNode *>(context))
        return action == ProjectAction::AddNewFile || action == ProjectAction::AddExistingFile
               || action == ProjectAction::AddExistingDirectory || action == ProjectAction::Rename
               || action == ProjectAction::RemoveFile;

    return BuildSystem::supportsAction(context, action, node);
}

static QString newFilesForFunction(const std::string &cmakeFunction,
                                   const FilePaths &filePaths,
                                   const FilePath &projDir)
{
    auto relativeFilePaths = [projDir](const FilePaths &filePaths) {
        return Utils::transform(filePaths, [projDir](const FilePath &path) {
            return path.canonicalPath().relativePathFrom(projDir).cleanPath().toString();
        });
    };

    if (cmakeFunction == "qt_add_qml_module" || cmakeFunction == "qt6_add_qml_module") {
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

        QStringList result;
        if (!sourceFiles.isEmpty())
            result << QString("SOURCES %1").arg(relativeFilePaths(sourceFiles).join(" "));
        if (!resourceFiles.isEmpty())
            result << QString("RESOURCES %1").arg(relativeFilePaths(resourceFiles).join(" "));
        if (!qmlFiles.isEmpty())
            result << QString("QML_FILES %1").arg(relativeFilePaths(qmlFiles).join(" "));

        return result.join("\n");
    }

    return relativeFilePaths(filePaths).join(" ");
}

bool CMakeBuildSystem::addFiles(Node *context, const FilePaths &filePaths, FilePaths *notAdded)
{
    if (notAdded)
        *notAdded = filePaths;
    if (auto n = dynamic_cast<CMakeTargetNode *>(context)) {
        const QString targetName = n->buildKey();
        auto target = Utils::findOrDefault(buildTargets(),
                                           [targetName](const CMakeBuildTarget &target) {
                                               return target.title == targetName;
                                           });

        if (target.backtrace.isEmpty()) {
            qCCritical(cmakeBuildSystemLog) << "target.backtrace for" << targetName << "is empty. "
                                            << "The location where to add the files is unknown.";
            return false;
        }

        const FilePath targetCMakeFile = target.backtrace.last().path;
        const int targetDefinitionLine = target.backtrace.last().line;

        // Have a fresh look at the CMake file, not relying on a cached value
        Core::DocumentManager::saveModifiedDocumentSilently(
            Core::DocumentModel::documentForFilePath(targetCMakeFile));
        expected_str<QByteArray> fileContent = targetCMakeFile.fileContents();
        cmListFile cmakeListFile;
        std::string errorString;
        if (fileContent) {
            fileContent = fileContent->replace("\r\n", "\n");
            if (!cmakeListFile.ParseString(fileContent->toStdString(),
                                           targetCMakeFile.fileName().toStdString(),
                                           errorString)) {
                qCCritical(cmakeBuildSystemLog).noquote()
                    << targetCMakeFile.path() << "failed to parse! Error:" << errorString;
                return false;
            }
        }

        auto function = std::find_if(cmakeListFile.Functions.begin(),
                                     cmakeListFile.Functions.end(),
                                     [targetDefinitionLine](const auto &func) {
                                         return func.Line() == targetDefinitionLine;
                                     });

        if (function == cmakeListFile.Functions.end()) {
            qCCritical(cmakeBuildSystemLog) << "Function that defined the target" << targetName
                                            << "could not be found at" << targetDefinitionLine;
            return false;
        }

        // Special case: when qt_add_executable and qt_add_qml_module use the same target name
        // then qt_add_qml_module function should be used
        const std::string target_name = targetName.toStdString();
        auto add_qml_module_func
            = std::find_if(cmakeListFile.Functions.begin(),
                           cmakeListFile.Functions.end(),
                           [target_name](const auto &func) {
                               return (func.LowerCaseName() == "qt_add_qml_module"
                                       || func.LowerCaseName() == "qt6_add_qml_module")
                                      && func.Arguments().front().Value == target_name;
                           });
        if (add_qml_module_func != cmakeListFile.Functions.end())
            function = add_qml_module_func;

        const QString newSourceFiles = newFilesForFunction(function->LowerCaseName(),
                                                           filePaths,
                                                           n->filePath().canonicalPath());

        static QSet<std::string> knownFunctions{"add_executable",
                                                "add_library",
                                                "qt_add_executable",
                                                "qt_add_library",
                                                "qt6_add_executable",
                                                "qt6_add_library",
                                                "qt_add_qml_module",
                                                "qt6_add_qml_module"};

        int line = 0;
        int column = 0;
        int extraChars = 0;
        QString snippet;

        auto afterFunctionLastArgument =
            [&line, &column, &snippet, &extraChars, newSourceFiles](const auto &f) {
                auto lastArgument = f->Arguments().back();

                line = lastArgument.Line;
                column = lastArgument.Column + static_cast<int>(lastArgument.Value.size()) - 1;
                snippet = QString("\n%1").arg(newSourceFiles);

                // Take into consideration the quotes
                if (lastArgument.Delim == cmListFileArgument::Quoted)
                    extraChars = 2;
            };

        if (knownFunctions.contains(function->LowerCaseName())) {
            afterFunctionLastArgument(function);
        } else {
            auto targetSourcesFunc = std::find_if(cmakeListFile.Functions.begin(),
                                                  cmakeListFile.Functions.end(),
                                                  [target_name](const auto &func) {
                                                      return func.LowerCaseName()
                                                                 == "target_sources"
                                                             && func.Arguments().front().Value
                                                                    == target_name;
                                                  });

            if (targetSourcesFunc == cmakeListFile.Functions.end()) {
                line = function->LineEnd() + 1;
                column = 0;
                snippet = QString("\ntarget_sources(%1\n  PRIVATE\n    %2\n)\n")
                              .arg(targetName)
                              .arg(newSourceFiles);
            } else {
                afterFunctionLastArgument(targetSourcesFunc);
            }
        }

        BaseTextEditor *editor = qobject_cast<BaseTextEditor *>(
            Core::EditorManager::openEditorAt({targetCMakeFile, line, column + extraChars},
                                              Constants::CMAKE_EDITOR_ID,
                                              Core::EditorManager::DoNotMakeVisible));
        if (!editor) {
            qCCritical(cmakeBuildSystemLog).noquote()
                << "BaseTextEditor cannot be obtained for" << targetCMakeFile.path() << line
                << int(column + extraChars);
            return false;
        }

        editor->insert(snippet);
        editor->editorWidget()->autoIndent();
        if (!Core::DocumentManager::saveDocument(editor->document())) {
            qCCritical(cmakeBuildSystemLog).noquote()
                << "Changes to" << targetCMakeFile.path() << "could not be saved.";
            return false;
        }

        if (notAdded)
            notAdded->clear();
        return true;
    }

    return BuildSystem::addFiles(context, filePaths, notAdded);
}

std::optional<CMakeBuildSystem::ProjectFileArgumentPosition>
CMakeBuildSystem::projectFileArgumentPosition(const QString &targetName, const QString &fileName)
{
    auto target = Utils::findOrDefault(buildTargets(), [targetName](const CMakeBuildTarget &target) {
        return target.title == targetName;
    });

    if (target.backtrace.isEmpty())
        return std::nullopt;

    const FilePath targetCMakeFile = target.backtrace.last().path;

    // Have a fresh look at the CMake file, not relying on a cached value
    Core::DocumentManager::saveModifiedDocumentSilently(
        Core::DocumentModel::documentForFilePath(targetCMakeFile));
    expected_str<QByteArray> fileContent = targetCMakeFile.fileContents();
    cmListFile cmakeListFile;
    std::string errorString;
    if (fileContent) {
        fileContent = fileContent->replace("\r\n", "\n");
        if (!cmakeListFile.ParseString(fileContent->toStdString(),
                                       targetCMakeFile.fileName().toStdString(),
                                       errorString))
            return std::nullopt;
    }

    const int targetDefinitionLine = target.backtrace.last().line;

    auto function = std::find_if(cmakeListFile.Functions.begin(),
                                 cmakeListFile.Functions.end(),
                                 [targetDefinitionLine](const auto &func) {
                                     return func.Line() == targetDefinitionLine;
                                 });

    const std::string target_name = targetName.toStdString();
    auto targetSourcesFunc = std::find_if(cmakeListFile.Functions.begin(),
                                          cmakeListFile.Functions.end(),
                                          [target_name](const auto &func) {
                                              return func.LowerCaseName() == "target_sources"
                                                     && func.Arguments().size() > 1
                                                     && func.Arguments().front().Value
                                                            == target_name;
                                          });
    auto addQmlModuleFunc = std::find_if(cmakeListFile.Functions.begin(),
                                         cmakeListFile.Functions.end(),
                                         [target_name](const auto &func) {
                                             return (func.LowerCaseName() == "qt_add_qml_module"
                                                     || func.LowerCaseName() == "qt6_add_qml_module")
                                                    && func.Arguments().size() > 1
                                                    && func.Arguments().front().Value
                                                           == target_name;
                                         });

    for (const auto &func : {function, targetSourcesFunc, addQmlModuleFunc}) {
        if (func == cmakeListFile.Functions.end())
            continue;
        auto filePathArgument = Utils::findOrDefault(func->Arguments(),
                                                     [file_name = fileName.toStdString()](
                                                         const auto &arg) {
                                                         return arg.Value == file_name;
                                                     });

        if (!filePathArgument.Value.empty()) {
            return ProjectFileArgumentPosition{filePathArgument, targetCMakeFile, fileName};
        } else {
            // Check if the filename is part of globbing variable result
            const auto globFunctions = std::get<0>(
                Utils::partition(cmakeListFile.Functions, [](const auto &f) {
                    return f.LowerCaseName() == "file" && f.Arguments().size() > 2
                           && (f.Arguments().front().Value == "GLOB"
                               || f.Arguments().front().Value == "GLOB_RECURSE");
                }));

            const auto globVariables = Utils::transform<QSet>(globFunctions, [](const auto &func) {
                return std::string("${") + func.Arguments()[1].Value + "}";
            });

            const auto haveGlobbing = Utils::anyOf(func->Arguments(),
                                                   [globVariables](const auto &arg) {
                                                       return globVariables.contains(arg.Value);
                                                   });

            if (haveGlobbing) {
                return ProjectFileArgumentPosition{filePathArgument,
                                                   targetCMakeFile,
                                                   fileName,
                                                   true};
            }

            // Check if the filename is part of a variable set by the user
            const auto setFunctions = std::get<0>(
                Utils::partition(cmakeListFile.Functions, [](const auto &f) {
                    return f.LowerCaseName() == "set" && f.Arguments().size() > 1;
                }));

            for (const auto &arg : func->Arguments()) {
                auto matchedFunctions = Utils::filtered(setFunctions, [arg](const auto &f) {
                    return arg.Value == std::string("${") + f.Arguments()[0].Value + "}";
                });

                for (const auto &f : matchedFunctions) {
                    filePathArgument = Utils::findOrDefault(f.Arguments(),
                                                            [file_name = fileName.toStdString()](
                                                                const auto &arg) {
                                                                return arg.Value == file_name;
                                                            });

                    if (!filePathArgument.Value.empty()) {
                        return ProjectFileArgumentPosition{filePathArgument,
                                                           targetCMakeFile,
                                                           fileName};
                    }
                }
            }
        }
    }

    return std::nullopt;
}

RemovedFilesFromProject CMakeBuildSystem::removeFiles(Node *context,
                                                      const FilePaths &filePaths,
                                                      FilePaths *notRemoved)
{
    FilePaths badFiles;
    if (auto n = dynamic_cast<CMakeTargetNode *>(context)) {
        const FilePath projDir = n->filePath().canonicalPath();
        const QString targetName = n->buildKey();

        for (const auto &file : filePaths) {
            const QString fileName
                = file.canonicalPath().relativePathFrom(projDir).cleanPath().toString();

            auto filePos = projectFileArgumentPosition(targetName, fileName);
            if (filePos) {
                if (!filePos.value().cmakeFile.exists()) {
                    badFiles << file;

                    qCCritical(cmakeBuildSystemLog).noquote()
                        << "File" << filePos.value().cmakeFile.path() << "does not exist.";
                    continue;
                }

                BaseTextEditor *editor = qobject_cast<BaseTextEditor *>(
                    Core::EditorManager::openEditorAt({filePos.value().cmakeFile,
                                                       static_cast<int>(filePos.value().argumentPosition.Line),
                                                       static_cast<int>(filePos.value().argumentPosition.Column
                                                                        - 1)},
                                                      Constants::CMAKE_EDITOR_ID,
                                                      Core::EditorManager::DoNotMakeVisible));
                if (!editor) {
                    badFiles << file;

                    qCCritical(cmakeBuildSystemLog).noquote()
                        << "BaseTextEditor cannot be obtained for"
                        << filePos.value().cmakeFile.path() << filePos.value().argumentPosition.Line
                        << int(filePos.value().argumentPosition.Column - 1);
                    continue;
                }

                // If quotes were used for the source file, remove the quotes too
                int extraChars = 0;
                if (filePos->argumentPosition.Delim == cmListFileArgument::Quoted)
                    extraChars = 2;

                if (!filePos.value().fromGlobbing)
                    editor->replace(filePos.value().relativeFileName.length() + extraChars, "");

                editor->editorWidget()->autoIndent();
                if (!Core::DocumentManager::saveDocument(editor->document())) {
                    badFiles << file;

                    qCCritical(cmakeBuildSystemLog).noquote()
                        << "Changes to" << filePos.value().cmakeFile.path()
                        << "could not be saved.";
                    continue;
                }
            } else {
                badFiles << file;
            }
        }

        if (notRemoved && !badFiles.isEmpty())
            *notRemoved = badFiles;

        return badFiles.isEmpty() ? RemovedFilesFromProject::Ok : RemovedFilesFromProject::Error;
    }

    return RemovedFilesFromProject::Error;
}

bool CMakeBuildSystem::canRenameFile(Node *context,
                                     const FilePath &oldFilePath,
                                     const FilePath &newFilePath)
{
    // "canRenameFile" will cause an actual rename after the function call.
    // This will make the a sequence like
    //    canonicalPath().relativePathFrom(projDir).cleanPath().toString()
    // to fail if the file doesn't exist on disk
    // therefore cache the results for the subsequent "renameFile" call
    // where oldFilePath has already been renamed as newFilePath.

    if (auto n = dynamic_cast<CMakeTargetNode *>(context)) {
        const FilePath projDir = n->filePath().canonicalPath();
        const QString oldRelPathName
            = oldFilePath.canonicalPath().relativePathFrom(projDir).cleanPath().toString();

        const QString targetName = n->buildKey();

        const QString key
            = QStringList{projDir.path(), targetName, oldFilePath.path(), newFilePath.path()}
                  .join(";");

        auto filePos = projectFileArgumentPosition(targetName, oldRelPathName);
        if (!filePos)
            return false;

        m_filesToBeRenamed.insert(key, filePos.value());
        return true;
    }
    return false;
}

bool CMakeBuildSystem::renameFile(Node *context,
                                  const FilePath &oldFilePath,
                                  const FilePath &newFilePath)
{
    if (auto n = dynamic_cast<CMakeTargetNode *>(context)) {
        const FilePath projDir = n->filePath().canonicalPath();
        const QString newRelPathName
            = newFilePath.canonicalPath().relativePathFrom(projDir).cleanPath().toString();

        const QString targetName = n->buildKey();
        const QString key
            = QStringList{projDir.path(), targetName, oldFilePath.path(), newFilePath.path()}.join(
                ";");

        auto fileToRename = m_filesToBeRenamed.take(key);
        if (!fileToRename.cmakeFile.exists()) {
            qCCritical(cmakeBuildSystemLog).noquote()
                << "File" << fileToRename.cmakeFile.path() << "does not exist.";
            return false;
        }

        BaseTextEditor *editor = qobject_cast<BaseTextEditor *>(
            Core::EditorManager::openEditorAt({fileToRename.cmakeFile,
                                               static_cast<int>(fileToRename.argumentPosition.Line),
                                               static_cast<int>(fileToRename.argumentPosition.Column
                                                                - 1)},
                                              Constants::CMAKE_EDITOR_ID,
                                              Core::EditorManager::DoNotMakeVisible));
        if (!editor) {
            qCCritical(cmakeBuildSystemLog).noquote()
                << "BaseTextEditor cannot be obtained for" << fileToRename.cmakeFile.path()
                << fileToRename.argumentPosition.Line << int(fileToRename.argumentPosition.Column);
            return false;
        }

        // If quotes were used for the source file, skip the starting quote
        if (fileToRename.argumentPosition.Delim == cmListFileArgument::Quoted)
            editor->setCursorPosition(editor->position() + 1);

        if (!fileToRename.fromGlobbing)
            editor->replace(fileToRename.relativeFileName.length(), newRelPathName);

        editor->editorWidget()->autoIndent();
        if (!Core::DocumentManager::saveDocument(editor->document())) {
            qCCritical(cmakeBuildSystemLog).noquote()
                << "Changes to" << fileToRename.cmakeFile.path() << "could not be saved.";
            return false;
        }

        return true;
    }

    return false;
}

FilePaths CMakeBuildSystem::filesGeneratedFrom(const FilePath &sourceFile) const
{
    FilePath project = projectDirectory();
    FilePath baseDirectory = sourceFile.parentDir();

    while (baseDirectory.isChildOf(project)) {
        const FilePath cmakeListsTxt = baseDirectory.pathAppended("CMakeLists.txt");
        if (cmakeListsTxt.exists())
            break;
        baseDirectory = baseDirectory.parentDir();
    }

    const FilePath relativePath = baseDirectory.relativePathFrom(project);
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
                    const FilePath filePath = n->filePath();
                    if (!filePath.contains(autogenSignature))
                        return false;

                    return Project::GeneratedFiles(n) && filePath.endsWith(generatedFileName);
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

    if (!buildConfiguration()->isActive()) {
        qCDebug(cmakeBuildSystemLog) << "setting parameters and requesting reparse: SKIPPING since BC is not active -- clearing state.";
        stopParsingAndClearState();
        return; // ignore request, this build configuration is not active!
    }

    const CMakeTool *tool = parameters.cmakeTool();
    if (!tool || !tool->isValid()) {
        TaskHub::addTask(
            BuildSystemTask(Task::Error,
                            Tr::tr("The kit needs to define a CMake tool to parse this project.")));
        return;
    }
    if (!tool->hasFileApi()) {
        TaskHub::addTask(
            BuildSystemTask(Task::Error,
                            CMakeKitAspect::msgUnsupportedVersion(tool->version().fullVersion)));
        return;
    }
    QTC_ASSERT(parameters.isValid(), return );

    m_parameters = parameters;
    ensureBuildDirectory(parameters);
    updateReparseParameters(reparseParameters);

    m_reader.setParameters(m_parameters);

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

    int answer = QMessageBox::question(Core::ICore::dialogParent(),
                                       Tr::tr("Apply configuration changes?"),
                                       "<p>" + Tr::tr("Run CMake with configuration changes?")
                                       + "</p><pre>"
                                       + parameters.configurationChangesArguments.join("\n")
                                       + "</pre>",
                                       QMessageBox::Apply | QMessageBox::Discard,
                                       QMessageBox::Apply);
    return answer == QMessageBox::Apply;
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

    if (reparseFlags == REPARSE_DEFAULT)
        return false;

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
        m_parameters.buildDirectory / "CMakeCache.txt",
        m_parameters.buildDirectory / "CMakeCache.txt.prev",
        m_parameters.buildDirectory / "CMakeFiles",
        m_parameters.buildDirectory / ".cmake/api/v1/reply",
        m_parameters.buildDirectory / ".cmake/api/v1/reply.prev",
        m_parameters.buildDirectory / Constants::PACKAGE_MANAGER_DIR
    };

    for (const FilePath &path : pathsToDelete)
        path.removeRecursively();

    emit configurationCleared();
}

void CMakeBuildSystem::combineScanAndParse(bool restoredFromBackup)
{
    if (buildConfiguration()->isActive()) {
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
    return Utils::transform(Utils::filtered(cmakeFiles,
                                            [](const CMakeFileInfo &info) {
                                                return !info.isGenerated;
                                            }),
                            [](const CMakeFileInfo &info) { return info.path; });
}

void CMakeBuildSystem::updateProjectData()
{
    qCDebug(cmakeBuildSystemLog) << "Updating CMake project data";

    QTC_ASSERT(m_treeScanner.isFinished() && !m_reader.isParsing(), return );

    buildConfiguration()->project()->setExtraProjectFiles(projectFilesToWatch(m_cmakeFiles));

    CMakeConfig patchedConfig = configurationFromCMake();
    {
        QSet<QString> res;
        QStringList apps;
        for (const auto &target : std::as_const(m_buildTargets)) {
            if (target.targetType == DynamicLibraryType) {
                res.insert(target.executable.parentDir().toString());
                apps.push_back(target.executable.toUserOutput());
            }
            // ### shall we add also the ExecutableType ?
        }
        {
            CMakeConfigItem paths;
            paths.key = Android::Constants::ANDROID_SO_LIBS_PATHS;
            paths.values = Utils::toList(res);
            patchedConfig.append(paths);
        }

        apps.sort();
        {
            CMakeConfigItem appsPaths;
            appsPaths.key = "TARGETS_BUILD_PATH";
            appsPaths.values = apps;
            patchedConfig.append(appsPaths);
        }
    }

    Project *p = project();
    {
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

    QtMajorVersion qtVersion = kitInfo.projectPartQtVersion;
    if (qtVersion == QtMajorVersion::None)
        qtVersion = qtVersionFromCMake({{QtMajorVersion::Qt6, {"Qt6", "Qt6Core"}},
                                        {QtMajorVersion::Qt5, {"Qt5", "Qt5Core"}},
                                        {QtMajorVersion::Qt4, {"Qt4", "Qt4Core"}}
                                       });

    QString errorMessage;
    RawProjectParts rpps = m_reader.createRawProjectParts(errorMessage);
    if (!errorMessage.isEmpty())
        setError(errorMessage);
    qCDebug(cmakeBuildSystemLog) << "Raw project parts created." << errorMessage;

    for (RawProjectPart &rpp : rpps) {
        rpp.setQtVersion(qtVersion); // TODO: Check if project actually uses Qt.
        const FilePath includeFileBaseDir = buildConfiguration()->buildDirectory();
        QStringList cxxFlags = rpp.flagsForCxx.commandLineFlags;
        QStringList cFlags = rpp.flagsForC.commandLineFlags;
        addTargetFlagForIos(cFlags, cxxFlags, this, [this] {
            return m_configurationFromCMake.stringValueOf("CMAKE_OSX_DEPLOYMENT_TARGET");
        });
        if (kitInfo.cxxToolChain)
            rpp.setFlagsForCxx({kitInfo.cxxToolChain, cxxFlags, includeFileBaseDir});
        if (kitInfo.cToolChain)
            rpp.setFlagsForC({kitInfo.cToolChain, cFlags, includeFileBaseDir});
    }

    m_cppCodeModelUpdater->update({p, kitInfo, buildConfiguration()->environment(), rpps},
                                  m_extraCompilers);

    {
        const bool mergedHeaderPathsAndQmlImportPaths = kit()->value(
                    QtSupport::Constants::KIT_HAS_MERGED_HEADER_PATHS_WITH_QML_IMPORT_PATHS, false).toBool();
        QStringList extraHeaderPaths;
        QList<QByteArray> moduleMappings;
        for (const RawProjectPart &rpp : std::as_const(rpps)) {
            FilePath moduleMapFile = buildConfiguration()->buildDirectory()
                    .pathAppended("qml_module_mappings/" + rpp.buildSystemTarget);
            if (expected_str<QByteArray> content = moduleMapFile.fileContents()) {
                auto lines = content->split('\n');
                for (const QByteArray &line : lines) {
                    if (!line.isEmpty())
                        moduleMappings.append(line.simplified());
                }
            }

            if (mergedHeaderPathsAndQmlImportPaths) {
                for (const auto &headerPath : rpp.headerPaths) {
                    if (headerPath.type == HeaderPathType::User || headerPath.type == HeaderPathType::System)
                        extraHeaderPaths.append(headerPath.path);
                }
            }
        }
        updateQmlJSCodeModel(extraHeaderPaths, moduleMappings);
    }
    updateInitialCMakeExpandableVars();

    emit buildConfiguration()->buildTypeChanged();

    qCDebug(cmakeBuildSystemLog) << "All CMake project data up to date.";
}

void CMakeBuildSystem::handleTreeScanningFinished()
{
    TreeScanner::Result result = m_treeScanner.release();
    m_allFiles = result.folderNode;
    qDeleteAll(result.allFiles);

    updateFileSystemNodes();
}

void CMakeBuildSystem::updateFileSystemNodes()
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

    if (m_allFiles)
        addFileSystemNodes(newRoot.get(), m_allFiles);
    setRootProjectNode(std::move(newRoot));

    m_reader.resetData();

    m_currentGuard = {};
    emitBuildSystemUpdated();

    qCDebug(cmakeBuildSystemLog) << "All fallback CMake project data up to date.";
}

void CMakeBuildSystem::updateFallbackProjectData()
{
    qCDebug(cmakeBuildSystemLog) << "Updating fallback CMake project data";
    qCDebug(cmakeBuildSystemLog) << "Starting TreeScanner";
    QTC_CHECK(m_treeScanner.isFinished());
    if (m_treeScanner.asyncScanForFiles(projectDirectory()))
        Core::ProgressManager::addTask(m_treeScanner.future(),
                                       Tr::tr("Scan \"%1\" project tree")
                                           .arg(project()->displayName()),
                                       "CMake.Scan.Tree");
}

void CMakeBuildSystem::updateCMakeConfiguration(QString &errorMessage)
{
    CMakeConfig cmakeConfig = m_reader.takeParsedConfiguration(errorMessage);
    for (auto &ci : cmakeConfig)
        ci.inCMakeCache = true;
    if (!errorMessage.isEmpty()) {
        const CMakeConfig changes = configurationChanges();
        for (const auto &ci : changes) {
            if (ci.isInitial)
                continue;
            const bool haveConfigItem = Utils::contains(cmakeConfig, [ci](const CMakeConfigItem& i) {
                return i.key == ci.key;
            });
            if (!haveConfigItem)
                cmakeConfig.append(ci);
        }
    }
    setConfigurationFromCMake(cmakeConfig);
}

void CMakeBuildSystem::handleParsingSucceeded(bool restoredFromBackup)
{
    if (!buildConfiguration()->isActive()) {
        stopParsingAndClearState();
        return;
    }

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
        setupCMakeSymbolsHash();

        checkAndReportError(errorMessage);
    }

    {
        updateCMakeConfiguration(errorMessage);
        checkAndReportError(errorMessage);
    }

    if (const CMakeTool *tool = m_parameters.cmakeTool())
        m_ctestPath = tool->cmakeExecutable().withNewPath(m_reader.ctestPath());

    setApplicationTargets(appTargets());

    // Note: This is practically always wrong and resulting in an empty view.
    // Setting the real data is triggered from a successful run of a
    // MakeInstallStep.
    setDeploymentData(deploymentDataFromFile());

    QTC_ASSERT(m_waitingForParse, return );
    m_waitingForParse = false;

    combineScanAndParse(restoredFromBackup);
}

void CMakeBuildSystem::handleParsingFailed(const QString &msg)
{
    setError(msg);

    QString errorMessage;
    updateCMakeConfiguration(errorMessage);
    // ignore errorMessage here, we already got one.

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
        // Build configuration has changed:
        qCDebug(cmakeBuildSystemLog) << "Requesting parse due to active BC changed";
        reparse(CMakeBuildSystem::REPARSE_DEFAULT);
    });
    connect(project(), &Project::activeTargetChanged, this, [this] {
        // Build configuration has changed:
        qCDebug(cmakeBuildSystemLog) << "Requesting parse due to active target changed";
        reparse(CMakeBuildSystem::REPARSE_DEFAULT);
    });

    // BuildConfiguration changed:
    connect(buildConfiguration(), &BuildConfiguration::environmentChanged, this, [this] {
        // The environment on our BC has changed, force CMake run to catch up with possible changes
        qCDebug(cmakeBuildSystemLog) << "Requesting parse due to environment change";
        reparse(CMakeBuildSystem::REPARSE_FORCE_CMAKE_RUN);
    });
    connect(buildConfiguration(), &BuildConfiguration::buildDirectoryChanged, this, [this] {
        // The build directory of our BC has changed:
        // Does the directory contain a CMakeCache ? Existing build, just parse
        // No CMakeCache? Run with initial arguments!
        qCDebug(cmakeBuildSystemLog) << "Requesting parse due to build directory change";
        const BuildDirParameters parameters(this);
        const FilePath cmakeCacheTxt = parameters.buildDirectory.pathAppended("CMakeCache.txt");
        const bool hasCMakeCache = cmakeCacheTxt.exists();
        const auto options = ReparseParameters(
                    hasCMakeCache
                    ? REPARSE_DEFAULT
                    : (REPARSE_FORCE_INITIAL_CONFIGURATION | REPARSE_FORCE_CMAKE_RUN));
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
        if (buildConfiguration()->isActive() && !isParsing()) {
            if (settings().autorunCMake()) {
                qCDebug(cmakeBuildSystemLog) << "Requesting parse due to dirty project file";
                reparse(CMakeBuildSystem::REPARSE_FORCE_CMAKE_RUN);
            }
        }
    });

    // Force initial parsing run:
    if (buildConfiguration()->isActive()) {
        qCDebug(cmakeBuildSystemLog) << "Initial run:";
        reparse(CMakeBuildSystem::REPARSE_DEFAULT);
    }
}

void CMakeBuildSystem::setupCMakeSymbolsHash()
{
    m_cmakeSymbolsHash.clear();

    m_projectKeywords.functions.clear();
    m_projectKeywords.variables.clear();

    auto handleFunctionMacroOption = [&](const CMakeFileInfo &cmakeFile,
                                         const cmListFileFunction &func) {
        if (func.LowerCaseName() != "function" && func.LowerCaseName() != "macro"
            && func.LowerCaseName() != "option")
            return;

        if (func.Arguments().size() == 0)
            return;
        auto arg = func.Arguments()[0];

        Utils::Link link;
        link.targetFilePath = cmakeFile.path;
        link.targetLine = arg.Line;
        link.targetColumn = arg.Column - 1;
        m_cmakeSymbolsHash.insert(QString::fromUtf8(arg.Value), link);

        if (func.LowerCaseName() == "option")
            m_projectKeywords.variables[QString::fromUtf8(arg.Value)] = FilePath();
        else
            m_projectKeywords.functions[QString::fromUtf8(arg.Value)] = FilePath();
    };

    m_projectImportedTargets.clear();
    auto handleImportedTargets = [&](const CMakeFileInfo &cmakeFile,
                                     const cmListFileFunction &func) {
        if (func.LowerCaseName() != "add_library")
            return;

        if (func.Arguments().size() == 0)
            return;
        auto arg = func.Arguments()[0];
        const QString targetName = QString::fromUtf8(arg.Value);

        const bool haveImported = Utils::contains(func.Arguments(), [](const auto &arg) {
            return arg.Value == "IMPORTED";
        });
        if (haveImported && !targetName.contains("${")) {
            m_projectImportedTargets << targetName;

            // Allow navigation to the imported target
            Utils::Link link;
            link.targetFilePath = cmakeFile.path;
            link.targetLine = arg.Line;
            link.targetColumn = arg.Column - 1;
            m_cmakeSymbolsHash.insert(targetName, link);
        }
    };

    // Handle project targets, unfortunately the CMake file-api doesn't deliver the
    // column of the target, just the line. Make sure to find it out
    QHash<FilePath, QPair<int, QString>> projectTargetsSourceAndLine;
    for (const auto &target : std::as_const(buildTargets())) {
        if (target.targetType == TargetType::UtilityType)
            continue;
        if (target.backtrace.isEmpty())
            continue;

        projectTargetsSourceAndLine.insert(target.backtrace.last().path,
                                           {target.backtrace.last().line, target.title});
    }
    auto handleProjectTargets = [&](const CMakeFileInfo &cmakeFile, const cmListFileFunction &func) {
        const auto it = projectTargetsSourceAndLine.find(cmakeFile.path);
        if (it == projectTargetsSourceAndLine.end() || it->first != func.Line())
            return;

        if (func.Arguments().size() == 0)
            return;
        auto arg = func.Arguments()[0];

        Utils::Link link;
        link.targetFilePath = cmakeFile.path;
        link.targetLine = arg.Line;
        link.targetColumn = arg.Column - 1;
        m_cmakeSymbolsHash.insert(it->second, link);
    };

    // Gather the exported variables for the Find<Package> CMake packages
    m_projectFindPackageVariables.clear();

    const std::string fphsFunctionName = "find_package_handle_standard_args";
    CMakeKeywords keywords;
    if (auto tool = CMakeKitAspect::cmakeTool(target()->kit()))
        keywords = tool->keywords();
    QSet<std::string> fphsFunctionArgs;
    if (keywords.functionArgs.contains(QString::fromStdString(fphsFunctionName))) {
        const QList<std::string> args
            = Utils::transform(keywords.functionArgs.value(QString::fromStdString(fphsFunctionName)),
                               &QString::toStdString);
        fphsFunctionArgs = Utils::toSet(args);
    }

    auto handleFindPackageVariables = [&](const CMakeFileInfo &cmakeFile, const cmListFileFunction &func) {
        if (func.LowerCaseName() != fphsFunctionName)
            return;

        if (func.Arguments().size() == 0)
            return;
        auto firstArgument = func.Arguments()[0];
        const auto filteredArguments = Utils::filtered(func.Arguments(), [&](const auto &arg) {
            return !fphsFunctionArgs.contains(arg.Value) && arg != firstArgument;
        });

        for (const auto &arg : filteredArguments) {
            const QString value = QString::fromUtf8(arg.Value);
            if (value.contains("${") || (value.startsWith('"') && value.endsWith('"'))
                || (value.startsWith("'") && value.endsWith("'")))
                continue;

            m_projectFindPackageVariables << value;

            Utils::Link link;
            link.targetFilePath = cmakeFile.path;
            link.targetLine = arg.Line;
            link.targetColumn = arg.Column - 1;
            m_cmakeSymbolsHash.insert(value, link);
        }
    };

    // Prepare a hash with all .cmake files
    m_dotCMakeFilesHash.clear();
    auto handleDotCMakeFiles = [&](const CMakeFileInfo &cmakeFile) {
        if (cmakeFile.path.suffix() == "cmake") {
            Utils::Link link;
            link.targetFilePath = cmakeFile.path;
            link.targetLine = 1;
            link.targetColumn = 0;
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
            link.targetLine = 1;
            link.targetColumn = 0;
            m_findPackagesFilesHash.insert(!findPackageName.isEmpty() ? findPackageName
                                                                      : configPackageName,
                                           link);
        }
    };

    for (const auto &cmakeFile : std::as_const(m_cmakeFiles)) {
        for (const auto &func : cmakeFile.cmakeListFile.Functions) {
            handleFunctionMacroOption(cmakeFile, func);
            handleImportedTargets(cmakeFile, func);
            handleProjectTargets(cmakeFile, func);
            handleFindPackageVariables(cmakeFile, func);
        }
        handleDotCMakeFiles(cmakeFile);
        handleFindPackageCMakeFiles(cmakeFile);
    }

    m_projectFindPackageVariables.removeDuplicates();
}

void CMakeBuildSystem::ensureBuildDirectory(const BuildDirParameters &parameters)
{
    const FilePath bdir = parameters.buildDirectory;

    if (!buildConfiguration()->createBuildDirectory()) {
        handleParsingFailed(Tr::tr("Failed to create build directory \"%1\".").arg(bdir.toUserOutput()));
        return;
    }

    const CMakeTool *tool = parameters.cmakeTool();
    if (!tool) {
        handleParsingFailed(Tr::tr("No CMake tool set up in kit."));
        return;
    }

    if (tool->cmakeExecutable().needsDevice()) {
        if (!tool->cmakeExecutable().ensureReachable(bdir)) {
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
            const QJsonDocument json
                = QJsonDocument::fromJson(m_ctestProcess->readAllRawStandardOutput());
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

static FilePaths librarySearchPaths(const CMakeBuildSystem *bs, const QString &buildKey)
{
    const CMakeBuildTarget cmakeBuildTarget
        = Utils::findOrDefault(bs->buildTargets(), Utils::equal(&CMakeBuildTarget::title, buildKey));

    return cmakeBuildTarget.libraryDirectories;
}

const QList<BuildTargetInfo> CMakeBuildSystem::appTargets() const
{
    QList<BuildTargetInfo> appTargetList;
    const bool forAndroid = DeviceTypeKitAspect::deviceTypeId(kit())
                            == Android::Constants::ANDROID_DEVICE_TYPE;
    for (const CMakeBuildTarget &ct : m_buildTargets) {
        if (CMakeBuildSystem::filteredOutTarget(ct))
            continue;

        if (ct.targetType == ExecutableType || (forAndroid && ct.targetType == DynamicLibraryType)) {
            const QString buildKey = ct.title;

            BuildTargetInfo bti;
            bti.displayName = ct.title;
            bti.targetFilePath = ct.executable;
            bti.projectFilePath = ct.sourceDirectory.cleanPath();
            bti.workingDirectory = ct.workingDirectory;
            bti.buildKey = buildKey;
            bti.usesTerminal = !ct.linksToQtGui;
            bti.isQtcRunnable = ct.qtcRunnable;

            // Workaround for QTCREATORBUG-19354:
            bti.runEnvModifier = [this, buildKey](Environment &env, bool enabled) {
                if (enabled)
                    env.prependOrSetLibrarySearchPaths(librarySearchPaths(this, buildKey));
            };

            appTargetList.append(bti);
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

CommandLine CMakeBuildSystem::commandLineForTests(const QList<QString> &tests,
                                                  const QStringList &options) const
{
    QStringList args = options;
    const QSet<QString> testsSet = Utils::toSet(tests);
    auto current = Utils::transform<QSet<QString>>(m_testNames, &TestCaseInfo::name);
    if (tests.isEmpty() || current == testsSet)
        return {m_ctestPath, args};

    QString testNumbers("0,0,0"); // start, end, stride
    for (const TestCaseInfo &info : m_testNames) {
        if (testsSet.contains(info.name))
            testNumbers += QString(",%1").arg(info.number);
    }
    args << "-I" << testNumbers;
    return {m_ctestPath, args};
}

DeploymentData CMakeBuildSystem::deploymentDataFromFile() const
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
        return result;

    deploymentPrefix = result.addFilesFromDeploymentFile(deploymentFilePath, sourceDir);
    for (const CMakeBuildTarget &ct : m_buildTargets) {
        if (ct.targetType == ExecutableType || ct.targetType == DynamicLibraryType) {
            if (!ct.executable.isEmpty()
                    && result.deployableForLocalFile(ct.executable).localFilePath() != ct.executable) {
                result.addFile(ct.executable,
                               deploymentPrefix + buildDir.relativeChildPath(ct.executable).toString(),
                               DeployableFile::TypeExecutable);
            }
        }
    }

    return result;
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

void CMakeBuildSystem::updateQmlJSCodeModel(const QStringList &extraHeaderPaths,
                                            const QList<QByteArray> &moduleMappings)
{
    QmlJS::ModelManagerInterface *modelManager = QmlJS::ModelManagerInterface::instance();

    if (!modelManager)
        return;

    Project *p = project();
    QmlJS::ModelManagerInterface::ProjectInfo projectInfo
        = modelManager->defaultProjectInfoForProject(p, p->files(Project::HiddenRccFolders));

    projectInfo.importPaths.clear();

    auto addImports = [&projectInfo](const QString &imports) {
        const QStringList importList = CMakeConfigItem::cmakeSplitValue(imports);
        for (const QString &import : importList)
            projectInfo.importPaths.maybeInsert(FilePath::fromUserInput(import), QmlJS::Dialect::Qml);
    };

    const CMakeConfig &cm = configurationFromCMake();
    addImports(cm.stringValueOf("QML_IMPORT_PATH"));
    addImports(kit()->value(QtSupport::Constants::KIT_QML_IMPORT_PATH).toString());

    for (const QString &extraHeaderPath : extraHeaderPaths)
        projectInfo.importPaths.maybeInsert(FilePath::fromString(extraHeaderPath),
                                            QmlJS::Dialect::Qml);

    for (const QByteArray &mm : moduleMappings) {
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
    modelManager->updateProjectInfo(projectInfo, p);
}

void CMakeBuildSystem::updateInitialCMakeExpandableVars()
{
    const CMakeConfig &cm = configurationFromCMake();
    const CMakeConfig &initialConfig =
        cmakeBuildConfiguration()->initialCMakeArguments.cmakeConfiguration();

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

                config << item;
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

                    config << item;
                }
            }
        }
    }

    if (!config.isEmpty())
        emit configurationChanged(config);
}

MakeInstallCommand CMakeBuildSystem::makeInstallCommand(const FilePath &installRoot) const
{
    MakeInstallCommand cmd;
    if (CMakeTool *tool = CMakeKitAspect::cmakeTool(target()->kit()))
        cmd.command.setExecutable(tool->cmakeExecutable());

    QString installTarget = "install";
    if (usesAllCapsTargets())
        installTarget = "INSTALL";

    FilePath buildDirectory = ".";
    if (auto bc = buildConfiguration())
        buildDirectory = bc->buildDirectory();

    cmd.command.addArg("--build");
    cmd.command.addArg(buildDirectory.path());
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
    const CMakeTool * const cmakeTool
            = CMakeKitAspect::cmakeTool(buildConfiguration()->target()->kit());
    if (!cmakeTool)
        return {};
    QList<QPair<Id, QString>> result;
    const QList<CMakeTool::Generator> &generators = cmakeTool->supportedGenerators();
    for (const CMakeTool::Generator &generator : generators) {
        result << qMakePair(Id::fromSetting(generator.name),
                            Tr::tr("%1 (via cmake)").arg(generator.name));
    }
    return result;
}

void CMakeBuildSystem::runGenerator(Id id)
{
    QTC_ASSERT(cmakeBuildConfiguration(), return);
    const auto showError = [](const QString &detail) {
        Core::MessageManager::writeDisrupting(
            addCMakePrefix(Tr::tr("cmake generator failed: %1.").arg(detail)));
    };
    const CMakeTool * const cmakeTool
            = CMakeKitAspect::cmakeTool(buildConfiguration()->target()->kit());
    if (!cmakeTool) {
        showError(Tr::tr("Kit does not have a cmake binary set."));
        return;
    }
    const QString generator = id.toSetting().toString();
    const FilePath outDir = buildConfiguration()->buildDirectory()
            / ("qtc_" + FileUtils::fileSystemFriendlyName(generator));
    if (!outDir.ensureWritableDir()) {
        showError(Tr::tr("Cannot create output directory \"%1\".").arg(outDir.toString()));
        return;
    }
    CommandLine cmdLine(cmakeTool->cmakeExecutable(), {"-S", buildConfiguration()->target()
                        ->project()->projectDirectory().toUserOutput(), "-G", generator});
    if (!cmdLine.executable().isExecutableFile()) {
        showError(Tr::tr("No valid cmake executable."));
        return;
    }
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
