// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "fileapireader.h"

#include "cmakeautogenparser.h"
#include "cmakeoutputparser.h"
#include "cmakeprojectconstants.h"
#include "cmakeprojectmanagertr.h"
#include "cmakespecificsettings.h"
#include "cmaketoolmanager.h"
#include "cmakeutils.h"
#include "fileapidataextractor.h"
#include "fileapiparser.h"

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/outputwindow.h>
#include <coreplugin/progressmanager/processprogress.h>

#include <projectexplorer/buildsystem.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/taskhub.h>

#include <utils/algorithm.h>
#include <utils/async.h>
#include <utils/fileutils.h>
#include <utils/outputformat.h>
#include <utils/outputformatter.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/stringutils.h>
#include <utils/temporarydirectory.h>

#include <QElapsedTimer>
#include <QGuiApplication>
#include <QLoggingCategory>

#ifdef WITH_TESTS
#include <QFile>
#include <QTemporaryDir>
#include <QTest>
#endif

using namespace ProjectExplorer;
using namespace QtTaskTree;
using namespace Utils;

namespace CMakeProjectManager::Internal {

static Q_LOGGING_CATEGORY(cmakeFileApiMode, "qtc.cmake.fileApiMode", QtWarningMsg);

using namespace FileApiDetails;


static QString stripTrailingNewline(QString str)
{
    if (str.endsWith('\n'))
        str.chop(1);
    return str;
}

/*!
    Brings the copy of the \c cmake-helper directory in \a localDirectory in
    line with the one the running \QC ships in \a ideDirectory.

    A build directory keeps its copy, so one configured by an earlier \QC would
    go on including the helpers of that version, and a helper file added since
    would never arrive at all. The copy belongs to the IDE rather than to the
    project, so whatever differs from what is installed is replaced.

    Returns what could not be copied and leaves the rest of the directory as it
    was: a missing helper costs a feature, while stopping the configure over it
    would cost the project.
*/
static QStringList updateCMakeHelperDirectory(const FilePath &ideDirectory,
                                              const FilePath &localDirectory)
{
    QStringList errors;
    ideDirectory.iterateDirectory(
        [&](const FilePath &source) {
            const Result<QByteArray> installed = source.fileContents();
            if (!installed) {
                errors << installed.error();
                return IterationPolicy::Continue;
            }
            const FilePath target
                = localDirectory.resolvePath(source.relativePathFromDir(ideDirectory));
            if (!target.isFile()) {
                if (const Result<> created = target.parentDir().ensureWritableDir(); !created) {
                    errors << created.error();
                    return IterationPolicy::Continue;
                }
                // The contents are written rather than the file copied, which
                // would bring the permissions of the installation along: a
                // read-only one leaves a copy that the next configure cannot
                // replace.
                if (const Result<qint64> written = target.writeFileContents(*installed); !written)
                    errors << written.error();
                return IterationPolicy::Continue;
            }
            if (const Result<QByteArray> local = target.fileContents();
                local && *local == *installed) {
                return IterationPolicy::Continue;
            }
            // The helper that is there keeps its place until there is one to
            // take it: the project includes these files, so a configure
            // survives a stale one and stops over a missing one.
            const FilePath fresh = target.stringAppended(".new");
            if (const Result<qint64> written = fresh.writeFileContents(*installed); !written) {
                errors << written.error();
                return IterationPolicy::Continue;
            }
            if (const Result<> removed = target.removeFile(); !removed) {
                errors << removed.error();
                fresh.removeFile();
                return IterationPolicy::Continue;
            }
            if (const Result<> renamed = fresh.renameFile(target); !renamed)
                errors << renamed.error();
            return IterationPolicy::Continue;
        },
        {{}, DirFilterFlag::Files, DirIteratorFlag::Subdirectories});

    return errors;
}

#ifdef WITH_TESTS
class CMakeHelperDirectoryTest final : public QObject
{
    Q_OBJECT

private slots:
    void testUpdatesWhatDiffers()
    {
        QTemporaryDir temporaryDir;
        QVERIFY(temporaryDir.isValid());
        const FilePath directory = FilePath::fromString(temporaryDir.path());
        const FilePath ideDirectory = directory / "ide";
        const FilePath localDirectory = directory / "local";

        const auto writeFile = [](const FilePath &file, const QByteArray &contents) {
            return file.parentDir().ensureWritableDir() && file.writeFileContents(contents);
        };
        QVERIFY(writeFile(ideDirectory / "unchanged.cmake", "same\n"));
        QVERIFY(writeFile(ideDirectory / "changed.cmake", "new\n"));
        QVERIFY(writeFile(ideDirectory / "added.cmake", "added\n"));
        QVERIFY(writeFile(ideDirectory / "sub" / "nested.cmake", "nested\n"));
        QVERIFY(writeFile(localDirectory / "unchanged.cmake", "same\n"));
        QVERIFY(writeFile(localDirectory / "changed.cmake", "old\n"));
        QVERIFY(writeFile(localDirectory / "ownedByTheProject.cmake", "kept\n"));

        // A file of an earlier configure is older than the ones of this run,
        // which is what says whether it was written again.
        const QDateTime before = QDateTime::currentDateTime().addSecs(-3600);
        QFile unchanged((localDirectory / "unchanged.cmake").toFSPathString());
        QVERIFY(unchanged.open(QIODevice::ReadWrite));
        QVERIFY(unchanged.setFileTime(before, QFileDevice::FileModificationTime));
        unchanged.close();

        QCOMPARE(updateCMakeHelperDirectory(ideDirectory, localDirectory), QStringList());

        QCOMPARE((localDirectory / "unchanged.cmake").lastModified(), before);
        QCOMPARE((localDirectory / "changed.cmake").fileContents().value_or(QByteArray()),
                 QByteArray("new\n"));
        QCOMPARE((localDirectory / "added.cmake").fileContents().value_or(QByteArray()),
                 QByteArray("added\n"));
        QCOMPARE((localDirectory / "sub" / "nested.cmake").fileContents().value_or(QByteArray()),
                 QByteArray("nested\n"));
        // What the IDE does not ship is none of its business.
        QVERIFY((localDirectory / "ownedByTheProject.cmake").isFile());
    }

    void testReportsWhatItCouldNotWrite()
    {
        QTemporaryDir temporaryDir;
        QVERIFY(temporaryDir.isValid());
        const FilePath directory = FilePath::fromString(temporaryDir.path());
        const FilePath ideDirectory = directory / "ide";
        const FilePath localDirectory = directory / "local";

        QVERIFY((ideDirectory / "blocked.cmake").parentDir().ensureWritableDir());
        QVERIFY((ideDirectory / "blocked.cmake").writeFileContents("blocked\n"));
        QVERIFY((ideDirectory / "copied.cmake").writeFileContents("copied\n"));
        // A directory where a file belongs is not something a copy gets past.
        QVERIFY((localDirectory / "blocked.cmake").ensureWritableDir());

        const QStringList errors = updateCMakeHelperDirectory(ideDirectory, localDirectory);

        QCOMPARE(errors.size(), 1);
        // The one that could not be written stops neither the others nor the
        // configure that follows.
        QCOMPARE((localDirectory / "copied.cmake").fileContents().value_or(QByteArray()),
                 QByteArray("copied\n"));
    }

    void testLeavesTheInstallationBehind()
    {
        QTemporaryDir temporaryDir;
        QVERIFY(temporaryDir.isValid());
        const FilePath directory = FilePath::fromString(temporaryDir.path());
        const FilePath ideDirectory = directory / "ide";
        const FilePath localDirectory = directory / "local";

        const FilePath installed = ideDirectory / "helper.cmake";
        QVERIFY(installed.parentDir().ensureWritableDir());
        QVERIFY(installed.writeFileContents("helper\n"));
        QVERIFY(installed.setPermissions(QFile::ReadOwner | QFile::ReadGroup | QFile::ReadOther));

        QCOMPARE(updateCMakeHelperDirectory(ideDirectory, localDirectory), QStringList());

        // An installation can be read-only, and what is written into a build
        // directory has to stay the IDE's to replace on the next configure.
        const FilePath local = localDirectory / "helper.cmake";
        QCOMPARE(local.fileContents().value_or(QByteArray()), QByteArray("helper\n"));
        QVERIFY(local.isWritableFile());
    }

    void testKeepsWhatItCannotReplace()
    {
        QTemporaryDir temporaryDir;
        QVERIFY(temporaryDir.isValid());
        const FilePath directory = FilePath::fromString(temporaryDir.path());
        const FilePath ideDirectory = directory / "ide";
        const FilePath localDirectory = directory / "local";

        QVERIFY((ideDirectory / "helper.cmake").parentDir().ensureWritableDir());
        QVERIFY((ideDirectory / "helper.cmake").writeFileContents("new\n"));
        QVERIFY((localDirectory / "helper.cmake").parentDir().ensureWritableDir());
        QVERIFY((localDirectory / "helper.cmake").writeFileContents("old\n"));
        // Nothing can be written where the new contents go.
        QVERIFY((localDirectory / "helper.cmake.new").ensureWritableDir());

        const QStringList errors = updateCMakeHelperDirectory(ideDirectory, localDirectory);

        // The helper of the earlier configure is still there. A file the
        // project includes and that is gone stops the configure, which is
        // what the one that could not be replaced would have survived.
        QCOMPARE(errors.size(), 1);
        QCOMPARE((localDirectory / "helper.cmake").fileContents().value_or(QByteArray()),
                 QByteArray("old\n"));
    }
};

QObject *createCMakeHelperDirectoryTest()
{
    return new CMakeHelperDirectoryTest;
}
#endif

static void doParse(QPromise<FileApiQtcData> &promise,
                    const FilePath &replyFilePath,
                    const FilePath &sourceDirectory,
                    const FilePath &buildDirectory,
                    const QString &cmakeBuildType,
                    const QHash<FilePath, FilePaths> &projectHeaders)
{
    FileApiQtcData result;
    const QFuture<void> canceler(promise.future());
    FileApiData data = FileApiParser::parseData(canceler,
                                                replyFilePath,
                                                buildDirectory,
                                                cmakeBuildType,
                                                result.errorMessage);
    if (result.errorMessage.isEmpty()) {
        result = extractData(canceler, data, sourceDirectory, buildDirectory, projectHeaders);
    } else {
        qWarning() << result.errorMessage;
        result.cache = std::move(data.cache);
    }
    promise.addResult(std::move(result));
}

// --------------------------------------------------------------------
// FileApiReader:
// --------------------------------------------------------------------
FileApiReader::~FileApiReader()
{
    resetData();
}

void FileApiReader::setProjectHeaders(const QHash<FilePath, FilePaths> &projectHeaders)
{
    m_projectHeaders = projectHeaders;
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
    m_data.cmakeFiles.clear();
    if (!m_parameters.sourceDirectory.isEmpty()) {
        CMakeFileInfo cmakeListsTxt;
        cmakeListsTxt.path = m_parameters.sourceDirectory.pathAppended(Constants::CMAKE_LISTS_TXT);
        cmakeListsTxt.isCMakeListsDotTxt = true;
        m_data.cmakeFiles.insert(cmakeListsTxt);
    }
    m_data.cache.clear();
    m_data.buildTargets.clear();
    m_data.projectParts.clear();
    m_data.rootProjectNode.reset();
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
    QTC_CHECK(!m_taskTreeRunner.isRunning());
    qCDebug(cmakeFileApiMode) << "FileApiReader: CONFIGURATION STARTED SIGNAL";
    emit configurationStarted();

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
    const bool hasArguments = !args.isEmpty();
    const bool replyFileMissing = !replyFile.exists();
    const bool cmakeFilesChanged = m_parameters.isValid()
                                   && cmakeSettingsForProject(m_parameters.project).autorunCMake()
                                   && anyOf(m_data.cmakeFiles, [&replyFile](const CMakeFileInfo &info) {
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

    // Snapshot parameters for capture in lambdas
    const BuildDirParameters params = m_parameters;
    const QString cmakeBuildType = params.cmakeBuildType == "Build" ? "" : params.cmakeBuildType;

    const QHash<FilePath, FilePaths> projectHeaders = m_projectHeaders;

    const auto onParseSetup = [this, params, cmakeBuildType, projectHeaders](
                                  Async<FileApiQtcData> &task) {
        const FilePath replyFilePath = FileApiParser::scanForCMakeReplyFile(params.buildDirectory);
        m_lastReplyTimestamp = replyFilePath.lastModified();
        task.setConcurrentCallData(doParse, replyFilePath, params.sourceDirectory,
                                   params.buildDirectory, cmakeBuildType, projectHeaders);
        task.setThreadPool(ProjectExplorerPlugin::sharedThreadPool());
    };

    const auto onParseDone = [this](const Async<FileApiQtcData> &task) {
        m_data = task.takeResult();
    };

    const auto onRecipeDone = [this] {
        if (m_data.errorMessage.isEmpty())
            emit dataAvailable(m_restoredFromBackup);
        else
            emit errorOccurred(m_data.errorMessage);
    };

    GroupItem updateTask = nullItem;

    if (mustUpdate) {
        qCDebug(cmakeFileApiMode) << QString("FileApiReader: Starting CMake with \"%1\".")
                                         .arg(args.join("\", \""));

        const Storage<QElapsedTimer> elapsed;
        auto outputFormatter = std::make_shared<OutputFormatter>();

        const auto onCMakeSetup = [this, params, args, outputFormatter, elapsed] (Process &process) {
            const FilePath cmakeExe = params.cmakeExecutable;
            const FilePath buildDir = params.buildDirectory;
            const QString mountHint = Tr::tr(
                "You may need to add the project directory to the list of directories that are "
                "mounted by the build device.");

            const auto failSetup = [this](const QStringList &lines) {
                m_lastCMakeFailed = true;
                BuildSystem::appendBuildSystemOutput(addCMakePrefix(QStringList{QString()} + lines).join('\n'));
                return SetupResult::StopWithError;
            };

            if (!cmakeExe.ensureReachable(params.sourceDirectory)) {
                return failSetup(
                    { Tr::tr("The source directory %1 is not reachable by the CMake executable %2.")
                         .arg(params.sourceDirectory.displayName(), cmakeExe.displayName()),
                     mountHint});
            }

            if (!cmakeExe.ensureReachable(params.buildDirectory)) {
                return failSetup(
                    {Tr::tr("The build directory %1 is not reachable by the CMake executable %2.")
                         .arg(params.buildDirectory.displayName(), cmakeExe.displayName()),
                     mountHint});
            }

            if (!buildDir.exists()) {
                return failSetup({Tr::tr("The build directory \"%1\" does not exist")
                                      .arg(buildDir.toUserOutput())});
            }

            if (!buildDir.isLocal() && !cmakeExe.isSameDevice(buildDir)) {
                return failSetup({Tr::tr("CMake executable \"%1\" and build directory \"%2\" must "
                                         "be on the same device.")
                                      .arg(cmakeExe.toUserOutput(), buildDir.toUserOutput())});
            }

            if (m_watcher) {
                disconnect(m_watcher.get(), &FilePathWatcher::pathChanged,
                           this, &FileApiReader::handleReplyIndexFileChange);
            }
            m_watcher.reset();

            makeBackupConfiguration(true);
            writeConfigurationIntoBuildDirectory(args);

            // Output parsers
            const auto parser = new CMakeOutputParser;
            outputFormatter->addLineParsers({new CMakeAutogenParser, parser});
            outputFormatter->addLineParsers(params.outputParsers());
            // Only reaches the formatter once the parser is registered with it.
            parser->setSourceDirectories({params.sourceDirectory, params.buildDirectory});

            // Copy cmake-helper directory
            const FilePath ideCMakeHelperDir = Core::ICore::resourcePath("cmake-helper");
            const FilePath localCMakeHelperDir = buildDir
                                                / (QString(ProjectExplorer::Constants::PROJECT_QTC_DIR)
                                                   + "/cmake-helper");
            if (!ideCMakeHelperDir.isDir()) {
                BuildSystem::appendBuildSystemOutput(
                    Tr::tr("The %1 installation is missing the \"cmake-helper\" directory. "
                           "It was expected here: \"%2\".")
                        .arg(QGuiApplication::applicationDisplayName(),
                             ideCMakeHelperDir.toUserOutput()));
            } else if (!localCMakeHelperDir.exists()) {
                const auto result = ideCMakeHelperDir.copyRecursively(localCMakeHelperDir);
                if (!result) {
                    BuildSystem::appendBuildSystemOutput(
                        addCMakePrefix(
                            {Tr::tr("Failed to copy \"cmake-helper\" folder:"), result.error()})
                            .join('\n'));
                }
            } else {
                const QStringList errors
                    = updateCMakeHelperDirectory(ideCMakeHelperDir, localCMakeHelperDir);
                if (!errors.isEmpty()) {
                    BuildSystem::appendBuildSystemOutput(
                        addCMakePrefix(
                            QStringList{Tr::tr("Failed to update the \"cmake-helper\" folder:")}
                            + errors)
                            .join('\n'));
                }
            }

            // Clean output if configured
            if (cmakeSettingsForProject(params.project).cleanOldOutput()) {
                ProjectExplorerPlugin::buildSystemOutput()->clearLinesPrefixedWith(
                    Constants::OUTPUT_PREFIX, true);
                Core::MessageManager::clearLinesPrefixedWith(Constants::OUTPUT_PREFIX, false);
            }

            // Build command line
            const FilePath sourceDir = cmakeExe.withNewMappedPath(params.sourceDirectory);
            CommandLine commandLine(cmakeExe,
                                    {"-S",
                                     CMakeToolManager::mappedFilePath(params.project, sourceDir)
                                         .path(),
                                     "-B",
                                     CMakeToolManager::mappedFilePath(params.project, buildDir)
                                         .path()});
            commandLine.addArgs(args);

            // Configure process
            process.setWorkingDirectory(buildDir);
            process.setEnvironment(params.environment);
            process.setCommand(commandLine);

            process.setStdOutLineCallback([this](const QString &s) {
                BuildSystem::appendBuildSystemOutput(addCMakePrefix(stripTrailingNewline(s)));
                if (s.endsWith("Waiting for debugger client to connect...\n"))
                    emit debuggingStarted();
            });
            process.setStdErrLineCallback([outputFormatter](const QString &s) {
                outputFormatter->appendMessage(s, StdErrFormat);
                BuildSystem::appendBuildSystemOutput(addCMakePrefix(stripTrailingNewline(s)));
            });

            TaskHub::clearTasks(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM);

            BuildSystem::startNewBuildSystemOutput(
                addCMakePrefix(Tr::tr("Running %1 in %2.")
                                   .arg(commandLine.toUserOutput(), buildDir.toUserOutput())));
            auto *progress = new Core::ProcessProgress(&process);
            progress->setDisplayName(
                Tr::tr("Configuring \"%1\"").arg(params.projectName));

            elapsed->start();

            return SetupResult::Continue;
        };

        const auto onCMakeDone = [this, outputFormatter, elapsed]
            (const Process &process, DoneWith result) {
            outputFormatter->flush();
            m_lastCMakeFailed = result != DoneWith::Success;
            if (result != DoneWith::Success) {
                makeBackupConfiguration(false);
                if (result != DoneWith::Cancel) {
                    const QString message = process.exitMessage();
                    BuildSystem::appendBuildSystemOutput(
                        addCMakePrefix({QString(), message}).join('\n'));
                    TaskHub::addTask<CMakeTask>(Task::Error, message);
                }
            }
            BuildSystem::appendBuildSystemOutput(
                addCMakePrefix({QString(), formatElapsedTime(elapsed->elapsed())}).join('\n'));
            setupCMakeFileApi();
        };

        const auto canceler = [this] { return makeObjectSignal(this, &FileApiReader::cancelCMakeRequested); };

        updateTask = Group {
            finishAllAndSuccess,
            elapsed,
            ProcessTask(onCMakeSetup, onCMakeDone).withCancel(canceler)
        };
    }

    m_restoredFromBackup = false;
    m_taskTreeRunner.start({
        updateTask,
        AsyncTask<FileApiQtcData>(onParseSetup, onParseDone, CallDoneFlag::OnSuccess)
    }, {}, onRecipeDone);
}

void FileApiReader::stop()
{
    m_taskTreeRunner.reset();
}

void FileApiReader::stopCMakeRun()
{
    emit cancelCMakeRequested();
}

bool FileApiReader::isParsing() const
{
    return m_taskTreeRunner.isRunning();
}

QList<CMakeBuildTarget> FileApiReader::takeBuildTargets(QString &errorMessage){
    Q_UNUSED(errorMessage)

    return std::exchange(m_data.buildTargets, {});
}

QSet<CMakeFileInfo> FileApiReader::takeCMakeFileInfos(QString &errorMessage)
{
    Q_UNUSED(errorMessage)

    return std::exchange(m_data.cmakeFiles, {});
}

CMakeConfig FileApiReader::takeParsedConfiguration()
{
    return std::exchange(m_data.cache, {});
}

InstallRuleDeployment FileApiReader::takeDeployment()
{
    return std::exchange(m_data.deployment, {});
}

QString FileApiReader::ctestPath() const
{
    // if we failed to run cmake we should not offer ctest information either
    return m_lastCMakeFailed ? QString() : m_data.ctestPath;
}

bool FileApiReader::isMultiConfig() const
{
    return m_data.isMultiConfig;
}

bool FileApiReader::usesAllCapsTargets() const
{
    return m_data.usesAllCapsTargets;
}

RawProjectParts FileApiReader::createRawProjectParts(QString &errorMessage)
{
    Q_UNUSED(errorMessage)

    return std::exchange(m_data.projectParts, {});
}


void FileApiReader::makeBackupConfiguration(bool store)
{
    FilePath reply = m_parameters.buildDirectory.pathAppended(".cmake/api/v1/reply");
    FilePath replyPrev = m_parameters.buildDirectory.pathAppended(".cmake/api/v1/reply.prev");
    if (!store)
        std::swap(reply, replyPrev);

    const bool existed = reply.exists();
    if (existed) {
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
    if (!store)
        m_restoredFromBackup = existed;
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
    if (replyIndexfile.isEmpty())
        return;
    Result<std::unique_ptr<FilePathWatcher>> res = replyIndexfile.watch();
    QTC_ASSERT_RESULT(res, return);

    connect(res->get(), &FilePathWatcher::pathChanged, this, &FileApiReader::handleReplyIndexFileChange);
    m_watcher = std::move(*res);
}

QString FileApiReader::cmakeGenerator() const
{
    return m_data.cmakeGenerator;
}

std::unique_ptr<CMakeProjectNode> FileApiReader::rootProjectNode()
{
    return std::exchange(m_data.rootProjectNode, {});
}

FilePath FileApiReader::topCmakeFile() const
{
    return m_data.cmakeFiles.size() == 1 ? (*m_data.cmakeFiles.begin()).path : FilePath{};
}

void FileApiReader::handleReplyIndexFileChange(const FilePath &indexFile)
{
    if (m_taskTreeRunner.isRunning())
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

#ifdef WITH_TESTS
#include <fileapireader.moc>
#endif
