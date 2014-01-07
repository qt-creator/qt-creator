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

#include "documentmodel.h"
#include "ieditor.h"
#include <coreplugin/documentmanager.h>
#include <coreplugin/idocument.h>

#include <utils/qtcassert.h>

#include <QDir>
#include <QIcon>

namespace Core {

struct DocumentModelPrivate
{
    DocumentModelPrivate();
    ~DocumentModelPrivate();

    const QIcon m_lockedIcon;
    const QIcon m_unlockedIcon;

    QList<DocumentModel::Entry *> m_documents;
    QMap<IDocument *, QList<IEditor *> > m_editors;
};

DocumentModelPrivate::DocumentModelPrivate() :
    m_lockedIcon(QLatin1String(":/core/images/locked.png")),
    m_unlockedIcon(QLatin1String(":/core/images/unlocked.png"))
{
}

DocumentModelPrivate::~DocumentModelPrivate()
{
    qDeleteAll(m_documents);
}

DocumentModel::Entry::Entry() :
    document(0)
{
}

DocumentModel::DocumentModel(QObject *parent) :
    QAbstractItemModel(parent), d(new DocumentModelPrivate)
{
}

DocumentModel::~DocumentModel()
{
    delete d;
}

QIcon DocumentModel::lockedIcon() const
{
    return d->m_lockedIcon;
}

QIcon DocumentModel::unlockedIcon() const
{
    return d->m_unlockedIcon;
}

QString DocumentModel::Entry::fileName() const {
    return document ? document->filePath() : m_fileName;
}

QString DocumentModel::Entry::displayName() const {
    return document ? document->displayName() : m_displayName;
}

Id DocumentModel::Entry::id() const
{
    return m_id;
}

int DocumentModel::columnCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return 2;
    return 0;
}

int DocumentModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return d->m_documents.count() + 1/*<no document>*/;
    return 0;
}

// TODO remove
QList<IEditor *> DocumentModel::oneEditorForEachOpenedDocument() const
{
    QList<IEditor *> result;
    QMapIterator<IDocument *, QList<IEditor *> > it(d->m_editors);
    while (it.hasNext())
        result << it.next().value().first();
    return result;
}

void DocumentModel::addEditor(IEditor *editor, bool *isNewDocument)
{
    if (!editor)
        return;

    QList<IEditor *> &editorList = d->m_editors[editor->document()];
    bool isNew = editorList.isEmpty();
    if (isNewDocument)
        *isNewDocument = isNew;
    editorList << editor;
    if (isNew) {
        Entry *entry = new Entry;
        entry->document = editor->document();
        entry->m_id = editor->id();
        addEntry(entry);
    }
}

void DocumentModel::addRestoredDocument(const QString &fileName, const QString &displayName, const Id &id)
{
    Entry *entry = new Entry;
    entry->m_fileName = fileName;
    entry->m_displayName = displayName;
    entry->m_id = id;
    addEntry(entry);
}

DocumentModel::Entry *DocumentModel::firstRestoredDocument() const
{
    for (int i = 0; i < d->m_documents.count(); ++i)
        if (!d->m_documents.at(i)->document)
            return d->m_documents.at(i);
    return 0;
}

void DocumentModel::addEntry(Entry *entry)
{
    QString fileName = entry->fileName();

    // replace a non-loaded entry (aka 'restored') if possible
    int previousIndex = indexOfFilePath(fileName);
    if (previousIndex >= 0) {
        if (entry->document && d->m_documents.at(previousIndex)->document == 0) {
            Entry *previousEntry = d->m_documents.at(previousIndex);
            d->m_documents[previousIndex] = entry;
            delete previousEntry;
            connect(entry->document, SIGNAL(changed()), this, SLOT(itemChanged()));
        } else {
            delete entry;
        }
        return;
    }

    int index;
    QString displayName = entry->displayName();
    for (index = 0; index < d->m_documents.count(); ++index) {
        if (displayName < d->m_documents.at(index)->displayName())
            break;
    }
    int row = index + 1/*<no document>*/;
    beginInsertRows(QModelIndex(), row, row);
    d->m_documents.insert(index, entry);
    if (entry->document)
        connect(entry->document, SIGNAL(changed()), this, SLOT(itemChanged()));
    endInsertRows();
}

int DocumentModel::indexOfFilePath(const QString &filePath) const
{
    if (filePath.isEmpty())
        return -1;
    const QString fixedPath = DocumentManager::fixFileName(filePath, DocumentManager::KeepLinks);
    for (int i = 0; i < d->m_documents.count(); ++i) {
        if (DocumentManager::fixFileName(d->m_documents.at(i)->fileName(), DocumentManager::KeepLinks) == fixedPath)
            return i;
    }
    return -1;
}

void DocumentModel::removeEntry(DocumentModel::Entry *entry)
{
    QTC_ASSERT(!entry->document, return); // we wouldn't know what to do with the associated editors
    int index = d->m_documents.indexOf(entry);
    removeDocument(index);
}

void DocumentModel::removeEditor(IEditor *editor, bool *lastOneForDocument)
{
    if (lastOneForDocument)
        *lastOneForDocument = false;
    QTC_ASSERT(editor, return);
    IDocument *document = editor->document();
    QTC_ASSERT(d->m_editors.contains(document), return);
    d->m_editors[document].removeAll(editor);
    if (d->m_editors.value(document).isEmpty()) {
        if (lastOneForDocument)
            *lastOneForDocument = true;
        d->m_editors.remove(document);
        removeDocument(indexOfDocument(document));
    }
}

void DocumentModel::removeDocument(const QString &fileName)
{
    int index = indexOfFilePath(fileName);
    QTC_ASSERT(!d->m_documents.at(index)->document, return); // we wouldn't know what to do with the associated editors
    removeDocument(index);
}

void DocumentModel::removeDocument(int idx)
{
    if (idx < 0)
        return;
    QTC_ASSERT(idx < d->m_documents.size(), return);
    IDocument *document = d->m_documents.at(idx)->document;
    int row = idx + 1/*<no document>*/;
    beginRemoveRows(QModelIndex(), row, row);
    delete d->m_documents.takeAt(idx);
    endRemoveRows();
    if (document)
        disconnect(document, SIGNAL(changed()), this, SLOT(itemChanged()));
}

void DocumentModel::removeAllRestoredDocuments()
{
    for (int i = d->m_documents.count()-1; i >= 0; --i) {
        if (!d->m_documents.at(i)->document) {
            int row = i + 1/*<no document>*/;
            beginRemoveRows(QModelIndex(), row, row);
            delete d->m_documents.takeAt(i);
            endRemoveRows();
        }
    }
}

QList<IEditor *> DocumentModel::editorsForDocument(IDocument *document) const
{
    return d->m_editors.value(document);
}

QList<IEditor *> DocumentModel::editorsForDocuments(const QList<IDocument *> &documents) const
{
    QList<IEditor *> result;
    foreach (IDocument *document, documents)
        result += d->m_editors.value(document);
    return result;
}

int DocumentModel::indexOfDocument(IDocument *document) const
{
    for (int i = 0; i < d->m_documents.count(); ++i)
        if (d->m_documents.at(i)->document == document)
            return i;
    return -1;
}

DocumentModel::Entry *DocumentModel::entryForDocument(IDocument *document) const
{
    int index = indexOfDocument(document);
    if (index < 0)
        return 0;
    return d->m_documents.at(index);
}

QList<IDocument *> DocumentModel::openedDocuments() const
{
    return d->m_editors.keys();
}

IDocument *DocumentModel::documentForFilePath(const QString &filePath) const
{
    int index = indexOfFilePath(filePath);
    if (index < 0)
        return 0;
    return d->m_documents.at(index)->document;
}

QList<IEditor *> DocumentModel::editorsForFilePath(const QString &filePath) const
{
    IDocument *document = documentForFilePath(filePath);
    if (document)
        return editorsForDocument(document);
    return QList<IEditor *>();
}

QModelIndex DocumentModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    if (column < 0 || column > 1 || row < 0 || row >= d->m_documents.count() + 1/*<no document>*/)
        return QModelIndex();
    return createIndex(row, column);
}

DocumentModel::Entry *DocumentModel::documentAtRow(int row) const
{
    int entryIndex = row - 1/*<no document>*/;
    if (entryIndex < 0)
        return 0;
    return d->m_documents[entryIndex];
}

int DocumentModel::documentCount() const
{
    return d->m_documents.count();
}

QVariant DocumentModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || (index.column() != 0 && role < Qt::UserRole))
        return QVariant();
    int entryIndex = index.row() - 1/*<no document>*/;
    if (entryIndex < 0) {
        // <no document> entry
        switch (role) {
        case Qt::DisplayRole:
            return tr("<no document>");
        case Qt::ToolTipRole:
            return tr("No document is selected.");
        default:
            return QVariant();
        }
    }
    const Entry *e = d->m_documents.at(entryIndex);
    switch (role) {
    case Qt::DisplayRole:
        return (e->document && e->document->isModified())
                ? e->displayName() + QLatin1Char('*')
                : e->displayName();
    case Qt::DecorationRole:
    {
        bool showLock = false;
        if (e->document) {
            showLock = e->document->filePath().isEmpty()
                    ? false
                    : e->document->isFileReadOnly();
        } else {
            showLock = !QFileInfo(e->m_fileName).isWritable();
        }
        return showLock ? d->m_lockedIcon : QIcon();
    }
    case Qt::ToolTipRole:
        return e->fileName().isEmpty()
                ? e->displayName()
                : QDir::toNativeSeparators(e->fileName());
    default:
        return QVariant();
    }
    return QVariant();
}

int DocumentModel::rowOfDocument(IDocument *document) const
{
    if (!document)
        return 0 /*<no document>*/;
    return indexOfDocument(document) + 1/*<no document>*/;
}

void DocumentModel::itemChanged()
{
    IDocument *document = qobject_cast<IDocument *>(sender());

    int idx = indexOfDocument(document);
    if (idx < 0)
        return;
    QModelIndex mindex = index(idx + 1/*<no document>*/, 0);
    emit dataChanged(mindex, mindex);
}

QList<DocumentModel::Entry *> DocumentModel::documents() const
{
    return d->m_documents;
}

} // namespace Core
