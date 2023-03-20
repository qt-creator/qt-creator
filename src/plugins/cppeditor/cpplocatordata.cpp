// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cpplocatordata.h"

#include <utils/stringtable.h>

using namespace Utils;

namespace CppEditor {

enum { MaxPendingDocuments = 10 };

CppLocatorData::CppLocatorData()
{
    m_search.setSymbolsToSearchFor(SymbolSearcher::Enums |
                                   SymbolSearcher::Classes |
                                   SymbolSearcher::Functions |
                                   SymbolSearcher::TypeAliases);
    m_pendingDocuments.reserve(MaxPendingDocuments);
}

QList<IndexItem::Ptr> CppLocatorData::findSymbols(IndexItem::ItemType type,
                                                  const QString &symbolName) const
{
    QList<IndexItem::Ptr> matches;
    filterAllFiles([&](const IndexItem::Ptr &info) {
        if (info->type() & type) {
            if (info->symbolName() == symbolName || info->scopedSymbolName() == symbolName)
                matches << info;
        }
        if (info->type() & IndexItem::Enum)
            return IndexItem::Continue;
        return IndexItem::Recurse;
    });
    return matches;
}

void CppLocatorData::onDocumentUpdated(const CPlusPlus::Document::Ptr &document)
{
    QMutexLocker locker(&m_pendingDocumentsMutex);

    bool isPending = false;
    for (int i = 0, ei = m_pendingDocuments.size(); i < ei; ++i) {
        const CPlusPlus::Document::Ptr &doc = m_pendingDocuments.at(i);
        if (doc->filePath() == document->filePath()) {
            isPending = true;
            if (document->revision() >= doc->revision())
                m_pendingDocuments[i] = document;
            break;
        }
    }

    if (!isPending && document->filePath().suffix() != "moc")
        m_pendingDocuments.append(document);

    flushPendingDocument(false);
}

void CppLocatorData::onAboutToRemoveFiles(const QStringList &files)
{
    if (files.isEmpty())
        return;

    QMutexLocker locker(&m_pendingDocumentsMutex);

    for (const QString &file : files) {
        m_infosByFile.remove(file);

        for (int i = 0; i < m_pendingDocuments.size(); ++i) {
            if (m_pendingDocuments.at(i)->filePath().path() == file) {
                m_pendingDocuments.remove(i);
                break;
            }
        }
    }

    StringTable::scheduleGC();
    flushPendingDocument(false);
}

void CppLocatorData::flushPendingDocument(bool force) const
{
    // TODO: move this off the UI thread and into a future.
    if (!force && m_pendingDocuments.size() < MaxPendingDocuments)
        return;
    if (m_pendingDocuments.isEmpty())
        return;

    for (CPlusPlus::Document::Ptr doc : std::as_const(m_pendingDocuments))
        m_infosByFile.insert(StringTable::insert(doc->filePath().toString()), m_search(doc));

    m_pendingDocuments.clear();
    m_pendingDocuments.reserve(MaxPendingDocuments);
}

} // namespace CppEditor
