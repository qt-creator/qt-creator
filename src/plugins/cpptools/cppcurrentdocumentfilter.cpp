/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/
#include "cppcurrentdocumentfilter.h"
#include "cppmodelmanager.h"

#include <coreplugin/editormanager/editormanager.h>
#include <cplusplus/CppDocument.h>

#include <QtCore/QStringMatcher>

using namespace CppTools::Internal;
using namespace CPlusPlus;

CppCurrentDocumentFilter::CppCurrentDocumentFilter(CppModelManager *manager, Core::EditorManager *editorManager)
    : m_modelManager(manager)
{
    setShortcutString(".");
    setIncludedByDefault(false);

    search.setSymbolsToSearchFor(SearchSymbols::Declarations |
                                 SearchSymbols::Enums |
                                 SearchSymbols::Functions |
                                 SearchSymbols::Classes);

    search.setSeparateScope(true);

    connect(manager, SIGNAL(documentUpdated(CPlusPlus::Document::Ptr)),
            this,    SLOT(onDocumentUpdated(CPlusPlus::Document::Ptr)));
    connect(editorManager, SIGNAL(currentEditorChanged(Core::IEditor*)),
            this,          SLOT(onCurrentEditorChanged(Core::IEditor*)));
    connect(editorManager, SIGNAL(editorAboutToClose(Core::IEditor*)),
            this,          SLOT(onEditorAboutToClose(Core::IEditor*)));
}

QList<Locator::FilterEntry> CppCurrentDocumentFilter::matchesFor(const QString & origEntry)
{
    QString entry = trimWildcards(origEntry);
    QList<Locator::FilterEntry> goodEntries;
    QList<Locator::FilterEntry> betterEntries;
    QStringMatcher matcher(entry, Qt::CaseInsensitive);
    const QRegExp regexp("*"+entry+"*", Qt::CaseInsensitive, QRegExp::Wildcard);
    if (!regexp.isValid())
        return goodEntries;
    bool hasWildcard = (entry.contains('*') || entry.contains('?'));

    if (m_currentFileName.isEmpty())
        return goodEntries;

    if (m_itemsOfCurrentDoc.isEmpty()) {
        Snapshot snapshot = m_modelManager->snapshot();
        Document::Ptr thisDocument = snapshot.value(m_currentFileName);
        if (thisDocument)
            m_itemsOfCurrentDoc = search(thisDocument);
    }

    foreach (const ModelItemInfo & info, m_itemsOfCurrentDoc)
    {
        if ((hasWildcard && regexp.exactMatch(info.symbolName))
            || (!hasWildcard && matcher.indexIn(info.symbolName) != -1))
        {
            QString symbolName = info.symbolName;// + (info.type == ModelItemInfo::Declaration ? ";" : " {...}");
            QVariant id = qVariantFromValue(info);
            Locator::FilterEntry filterEntry(this, symbolName, id, info.icon);
            filterEntry.extraInfo = info.symbolType;

            if (info.symbolName.startsWith(entry))
                betterEntries.append(filterEntry);
            else
                goodEntries.append(filterEntry);
        }
    }

    // entries are unsorted by design!

    betterEntries += goodEntries;
    return betterEntries;
}

void CppCurrentDocumentFilter::accept(Locator::FilterEntry selection) const
{
    ModelItemInfo info = qvariant_cast<CppTools::Internal::ModelItemInfo>(selection.internalData);
    TextEditor::BaseTextEditor::openEditorAt(info.fileName, info.line);
}

void CppCurrentDocumentFilter::refresh(QFutureInterface<void> &future)
{
    Q_UNUSED(future)
}

void CppCurrentDocumentFilter::onDocumentUpdated(Document::Ptr doc)
{
    if (m_currentFileName == doc->fileName()) {
        m_itemsOfCurrentDoc.clear();
    }
}

void CppCurrentDocumentFilter::onCurrentEditorChanged(Core::IEditor * currentEditor)
{
    if (currentEditor) {
        m_currentFileName = currentEditor->file()->fileName();
    } else {
        m_currentFileName.clear();
    }
    m_itemsOfCurrentDoc.clear();
}

void CppCurrentDocumentFilter::onEditorAboutToClose(Core::IEditor * editorAboutToClose)
{
    if (!editorAboutToClose) return;
    if (m_currentFileName == editorAboutToClose->file()->fileName()) {
        m_currentFileName.clear();
        m_itemsOfCurrentDoc.clear();
    }
}
