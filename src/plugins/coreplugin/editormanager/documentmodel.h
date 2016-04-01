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

#ifndef DOCUMENTMODEL_H
#define DOCUMENTMODEL_H

#include "../core_global.h"
#include "../id.h"

#include <utils/fileutils.h>

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
        Utils::FileName fileName() const;
        QString displayName() const;
        QString plainDisplayName() const;
        QString uniqueDisplayName() const;
        Id id() const;

        IDocument *document;
        bool isSuspended;
    };

    static Entry *entryAtRow(int row);
    static int rowOfDocument(IDocument *document);

    static int entryCount();
    static QList<Entry *> entries();
    static int indexOfDocument(IDocument *document);
    static Entry *entryForDocument(IDocument *document);
    static QList<IDocument *> openedDocuments();

    static IDocument *documentForFilePath(const QString &filePath);
    static QList<IEditor *> editorsForFilePath(const QString &filePath);
    static QList<IEditor *> editorsForDocument(IDocument *document);
    static QList<IEditor *> editorsForDocuments(const QList<IDocument *> &entries);
    static QList<IEditor *> editorsForOpenedDocuments();

    // editor manager related functions, nobody else should call it
    static void addEditor(IEditor *editor, bool *isNewDocument);
    static void addSuspendedDocument(const QString &fileName, const QString &displayName, Id id);
    static Entry *firstSuspendedEntry();
    static void removeEditor(IEditor *editor, bool *lastOneForDocument);
    static void removeDocument(const QString &fileName);
    static void removeEntry(Entry *entry);
    static void removeAllSuspendedEntries();

private:
    DocumentModel();
};

} // namespace Core

#endif // DOCUMENTMODEL_H
