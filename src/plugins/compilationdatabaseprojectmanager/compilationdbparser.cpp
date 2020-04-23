/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "compilationdbparser.h"

#include <coreplugin/progressmanager/progressmanager.h>
#include <projectexplorer/task.h>
#include <projectexplorer/treescanner.h>
#include <utils/mimetypes/mimetype.h>
#include <utils/runextensions.h>

#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

using namespace ProjectExplorer;
using namespace Utils;

namespace CompilationDatabaseProjectManager {
namespace Internal {

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
{
    connect(&m_parserWatcher, &QFutureWatcher<void>::finished, this, [this] {
        m_dbContents = m_parserWatcher.result();
        if (!m_treeScanner || m_treeScanner->isFinished())
            finish(ParseResult::Success);
    });
}

void CompilationDbParser::start()
{
    // Check hash first.
    QFile file(m_projectFilePath.toString());
    if (!file.open(QIODevice::ReadOnly)) {
        finish(ParseResult::Failure);
        return;
    }
    m_projectFileContents = file.readAll();
    const QByteArray newHash = QCryptographicHash::hash(m_projectFileContents,
                                                        QCryptographicHash::Sha1);
    if (m_projectFileHash == newHash) {
        finish(ParseResult::Cached);
        return;
    }
    m_projectFileHash = newHash;

    // Thread 1: Scan disk.
    if (!m_rootPath.isEmpty()) {
        m_treeScanner = new TreeScanner(this);
        m_treeScanner->setFilter([this](const MimeType &mimeType, const FilePath &fn) {
            // Mime checks requires more resources, so keep it last in check list
            bool isIgnored = fn.toString().startsWith(m_projectFilePath.toString() + ".user")
                    || TreeScanner::isWellKnownBinary(mimeType, fn);

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
        m_treeScanner->setTypeFactory([](const Utils::MimeType &mimeType, const Utils::FilePath &fn) {
            return TreeScanner::genericFileType(mimeType, fn);
        });
        m_treeScanner->asyncScanForFiles(m_rootPath);
        Core::ProgressManager::addTask(m_treeScanner->future(),
                                       tr("Scan \"%1\" project tree").arg(m_projectName),
                                       "CompilationDatabase.Scan.Tree");
        connect(m_treeScanner, &TreeScanner::finished, this, [this] {
            if (m_parserWatcher.isFinished())
                finish(ParseResult::Success);
        });
    }

    // Thread 2: Parse the project file.
    const QFuture<DbContents> future = runAsync(&CompilationDbParser::parseProject, this);
    Core::ProgressManager::addTask(future,
                                   tr("Parse \"%1\" project").arg(m_projectName),
                                   "CompilationDatabase.Parse");
    m_parserWatcher.setFuture(future);
}

void CompilationDbParser::stop()
{
    disconnect();
    m_parserWatcher.disconnect();
    m_parserWatcher.cancel();
    if (m_treeScanner) {
        m_treeScanner->disconnect();
        m_treeScanner->future().cancel();
    }
    m_guard = {};
    deleteLater();
}

QList<FileNode *> CompilationDbParser::scannedFiles() const
{
    return m_treeScanner && !m_treeScanner->future().isCanceled()
            ? m_treeScanner->release() : QList<FileNode *>();
}

void CompilationDbParser::finish(ParseResult result)
{
    emit finished(result);
    deleteLater();
}

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

static FilePath jsonObjectFilename(const QJsonObject &object)
{
    const QString workingDir = QDir::fromNativeSeparators(object["directory"].toString());
    FilePath fileName = FilePath::fromString(QDir::fromNativeSeparators(object["file"].toString()));
    if (fileName.toFileInfo().isRelative())
        fileName = FilePath::fromString(QDir::cleanPath(workingDir + "/" + fileName.toString()));
    return fileName;
}

std::vector<DbEntry> CompilationDbParser::readJsonObjects() const
{
    std::vector<DbEntry> result;

    int objectStart = m_projectFileContents.indexOf('{');
    int objectEnd = m_projectFileContents.indexOf('}', objectStart + 1);

    QSet<QString> flagsCache;
    while (objectStart >= 0 && objectEnd >= 0) {
        const QJsonDocument document = QJsonDocument::fromJson(
                    m_projectFileContents.mid(objectStart, objectEnd - objectStart + 1));
        if (document.isNull()) {
            // The end was found incorrectly, search for the next one.
            objectEnd = m_projectFileContents.indexOf('}', objectEnd + 1);
            continue;
        }

        const QJsonObject object = document.object();
        const Utils::FilePath fileName = jsonObjectFilename(object);
        const QStringList flags = filterFromFileName(jsonObjectFlags(object, flagsCache),
                                                     fileName.toFileInfo().baseName());
        result.push_back({flags, fileName, object["directory"].toString()});

        objectStart = m_projectFileContents.indexOf('{', objectEnd + 1);
        objectEnd = m_projectFileContents.indexOf('}', objectStart + 1);
    }

    return result;
}

QStringList readExtraFiles(const QString &filePath)
{
    QStringList result;

    QFile file(filePath);
    if (file.open(QFile::ReadOnly)) {
        QTextStream stream(&file);

        while (!stream.atEnd()) {
            QString line = stream.readLine();
            line = line.trimmed();

            if (line.isEmpty() || line.startsWith('#'))
                continue;

            result.push_back(line);
        }
    }

    return result;
}

DbContents CompilationDbParser::parseProject()
{
    DbContents dbContents;
    dbContents.entries = readJsonObjects();
    dbContents.extraFileName = m_projectFilePath.toString() +
            Constants::COMPILATIONDATABASEPROJECT_FILES_SUFFIX;
    dbContents.extras = readExtraFiles(dbContents.extraFileName);
    std::sort(dbContents.entries.begin(), dbContents.entries.end(),
              [](const DbEntry &lhs, const DbEntry &rhs) {
        return std::lexicographical_compare(lhs.flags.begin(), lhs.flags.end(),
                                            rhs.flags.begin(), rhs.flags.end());
    });
    return dbContents;
}

} // namespace Internal
} // namespace CompilationDatabaseProjectManager
