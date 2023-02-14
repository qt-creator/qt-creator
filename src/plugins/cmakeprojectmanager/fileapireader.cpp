// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "fileapireader.h"

#include "cmakeprocess.h"
#include "cmakeprojectmanagertr.h"
#include "cmakeprojectplugin.h"
#include "cmakespecificsettings.h"
#include "fileapidataextractor.h"
#include "fileapiparser.h"

#include <coreplugin/messagemanager.h>

#include <projectexplorer/projectexplorer.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>
#include <utils/runextensions.h>

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
    QObject::connect(&m_watcher,
                     &FileSystemWatcher::directoryChanged,
                     this,
                     &FileApiReader::replyDirectoryHasChanged);
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

    // Reset watcher:
    m_watcher.clear();

    FileApiParser::setupCMakeFileApi(m_parameters.buildDirectory, m_watcher);

    resetData();
}

void FileApiReader::resetData()
{
    m_cmakeFiles.clear();
    if (!m_parameters.sourceDirectory.isEmpty()) {
        CMakeFileInfo cmakeListsTxt;
        cmakeListsTxt.path = m_parameters.sourceDirectory.pathAppended("CMakeLists.txt");
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
                          bool forceExtraConfiguration)
{
    qCDebug(cmakeFileApiMode) << "Parse called with arguments: ForceCMakeRun:" << forceCMakeRun
                              << " - forceConfiguration:" << forceInitialConfiguration
                              << " - forceExtraConfiguration:" << forceExtraConfiguration;
    startState();

    const QStringList args = (forceInitialConfiguration ? m_parameters.initialCMakeArguments
                                                        : QStringList())
                             + (forceExtraConfiguration
                                    ? (m_parameters.configurationChangesArguments
                                       + m_parameters.additionalCMakeArguments)
                                    : QStringList());
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
    const auto settings = CMakeSpecificSettings::instance();
    const bool cmakeFilesChanged = m_parameters.cmakeTool() && settings->autorunCMake.value()
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
        m_future->waitForFinished();
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

QSet<FilePath> FileApiReader::projectFilesToWatch() const
{
    return Utils::transform(
                Utils::filtered(m_cmakeFiles,
                                [](const CMakeFileInfo &info) { return !info.isGenerated; }),
                [](const CMakeFileInfo &info) { return info.path;});
}

QList<CMakeBuildTarget> FileApiReader::takeBuildTargets(QString &errorMessage){
    Q_UNUSED(errorMessage)

    return std::exchange(m_buildTargets, {});
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
    const QString cmakeBuildType = m_parameters.cmakeBuildType == "Build" ? "" : m_parameters.cmakeBuildType;

    m_lastReplyTimestamp = replyFilePath.lastModified();

    m_future = runAsync(ProjectExplorerPlugin::sharedThreadPool(),
                        [replyFilePath, sourceDirectory, buildDirectory, cmakeBuildType](
                            QFutureInterface<std::shared_ptr<FileApiQtcData>> &fi) {
                            auto result = std::make_shared<FileApiQtcData>();
                            FileApiData data = FileApiParser::parseData(fi,
                                                                        replyFilePath,
                                                                        cmakeBuildType,
                                                                        result->errorMessage);
                            if (result->errorMessage.isEmpty())
                                *result = extractData(data, sourceDirectory, buildDirectory);
                            else
                                qWarning() << result->errorMessage;

                            fi.reportResult(result);
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
                      m_isMultiConfig = std::move(value->isMultiConfig);
                      m_usesAllCapsTargets = std::move(value->usesAllCapsTargets);

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
            Core::MessageManager::writeFlashing(Tr::tr("Failed to rename \"%1\" to \"%2\".")
                                                    .arg(reply.toString(), replyPrev.toString()));
    }

    FilePath cmakeCacheTxt = m_parameters.buildDirectory.pathAppended("CMakeCache.txt");
    FilePath cmakeCacheTxtPrev = m_parameters.buildDirectory.pathAppended("CMakeCache.txt.prev");
    if (!store)
        std::swap(cmakeCacheTxt, cmakeCacheTxtPrev);

    if (cmakeCacheTxt.exists())
        if (!FileUtils::copyIfDifferent(cmakeCacheTxt, cmakeCacheTxtPrev))
            Core::MessageManager::writeFlashing(
                Tr::tr("Failed to copy \"%1\" to \"%2\".")
                    .arg(cmakeCacheTxt.toString(), cmakeCacheTxtPrev.toString()));
}

void FileApiReader::writeConfigurationIntoBuildDirectory(const QStringList &configurationArguments)
{
    const FilePath buildDir = m_parameters.buildDirectory;
    QTC_CHECK(buildDir.ensureWritableDir());

    QByteArray contents;
    QStringList unknownOptions;
    contents.append("# This file is managed by Qt Creator, do not edit!\n\n");
    contents.append(
        transform(CMakeConfig::fromArguments(configurationArguments, unknownOptions).toList(),
                  [](const CMakeConfigItem &item) { return item.toCMakeSetLine(nullptr); })
            .join('\n')
            .toUtf8());

    const FilePath settingsFile = buildDir / "qtcsettings.cmake";
    QTC_CHECK(settingsFile.writeFileContents(contents));
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

    qCDebug(cmakeFileApiMode) << ">>>>>> Running cmake with arguments:" << configurationArguments;
    // Reset watcher:
    m_watcher.removeFiles(m_watcher.filePaths());
    m_watcher.removeDirectories(m_watcher.directoryPaths());

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

    FileApiParser::setupCMakeFileApi(m_parameters.buildDirectory, m_watcher);

    endState(FileApiParser::scanForCMakeReplyFile(m_parameters.buildDirectory),
             m_lastCMakeExitCode != 0);
}

void FileApiReader::replyDirectoryHasChanged(const QString &directory) const
{
    if (m_isParsing)
        return; // This has been triggered by ourselves, ignore.

    const FilePath reply = FileApiParser::scanForCMakeReplyFile(m_parameters.buildDirectory);
    const FilePath dir = reply.absolutePath();
    if (dir.isEmpty())
        return; // CMake started to fill the result dir, but has not written a result file yet
    QTC_CHECK(!dir.needsDevice());
    QTC_ASSERT(dir.path() == directory, return);

    if (m_lastReplyTimestamp.isValid() && reply.lastModified() > m_lastReplyTimestamp)
        emit dirty();
}

} // CMakeProjectManager::Internal
