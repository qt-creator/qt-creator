// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "compilationdbparser.h"

#include "compilationdatabaseconstants.h"
#include "compilationdatabaseprojectmanagertr.h"

#include <coreplugin/progressmanager/progressmanager.h>

#include <projectexplorer/treescanner.h>

#include <utils/async.h>
#include <utils/futuresynchronizer.h>
#include <utils/mimeutils.h>

#include <QCryptographicHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <vector>

using namespace ProjectExplorer;
using namespace Tasking;
using namespace Utils;

namespace CompilationDatabaseProjectManager::Internal {

static QStringList jsonObjectFlags(const QJsonObject &object, QSet<QString> &flagsCache)
{
    QStringList flags;
    const QJsonArray arguments = object["arguments"].toArray();
    if (arguments.isEmpty()) {
        flags = splitCommandLine(object["command"].toString(), flagsCache);
    } else {
        flags.reserve(arguments.size());
        for (const QJsonValue &arg : arguments) {
            auto flagIt = flagsCache.insert(arg.toString());
            flags.append(*flagIt);
        }
    }
    return flags;
}

static FilePath jsonObjectFilePath(const QJsonObject &object)
{
    const FilePath workingDir = FilePath::fromUserInput(object["directory"].toString());
    return workingDir.resolvePath(object["file"].toString());
}

static std::vector<DbEntry> readJsonObjects(const QByteArray &projectFileContents)
{
    std::vector<DbEntry> result;

    int objectStart = projectFileContents.indexOf('{');
    int objectEnd = projectFileContents.indexOf('}', objectStart + 1);

    QSet<QString> flagsCache;
    while (objectStart >= 0 && objectEnd >= 0) {
        const QJsonDocument document = QJsonDocument::fromJson(
            projectFileContents.mid(objectStart, objectEnd - objectStart + 1));
        if (document.isNull()) {
            // The end was found incorrectly, search for the next one.
            objectEnd = projectFileContents.indexOf('}', objectEnd + 1);
            continue;
        }

        const QJsonObject object = document.object();
        const FilePath filePath = jsonObjectFilePath(object);
        const QStringList flags = filterFromFileName(jsonObjectFlags(object, flagsCache),
                                                     filePath.fileName());
        result.push_back({flags, filePath, FilePath::fromUserInput(object["directory"].toString())});

        objectStart = projectFileContents.indexOf('{', objectEnd + 1);
        objectEnd = projectFileContents.indexOf('}', objectStart + 1);
    }
    return result;
}

static QStringList readExtraFiles(const FilePath &filePath)
{
    QStringList result;

    QFile file(filePath.toFSPathString());
    if (file.open(QFile::ReadOnly)) {
        QTextStream stream(&file);
        while (!stream.atEnd()) {
            const QString line = stream.readLine().trimmed();
            if (line.isEmpty() || line.startsWith('#'))
                continue;

            result.push_back(line);
        }
    }
    return result;
}

static DbContents parseProject(const QByteArray &projectFileContents,
                               const FilePath &projectFilePath)
{
    DbContents dbContents;
    dbContents.entries = readJsonObjects(projectFileContents);
    dbContents.extraFileName = projectFilePath.stringAppended(
        Constants::COMPILATIONDATABASEPROJECT_FILES_SUFFIX);
    dbContents.extras = readExtraFiles(dbContents.extraFileName);
    std::sort(dbContents.entries.begin(), dbContents.entries.end(),
              [](const DbEntry &lhs, const DbEntry &rhs) {
                  return std::lexicographical_compare(lhs.flags.begin(), lhs.flags.end(),
                                                      rhs.flags.begin(), rhs.flags.end());
              });
    return dbContents;
}

CompilationDbParser::CompilationDbParser(const QString &projectName,
                                         const FilePath &projectPath,
                                         const FilePath &rootPath,
                                         MimeBinaryCache &mimeBinaryCache,
                                         BuildSystem::ParseGuard &&guard,
                                         QObject *parent)
    : QObject(parent)
    , m_projectName(projectName)
    , m_projectFilePath(projectPath)
    , m_rootPath(rootPath)
    , m_mimeBinaryCache(mimeBinaryCache)
    , m_guard(std::move(guard))
{}

void CompilationDbParser::start()
{
    // Check hash first.
    Result<QByteArray> fileContents = m_projectFilePath.fileContents();
    if (!fileContents) {
        finish(ParseResult::Failure);
        return;
    }

    m_projectFileContents = *std::move(fileContents);
    const QByteArray newHash
        = QCryptographicHash::hash(m_projectFileContents, QCryptographicHash::Sha1);
    if (m_projectFileHash == newHash) {
        finish(ParseResult::Cached);
        return;
    }
    m_projectFileHash = newHash;

    GroupItem treeScannerTask = nullItem;

    if (!m_rootPath.isEmpty()) {
        using ResultType = TreeScanner::Result;
        const auto onSetup = [this](Async<ResultType> &task) {
            const auto filter = [this](const MimeType &mimeType, const FilePath &fn) {
                if (fn.isDir())
                    return false;
                // Mime checks requires more resources, so keep it last in check list
                bool isIgnored = fn.startsWith(m_projectFilePath.path() + ".user")
                                 || TreeScanner::isWellKnownBinary(mimeType, fn);

                // Cache mime check result for speed up
                if (!isIgnored) {
                    if (auto it = m_mimeBinaryCache.get<std::optional<bool>>(
                            [mimeType](const QHash<QString, bool> &cache) -> std::optional<bool> {
                                const auto cache_it = cache.find(mimeType.name());
                                if (cache_it != cache.end())
                                    return *cache_it;
                                return {};
                            })) {
                        isIgnored = *it;
                    } else {
                        isIgnored = TreeScanner::isMimeBinary(mimeType, fn);
                        m_mimeBinaryCache.writeLocked()->insert(mimeType.name(), isIgnored);
                    }
                }
                return isIgnored;
            };
            task.setConcurrentCallData(&TreeScanner::scanForFiles, m_rootPath, filter,
                                       QDir::AllEntries | QDir::NoDotAndDotDot,
                                       &TreeScanner::genericFileType);
            task.setFutureSynchronizer(nullptr); // Because of m_mimeBinaryCache usage.
            QObject::connect(&task, &AsyncBase::started, this, [this, taskPtr = &task] {
                Core::ProgressManager::addTask(taskPtr->future(),
                                       Tr::tr("Scan \"%1\" project tree").arg(m_projectName),
                                       "CompilationDatabase.Scan.Tree");
            });
        };
        const auto onDone = [this](const Async<ResultType> &task) {
            if (task.isResultAvailable())
                m_scannedFiles = task.result().takeAllFiles();
        };
        treeScannerTask = AsyncTask<ResultType>(onSetup, onDone);
    }

    const auto onSetup = [this](Async<DbContents> &task) {
        task.setConcurrentCallData(parseProject, m_projectFileContents, m_projectFilePath);
        QObject::connect(&task, &AsyncBase::started, this, [this, taskPtr = &task] {
            Core::ProgressManager::addTask(taskPtr->future(),
                                           Tr::tr("Parse \"%1\" project").arg(m_projectName),
                                           "CompilationDatabase.Parse");
        });
    };
    const auto onDone = [this](const Async<DbContents> &task) { m_dbContents = task.result(); };

    const auto onTreeDone = [this] { finish(ParseResult::Success); };

    const Group recipe {
        parallel,
        finishAllAndSuccess,
        treeScannerTask,
        AsyncTask<DbContents>(onSetup, onDone)
    };
    m_taskTreeRunner.start(recipe, {}, onTreeDone);
}

void CompilationDbParser::stop()
{
    disconnect();
    m_taskTreeRunner.reset();
    m_guard = {};
    deleteLater();
}

void CompilationDbParser::finish(ParseResult result)
{
    if (result != ParseResult::Failure)
        m_guard.markAsSuccess();

    emit finished(result);
    deleteLater();
}

} // namespace CompilationDatabaseProjectManager::Internal
