// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppcurrentdocumentfilter.h"

#include "cppeditorconstants.h"
#include "cppeditortr.h"
#include "cppmodelmanager.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <utils/algorithm.h>

#include <QHash>
#include <QRegularExpression>

using namespace Core;
using namespace CPlusPlus;

namespace CppEditor::Internal {

CppCurrentDocumentFilter::CppCurrentDocumentFilter()
    : m_modelManager(CppModelManager::instance())
{
    setId(Constants::CURRENT_DOCUMENT_FILTER_ID);
    setDisplayName(Tr::tr(Constants::CURRENT_DOCUMENT_FILTER_DISPLAY_NAME));
    setDescription(Tr::tr(Constants::CURRENT_DOCUMENT_FILTER_DESCRIPTION));
    setDefaultShortcutString(".");
    setPriority(High);
    setDefaultIncludedByDefault(false);

    search.setSymbolsToSearchFor(SymbolSearcher::Declarations |
                                 SymbolSearcher::Enums |
                                 SymbolSearcher::Functions |
                                 SymbolSearcher::Classes);

    connect(m_modelManager, &CppModelManager::documentUpdated,
            this, &CppCurrentDocumentFilter::onDocumentUpdated);
    connect(EditorManager::instance(), &EditorManager::currentEditorChanged,
            this, &CppCurrentDocumentFilter::onCurrentEditorChanged);
    connect(EditorManager::instance(), &EditorManager::editorAboutToClose,
            this, &CppCurrentDocumentFilter::onEditorAboutToClose);
}

void CppCurrentDocumentFilter::makeAuxiliary()
{
    setId({});
    setDisplayName({});
    setDefaultShortcutString({});
    setEnabled(false);
    setHidden(true);
}

QList<LocatorFilterEntry> CppCurrentDocumentFilter::matchesFor(
        QFutureInterface<LocatorFilterEntry> &future, const QString &entry)
{
    const QRegularExpression regexp = createRegExp(entry);
    if (!regexp.isValid())
        return {};

    struct Entry
    {
        LocatorFilterEntry entry;
        IndexItem::Ptr info;
    };
    QList<Entry> goodEntries;
    QList<Entry> betterEntries;
    const QList<IndexItem::Ptr> items = itemsOfCurrentDocument();
    for (const IndexItem::Ptr &info : items) {
        if (future.isCanceled())
            break;

        QString matchString = info->symbolName();
        if (info->type() == IndexItem::Declaration)
            matchString = info->representDeclaration();
        else if (info->type() == IndexItem::Function)
            matchString += info->symbolType();

        QRegularExpressionMatch match = regexp.match(matchString);
        if (match.hasMatch()) {
            const bool betterMatch = match.capturedStart() == 0;
            QString name = matchString;
            QString extraInfo = info->symbolScope();
            if (info->type() == IndexItem::Function) {
                if (info->unqualifiedNameAndScope(matchString, &name, &extraInfo)) {
                    name += info->symbolType();
                    match = regexp.match(name);
                }
            }

            LocatorFilterEntry filterEntry;
            filterEntry.displayName = name;
            filterEntry.displayIcon = info->icon();
            filterEntry.linkForEditor = {info->filePath(), info->line(), info->column()};
            filterEntry.extraInfo = extraInfo;
            if (match.hasMatch()) {
                filterEntry.highlightInfo = highlightInfo(match);
            } else {
                match = regexp.match(extraInfo);
                filterEntry.highlightInfo =
                        highlightInfo(match, LocatorFilterEntry::HighlightInfo::ExtraInfo);
            }

            if (betterMatch)
                betterEntries.append({filterEntry, info});
            else
                goodEntries.append({filterEntry, info});
        }
    }

    // entries are unsorted by design!
    betterEntries += goodEntries;

    QHash<QString, QList<Entry>> possibleDuplicates;
    for (const Entry &e : std::as_const(betterEntries))
        possibleDuplicates[e.info->scopedSymbolName() + e.info->symbolType()] << e;
    for (auto it = possibleDuplicates.cbegin(); it != possibleDuplicates.cend(); ++it) {
        const QList<Entry> &duplicates = it.value();
        if (duplicates.size() == 1)
            continue;
        QList<Entry> declarations;
        QList<Entry> definitions;
        for (const Entry &candidate : duplicates) {
            const IndexItem::Ptr info = candidate.info;
            if (info->type() != IndexItem::Function)
                break;
            if (info->isFunctionDefinition())
                definitions << candidate;
            else
                declarations << candidate;
        }
        if (definitions.size() == 1
            && declarations.size() + definitions.size() == duplicates.size()) {
            for (const Entry &decl : std::as_const(declarations)) {
                Utils::erase(betterEntries, [&decl](const Entry &e) {
                    return e.info == decl.info;
                });
            }
        }
    }
    return Utils::transform(betterEntries, [](const Entry &entry) { return entry.entry; });
}

void CppCurrentDocumentFilter::onDocumentUpdated(Document::Ptr doc)
{
    QMutexLocker locker(&m_mutex);
    if (m_currentFileName == doc->filePath())
        m_itemsOfCurrentDoc.clear();
}

void CppCurrentDocumentFilter::onCurrentEditorChanged(IEditor *currentEditor)
{
    QMutexLocker locker(&m_mutex);
    if (currentEditor)
        m_currentFileName = currentEditor->document()->filePath();
    else
        m_currentFileName.clear();
    m_itemsOfCurrentDoc.clear();
}

void CppCurrentDocumentFilter::onEditorAboutToClose(IEditor *editorAboutToClose)
{
    if (!editorAboutToClose)
        return;

    QMutexLocker locker(&m_mutex);
    if (m_currentFileName == editorAboutToClose->document()->filePath()) {
        m_currentFileName.clear();
        m_itemsOfCurrentDoc.clear();
    }
}

QList<IndexItem::Ptr> CppCurrentDocumentFilter::itemsOfCurrentDocument()
{
    QMutexLocker locker(&m_mutex);

    if (m_currentFileName.isEmpty())
        return QList<IndexItem::Ptr>();

    if (m_itemsOfCurrentDoc.isEmpty()) {
        const Snapshot snapshot = m_modelManager->snapshot();
        if (const Document::Ptr thisDocument = snapshot.document(m_currentFileName)) {
            IndexItem::Ptr rootNode = search(thisDocument);
            rootNode->visitAllChildren([&](const IndexItem::Ptr &info) {
                m_itemsOfCurrentDoc.append(info);
                return IndexItem::Recurse;
            });
        }
    }

    return m_itemsOfCurrentDoc;
}

} // namespace CppEditor::Internal
