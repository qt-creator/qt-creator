// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "fileapireader.h"

#include "cmakeprocess.h"
#include "cmakeprojectmanagertr.h"
#include "cmakeprojectconstants.h"
#include "cmakespecificsettings.h"
#include "fileapidataextractor.h"
#include "fileapiparser.h"

#include <coreplugin/messagemanager.h>

#include <projectexplorer/projectexplorer.h>

#include <utils/algorithm.h>
#include <utils/async.h>
#include <utils/fileutils.h>
#include <utils/futuresynchronizer.h>
#include <utils/qtcassert.h>
#include <utils/temporarydirectory.h>

#include <QLoggingCategory>

using namespace ProjectExplorer;
using namespace Utils;

namespace CMakeProjectManager::Internal {

static Q_LOGGING_CATEGORY(cmakeFileApiMode, "qtc.cmake.fileApiMode", QtWarningMsg);

using namespace FileApiDetails;

// --------------------------------------------------------------------
// FileApiReader:
// --------------------------------------------------------------------

FileApiReader::FileApiReader()
    : m_lastReplyTimestamp()
{
}

FileApiReader::~FileApiReader()
{
    stop();
    resetData();
}

void FileApiReader::setParameters(const BuildDirParameters &p)
{
    qCDebug(cmakeFileApiMode)
        << "\n\n\n\n\n=============================================================\n";

    // Update:
    m_parameters = p;
    qCDebug(cmakeFileApiMode) << "Work directory:" << m_parameters.buildDirectory.toUserOutput();

    setupCMakeFileApi();

    resetData();
}

void FileApiReader::resetData()
{
    m_cmakeFiles.clear();
    if (!m_parameters.sourceDirectory.isEmpty()) {
        CMakeFileInfo cmakeListsTxt;
        cmakeListsTxt.path = m_parameters.sourceDirectory.pathAppended(Constants::CMAKE_LISTS_TXT);
        cmakeListsTxt.isCMakeListsDotTxt = true;
        m_cmakeFiles.insert(cmakeListsTxt);
    }

    m_cache.clear();
    m_buildTargets.clear();
    m_projectParts.clear();
    m_rootProjectNode.reset();
}

void FileApiReader::parse(bool forceCMakeRun,
                          bool forceInitialConfiguration,
                          bool forceExtraConfiguration,
                          bool debugging,
                          bool profiling)
{
    qCDebug(cmakeFileApiMode) << "Parse called with arguments: ForceCMakeRun:" << forceCMakeRun
                              << " - forceConfiguration:" << forceInitialConfiguration
                              << " - forceExtraConfiguration:" << forceExtraConfiguration;
    startState();

    QStringList args = (forceInitialConfiguration ? m_parameters.initialCMakeArguments
                                                  : QStringList())
                       + (forceExtraConfiguration ? m_parameters.configurationChangesArguments
                                                  : QStringList())
                       + (forceCMakeRun ? m_parameters.additionalCMakeArguments : QStringList());
    if (debugging) {
        if (TemporaryDirectory::masterDirectoryFilePath().osType() == Utils::OsType::OsTypeWindows) {
            args << "--debugger"
                 << "--debugger-pipe \\\\.\\pipe\\cmake-dap";
        } else {
            FilePath file = TemporaryDirectory::masterDirectoryFilePath() / "cmake-dap.sock";
            file.removeFile();
            args << "--debugger"
                 << "--debugger-pipe=" + file.path();
        }
    }

    if (profiling) {
        const FilePath file = TemporaryDirectory::masterDirectoryFilePath() / "cmake-profile.json";
        args << "--profiling-format=google-trace"
             << "--profiling-output=" + file.path();
    }

    qCDebug(cmakeFileApiMode) << "Parameters request these CMake arguments:" << args;

    const FilePath replyFile = FileApiParser::scanForCMakeReplyFile(m_parameters.buildDirectory);
    // Only need to update when one of the following conditions is met:
    //  * The user forces the cmake run,
    //  * The user provided arguments,
    //  * There is no reply file,
    //  * One of the cmakefiles is newer than the replyFile and the user asked
    //    for creator to run CMake as needed,
    //  * A query file is newer than the reply file
    const bool hasArguments = !args.isEmpty();
    const bool replyFileMissing = !replyFile.exists();
    const bool cmakeFilesChanged = m_parameters.cmakeTool()
                                   && settings(m_parameters.project).autorunCMake()
                                   && anyOf(m_cmakeFiles, [&replyFile](const CMakeFileInfo &info) {
                                          return !info.isGenerated
                                                 && info.path.lastModified() > replyFile.lastModified();
                                      });
    const bool queryFileChanged = anyOf(FileApiParser::cmakeQueryFilePaths(m_parameters.buildDirectory),
                                        [&replyFile](const FilePath &qf) {
                                            return qf.lastModified() > replyFile.lastModified();
                                        });

    const bool mustUpdate = forceCMakeRun || hasArguments || replyFileMissing || cmakeFilesChanged
                            || queryFileChanged;
    qCDebug(cmakeFileApiMode) << QString("Do I need to run CMake? %1 "
                                         "(force: %2 | args: %3 | missing reply: %4 | "
                                         "cmakeFilesChanged: %5 | "
                                         "queryFileChanged: %6)")
                                     .arg(mustUpdate)
                                     .arg(forceCMakeRun)
                                     .arg(hasArguments)
                                     .arg(replyFileMissing)
                                     .arg(cmakeFilesChanged)
                                     .arg(queryFileChanged);

    if (mustUpdate) {
        qCDebug(cmakeFileApiMode) << QString("FileApiReader: Starting CMake with \"%1\".")
                                         .arg(args.join("\", \""));
        startCMakeState(args);
    } else {
        endState(replyFile, false);
    }
}

void FileApiReader::stop()
{
    if (m_cmakeProcess)
        disconnect(m_cmakeProcess.get(), nullptr, this, nullptr);
    m_cmakeProcess.reset();

    if (m_future) {
        m_future->cancel();
        Utils::futureSynchronizer()->addFuture(*m_future);
    }
    m_future = {};
    m_isParsing = false;
}

void FileApiReader::stopCMakeRun()
{
    if (m_cmakeProcess)
        m_cmakeProcess->stop();
}

bool FileApiReader::isParsing() const
{
    return m_isParsing;
}

QList<CMakeBuildTarget> FileApiReader::takeBuildTargets(QString &errorMessage){
    Q_UNUSED(errorMessage)

    return std::exchange(m_buildTargets, {});
}

QSet<CMakeFileInfo> FileApiReader::takeCMakeFileInfos(QString &errorMessage)
{
    Q_UNUSED(errorMessage)

    return std::exchange(m_cmakeFiles, {});
}

CMakeConfig FileApiReader::takeParsedConfiguration(QString &errorMessage)
{
    if (m_lastCMakeExitCode != 0)
        errorMessage = Tr::tr("CMake returned error code: %1").arg(m_lastCMakeExitCode);

    return std::exchange(m_cache, {});
}

QString FileApiReader::ctestPath() const
{
    // if we failed to run cmake we should not offer ctest information either
    return m_lastCMakeExitCode == 0 ? m_ctestPath : QString();
}

bool FileApiReader::isMultiConfig() const
{
    return m_isMultiConfig;
}

bool FileApiReader::usesAllCapsTargets() const
{
    return m_usesAllCapsTargets;
}

RawProjectParts FileApiReader::createRawProjectParts(QString &errorMessage)
{
    Q_UNUSED(errorMessage)

    return std::exchange(m_projectParts, {});
}

void FileApiReader::startState()
{
    qCDebug(cmakeFileApiMode) << "FileApiReader: START STATE.";
    QTC_ASSERT(!m_isParsing, return );
    QTC_ASSERT(!m_future.has_value(), return );

    m_isParsing = true;

    qCDebug(cmakeFileApiMode) << "FileApiReader: CONFIGURATION STARTED SIGNAL";
    emit configurationStarted();
}

void FileApiReader::endState(const FilePath &replyFilePath, bool restoredFromBackup)
{
    qCDebug(cmakeFileApiMode) << "FileApiReader: END STATE.";
    QTC_ASSERT(m_isParsing, return );
    QTC_ASSERT(!m_future.has_value(), return );

    const FilePath sourceDirectory = m_parameters.sourceDirectory;
    const FilePath buildDirectory = m_parameters.buildDirectory;
    const QString cmakeBuildType = m_parameters.cmakeBuildType == "Build"
                                       ? "" : m_parameters.cmakeBuildType;

    m_lastReplyTimestamp = replyFilePath.lastModified();

    m_future = Utils::asyncRun(ProjectExplorerPlugin::sharedThreadPool(),
                        [replyFilePath, sourceDirectory, buildDirectory, cmakeBuildType](
                            QPromise<std::shared_ptr<FileApiQtcData>> &promise) {
                            auto result = std::make_shared<FileApiQtcData>();
                            FileApiData data = FileApiParser::parseData(promise,
                                                                        replyFilePath,
                                                                        buildDirectory,
                                                                        cmakeBuildType,
                                                                        result->errorMessage);
                            if (result->errorMessage.isEmpty()) {
                                *result = extractData(QFuture<void>(promise.future()), data,
                                                      sourceDirectory, buildDirectory);
                            } else {
                                qWarning() << result->errorMessage;
                                result->cache = std::move(data.cache);
                            }

                            promise.addResult(result);
                        });
    onResultReady(m_future.value(),
                  this,
                  [this, sourceDirectory, buildDirectory, restoredFromBackup](
                      const std::shared_ptr<FileApiQtcData> &value) {
                      m_isParsing = false;
                      m_cache = std::move(value->cache);
                      m_cmakeFiles = std::move(value->cmakeFiles);
                      m_buildTargets = std::move(value->buildTargets);
                      m_projectParts = std::move(value->projectParts);
                      m_rootProjectNode = std::move(value->rootProjectNode);
                      m_ctestPath = std::move(value->ctestPath);
                      m_isMultiConfig = value->isMultiConfig;
                      m_usesAllCapsTargets = value->usesAllCapsTargets;
                      m_cmakeGenerator = value->cmakeGenerator;

                      if (value->errorMessage.isEmpty()) {
                          emit this->dataAvailable(restoredFromBackup);
                      } else {
                          emit this->errorOccurred(value->errorMessage);
                      }
                      m_future = {};
                  });
}

void FileApiReader::makeBackupConfiguration(bool store)
{
    FilePath reply = m_parameters.buildDirectory.pathAppended(".cmake/api/v1/reply");
    FilePath replyPrev = m_parameters.buildDirectory.pathAppended(".cmake/api/v1/reply.prev");
    if (!store)
        std::swap(reply, replyPrev);

    if (reply.exists()) {
        if (replyPrev.exists())
            replyPrev.removeRecursively();
        QTC_CHECK(!replyPrev.exists());
        if (!reply.renameFile(replyPrev))
            Core::MessageManager::writeFlashing(
                addCMakePrefix(Tr::tr("Failed to rename \"%1\" to \"%2\".")
                                   .arg(reply.toUserOutput(), replyPrev.toUserOutput())));
    }

    FilePath cmakeCacheTxt = m_parameters.buildDirectory.pathAppended(Constants::CMAKE_CACHE_TXT);
    FilePath cmakeCacheTxtPrev = m_parameters.buildDirectory.pathAppended(Constants::CMAKE_CACHE_TXT_PREV);
    if (!store)
        std::swap(cmakeCacheTxt, cmakeCacheTxtPrev);

    if (cmakeCacheTxt.exists()) {
        if (Result<> res = FileUtils::copyIfDifferent(cmakeCacheTxt, cmakeCacheTxtPrev); !res) {
            Core::MessageManager::writeFlashing(addCMakePrefix(
                Tr::tr("Failed to copy \"%1\" to \"%2\": %3")
                    .arg(cmakeCacheTxt.toUserOutput(), cmakeCacheTxtPrev.toUserOutput(), res.error())));
        }
    }
}

void FileApiReader::writeConfigurationIntoBuildDirectory(const QStringList &configurationArguments)
{
    const FilePath buildDir = m_parameters.buildDirectory;
    QTC_ASSERT_RESULT(buildDir.ensureWritableDir(), return);

    QByteArray contents;
    QStringList unknownOptions;
    contents.append("# This file is managed by Qt Creator, do not edit!\n\n");
    contents.append(
        transform(CMakeConfig::fromArguments(configurationArguments, unknownOptions).toList(),
                  [](const CMakeConfigItem &item) { return item.toCMakeSetLine(nullptr); })
            .join('\n')
            .toUtf8());

    const FilePath settingsFile = buildDir / "qtcsettings.cmake";
    QTC_ASSERT_RESULT(settingsFile.writeFileContents(contents), return);
}

void FileApiReader::setupCMakeFileApi()
{
    FileApiParser::setupCMakeFileApi(m_parameters.buildDirectory);

    const FilePath replyIndexfile = FileApiParser::scanForCMakeReplyFile(m_parameters.buildDirectory);
    Result<std::unique_ptr<FilePathWatcher>> res = replyIndexfile.watch();
    QTC_ASSERT_RESULT(res, return);

    connect(res->get(), &FilePathWatcher::pathChanged, this, &FileApiReader::handleReplyIndexFileChange);
    m_watcher = std::move(*res);
}

QString FileApiReader::cmakeGenerator() const
{
    return m_cmakeGenerator;
}

std::unique_ptr<CMakeProjectNode> FileApiReader::rootProjectNode()
{
    return std::exchange(m_rootProjectNode, {});
}

FilePath FileApiReader::topCmakeFile() const
{
    return m_cmakeFiles.size() == 1 ? (*m_cmakeFiles.begin()).path : FilePath{};
}

int FileApiReader::lastCMakeExitCode() const
{
    return m_lastCMakeExitCode;
}

void FileApiReader::startCMakeState(const QStringList &configurationArguments)
{
    qCDebug(cmakeFileApiMode) << "FileApiReader: START CMAKE STATE.";
    QTC_ASSERT(!m_cmakeProcess, return );

    m_cmakeProcess = std::make_unique<CMakeProcess>();

    connect(m_cmakeProcess.get(), &CMakeProcess::finished, this, &FileApiReader::cmakeFinishedState);
    connect(m_cmakeProcess.get(), &CMakeProcess::stdOutReady, this, [this](const QString &data) {
        if (data.endsWith("Waiting for debugger client to connect...\n"))
            emit debuggingStarted();
    });

    qCDebug(cmakeFileApiMode) << ">>>>>> Running cmake with arguments:" << configurationArguments;
    // Reset watcher:
    disconnect(m_watcher.get(), &FilePathWatcher::pathChanged, this, &FileApiReader::handleReplyIndexFileChange);
    m_watcher.reset();

    makeBackupConfiguration(true);
    writeConfigurationIntoBuildDirectory(configurationArguments);
    m_cmakeProcess->run(m_parameters, configurationArguments);
}

void FileApiReader::cmakeFinishedState(int exitCode)
{
    qCDebug(cmakeFileApiMode) << "FileApiReader: CMAKE FINISHED STATE.";

    m_lastCMakeExitCode = exitCode;
    m_cmakeProcess.release()->deleteLater();

    if (m_lastCMakeExitCode != 0)
        makeBackupConfiguration(false);

    setupCMakeFileApi();

    endState(FileApiParser::scanForCMakeReplyFile(m_parameters.buildDirectory),
             m_lastCMakeExitCode != 0);
}

void FileApiReader::handleReplyIndexFileChange(const FilePath &indexFile)
{
    if (m_isParsing)
        return; // This has been triggered by ourselves, ignore.

    const FilePath reply = FileApiParser::scanForCMakeReplyFile(m_parameters.buildDirectory);
    const FilePath dir = reply.absolutePath();
    if (dir.isEmpty())
        return; // CMake started to fill the result dir, but has not written a result file yet
    QTC_CHECK(dir.isLocal());
    QTC_ASSERT(dir == indexFile.parentDir(), return);

    if (m_lastReplyTimestamp.isValid() && reply.lastModified() > m_lastReplyTimestamp) {
        m_lastReplyTimestamp = reply.lastModified();
        emit dirty();
    }
}

} // CMakeProjectManager::Internal
