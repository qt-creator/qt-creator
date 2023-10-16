// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "documentmodel.h"
#include "documentmodel_p.h"

#include "ieditor.h"
#include "../coreplugintr.h"
#include "../documentmanager.h"
#include "../idocument.h"

#include <utils/algorithm.h>
#include <utils/dropsupport.h>
#include <utils/filepath.h>
#include <utils/fileutils.h>
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
    return (cmp < 0) || (cmp == 0 && e1->filePath() < e2->filePath());
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
        return {-1, -1};
    return {to_remove, to_insert};
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

DocumentModel::Entry *DocumentModelPrivate::addEntry(DocumentModel::Entry *entry)
{
    const Utils::FilePath filePath = entry->filePath();

    // replace a non-loaded entry (aka 'suspended') if possible
    DocumentModel::Entry *previousEntry = DocumentModel::entryForFilePath(filePath);
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
        disambiguateDisplayNames(previousEntry);
        return nullptr;
    }

    auto positions = positionEntry(m_entries, entry);
    // Do not remove anything (new entry), insert somewhere:
    QTC_CHECK(positions.first == -1 && positions.second >= 0);

    int row = positions.second + 1/*<no document>*/;
    beginInsertRows(QModelIndex(), row, row);
    m_entries.insert(positions.second, entry);
    FilePath fixedPath = DocumentManager::filePathKey(filePath, DocumentManager::ResolveLinks);
    if (!fixedPath.isEmpty())
        m_entryByFixedPath[fixedPath] = entry;
    connect(entry->document, &IDocument::changed, this, [this, document = entry->document] {
        itemChanged(document);
    });
    endInsertRows();
    disambiguateDisplayNames(entry);
    return entry;
}

bool DocumentModelPrivate::disambiguateDisplayNames(DocumentModel::Entry *entry)
{
    const QString displayName = entry->plainDisplayName();

    QList<DocumentModel::Entry *> dups;
    FilePaths paths;
    int minIdx = m_entries.count();
    int maxIdx = 0;

    for (int i = 0; i < m_entries.count(); ++i) {
        DocumentModel::Entry *e = m_entries.at(i);
        if (e == entry || e->plainDisplayName() == displayName) {
            if (minIdx > i)
                minIdx = i;
            if (maxIdx < i)
                maxIdx = i;
            dups += e;
            if (!e->filePath().isEmpty())
                paths += e->filePath();
        }
    }

    const auto triggerDataChanged = [this](int minIdx, int maxIdx) {
        const QModelIndex idxMin = index(minIdx + 1 /*<no document>*/, 0);
        const QModelIndex idxMax = index(maxIdx + 1 /*<no document>*/, 0);
        if (idxMin.isValid() && idxMax.isValid())
            emit dataChanged(idxMin, idxMax);
    };

    if (dups.count() == 1) {
        dups.at(0)->document->setUniqueDisplayName({});
        triggerDataChanged(minIdx, maxIdx);
        return false;
    }

    const FilePath commonAncestor = FileUtils::commonPath(paths);

    int countWithoutFilePath = 0;
    for (DocumentModel::Entry *e : std::as_const(dups)) {
        const FilePath path = e->filePath();
        if (path.isEmpty()) {
            e->document->setUniqueDisplayName(QStringLiteral("%1 (%2)")
                                                  .arg(e->document->displayName())
                                                  .arg(++countWithoutFilePath));
            continue;
        }
        const QString uniqueDisplayName = path.relativeChildPath(commonAncestor).toString();
        if (uniqueDisplayName != "" && e->document->uniqueDisplayName() != uniqueDisplayName) {
            e->document->setUniqueDisplayName(uniqueDisplayName);
        }
    }
    triggerDataChanged(minIdx, maxIdx);
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

std::optional<int> DocumentModelPrivate::indexOfFilePath(const Utils::FilePath &filePath) const
{
    if (filePath.isEmpty())
        return std::nullopt;
    const FilePath fixedPath = DocumentManager::filePathKey(filePath, DocumentManager::ResolveLinks);
    const int index = m_entries.indexOf(m_entryByFixedPath.value(fixedPath));
    if (index < 0)
        return std::nullopt;
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

    const FilePath fixedPath = DocumentManager::filePathKey(entry->filePath(),
                                                            DocumentManager::ResolveLinks);
    if (!fixedPath.isEmpty())
        m_entryByFixedPath.remove(fixedPath);
    disconnect(entry->document, &IDocument::changed, this, nullptr);
    disambiguateDisplayNames(entry);
    delete entry;
}

std::optional<int> DocumentModelPrivate::indexOfDocument(IDocument *document) const
{
    const int index = Utils::indexOf(m_entries, [&document](DocumentModel::Entry *entry) {
        return entry->document == document;
    });
    if (index < 0)
        return std::nullopt;
    return index;
}

Qt::ItemFlags DocumentModelPrivate::flags(const QModelIndex &index) const
{
    const DocumentModel::Entry *e = DocumentModel::entryAtRow(index.row());
    if (!e || e->filePath().isEmpty())
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    return Qt::ItemIsDragEnabled | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QMimeData *DocumentModelPrivate::mimeData(const QModelIndexList &indexes) const
{
    auto data = new Utils::DropMimeData;
    for (const QModelIndex &index : indexes) {
        const DocumentModel::Entry *e = DocumentModel::entryAtRow(index.row());
        if (!e || e->filePath().isEmpty())
            continue;
        data->addFile(e->filePath());
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
            return Tr::tr("<no document>");
        case Qt::ToolTipRole:
            return Tr::tr("No document is selected.");
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
        return QVariant();
    case Qt::ToolTipRole:
        return entry->filePath().isEmpty() ? entry->displayName() : entry->filePath().toUserOutput();
    case DocumentModel::FilePathRole:
        return entry->filePath().toVariant();
    default:
        break;
    }
    return QVariant();
}

void DocumentModelPrivate::itemChanged(IDocument *document)
{
    const std::optional<int> idx = indexOfDocument(document);
    if (!idx)
        return;
    const FilePath fixedPath = DocumentManager::filePathKey(document->filePath(),
                                                            DocumentManager::ResolveLinks);
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

DocumentModel::Entry *DocumentModelPrivate::addSuspendedDocument(const FilePath &filePath,
                                                                 const QString &displayName,
                                                                 Id id)
{
    QTC_CHECK(id.isValid());
    auto entry = new DocumentModel::Entry;
    entry->document = new IDocument;
    entry->document->setFilePath(filePath);
    if (!displayName.isEmpty())
        entry->document->setPreferredDisplayName(displayName);
    entry->document->setId(id);
    entry->isSuspended = true;
    return d->addEntry(entry);
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

        const FilePath fixedPath = DocumentManager::filePathKey(entry->filePath(),
                                                                DocumentManager::ResolveLinks);
        int row = i + 1/*<no document>*/;
        d->beginRemoveRows(QModelIndex(), row, row);
        delete d->m_entries.takeAt(i);
        d->endRemoveRows();

        if (!fixedPath.isEmpty())
            d->m_entryByFixedPath.remove(fixedPath);
    }
    QSet<QString> displayNames;
    for (DocumentModel::Entry *entry : std::as_const(d->m_entries)) {
        const QString displayName = entry->plainDisplayName();
        if (Utils::insert(displayNames, displayName))
            d->disambiguateDisplayNames(entry);
    }
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

Utils::FilePath DocumentModel::Entry::filePath() const
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
    for (IDocument *document : documents)
        result += d->m_editors.value(document);
    return result;
}

std::optional<int> DocumentModel::indexOfDocument(IDocument *document)
{
    return d->indexOfDocument(document);
}

std::optional<int> DocumentModel::indexOfFilePath(const Utils::FilePath &filePath)
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
    if (filePath.isEmpty())
        return nullptr;
    const FilePath fixedPath = DocumentManager::filePathKey(filePath, DocumentManager::ResolveLinks);
    return d->m_entryByFixedPath.value(fixedPath);
}

QList<IDocument *> DocumentModel::openedDocuments()
{
    return d->m_editors.keys();
}

IDocument *DocumentModel::documentForFilePath(const Utils::FilePath &filePath)
{
    const Entry *entry = entryForFilePath(filePath);
    return entry ? entry->document : nullptr;
}

QList<IEditor *> DocumentModel::editorsForFilePath(const Utils::FilePath &filePath)
{
    IDocument *document = documentForFilePath(filePath);
    if (document)
        return editorsForDocument(document);
    return {};
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

std::optional<int> DocumentModel::rowOfDocument(IDocument *document)
{
    if (!document)
        return 0 /*<no document>*/;
    const std::optional<int> index = indexOfDocument(document);
    if (index)
        return *index + 1/*correction for <no document>*/;
    return std::nullopt;
}

QList<DocumentModel::Entry *> DocumentModel::entries()
{
    return d->m_entries;
}

} // namespace Core
