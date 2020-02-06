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

#include "diffeditor_global.h"
#include "diffutils.h"

#include <QObject>

QT_FORWARD_DECLARE_CLASS(QMenu)

namespace Core { class IDocument; }

namespace DiffEditor {

namespace Internal { class DiffEditorDocument; }

class ChunkSelection;

class DIFFEDITOR_EXPORT DiffEditorController : public QObject
{
    Q_OBJECT
public:
    explicit DiffEditorController(Core::IDocument *document);

    void requestReload();
    bool isReloading() const;

    QString baseDirectory() const;
    void setBaseDirectory(const QString &directory);
    int contextLineCount() const;
    bool ignoreWhitespace() const;

    enum PatchOption {
        NoOption = 0,
        Revert = 1,
        AddPrefix = 2
    };
    Q_DECLARE_FLAGS(PatchOptions, PatchOption)
    QString makePatch(int fileIndex, int chunkIndex, const ChunkSelection &selection,
                      PatchOptions options) const;

    static Core::IDocument *findOrCreateDocument(const QString &vcsId,
                                                 const QString &displayName);
    static DiffEditorController *controller(Core::IDocument *document);

    void requestChunkActions(QMenu *menu, int fileIndex, int chunkIndex,
                             const ChunkSelection &selection);
    bool chunkExists(int fileIndex, int chunkIndex) const;
    Core::IDocument *document() const;

    // reloadFinished() should be called inside the reloader (for synchronous reload)
    // or later (for asynchronous reload)
    void setReloader(const std::function<void ()> &reloader);

signals:
    void chunkActionsRequested(QMenu *menu, int fileIndex, int chunkIndex,
                               const ChunkSelection &selection);

protected:
    void reloadFinished(bool success);

    void setDiffFiles(const QList<FileData> &diffFileList,
                      const QString &baseDirectory = QString(),
                      const QString &startupFile = QString());
    void setDescription(const QString &description);
    QString description() const;
    void forceContextLineCount(int lines);

private:
    Internal::DiffEditorDocument *const m_document;
    bool m_isReloading = false;
    std::function<void()> m_reloader;

    friend class Internal::DiffEditorDocument;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(DiffEditorController::PatchOptions)

} // namespace DiffEditor
