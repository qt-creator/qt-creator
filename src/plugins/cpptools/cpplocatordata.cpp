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

#include "cpplocatordata.h"
#include "cpptoolsplugin.h"

using namespace CppTools;
using namespace CppTools::Internal;

enum { MaxPendingDocuments = 10 };

CppLocatorData::CppLocatorData()
    : m_strings(&CppToolsPlugin::stringTable())
    , m_search(CppToolsPlugin::stringTable())
    , m_pendingDocumentsMutex(QMutex::Recursive)
{
    m_search.setSymbolsToSearchFor(SymbolSearcher::Enums |
                                   SymbolSearcher::Classes |
                                   SymbolSearcher::Functions);
    m_pendingDocuments.reserve(MaxPendingDocuments);
}

void CppLocatorData::onDocumentUpdated(const CPlusPlus::Document::Ptr &document)
{
    QMutexLocker locker(&m_pendingDocumentsMutex);

    int i = 0, ei = m_pendingDocuments.size();
    for (; i < ei; ++i) {
        const CPlusPlus::Document::Ptr &doc = m_pendingDocuments.at(i);
        if (doc->fileName() == document->fileName() && doc->revision() <= document->revision()) {
            m_pendingDocuments[i] = document;
            break;
        }
    }

    if (i == ei && QFileInfo(document->fileName()).suffix() != QLatin1String("moc"))
        m_pendingDocuments.append(document);

    flushPendingDocument(false);
}

void CppLocatorData::onAboutToRemoveFiles(const QStringList &files)
{
    if (files.isEmpty())
        return;

    QMutexLocker locker(&m_pendingDocumentsMutex);

    foreach (const QString &file, files) {
        m_infosByFile.remove(file);

        for (int i = 0; i < m_pendingDocuments.size(); ++i) {
            if (m_pendingDocuments.at(i)->fileName() == file) {
                m_pendingDocuments.remove(i);
                break;
            }
        }
    }

    m_strings->scheduleGC();
    flushPendingDocument(false);
}

void CppLocatorData::flushPendingDocument(bool force) const
{
    // TODO: move this off the UI thread and into a future.
    QMutexLocker locker(&m_pendingDocumentsMutex);
    if (!force && m_pendingDocuments.size() < MaxPendingDocuments)
        return;
    if (m_pendingDocuments.isEmpty())
        return;

    foreach (CPlusPlus::Document::Ptr doc, m_pendingDocuments)
        m_infosByFile.insert(findOrInsertFilePath(doc->fileName()), m_search(doc));

    m_pendingDocuments.clear();
    m_pendingDocuments.reserve(MaxPendingDocuments);
}

QList<IndexItem::Ptr> CppLocatorData::allIndexItems(
        const QHash<QString, QList<IndexItem::Ptr>> &items) const
{
    QList<IndexItem::Ptr> result;
    QHashIterator<QString, QList<IndexItem::Ptr> > it(items);
    while (it.hasNext()) {
        it.next();
        result.append(it.value());
    }
    return result;
}
