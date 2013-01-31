/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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
#include "cppcurrentdocumentfilter.h"
#include "cppmodelmanager.h"

#include <coreplugin/editormanager/editormanager.h>
#include <cplusplus/CppDocument.h>

#include <QStringMatcher>

using namespace CppTools::Internal;
using namespace CPlusPlus;

CppCurrentDocumentFilter::CppCurrentDocumentFilter(CppModelManager *manager, Core::EditorManager *editorManager)
    : m_modelManager(manager)
{
    setId("Methods in current Document");
    setDisplayName(tr("C++ Methods in Current Document"));
    setShortcutString(QString(QLatin1Char('.')));
    setIncludedByDefault(false);

    search.setSymbolsToSearchFor(SymbolSearcher::Declarations |
                                 SymbolSearcher::Enums |
                                 SymbolSearcher::Functions |
                                 SymbolSearcher::Classes);

    search.setSeparateScope(true);

    connect(manager, SIGNAL(documentUpdated(CPlusPlus::Document::Ptr)),
            this,    SLOT(onDocumentUpdated(CPlusPlus::Document::Ptr)));
    connect(editorManager, SIGNAL(currentEditorChanged(Core::IEditor*)),
            this,          SLOT(onCurrentEditorChanged(Core::IEditor*)));
    connect(editorManager, SIGNAL(editorAboutToClose(Core::IEditor*)),
            this,          SLOT(onEditorAboutToClose(Core::IEditor*)));
}

QList<Locator::FilterEntry> CppCurrentDocumentFilter::matchesFor(QFutureInterface<Locator::FilterEntry> &future, const QString & origEntry)
{
    QString entry = trimWildcards(origEntry);
    QList<Locator::FilterEntry> goodEntries;
    QList<Locator::FilterEntry> betterEntries;
    QStringMatcher matcher(entry, Qt::CaseInsensitive);
    const QChar asterisk = QLatin1Char('*');
    QRegExp regexp(asterisk + entry + asterisk, Qt::CaseInsensitive, QRegExp::Wildcard);
    if (!regexp.isValid())
        return goodEntries;
    bool hasWildcard = (entry.contains(asterisk) || entry.contains(QLatin1Char('?')));

    if (m_currentFileName.isEmpty())
        return goodEntries;

    if (m_itemsOfCurrentDoc.isEmpty()) {
        Snapshot snapshot = m_modelManager->snapshot();
        Document::Ptr thisDocument = snapshot.document(m_currentFileName);
        if (thisDocument)
            m_itemsOfCurrentDoc = search(thisDocument);
    }

    foreach (const ModelItemInfo & info, m_itemsOfCurrentDoc)
    {
        if (future.isCanceled())
            break;

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
    ModelItemInfo info = qvariant_cast<CppTools::ModelItemInfo>(selection.internalData);
    TextEditor::BaseTextEditorWidget::openEditorAt(info.fileName, info.line, info.column,
                                             Core::Id(), Core::EditorManager::ModeSwitch);
}

void CppCurrentDocumentFilter::refresh(QFutureInterface<void> &future)
{
    Q_UNUSED(future)
}

void CppCurrentDocumentFilter::onDocumentUpdated(Document::Ptr doc)
{
    if (m_currentFileName == doc->fileName())
        m_itemsOfCurrentDoc.clear();
}

void CppCurrentDocumentFilter::onCurrentEditorChanged(Core::IEditor * currentEditor)
{
    if (currentEditor)
        m_currentFileName = currentEditor->document()->fileName();
    else
        m_currentFileName.clear();
    m_itemsOfCurrentDoc.clear();
}

void CppCurrentDocumentFilter::onEditorAboutToClose(Core::IEditor * editorAboutToClose)
{
    if (!editorAboutToClose) return;
    if (m_currentFileName == editorAboutToClose->document()->fileName()) {
        m_currentFileName.clear();
        m_itemsOfCurrentDoc.clear();
    }
}
