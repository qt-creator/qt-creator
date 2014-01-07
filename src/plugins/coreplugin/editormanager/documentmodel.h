/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef DOCUMENTMODEL_H
#define DOCUMENTMODEL_H

#include "../core_global.h"
#include "../id.h"

#include <QAbstractItemModel>

QT_FORWARD_DECLARE_CLASS(QIcon)

namespace Core {

struct DocumentModelPrivate;
class IEditor;
class IDocument;

class CORE_EXPORT DocumentModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit DocumentModel(QObject *parent);
    virtual ~DocumentModel();

    QIcon lockedIcon() const;
    QIcon unlockedIcon() const;

    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QModelIndex parent(const QModelIndex &/*index*/) const { return QModelIndex(); }
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QModelIndex index(int row, int column = 0, const QModelIndex &parent = QModelIndex()) const;

    struct CORE_EXPORT Entry {
        Entry();
        IDocument *document;
        QString fileName() const;
        QString displayName() const;
        Id id() const;
        QString m_fileName;
        QString m_displayName;
        Id m_id;
    };

    Entry *documentAtRow(int row) const;
    int rowOfDocument(IDocument *document) const;

    int documentCount() const;
    QList<Entry *> documents() const;
    int indexOfDocument(IDocument *document) const;
    int indexOfFilePath(const QString &filePath) const;
    Entry *entryForDocument(IDocument *document) const;
    QList<IDocument *> openedDocuments() const;

    IDocument *documentForFilePath(const QString &filePath) const;
    QList<IEditor *> editorsForFilePath(const QString &filePath) const;
    QList<IEditor *> editorsForDocument(IDocument *document) const;
    QList<IEditor *> editorsForDocuments(const QList<IDocument *> &documents) const;
    QList<IEditor *> oneEditorForEachOpenedDocument() const;

    // editor manager related functions, nobody else should call it
    void addEditor(IEditor *editor, bool *isNewDocument);
    void addRestoredDocument(const QString &fileName, const QString &displayName, const Id &id);
    Entry *firstRestoredDocument() const;
    void removeEditor(IEditor *editor, bool *lastOneForDocument);
    void removeDocument(const QString &fileName);
    void removeEntry(Entry *entry);
    void removeAllRestoredDocuments();

private slots:
    void itemChanged();

private:
    void addEntry(Entry *entry);
    void removeDocument(int idx);

    DocumentModelPrivate *d;
};

} // namespace Core

#endif // DOCUMENTMODEL_H
