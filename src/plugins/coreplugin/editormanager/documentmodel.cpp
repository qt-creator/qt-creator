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

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QDir>
#include <QIcon>

namespace Core {

class DocumentModelPrivate : public QAbstractItemModel
{
    Q_OBJECT

public:
    DocumentModelPrivate();
    ~DocumentModelPrivate();

    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QModelIndex parent(const QModelIndex &/*index*/) const { return QModelIndex(); }
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QModelIndex index(int row, int column = 0, const QModelIndex &parent = QModelIndex()) const;

    void addEntry(DocumentModel::Entry *entry);
    void removeDocument(int idx);

    int indexOfFilePath(const QString &filePath) const;
    int indexOfDocument(IDocument *document) const;

private slots:
    friend class DocumentModel;
    void itemChanged();

private:
    const QIcon m_lockedIcon;
    const QIcon m_unlockedIcon;

    QList<DocumentModel::Entry *> m_entries;
    QMap<IDocument *, QList<IEditor *> > m_editors;
};

DocumentModelPrivate::DocumentModelPrivate() :
    m_lockedIcon(QLatin1String(":/core/images/locked.png")),
    m_unlockedIcon(QLatin1String(":/core/images/unlocked.png"))
{
}

DocumentModelPrivate::~DocumentModelPrivate()
{
    qDeleteAll(m_entries);
}

static DocumentModelPrivate *d;

DocumentModel::Entry::Entry() :
    document(0)
{
}

DocumentModel::DocumentModel()
{
}

void DocumentModel::init()
{
    d = new DocumentModelPrivate;
}

void DocumentModel::destroy()
{
    delete d;
}

QIcon DocumentModel::lockedIcon()
{
    return d->m_lockedIcon;
}

QIcon DocumentModel::unlockedIcon()
{
    return d->m_unlockedIcon;
}

QAbstractItemModel *DocumentModel::model()
{
    return d;
}

QString DocumentModel::Entry::fileName() const
{
    return document ? document->filePath() : m_fileName;
}

QString DocumentModel::Entry::displayName() const
{
    return document ? document->displayName() : m_displayName;
}

Id DocumentModel::Entry::id() const
{
    return document ? document->id() : m_id;
}

int DocumentModelPrivate::columnCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return 2;
    return 0;
}

int DocumentModelPrivate::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return m_entries.count() + 1/*<no document>*/;
    return 0;
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
        d->addEntry(entry);
    }
}

void DocumentModel::addRestoredDocument(const QString &fileName, const QString &displayName, Id id)
{
    Entry *entry = new Entry;
    entry->m_fileName = fileName;
    entry->m_displayName = displayName;
    entry->m_id = id;
    d->addEntry(entry);
}

DocumentModel::Entry *DocumentModel::firstRestoredEntry()
{
    return Utils::findOrDefault(d->m_entries, [](Entry *entry) { return !entry->document; });
}

void DocumentModelPrivate::addEntry(DocumentModel::Entry *entry)
{
    QString fileName = entry->fileName();

    // replace a non-loaded entry (aka 'restored') if possible
    int previousIndex = indexOfFilePath(fileName);
    if (previousIndex >= 0) {
        if (entry->document && m_entries.at(previousIndex)->document == 0) {
            DocumentModel::Entry *previousEntry = m_entries.at(previousIndex);
            m_entries[previousIndex] = entry;
            delete previousEntry;
            connect(entry->document, SIGNAL(changed()), this, SLOT(itemChanged()));
        } else {
            delete entry;
        }
        return;
    }

    int index;
    QString displayName = entry->displayName();
    for (index = 0; index < m_entries.count(); ++index) {
        if (displayName.localeAwareCompare(m_entries.at(index)->displayName()) < 0)
            break;
    }
    int row = index + 1/*<no document>*/;
    beginInsertRows(QModelIndex(), row, row);
    m_entries.insert(index, entry);
    if (entry->document)
        connect(entry->document, SIGNAL(changed()), this, SLOT(itemChanged()));
    endInsertRows();
}

int DocumentModelPrivate::indexOfFilePath(const QString &filePath) const
{
    if (filePath.isEmpty())
        return -1;
    const QString fixedPath = DocumentManager::fixFileName(filePath, DocumentManager::KeepLinks);
    return Utils::indexOf(m_entries, [&fixedPath](DocumentModel::Entry *entry) {
        return DocumentManager::fixFileName(entry->fileName(),
                                            DocumentManager::KeepLinks) == fixedPath;
    });
}

void DocumentModel::removeEntry(DocumentModel::Entry *entry)
{
    QTC_ASSERT(!entry->document, return); // we wouldn't know what to do with the associated editors
    int index = d->m_entries.indexOf(entry);
    d->removeDocument(index);
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
        d->removeDocument(indexOfDocument(document));
    }
}

void DocumentModel::removeDocument(const QString &fileName)
{
    int index = d->indexOfFilePath(fileName);
    QTC_ASSERT(!d->m_entries.at(index)->document, return); // we wouldn't know what to do with the associated editors
    d->removeDocument(index);
}

void DocumentModelPrivate::removeDocument(int idx)
{
    if (idx < 0)
        return;
    QTC_ASSERT(idx < d->m_entries.size(), return);
    IDocument *document = d->m_entries.at(idx)->document;
    int row = idx + 1/*<no document>*/;
    beginRemoveRows(QModelIndex(), row, row);
    delete d->m_entries.takeAt(idx);
    endRemoveRows();
    if (document)
        disconnect(document, SIGNAL(changed()), this, SLOT(itemChanged()));
}

void DocumentModel::removeAllRestoredEntries()
{
    for (int i = d->m_entries.count()-1; i >= 0; --i) {
        if (!d->m_entries.at(i)->document) {
            int row = i + 1/*<no document>*/;
            d->beginRemoveRows(QModelIndex(), row, row);
            delete d->m_entries.takeAt(i);
            d->endRemoveRows();
        }
    }
}

QList<IEditor *> DocumentModel::editorsForDocument(IDocument *document)
{
    return d->m_editors.value(document);
}

QList<IEditor *> DocumentModel::editorsForOpenedDocuments()
{
    return editorsForDocuments(openedDocuments());
}

QList<IEditor *> DocumentModel::editorsForDocuments(const QList<IDocument *> &documents)
{
    QList<IEditor *> result;
    foreach (IDocument *document, documents)
        result += d->m_editors.value(document);
    return result;
}

int DocumentModel::indexOfDocument(IDocument *document)
{
    return d->indexOfDocument(document);
}

int DocumentModelPrivate::indexOfDocument(IDocument *document) const
{
    return Utils::indexOf(m_entries, [&document](DocumentModel::Entry *entry) {
        return entry->document == document;
    });
}

DocumentModel::Entry *DocumentModel::entryForDocument(IDocument *document)
{
    return Utils::findOrDefault(d->m_entries,
                                [&document](Entry *entry) { return entry->document == document; });
}

QList<IDocument *> DocumentModel::openedDocuments()
{
    return d->m_editors.keys();
}

IDocument *DocumentModel::documentForFilePath(const QString &filePath)
{
    Entry *e = Utils::findOrDefault(d->m_entries, Utils::equal(&Entry::fileName, filePath));
    return e ? e->document : 0;
}

QList<IEditor *> DocumentModel::editorsForFilePath(const QString &filePath)
{
    IDocument *document = documentForFilePath(filePath);
    if (document)
        return editorsForDocument(document);
    return QList<IEditor *>();
}

QModelIndex DocumentModelPrivate::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    if (column < 0 || column > 1 || row < 0 || row >= m_entries.count() + 1/*<no document>*/)
        return QModelIndex();
    return createIndex(row, column);
}

DocumentModel::Entry *DocumentModel::entryAtRow(int row)
{
    int entryIndex = row - 1/*<no document>*/;
    if (entryIndex < 0)
        return 0;
    return d->m_entries[entryIndex];
}

int DocumentModel::entryCount()
{
    return d->m_entries.count();
}

QVariant DocumentModelPrivate::data(const QModelIndex &index, int role) const
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
    const DocumentModel::Entry *e = m_entries.at(entryIndex);
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
        return showLock ? m_lockedIcon : QIcon();
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

int DocumentModel::rowOfDocument(IDocument *document)
{
    if (!document)
        return 0 /*<no document>*/;
    return indexOfDocument(document) + 1/*<no document>*/;
}

void DocumentModelPrivate::itemChanged()
{
    IDocument *document = qobject_cast<IDocument *>(sender());

    int idx = indexOfDocument(document);
    if (idx < 0)
        return;
    QModelIndex mindex = index(idx + 1/*<no document>*/, 0);
    emit dataChanged(mindex, mindex);
}

QList<DocumentModel::Entry *> DocumentModel::entries()
{
    return d->m_entries;
}

} // namespace Core

#include "documentmodel.moc"
