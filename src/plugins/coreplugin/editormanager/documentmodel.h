/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
    static QIcon unlockedIcon();

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
        bool isRestored;
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
    static void addRestoredDocument(const QString &fileName, const QString &displayName, Id id);
    static Entry *firstRestoredEntry();
    static void removeEditor(IEditor *editor, bool *lastOneForDocument);
    static void removeDocument(const QString &fileName);
    static void removeEntry(Entry *entry);
    static void removeAllRestoredEntries();

private:
    DocumentModel();
};

} // namespace Core

#endif // DOCUMENTMODEL_H
