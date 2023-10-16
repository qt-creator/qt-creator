// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../core_global.h"

#include <utils/filepath.h>
#include <utils/id.h>

#include <optional>

QT_BEGIN_NAMESPACE
class QAbstractItemModel;
class QIcon;
QT_END_NAMESPACE

namespace Core {

class IEditor;
class IDocument;

class CORE_EXPORT DocumentModel
{
public:
    static void init();
    static void destroy();

    static QIcon lockedIcon();
    static QAbstractItemModel *model();

    struct CORE_EXPORT Entry {
        Entry();
        ~Entry();
        Utils::FilePath filePath() const;
        QString displayName() const;
        QString plainDisplayName() const;
        QString uniqueDisplayName() const;
        Utils::Id id() const;

        IDocument *document;
        // When an entry is suspended, it means that it is not in memory,
        // and there is no IEditor for it and only a dummy IDocument.
        // This is typically the case for files that have not been opened yet,
        // but can also happen later after they have been opened.
        // The related setting for this is found in:
        // Edit > Preferences > Environment > System > Auto-suspend unmodified files
        bool isSuspended;
        // The entry has been pinned, which means that it should stick to
        // the top of any lists of open files, and that any actions that close
        // files in bulk should not close this one.
        bool pinned;
    };

    static Entry *entryAtRow(int row);
    static std::optional<int> rowOfDocument(IDocument *document);

    static int entryCount();
    static QList<Entry *> entries();
    static std::optional<int> indexOfDocument(IDocument *document);
    static std::optional<int> indexOfFilePath(const Utils::FilePath &filePath);
    static Entry *entryForDocument(IDocument *document);
    static Entry *entryForFilePath(const Utils::FilePath &filePath);
    static QList<IDocument *> openedDocuments();

    static IDocument *documentForFilePath(const Utils::FilePath &filePath);
    static QList<IEditor *> editorsForFilePath(const Utils::FilePath &filePath);
    static QList<IEditor *> editorsForDocument(IDocument *document);
    static QList<IEditor *> editorsForDocuments(const QList<IDocument *> &entries);
    static QList<IEditor *> editorsForOpenedDocuments();

    static const int FilePathRole = Qt::UserRole + 23;

private:
    DocumentModel();
};

} // namespace Core

Q_DECLARE_METATYPE(Core::DocumentModel::Entry *)
