// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "diffutils.h"

#include <coreplugin/patchtool.h>
#include <coreplugin/textdocument.h>

namespace DiffEditor {

class DiffEditorController;
class ChunkSelection;

namespace Internal {

class DiffEditorDocument : public Core::BaseTextDocument
{
    Q_OBJECT
    Q_PROPERTY(QString plainText READ plainText STORED false) // For access by code pasters
public:
    DiffEditorDocument();

    DiffEditorController *controller() const;

    enum State {
        LoadOK,
        Reloading,
        LoadFailed
    };

    static ChunkData filterChunk(const ChunkData &data, const ChunkSelection &selection,
                                 Core::PatchAction patchAction);
    QString makePatch(int fileIndex, int chunkIndex, const ChunkSelection &selection,
                      Core::PatchAction patchAction, bool addPrefix = false,
                      const QString &overriddenFileName = {}) const;

    void setDiffFiles(const QList<FileData> &data);
    QList<FileData> diffFiles() const;
    Utils::FilePath workingDirectory() const;
    void setWorkingDirectory(const Utils::FilePath &directory);
    void setStartupFile(const QString &startupFile);
    QString startupFile() const;

    void setDescription(const QString &description);
    QString description() const;

    void setContextLineCount(int lines);
    int contextLineCount() const;
    void forceContextLineCount(int lines);
    bool isContextLineCountForced() const;
    void setIgnoreWhitespace(bool ignore);
    bool ignoreWhitespace() const;

    bool setContents(const QByteArray &contents) override;
    Utils::FilePath fallbackSaveAsPath() const override;
    QString fallbackSaveAsFileName() const override;

    bool isSaveAsAllowed() const override;
    void reload();
    bool reload(QString *errorString, ReloadFlag flag, ChangeType type) override;
    OpenResult open(QString *errorString, const Utils::FilePath &filePath,
                    const Utils::FilePath &realFilePath) override;
    bool selectEncoding();
    State state() const { return m_state; }

    QString plainText() const;

signals:
    void temporaryStateChanged();
    void documentChanged();
    void descriptionChanged();

protected:
    bool saveImpl(QString *errorString, const Utils::FilePath &filePath, bool autoSave) override;

private:
    void beginReload();
    void endReload(bool success);
    void setController(DiffEditorController *controller);

    DiffEditorController *m_controller = nullptr;
    QList<FileData> m_diffFiles;
    Utils::FilePath m_workingDirectory;
    QString m_startupFile;
    QString m_description;
    int m_contextLineCount = 3;
    bool m_isContextLineCountForced = false;
    bool m_ignoreWhitespace = false;
    State m_state = LoadOK;

    friend class ::DiffEditor::DiffEditorController;
};

} // namespace Internal
} // namespace DiffEditor
