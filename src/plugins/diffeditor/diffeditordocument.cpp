/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "diffeditordocument.h"
#include "diffeditorconstants.h"
#include "diffeditorcontroller.h"
#include "diffutils.h"

#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <coreplugin/editormanager/editormanager.h>

#include <QCoreApplication>
#include <QFile>
#include <QDir>
#include <QMenu>
#include <QTextCodec>
#include <QUuid>

using namespace Utils;

namespace DiffEditor {
namespace Internal {

DiffEditorDocument::DiffEditorDocument() :
    Core::BaseTextDocument(),
    m_controller(0),
    m_contextLineCount(3),
    m_isContextLineCountForced(false),
    m_ignoreWhitespace(false)
{
    setId(Constants::DIFF_EDITOR_ID);
    setMimeType(QLatin1String(Constants::DIFF_EDITOR_MIMETYPE));
    setTemporary(true);
}

/**
 * @brief Set a controller for a document
 * @param controller The controller to set.
 *
 * This method takes ownership of the controller and will delete it after it is done with it.
 */
void DiffEditorDocument::setController(DiffEditorController *controller)
{
    if (m_controller == controller)
        return;

    QTC_ASSERT(isTemporary(), return);

    if (m_controller)
        m_controller->deleteLater();
    m_controller = controller;

    if (m_controller) {
        connect(this, &DiffEditorDocument::chunkActionsRequested,
                m_controller, &DiffEditorController::requestChunkActions);
        connect(this, &DiffEditorDocument::requestMoreInformation,
                m_controller, &DiffEditorController::requestMoreInformation);
    }
}

DiffEditorController *DiffEditorDocument::controller() const
{
    return m_controller;
}

QString DiffEditorDocument::makePatch(int fileIndex, int chunkIndex,
                                      bool revert, bool addPrefix,
                                      const QString &overriddenFileName) const
{
    if (fileIndex < 0 || chunkIndex < 0)
        return QString();

    if (fileIndex >= m_diffFiles.count())
        return QString();

    const FileData &fileData = m_diffFiles.at(fileIndex);
    if (chunkIndex >= fileData.chunks.count())
        return QString();

    const ChunkData &chunkData = fileData.chunks.at(chunkIndex);
    const bool lastChunk = (chunkIndex == fileData.chunks.count() - 1);

    const QString fileName = !overriddenFileName.isEmpty()
            ? overriddenFileName : revert
              ? fileData.rightFileInfo.fileName
              : fileData.leftFileInfo.fileName;

    QString leftPrefix, rightPrefix;
    if (addPrefix) {
        leftPrefix = QLatin1String("a/");
        rightPrefix = QLatin1String("b/");
    }
    return DiffUtils::makePatch(chunkData,
                                leftPrefix + fileName,
                                rightPrefix + fileName,
                                lastChunk && fileData.lastChunkAtTheEndOfFile);
}

void DiffEditorDocument::setDiffFiles(const QList<FileData> &data, const QString &directory,
                                      const QString &startupFile)
{
    m_diffFiles = data;
    m_baseDirectory = directory;
    m_startupFile = startupFile;
    emit documentChanged();
}

QList<FileData> DiffEditorDocument::diffFiles() const
{
    return m_diffFiles;
}

QString DiffEditorDocument::baseDirectory() const
{
    return m_baseDirectory;
}

QString DiffEditorDocument::startupFile() const
{
    return m_startupFile;
}

void DiffEditorDocument::setDescription(const QString &description)
{
    if (m_description == description)
        return;

    m_description = description;
    emit descriptionChanged();
}

QString DiffEditorDocument::description() const
{
    return m_description;
}

void DiffEditorDocument::setContextLineCount(int lines)
{
    QTC_ASSERT(!m_isContextLineCountForced, return);
    m_contextLineCount = lines;
}

int DiffEditorDocument::contextLineCount() const
{
    return m_contextLineCount;
}

void DiffEditorDocument::forceContextLineCount(int lines)
{
    m_contextLineCount = lines;
    m_isContextLineCountForced = true;
}

bool DiffEditorDocument::isContextLineCountForced() const
{
    return m_isContextLineCountForced;
}

void DiffEditorDocument::setIgnoreWhitespace(bool ignore)
{
    m_ignoreWhitespace = ignore;
}

bool DiffEditorDocument::ignoreWhitespace() const
{
    return m_ignoreWhitespace;
}

bool DiffEditorDocument::setContents(const QByteArray &contents)
{
    Q_UNUSED(contents);
    return true;
}

QString DiffEditorDocument::fallbackSaveAsPath() const
{
    if (!m_baseDirectory.isEmpty())
        return m_baseDirectory;
    return QDir::homePath();
}

bool DiffEditorDocument::save(QString *errorString, const QString &fileName, bool autoSave)
{
    Q_UNUSED(errorString)
    Q_UNUSED(autoSave)

    const bool ok = write(fileName, format(), plainText(), errorString);

    if (!ok)
        return false;

    setController(0);
    setDescription(QString());
    Core::EditorManager::clearUniqueId(this);

    const QFileInfo fi(fileName);
    setTemporary(false);
    setFilePath(FileName::fromString(fi.absoluteFilePath()));
    setPreferredDisplayName(QString());
    emit temporaryStateChanged();

    return true;
}

void DiffEditorDocument::reload()
{
    if (m_controller) {
        m_controller->requestReload();
    } else {
        QString errorMessage;
        reload(&errorMessage, Core::IDocument::FlagReload, Core::IDocument::TypeContents);
    }
}

bool DiffEditorDocument::reload(QString *errorString, ReloadFlag flag, ChangeType type)
{
    Q_UNUSED(type)
    if (flag == FlagIgnore)
        return true;
    return open(errorString, filePath().toString(), filePath().toString()) == OpenResult::Success;
}

Core::IDocument::OpenResult DiffEditorDocument::open(QString *errorString, const QString &fileName,
                              const QString &realFileName)
{
    QTC_CHECK(fileName == realFileName); // does not support autosave
    beginReload();
    QString patch;
    ReadResult readResult = read(fileName, &patch, errorString);
    if (readResult == TextFileFormat::ReadEncodingError)
        return OpenResult::CannotHandle;
    else if (readResult != TextFileFormat::ReadSuccess)
        return OpenResult::ReadError;

    bool ok = false;
    QList<FileData> fileDataList = DiffUtils::readPatch(patch, &ok);
    if (!ok) {
        *errorString = tr("Could not parse patch file \"%1\". "
                          "The content is not of unified diff format.")
                .arg(fileName);
    } else {
        const QFileInfo fi(fileName);
        setTemporary(false);
        emit temporaryStateChanged();
        setFilePath(FileName::fromString(fi.absoluteFilePath()));
        setDiffFiles(fileDataList, fi.absolutePath());
    }
    endReload(ok);
    return ok ? OpenResult::Success : OpenResult::CannotHandle;
}

QString DiffEditorDocument::fallbackSaveAsFileName() const
{
    const int maxSubjectLength = 50;

    const QString desc = description();
    if (!desc.isEmpty()) {
        QString name = QString::fromLatin1("0001-%1").arg(desc.left(desc.indexOf(QLatin1Char('\n'))));
        name = FileUtils::fileSystemFriendlyName(name);
        name.truncate(maxSubjectLength);
        name.append(QLatin1String(".patch"));
        return name;
    }
    return QStringLiteral("0001.patch");
}

// ### fixme: git-specific handling should be done in the git plugin:
// Remove unexpanded branches and follows-tag, clear indentation
// and create E-mail
static void formatGitDescription(QString *description)
{
    QString result;
    result.reserve(description->size());
    foreach (QString line, description->split(QLatin1Char('\n'))) {
        if (line.startsWith(QLatin1String("commit "))
            || line.startsWith(QLatin1String("Branches: <Expand>"))) {
            continue;
        }
        if (line.startsWith(QLatin1String("Author: ")))
            line.replace(0, 8, QStringLiteral("From: "));
        else if (line.startsWith(QLatin1String("    ")))
            line.remove(0, 4);
        result.append(line);
        result.append(QLatin1Char('\n'));
    }
    *description = result;
}

QString DiffEditorDocument::plainText() const
{
    QString result = description();
    const int formattingOptions = DiffUtils::GitFormat;
    if (formattingOptions & DiffUtils::GitFormat)
        formatGitDescription(&result);

    const QString diff = DiffUtils::makePatch(diffFiles(), formattingOptions);
    if (!diff.isEmpty()) {
        if (!result.isEmpty())
            result += QLatin1Char('\n');
        result += diff;
    }
    return result;
}

void DiffEditorDocument::beginReload()
{
    emit aboutToReload();
    m_isReloading = true;
    const bool blocked = blockSignals(true);
    setDiffFiles(QList<FileData>(), QString());
    setDescription(QString());
    blockSignals(blocked);
}

void DiffEditorDocument::endReload(bool success)
{
    m_isReloading = false;
    emit reloadFinished(success);
}

} // namespace Internal
} // namespace DiffEditor
