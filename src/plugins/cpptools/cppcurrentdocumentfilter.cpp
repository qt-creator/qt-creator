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

#include "cppcurrentdocumentfilter.h"

#include "cppmodelmanager.h"

#include <QStringMatcher>

using namespace CppTools::Internal;
using namespace CPlusPlus;

CppCurrentDocumentFilter::CppCurrentDocumentFilter(CppModelManager *manager,
                                                   StringTable &stringTable)
    : m_modelManager(manager)
    , search(stringTable)
{
    setId("Methods in current Document");
    setDisplayName(tr("C++ Symbols in Current Document"));
    setShortcutString(QString(QLatin1Char('.')));
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

    if (m_currentFileName.isEmpty())
        return goodEntries;

    if (m_itemsOfCurrentDoc.isEmpty()) {
        Snapshot snapshot = m_modelManager->snapshot();
        Document::Ptr thisDocument = snapshot.document(m_currentFileName);
        if (thisDocument)
            m_itemsOfCurrentDoc = search(thisDocument);
    }

    const Qt::CaseSensitivity caseSensitivityForPrefix = caseSensitivity(entry);

    foreach (ModelItemInfo::Ptr info, m_itemsOfCurrentDoc) {
        if (future.isCanceled())
            break;

        QString matchString = info->symbolName();
        if (info->type() == ModelItemInfo::Declaration)
            matchString = ModelItemInfo::representDeclaration(info->symbolName(),
                                                              info->symbolType());
        else if (info->type() == ModelItemInfo::Function)
            matchString += info->symbolType();

        if ((hasWildcard && regexp.exactMatch(matchString))
            || (!hasWildcard && matcher.indexIn(matchString) != -1))
        {
            QVariant id = qVariantFromValue(info);
            QString name = matchString;
            QString extraInfo = info->symbolScope();
            if (info->type() == ModelItemInfo::Function) {
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
    ModelItemInfo::Ptr info = qvariant_cast<CppTools::ModelItemInfo::Ptr>(selection.internalData);
    Core::EditorManager::openEditorAt(info->fileName(), info->line(), info->column());
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
        m_currentFileName = currentEditor->document()->filePath();
    else
        m_currentFileName.clear();
    m_itemsOfCurrentDoc.clear();
}

void CppCurrentDocumentFilter::onEditorAboutToClose(Core::IEditor * editorAboutToClose)
{
    if (!editorAboutToClose) return;
    if (m_currentFileName == editorAboutToClose->document()->filePath()) {
        m_currentFileName.clear();
        m_itemsOfCurrentDoc.clear();
    }
}
