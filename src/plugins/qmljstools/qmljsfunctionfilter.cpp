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

#include "qmljsfunctionfilter.h"

#include "qmljslocatordata.h"

#include <coreplugin/editormanager/editormanager.h>
#include <utils/algorithm.h>

#include <QRegularExpression>

#include <numeric>

using namespace QmlJSTools::Internal;

Q_DECLARE_METATYPE(LocatorData::Entry)

FunctionFilter::FunctionFilter(LocatorData *data, QObject *parent)
    : Core::ILocatorFilter(parent)
    , m_data(data)
{
    setId("Functions");
    setDisplayName(tr("QML Functions"));
    setShortcutString("m");
    setIncludedByDefault(false);
}

FunctionFilter::~FunctionFilter() = default;

void FunctionFilter::refresh(QFutureInterface<void> &)
{
}

QList<Core::LocatorFilterEntry> FunctionFilter::matchesFor(
        QFutureInterface<Core::LocatorFilterEntry> &future,
        const QString &entry)
{
    QList<Core::LocatorFilterEntry> entries[int(MatchLevel::Count)];
    const Qt::CaseSensitivity caseSensitivityForPrefix = caseSensitivity(entry);

    const QRegularExpression regexp = createRegExp(entry);
    if (!regexp.isValid())
        return {};

    const QHash<QString, QList<LocatorData::Entry> > locatorEntries = m_data->entries();
    for (const QList<LocatorData::Entry> &items : locatorEntries) {
        if (future.isCanceled())
            break;

        for (const LocatorData::Entry &info : items) {
            if (info.type != LocatorData::Function)
                continue;

            const QRegularExpressionMatch match = regexp.match(info.symbolName);
            if (match.hasMatch()) {
                QVariant id = QVariant::fromValue(info);
                Core::LocatorFilterEntry filterEntry(this, info.displayName, id/*, info.icon*/);
                filterEntry.extraInfo = info.extraInfo;
                filterEntry.highlightInfo = highlightInfo(match);

                if (filterEntry.displayName.startsWith(entry, caseSensitivityForPrefix))
                    entries[int(MatchLevel::Best)].append(filterEntry);
                else if (filterEntry.displayName.contains(entry, caseSensitivityForPrefix))
                    entries[int(MatchLevel::Better)].append(filterEntry);
                else
                    entries[int(MatchLevel::Good)].append(filterEntry);
            }
        }
    }

    for (auto &entry : entries) {
        if (entry.size() < 1000)
            Utils::sort(entry, Core::LocatorFilterEntry::compareLexigraphically);
    }

    return std::accumulate(std::begin(entries), std::end(entries), QList<Core::LocatorFilterEntry>());
}

void FunctionFilter::accept(Core::LocatorFilterEntry selection,
                            QString *newText, int *selectionStart, int *selectionLength) const
{
    Q_UNUSED(newText)
    Q_UNUSED(selectionStart)
    Q_UNUSED(selectionLength)
    const LocatorData::Entry entry = qvariant_cast<LocatorData::Entry>(selection.internalData);
    Core::EditorManager::openEditorAt(entry.fileName, entry.line, entry.column);
}
