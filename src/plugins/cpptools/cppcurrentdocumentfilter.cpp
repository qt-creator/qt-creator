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

#include "cppcurrentdocumentfilter.h"

#include "cppmodelmanager.h"

#include <coreplugin/idocument.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>

#include <QStringMatcher>

using namespace CppTools::Internal;
using namespace CPlusPlus;

CppCurrentDocumentFilter::CppCurrentDocumentFilter(CppTools::CppModelManager *manager,
                                                   StringTable &stringTable)
    : m_modelManager(manager)
    , search(stringTable)
{
    setId("Methods in current Document");
    setDisplayName(tr("C++ Symbols in Current Document"));
    setShortcutString(QString(QLatin1Char('.')));
    setPriority(High);
    setIncludedByDefault(false);

    search.setSymbolsToSearchFor(SymbolSearcher::Declarations |
                                 SymbolSearcher::Enums |
                                 SymbolSearcher::Functions |
                                 SymbolSearcher::Classes);

    connect(manager, SIGNAL(documentUpdated(CPlusPlus::Document::Ptr)),
            this,    SLOT(onDocumentUpdated(CPlusPlus::Document::Ptr)));
    connect(Core::EditorManager::instance(), SIGNAL(currentEditorChanged(Core::IEditor*)),
            this,          SLOT(onCurrentEditorChanged(Core::IEditor*)));
    connect(Core::EditorManager::instance(), SIGNAL(editorAboutToClose(Core::IEditor*)),
            this,          SLOT(onEditorAboutToClose(Core::IEditor*)));
}

QList<Core::LocatorFilterEntry> CppCurrentDocumentFilter::matchesFor(
        QFutureInterface<Core::LocatorFilterEntry> &future, const QString & origEntry)
{
    QString entry = trimWildcards(origEntry);
    QList<Core::LocatorFilterEntry> goodEntries;
    QList<Core::LocatorFilterEntry> betterEntries;
    QStringMatcher matcher(entry, Qt::CaseInsensitive);
    const QChar asterisk = QLatin1Char('*');
    QRegExp regexp(asterisk + entry + asterisk, Qt::CaseInsensitive, QRegExp::Wildcard);
    if (!regexp.isValid())
        return goodEntries;
    bool hasWildcard = (entry.contains(asterisk) || entry.contains(QLatin1Char('?')));
    const Qt::CaseSensitivity caseSensitivityForPrefix = caseSensitivity(entry);

    foreach (IndexItem::Ptr info, itemsOfCurrentDocument()) {
        if (future.isCanceled())
            break;

        QString matchString = info->symbolName();
        if (info->type() == IndexItem::Declaration)
            matchString = info->representDeclaration();
        else if (info->type() == IndexItem::Function)
            matchString += info->symbolType();

        if ((hasWildcard && regexp.exactMatch(matchString))
            || (!hasWildcard && matcher.indexIn(matchString) != -1))
        {
            QVariant id = qVariantFromValue(info);
            QString name = matchString;
            QString extraInfo = info->symbolScope();
            if (info->type() == IndexItem::Function) {
                if (info->unqualifiedNameAndScope(matchString, &name, &extraInfo))
                    name += info->symbolType();
            }
            Core::LocatorFilterEntry filterEntry(this, name, id, info->icon());
            filterEntry.extraInfo = extraInfo;

            if (matchString.startsWith(entry, caseSensitivityForPrefix))
                betterEntries.append(filterEntry);
            else
                goodEntries.append(filterEntry);
        }
    }

    // entries are unsorted by design!

    betterEntries += goodEntries;
    return betterEntries;
}

void CppCurrentDocumentFilter::accept(Core::LocatorFilterEntry selection) const
{
    IndexItem::Ptr info = qvariant_cast<CppTools::IndexItem::Ptr>(selection.internalData);
    Core::EditorManager::openEditorAt(info->fileName(), info->line(), info->column());
}

void CppCurrentDocumentFilter::refresh(QFutureInterface<void> &future)
{
    Q_UNUSED(future)
}

void CppCurrentDocumentFilter::onDocumentUpdated(Document::Ptr doc)
{
    QMutexLocker locker(&m_mutex);
    if (m_currentFileName == doc->fileName())
        m_itemsOfCurrentDoc.clear();
}

void CppCurrentDocumentFilter::onCurrentEditorChanged(Core::IEditor *currentEditor)
{
    QMutexLocker locker(&m_mutex);
    if (currentEditor)
        m_currentFileName = currentEditor->document()->filePath().toString();
    else
        m_currentFileName.clear();
    m_itemsOfCurrentDoc.clear();
}

void CppCurrentDocumentFilter::onEditorAboutToClose(Core::IEditor *editorAboutToClose)
{
    if (!editorAboutToClose)
        return;

    QMutexLocker locker(&m_mutex);
    if (m_currentFileName == editorAboutToClose->document()->filePath().toString()) {
        m_currentFileName.clear();
        m_itemsOfCurrentDoc.clear();
    }
}

QList<CppTools::IndexItem::Ptr> CppCurrentDocumentFilter::itemsOfCurrentDocument()
{
    QMutexLocker locker(&m_mutex);

    if (m_currentFileName.isEmpty())
        return QList<CppTools::IndexItem::Ptr>();

    if (m_itemsOfCurrentDoc.isEmpty()) {
        const Snapshot snapshot = m_modelManager->snapshot();
        if (const Document::Ptr thisDocument = snapshot.document(m_currentFileName)) {
            IndexItem::Ptr rootNode = search(thisDocument);
            rootNode->visitAllChildren([&](const IndexItem::Ptr &info) -> IndexItem::VisitorResult {
                m_itemsOfCurrentDoc.append(info);
                return IndexItem::Recurse;
            });
        }
    }

    return m_itemsOfCurrentDoc;
}
