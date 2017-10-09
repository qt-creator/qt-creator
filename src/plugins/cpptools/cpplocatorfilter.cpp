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

#include "cpplocatorfilter.h"
#include "cppmodelmanager.h"

#include <coreplugin/editormanager/editormanager.h>
#include <utils/algorithm.h>

#include <QRegularExpression>

#include <algorithm>

using namespace CppTools;
using namespace CppTools::Internal;

CppLocatorFilter::CppLocatorFilter(CppLocatorData *locatorData)
    : m_data(locatorData)
{
    setId("Classes and Methods");
    setDisplayName(tr("C++ Classes, Enums and Functions"));
    setShortcutString(QString(QLatin1Char(':')));
    setIncludedByDefault(false);
}

CppLocatorFilter::~CppLocatorFilter()
{
}

Core::LocatorFilterEntry CppLocatorFilter::filterEntryFromIndexItem(IndexItem::Ptr info)
{
    const QVariant id = qVariantFromValue(info);
    Core::LocatorFilterEntry filterEntry(this, info->scopedSymbolName(), id, info->icon());
    if (info->type() == IndexItem::Class || info->type() == IndexItem::Enum)
        filterEntry.extraInfo = info->shortNativeFilePath();
    else
        filterEntry.extraInfo = info->symbolType();

    return filterEntry;
}

void CppLocatorFilter::refresh(QFutureInterface<void> &future)
{
    Q_UNUSED(future)
}

QList<Core::LocatorFilterEntry> CppLocatorFilter::matchesFor(
        QFutureInterface<Core::LocatorFilterEntry> &future, const QString &entry)
{
    QList<Core::LocatorFilterEntry> goodEntries;
    QList<Core::LocatorFilterEntry> betterEntries;
    QList<Core::LocatorFilterEntry> bestEntries;
    const Qt::CaseSensitivity caseSensitivityForPrefix = caseSensitivity(entry);
    bool hasColonColon = entry.contains(QLatin1String("::"));
    const IndexItem::ItemType wanted = matchTypes();

    const QRegularExpression regexp = createRegExp(entry);
    if (!regexp.isValid())
        return goodEntries;

    m_data->filterAllFiles([&](const IndexItem::Ptr &info) -> IndexItem::VisitorResult {
        if (future.isCanceled())
            return IndexItem::Break;
        if (info->type() & wanted) {
            QString matchString = hasColonColon ? info->scopedSymbolName() : info->symbolName();
            QRegularExpressionMatch match = regexp.match(matchString);
            if (match.hasMatch()) {
                Core::LocatorFilterEntry filterEntry = filterEntryFromIndexItem(info);

                // Highlight the matched characters, therefore it may be necessary
                // to update the match if the displayName is different from matchString
                if (matchString != filterEntry.displayName)
                    match = regexp.match(filterEntry.displayName);
                filterEntry.highlightInfo = highlightInfo(match);

                if (matchString.startsWith(entry, caseSensitivityForPrefix))
                    bestEntries.append(filterEntry);
                else if (matchString.contains(entry, caseSensitivityForPrefix))
                    betterEntries.append(filterEntry);
                else
                    goodEntries.append(filterEntry);
            }
        }

        if (info->type() & IndexItem::Enum)
            return IndexItem::Continue;
        else
            return IndexItem::Recurse;
    });

    if (goodEntries.size() < 1000)
        Utils::sort(goodEntries, Core::LocatorFilterEntry::compareLexigraphically);
    if (betterEntries.size() < 1000)
        Utils::sort(betterEntries, Core::LocatorFilterEntry::compareLexigraphically);
    if (bestEntries.size() < 1000)
        Utils::sort(bestEntries, Core::LocatorFilterEntry::compareLexigraphically);

    bestEntries += betterEntries;
    bestEntries += goodEntries;
    return bestEntries;
}

void CppLocatorFilter::accept(Core::LocatorFilterEntry selection,
                              QString *newText, int *selectionStart, int *selectionLength) const
{
    Q_UNUSED(newText)
    Q_UNUSED(selectionStart)
    Q_UNUSED(selectionLength)
    IndexItem::Ptr info = qvariant_cast<IndexItem::Ptr>(selection.internalData);
    Core::EditorManager::openEditorAt(info->fileName(), info->line(), info->column());
}
