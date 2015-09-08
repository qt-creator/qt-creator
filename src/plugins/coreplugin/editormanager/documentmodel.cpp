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

#include "documentmodel.h"
#include "ieditor.h"
#include <coreplugin/documentmanager.h>
#include <coreplugin/idocument.h>

#include <utils/algorithm.h>
#include <utils/dropsupport.h>
#include <utils/qtcassert.h>

#include <QAbstractItemModel>
#include <QDir>
#include <QIcon>
#include <QMimeData>
#include <QSet>
#include <QUrl>

namespace Core {

class DocumentModelPrivate : public QAbstractItemModel
{
    Q_OBJECT

public:
    DocumentModelPrivate();
    ~DocumentModelPrivate();

    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QMimeData *mimeData(const QModelIndexList &indexes) const;
    QModelIndex parent(const QModelIndex &/*index*/) const { return QModelIndex(); }
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QModelIndex index(int row, int column = 0, const QModelIndex &parent = QModelIndex()) const;

    Qt::DropActions supportedDragActions() const;
    QStringList mimeTypes() const;

    void addEntry(DocumentModel::Entry *entry);
    void removeDocument(int idx);

    int indexOfFilePath(const Utils::FileName &filePath) const;
    int indexOfDocument(IDocument *document) const;

    bool disambiguateDisplayNames(DocumentModel::Entry *entry);

private slots:
    friend class DocumentModel;
    void itemChanged();

private:
    class DynamicEntry
    {
    public:
        DocumentModel::Entry *entry;
        int pathComponents;

        DynamicEntry(DocumentModel::Entry *e) :
            entry(e),
            pathComponents(0)
        {
        }

        DocumentModel::Entry *operator->() const { return entry; }

        void disambiguate()
        {
            entry->document->setUniqueDisplayName(entry->fileName().fileName(++pathComponents));
        }

        void setNumberedName(int number)
        {
            entry->document->setUniqueDisplayName(QStringLiteral("%1 (%2)")
                                                  .arg(entry->document->displayName())
                                                  .arg(number));
        }
    };

    const QIcon m_lockedIcon;
    const QIcon m_unlockedIcon;

    QList<DocumentModel::Entry *> m_entries;
    QMap<IDocument *, QList<IEditor *> > m_editors;
    QHash<QString, DocumentModel::Entry *> m_entryByFixedPath;
};

class RestoredDocument : public IDocument
{
public:
    bool save(QString *, const QString &, bool) override { return false; }
    QString defaultPath() const override { return filePath().toFileInfo().absolutePath(); }
    QString suggestedFileName() const override { return filePath().fileName(); }
    bool isModified() const override { return false; }
    bool isSaveAsAllowed() const override { return false; }
    bool reload(QString *, ReloadFlag, ChangeType) override { return true; }
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
    document(0),
    isRestored(false)
{
}

DocumentModel::Entry::~Entry()
{
    if (isRestored)
        delete document;
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

Utils::FileName DocumentModel::Entry::fileName() const
{
    return document->filePath();
}

QString DocumentModel::Entry::displayName() const
{
    return document->displayName();
}

QString DocumentModel::Entry::plainDisplayName() const
{
    return document->plainDisplayName();
}

Id DocumentModel::Entry::id() const
{
    return document->id();
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
    entry->document = new RestoredDocument;
    entry->document->setFilePath(Utils::FileName::fromString(fileName));
    entry->document->setPreferredDisplayName(displayName);
    entry->document->setId(id);
    entry->isRestored = true;
    d->addEntry(entry);
}

DocumentModel::Entry *DocumentModel::firstRestoredEntry()
{
    return Utils::findOrDefault(d->m_entries, [](Entry *entry) { return entry->isRestored; });
}

void DocumentModelPrivate::addEntry(DocumentModel::Entry *entry)
{
    const Utils::FileName fileName = entry->fileName();
    QString fixedPath;
    if (!fileName.isEmpty())
        fixedPath = DocumentManager::fixFileName(fileName.toString(), DocumentManager::ResolveLinks);

    // replace a non-loaded entry (aka 'restored') if possible
    int previousIndex = indexOfFilePath(fileName);
    if (previousIndex >= 0) {
        DocumentModel::Entry *previousEntry = m_entries.at(previousIndex);
        const bool replace = !entry->isRestored && previousEntry->isRestored;
        if (replace) {
            delete previousEntry;
            m_entries[previousIndex] = entry;
            if (!fixedPath.isEmpty())
                m_entryByFixedPath[fixedPath] = entry;
        } else {
            delete entry;
            entry = previousEntry;
        }
        previousEntry = 0;
        disambiguateDisplayNames(entry);
        if (replace)
            connect(entry->document, SIGNAL(changed()), this, SLOT(itemChanged()));
        return;
    }

    int index;
    const QString displayName = entry->plainDisplayName();
    for (index = 0; index < m_entries.count(); ++index) {
        int cmp = displayName.localeAwareCompare(m_entries.at(index)->plainDisplayName());
        if (cmp < 0)
            break;
        if (cmp == 0 && fileName < d->m_entries.at(index)->fileName())
            break;
    }
    int row = index + 1/*<no document>*/;
    beginInsertRows(QModelIndex(), row, row);
    m_entries.insert(index, entry);
    disambiguateDisplayNames(entry);
    if (!fixedPath.isEmpty())
        m_entryByFixedPath[fixedPath] = entry;
    connect(entry->document, SIGNAL(changed()), this, SLOT(itemChanged()));
    endInsertRows();
}

bool DocumentModelPrivate::disambiguateDisplayNames(DocumentModel::Entry *entry)
{
    const QString displayName = entry->plainDisplayName();
    int minIdx = -1, maxIdx = -1;

    QList<DynamicEntry> dups;

    for (int i = 0, total = m_entries.count(); i < total; ++i) {
        DocumentModel::Entry *e = m_entries.at(i);
        if (e == entry || e->plainDisplayName() == displayName) {
            e->document->setUniqueDisplayName(QString());
            dups += DynamicEntry(e);
            maxIdx = i;
            if (minIdx < 0)
                minIdx = i;
        }
    }

    const int dupsCount = dups.count();
    if (dupsCount == 0)
        return false;

    if (dupsCount > 1) {
        int serial = 0;
        int count = 0;
        // increase uniqueness unless no dups are left
        forever {
            bool seenDups = false;
            for (int i = 0; i < dupsCount - 1; ++i) {
                DynamicEntry &e = dups[i];
                const Utils::FileName myFileName = e->document->filePath();
                if (e->document->isTemporary() || myFileName.isEmpty() || count > 10) {
                    // path-less entry, append number
                    e.setNumberedName(++serial);
                    continue;
                }
                for (int j = i + 1; j < dupsCount; ++j) {
                    DynamicEntry &e2 = dups[j];
                    if (e->displayName() == e2->displayName()) {
                        const Utils::FileName otherFileName = e2->document->filePath();
                        if (otherFileName.isEmpty())
                            continue;
                        seenDups = true;
                        e2.disambiguate();
                        if (j > maxIdx)
                            maxIdx = j;
                    }
                }
                if (seenDups) {
                    e.disambiguate();
                    ++count;
                    break;
                }
            }
            if (!seenDups)
                break;
        }
    }

    emit dataChanged(index(minIdx + 1, 0), index(maxIdx + 1, 0));
    return true;
}

int DocumentModelPrivate::indexOfFilePath(const Utils::FileName &filePath) const
{
    if (filePath.isEmpty())
        return -1;
    const QString fixedPath = DocumentManager::fixFileName(filePath.toString(),
                                                           DocumentManager::ResolveLinks);
    return m_entries.indexOf(m_entryByFixedPath.value(fixedPath));
}

void DocumentModel::removeEntry(DocumentModel::Entry *entry)
{
    // For non restored entries, we wouldn't know what to do with the associated editors
    QTC_ASSERT(entry->isRestored, return);
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
    int index = d->indexOfFilePath(Utils::FileName::fromString(fileName));
    // For non restored entries, we wouldn't know what to do with the associated editors
    QTC_ASSERT(d->m_entries.at(index)->isRestored, return);
    d->removeDocument(index);
}

void DocumentModelPrivate::removeDocument(int idx)
{
    if (idx < 0)
        return;
    QTC_ASSERT(idx < d->m_entries.size(), return);
    int row = idx + 1/*<no document>*/;
    beginRemoveRows(QModelIndex(), row, row);
    DocumentModel::Entry *entry = d->m_entries.takeAt(idx);
    endRemoveRows();

    const QString fileName = entry->fileName().toString();
    if (!fileName.isEmpty()) {
        const QString fixedPath = DocumentManager::fixFileName(fileName,
                                                               DocumentManager::ResolveLinks);
        m_entryByFixedPath.remove(fixedPath);
    }
    disconnect(entry->document, SIGNAL(changed()), this, SLOT(itemChanged()));
    disambiguateDisplayNames(entry);
    delete entry;
}

void DocumentModel::removeAllRestoredEntries()
{
    for (int i = d->m_entries.count()-1; i >= 0; --i) {
        if (d->m_entries.at(i)->isRestored) {
            int row = i + 1/*<no document>*/;
            d->beginRemoveRows(QModelIndex(), row, row);
            delete d->m_entries.takeAt(i);
            d->endRemoveRows();
        }
    }
    QSet<QString> displayNames;
    foreach (DocumentModel::Entry *entry, d->m_entries) {
        const QString displayName = entry->plainDisplayName();
        if (displayNames.contains(displayName))
            continue;
        displayNames.insert(displayName);
        d->disambiguateDisplayNames(entry);
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
    const int index = d->indexOfFilePath(Utils::FileName::fromString(filePath));
    if (index < 0)
        return 0;
    return d->m_entries.at(index)->document;
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

Qt::DropActions DocumentModelPrivate::supportedDragActions() const
{
    return Qt::MoveAction;
}

QStringList DocumentModelPrivate::mimeTypes() const
{
    return Utils::DropSupport::mimeTypesForFilePaths();
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
    case Qt::DisplayRole: {
        QString name = e->displayName();
        if (e->document->isModified())
            name += QLatin1Char('*');
        return name;
    }
    case Qt::DecorationRole:
        return e->document->isFileReadOnly() ? m_lockedIcon : QIcon();
    case Qt::ToolTipRole:
        return e->fileName().isEmpty() ? e->displayName() : e->fileName().toUserOutput();
    default:
        return QVariant();
    }
    return QVariant();
}

Qt::ItemFlags DocumentModelPrivate::flags(const QModelIndex &index) const
{
    const DocumentModel::Entry *e = DocumentModel::entryAtRow(index.row());
    if (!e || e->fileName().isEmpty())
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    return Qt::ItemIsDragEnabled | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QMimeData *DocumentModelPrivate::mimeData(const QModelIndexList &indexes) const
{
    auto data = new Utils::DropMimeData;
    foreach (const QModelIndex &index, indexes) {
        const DocumentModel::Entry *e = DocumentModel::entryAtRow(index.row());
        if (!e || e->fileName().isEmpty())
            continue;
        data->addFile(e->fileName().toString());
    }
    return data;
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
    const QString fileName = document->filePath().toString();
    QString fixedPath;
    if (!fileName.isEmpty())
        fixedPath = DocumentManager::fixFileName(fileName, DocumentManager::ResolveLinks);
    DocumentModel::Entry *entry = d->m_entries.at(idx);
    bool found = false;
    // The entry's fileName might have changed, so find the previous fileName that was associated
    // with it and remove it, then add the new fileName.
    for (auto it = m_entryByFixedPath.begin(), end = m_entryByFixedPath.end(); it != end; ++it) {
        if (it.value() == entry) {
            found = true;
            if (it.key() != fixedPath) {
                m_entryByFixedPath.remove(it.key());
                if (!fixedPath.isEmpty())
                    m_entryByFixedPath[fixedPath] = entry;
            }
            break;
        }
    }
    if (!found && !fixedPath.isEmpty())
        m_entryByFixedPath[fixedPath] = entry;
    if (!disambiguateDisplayNames(d->m_entries.at(idx))) {
        QModelIndex mindex = index(idx + 1/*<no document>*/, 0);
        emit dataChanged(mindex, mindex);
    }
}

QList<DocumentModel::Entry *> DocumentModel::entries()
{
    return d->m_entries;
}

} // namespace Core

#include "documentmodel.moc"
