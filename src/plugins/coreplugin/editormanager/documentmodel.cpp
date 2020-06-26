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

#include "documentmodel.h"
#include "documentmodel_p.h"

#include "ieditor.h"
#include <coreplugin/documentmanager.h>
#include <coreplugin/idocument.h>

#include <utils/algorithm.h>
#include <utils/dropsupport.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <QAbstractItemModel>
#include <QDir>
#include <QIcon>
#include <QMimeData>
#include <QSet>
#include <QUrl>

using namespace Utils;

static Core::Internal::DocumentModelPrivate *d;

namespace Core {
namespace Internal {

namespace {
bool compare(const DocumentModel::Entry *e1, const DocumentModel::Entry *e2)
{
    // Pinned files should go at the top.
    if (e1->pinned != e2->pinned)
        return e1->pinned;

    const int cmp = e1->plainDisplayName().localeAwareCompare(e2->plainDisplayName());
    return (cmp < 0) || (cmp == 0 && e1->fileName() < e2->fileName());
}

// Return a pair of indices. The first is the index that needs to be removed or -1 if no removal
// is necessary. The second is the index to add the entry into, or -1 if no addition is necessary.
// If the entry does not need to be moved, then (-1, -1) will be returned as no action is needed.
std::pair<int, int> positionEntry(const QList<DocumentModel::Entry *> &list,
                                  DocumentModel::Entry *entry)
{
    const int to_remove = list.indexOf(entry);

    const QList<DocumentModel::Entry *> toSort
            = Utils::filtered(list, [entry](DocumentModel::Entry *e) { return e != entry; });

    const auto begin = std::begin(toSort);
    const auto end = std::end(toSort);
    const auto to_insert
            = static_cast<int>(std::distance(begin, std::lower_bound(begin, end, entry, &compare)));
    if (to_remove == to_insert)
        return std::make_pair(-1, -1);
    return std::make_pair(to_remove, to_insert);
}
} // namespace

DocumentModelPrivate::~DocumentModelPrivate()
{
    qDeleteAll(m_entries);
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

void DocumentModelPrivate::addEntry(DocumentModel::Entry *entry)
{
    const Utils::FilePath fileName = entry->fileName();
    QString fixedPath;
    if (!fileName.isEmpty())
        fixedPath = DocumentManager::filePathKey(fileName.toString(), DocumentManager::ResolveLinks);

    // replace a non-loaded entry (aka 'suspended') if possible
    DocumentModel::Entry *previousEntry = DocumentModel::entryForFilePath(fileName);
    if (previousEntry) {
        const bool replace = !entry->isSuspended && previousEntry->isSuspended;
        if (replace) {
            previousEntry->isSuspended = false;
            delete previousEntry->document;
            previousEntry->document = entry->document;
            connect(previousEntry->document, &IDocument::changed,
                    this, [this, document = previousEntry->document] { itemChanged(document); });
        }
        delete entry;
        entry = nullptr;
        disambiguateDisplayNames(previousEntry);
        return;
    }

    auto positions = positionEntry(m_entries, entry);
    // Do not remove anything (new entry), insert somewhere:
    QTC_CHECK(positions.first == -1 && positions.second >= 0);

    int row = positions.second + 1/*<no document>*/;
    beginInsertRows(QModelIndex(), row, row);
    m_entries.insert(positions.second, entry);
    disambiguateDisplayNames(entry);
    if (!fixedPath.isEmpty())
        m_entryByFixedPath[fixedPath] = entry;
    connect(entry->document, &IDocument::changed, this, [this, document = entry->document] {
        itemChanged(document);
    });
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
                const Utils::FilePath myFileName = e->document->filePath();
                if (e->document->isTemporary() || myFileName.isEmpty() || count > 10) {
                    // path-less entry, append number
                    e.setNumberedName(++serial);
                    continue;
                }
                for (int j = i + 1; j < dupsCount; ++j) {
                    DynamicEntry &e2 = dups[j];
                    if (e->displayName().compare(e2->displayName(), Utils::HostOsInfo::fileNameCaseSensitivity()) == 0) {
                        const Utils::FilePath otherFileName = e2->document->filePath();
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

void DocumentModelPrivate::setPinned(DocumentModel::Entry *entry, bool pinned)
{
    if (entry->pinned == pinned)
        return;

    entry->pinned = pinned;
    // Ensure that this entry is re-sorted in the list of open documents
    // now that its pinned state has changed.
    d->itemChanged(entry->document);
}

QIcon DocumentModelPrivate::lockedIcon()
{
    const static QIcon icon = Utils::Icons::LOCKED.icon();
    return icon;
}

QIcon DocumentModelPrivate::pinnedIcon()
{
    const static QIcon icon = Utils::Icons::PINNED.icon();
    return icon;
}

Utils::optional<int> DocumentModelPrivate::indexOfFilePath(const Utils::FilePath &filePath) const
{
    if (filePath.isEmpty())
        return Utils::nullopt;
    const QString fixedPath = DocumentManager::filePathKey(filePath.toString(),
                                                           DocumentManager::ResolveLinks);
    const int index = m_entries.indexOf(m_entryByFixedPath.value(fixedPath));
    if (index < 0)
        return Utils::nullopt;
    return index;
}

void DocumentModelPrivate::removeDocument(int idx)
{
    if (idx < 0)
        return;
    QTC_ASSERT(idx < m_entries.size(), return);
    int row = idx + 1/*<no document>*/;
    beginRemoveRows(QModelIndex(), row, row);
    DocumentModel::Entry *entry = m_entries.takeAt(idx);
    endRemoveRows();

    const QString fileName = entry->fileName().toString();
    if (!fileName.isEmpty()) {
        const QString fixedPath = DocumentManager::filePathKey(fileName,
                                                               DocumentManager::ResolveLinks);
        m_entryByFixedPath.remove(fixedPath);
    }
    disconnect(entry->document, &IDocument::changed, this, nullptr);
    disambiguateDisplayNames(entry);
    delete entry;
}

Utils::optional<int> DocumentModelPrivate::indexOfDocument(IDocument *document) const
{
    const int index = Utils::indexOf(m_entries, [&document](DocumentModel::Entry *entry) {
        return entry->document == document;
    });
    if (index < 0)
        return Utils::nullopt;
    return index;
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

QVariant DocumentModelPrivate::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || (index.column() != 0 && role < Qt::UserRole))
        return QVariant();
    const DocumentModel::Entry *entry = DocumentModel::entryAtRow(index.row());
    if (!entry) {
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
    switch (role) {
    case Qt::DisplayRole: {
        QString name = entry->displayName();
        if (entry->document->isModified())
            name += QLatin1Char('*');
        return name;
    }
    case Qt::DecorationRole:
        if (entry->document->isFileReadOnly())
            return lockedIcon();
        if (entry->pinned)
            return pinnedIcon();
        return QIcon();
    case Qt::ToolTipRole:
        return entry->fileName().isEmpty() ? entry->displayName() : entry->fileName().toUserOutput();
    default:
        break;
    }
    return QVariant();
}

void DocumentModelPrivate::itemChanged(IDocument *document)
{
    const Utils::optional<int> idx = indexOfDocument(document);
    if (!idx)
        return;
    const QString fileName = document->filePath().toString();
    QString fixedPath;
    if (!fileName.isEmpty())
        fixedPath = DocumentManager::filePathKey(fileName, DocumentManager::ResolveLinks);
    DocumentModel::Entry *entry = m_entries.at(idx.value());
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

    if (!disambiguateDisplayNames(m_entries.at(idx.value()))) {
        QModelIndex mindex = index(idx.value() + 1/*<no document>*/, 0);
        emit dataChanged(mindex, mindex);
    }

    // Make sure the entries stay sorted:
    auto positions = positionEntry(m_entries, entry);
    if (positions.first >= 0 && positions.second >= 0) {
        // Entry did move: update its position.

        // Account for the <no document> entry.
        static const int noDocumentEntryOffset = 1;
        const int fromIndex = positions.first + noDocumentEntryOffset;
        const int toIndex = positions.second + noDocumentEntryOffset;
        // Account for the weird requirements of beginMoveRows().
        const int effectiveToIndex = toIndex > fromIndex ? toIndex + 1 : toIndex;
        beginMoveRows(QModelIndex(), fromIndex, fromIndex, QModelIndex(), effectiveToIndex);

        m_entries.move(fromIndex - 1, toIndex - 1);

        endMoveRows();
    } else {
        // Nothing to remove or add: The entry did not move.
        QTC_CHECK(positions.first == -1 && positions.second == -1);
    }
}

void DocumentModelPrivate::addEditor(IEditor *editor, bool *isNewDocument)
{
    if (!editor)
        return;

    QList<IEditor *> &editorList = d->m_editors[editor->document()];
    bool isNew = editorList.isEmpty();
    if (isNewDocument)
        *isNewDocument = isNew;
    editorList << editor;
    if (isNew) {
        auto entry = new DocumentModel::Entry;
        entry->document = editor->document();
        d->addEntry(entry);
    }
}

/*!
    \class Core::DocumentModel
    \inmodule QtCreator
    \internal
*/

/*!
    \class Core::DocumentModel::Entry
    \inmodule QtCreator
    \internal
*/

DocumentModel::Entry *DocumentModelPrivate::addSuspendedDocument(const QString &fileName,
                                                                 const QString &displayName,
                                                                 Id id)
{
    auto entry = new DocumentModel::Entry;
    entry->document = new IDocument;
    entry->document->setFilePath(Utils::FilePath::fromString(fileName));
    entry->document->setPreferredDisplayName(displayName);
    entry->document->setId(id);
    entry->isSuspended = true;
    d->addEntry(entry);
    return entry;
}

DocumentModel::Entry *DocumentModelPrivate::firstSuspendedEntry()
{
    return Utils::findOrDefault(d->m_entries, [](DocumentModel::Entry *entry) { return entry->isSuspended; });
}

/*!
    Removes an editor from the list of open editors for its entry. If the editor is the last
    one, the entry is put into suspended state.
    Returns the affected entry.
*/
DocumentModel::Entry *DocumentModelPrivate::removeEditor(IEditor *editor)
{
    QTC_ASSERT(editor, return nullptr);
    IDocument *document = editor->document();
    QTC_ASSERT(d->m_editors.contains(document), return nullptr);
    d->m_editors[document].removeAll(editor);
    DocumentModel::Entry *entry = DocumentModel::entryForDocument(document);
    QTC_ASSERT(entry, return nullptr);
    if (d->m_editors.value(document).isEmpty()) {
        d->m_editors.remove(document);
        entry->document = new IDocument;
        entry->document->setFilePath(document->filePath());
        entry->document->setPreferredDisplayName(document->preferredDisplayName());
        entry->document->setUniqueDisplayName(document->uniqueDisplayName());
        entry->document->setId(document->id());
        entry->isSuspended = true;
    }
    return entry;
}

void DocumentModelPrivate::removeEntry(DocumentModel::Entry *entry)
{
    // For non suspended entries, we wouldn't know what to do with the associated editors
    QTC_ASSERT(entry->isSuspended, return);
    int index = d->m_entries.indexOf(entry);
    d->removeDocument(index);
}

void DocumentModelPrivate::removeAllSuspendedEntries(PinnedFileRemovalPolicy pinnedFileRemovalPolicy)
{
    for (int i = d->m_entries.count()-1; i >= 0; --i) {
        const DocumentModel::Entry *entry = d->m_entries.at(i);
        if (!entry->isSuspended)
            continue;

        if (pinnedFileRemovalPolicy == DoNotRemovePinnedFiles && entry->pinned)
            continue;

        int row = i + 1/*<no document>*/;
        d->beginRemoveRows(QModelIndex(), row, row);
        delete d->m_entries.takeAt(i);
        d->endRemoveRows();
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

DocumentModelPrivate::DynamicEntry::DynamicEntry(DocumentModel::Entry *e) :
    entry(e),
    pathComponents(0)
{
}

DocumentModel::Entry *DocumentModelPrivate::DynamicEntry::operator->() const
{
    return entry;
}

void DocumentModelPrivate::DynamicEntry::disambiguate()
{
    const QString display = entry->fileName().fileNameWithPathComponents(++pathComponents);
    entry->document->setUniqueDisplayName(display);
}

void DocumentModelPrivate::DynamicEntry::setNumberedName(int number)
{
    entry->document->setUniqueDisplayName(QStringLiteral("%1 (%2)")
                                          .arg(entry->document->displayName())
                                          .arg(number));
}

} // Internal

DocumentModel::Entry::Entry() :
    document(nullptr),
    isSuspended(false),
    pinned(false)
{
}

DocumentModel::Entry::~Entry()
{
    if (isSuspended)
        delete document;
}

DocumentModel::DocumentModel() = default;

void DocumentModel::init()
{
    d = new Internal::DocumentModelPrivate;
}

void DocumentModel::destroy()
{
    delete d;
}

QIcon DocumentModel::lockedIcon()
{
    return Internal::DocumentModelPrivate::lockedIcon();
}

QAbstractItemModel *DocumentModel::model()
{
    return d;
}

Utils::FilePath DocumentModel::Entry::fileName() const
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

Utils::optional<int> DocumentModel::indexOfDocument(IDocument *document)
{
    return d->indexOfDocument(document);
}

Utils::optional<int> DocumentModel::indexOfFilePath(const Utils::FilePath &filePath)
{
    return d->indexOfFilePath(filePath);
}

DocumentModel::Entry *DocumentModel::entryForDocument(IDocument *document)
{
    return Utils::findOrDefault(d->m_entries,
                                [&document](Entry *entry) { return entry->document == document; });
}

DocumentModel::Entry *DocumentModel::entryForFilePath(const Utils::FilePath &filePath)
{
    const Utils::optional<int> index = d->indexOfFilePath(filePath);
    if (!index)
        return nullptr;
    return d->m_entries.at(*index);
}

QList<IDocument *> DocumentModel::openedDocuments()
{
    return d->m_editors.keys();
}

IDocument *DocumentModel::documentForFilePath(const QString &filePath)
{
    const Utils::optional<int> index = d->indexOfFilePath(Utils::FilePath::fromString(filePath));
    if (!index)
        return nullptr;
    return d->m_entries.at(*index)->document;
}

QList<IEditor *> DocumentModel::editorsForFilePath(const QString &filePath)
{
    IDocument *document = documentForFilePath(filePath);
    if (document)
        return editorsForDocument(document);
    return QList<IEditor *>();
}

DocumentModel::Entry *DocumentModel::entryAtRow(int row)
{
    int entryIndex = row - 1/*<no document>*/;
    if (entryIndex < 0)
        return nullptr;
    return d->m_entries[entryIndex];
}

int DocumentModel::entryCount()
{
    return d->m_entries.count();
}

Utils::optional<int> DocumentModel::rowOfDocument(IDocument *document)
{
    if (!document)
        return 0 /*<no document>*/;
    const Utils::optional<int> index = indexOfDocument(document);
    if (index)
        return *index + 1/*correction for <no document>*/;
    return Utils::nullopt;
}

QList<DocumentModel::Entry *> DocumentModel::entries()
{
    return d->m_entries;
}

} // namespace Core
