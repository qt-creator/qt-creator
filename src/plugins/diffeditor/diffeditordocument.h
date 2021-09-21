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

#pragma once

#include "diffutils.h"

#include <coreplugin/textdocument.h>

QT_FORWARD_DECLARE_CLASS(QMenu)

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

    static ChunkData filterChunk(const ChunkData &data,
                                 const ChunkSelection &selection, bool revert);
    QString makePatch(int fileIndex, int chunkIndex, const ChunkSelection &selection,
                      bool revert, bool addPrefix = false,
                      const QString &overriddenFileName = QString()) const;

    void setDiffFiles(const QList<FileData> &data, const Utils::FilePath &directory,
                      const QString &startupFile = QString());
    QList<FileData> diffFiles() const;
    Utils::FilePath baseDirectory() const;
    void setBaseDirectory(const Utils::FilePath &directory);
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
    bool save(QString *errorString, const Utils::FilePath &filePath, bool autoSave) override;
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

private:
    void beginReload();
    void endReload(bool success);
    void setController(DiffEditorController *controller);

    DiffEditorController *m_controller = nullptr;
    QList<FileData> m_diffFiles;
    Utils::FilePath m_baseDirectory;
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
