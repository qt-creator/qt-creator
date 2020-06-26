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

#include "../core_global.h"

#include <utils/fileutils.h>
#include <utils/id.h>
#include <utils/optional.h>

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
        Utils::FilePath fileName() const;
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
        // Options > Environment > System > Auto-suspend unmodified files
        bool isSuspended;
        // The entry has been pinned, which means that it should stick to
        // the top of any lists of open files, and that any actions that close
        // files in bulk should not close this one.
        bool pinned;
    };

    static Entry *entryAtRow(int row);
    static Utils::optional<int> rowOfDocument(IDocument *document);

    static int entryCount();
    static QList<Entry *> entries();
    static Utils::optional<int> indexOfDocument(IDocument *document);
    static Utils::optional<int> indexOfFilePath(const Utils::FilePath &filePath);
    static Entry *entryForDocument(IDocument *document);
    static Entry *entryForFilePath(const Utils::FilePath &filePath);
    static QList<IDocument *> openedDocuments();

    static IDocument *documentForFilePath(const QString &filePath);
    static QList<IEditor *> editorsForFilePath(const QString &filePath);
    static QList<IEditor *> editorsForDocument(IDocument *document);
    static QList<IEditor *> editorsForDocuments(const QList<IDocument *> &entries);
    static QList<IEditor *> editorsForOpenedDocuments();

private:
    DocumentModel();
};

} // namespace Core

Q_DECLARE_METATYPE(Core::DocumentModel::Entry *)
