// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "diffeditordocument.h"
#include "diffeditorconstants.h"
#include "diffeditorcontroller.h"
#include "diffeditortr.h"

#include <utils/fileutils.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>

#include <coreplugin/dialogs/codecselector.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>

#include <QCoreApplication>
#include <QFile>
#include <QDir>
#include <QMenu>
#include <QTextCodec>
#include <QUuid>

using namespace Core;
using namespace Utils;

namespace DiffEditor::Internal {

DiffEditorDocument::DiffEditorDocument() :
    Core::BaseTextDocument()
{
    setId(Constants::DIFF_EDITOR_ID);
    setMimeType(Constants::DIFF_EDITOR_MIMETYPE);
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
}

DiffEditorController *DiffEditorDocument::controller() const
{
    return m_controller;
}

static void appendRow(ChunkData *chunk, const RowData &row)
{
    const bool isSeparator = row.line[LeftSide].textLineType == TextLineData::Separator
            && row.line[RightSide].textLineType == TextLineData::Separator;
    if (!isSeparator)
        chunk->rows.append(row);
}

ChunkData DiffEditorDocument::filterChunk(const ChunkData &data, const ChunkSelection &selection,
                                          PatchAction patchAction)
{
    if (selection.isNull())
        return data;

    ChunkData chunk(data);
    chunk.rows.clear();
    for (int i = 0; i < data.rows.count(); ++i) {
        RowData row = data.rows[i];
        const bool isLeftSelected = selection.selection[LeftSide].contains(i);
        const bool isRightSelected = selection.selection[RightSide].contains(i);

        if (isLeftSelected || isRightSelected) {
            if (row.equal || (isLeftSelected && isRightSelected)) {
                appendRow(&chunk, row);
            } else if (isLeftSelected) {
                RowData newRow = row;

                row.line[RightSide] = TextLineData(TextLineData::Separator);
                appendRow(&chunk, row);

                if (patchAction == PatchAction::Revert) {
                    newRow.line[LeftSide] = newRow.line[RightSide];
                    newRow.equal = true;
                    appendRow(&chunk, newRow);
                }
            } else { // isRightSelected
                if (patchAction == PatchAction::Apply) {
                    RowData newRow = row;
                    newRow.line[RightSide] = newRow.line[LeftSide];
                    newRow.equal = true;
                    appendRow(&chunk, newRow);
                }

                row.line[LeftSide] = TextLineData(TextLineData::Separator);
                appendRow(&chunk, row);
            }
        } else {
            if (patchAction == PatchAction::Apply)
                row.line[RightSide] = row.line[LeftSide];
            else
                row.line[LeftSide] = row.line[RightSide];
            row.equal = true;
            appendRow(&chunk, row);
        }
    }

    return chunk;
}

QString DiffEditorDocument::makePatch(int fileIndex, int chunkIndex,
                                      const ChunkSelection &selection, PatchAction patchAction,
                                      bool addPrefix, const QString &overriddenFileName) const
{
    if (fileIndex < 0 || chunkIndex < 0 || fileIndex >= m_diffFiles.count())
        return {};

    const FileData &fileData = m_diffFiles.at(fileIndex);
    if (chunkIndex >= fileData.chunks.count())
        return {};

    const ChunkData chunkData = filterChunk(fileData.chunks.at(chunkIndex), selection, patchAction);
    const bool lastChunk = (chunkIndex == fileData.chunks.count() - 1);

    const QString fileName = !overriddenFileName.isEmpty()
            ? overriddenFileName : patchAction == PatchAction::Apply
              ? fileData.fileInfo[LeftSide].fileName : fileData.fileInfo[RightSide].fileName;

    const QString leftFileName = addPrefix ? QString("a/") + fileName : fileName;
    const QString rightFileName = addPrefix ? QString("b/") + fileName : fileName;
    return DiffUtils::makePatch(chunkData, leftFileName, rightFileName,
                                lastChunk && fileData.lastChunkAtTheEndOfFile);
}

void DiffEditorDocument::setDiffFiles(const QList<FileData> &data)
{
    m_diffFiles = data;
    emit documentChanged();
}

QList<FileData> DiffEditorDocument::diffFiles() const
{
    return m_diffFiles;
}

FilePath DiffEditorDocument::workingDirectory() const
{
    return m_workingDirectory;
}

void DiffEditorDocument::setWorkingDirectory(const FilePath &directory)
{
    m_workingDirectory = directory;
}

void DiffEditorDocument::setStartupFile(const QString &startupFile)
{
    m_startupFile = startupFile;
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
    Q_UNUSED(contents)
    return true;
}

FilePath DiffEditorDocument::fallbackSaveAsPath() const
{
    if (!m_workingDirectory.isEmpty())
        return m_workingDirectory;
    return FileUtils::homePath();
}

bool DiffEditorDocument::isSaveAsAllowed() const
{
    return state() == LoadOK;
}

bool DiffEditorDocument::saveImpl(QString *errorString, const FilePath &filePath, bool autoSave)
{
    Q_UNUSED(errorString)
    Q_UNUSED(autoSave)

    if (state() != LoadOK)
        return false;

    const bool ok = write(filePath, format(), plainText(), errorString);

    if (!ok)
        return false;

    setController(nullptr);
    setDescription({});
    Core::EditorManager::clearUniqueId(this);

    setTemporary(false);
    setFilePath(filePath.absoluteFilePath());
    setPreferredDisplayName({});
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
    return open(errorString, filePath(), filePath()) == OpenResult::Success;
}

Core::IDocument::OpenResult DiffEditorDocument::open(QString *errorString, const FilePath &filePath,
                                                     const FilePath &realFilePath)
{
    QTC_CHECK(filePath == realFilePath); // does not support autosave
    beginReload();
    QString patch;
    ReadResult readResult = read(filePath, &patch, errorString);
    if (readResult == TextFileFormat::ReadIOError
        || readResult == TextFileFormat::ReadMemoryAllocationError) {
        return OpenResult::ReadError;
    }

    const std::optional<QList<FileData>> fileDataList = DiffUtils::readPatch(patch);
    bool ok = fileDataList.has_value();
    if (!ok) {
        *errorString = Tr::tr("Could not parse patch file \"%1\". "
                              "The content is not of unified diff format.")
                .arg(filePath.toUserOutput());
    } else {
        setTemporary(false);
        emit temporaryStateChanged();
        setFilePath(filePath.absoluteFilePath());
        setWorkingDirectory(filePath.absoluteFilePath());
        setDiffFiles(*fileDataList);
    }
    endReload(ok);
    if (!ok && readResult == TextFileFormat::ReadEncodingError)
        ok = selectEncoding();
    return ok ? OpenResult::Success : OpenResult::CannotHandle;
}

bool DiffEditorDocument::selectEncoding()
{
    Core::CodecSelector codecSelector(Core::ICore::dialogParent(), this);
    switch (codecSelector.exec()) {
    case Core::CodecSelector::Reload: {
        setCodec(codecSelector.selectedCodec());
        QString errorMessage;
        return reload(&errorMessage, Core::IDocument::FlagReload, Core::IDocument::TypeContents);
    }
    case Core::CodecSelector::Save:
        setCodec(codecSelector.selectedCodec());
        return Core::EditorManager::saveDocument(this);
    case Core::CodecSelector::Cancel:
        break;
    }
    return false;
}

QString DiffEditorDocument::fallbackSaveAsFileName() const
{
    const int maxSubjectLength = 50;

    const QString desc = description();
    if (!desc.isEmpty()) {
        QString name = QString::fromLatin1("0001-%1").arg(desc.left(desc.indexOf('\n')));
        name = FileUtils::fileSystemFriendlyName(name);
        name.truncate(maxSubjectLength);
        name.append(".patch");
        return name;
    }
    return QStringLiteral("0001.patch");
}

// ### fixme: git-specific handling should be done in the git plugin:
// Remove unexpanded branches and follows-tag, clear indentation
// and create E-mail
static QString formatGitDescription(const QString &description)
{
    QString result;
    result.reserve(description.size());
    const auto descriptionList = description.split('\n');
    for (QString line : descriptionList) {
        if (line.startsWith("commit ") || line.startsWith("Branches: <Expand>"))
            continue;

        if (line.startsWith("Author: "))
            line.replace(0, 8, "From: ");
        else if (line.startsWith("    "))
            line.remove(0, 4);
        result.append(line);
        result.append('\n');
    }
    return result;
}

QString DiffEditorDocument::plainText() const
{
    return Utils::joinStrings({formatGitDescription(description()),
                               DiffUtils::makePatch(diffFiles())}, '\n');
}

void DiffEditorDocument::beginReload()
{
    emit aboutToReload();
    m_state = Reloading;
    emit changed();
    QSignalBlocker blocker(this);
    setDiffFiles({});
    setDescription({});
}

void DiffEditorDocument::endReload(bool success)
{
    m_state = success ? LoadOK : LoadFailed;
    emit changed();
    emit reloadFinished(success);
}

} // namespace DiffEditor::Internal
